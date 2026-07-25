// ============================================================
// MMU.CPP - SECCIÓN CORREGIDA PARA JOYPAD
// ============================================================
// Reemplaza la función readMemory en tu mmu.cpp

#include "mmu.h"
#include "../APU/apu.h"  // APU para registros de audio
#include <iostream>
#include <iomanip>

// --- Constructor ---
mmu::mmu(const std::string& romPath) : cart(romPath) 
{
    VRAM.fill(0);
    WRAM.fill(0);
    HRAM.fill(0);
    IO.fill(0);
    OAM.fill(0);
    BG_PAL.fill(0xFF);  // En hardware la palette RAM arranca sin inicializar (≈blanco)
    OBJ_PAL.fill(0xFF);
    IE = 0;

    // Modo CGB si el cartucho lo declara en el header (0x143)
    cgb_mode = (cart.getCGBFlag() & 0x80) != 0;
    if (cgb_mode)
        std::cout << "[MMU] Cartucho CGB detectado. Modo Game Boy Color activado.\n";

    // Estado de registros I/O que deja el boot ROM de la DMG (Pan Docs,
    // "Power Up Sequence"). Sin esto los juegos que asumen LCD encendida
    // (LCDC=0x91) se quedan esperando LY==0x91 para siempre → pantalla negra.
    IO[0x00] = 0xCF; // P1/JOYP
    IO[0x02] = 0x7E; // SC
    IO[0x04] = 0xAB; // DIV
    IO[0x07] = 0xF8; // TAC
    IO[0x0F] = 0xE1; // IF - Interrupt Flag
    IO[0x10] = 0x80; // NR10
    IO[0x11] = 0xBF; // NR11
    IO[0x12] = 0xF3; // NR12
    IO[0x13] = 0xFF; // NR13
    IO[0x14] = 0xBF; // NR14
    IO[0x16] = 0x3F; // NR21
    IO[0x18] = 0xFF; // NR23
    IO[0x19] = 0xBF; // NR24
    IO[0x1A] = 0x7F; // NR30
    IO[0x1B] = 0xFF; // NR31
    IO[0x1C] = 0x9F; // NR32
    IO[0x1D] = 0xFF; // NR33
    IO[0x1E] = 0xBF; // NR34
    IO[0x20] = 0xFF; // NR41
    IO[0x23] = 0xBF; // NR44
    IO[0x24] = 0x77; // NR50
    IO[0x25] = 0xF3; // NR51
    IO[0x26] = 0xF1; // NR52
    IO[0x40] = 0x91; // LCDC: LCD encendida, BG activado
    IO[0x41] = 0x85; // STAT
    IO[0x46] = 0xFF; // DMA
    IO[0x47] = 0xFC; // BGP
    IO[0x48] = 0xFF; // OBP0
    IO[0x49] = 0xFF; // OBP1
    
    std::cout << "MMU Inicializada. Cartucho conectado: " << romPath << "\n";
}

// --- Helper: Calcular Offset ---
uint16_t mmu::offSet(uint16_t address, uint16_t base)
{
    return address - base;
}

// --- Helper: VRAM DMA de CGB (HDMA/GDMA) ---
// Simplificación: tanto la transferencia general (bit 7 = 0) como la de
// HBlank (bit 7 = 1) se ejecutan completas de inmediato. FF55 lee 0xFF
// (transferencia terminada), así que los juegos que sondean ven "listo".
void mmu::doHDMATransfer(uint8_t value)
{
    uint16_t src = ((IO[0x51] << 8) | IO[0x52]) & 0xFFF0;
    uint16_t dst = (((IO[0x53] << 8) | IO[0x54]) & 0x1FF0);
    int      len = ((value & 0x7F) + 1) * 0x10;

    for (int i = 0; i < len; i++) {
        uint32_t vram_off = static_cast<uint32_t>(vram_bank) * 0x2000u
                          + ((dst + i) & 0x1FFF);
        VRAM[vram_off] = readMemory(src + i);
    }

    // Dejar los registros de origen/destino apuntando al final (comportamiento real)
    uint16_t new_src = src + len;
    uint16_t new_dst = dst + len;
    IO[0x51] = new_src >> 8;
    IO[0x52] = new_src & 0xF0;
    IO[0x53] = (new_dst >> 8) & 0x1F;
    IO[0x54] = new_dst & 0xF0;
}

