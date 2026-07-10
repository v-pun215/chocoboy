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
    auto now = chrono::system_clock::now();
    auto elapsed = now-last_time;
    auto sec_elapsed = chrono::duration_cast<chrono::seconds>(elapsed).count();
    if (sec_elapsed==0) {
        return;
    }
    last_time += chrono::seconds(sec_elapsed);
    long long total_secs = rtc_s;
    long long total_mins = rtc_m;
    long long total_hours = rtc_h;
    long long temp_dl = rtc_dl;
    if ((rtc_dh&1)==1) {
        temp_dl+=256;
    }
    if (((rtc_dh >> 6) & 1) == 0) {
        total_secs+=sec_elapsed;
        if (total_secs>=60) {
            // overflow
            total_mins+=total_secs/60;
            total_secs%=60;
        }
        if (total_mins>=60) {
            // overflow
            total_hours+=total_mins/60;
            total_mins%=60;
        }
        if (total_hours>=24) {
            // overflow
            temp_dl+=total_hours/24;
            total_hours%=24;
        }
        if (temp_dl>511) {
            rtc_dh|= (1 << 7);
        }
        temp_dl%=512;
        rtc_dh&=254;

        rtc_dh |= (temp_dl>>8)&1;
        rtc_dl = temp_dl & 255;

        rtc_s = total_secs;
        rtc_m = total_mins;
        rtc_h = total_hours;


    }
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
                    is_latch=true;
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
            return tmr.DIV;

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
            return apu.ch1.sweep;

            case 0xff11:
            return apu.ch1.len_duty | 0x3f;

            case 0xff12:
            return apu.ch1.vol_env;

            case 0xff13:
            return 0xff;

            case 0xff14:
            return apu.ch1.period_high_ctrl | 0xbf;
            
            // channel 2 
            case 0xff16:
            return apu.ch2.len_duty | 0x3f;
            
            case 0xff17:
            return apu.ch2.vol_env;

            case 0xff18:
            return 0xff; // write only

            case 0xff19:
            return apu.ch2.period_high_ctrl | 0xbf;

            // channel 3
            case 0xff1a:
            return apu.ch3.dac;

            case 0xff1b:
            return 0xFF; // write only

            case 0xff1c:
            return apu.ch3.output_level;

            case 0xff1d:
            return 0xff; // write only

            case 0xff1e:
            return apu.ch3.period_high_ctrl | 0xbf;

            case 0xFF30 ... 0xFF3F:
            return apu.ch3.wave_ram[address-0xFF30];
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
                    is_latch = true;
                    cout << "rtc latch triggered!\n";
                    auto now = chrono::system_clock::now();
                    auto elapsed = now-last_time;
                    last_time = now;
                    long long total_secs = rtc_s;
                    long long total_mins = rtc_m;
                    long long total_hours = rtc_h;
                    long long temp_dl = rtc_dl;
                    if ((rtc_dh&1)==1) {
                        temp_dl+=256;
                    }
                    if (((rtc_dh >> 6) & 1) == 0) {
                        total_secs+=chrono::duration_cast<chrono::seconds>(elapsed).count();
                        if (total_secs>=60) {
                            // overflow
                            total_mins+=total_secs/60;
                            total_secs%=60;
                        }
                        if (total_mins>=60) {
                            // overflow
                            total_hours+=total_mins/60;
                            total_mins%=60;
                        }
                        if (total_hours>=24) {
                            // overflow
                            temp_dl+=total_hours/24;
                            total_hours%=24;
                        }
                        if (temp_dl>511) {
                            rtc_dh|= (1 << 7);
                        }
                        temp_dl%=512;
                        rtc_dh&=254;

                        rtc_dh |= (temp_dl>>8)&1;
                        rtc_dl = temp_dl & 255;

                        rtc_s = total_secs;
                        rtc_m = total_mins;
                        rtc_h = total_hours;

                        latch_rtc_dh = rtc_dh;
                        latch_rtc_dl = rtc_dl;
                        latch_rtc_s = total_secs;
                        latch_rtc_m = total_mins;
                        latch_rtc_h = total_hours;



                    }


                } else {
                    is_latch = false;
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
                } else if (ram_bank >= 0x08 && ram_bank <= 0x0C) {
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
            switch (address) {
                case 0xFF25:
                break;

                case 0xff10:
                // nr10
                apu.ch1.sweep = content;
                break;

                case 0xff11:
                apu.ch1.len_duty = content;
                break;

                case 0xff12:
                apu.ch1.vol_env = content;
                break;

                case 0xff13:
                apu.ch1.period_low = content;
                break;

                case 0xff14:
                apu.ch1.period_high_ctrl = content;
                if ((content >> 7) & 1) {
                    apu.ch1.trigger();
                }
                break;


                case 0xFF26:
                apu.audio_master_control = content & 0x80;
                break;
                case 0xFF16:
                apu.ch2.len_duty = content;
                break;

                case 0xff17:
                apu.ch2.vol_env = content;
                break;

                case 0xff18:
                apu.ch2.period_low = content;
                break;

                case 0xff19:
                apu.ch2.period_high_ctrl = content;
                if (content&0x80) {
                    apu.ch2.trigger();
                }
                break;
                // channel 3
                case 0xff1a:
                apu.ch3.dac = content;
                if ((content >> 7) & 1) {
                    apu.ch3.dac_enabled = true;
                } else {
                    apu.ch3.dac_enabled=false;
                }
                break;

                case 0xff1b:
                apu.ch3.len_timer = content;
                break;

                case 0xff1c:
                apu.ch3.output_level = content;
                break;


                case 0xff1d:
                apu.ch3.period_low=content;
                break;

                case 0xff1e:
                apu.ch3.period_high_ctrl = content;
                if ((content >> 7) & 1) {
                    apu.ch3.trigger();
                }
                break;
            }
        } else if (address >= 0xFF30 && address <= 0xFF3F) {
            // wave pattern
            apu.ch3.wave_ram[address-0xff30] = content;
        } else if (address == 0xFF04) {
            tmr.DIV = 0;
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