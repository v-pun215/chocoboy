#include <array>
#include <imgui.h>
#include "ppu.h"
#include "cpu.h"
#include "memory.h"
#include "debugger.h"
#include <tuple>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <functional>
#include <cstdint>

void render_cpu(memory& mem, cpu& cpu, bool& paused, bool& step) {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200.0f, 450.0f), ImGuiCond_Always);
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    ImGui::Begin("CPU", nullptr, window_flags);

    const char* text = paused ? "Resume" : "Pause";
    if (ImGui::Button(text)) {
        paused = !paused;
    }
    ImGui::SameLine();
    if (ImGui::Button("Step")) {
        step = !step;
    }
    ImGui::Text("Previous opcode: 0x%X", mem.read(cpu.PC-1));
    ImGui::Text("Current opcode: 0x%X", mem.read(cpu.PC));
    ImGui::Text("Next opcode: 0x%X", mem.read(cpu.PC+1));
    ImGui::Text("Cycles: %d", cpu.all_cycles);
    ImGui::Text("IME: %d", cpu.IME);
    ImGui::Text("Halted: %d", cpu.halted);
    array<tuple<const char*, function<uint16_t()>>, 10> registers {{
        {"A", [&cpu]() { return cpu.registers[cpu.A]; }},
        {"F", [&cpu]() { return cpu.F(); }},
        {"B", [&cpu]() { return cpu.registers[cpu.B]; }},
        {"C", [&cpu]() { return cpu.registers[cpu.C]; }},
        {"D", [&cpu]() { return cpu.registers[cpu.D]; }},
        {"E", [&cpu]() { return cpu.registers[cpu.E]; }},
        {"H", [&cpu]() { return cpu.registers[cpu.H]; }},
        {"L", [&cpu]() { return cpu.registers[cpu.L]; }},
        {"SP", [&cpu]() { return cpu.SP; }},
        {"PC", [&cpu]() { return cpu.PC; }}
    }};
    array<tuple<const char*, function<uint8_t()>>, 10> flags {{
        {"Z", [&cpu]() { return cpu.flag_z; }},
        {"N", [&cpu]() { return cpu.flag_n; }},
        {"H", [&cpu]() { return cpu.flag_h; }},
        {"C", [&cpu]() { return cpu.flag_c; }},
    }};
    if (ImGui::BeginTable("cpu_registers", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Register");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();
        for (int row = 0; row < 10; row++)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text(get<0>(registers[row]), row);

            ImGui::TableSetColumnIndex(1);
            auto& [name, getter] = registers[row];
            ImGui::Text("%s: 0x%04X", name, getter());

        }
        ImGui::EndTable();
    }


    if (ImGui::BeginTable("flags", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Flag");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();



        for (int row = 0; row < 4;row++) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text(get<0>(flags[row]), row);

            ImGui::TableSetColumnIndex(1);
            auto& [name, getter] = flags[row];
            ImGui::Text("%s: 0x%04X", name, getter());
        }
        ImGui::EndTable();
    }
    ImGui::End();
    ImGui::Render();
}

void debugger::render_debugger(memory& mem, cpu& cpu, bool& paused, bool& step) {
    // cpu
    render_cpu(mem, cpu, paused, step);
    
}