// --- Helper: DMA Transfer ---
void mmu::DMA(uint8_t value) 
{
    uint16_t base = value << 8;
    
    // DEBUG: Log DMA transfers
    static int dma_count = 0;
    dma_count++;
    if (dma_count <= 10 || dma_count == 100 || dma_count == 500) {
        std::cout << "[DMA #" << dma_count << "] Transfer from 0x" 
                  << std::hex << base << std::dec << " to OAM\n";
        
        // Sample source data before copy
        std::cout << "  Source first 8 bytes: ";
        for (int i = 0; i < 8; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << (int)readMemory(base + i) << " ";
        }
        std::cout << std::dec << "\n";
    }
    
    for (size_t i = 0; i < OAM.size(); i++) 
    {
        OAM[i] = readMemory(base + static_cast<uint16_t>(i));
    }
    
    // After copy, check if any sprites are non-zero
    if (dma_count <= 10) {
        int non_zero = 0;
        for (size_t i = 0; i < OAM.size(); i += 4) {
            if (OAM[i] != 0 || OAM[i+1] != 0) non_zero++;
        }
        std::cout << "  After DMA: " << non_zero << "/40 non-zero sprites in OAM\n";
    }
}

// ============================================================
// READ MEMORY - CORREGIDA CON JOYPAD ARREGLADO
// ============================================================
uint8_t mmu::readMemory(uint16_t address) 
{   
    // Interrupciones (Registro IF e IE)
    // IF usa IO[0x0F] directamente - bits 5-7 siempre retornan 1
    if (address == 0xFF0F) {
        uint8_t result = IO[0x0F] | 0xE0;
        // DEBUG: Log cuando IF tiene VBlank activo
        static int if_read_vblank_count = 0;
        if ((IO[0x0F] & 0x01) && if_read_vblank_count < 20) {
            if_read_vblank_count++;
            std::cout << "[MMU] IF read while VBlank active! Returning 0x" 
                      << std::hex << (int)result << std::dec << "\n";
        }
        return result;
    }
    if (address == 0xFFFF) return IE;
    
    // ROM (Cartucho)
    if (address <= 0x7FFF) {   
        return cart.readCartridge(address);
    }
    // VRAM
    else if (address >= 0x8000 && address <= 0x9FFF) {
        // TODO: Verificar si PPU está en modo 3 (Drawing)
        // Durante modo 3, VRAM no es accesible
        return VRAM[vramIndex(address)];
    }
    // RAM Externa (Cartucho)
    else if (address >= 0xA000 && address <= 0xBFFF) {
        return cart.readCartridge(address);
    }
    // WRAM (Work RAM)
    else if (address >= 0xC000 && address <= 0xDFFF) {
        return WRAM[wramIndex(address)];
    }
    // ECHO RAM (Espejo de WRAM)
    else if (address >= 0xE000 && address <= 0xFDFF) {
        return WRAM[wramIndex(address)];
    }
    // OAM (Object Attribute Memory)
    else if (address >= 0xFE00 && address <= 0xFE9F) {
        // TODO: Verificar si PPU está en modo 2 o 3
        // Durante modos 2 y 3, OAM no es accesible
        return OAM[offSet(address, 0xFE00)];
    }
    // Zona Prohibida (Unusable)
    else if (address >= 0xFEA0 && address <= 0xFEFF) {
        return 0xFF;
    }
    
    // --- I/O Registers (0xFF00 - 0xFF7F) ---
    else if (address >= 0xFF00 && address <= 0xFF7F) {
        
        // ============================================================
        // HARD HOOK: LY (0xFF44) - DEBUG CRÍTICO
        // ============================================================
        if (address == 0xFF44) {
            uint8_t val = IO[0x44];
            
            // DEBUG: Log para confirmar qué ve la CPU
            static int ly_read_count = 0;
            static uint8_t last_ly = 0xFF;
            ly_read_count++;
            
            // Log cuando LY cambia o primeras lecturas
            if (val != last_ly || ly_read_count <= 10) {
                if (ly_read_count <= 50 || val == 144) {
                    std::cout << "[MMU DEBUG] Read LY (FF44) returned: " << std::dec << (int)val
                              << " (read #" << ly_read_count << ")\n";
                }
                last_ly = val;
            }
            
            // Log especial cuando LY llega a 144
            static bool logged_144 = false;
            if (val == 144 && !logged_144) {
                std::cout << "[MMU DEBUG] *** LY=144 VISIBLE TO CPU! ***\n";
                logged_144 = true;
            }
            
            return val;
        }
        
        // ============================================================
        // JOYPAD (0xFF00) - ACTIVE LOW MATRIX LOGIC
        // ============================================================
        if (address == 0xFF00) {
            uint8_t p1 = IO[0x00];
            
            // Bits 6-7 siempre en 1 (no se usan)
            uint8_t result = 0xC0;
            
            // Mantener bits 4-5 (selección de grupo)
            result |= (p1 & 0x30);
            
            // Por defecto, todos los botones NO presionados (bits en 1 = Active Low)
            uint8_t button_state = 0x0F;
            
            bool select_dpad = !(p1 & 0x10);    // Bit 4 = 0 → D-Pad seleccionado
            bool select_buttons = !(p1 & 0x20); // Bit 5 = 0 → Botones seleccionados
            
            // Matriz de botones (Active Low: 0 = presionado)
            if (select_dpad) {
                if (button_right)  button_state &= ~0x01;
                if (button_left)   button_state &= ~0x02;
                if (button_up)     button_state &= ~0x04;
                if (button_down)   button_state &= ~0x08;
            }
            
            if (select_buttons) {
                if (button_a)      button_state &= ~0x01;
                if (button_b)      button_state &= ~0x02;
                if (button_select) button_state &= ~0x04;
                if (button_start)  button_state &= ~0x08;
            }
            
            result |= button_state;
            
            // DEBUG: Log cuando hay botones presionados (especially START)
            static int joypad_read_count = 0;
            static int start_detected_count = 0;
            joypad_read_count++;
            
            // Log if START is pressed and buttons are being read
            if (button_start && select_buttons) {
                start_detected_count++;
                if (start_detected_count <= 20) {
                    std::cout << "[JOYPAD START!] Read #" << start_detected_count
                              << " P1=0x" << std::hex << (int)result
                              << " btn_state=0x" << (int)button_state
                              << " (START should clear bit 3)" << std::dec << "\n";
                }
            }
            
            // Also log if START is pressed but wrong button group selected
            if (button_start && !select_buttons && joypad_read_count <= 200) {
                std::cout << "[JOYPAD MISS] START pressed but dpad selected, result=0x" 
                          << std::hex << (int)result << std::dec << "\n";
            }
            
            if (button_state != 0x0F && joypad_read_count <= 50) {
                std::cout << "[JOYPAD] Read P1=0x" << std::hex << (int)result
                          << " buttons=" << (int)button_state
                          << " sel_dpad=" << select_dpad
                          << " sel_btn=" << select_buttons
                          << std::dec << "\n";
            }
            
            // Log periódico para ver si el juego lee joypad
            if (joypad_read_count == 1000) {
                std::cout << "[JOYPAD] 1000 reads so far. Game IS polling joypad.\n";
            }
            
            return result;
        }
        
        // ============================================================
        // REGISTROS DE AUDIO (APU) - $FF10-$FF3F
        // ============================================================
        if (address >= 0xFF10 && address <= 0xFF3F) {
            if (apu) {
                return apu->readByte(address);
            }
            // Si no hay APU conectada, retornar valor por defecto
            return 0xFF;
        }
        
        // Registros CGB (solo existen en modo Game Boy Color)
        if (cgb_mode) {
            switch (address) {
                case 0xFF4D: // KEY1: velocidad actual + switch armado
                    return (double_speed ? 0x80 : 0x00)
                         | (speed_switch_armed ? 0x01 : 0x00) | 0x7E;
                case 0xFF4F: // VBK: banco de VRAM
                    return 0xFE | vram_bank;
                case 0xFF70: // SVBK: banco de WRAM
                    return 0xF8 | wram_bank;
                case 0xFF68: return bcps | 0x40;
                case 0xFF69: return BG_PAL[bcps & 0x3F];
                case 0xFF6A: return ocps | 0x40;
                case 0xFF6B: return OBJ_PAL[ocps & 0x3F];
                case 0xFF55: // HDMA5: las transferencias se hacen al instante
                    return 0xFF;
            }
        }

        // Registros inexistentes en DMG (huecos y rango CGB: KEY1, VBK,
        // HDMA, paletas CGB, SVBK...). En hardware real leen 0xFF; devolver
        // 0x00 hace que los juegos crean estar en una CGB (p.ej. la rutina
        // de doble velocidad de Blargg ejecuta STOP y congela todo).
        if (address == 0xFF03 ||
            (address >= 0xFF08 && address <= 0xFF0E) ||
            (address >= 0xFF4C && address <= 0xFF7F)) {
            return 0xFF;
        }

        // Otros registros I/O
        return IO[offSet(address, 0xFF00)];
    }

    // HRAM (High RAM)
    else if (address >= 0xFF80 && address <= 0xFFFE) {
        // DEBUG: Track reads from 0xFF85 (heavily polled in Tetris)
        if (address == 0xFF85) {
            static int ff85_read_count = 0;
            ff85_read_count++;
            if (ff85_read_count <= 10 || ff85_read_count == 1000 || ff85_read_count == 10000) {
                std::cout << "[HRAM 0xFF85] Read #" << ff85_read_count 
                          << " value=0x" << std::hex << (int)HRAM[0x05] << std::dec << "\n";
            }
        }
        return HRAM[offSet(address, 0xFF80)];
    }
    
    std::cout << "Memory Read Error: Address not mapped " << std::hex << address << "\n";
    return 0xFF;
}

