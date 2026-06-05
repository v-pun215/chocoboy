#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

struct memory {
    array<uint8_t, 2097000> ROM = {}; // 2MiB combined ROM
    uint8_t rom_bank = 1;
    array<uint8_t, 8192> VRAM = {}; // 8KB VRAM
    array<uint8_t, 8192> ERAM = {}; // 8KB External RAM (local game storage?)

    array<uint8_t, 8192> WRAM = {}; // 8KB Work RAM (actual runtime meory)

    array<uint8_t, 160> OAM = {}; // object attribute memory (sprites and stuff)

    array<uint8_t, 127> HRAM = {}; // high ram (0-0)

    uint8_t IE = 0; // Interrupt Enable register



    uint8_t read_ROM(uint16_t address) {
        if (address >= 0x0000 && address <= 0x3FFF) {
            return ROM[address];
        } else if (address >= 0x4000 && address <= 0x7FFF ) {
            return rom_bank * 16384 + address - 0x4000;
        }
        return ROM[address];
    }

    uint8_t read(uint16_t address) {
        cout << "READ ADDRESS: " << hex<<address << '\n';
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
            
            if (address == 0xFF00) {
                // joypad input 
            } else if (address >= 0xFF01 && address <=0xFF02) {
                //serial transfer
                cout << "SERIAL: " << content <<'\n';
            } else if (address == 0xFF0F) {
                // interrupts
            } else if (address >= 0xFF10 && address <= 0xFF26) {
                //audio
            } else if (address >= 0xFF30 && address <= 0xFF3F) {
                // wave pattern
            } //... TO DO

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

            cout << "ROM loaded successfully!\n";
        } catch (const exception& e){
            cerr << "Caught: " << e.what() << '\n';
        }
    }
};

struct cpu { // 8-bit custom Sharp LR35902 processor
    enum registernams { 
        B, //0
        C, //1
        D, //2
        E,//3
        H,//4
        L,//5
        notused,//6
        A//7
    };

    array<uint8_t, 8> registers = {}; // r8
    uint16_t PC = 0; // program counter
    uint16_t SP = 0; //stack pointer
    bool halted = false;

    // Flags register
    bool flag_z = 0; // zero flag
    bool flag_n = 0; // subtraction flag (BCD)
    bool flag_h = 0; // half carry flag (BCD)
    bool flag_c = 0; // carry flag
    

    uint8_t fetch(memory& mem) {
        auto value = mem.read(PC);
        PC+=1;
        return value;
    }
    uint16_t HL() {
        return (registers[H] << 8) | registers[L];
    }
    void write_HL(uint16_t val) {
        auto low = val & 0xFF;
        auto high = (val >> 8) & 0xFF;
        registers[H] = high;
        registers[L] = low;
    }

    void write_register_r16(uint8_t& low_register, uint8_t& high_register, uint16_t val) {
        /*
        high register: first value (eg. H)
        low register: second value (eg. L)
        val: val to write
        */
        auto low = val & 0xFF;
        auto high = (val >> 8) & 0xFF;
        high_register = high;
        low_register = low;
    }
    uint16_t BC() {
        return (registers[B] << 8) | registers[C];
    }
    uint16_t DE() {
        return (registers[D] << 8) | registers[E];
    }

