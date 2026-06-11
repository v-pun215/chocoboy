#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

using namespace std;
bool debug = false;
bool serial = false;
struct memory {
    array<uint8_t, 2097000> ROM = {}; // 2MiB combined ROM
    uint8_t rom_bank = 1;
    array<uint8_t, 8192> VRAM = {}; // 8KB VRAM
    array<uint8_t, 8192> ERAM = {}; // 8KB External RAM (local game storage?)

    array<uint8_t, 8192> WRAM = {}; // 8KB Work RAM (actual runtime meory)

    array<uint8_t, 160> OAM = {}; // object attribute memory (sprites and stuff)

    array<uint8_t, 127> HRAM = {}; // high ram (0-0)

    uint8_t IE = 0; // Interrupt Enable register

    vector<uint16_t> stack{};
    uint8_t ly=0;



    uint8_t read_ROM(uint16_t address) {
        if (address >= 0x0000 && address <= 0x3FFF) {
            return ROM[address];
        } else if (address >= 0x4000 && address <= 0x7FFF ) {
            return ROM[(rom_bank * 0x4000) + (address - 0x4000)];
        }
        return ROM[address];
    }

    uint8_t read(uint16_t address) {
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
            if (address == 0xFF44) {
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
                if (serial) {cout << "SERIAL: " << (int)content <<'\n';}
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

            if (debug) {cout << "ROM loaded successfully!\n";}
        } catch (const exception& e){
            cerr << "Caught: " << e.what() << '\n';
            exit(EXIT_FAILURE);
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
    uint16_t PC = 0x0100; // program counter
    uint16_t SP = 0xFFFE; //stack pointer
    bool IME = 0; // Interrupt Master Enable
    bool IME_pending = 0; // next instruction IME enable;
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

    void write_register_r16(uint8_t& high_register, uint8_t& low_register, uint16_t val) {
        high_register = (val >> 8) & 0xFF;
        low_register = val & 0xFF;
    }
    uint16_t BC() {
        return (registers[B] << 8) | registers[C];
    }
    uint16_t DE() {
        return (registers[D] << 8) | registers[E];
    }
    uint8_t F() {
        return (flag_z << 7) | (flag_n << 6) | (flag_h << 5) | (flag_c << 4);
    }

    uint8_t decode(uint8_t opcode, memory& mem) { //returns no of cycles took
        if (debug){cout << "EXECUTE OPCODE: " << hex << (int)opcode<< '\n';}
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
            case 0xE2: {
                auto address = 0xFF00+registers[C];
                mem.write(address, registers[A]);
                cycles=8;
                break;
            }
            // LDH A, [C]
            case 0xF2: {
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

                flag_c = (result > 0xFF);

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

                flag_c = (result > 0xFF);
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
                flag_h = (((registers[A] & 0x0F) + (n8 & 0x0F)) > 0x0F);

                flag_c = (result > 0xFF);
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
                uint16_t ogHL = HL();
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

                    case 3: // SP
                    // ADD HL, SP
                    result = HL() + SP;
                    second_op = SP;
                    break;
                }
                write_register_r16(registers[H], registers[L], result);
                flag_n = 0;
                flag_h = ((ogHL& 0x0FFF) + (second_op & 0x0FFF)) > 0x0FFF;
                flag_c = (result > 0xFFFF);
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
                auto r8 = (opcode >> 3) & 0x07;
                auto result = registers[r8] - 1;

                cycles=4;

                flag_z = (result == 0);
                flag_n = 1;
                flag_h = (registers[r8] & 0xF) == 0;
                registers[r8]-=1;
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
                uint8_t r8 = (opcode >> 3) & 0x07;
                uint8_t result = registers[r8] +1;
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
                // SUB A, n8
                uint8_t n8 = fetch(mem);
                uint8_t result = registers[A] - n8;
                cycles=8;
                flag_z = (result == 0);
                flag_n = 1;
                flag_h = (n8 & 0xF) > (registers[A] & 0xF);
                flag_c = n8 > registers[A];
                registers[A]=result;
                break;
            }
            case 0xC3: {
                // JP n16
                uint8_t low = fetch(mem);
                uint8_t high = fetch(mem);
                uint16_t n16 = ((high << 8) | low);

                PC = n16;
                cycles = 16;
                break;
            }

            case 0xCD: {
                // CALL n16
                uint8_t low = fetch(mem);
                uint8_t high = fetch(mem);
                uint16_t n16 = ((high << 8) | low);
                SP -= 2;
                mem.write(SP + 1, (PC >> 8) & 0xFF);
                mem.write(SP, PC & 0xFF);
                if (debug){
                cout << "CALL n16. SP: " << hex << SP 
                    << " pushing PC: " << PC 
                    << " to addresses: " << SP << " and " << (SP+1) << '\n';
                }PC = n16;
                cycles = 24;
                break;
            }

            case 0xC4:
            case 0xD4:
            case 0xCC:
            case 0xDC: {
                // CALL cc, n16
                if(debug){cout << "CALL cc, n16. SP: " << SP << '\n';}
                uint8_t cc = (opcode >> 3) & 0x03;
                bool condition_met = false;
                uint8_t low = fetch(mem);
                uint8_t high = fetch(mem);
                if (debug){cout << "flags: z,n,h,c: " << flag_z <<' '<< flag_n << ' ' << flag_h << ' ' << flag_c << '\n';}
                switch (cc) {
                    case 0: {
                        // NZ
                        condition_met = !flag_z;
                        break;
                    }
                    case 1: {
                        // Z
                        condition_met = flag_z;
                        break;
                    }
                    case 2: {
                        // NC
                        condition_met = !flag_c;
                        break;
                    }
                    case 3: {
                        // C
                        condition_met = flag_c;
                        break;
                    }
                }
                if (condition_met) {
                    // essentially just call n16
                    uint16_t n16 = ((high << 8) | low);
                    SP-=2;
                    mem.write(SP+1, (PC  >>8) & 0xFF); // high
                    mem.write(SP, PC & 0xFF); // low

                    PC = n16; // implicit JP n16
                    cycles=24;
                } else {
                    cycles=12;
                }
                break;
            }

            case 0xE9: {
                // JP HL
                cycles=4;
                PC=HL();
                break;
            }

            case 0xC2:
            case 0xD2:
            case 0xCA:
            case 0xDA: {
                // JP cc, n16
                uint8_t cc = (opcode >> 3) & 0x03;
                bool condition_met = false;
                uint8_t low = fetch(mem);
                uint8_t high = fetch(mem);
                switch (cc) {
                    case 0: {
                        // NZ
                        condition_met = !flag_z;
                        break;
                    }
                    case 1: {
                        // Z
                        condition_met = flag_z;
                        break;
                    }
                    case 2: {
                        // NC
                        condition_met = !flag_c;
                        break;
                    }
                    case 3: {
                        // C
                        condition_met = flag_c;
                        break;
                    }
                }
                if (condition_met) {
                    // essentially just JP n16
                    uint16_t n16 = ((high << 8) | low);

                    PC = n16;
                    cycles = 16;
                } else {
                    cycles=12;
                }
                break;
            }

            case 0x18: {
                // JR n16 (e8)
                int8_t offset = fetch(mem);
                PC+=offset;
                cycles=12;
                break;
            }

            case 0x20:
            case 0x30:
            case 0x28:
            case 0x38: {
                // JR cc, n16
                int8_t offset = fetch(mem);
                uint8_t cc = (opcode >> 3) & 0x03;
                bool condition_met = false;
                switch (cc) {
                    case 0:
                    // NZ
                    condition_met = !flag_z;
                    break;

                    case 1:
                    //Z
                    condition_met=flag_z;
                    break;

                    case 2:
                    //NC
                    condition_met=!flag_c;
                    break;

                    case 3:
                    // C
                    condition_met=flag_c;
                    break;
                }
                if (condition_met) {
                    PC+=offset;
                    cycles=12;
                } else {
                    cycles=8;
                }
                break;
            }

            case 0xC0:
            case 0xD0:
            case 0xC8:
            case 0xD8: {
                // RET CC
                if (debug) {cout << "RET CC. SP: " << SP << '\n';}
                uint8_t cc = (opcode >> 3) & 0x03; bool condition_met = false;
                switch (cc) {
                    case 0:
                    // NZ
                    condition_met = !flag_z;
                    break;

                    case 1:
                    // Z
                    condition_met = flag_z;
                    break;

                    case 2:
                    // NC
                    condition_met = !flag_c;
                    break;

                    case 3:
                    // C
                    condition_met = flag_c;
                    break;
                }
                if (condition_met) {
                    
                    uint8_t low = mem.read(SP);
                    uint8_t high = mem.read(SP + 1);
                    if (debug) {cout << "RET cc SP read from mem: "  <<low <<'\n';}
                    SP+=2;
                    PC=(high << 8) | low;
                    cycles=20;
                } else{
                    cycles= 8;
                }
                break;
            }

            case 0xC9: {
                // RET
                uint8_t low = mem.read(SP);
                uint8_t high = mem.read(SP + 1);
                SP += 2;
                PC = (high << 8) | low;
                if (debug) {
                cout << "RET. SP: " << hex << SP 
                    << " low: " << (int)low 
                    << " high: " << (int)high 
                    << " jumping to: " << PC << '\n';
                }cycles = 16;
                break;
            }

            case 0xFB: {
                // EI
                IME_pending = true;
                cycles=4;
                break;
            }

            case 0xD9: {
                // RETI
                if (debug) {cout << "RETI. SP: " << SP << '\n';}
                IME_pending = true;
                uint16_t addr = mem.read(PC);
                SP-=2;
                PC=addr;
                cycles=16;
                break;
            }

            
            case 0xC7:
            case 0xD7:
            case 0xE7:
            case 0xF7:
            case 0xCF:
            case 0xDF:
            case 0xEF:
            case 0xFF: {
                // RST vec
                if (debug) {cout << "RST vec. SP: " << SP << '\n';}
                uint8_t vec = opcode & 0x38;
                // essentially call vec
                SP-=2;
                mem.write(SP + 1, (PC >> 8) & 0xFF);  // high byte of PC
                mem.write(SP, PC & 0xFF); // low byte of PC


                PC = vec; // implicit JP n16
                cycles=16;
                break;

            }

            case 0xC1:
            case 0xD1:
            case 0xE1:
            case 0xF1: {
                // POP r16
                
                auto r16 = (opcode >> 4) & 0x03;

                switch (r16) {
                    case 0:
                    // BC
                    registers[C] = mem.read(SP);SP++;
                    registers[B] = mem.read(SP);SP++;
                    break;

                    case 1:
                    // DE
                    registers[E] = mem.read(SP);SP++;
                    registers[D] = mem.read(SP);SP++;

                    break;

                    case 2:
                    // HL
                    registers[L] = mem.read(SP);SP++;
                    registers[H] = mem.read(SP);SP++;
                    break;

                    case 3:
                    // POP AF
                    uint8_t f = mem.read(SP);
                    SP++;
                    registers[A] = mem.read(SP);
                    SP++;

                    flag_z = (f >>7) & 1;
                    flag_n = (f >> 6) &1;
                    flag_h = (f >> 5) &1;
                    flag_c = (f >> 4) &1; 
                    
                    break;
                }

                cycles=12;
                if (debug) {cout << "POP r16. SP: " << SP << '\n';}
                break;


            }

            case 0xC5:
            case 0xD5:
            case 0xE5:
            case 0xF5: {
                // PUSH r16
                if (debug) {cout << "PUSH. SP: " << SP << '\n';}
                uint16_t r16 = (opcode >> 4) & 0x03;
                uint8_t high, low;
                switch (r16) {
                    case 0:
                    // BC
                    high = registers[B];
                    low = registers[C];
                    break;

                    case 1:
                    // DE
                    high = registers[D];
                    low = registers[E];
                    break;

                    case 2:
                    // HL
                    high = registers[H];
                    low = registers[L];
                    break;
                    case 3:
                    // PUSH AF
                    high = registers[A];
                    low = (flag_z << 7) | (flag_n << 6) | (flag_h << 5) | (flag_c << 4);
                    break;
                }
                SP--;
                mem.write(SP, high);
                SP--;
                mem.write(SP, low);
                cycles=16;
                break;
            }

            case 0xA0 ... 0xA7: {
                // AND A, r8
                uint8_t r8 = opcode & 0x07;
                cycles=4;
                bool yo = false;
                if (opcode == 0xA6) {
                    // AND A, [HL]
                    r8 = mem.read(HL());
                    cycles=8;
                    yo=true;

                }
                uint8_t result;
                if (!yo) {
                    result = registers[r8] & registers[A];
                } else {
                    result = r8 & registers[A];
                }
                flag_z = (result==0);
                flag_n =0;
                flag_h = 1;
                flag_c = 0;
                registers[A]=result;

                break;
            }

            case 0xE6: {
                // AND A, n8
                uint8_t n8 = fetch(mem);
                auto result = n8 & registers[A];
                cycles=8;
                flag_z = (result==0);
                flag_n = 0;
                flag_h=1;
                flag_c=0;
                registers[A]=result;
                break;
            }

            case 0x2F: {
                // CPL
                registers[A]=~registers[A];
                cycles=4;
                flag_n = 1;
                flag_h = 1;
                break;
            }

            case 0xB0 ... 0xB7: {
                // OR A, r8
                auto r8 = opcode & 0x07;
                cycles=4;bool yo=false;
                if (opcode==0xB6) {
                    // OR A, [HL]
                    r8 = mem.read(HL());
                    cycles=8;
                    yo = true;
                }
                uint8_t result;
                if (!yo) {
                    result = registers[r8]|registers[A];
                } else {
                    result = r8|registers[A];
                }
                flag_z=(result==0);
                flag_n=0;
                flag_h=0;
                flag_c=0;

                registers[A]=result;
                break;
            }

            case 0xF6: {
                // OR A, n8
                uint8_t n8 = fetch(mem);
                cycles=8;
                auto result = n8|registers[A];
                flag_z=(result==0);
                flag_n=0;
                flag_h=0;
                flag_c=0;

                registers[A]=result;
                break;
            }
            case 0xA8 ... 0xAF: {
                // XOR A, r8
                auto r8 = opcode & 0x07;
                cycles=4;
                bool yo=false;
                if (opcode==0xAE) {
                    // XOR A, [HL]
                    r8 = mem.read(HL());
                    cycles=8;
                    yo=true;
                }
                uint8_t result;
                if (yo) {
                    result = r8^registers[A];
                } else {
                    result = registers[r8]^registers[A];

                }
                flag_z=(result==0);
                flag_n=0;
                flag_h=0;
                flag_c=0;

                registers[A]=result;
                break;
            }

            case 0xEE: {
                // XOR A, n8
                uint8_t n8 = fetch(mem);
                cycles=8;
                auto result = n8^registers[A];
                flag_z=(result==0);
                flag_n=0;
                flag_h=0;
                flag_c=0;

                registers[A]=result;
                break;
            }


            case 0xF3: {
                // DI
                IME=false;
                cycles=4;
                break;
            }

            case 0xF9: {
                // LD SP, HL
                SP = HL();
                cycles=8;
                break;
            }

            case 0x08: {
                // LD [n16], SP
                auto low = fetch(mem);
                auto high = fetch(mem);
                auto n16 = (high <<8) | low;

                mem.write(n16, SP);
                mem.write(n16+1, SP >> 8);

                cycles=20;
                break;
            }

            case 0x17: {
                // RLA
                uint8_t old_carry_flag = flag_c;
                uint8_t first_bit_A = registers[A] >> 7;
                uint8_t result = registers[A]<<1 | old_carry_flag;
                registers[A] = result;
                flag_c = first_bit_A;
                flag_z=0;
                flag_n=0;
                flag_h=0;
                cycles=4;
                break;
            }

            case 0x07: {
                // RLCA
                uint8_t old_carry_flag = flag_c;
                uint8_t first_bit_A = registers[A] >> 7;
                uint8_t result = registers[A]<<1 | first_bit_A;
                registers[A] = result;
                flag_c = first_bit_A;
                flag_z=0;
                flag_n=0;
                flag_h=0;
                cycles=4;
                break;
            }

            case 0x1F: {
                // RRA
                uint8_t old_flag_c = flag_c;
                uint8_t last_bit_of_r8;
                uint8_t result;


                last_bit_of_r8 = registers[A] &1;
                result = (registers[A] >> 1) | (old_flag_c << 7);
                cycles=4;
                registers[A]=result;

                flag_c = last_bit_of_r8;
                flag_z =0;
                flag_n=0;
                flag_h=0;
                break;
            }

            case 0x0F: {
                // RRCA
                uint8_t old_flag_c = flag_c;
                uint8_t last_bit_of_r8;
                uint8_t result;


                last_bit_of_r8 = registers[A] &1;
                result = (registers[A] >> 1) | (last_bit_of_r8 << 7);
                cycles=4;
                registers[A]=result;

                flag_c = last_bit_of_r8;
                flag_z =0;
                flag_n=0;
                flag_h=0;
                break;
            }

            case 0xCB: {
                // $CB prefix
                uint8_t opcode_cb = fetch(mem);
                auto r8 = opcode_cb & 0x7;
                switch (opcode_cb) {
                    case 0x40 ... 0x7F: {
                        // BIT u3,r8
                        auto u3 = (opcode >> 3) & 0x7;
                        bool yes;
                        if (r8==6) { // BIT u3, [HL]
                            bool yes = (mem.read(HL()) >> 3) & 0x1;
                            cycles=12;
                        } else {
                            bool yes = (registers[r8]>>u3) & 0x1;
                            cycles=8;
                        }
                        flag_z = (!yes);
                        flag_n = 0;
                        flag_h=1;
                        
                        break;
                    }

                    case 0x80 ... 0xBF: {
                        // RES u3, r8
                        auto u3 = (opcode >> 3) & 0x7;
                        if (r8==6) {
                            // RES u3, [HL]
                            auto result = mem.read(HL()) & ~(1 << u3);
                            mem.write(HL(), result);
                            cycles=16;
                        } else {
                            registers[r8] &= ~(1 << u3);
                            cycles=8;
                        }

                        break;
                    }

                    case 0xC0 ... 0xFF: {
                        //SET u3,r8
                        auto u3 = (opcode >> 3) & 0x7;
                        if (r8==6) {
                            // SET u3, [HL]
                            auto result = mem.read(HL()) & (1 << u3);
                            mem.write(HL(), result);
                            cycles=16;
                        } else {
                            registers[r8] &= (1 << u3);
                            cycles=8;
                        }

                        break;
                    }
                    case 0x10 ... 0x17: {
                        // RL r8
                        uint8_t old_flag_c = flag_c;
                        uint8_t first_bit_of_r8;
                        uint8_t result;
                        if (r8==6) {
                            // RL [HL]
                            first_bit_of_r8 = mem.read(HL()) >> 7;
                            cycles=16;
                            result = mem.read(HL()) << 1 | old_flag_c;
                            mem.write(HL(), result);
                        } else {
                            first_bit_of_r8 = registers[r8] >> 7;
                            result = registers[r8] << 1 |old_flag_c;
                            cycles=8;
                            registers[r8]=result;
                        }
                        flag_c = first_bit_of_r8;
                        flag_z = (result ==0);
                        flag_n=0;
                        flag_h=0;
                        break;
                    }

                    case 0x00 ... 0x07: {
                        // RLC r8
                        uint8_t old_flag_c = flag_c;
                        uint8_t first_bit_of_r8;
                        uint8_t result;
                        if (r8==6) {
                            // RLC [HL]
                            first_bit_of_r8 = mem.read(HL()) >> 7;
                            cycles=16;
                            result = mem.read(HL()) << 1 | first_bit_of_r8;
                            mem.write(HL(), result);
                        } else {
                            first_bit_of_r8 = registers[r8] >> 7;
                            result = registers[r8] << 1 |first_bit_of_r8;
                            cycles=8;
                            registers[r8]=result;
                        }
                        flag_c = first_bit_of_r8;
                        flag_z = (result ==0);
                        flag_n=0;
                        flag_h=0;
                        break;
                    }

                    case 0x18 ... 0x1F: {
                        // RR r8
                        uint8_t old_flag_c = flag_c;
                        uint8_t last_bit_of_r8;
                        uint8_t result;
                        if (r8==6) {
                            // RR [HL]
                            last_bit_of_r8 = mem.read(HL()) & 1;
                            cycles=16;
                            result = mem.read(HL()) >> 1 | (old_flag_c<<7);
                            mem.write(HL(), result);
                        } else {
                            last_bit_of_r8 = registers[r8] &1;
                            result = (registers[r8] >> 1) | (old_flag_c << 7);
                            cycles=8;
                            registers[r8]=result;
                        }
                        flag_c = last_bit_of_r8;
                        flag_z = (result ==0);
                        flag_n=0;
                        flag_h=0;
                        break;
                    }
                    case 0x08 ... 0x0F: {
                        // RRC r8
                        uint8_t old_flag_c = flag_c;
                        uint8_t last_bit_of_r8;
                        uint8_t result;
                        if (r8==6) {
                            // RRC [HL]
                            last_bit_of_r8 = mem.read(HL()) & 1;
                            cycles=16;
                            result = mem.read(HL()) >> 1 | (last_bit_of_r8<<7);
                            mem.write(HL(), result);
                        } else {
                            last_bit_of_r8 = registers[r8] &1;
                            result = (registers[r8] >> 1) | (last_bit_of_r8 << 7);
                            cycles=8;
                            registers[r8]=result;
                        }
                        flag_c = last_bit_of_r8;
                        flag_z = (result ==0);
                        flag_n=0;
                        flag_h=0;
                        break;
                    }

                    case 0x20 ... 0x27: {
                        // SLA r8
                        uint8_t first_bit_of_r8;
                        uint8_t result;
                        if (r8==6) {
                            // SLA [HL]
                            first_bit_of_r8 = mem.read(HL()) >> 7;
                            cycles=16;
                            result = mem.read(HL()) << 1;
                            mem.write(HL(), result);
                        } else {
                            first_bit_of_r8 = registers[r8] >> 7;
                            result = registers[r8] << 1;
                            cycles=8;
                            registers[r8]=result;
                        }
                        flag_c = first_bit_of_r8;
                        flag_z = (result ==0);
                        flag_n=0;
                        flag_h=0;
                        break;
                    }

                    case 0x28 ... 0x2F: {
                        // SRA r8
                        uint8_t last_bit_of_r8;
                        uint8_t result;
                        if (r8==6) {
                            // SRA [HL]
                            last_bit_of_r8 = mem.read(HL()) & 1;
                            cycles=16;
                            result = mem.read(HL()) >> 1 | (mem.read(HL()) & 0x80);
                            mem.write(HL(), result);
                        } else {
                            last_bit_of_r8 = registers[r8] &1;
                            result = (registers[r8] >> 1) | (registers[r8] & 0x80);
                            cycles=8;
                            registers[r8]=result;
                        }
                        flag_c = last_bit_of_r8;
                        flag_z = (result ==0);
                        flag_n=0;
                        flag_h=0;
                        break;
                    }

                    case 0x38 ... 0x3F: {
                        // SRL r8
                        uint8_t last_bit_of_r8;
                        uint8_t result;
                        if (r8==6) {
                            // SRL [HL]
                            last_bit_of_r8 = mem.read(HL()) & 1;
                            cycles=16;
                            result = mem.read(HL()) >> 1;
                            mem.write(HL(), result);
                        } else {
                            last_bit_of_r8 = registers[r8] &1;
                            result = (registers[r8] >> 1);
                            cycles=8;
                            registers[r8]=result;
                        }
                        flag_c = last_bit_of_r8;
                        flag_z = (result ==0);
                        flag_n=0;
                        flag_h=0;
                        break;
                    }

                    case 0x30 ... 0x37: {
                        // SWAP r8
                        uint8_t result;
                        if (r8==6) {
                            // SWAP [HL]
                            result = ((mem.read(HL() & 0x0F) << 4) | ((mem.read(HL() & 0xF0 >> 4))));
                            mem.write(HL(), result);
                            cycles=16;
                        } else {
                            result = ((mem.read(HL() & 0x0F) << 4) | ((mem.read(HL() & 0xF0) >> 4)));
                            registers[r8] = result;
                            cycles=8;
                        }
                        flag_z = (result ==0);
                        flag_n = 0;
                        flag_h = 0;
                        flag_c=0;
                        break;
                    }
                }
            }





            


        }
        return cycles;
    }
};

