#pragma once
#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include "memory.h"
using namespace std;
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
    int IME_pending = 0; // next instruction IME enable;
    bool halted = false;

    // Flags register
    bool flag_z = 0; // zero flag
    bool flag_n = 0; // subtraction flag (BCD)
    bool flag_h = 0; // half carry flag (BCD)
    bool flag_c = 0; // carry flag

    uint8_t fetch(memory& mem);
    uint16_t HL();
    void write_HL(uint16_t val);
    void write_register_r16(uint8_t& high_register, uint8_t& low_register, uint16_t val);
    uint16_t BC();
    uint16_t DE();
    uint8_t F();
    uint8_t decode(uint8_t opcode, memory& mem);
    void handle_interrupts(memory& mem);
};