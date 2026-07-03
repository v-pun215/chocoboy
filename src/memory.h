#pragma once
#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include "timer.h"
#include "joypad.h"
#include "ppu.h"
#include "debugger.h"

using namespace std;

struct memory {
    debugger debugging;
    joypad joy;
    timer tmr;
    PPU ppu;
    array<uint8_t, 2097152> ROM = {}; // 2MiB combined ROM
    array<uint8_t, 256> boot_ROM={};
    bool boot_enabled = true;
    // mbc
    uint8_t rom_bank = 1;
    uint8_t ram_bank = 0;
    bool ram_enabled = false;
    bool mbc1_mode = false;
    array<uint8_t, 8192> VRAM = {}; // 8KB VRAM
    array<uint8_t, 8192> ERAM = {}; // 8KB External RAM (local game storage?)

    array<uint8_t, 8192> WRAM = {}; // 8KB Work RAM (actual runtime meory)

    array<uint8_t, 160> OAM = {}; // object attribute memory (sprites and stuff)

    array<uint8_t, 127> HRAM = {}; // high ram (0-0)

    uint8_t IE = 0; // Interrupt Enable register
    uint8_t IF = 0;// Interrupt flag

    uint8_t read_ROM(uint16_t address);
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t content);
    void loadROM(string path);
    void boot(string boot_path);
};