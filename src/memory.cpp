#include <array>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include "cpu.h"
#include "memory.h"
#include <chrono>
#include "globals.h"
using namespace std;


uint8_t memory::read_ROM(uint16_t address) {
    uint8_t romtype = ROM[0x0147];
    if (address >= 0x0000 && address <= 0x3FFF) {
            return ROM[address];
    }
    else if (address >= 0x4000 && address <= 0x7FFF ) {
        switch (romtype) {
        case 0x00: {
            // MBC0
            return ROM[address];
        }
        case 0x01 ... 0x06: { // mbc1 and mbc 2
            return ROM[(rom_bank * 0x4000) + (address - 0x4000)]; 
        }
        case 0x0F ... 0x13: {
            // mbc 3
            return ROM[(rom_bank*0x4000) + (address - 0x4000)];
        }
        }
    }
    return ROM[address];
}

void memory::update_currn_rtc() {
    bool halted = (rtc_dh >> 6) & 1;

    auto now = chrono::system_clock::now();
    auto elapsed = now - last_time;
    auto sec_elapsed = chrono::duration_cast<chrono::seconds>(elapsed).count();
    if (sec_elapsed == 0) return;

    last_time += chrono::seconds(sec_elapsed);
    if (halted) return;

    long long total_secs = rtc_s + sec_elapsed;
    long long total_mins = rtc_m;
    long long total_hours = rtc_h;
    long long temp_dl = rtc_dl | ((rtc_dh & 1) << 8);

    if (total_secs >= 60)  {
        total_mins+= total_secs / 60;
        total_secs%= 60;
    }
    if (total_mins >= 60)  {
        total_hours+= total_mins / 60;
        total_mins%= 60;
    }
    if (total_hours >= 24) {
        temp_dl+= total_hours / 24;
        total_hours%= 24;
    }
    if (temp_dl > 511) {
        rtc_dh |= (1 << 7);
    }
    temp_dl %= 512;

    rtc_dh = (rtc_dh & 0xFE) | ((temp_dl >> 8) & 1);
    rtc_dl = temp_dl & 0xFF;
    rtc_s  = total_secs;
    rtc_m  = total_mins;
    rtc_h  = total_hours;
}

