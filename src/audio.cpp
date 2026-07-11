#include <iostream>
#include "audio.h"
#include <SDL.h>
#include <math.h>


void APU::update(uint8_t cycles) {
    bool apu_enabled = (audio_master_control>>7)&1;
    if (apu_enabled){

        ch1.tick_freq_tmr(cycles);
        ch2.tick_freq_tmr(cycles);
        ch3.tick_freq_tmr(cycles);
        ch4.tick_freq_tmr(cycles);

        //512hz = 8192 cycles
        frame_squencer_cycles+=cycles;
        while(frame_squencer_cycles>=8192) {
            frame_squencer_cycles-=8192;
            switch (frame_squencer_step%8) {
                case 0:
                case 4:
                ch1.tick_length();
                ch2.tick_length();
                ch3.tick_length();
                ch4.tick_length();
                break;

                case 2:
                case 6:
                ch1.tick_length();
                ch1.tick_sweep();
                ch2.tick_length();
                ch3.tick_length();
                ch4.tick_length();
                break;

                case 7:
                ch1.tick_env();
                ch2.tick_env();
                ch4.tick_env();
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
    uint8_t ch3_out = ch3.output();
    uint8_t ch4_out = ch4.output();

    int16_t ch1_signal = (int16_t)ch1_out - 7;
    int16_t ch2_signal = (int16_t)ch2_out - 7;
    int16_t ch3_signal = (int16_t)ch3_out - 7;
    int16_t ch4_signal = ((int16_t)ch4_out - 7);

    int32_t mixed = ch1_signal + ch2_signal + ch3_signal +ch4_signal;

    mixed *=125;

    if (mixed > 32767)  mixed = 32767;
    if (mixed < -32768) mixed = -32768;

    return (int16_t)mixed;
}