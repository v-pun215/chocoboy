#include <iostream>
#include "audio.h"
#include <SDL.h>
#include <math.h>


void APU::update(uint8_t cycles) {
    bool apu_enabled = (audio_master_control>>7)&1;
    if (apu_enabled){

        ch1.tick_freq_tmr(cycles);
        ch2.tick_freq_tmr(cycles);

        //512hz = 8192 cycles
        frame_squencer_cycles+=cycles;
        while(frame_squencer_cycles>=8192) {
            frame_squencer_cycles-=8192;
            switch (frame_squencer_step%8) {
                case 0:
                case 4:
                ch1.tick_length();
                ch2.tick_length();
                break;

                case 2:
                case 6:
                ch1.tick_length();
                ch1.tick_sweep();
                ch2.tick_length();
                break;

                case 7:
                ch1.tick_env();
                ch2.tick_env();
                break;
            }
            frame_squencer_step++;
        }
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
    uint8_t ch1_out = ch1.output();
    uint8_t ch2_out = ch2.output();

    uint8_t combined = ch1_out + ch2_out;

    return (int16_t)(combined*100-1500);
}