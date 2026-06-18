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

    int div_clocksum = 0;
    int timer_clocksum = 0;
    void handle_timer(uint8_t cycle, uint8_t& IF);

};