    uint8_t decode(uint8_t opcode, memory& mem) { //returns no of cycles took
        cout << "EXECUTE OPCODE: " << hex << (int)opcode<< '\n';
        uint8_t cycles = 0;
        switch (opcode) {
            case 0x00: { // NOP
                cycles = 4;
                //pass
                break;
            }
            case 0x40 ... 0x7F: {
                cycles = 4;
                auto destination = (opcode >> 3) & 0x07;
                auto source = opcode & 0x07;
                if (source==6 && destination !=6) {
                    // LD r8, [HL]
                    registers[destination] = mem.read(HL());
                    cycles=8;
                } else if (destination==6 && source!=6) {
                    // LD [HL], r8
                    mem.write(HL(), registers[source]);
                } else if (destination==6 && source==6) {
                    // HALT
                    halted=true; // to do
                } else {
                    // LD r8, r8
                    registers[destination]=registers[source];
                }
                break;
            }

            case 0x06:
            case 0x16:
            case 0x26:
            case 0x36:
            case 0x0E:
            case 0x1E:
            case 0x2E:
            case 0x3E: {
                auto destination = (opcode >> 3) & 0x07;
                auto n8 = fetch(mem);
                cycles=8;
                if (destination==6) {
                    // LD [HL], n8
                    mem.write(HL(), n8);
                    cycles=12;
                } else {
                    // LD r8, n8
                    registers[destination] = n8;
                }
                break;
            }

            case 0x01:
            case 0x11:
            case 0x21:
            case 0x31: { // LD r16, n16
                auto r16 = (opcode >> 4) & 0x03; // destination
                auto low = fetch(mem);
                auto high = fetch(mem);
                auto n16 = (high << 8) | low;
                switch (r16) {
                    case 0://BC
                    registers[B] = high;
                    registers[C] = low;
                    break;

                    case 1://DE
                    registers[D]=high;
                    registers[E]=low;
                    break;

                    case 2://HL
                    registers[H]=high;
                    registers[L]=low;
                    break;

                    case 3:
                    // LD SP, n16
                    SP = n16;
                    break;
                }
                cycles=12;
                break;
            }

            case 0xEA: {
                // LD [n16], A
                auto low = fetch(mem);
                auto high = fetch(mem);
                auto n16 = (high << 8) | low;
                cycles=16;
                mem.write(n16, registers[A]);
                break;
            }
            case 0xFA: {
                // LD A, [n16]
                auto low = fetch(mem);
                auto high = fetch(mem);
                auto n16 = (high<<8 ) | low;
                cycles=16;
                registers[A] = mem.read(n16);
                break;
            }

            // LD [r16], A 
            case 0x02:
            case 0x12: {
                auto r16 = (opcode >> 4) & 0x03; //dest
                auto address = (r16==0) ? ((registers[B] <<8) | registers[C]) : ((registers[D] <<8) | registers[E]);
                mem.write(address, registers[A]);
                cycles=8;
                break;

            }
            // LD [HL+], A 

            
            case 0x22: {
                mem.write(HL(), registers[A]);
                auto hl = HL();
                hl++;
                auto low = hl & 0xFF;
                auto high = (hl >> 8) & 0xFF;
                registers[H] = high;
                registers[L] = low;
                cycles=8;
                break;
            }
            // LD [HL-], A 
            case 0x32: {
                mem.write(HL(), registers[A]);
                auto hl = HL();
                hl--;
                auto low = hl & 0xFF;
                auto high = (hl >>8) & 0xFF;
                registers[H] = high;
                registers[L] = low;
                cycles=8;
                break;
            }
            // LD A, [r16]
            case 0x0A:
            case 0x1A: {
                auto r16 = (opcode >> 4) & 0x03; //dest
                auto address = (r16==0) ? ((registers[B] <<8) | registers[C]) : ((registers[D] <<8) | registers[E]);
                registers[A] = mem.read(address);
                cycles=8;
                break;
            }
            // LD A, [HL+]
            case 0x2A: {
                auto hl = HL();
                registers[A] = mem.read(hl);                
                hl++;
                auto low = hl & 0xFF;
                auto high = (hl >> 8) & 0xFF;
                registers[H] = high;
                registers[L] = low;
                cycles=8;
                break;
            }
            // LD A, [HL-]
            case 0x3A: {
                auto hl = HL();
                registers[A] = mem.read(hl);                
                hl--;
                auto low = hl & 0xFF;
                auto high = (hl >> 8) & 0xFF;
                registers[H] = high;
                registers[L] = low;
                cycles=8;
                break;
            }

            // LDH [n16], A
            case 0xE0: {
                auto high = 0xFF;
                auto low=fetch(mem);
                auto address = (high << 8) | low;
                mem.write(address,registers[A]);
                cycles=12;
                break;
            }
            // LDH A, [n16]
            case 0xF0: {
                auto high = 0xFF;
                auto low=fetch(mem);
                auto address=(high <<8) | low;
                registers[A] = mem.read(address);
                cycles=12;
                break;
            }
            // LDH [C], A
            case 0xE3: {
                auto address = 0xFF00+registers[C];
                mem.write(address, registers[A]);
                cycles=8;
                break;
            }
            // LDH A, [C]
            case 0xF3: {
                auto address= 0xFF00 + registers[C];
                registers[A] = mem.read(address);
                cycles= 8;
                break;  
            }

            case 0x88 ... 0x8F: {
                uint8_t val =0;
                if (opcode != 0x8E) { // ADC A, r8
                    auto r8 = opcode  & 0x07;
                    val = registers[r8];
                    cycles=4;
                } else { // ADC A, [HL]
                    val = mem.read(HL());
                    cycles=8;
                }
                auto result = registers[A] + flag_c + val;

                // flags
                flag_z = ((result & 0xFF) == 0);

                flag_n = 0;

                flag_h = (((registers[A] & 0x0F) + (val & 0x0F) + flag_c) > 0x0F);

                flag_c = (result > 0x0F);

                registers[A] = result;

                break;
            }

            case 0xCE: {
                // ADC A, n8
                auto n8 = fetch(mem);
                auto result = registers[A] + flag_c + n8;
                cycles = 8;
                flag_z = ((result & 0xFF) == 0);
                flag_n = 0;
                flag_h = (((registers[A] & 0x0F) + (n8 & 0x0F) + flag_c) > 0x0F);

                flag_c = (result > 0x0F);
                registers[A] = result;
                break;
            }

            case 0x80 ... 0x87: {
                uint8_t val =0;
                if (opcode != 0x86) { // ADD A, r8
                    auto r8 = opcode  & 0x07;
                    val = registers[r8];
                    cycles=4;
                } else { // ADD A, [HL]
                    val = mem.read(HL());
                    cycles=8;
                }
                auto result = registers[A] + val;
                flag_z = ((result & 0xFF) == 0);
                flag_n = 0;
                flag_h = (((registers[A] & 0x0F) + (val & 0x0F) + flag_c) > 0x0F);
                flag_c = (result > 0x0F);

                registers[A] = result;
                break;
            }
            
            case 0xC6: {
                // ADD A, n8
                auto n8 = fetch(mem);
                auto result = registers[A] + n8;
                cycles = 8;
                flag_z = ((result & 0xFF) == 0);
                flag_n = 0;
                flag_h = (((registers[A] & 0x0F) + (n8 & 0x0F) + flag_c) > 0x0F);

                flag_c = (result > 0x0F);
                registers[A] = result;
                break;
            }
            case 0x09:
            case 0x19:
            case 0x29:
            case 0x39: {
                // ADD HL, r16
                auto result = 0;
                auto r16 = (opcode >> 4) & 0x03;
                auto second_op = 0;
                switch (r16) {
                    case 0: // BC
                    result = HL() + BC();
                    second_op = BC();
                    break;

                    case 1: // DE
                    result = HL() + DE();
                    second_op = DE();
                    break;

                    case 2: // HL
                    result = HL() +HL();
                    second_op = HL();
                    break;

                    case 4: // SP
                    // ADD HL, SP
                    result = HL() + SP;
                    second_op = SP;
                    break;
                }
                flag_n = 0;
                flag_h = ((HL() & 0b111111111111) + (second_op & 0b111111111111)) > 0b111111111111;
                flag_c = ((HL() + second_op) > 0xFFFF);
                cycles=8;

                break;
                

            }

            case 0xB8 ... 0xBF: {
                auto r8 = opcode & 0x07;
                uint8_t result;
                if (opcode == 0xBE) {
                    // CP A, [HL]
                    result = registers[A] - mem.read(HL());
                    cycles=8;
                } else {
                    // CP A, r8
                    result = registers[A] - registers[r8];
                    cycles=4;
                }
                flag_z = (result == 0);
                flag_n = 1;
                flag_h = (registers[r8] & 0b1111) > (registers[A] & 0b1111);
                flag_c = registers[r8] > registers[A];
                break;
            }

            case 0xFE: {
                // CP A, n8
                uint8_t n8 = fetch(mem);
                cycles=8;
                auto result = registers[A] - n8;
                flag_z = (result == 0);
                flag_n = 1;
                flag_h = (n8 & 0b1111) > (registers[A] & 0b1111);
                flag_c = n8 > registers[A];
                break;
            }

            case 0x05:
            case 0x15:
            case 0x25:
            case 0x0D:
            case 0x1D:
            case 0x2D:
            case 0x3D: {
                // DEC r8
                auto r8 = opcode & 0x07;
                auto result = registers[r8] - 1;
                registers[r8]-=1;
                cycles=4;

                flag_z = (result == 0);
                flag_n = 1;
                flag_h = (registers[r8] & 0xF) == 0;
                break;
            }

            case 0x35: {
                // DEC [HL]
                auto curn_hl = mem.read(HL());
                auto result = curn_hl -1;
                mem.write(HL(), result);
                flag_z = (result == 0);
                flag_n = 1;
                flag_h = (curn_hl & 0xF) == 0;
                cycles=12;
                break;
            }

            case 0x0B:
            case 0x1B:
            case 0x2B:
            case 0x3B: {
                // DEC r16
                auto r16 = (opcode >> 4) & 0x03;
                uint16_t my_boi;
                cycles=8;
                switch (r16) {
                    case 0:
                    my_boi = BC();
                    write_register_r16(registers[B], registers[C], my_boi-1);
                    break;

                    case 1:
                    my_boi = DE();
                    write_register_r16(registers[D], registers[E], my_boi-1);
                    break;

                    case 2:
                    my_boi = HL();
                    write_register_r16(registers[H], registers[L], my_boi-1);
                    break;
                    case 3:
                    my_boi = SP;
                    SP-=1;
                    break;
                }
                break;
            }

            case 0x04:
            case 0x14:
            case 0x24:
            case 0x0C:
            case 0x1C:
            case 0x2C:
            case 0x3C: {
                // INC r8
                uint8_t r8 = opcode & 0x07;
                auto result = registers[r8] +1;
                cycles=4;
                flag_z = (result == 0);
                flag_n = 0;
                flag_h = (registers[r8] & 0xF) == 0xF;
                registers[r8] = result;
                break;
            }

            case 0x34: {
                // INC [HL]
                cycles=12;
                uint8_t curn_hl = mem.read(HL());
                auto result = curn_hl + 1;
                flag_z = (result == 0);
                flag_n = 0;
                flag_h = (curn_hl & 0xF) == 0xF;
                mem.write(HL(), result);
                break;
            }

            case 0x03:
            case 0x13:
            case 0x23:
            case 0x33: {
                // INC r16
                auto r16 = (opcode >> 4) & 0x03;
                uint16_t my_boi;
                cycles=8;
                switch (r16) {
                    case 0:
                    my_boi = BC();
                    write_register_r16(registers[B], registers[C], my_boi+1);
                    break;

                    case 1:
                    my_boi = DE();
                    write_register_r16(registers[D], registers[E], my_boi+1);
                    break;

                    case 2:
                    // INC SP
                    my_boi = HL();
                    write_register_r16(registers[H], registers[L], my_boi+1);
                    break;
                    case 3:
                    my_boi = SP;
                    SP+=1;
                    break;
                }
                break;
            }

            case 0x98 ... 0x9F: {
                // SBC A, r8
                auto r8 = opcode & 0x7;
                uint8_t result;
                if (opcode != 0x9E) {
                    result = registers[A] - flag_c - registers[r8];
                    cycles=4;
                    flag_h = ((registers[r8] & 0xF) + flag_c) > (registers[A] & 0xF); 
                    flag_c = (registers[r8] + flag_c) > registers[A];
                } else {
                    // SBC A, [HL]
                    auto curn_hl = mem.read(HL());
                    result = registers[A] - flag_c - curn_hl;
                    cycles=8;
                    flag_h = ((curn_hl & 0xF) + flag_c) > (registers[A] & 0xF);
                    flag_c = (curn_hl + flag_c) > registers[A];
                }
                flag_z = (result == 0);
                flag_n = 1;
                break;
            }

            case 0xDE: {
                // SBC A, n8
                auto n8 = fetch(mem);
                auto result = registers[A] - n8 - flag_c;
                flag_z = (result == 0);
                flag_n = 1;
                flag_h = ((n8 & 0xF) + flag_c) > (registers[A] & 0xF);
                flag_c = (n8 + flag_c) > registers[A];
                break;
            }

            case 0x90 ... 0x97: {
                auto r8 = (opcode & 0x07);
                uint8_t result;
                if (opcode == 0x96) {
                    // SUB A, [HL]
                    auto curn_hl = mem.read(HL());
                    result = registers[A] - curn_hl;
                    cycles=8;
                    flag_h = (curn_hl & 0xF) > (registers[A] & 0xF);
                    flag_c = curn_hl > registers[A];
                } else {
                    // SUB A, r8
                    result = registers[A] - registers[r8];
                    cycles=4;
                    flag_h = (registers[r8] & 0xF) > (registers[A] & 0xF);
                    flag_c = registers[r8] > registers[A];
                }
                flag_z = (result == 0);
                flag_n = 1;
                break;
            }

            case 0xD6: {
                uint8_t n8 = fetch(mem);
                auto result = registers[A] - n8;
                cycles=8;
                flag_z = (result == 0);
                flag_n = 1;
                flag_h = (n8 & 0xF) > (registers[A] & 0xF);
                flag_c = n8 > registers[A];
                break;
            }



            


        }
        return cycles;
    }
};

int main() {
    const char* rom_path = std::getenv("ROM_PATH");
    cpu gb_cpu;
    memory mem;
    
    mem.loadROM(rom_path);
    cout <<"STEP1\n";
    while (true) {
        if (gb_cpu.halted) {
            break;
        }
        gb_cpu.decode(gb_cpu.fetch(mem), mem);
    }

}