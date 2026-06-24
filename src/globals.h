#pragma once
#include <string>
using namespace std;
struct memory;
extern bool debug;
extern bool serial;
extern bool boot_rom_finished;
extern void dump_vram(memory& mem, const string& file);