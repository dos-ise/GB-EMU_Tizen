// ============================================================
// MMU.H - ACTUALIZADO CON SOPORTE PARA JOYPAD
// ============================================================

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "cartridge/cartridge.h"
#include "state/state.h"

class ppu; // Forward declaration
class timer; // Forward declaration
class cpu; // Forward declaration
class emcc_main; // Forward declaration
class APU; // Forward declaration - Audio Processing Unit

class mmu
{
    friend class ppu;
    friend class timer;
    friend class cpu;
    friend class emcc_main;
    
public:
    // Constructor explícito que recibe la ruta
    explicit mmu(const std::string& romPath);

    uint8_t readMemory(uint16_t address);
    void writeMemory(uint16_t address, uint8_t value);
    
    // ============================================================
    // NUEVA: Función para establecer estado de botones
    // ============================================================
    void setButton(int button_id, bool pressed);
    
    // ============================================================
    // NUEVA: Función para conectar la APU
    // ============================================================
    void setAPU(APU* apu_ptr);

    // ============================================================
    // NUEVAS: Estado CGB (Game Boy Color)
    // ============================================================
    bool isCGB()         const { return cgb_mode; }
    bool isDoubleSpeed() const { return double_speed; }

    // ============================================================
    // NUEVAS: Save states
    // ============================================================
    // Guarda/restaura toda la memoria + estado CGB + cartucho (RAM/MBC).
    // El estado de los botones NO se toca: es entrada en vivo del usuario.
    void saveState(StateWriter& out) const;
    void loadState(StateReader& in);

    const std::string& romTitle() const { return cart.getTitle(); }

private:
    // Instancia del cartucho
    cartridge cart;

    // Puntero a la APU (Audio)
    APU* apu = nullptr;

    // Regiones de memoria interna
    // En CGB la VRAM tiene 2 bancos de 8KB y la WRAM 8 bancos de 4KB.
    // En DMG solo se usa el banco 0 (VRAM) y los bancos 0-1 (WRAM).
    std::array<uint8_t, 0x4000> VRAM; // 2 x 8KB Video RAM
    std::array<uint8_t, 0x8000> WRAM; // 8 x 4KB Work RAM
    std::array<uint8_t, 0x007F> HRAM; // 127 bytes High RAM
    std::array<uint8_t, 0x0080> IO;   // 128 bytes I/O
    std::array<uint8_t, 0x00A0> OAM;  // Object Attribute Memory

    uint8_t IE; // Interrupt Enable (0xFFFF)
    uint8_t IF; // Interrupt Flag (0xFF0F)
    
    // ============================================================
    // NUEVAS: Variables para el estado de los botones
    // ============================================================
    bool button_right = false;
    bool button_left = false;
    bool button_up = false;
    bool button_down = false;
    bool button_a = false;
    bool button_b = false;
    bool button_select = false;
    bool button_start = false;

    // ============================================================
    // NUEVAS: Estado CGB (Game Boy Color)
    // ============================================================
    bool    cgb_mode           = false; // Cartucho con flag CGB (0x143 & 0x80)
    bool    double_speed       = false; // KEY1 bit 7
    bool    speed_switch_armed = false; // KEY1 bit 0 (se consume en STOP)
    uint8_t vram_bank          = 0;     // VBK  (0xFF4F)
    uint8_t wram_bank          = 1;     // SVBK (0xFF70)

    // Paletas de color CGB: 8 paletas x 4 colores x 2 bytes (RGB555)
    std::array<uint8_t, 64> BG_PAL;
    std::array<uint8_t, 64> OBJ_PAL;
    uint8_t bcps = 0; // Índice BG palette  (0xFF68, bit 7 = auto-incremento)
    uint8_t ocps = 0; // Índice OBJ palette (0xFF6A, bit 7 = auto-incremento)

    // Helpers de direccionamiento con bancos
    uint32_t vramIndex(uint16_t address) const {
        return static_cast<uint32_t>(vram_bank) * 0x2000u + (address - 0x8000);
    }
    uint32_t wramIndex(uint16_t address) const {
        if (address >= 0xE000) address -= 0x2000; // Echo RAM
        if (address <= 0xCFFF) return address - 0xC000;            // Banco 0 fijo
        uint8_t bank = (wram_bank == 0) ? 1 : wram_bank;           // Banco 1-7
        return static_cast<uint32_t>(bank) * 0x1000u + (address - 0xD000);
    }

    void doHDMATransfer(uint8_t value);

    // Funciones auxiliares privadas
    uint16_t offSet(uint16_t address, uint16_t base);
    void DMA(uint8_t value);
};

// ============================================================
// NOTAS:
// ============================================================
// 1. Añadida función setButton() para actualizar estado de botones
// 2. Añadidas variables privadas para almacenar estado de cada botón
// 3. Estas variables se usan en readMemory() cuando se lee 0xFF00