uint8_t memory::read(uint16_t address) {

    if (address >= 0x0000 && address <= 0x7FFF) { // Cartridge ROM
        if (boot_enabled && address >=0x0000 && address <= 0x00FF) {
            return boot_ROM[address];
        }
        return read_ROM(address);
    } else if (address >= 0x8000 && address <= 0x9FFF) { // VRAM
        return VRAM[address-0x8000];
    } else if (address >= 0xA000 && address <= 0xBFFF) { // ERAM
        uint8_t romtype = ROM[0x0147];

        
        if (ram_enabled) {
            if (romtype == 0x05 || romtype == 0x06) {
                uint16_t  mbc2_address = (address - 0xA000) & 0x01FF;
                return ERAM[mbc2_address] | 0xF0;

            } else if (romtype >=0x0F && romtype <= 0x13) {
                // mbc3
                if (ram_bank <=0x03) {
                    return ERAM[(ram_bank*0x2000) + (address -0xA000)];
                } else if (ram_bank >=0x08 && ram_bank <=0x0C) {
                    switch (ram_bank) {
                        case 0x08:
                        return (is_latch) ? latch_rtc_s : rtc_s;

                        case 0x09:
                        return (is_latch) ? latch_rtc_m : rtc_m;

                        case 0x0A:
                        return (is_latch) ? latch_rtc_h : rtc_h;

                        case 0x0B:
                        return (is_latch) ? latch_rtc_dl : rtc_dl;

                        case 0x0C:
                        return (is_latch) ? latch_rtc_dh : rtc_dh;
                    }
                }
            }
            
            else {
                return ERAM[(ram_bank * 0x2000) + (address - 0xA000)];
            }
        }
        return 0xFF;
    } else if (address >= 0xC000 && address <= 0xDFFF) { // WRAM
        return WRAM[address-0xC000];
    } else if (address >= 0xE000 && address <= 0xFDFF) { // echo ram
        return WRAM[address-0xE000];
    } else if (address >= 0xFE00 && address <= 0xFE9F) { // OAM
        return OAM[address-0xFE00];
    } else if (address >= 0xFEA0 && address <= 0xFEFF) { // not usable
        return 0xFF; //to do
    } else if (address >= 0xFF00 && address <= 0xFF7F) { // implment io ranges
        if (debug){cout << "I/O ADDRESS CALLED: " << address << '\n';}
        switch (address) {
            case 0xFF00:
            // joypad
            return joy.read_joyp();

            case 0xFF0F:
            return IF;

            case 0xFF04:
            return tmr.get_DIV();

            case 0xFF05:
            return tmr.TIMA;

            case 0xFF06:
            return tmr.TMA;

            case 0xFF07:
            return tmr.TAC;

            case 0xFF40:
            return ppu.LCDC;

            case 0xFF4A:
            return ppu.WY;

            case 0xFF4B:
            return ppu.WX;
            

            case 0xFF44:
            return ppu.LY;

            case 0xFF45:
            return ppu.LYC;

            case 0xFF41:
            return ppu.STAT;
            
            case 0xFF42: 
            return ppu.SCY;

            case 0xFF43:
            return ppu.SCX;

            case 0xFF47:
            return ppu.BGP;

            case 0xFF48:
            return ppu.OBP0;

            case 0xFF49:
            return ppu.OBP1;
            
            case 0xFF50:
            return boot_enabled ? 0 : 1;

            // audio

            // channel 1
            case 0xff10:
            //nr10
            return apu.ch1.sweep | 0x80;

            case 0xff11:
            return apu.ch1.len_duty | 0x3f;

            case 0xff12:
            return apu.ch1.vol_env;

            case 0xff13:
            return 0xff;

            case 0xff14:
            return (apu.ch1.period_high_ctrl & 0x40) | 0xBF;
            
            // channel 2 
            case 0xff16:
            return apu.ch2.len_duty | 0x3f;
            
            case 0xff17:
            return apu.ch2.vol_env;

            case 0xff18:
            return 0xff; // write only

            case 0xff19:
            return (apu.ch2.period_high_ctrl & 0x40) | 0xbf;

            // channel 3
            case 0xff1a:
            return apu.ch3.dac | 0x7F;

            case 0xff1b:
            return 0xFF; // write only

            case 0xff1c:
            return apu.ch3.output_level | 0x9F;

            case 0xff1d:
            return 0xff; // write only

            case 0xff1e:
            return (apu.ch3.period_high_ctrl & 0x40) | 0xBF;

            case 0xFF30 ... 0xFF3F:
            if (apu.ch3.enabled) {
                return apu.ch3.wave_ram[apu.ch3.wave_pos/2];
            }
            return apu.ch3.wave_ram[address-0xFF30];

            // channel 4
            case 0xff20:
            return 0xFF;

            case 0xff21:
            return apu.ch4.vol_env;

            case 0xff22:
            return apu.ch4.freq_rand;

            case 0xff23:
            return (apu.ch4.control & 0x40) | 0xBF;

            case 0xff26: {
                uint8_t status = apu.audio_master_control & 0x80;
                if (apu.ch4.enabled) status |= 0x08;
                if (apu.ch3.enabled) status |= 0x04;
                if (apu.ch2.enabled) status |= 0x02;
                if (apu.ch1.enabled) status |= 0x01;
                return status | 0x70;
            }

            case 0xFF24:
            return apu.master_volume_vin;

            case 0xFF25:
            return apu.sound_panning;
        }
        return 0xFF; // for now
    } else if (address >= 0xFF80 && address <= 0xFFFE) { // HRAM
        return HRAM[address-0xFF80];
    } else if (address==0xFFFF) {
        //cout << "IE READ: curn vlaue: " << IE << '\n';
        return IE;
    } else {
        cout << "ERROR: UNIMPL ADDR CALLED - " << address << '\n';
        return 0xFF;
    }
}
void memory::write(uint16_t address, uint8_t content) {
    uint8_t romtype = ROM[0x0147];

    if (address >= 0x0000 && address <= 0x7FFF) { // Cartridge ROM
        if (romtype == 0x00) {
            // MBC0
            // read only
            return;
        } else  if (romtype == 0x01 || romtype == 0x02 | romtype == 0x03){
            // MBC1
            if (address >= 0x0000 && address <= 0x1FFF) {
                ram_enabled = (content & 0x0F) == 0x0A;
            } else if (address >= 0x2000 && address <= 0x3FFF) { 
                auto shtuff = content & 0x1F; // lower 5 bits
                if (shtuff == 0) {
                    shtuff = 1;
                }
                rom_bank = (rom_bank & 0x60) | shtuff;
            } else if (address >=0x4000 && address <=0x5FFF) {
                if (!mbc1_mode) {
                    // rom banking mode
                    rom_bank = (rom_bank & 0x1F) | ((content & 0x03) << 5);
                } else {
                    // ram banking mode
                    ram_bank = content & 0x03;
                }
            } else if (address >=0x6000 && address <=0x7FFF) {
                mbc1_mode = content & 0x01;
            }
        } else if (romtype == 0x05 || romtype == 0x06) {
            // MBC 2
            if (address >=0x0000 && address <= 0x3FFF) {
                if ((address & 0x0100) ==0) {
                    ram_enabled = (content & 0x0F) == 0x0A;
                } else {
                    rom_bank = content & 0x0F;
                    if (rom_bank == 0) {
                        rom_bank = 1;
                    }
                }
            } else if (address >= 0x4000 && address <= 0x7FFF) {
                // read only
            }
        } else if (romtype >=0x0F && romtype <= 0x13) {
            // mbc3
            if (address >= 0x0000 && address <= 0x1FFF) {
                ram_enabled = (content&0x0F) == 0x0A;
            } else if (address >=0x2000 && address <= 0x3FFF) {
                rom_bank = content & 0x7F; // direct 7 bits
                if (rom_bank == 0) {
                    rom_bank =1;
                }
            } else if (address >= 0x4000 && address <=0x5FFF) {
                ram_bank = content;
            } else if (address >= 0x6000 && address <= 0x7FFF) {
                // rtc latch
                if (rtc_latch == 0x00 && content == 0x01) {
                    update_currn_rtc();
                    latch_rtc_s  = rtc_s;
                    latch_rtc_m  = rtc_m;
                    latch_rtc_h  = rtc_h;
                    latch_rtc_dl = rtc_dl;
                    latch_rtc_dh = rtc_dh;
                    is_latch = true;
                }
                rtc_latch = content;                
            }
        }
    } 
    

    
    else if (address >= 0x8000 && address <= 0x9FFF) { // VRAM
        VRAM[address-0x8000] = content;
    } else if (address >= 0xA000 && address <= 0xBFFF) { // ERAM
        if (ram_enabled) {
            uint8_t romtype = ROM[0x0147];
            if (romtype >= 0x0F && romtype <= 0x13) {
                if (ram_bank <= 0x03) {
                    ERAM[(ram_bank * 0x2000) + (address - 0xA000)] = content;
                    sram_dirty = true;
                    last_sram_write = chrono::steady_clock::now();
                } else if (ram_bank >= 0x08 && ram_bank <= 0x0C) {
                    sram_dirty = true;
                    last_sram_write = chrono::steady_clock::now();
                    switch (ram_bank) {
                        case 0x08:
                        rtc_s = content & 0x3F;
                        break;

                        case 0x09:
                        rtc_m = content & 0x3F;
                        break;

                        case 0x0A:
                        rtc_h = content & 0x1F;
                        break;

                        case 0x0B: 
                        rtc_dl = content;
                        break;
                        case 0x0C:
                        rtc_dh = content & 0xC1;
                    }
                    last_time = chrono::system_clock::now();
                }
            } else {
                ERAM[(ram_bank * 0x2000) + (address - 0xA000)] = content;
                sram_dirty = true;
                last_sram_write = chrono::steady_clock::now();
            }
        }


    } else if (address >= 0xC000 && address <= 0xDFFF) { // WRAM
        WRAM[address-0xC000] = content;
    } else if (address >= 0xE000 && address <= 0xFDFF) { // echo ram
        WRAM[address-0xE000] = content;
    } else if (address >= 0xFE00 && address <= 0xFE9F) { // OAM
        OAM[address-0xFE00] = content;
    } else if (address >= 0xFEA0 && address <= 0xFEFF) { // not usable
        //pass
    } else if (address >= 0xFF00 && address <= 0xFF7F) { // implment io ranges
        
        if (address == 0xFF00) {
            // joypad input 
            //cout << "JOYP write = " << hex << (int)content << '\n';
            joy.write_selected(content);
        } else if (address >= 0xFF01 && address <=0xFF02) {
            //serial transfer
            if (serial && address==0xFF01) {cout << "SERIAL: " << (char)content <<'\n';}
        } else if (address == 0xFF0F) {
            // interrupts
            IF = content;
        } else if (address >= 0xFF10 && address <= 0xFF26) {
            //audio
            if ((apu.audio_master_control & 0x80) == 0 && address != 0xFF26) {
                switch (address) {
                    case 0xff11:
                    apu.ch1.len_counter = 64 - (content&0x3f);
                    break;

                    case 0xff16:
                    apu.ch2.len_counter = 64 - (content&0x3f);
                    break;

                    case 0xff1b:
                    apu.ch3.len_counter = 256 - content;
                    break;

                    case 0xff20:
                    apu.ch4.len_counter = 64 - (content&0x3f);
                    break;
                }
                return; 
            }
            switch (address) {
                case 0xFF25:
                apu.sound_panning = content;
                break;

                case 0xff10: {
                    // nr10
                    bool old_neg = (apu.ch1.sweep >> 3) & 1;
                    apu.ch1.sweep = content;
                    bool new_neg = (content >> 3) & 1;
                    if (old_neg && !new_neg && apu.ch1.sweep_negate_since_trigger) {
                        apu.ch1.enabled = false;
                    }
                    break;
                }

                case 0xff11:
                apu.ch1.len_duty = content;
                apu.ch1.len_counter = 64 - (content & 0x3F);
                break;

                case 0xff12:
                apu.ch1.vol_env = content;
                apu.ch1.dac_enabled = (content & 0xf8) !=0;
                if (!apu.ch1.dac_enabled) {
                    apu.ch1.enabled = false;
                }
                break;

                case 0xff13:
                apu.ch1.period_low = content;
                break;

                case 0xff14: {
                    bool prev_len_enable = (apu.ch1.period_high_ctrl & 0x40);
                    apu.ch1.period_high_ctrl = content;
                    bool new_len_enable = (apu.ch1.period_high_ctrl & 0x40);

                    if (!prev_len_enable && new_len_enable && (apu.frame_squencer_step % 2 != 0)) {
                        if (apu.ch1.len_counter > 0) {
                            apu.ch1.len_counter--;
                            if (apu.ch1.len_counter == 0) {
                                apu.ch1.enabled = false;
                            }
                        }
                    }

                    if ((content >> 7) & 0x1) {
                        apu.ch1.trigger(apu.frame_squencer_step);
                    }
                    break;
                }


                case 0xFF26: {
                    bool was_on = (apu.audio_master_control & 0x80) != 0;
                    bool now_on = (content & 0x80) != 0;
                    apu.audio_master_control = content;

                    if (!now_on) {
                        apu.ch1.enabled = false;
                        apu.ch2.enabled = false;
                        apu.ch3.enabled = false;
                        apu.ch4.enabled = false;
                        apu.ch1.sweep = 0; apu.ch1.len_duty = 0; apu.ch1.vol_env = 0; apu.ch1.period_low = 0; apu.ch1.period_high_ctrl = 0;
                        apu.ch2.len_duty = 0; apu.ch2.vol_env = 0; apu.ch2.period_low = 0; apu.ch2.period_high_ctrl = 0;
                        apu.ch3.dac = 0; apu.ch3.len_timer = 0; apu.ch3.output_level = 0; apu.ch3.period_low = 0; apu.ch3.period_high_ctrl = 0;
                        apu.ch4.len_timer = 0; apu.ch4.vol_env = 0; apu.ch4.freq_rand = 0; apu.ch4.control = 0;
                        apu.master_volume_vin = 0;
                        apu.sound_panning = 0;
                    } else if (!was_on && now_on) {
                        // 0 to 1 trans only
                        apu.frame_squencer_step = 0;
                        apu.ch1.duty_pos = 0;
                        apu.ch2.duty_pos = 0;
                        apu.ch3.wave_pos = 0;
                        apu.ch3.currn_sample = 0;
                    }

                    break;
                }
                // channel 2
                case 0xFF16:
                apu.ch2.len_duty = content;
                apu.ch2.len_counter = 64 - (content & 0x3F);
                break;

                case 0xff17:
                apu.ch2.vol_env = content;
                apu.ch2.dac_enabled = (content & 0xF8) !=0; 
                if (!apu.ch2.dac_enabled) {
                    apu.ch2.enabled = false;
                }
                break;

                case 0xff18:
                apu.ch2.period_low = content;
                break;

                case 0xff19: {
                    bool prev_len_enable = (apu.ch2.period_high_ctrl & 0x40);
                    apu.ch2.period_high_ctrl = content;
                    bool new_len_enable = (apu.ch2.period_high_ctrl & 0x40);

                    if (!prev_len_enable && new_len_enable && (apu.frame_squencer_step % 2 != 0)) {
                        if (apu.ch2.len_counter > 0) {
                            apu.ch2.len_counter--;
                            if (apu.ch2.len_counter == 0) {
                                apu.ch2.enabled = false;
                            }
                        }
                    }

                    if ((content >> 7) & 0x1) {
                        apu.ch2.trigger(apu.frame_squencer_step);
                    }
                    break;
                }
                // channel 3
                case 0xff1a:
                apu.ch3.dac = content;
                if ((content >> 7) & 1) {
                    apu.ch3.dac_enabled = true;
                } else {
                    apu.ch3.dac_enabled=false;
                }
                if (!apu.ch3.dac_enabled) {
                    apu.ch3.enabled = false;
                }
                break;

                case 0xff1b:
                apu.ch3.len_timer = content;
                apu.ch3.len_counter = 256 - content;
                break;

                case 0xff1c:
                apu.ch3.output_level = content;
                break;


                case 0xff1d:
                apu.ch3.period_low=content;
                break;

                case 0xff1e: {
                    bool prev_len_enable = (apu.ch3.period_high_ctrl & 0x40);
                    apu.ch3.period_high_ctrl = content;
                    bool new_len_enable = (apu.ch3.period_high_ctrl & 0x40);

                    if (!prev_len_enable && new_len_enable && (apu.frame_squencer_step % 2 != 0)) {
                        if (apu.ch3.len_counter > 0) {
                            apu.ch3.len_counter--;
                            if (apu.ch3.len_counter == 0) {
                                apu.ch3.enabled = false;
                            }
                        }
                    }

                    if ((content >> 7) & 0x1) {
                        apu.ch3.trigger(apu.frame_squencer_step);
                    }
                    break;
                }

                // channel 4
                case 0xff20:
                apu.ch4.len_timer = content;
                apu.ch4.len_counter = 64 - (content & 0x3F);
                break;

                case 0xff21:
                apu.ch4.vol_env = content;
                apu.ch4.dac_enabled = (content & 0xF8) != 0;
                if (!apu.ch4.dac_enabled) {
                    apu.ch4.enabled = false;
                }
                break;

                case 0xff22:
                apu.ch4.freq_rand = content;
                break;

                case 0xff23: {
                    bool prev_len_enable = (apu.ch4.control & 0x40);
                    apu.ch4.control = content;
                    bool new_len_enable = (apu.ch4.control & 0x40);

                    if (!prev_len_enable && new_len_enable && (apu.frame_squencer_step % 2 != 0)) {
                        if (apu.ch4.len_counter > 0) {
                            apu.ch4.len_counter--;
                            if (apu.ch4.len_counter == 0) {
                                apu.ch4.enabled = false;
                            }
                        }
                    }
                    if ((content >> 7) & 0x1) {
                        apu.ch4.trigger(apu.frame_squencer_step);
                    }
                    break;
                }
                
                case 0xFF24:
                apu.master_volume_vin = content;
                break;
            }
        } else if (address >= 0xFF30 && address <= 0xFF3F) {
            // wave pattern (ch3)
            if (apu.ch3.enabled) {
                apu.ch3.wave_ram[apu.ch3.wave_pos/2]=content;
            } else {
                apu.ch3.wave_ram[address-0xff30] = content;
            }
        } else if (address == 0xFF04) {
            // div
            if (tmr.reset_div(IF)) {
                apu.clock_frame_squencer();
            }
        } else if (address == 0xFF05) {
            tmr.TIMA = content;
        } else if (address==0xFF06) {
            tmr.TMA = content; // to do
        } else if (address==0xFF07) {
            tmr.TAC = content;
        }  else if (address == 0xFF50) {
            boot_enabled = false;
        }

        switch (address) {
            case 0xFF4A:
            ppu.WY = content;
            break;

            case 0xFF4B:
            ppu.WX = content;
            break;

            case 0xFF40:
            ppu.LCDC = content;
            break;

            case 0xFF44:
            // read only
            break;

            case 0xFF45:
            ppu.LYC = content;
            ppu.check_lyc(*this);
            break;

            case 0xFF41:
            //cout << "WRITE STAT = " << hex << (int)content << "\n";
            ppu.STAT = (ppu.STAT & 0x07) | (content & 0x78);
            ppu.check_lyc(*this);
            break;

            case 0xFF42:
            ppu.SCY = content;
            break;

            case 0xFF43:
            ppu.SCX = content;
            break;

            case 0xFF46: {
                uint16_t source = content*0x100;
                for (int i=0;i<160;i++) {
                    OAM[i] = read(source+i);
                }
                break;
            }

            case 0xFF47:
            ppu.BGP = content;
            break;

            case 0xFF48:
            ppu.OBP0 = content;
            break;

            case 0xFF49:
            ppu.OBP1 = content;
            break;
        }

    } else if (address >= 0xFF80 && address <= 0xFFFE) { // HRAM
        HRAM[address-0xFF80] = content;
    } else if (address==0xFFFF) {
        //cout << hex << "WRITE IE = " << (int)content << '\n';
        //dump_vram(*this, "my_acid2_vram.bin");
        IE = content;
        return;
    } else {
        cout << "ERROR: UNIMPL ADDR CALLED - " << address << '\n';
    }
}

