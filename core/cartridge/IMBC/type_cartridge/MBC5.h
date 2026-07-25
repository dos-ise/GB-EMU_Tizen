#pragma once
#include "../IMBC.h"
#include <vector>

class MBC5 : public IMBC {
private:
    std::vector<uint8_t>& rom;
    std::vector<uint8_t>& ram;

    uint16_t romBank;      // 9 bits: 0x000 - 0x1FF (a diferencia de MBC1/3, el banco 0 SÍ es seleccionable)
    uint8_t  ramBank;      // 0x00 - 0x0F
    bool     ramEnabled;
    uint16_t romBanksCount;

public:
    MBC5(std::vector<uint8_t>& rom_ref, std::vector<uint8_t>& ram_ref, uint16_t banks);

    uint8_t readROM(uint16_t address) override;
    void writeROM(uint16_t address, uint8_t value) override;

    uint8_t readRAM(uint16_t address) override;
    void writeRAM(uint16_t address, uint8_t value) override;

    void saveState(StateWriter& out) const override;
    void loadState(StateReader& in) override;
};
