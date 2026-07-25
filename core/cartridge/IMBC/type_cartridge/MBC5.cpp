#include "MBC5.h"

MBC5::MBC5(std::vector<uint8_t>& rom_ref, std::vector<uint8_t>& ram_ref, uint16_t banks)
    : rom(rom_ref), ram(ram_ref), romBank(1), ramBank(0), ramEnabled(false), romBanksCount(banks)
{
}

uint8_t MBC5::readROM(uint16_t address) {
    // Banco 0 (Fijo)
    if (address < 0x4000) {
        return rom[address];
    }
    // Banco N (Switchable). En MBC5 el banco 0 también puede mapearse aquí.
    if (address >= 0x4000 && address <= 0x7FFF) {
        uint16_t bank = romBank & (romBanksCount - 1);
        uint32_t target = (static_cast<uint32_t>(bank) * 0x4000u) + (address - 0x4000);
        if (target < rom.size()) return rom[target];
    }
    return 0xFF;
}

void MBC5::writeROM(uint16_t address, uint8_t value) {
    // 1. RAM Enable
    if (address < 0x2000) {
        ramEnabled = ((value & 0x0F) == 0x0A);
    }
    // 2. ROM Bank Number: 8 bits bajos
    else if (address >= 0x2000 && address < 0x3000) {
        romBank = (romBank & 0x100) | value;
    }
    // 3. ROM Bank Number: bit 9
    else if (address >= 0x3000 && address < 0x4000) {
        romBank = (romBank & 0x0FF) | (static_cast<uint16_t>(value & 0x01) << 8);
    }
    // 4. RAM Bank Number (0x00-0x0F)
    else if (address >= 0x4000 && address < 0x6000) {
        ramBank = value & 0x0F;
    }
}

uint8_t MBC5::readRAM(uint16_t address) {
    if (!ramEnabled || ram.empty()) return 0xFF;

    uint32_t offset = (static_cast<uint32_t>(ramBank) * 0x2000u) + (address - 0xA000);
    if (offset < ram.size()) return ram[offset];
    return 0xFF;
}

void MBC5::writeRAM(uint16_t address, uint8_t value) {
    if (!ramEnabled || ram.empty()) return;

    uint32_t offset = (static_cast<uint32_t>(ramBank) * 0x2000u) + (address - 0xA000);
    if (offset < ram.size()) ram[offset] = value;
}

// ============================================================
// SAVE STATES
// ============================================================
void MBC5::saveState(StateWriter& out) const {
    out.write(romBank);
    out.write(ramBank);
    out.write(ramEnabled);
}

void MBC5::loadState(StateReader& in) {
    romBank    = in.read<uint16_t>();
    ramBank    = in.read<uint8_t>();
    ramEnabled = in.read<bool>();
}
