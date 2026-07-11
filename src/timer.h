#pragma once
#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

using namespace std;


struct timer {
    uint8_t DIV = 0;
    uint8_t TIMA = 0;
    uint8_t TMA = 0;
    uint8_t TAC = 0;

    int div_counter = 0;
    uint8_t get_DIV() const {
        return div_counter >> 8;
    }

    int handle_timer(uint8_t cycle, uint8_t& IF);

    bool reset_div(uint8_t& IF); // returns true if reset cros frame-squencer edge

};