int main() {
    auto rom_path = "./individual/01-special.gb";
    bool doctor = true;
    cpu gb_cpu;
    memory mem;
    // boot stuff
    gb_cpu.registers[gb_cpu.A] = 0x01;
    gb_cpu.registers[gb_cpu.B] = 0x00;
    gb_cpu.registers[gb_cpu.C] = 0x13;
    gb_cpu.registers[gb_cpu.D] = 0x00;
    gb_cpu.registers[gb_cpu.E] = 0xD8;
    gb_cpu.registers[gb_cpu.H] = 0x01;
    gb_cpu.registers[gb_cpu.L] = 0x4D;
    gb_cpu.SP = 0xFFFE;
    gb_cpu.PC = 0x0100;
    gb_cpu.flag_z =1;
    gb_cpu.flag_n=0;
    gb_cpu.flag_h=1;
    gb_cpu.flag_c=1;

    mem.loadROM(rom_path);
    while (true) {
        if (gb_cpu.halted) {
            break;
        }
        if (doctor) {
            cout << uppercase << hex
                << "A:" << setw(2) << setfill('0') << (int)gb_cpu.registers[gb_cpu.A]
                << " F:" << setw(2) << setfill('0') << (int)gb_cpu.F()
                << " B:" << setw(2) << setfill('0') << (int)gb_cpu.registers[gb_cpu.B]
                << " C:" << setw(2) << setfill('0') << (int)gb_cpu.registers[gb_cpu.C]
                << " D:" << setw(2) << setfill('0') << (int)gb_cpu.registers[gb_cpu.D]
                << " E:" << setw(2) << setfill('0') << (int)gb_cpu.registers[gb_cpu.E]
                << " H:" << setw(2) << setfill('0') << (int)gb_cpu.registers[gb_cpu.H]
                << " L:" << setw(2) << setfill('0') << (int)gb_cpu.registers[gb_cpu.L]
                << " SP:" << setw(4) << setfill('0') << (int)gb_cpu.SP
                << " PC:" << setw(4) << setfill('0') << (int)gb_cpu.PC
                << " PCMEM:" << setw(2) << setfill('0') << (int)mem.read(gb_cpu.PC)
                << "," << setw(2) << setfill('0') << (int)mem.read(gb_cpu.PC+1)
                << "," << setw(2) << setfill('0') << (int)mem.read(gb_cpu.PC+2)
                << "," << setw(2) << setfill('0') << (int)mem.read(gb_cpu.PC+3)
                << '\n';
        }
        gb_cpu.decode(gb_cpu.fetch(mem), mem);
        if (debug) {cout << "Register B: " << (int)gb_cpu.registers[gb_cpu.B] << " and L: " << (int)gb_cpu.registers[gb_cpu.L] << '\n'; }

        if (gb_cpu.IME_pending) {
            gb_cpu.IME=true;
            gb_cpu.IME_pending=false;
        }
    }

}