// ============================================================
// WRITE MEMORY - CORREGIDA
// ============================================================
void mmu::writeMemory(uint16_t address, uint8_t value) 
{
    // Interrupciones y DMA (Interceptar antes)
    if (address == 0xFF46) { DMA(value); return; }
    // IF usa IO[0x0F] directamente - SEMÁNTICA ESPECIAL
    // En Game Boy real:
    // - Hardware (PPU/Timer) PONE bits usando OR
    // - Software LIMPIA bits escribiendo 1s (acknowledge)
    // Pero muchos juegos simplemente sobrescriben, así que usamos escritura directa
    if (address == 0xFF0F) { 
        // DEBUG: Rastrear escrituras a IF
        static int if_write_count = 0;
        if_write_count++;
        if (if_write_count <= 20) {
            std::cout << "[MMU IF WRITE #" << if_write_count 
                      << "] value=0x" << std::hex << (int)value
                      << " IO[0x0F] was 0x" << (int)IO[0x0F]
                      << std::dec << "\n";
        }
        // Solo bits 0-4 son válidos
        IO[0x0F] = value & 0x1F; 
        return; 
    }
    if (address == 0xFFFF) { IE = value & 0x1F; return; } // Solo bits 0-4

    // Zona Prohibida (Ignorar)
    if (address >= 0xFEA0 && address <= 0xFEFF) {
        return; 
    }

    // ROM (Banking en el cartucho)
    if (address <= 0x7FFF) {
        cart.writeCartridge(address, value);
        return;
    }
    // VRAM
    else if (address >= 0x8000 && address <= 0x9FFF) {
        // TODO: Verificar si PPU está en modo 3 (Drawing)
        // Durante modo 3, ignorar escrituras
        VRAM[vramIndex(address)] = value;
        return;
    }
    // RAM Externa
    else if (address >= 0xA000 && address <= 0xBFFF) {
        cart.writeCartridge(address, value);
        return;
    }
    // WRAM
    else if (address >= 0xC000 && address <= 0xDFFF) {
        // DEBUG: Track writes to sprite buffer area (0xC000-0xC09F)
        static int sprite_buffer_writes = 0;
        if (address >= 0xC000 && address <= 0xC09F && value != 0) {
            sprite_buffer_writes++;
            if (sprite_buffer_writes <= 20) {
                std::cout << "[WRAM SPRITE BUFFER] Write #" << sprite_buffer_writes
                          << " to 0x" << std::hex << address 
                          << " = 0x" << (int)value << std::dec << "\n";
            }
            if (sprite_buffer_writes == 100) {
                std::cout << "[WRAM SPRITE BUFFER] 100 writes to sprite buffer area!\n";
            }
        }
        WRAM[wramIndex(address)] = value;
        return;
    }
    // ECHO RAM (Escribir en WRAM correspondiente)
    else if (address >= 0xE000 && address <= 0xFDFF) {
        WRAM[wramIndex(address)] = value;
        return;
    }
    // OAM
    else if (address >= 0xFE00 && address <= 0xFE9F) {
        // TODO: Verificar si PPU está en modo 2 o 3
        // Durante modos 2 y 3, ignorar escrituras
        OAM[offSet(address, 0xFE00)] = value;
        return;
    }
    // I/O Registers
    else if (address >= 0xFF00 && address <= 0xFF7F) {
        // ============================================================
        // JOYPAD (0xFF00) - ESCRITURA CORRECTA
        // ============================================================
        if (address == 0xFF00) {
            // DEBUG: Track P1 writes
            static int p1_write_count = 0;
            p1_write_count++;
            if (p1_write_count <= 20) {
                std::cout << "[JOYPAD WRITE #" << p1_write_count << "] value=0x" 
                          << std::hex << (int)value 
                          << " (select_dpad=" << !(value & 0x10)
                          << " select_btn=" << !(value & 0x20)
                          << ")" << std::dec << "\n";
            }
            
            // Solo bits 4 y 5 son escribibles
            // Mantener bits 0-3 (estado de botones) y bits 6-7 (siempre 1)
            IO[0x00] = (value & 0x30) | (IO[0x00] & 0xCF);
            return;
        }
        
        // Escritura especial para DIV (0xFF04)
        if (address == 0xFF04) {
            // Escribir CUALQUIER valor a DIV lo resetea a 0
            static int div_reset_count = 0;
            div_reset_count++;
            if (div_reset_count <= 20) {
                std::cout << "[MMU] DIV (0xFF04) reset to 0! (write #" << div_reset_count 
                          << ", was 0x" << std::hex << (int)IO[0x04] << ")\n" << std::dec;
            }
            IO[0x04] = 0;
            return;
        }
        
        if (address == 0xFF41) {
            // Bug del STAT en DMG: cualquier escritura a STAT se comporta
            // durante un ciclo como si se hubiera escrito 0xFF, disparando
            // una interrupción STAT espuria si la PPU está en modo 0/1 o
            // hay coincidencia LY==LYC. Juegos como Jurassic Park (Ocean)
            // dependen de este quirk para su secuenciador de intro.
            // La CGB no tiene este bug, así que solo aplica en modo DMG.
            if (!cgb_mode) {
                bool lcd_on = (IO[0x40] & 0x80) != 0;
                uint8_t mode = IO[0x41] & 0x03;
                bool lyc_match = (IO[0x41] & 0x04) != 0;
                if (lcd_on && (mode == 0 || mode == 1 || lyc_match)) {
                    IO[0x0F] |= 0x02; // IF bit 1 (STAT)
                }
            }
            // Bits 0-2 son de solo lectura (los mantiene la PPU)
            IO[0x41] = (IO[0x41] & 0x07) | (value & 0x78) | 0x80;
            return;
        }

        // Registros CGB (solo activos en modo Game Boy Color)
        if (cgb_mode) {
            switch (address) {
                case 0xFF4D: // KEY1: armar el cambio de velocidad (se consume en STOP)
                    speed_switch_armed = (value & 0x01) != 0;
                    return;
                case 0xFF4F: // VBK: banco de VRAM
                    vram_bank = value & 0x01;
                    return;
                case 0xFF70: // SVBK: banco de WRAM (0 se trata como 1)
                    wram_bank = value & 0x07;
                    if (wram_bank == 0) wram_bank = 1;
                    return;
                case 0xFF68: bcps = value & 0xBF; return;
                case 0xFF69: // BCPD: escribir palette RAM de BG
                    BG_PAL[bcps & 0x3F] = value;
                    if (bcps & 0x80) bcps = 0x80 | ((bcps + 1) & 0x3F);
                    return;
                case 0xFF6A: ocps = value & 0xBF; return;
                case 0xFF6B: // OCPD: escribir palette RAM de OBJ
                    OBJ_PAL[ocps & 0x3F] = value;
                    if (ocps & 0x80) ocps = 0x80 | ((ocps + 1) & 0x3F);
                    return;
                case 0xFF55: // HDMA5: lanzar transferencia VRAM DMA
                    doHDMATransfer(value);
                    return;
            }
        }

        // ============================================================
        // REGISTROS DE AUDIO (APU) - $FF10-$FF3F
        // ============================================================
        if (address >= 0xFF10 && address <= 0xFF3F) {
            if (apu) {
                apu->writeByte(address, value);
            }
            return;
        }

        // Otros registros I/O normales
        IO[offSet(address, 0xFF00)] = value;
        return;
    }
    // HRAM
    else if (address >= 0xFF80 && address <= 0xFFFE) {
        // DEBUG: Track writes to 0xFF85 (VBLANK_DONE)
        if (address == 0xFF85) {
            static int ff85_write_count = 0;
            ff85_write_count++;
            if (ff85_write_count <= 20) {
                std::cout << "[HRAM 0xFF85] Write #" << ff85_write_count 
                          << " value=0x" << std::hex << (int)value << std::dec << "\n";
            }
        }
        
        // DEBUG: Track writes to 0xFF99 (GAME_STATUS) - CRITICAL for state machine
        if (address == 0xFF99) {
            static int game_status_write_count = 0;
            game_status_write_count++;
            
            const char* state_name = "UNKNOWN";
            switch (value) {
                case 36: state_name = "MENU_COPYRIGHT_INIT"; break;
                case 37: state_name = "MENU_COPYRIGHT_1"; break;
                case 53: state_name = "MENU_COPYRIGHT_2"; break;
                case 6:  state_name = "MENU_TITLE_INIT"; break;
                case 7:  state_name = "MENU_TITLE"; break;
                case 8:  state_name = "MENU_GAME_TYPE"; break;
                case 20: state_name = "PLAYING"; break;
            }
            
            std::cout << "[GAME_STATUS WRITE #" << game_status_write_count 
                      << "] value=" << std::dec << (int)value 
                      << " (" << state_name << ")" 
                      << " addr=0x" << std::hex << address << std::dec << "\n";
        }
        
        HRAM[offSet(address, 0xFF80)] = value;
        return;
    }
    
    std::cout << "Memory WRITE Error: Address not mapped " << std::hex << address << "\n";
}

