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

        bool enabled = false;
        int frequency_timer = 0;
        int duty_pos = 0;
        int currn_vol = 0;
        int envelope_tmr = 0;
        int len_counter = 0;
        int shadow_period = 0;
        int sweep_tmr = 0;
        bool sweep_enabled=false;


        int get_period() {
            return period_low | ((period_high_ctrl & 0x07) << 8);
        }

        int get_duty_cyc_index() {
            return (len_duty >> 6) & 0x03;
        }

        int initial_vol() {
            return (vol_env >> 4) & 0x0f;
        }

        int envelope_dir() {
            return (vol_env >> 3) & 0x01;
        }

        int envelope_period() {
            return vol_env&0x07;
        }

        int len_enable() {
            return (period_high_ctrl >>6) & 1;
        }

        uint8_t output() {
            if (!enabled) {
                return 0;
            }
            const uint8_t duty_table[4][8] = {
                {0,0,0,0,0,0,0,1},
                {0,0,0,0,0,0,1,1},
                {0,0,0,0,1,1,1,1},
                {1,1,1,1,1,1,0,0}
            };

            return duty_table[get_duty_cyc_index()][duty_pos] ? currn_vol : 0;
        }

        void trigger() {
            enabled=true;
            frequency_timer=(2048-get_period()) * 4;
            currn_vol = initial_vol();
            envelope_tmr = envelope_period();
            if (len_counter==0) {
                len_counter=64;
            }
            // dac check
            if ((vol_env & 0xF8) == 0) {
                enabled=false;
            }

            shadow_period=get_period();
        }

        void tick_freq_tmr(int cycles) {
            frequency_timer-=cycles;
            while (frequency_timer <=0) {
                frequency_timer+=(2048 - get_period()) * 4;
                duty_pos = (duty_pos+1)%8;
            }

        }

        void tick_length() {
            if (len_enable() && len_counter>0) {
                len_counter--;
                if (len_counter==0){
                    enabled=false;
                }
            }
        }

        void tick_env() {
            if (envelope_period() ==0) {
                return;
            }

            if (envelope_tmr>0) {
                envelope_tmr--;
            }
            if (envelope_tmr==0) {
                envelope_tmr=envelope_period();
                if (envelope_period() ==1 && currn_vol<15) {
                    currn_vol++;
                } else if (envelope_dir() ==0 && currn_vol>0) {
                    currn_vol--;
                }
            }
        }
    };

    struct channel_2 {
        uint8_t len_duty = 0; // nr21
        uint8_t vol_env = 0; // nr22
        uint8_t period_low = 0; // nr23
        uint8_t period_high_ctrl = 0; // nr24

        bool enabled = false;
        int frequency_timer = 0;
        int duty_pos = 0;
        int currn_vol = 0;
        int envelope_tmr = 0;
        int len_counter = 0;


        int get_period() {
            return period_low | ((period_high_ctrl & 0x07) << 8);
        }

        int get_duty_cyc_index() {
            return (len_duty >> 6) & 0x03;
        }

        int initial_vol() {
            return (vol_env >> 4) & 0x0f;
        }

        int envelope_dir() {
            return (vol_env >> 3) & 0x01;
        }

        int envelope_period() {
            return vol_env&0x07;
        }

        int len_enable() {
            return (period_high_ctrl >>6) & 1;
        }

        uint8_t output() {
            if (!enabled) {
                return 0;
            }
            const uint8_t duty_table[4][8] = {
                {0,0,0,0,0,0,0,1},
                {0,0,0,0,0,0,1,1},
                {0,0,0,0,1,1,1,1},
                {1,1,1,1,1,1,0,0}
            };

            return duty_table[get_duty_cyc_index()][duty_pos] ? currn_vol : 0;
        }

        void trigger() {
            enabled=true;
            frequency_timer=(2048-get_period()) * 4;
            currn_vol = initial_vol();
            envelope_tmr = envelope_period();
            if (len_counter==0) {
                len_counter=64;
            }
            // dac check
            if ((vol_env & 0xF8) == 0) {
                enabled=false;
            }
        }

        void tick_freq_tmr(int cycles) {
            frequency_timer-=cycles;
            while (frequency_timer <=0) {
                frequency_timer+=(2048 - get_period()) * 4;
                duty_pos = (duty_pos+1)%8;
            }

        }

        void tick_length() {
            if (len_enable() && len_counter>0) {
                len_counter--;
                if (len_counter==0){
                    enabled=false;
                }
            }
        }

        void tick_env() {
            if (envelope_period() ==0) {
                return;
            }

            if (envelope_tmr>0) {
                envelope_tmr--;
            }
            if (envelope_tmr==0) {
                envelope_tmr=envelope_period();
                if (envelope_period() ==1 && currn_vol<15) {
                    currn_vol++;
                } else if (envelope_dir() ==0 && currn_vol>0) {
                    currn_vol--;
                }
            }
        }
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
        uint8_t control = 0; //nr44
    };

    channel_1 ch1;
    channel_2 ch2;
    channel_3 ch3;
    channel_4 ch4;

    int frame_squencer_cycles = 0;
    int frame_squencer_step=0;

    float cycle_add=0.0f;
    const float cyclers_per_sample=4194304.0f/44100.0f;

    static const int buffer_size = 4096;
    int frame_sample_count = 0;
    int16_t sample_buff[buffer_size] = {};

    int buffer_w_pos = 0;
    int buffer_r_pos = 0;

    void update(uint8_t cycles);
    void push_sample();
    int16_t mix();


};