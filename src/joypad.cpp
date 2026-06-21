#include <cstdint>
#include <iostream>
#include "joypad.h"

using namespace std;

void joypad::press_btn(bool& btn, bool pressed) {
    btn = pressed;
}

void joypad::write_selected(uint8_t val) {
    select_btns=(val>>5)&1;
    select_dpad=(val>>4)&1;
}

uint8_t joypad::read_joyp() {
    uint8_t JOYP = 0xFF;
    

    if (!select_btns) {
        JOYP&=~0x20;
        if (A) {
            JOYP&=~1;
        }
        if (B) {
            JOYP&=~0x2;
        }
        if (select) {
            JOYP&=~0x4;
        }
        if (start) {
            JOYP&=~0x8;
        }
    }
    if (!select_dpad) {
        JOYP&=~0x10;
        if (right) {
            JOYP&=~1;
        }
        if (left) {
            JOYP&=~0x2;
        }
        if (up) {
            JOYP&=~0x4;
        }
        if (down) {
            JOYP&=~0x8;
        }
    }
    return JOYP;
}