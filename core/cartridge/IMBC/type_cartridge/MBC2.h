#pragma once
#include "../IMBC.h"
#include <array>
#include <vector>

class MBC2 : public IMBC {
private:
    std::vector<uint8_t>& rom;

    // MBC2 lleva la RAM integrada en el chip: 512 posiciones de 4 bits.
    std::array<uint8_t, 512> builtinRam;

    uint8_t  romBank;      // 4 bits: 0x01 - 0x0F
    bool     ramEnabled;
    uint16_t romBanksCount;

public:
    MBC2(std::vector<uint8_t>& rom_ref, uint16_t banks);

    uint8_t readROM(uint16_t address) override;
    void writeROM(uint16_t address, uint8_t value) override;

    uint8_t readRAM(uint16_t address) override;
    void writeRAM(uint16_t address, uint8_t value) override;

    void saveState(StateWriter& out) const override;
    void loadState(StateReader& in) override;
};