// ============================================================
// FUNCIÓN PARA ACTUALIZAR ESTADO DE BOTONES
// ============================================================
// Button IDs: 0=Right, 1=Left, 2=Up, 3=Down, 4=A, 5=B, 6=Select, 7=Start
// ============================================================
void mmu::setButton(int button_id, bool pressed) {
    const char* button_names[] = {"RIGHT", "LEFT", "UP", "DOWN", "A", "B", "SELECT", "START"};
    
    if (button_id >= 0 && button_id <= 7) {
        std::cout << "[JOYPAD_BACKEND] setButton(" << button_names[button_id] 
                  << ", " << (pressed ? "PRESSED" : "RELEASED") << ")\n";
    }
    
    switch(button_id) {
        case 0: button_right = pressed; break;
        case 1: button_left = pressed; break;
        case 2: button_up = pressed; break;
        case 3: button_down = pressed; break;
        case 4: button_a = pressed; break;
        case 5: button_b = pressed; break;
        case 6: button_select = pressed; break;
        case 7: button_start = pressed; break;
    }
    
    // Opcionalmente, solicitar interrupción de Joypad (bit 4 de IF)
    // Esto despierta a la CPU si está en HALT esperando input
    if (pressed) {
        IO[0x0F] |= 0x10;  // Set Joypad interrupt flag
    }
}

