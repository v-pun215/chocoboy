#pragma once
#include <cstdint>
using namespace std;

struct joypad {
    bool A=false;
    bool B=false;
    bool select=false;
    bool start=false;
    bool up=false;
    bool down=false;
    bool left=false;
    bool right=false;

    // bits 5,4
    bool select_btns=false;
    bool select_dpad=false;

    void press_btn(bool& btn, bool pressed);
    void write_selected(uint8_t val);
    uint8_t read_joyp();
};