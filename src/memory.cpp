#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include "cpu.h"
#include "memory.h"
#include "globals.h"
using namespace std;


uint8_t memory::read_ROM(uint16_t address) {
    if (address >= 0x0000 && address <= 0x3FFF) {
        return ROM[address];
    } else if (address >= 0x4000 && address <= 0x7FFF ) {
        return ROM[(rom_bank * 0x4000) + (address - 0x4000)];
    }
    return ROM[address];
}

uint8_t memory::read(uint16_t address) {
    if (debug){cout << "READ ADDRESS: " << hex<<address << '\n';}
    if (address >= 0x0000 && address <= 0x7FFF) { // Cartridge ROM
        return read_ROM(address);
    } else if (address >= 0x8000 && address <= 0x9FFF) { // VRAM
        return VRAM[address-0x8000];
    } else if (address >= 0xA000 && address <= 0xBFFF) { // ERAM
        return ERAM[address-0xA000];
    } else if (address >= 0xC000 && address <= 0xDFFF) { // WRAM
        return WRAM[address-0xC000];
    } else if (address >= 0xE000 && address <= 0xFDFF) { // echo ram
        return WRAM[address-0xE000];
    } else if (address >= 0xFE00 && address <= 0xFE9F) { // OAM
        return OAM[address-0xFE00];
    } else if (address >= 0xFEA0 && address <= 0xFEFF) { // not usable
        return 0xFF; //to do
    } else if (address >= 0xFF00 && address <= 0xFF7F) { // implment io ranges
        if (debug){cout << "I/O ADDRESS CALLED: " << address << '\n';}
        switch (address) {
            case 0xFF0F:
            return IF;

            case 0xFF04:
            return tmr.DIV;

            case 0xFF05:
            return tmr.TIMA;

            case 0xFF06:
            return tmr.TMA;

            case 0xFF07:
            return tmr.TAC;

            case 0xFF44:
            return 0x90;
        }
        return 0xFF; // for now
    } else if (address >= 0xFF80 && address <= 0xFFFE) { // HRAM
        return HRAM[address-0xFF80];
    } else if (address==0xFFFF) {
        return IE;
    } else {
        cout << "ERROR: UNIMPL ADDR CALLED - " << address << '\n';
        return 0xFF;
    }
}
void memory::write(uint16_t address, uint8_t content) {
    if (address >= 0x0000 && address <= 0x7FFF) { // Cartridge ROM
        if (address >= 0x2000 && address <= 0x3FFF) { // MBC1
            auto shtuff = content & 0x1F; // lower 5 bits
            if (shtuff == 0) {
                shtuff = 1;
            }
            rom_bank = shtuff;
        }
    } else if (address >= 0x8000 && address <= 0x9FFF) { // VRAM
        VRAM[address-0x8000] = content;
    } else if (address >= 0xA000 && address <= 0xBFFF) { // ERAM
        ERAM[address-0xA000] = content;
    } else if (address >= 0xC000 && address <= 0xDFFF) { // WRAM
        WRAM[address-0xC000] = content;
    } else if (address >= 0xE000 && address <= 0xFDFF) { // echo ram
        WRAM[address-0xE000] = content;
    } else if (address >= 0xFE00 && address <= 0xFE9F) { // OAM
        OAM[address-0xFE00] = content;
    } else if (address >= 0xFEA0 && address <= 0xFEFF) { // not usable
        //pass
    } else if (address >= 0xFF00 && address <= 0xFF7F) { // implment io ranges
        
        if (address == 0xFF00) {
            // joypad input 
        } else if (address >= 0xFF01 && address <=0xFF02) {
            //serial transfer
            if (serial && address==0xFF01) {cout << "SERIAL: " << (char)content <<'\n';}
        } else if (address == 0xFF0F) {
            // interrupts
            IF = content;
        } else if (address >= 0xFF10 && address <= 0xFF26) {
            //audio
        } else if (address >= 0xFF30 && address <= 0xFF3F) {
            // wave pattern
        } else if (address == 0xFF04) {
            tmr.DIV = 0;
        } else if (address == 0xFF05) {
            tmr.TIMA = content;
        } else if (address==0xFF06) {
            tmr.TMA = content; // to do
        } else if (address==0xFF07) {
            tmr.TAC = content;
        }

    } else if (address >= 0xFF80 && address <= 0xFFFE) { // HRAM
        HRAM[address-0xFF80] = content;
    } else if (address==0xFFFF) {
        IE = content;
    } else {
        cout << "ERROR: UNIMPL ADDR CALLED - " << address << '\n';
    }
}

void memory::loadROM(string path) { // to-do: implement MBC
    try {
        ifstream rom(path, ios::binary);
        if (!rom.is_open()) {
            cout << "error: could not open ROM\n";
            throw runtime_error("cannot open ROM");
        }
        

        vector<uint8_t> buffer(
            (istreambuf_iterator<char>(rom)),
            istreambuf_iterator<char>()
        );
        if (buffer.size()<32768) { // less than 32 KiB
            cout << "error: ROM invalid - too small\n";
        } else if (buffer.size()<32768) {
            cout << "error: ROM too big for current implementation\n";
            throw runtime_error("cannot copy ROM into memory");
        }
        
        copy(
            buffer.begin(),
            buffer.end(),
            ROM.begin()
        );

        if (debug) {cout << "ROM loaded successfully!\n";}
    } catch (const exception& e){
        cerr << "Caught: " << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
}