// ============================================================
// FUNCIÓN PARA CONECTAR LA APU
// ============================================================
void mmu::setAPU(APU* apu_ptr) {
    apu = apu_ptr;
    std::cout << "[MMU] APU conectada para manejar registros de audio ($FF10-$FF3F)\n";
}
// ============================================================
// SAVE STATES
// ============================================================
void mmu::saveState(StateWriter& out) const {
    out.writeArray(VRAM);
    out.writeArray(WRAM);
    out.writeArray(HRAM);
    out.writeArray(IO);
    out.writeArray(OAM);
    out.write(IE);
    out.write(IF);

    out.write(cgb_mode);
    out.write(double_speed);
    out.write(speed_switch_armed);
    out.write(vram_bank);
    out.write(wram_bank);
    out.writeArray(BG_PAL);
    out.writeArray(OBJ_PAL);
    out.write(bcps);
    out.write(ocps);

    cart.saveState(out);
}

void mmu::loadState(StateReader& in) {
    in.readArray(VRAM);
    in.readArray(WRAM);
    in.readArray(HRAM);
    in.readArray(IO);
    in.readArray(OAM);
    IE = in.read<uint8_t>();
    IF = in.read<uint8_t>();

    cgb_mode           = in.read<bool>();
    double_speed       = in.read<bool>();
    speed_switch_armed = in.read<bool>();
    vram_bank          = in.read<uint8_t>();
    wram_bank          = in.read<uint8_t>();
    in.readArray(BG_PAL);
    in.readArray(OBJ_PAL);
    bcps = in.read<uint8_t>();
    ocps = in.read<uint8_t>();

    cart.loadState(in);
}
