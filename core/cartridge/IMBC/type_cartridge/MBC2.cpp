#include "MBC2.h"

MBC2::MBC2(std::vector<uint8_t>& rom_ref, uint16_t banks)
    : rom(rom_ref), romBank(1), ramEnabled(false), romBanksCount(banks)
{
    builtinRam.fill(0xFF);
}

uint8_t MBC2::readROM(uint16_t address) {
    // Banco 0 (Fijo)
    if (address < 0x4000) {
        return rom[address];
    }
    // Banco N (Switchable)
    if (address >= 0x4000 && address <= 0x7FFF) {
        uint8_t bank = romBank & (romBanksCount - 1);
        if (bank == 0) bank = 1;
        uint32_t target = (static_cast<uint32_t>(bank) * 0x4000u) + (address - 0x4000);
        if (target < rom.size()) return rom[target];
    }
    return 0xFF;
}

void MBC2::writeROM(uint16_t address, uint8_t value) {
    // En MBC2 ambos registros viven en 0x0000-0x3FFF y se distinguen por
    // el bit 8 de la dirección: 0 = RAM enable, 1 = ROM bank.
    if (address < 0x4000) {
        if (address & 0x0100) {
            romBank = value & 0x0F;
            if (romBank == 0) romBank = 1;
        } else {
            ramEnabled = ((value & 0x0F) == 0x0A);
        }
    }
}

uint8_t MBC2::readRAM(uint16_t address) {
    if (!ramEnabled) return 0xFF;

    // Solo 512 posiciones; el resto del rango 0xA000-0xBFFF es eco.
    // Los 4 bits altos no existen en hardware y se leen a 1.
    uint16_t offset = (address - 0xA000) & 0x01FF;
    return 0xF0 | (builtinRam[offset] & 0x0F);
}

void MBC2::writeRAM(uint16_t address, uint8_t value) {
    if (!ramEnabled) return;

    uint16_t offset = (address - 0xA000) & 0x01FF;
    builtinRam[offset] = value & 0x0F;
}

// ============================================================
// SAVE STATES
// ============================================================
void MBC2::saveState(StateWriter& out) const {
    out.write(romBank);
    out.write(ramEnabled);
    out.writeArray(builtinRam);
}

void MBC2::loadState(StateReader& in) {
    romBank    = in.read<uint8_t>();
    ramEnabled = in.read<bool>();
    in.readArray(builtinRam);
}
