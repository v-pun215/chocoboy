#include <iostream>
#include "audio.h"
#include <SDL.h>
#include <math.h>


void APU::update(uint8_t cycles) {
    bool apu_enabled = (audio_master_control>>7)&1;
    if (!apu_enabled){
        return;
    }

    ch2.tick_freq_tmr(cycles);

    //512hz = 8192 cycles
    frame_squencer_cycles+=cycles;
    while(frame_squencer_cycles>=8192) {
        frame_squencer_cycles-=8192;
        switch (frame_squencer_step%8) {
            case 0:
            case 2:
            case 4:
            case 6:
            ch2.tick_length();
            break;

            case 7:
            ch2.tick_env();
            break;
        }
        frame_squencer_step++;
    }

    cycle_add+=cycles;
    while(cycle_add>=cyclers_per_sample) {
        cycle_add-=cyclers_per_sample;
        push_sample();
    }
}

void APU::push_sample() {
    if (frame_sample_count < buffer_size) {

        sample_buff[frame_sample_count] = mix();
        frame_sample_count++;
    }
}

int16_t APU::mix() {
    uint8_t ch2_out = ch2.output();
    return (int16_t)(ch2_out*200-1500);
}