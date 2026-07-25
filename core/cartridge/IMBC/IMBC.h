#ifndef IMBC_H
#define IMBC_H

#include <fstream>
#include <iostream>

#include "state/state.h"

class IMBC
{
    public:
        virtual ~IMBC() = default;

        //
        virtual uint8_t readROM(uint16_t address) = 0;
        virtual void writeROM(uint16_t address, uint8_t value) = 0;
        virtual uint8_t readRAM(uint16_t address) = 0;
        virtual void writeRAM(uint16_t address, uint8_t value) = 0;

        // Save states: registros de banking internos del mapper.
        // RomOnly no tiene estado, por eso el default es vacío.
        virtual void saveState(StateWriter& out) const { (void)out; }
        virtual void loadState(StateReader& in) { (void)in; }
};
#endif // CARTRIDGE_H