void memory::loadROM(string path) { // to-do: implement MBC
    try {
        ifstream rom(path, ios::binary);
        if (!rom.is_open()) {
            cout << "error: could not open ROM\n";
            throw runtime_error("cannot open ROM");
        }
        

        vector<uint8_t> buffer(
            (istreambuf_iterator<char>(rom)),
            istreambuf_iterator<char>()
        );
        if (buffer.size()<32768) { // less than 32 KiB
            //cout << "error: ROM invalid - too small\n";
        } else if (buffer.size()<32768) {
            cout << "error: ROM too big for current implementation\n";
            throw runtime_error("cannot copy ROM into memory");
        }
        ROM.resize(buffer.size());
        copy(
            buffer.begin(),
            buffer.end(),
            ROM.begin()
        );
        size_t dot = path.find_last_of('.');
        save_path = (dot == string::npos) ? path + ".sav" : path.substr(0, dot) + ".sav";

        if (has_battery()) {
            loadGame(save_path);
        }

    } catch (const exception& e){
        cerr << "Caught: " << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
}

void memory::boot(string boot_rom) {
    cout << "Booting...\n";
    try {
        ifstream rom(boot_rom, ios::binary);
        if (!rom.is_open()) {
            cout << "error: could not open BOOT ROM\n";
            throw runtime_error("cannot open BOOT ROM");
        }
        

        vector<uint8_t> buffer(
            (istreambuf_iterator<char>(rom)),
            istreambuf_iterator<char>()
        );
        
        copy(
            buffer.begin(),
            buffer.end(),
            boot_ROM.begin()
        );
    } catch (const exception& e){
        cerr << "Caught: " << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
}


bool memory::has_battery() {
    uint8_t romtype = ROM[0x0147];
    switch (romtype) {
        case 0x03: 
        case 0x06: 
        case 0x0F:
        case 0x10: 
        case 0x13:
            return true;
        default:
            return false;
    }
}

void memory::saveGame(string path) {
    if (!has_battery()) return;

    ofstream out(path, ios::binary);
    if (!out.is_open()) {
        cout << "could not opn save file for writing\n";
        return;
    }

    out.write(reinterpret_cast<const char*>(ERAM.data()), ERAM.size());

    uint8_t romtype = ROM[0x0147];
    if (romtype == 0x0F || romtype == 0x10) {
        out.write(reinterpret_cast<const char*>(&rtc_s), 1);
        out.write(reinterpret_cast<const char*>(&rtc_m), 1);
        out.write(reinterpret_cast<const char*>(&rtc_h), 1);
        out.write(reinterpret_cast<const char*>(&rtc_dl), 1);
        out.write(reinterpret_cast<const char*>(&rtc_dh), 1);

        int64_t epoch = chrono::duration_cast<chrono::seconds>(last_time.time_since_epoch()).count();
        out.write(reinterpret_cast<const char*>(&epoch), sizeof(epoch));
    }
}

void memory::loadGame(string path) {
    if (!has_battery()) return;

    ifstream in(path, ios::binary);
    if (!in.is_open()) {
        return;
    }

    in.read(reinterpret_cast<char*>(ERAM.data()), ERAM.size());

    uint8_t romtype = ROM[0x0147];
    if (romtype == 0x0F || romtype == 0x10) {
        in.read(reinterpret_cast<char*>(&rtc_s), 1);
        in.read(reinterpret_cast<char*>(&rtc_m), 1);
        in.read(reinterpret_cast<char*>(&rtc_h), 1);
        in.read(reinterpret_cast<char*>(&rtc_dl), 1);
        in.read(reinterpret_cast<char*>(&rtc_dh), 1);

        int64_t epoch = 0;
        in.read(reinterpret_cast<char*>(&epoch), sizeof(epoch));

        last_time = in
            ? chrono::system_clock::time_point(chrono::seconds(epoch))
            : chrono::system_clock::now();
    }
}
void memory::flush_save_if_dirty() {
    if (!sram_dirty || !has_battery()) {
        sram_dirty = false;
        return;
    }

    auto now = chrono::steady_clock::now();
    auto timee = chrono::duration_cast<chrono::milliseconds>(now - last_sram_write).count();

    if (timee < 1000) {
        return;
    }

    saveGame(save_path);
    sram_dirty = false;
}