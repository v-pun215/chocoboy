#include <iostream>
#include "audio.h"
#include <SDL.h>
#include <math.h>

void APU::clock_frame_squencer() {
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

void APU::update(uint8_t cycles) {
    bool apu_enabled = (audio_master_control>>7)&1;
    if (apu_enabled){

        ch1.tick_freq_tmr(cycles);
        ch2.tick_freq_tmr(cycles);
        ch3.tick_freq_tmr(cycles);
        ch4.tick_freq_tmr(cycles);

    }
    cycle_add+=cycles;
    while(cycle_add>=cyclers_per_sample) {
        cycle_add-=cyclers_per_sample;
        push_sample();
    }
}

void APU::push_sample() {
    if (frame_sample_count + 1 < buffer_size) {
        int16_t l, r;
        mix(l, r);
        sample_buff[frame_sample_count]     = l;
        sample_buff[frame_sample_count + 1] = r;
        frame_sample_count += 2;
    }
}

// audio.cpp
void APU::mix(int16_t& left, int16_t& right) {
    uint8_t ch1_out = ch1.output();
    uint8_t ch2_out = ch2.output();
    uint8_t ch3_out = ch3.output();
    uint8_t ch4_out = ch4.output();

    int16_t ch1_signal = (int16_t)ch1_out - 7;
    int16_t ch2_signal = (int16_t)ch2_out - 7;
    int16_t ch3_signal = (int16_t)ch3_out - 7;
    int16_t ch4_signal = (int16_t)ch4_out - 7;

    int32_t left_mix=0,right_mix=0;

    if (ch_left(1)) left_mix += ch1_signal;
    if (ch_left(2)) left_mix+= ch2_signal;
    if (ch_left(3)) left_mix += ch3_signal;
    if (ch_left(4)) left_mix += ch4_signal;

    if (ch_right(1)) right_mix+= ch1_signal;
    if (ch_right(2)) right_mix+= ch2_signal;
    if (ch_right(3)) right_mix+= ch3_signal;
    if (ch_right(4)) right_mix+= ch4_signal;

    left_mix *= (left_vol()+1);
    right_mix *= (right_vol()+1);

    left_mix *= 16;
    right_mix *= 16;

    if (left_mix > 32767) left_mix = 32767;
    if (left_mix < -32768) left_mix = -32768;
    if (right_mix > 32767) right_mix = 32767;
    if (right_mix < -32768) right_mix = -32768;

    left  = (int16_t)left_mix;
    right = (int16_t)right_mix;
}