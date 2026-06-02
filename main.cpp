#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

struct memory {
    array<uint8_t, 32768> ROM = {}; // 32KB combined ROM
    array<uint8_t, 8192> VRAM = {}; // 8KB VRAM
    array<uint8_t, 8192> ERAM = {}; // 8KB External RAM (local game storage?)

    array<uint8_t, 8192> WRAM = {}; // 8KB Work RAM (actual runtime meory)

    array<uint8_t, 160> OAM = {}; // object attribute memory (sprites and stuff)

    array<uint8_t, 126> HRAM = {}; // high ram (0-0)

    uint8_t IE = 0; // Interrupt Enable register

    uint8_t read(uint16_t address) {
        if (address >= 0x0000 && address <= 0x7FFF) { // Cartridge ROM
            return ROM[address];
        } else if (address >= 0x8000 && address <= 0x9FFF) { // VRAM
            return VRAM[address-0x8000];
        } else if (address >= 0xA000 && address <= 0xBFFF) { // ERAM
            return ERAM[address-0xA000];
        } else if (address >= 0xC000 && address <= 0xDFFF) { // WRAM
            return WRAM[address-0xC000];
        } else if (address >= 0xE000 && address <= 0xFDFF) { // echo ram
            return WRAM[address-0xC000];
        } else if (address >= 0xFE00 && address <= 0xFE9F) { // OAM
            return OAM[address-0xFE00];
        } else if (address >= 0xFEA0 && address <= 0xFEFF) { // not usable
            return 0xFF; //to do
        } else if (address >= 0xFF00 && address <= 0xFF7F) { // implment io ranges
            // TO-DO
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
    void write(uint16_t address, uint8_t content) {
        if (address >= 0x0000 && address <= 0x7FFF) { // Cartridge ROM
            // pass
        } else if (address >= 0x8000 && address <= 0x9FFF) { // VRAM
            VRAM[address-0x8000] = content;
        } else if (address >= 0xA000 && address <= 0xBFFF) { // ERAM
            WRAM[address-0xA000] = content; // redirect to WRAM (echo)
        } else if (address >= 0xC000 && address <= 0xDFFF) { // WRAM
            WRAM[address-0xC000] = content;
        } else if (address >= 0xE000 && address <= 0xFDFF) { // echo ram
            //pass (prohibited)
        } else if (address >= 0xFE00 && address <= 0xFE9F) { // OAM
            OAM[address-0xFE00] = content;
        } else if (address >= 0xFEA0 && address <= 0xFEFF) { // not usable
            //pass
        } else if (address >= 0xFF00 && address <= 0xFF7F) { // implment io ranges
            //pass (TO DO)
        } else if (address >= 0xFF80 && address <= 0xFFFE) { // HRAM
            HRAM[address-0xFF80] = content;
        } else if (address==0xFFFF) {
            IE = content;
        } else {
            cout << "ERROR: UNIMPL ADDR CALLED - " << address << '\n';
        }
    }

    void loadROM(string path) { // to-do: implement MBC
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
        } catch (const exception& e){
            cerr << "Caught: " << e.what() << '\n';
        }
    }
};

struct cpu { // 8-bit custom Sharp LR35902 processor
    enum registerNames { 
        A,
        B,C,
        D,E,
        H,L,
    };

    array<uint8_t, 7> registers = {}; // r8
    uint16_t PC = 0; // program counter
    uint16_t SP = 0; //stack pointer

    // Flags register
    bool flag_z = 0; // zero flag
    bool flag_n = 0; // subtraction flag (BCD)
    bool flag_h = 0; // half carry flag (BCD)
    bool flag_c = 0; // carry flag
    

    uint8_t fetch(memory mem) {
        auto value = mem.read(PC);
        PC+=1;
        return value;
    }

    void decode(uint8_t opcode) {
        switch (opcode) {
            /* ADC family of opcodes*/
            case 0x88: { // ADC A, B (r8)
                registers[A]+= registers[B] + flag_c;
            }
        }
    }
};

int main() {
    
}