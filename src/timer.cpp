#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include "timer.h"

using namespace std;


void timer::handle_timer(uint8_t cycle, uint8_t& IF) {
    // DIV is set
    div_clocksum+=cycle;
    if (div_clocksum >= 256) {
        div_clocksum-=256;
        DIV++;
    }

    // check if timer on
    if ((TAC >> 2) & 0x1) {
        timer_clocksum += cycle;

        int freq = 4096;//hz
        if ((TAC & 3) == 1) {
            freq = 262144;
        } else if ((TAC & 3) == 2) {
            freq = 65536;
        } else if ((TAC & 3) == 3) {
            freq = 16384;
        }

        while (timer_clocksum>=(4194304/freq)) {
            TIMA++;

            if (TIMA == 0) {
                IF = IF | 4;
                TIMA = TMA; 
            }
            timer_clocksum-=(4194304 / freq);
        }
    }
}
