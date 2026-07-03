#pragma once
#include <array>
#include <imgui.h>
#include "ppu.h"
#include "cpu.h"
#include "memory.h"

struct debugger {
    void render_debugger(memory& mem, cpu& cpu, bool& paused, bool& step);
};