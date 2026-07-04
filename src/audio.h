#pragma once

#include <iostream>
#include <cstdint>
#include <array>

using namespace std;

struct APU {
    /*global common registers*/
    // NR52
    uint8_t audio_master_control = 0;

    // NR51
    uint8_t sound_panning = 0;

    // NR50
    uint8_t master_volume_vin = 0; 

    struct channel_1 {
        uint8_t sweep = 0; // nr10
        uint8_t len_duty = 0; // nr11
        uint8_t vol_env = 0; // nr12
        uint8_t period_low = 0; // nr13
        uint8_t period_high_ctrl = 0; // nr14
    };

    struct channel_2 {
        uint8_t len_duty = 0; // nr21
        uint8_t vol_env = 0; // nr22
        uint8_t period_low = 0; // nr23
        uint8_t period_high_ctrl = 0; // nr24
    };

    struct channel_3 {
        uint8_t dac = 0; // nr30
        uint8_t len_timer = 0; // nr31
        uint8_t output_level = 0; // nr32
        uint8_t period_low = 0; // nr33
        uint8_t period_high_ctrl = 0; // nr34
        array<uint8_t, 16> wave_ram{}; 
    };

    struct channel_4 {
        uint8_t len_timer = 0; // nr41
        uint8_t vol_env = 0; // nr42
        uint8_t freq_rand = 0; // nr43
        uint8_t control = 0;
    };


};