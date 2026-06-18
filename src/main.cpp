#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include "globals.h"
#include "timer.h"
#include "memory.h"
#include "cpu.h"
using namespace std;








void doctor_print(cpu& gb_cpu, memory& mem) {
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


int main(int argc, char* argv[]) {
    bool doctor = false;
    if (argc <2) {
        cout << "USAGE: ROM_PATH [SERIAL/DOCTOR]\n";
        exit(EXIT_FAILURE);
    }
    std::string mode = argv[2]; // Converts char* to std::string for safe comparison
    if (mode == "SERIAL") {
        serial = true;
    } else if (mode == "DOCTOR") {
        doctor = true;
    }
    auto rom_path = argv[1];
    
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
        if (!gb_cpu.halted) { 
            if (doctor) {doctor_print(gb_cpu, mem);}
            uint8_t cycles = gb_cpu.decode(gb_cpu.fetch(mem), mem);
            mem.tmr.handle_timer(cycles, mem.IF);
        } else {
            mem.tmr.handle_timer(4, mem.IF);
            if ((mem.IE & mem.IF) !=0) {
                gb_cpu.halted=false;
            }
        }
        gb_cpu.handle_interrupts(mem);
    }
}