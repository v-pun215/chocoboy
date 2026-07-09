#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include "globals.h"
#include "timer.h"
#include <chrono>
#include "memory.h"
#include "cpu.h"
#include "ppu.h"
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
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
        cout << "USAGE: chocoboy ROM_PATH [SERIAL/DOCTOR]\n";
        exit(EXIT_FAILURE);
    }
    if (argc>2) {
        std::string mode = argv[2]; // Converts char* to std::string for safe comparison
        if (mode == "SERIAL") {
            serial = true;
        } else if (mode == "DOCTOR") {
            doctor = true;
        }
    }
    auto rom_path = argv[1];
    
    cpu gb_cpu;
    memory mem;
    // boot stuff
    /*
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
    gb_cpu.flag_c=1;*/
    mem.boot("./roms/dmg_boot.bin");
    mem.loadROM(rom_path);
    mem.ppu.initSDL(mem);
    //PPU ppu;
    //ppu.initSDL();
    int all_cycles=0;
    bool paused = false;
    bool step = false;
    auto t_start = std::chrono::high_resolution_clock::now();
    auto t_start_cpu = std::chrono::high_resolution_clock::now();
    int cycles_per_second = 0;

    while (true) {
        if (paused) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT) exit(0);
            }


            //mem.debugging.render_debugger(mem, gb_cpu, paused, step);

            SDL_RenderClear(mem.ppu.renderer);
            SDL_UpdateTexture(mem.ppu.texture, NULL, mem.ppu.framebuffer, 160*3);
            SDL_RenderCopy(mem.ppu.renderer, mem.ppu.texture, NULL, NULL);
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), mem.ppu.renderer);
            SDL_RenderPresent(mem.ppu.renderer);

            SDL_Delay(16);
            if (!step) continue;
        }
        if (step) {
            if (paused) {
                //cout << "paused and step is true\n";
                step = false;
            } else {
                step = false;
                paused = true;
            }
        }
        uint8_t cycles = 0;
        //cout << "LY: " << (int)mem.ppu.LY << '\n';
        if (!gb_cpu.halted) { 
            if (doctor) {doctor_print(gb_cpu, mem);}
            cycles = gb_cpu.decode(gb_cpu.fetch(mem), mem);

        } else {
            cycles=4;
            
            if ((mem.IE & mem.IF) !=0) {
                gb_cpu.halted=false;
            }
        }
        all_cycles+=cycles;
        cycles_per_second+=cycles;
        gb_cpu.all_cycles+=cycles;
        mem.tmr.handle_timer(cycles, mem.IF);
        mem.ppu.update(cycles, mem, gb_cpu, paused, step);
        mem.apu.update(cycles);
        gb_cpu.handle_interrupts(mem);



        auto now = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(now - t_start_cpu).count();

        if (elapsed >= 1000000) { // 1 second in microseconds
            // Calculate how many cycles *should* have run in this exact timeframe
            double target_cycles = 4194304.0 * (elapsed / 1000000.0);
            
            cout << "True Emulation Speed: " << (cycles_per_second / target_cycles) * 100.0 << "%\n";
            
            cycles_per_second = 0;
            t_start_cpu = now;
        }
    }
}