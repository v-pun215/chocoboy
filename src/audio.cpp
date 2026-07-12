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

    for (int i=0;i<cycles;i++) {
        if (apu_enabled){
            ch1.tick_freq_tmr(1);
            ch2.tick_freq_tmr(1);
            ch3.tick_freq_tmr(1);
            ch4.tick_freq_tmr(1);

        }
        acc1+=ch1.dac_enabled ? (int32_t)ch1.output() -7 :0;
        acc2+=ch2.dac_enabled ? (int32_t)ch2.output() -7 :0;
        acc3+=ch3.dac_enabled ? (int32_t)ch3.output() -7 :0;
        acc4+=ch4.dac_enabled ? (int32_t)ch4.output() -7 :0;
        acc_n++;
        cycle_add++;

        if (cycle_add>=cyclers_per_sample){
            cycle_add-=cyclers_per_sample;
            push_sample();
        }
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
    float ch1_avg = acc_n > 0 ? (float)acc1 / acc_n : 0.0f;
    float ch2_avg = acc_n > 0 ? (float)acc2 / acc_n : 0.0f;
    float ch3_avg = acc_n > 0 ? (float)acc3 / acc_n : 0.0f;
    float ch4_avg = acc_n > 0 ? (float)acc4 / acc_n : 0.0f;
    acc1 = acc2 = acc3 = acc4 = 0;
    acc_n = 0;

    bool any_dac_on = ch1.dac_enabled || ch2.dac_enabled || ch3.dac_enabled || ch4.dac_enabled;

    float left_mix=0.0f,right_mix=0.0f;

    if (ch_left(1)) left_mix += ch1_avg;
    if (ch_left(2)) left_mix+= ch2_avg;
    if (ch_left(3)) left_mix += ch3_avg;
    if (ch_left(4)) left_mix += ch4_avg;

    if (ch_right(1)) right_mix+= ch1_avg;
    if (ch_right(2)) right_mix+= ch2_avg;
    if (ch_right(3)) right_mix+= ch3_avg;
    if (ch_right(4)) right_mix+= ch4_avg;

    left_mix *= (left_vol()+1);
    right_mix *= (right_vol()+1);

    float left_in = (float)left_mix;
    float right_in = (float)right_mix;

    const float charge_factor = 0.996f;
    float left_out, right_out;
    if (any_dac_on) {
        left_out = left_in - hpf_prev_in_l + charge_factor*hpf_prev_out_l;
        hpf_prev_in_l=left_in;
        hpf_prev_out_l=left_out;

        right_out = right_in - hpf_prev_in_r + charge_factor*hpf_prev_out_r;
        hpf_prev_in_r=right_in;
        hpf_prev_out_r=right_out;
    } else {
        left_out = 0.0f;
        right_out =0.0f;
        hpf_prev_in_l=hpf_prev_out_l=0.0f;
        hpf_prev_in_r=hpf_prev_out_r=0.0f;
    }
    float left_scaled = left_out*50.0f;
    float right_scaled = right_out*50.0f;

    if (left_scaled>32767.0f) {
        left_scaled=32767.0f;
    }
    if (left_scaled<-32768.0f) {
        left_scaled=-32768.0f;
    }
    if (right_scaled>32767.0f) {
        right_scaled=32767.0f;
    }
    if (right_scaled<-32768.0f) {
        right_scaled=-32768.0f;
    }

    left = (int16_t)left_scaled;
    right = (int16_t)right_scaled;
}