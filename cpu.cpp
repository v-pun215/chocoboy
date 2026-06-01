#include <bits/stdc++.h>

using namespace std;

struct GB_CPU { // 8-bit custom Sharp LR35902 processor
    enum registerNames {
        AF,
        BC,
        DE,
        HL,
        SP, // stack pointer
        PC // program counter
    };

    array<uint16_t, 6> registers = {};
};