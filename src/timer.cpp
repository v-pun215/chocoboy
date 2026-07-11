#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include "timer.h"

using namespace std;

static inline bool bit12(uint16_t cntr) {
    return (cntr>>12)&1;
}

static inline bool TAC_bit(uint16_t cntr, uint8_t tac) {
    switch (tac & 0x03) {
        case 0:
        return (cntr>>9) & 1;

        case 1:
        return (cntr>>3) & 1;

        case 2:
        return (cntr>>5)&1;

        case 3:
        return (cntr>>7)&1;
    }
    return false;

}

int timer::handle_timer(uint8_t cycle, uint8_t& IF) {
    int fs_clucks = 0;
    bool tac_enabled = (TAC>>2)&1;

    for (uint8_t i = 0; i < cycle; i++) {
        bool prev_bit12= bit12(div_counter);
        bool prev_tacbit= tac_enabled && TAC_bit(div_counter, TAC);

        div_counter++;

        if (prev_bit12 && !bit12(div_counter)) {
            fs_clucks++;
        }
        if (prev_tacbit && !(tac_enabled && TAC_bit(div_counter, TAC))) {
            TIMA++;
            if (TIMA == 0) {
                IF |= 4;
                TIMA = TMA;
            }
        }
    }
    return fs_clucks;
}

bool timer::reset_div(uint8_t& IF) {
    bool is_bit2_set = bit12(div_counter);
    bool is_tacb_set = ((TAC >>2)& 1) && TAC_bit(div_counter,TAC);

    div_counter = 0;

    if (is_tacb_set) {
        TIMA++;
        if (TIMA==0) {
            IF|=4;
            TIMA=TMA;
        }
    }
    return is_bit2_set;
}