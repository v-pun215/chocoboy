#include <iostream>
#include <SDL.h>
#include "ppu.h"
#include "cpu.h"
#include "memory.h"
#include <vector>
using namespace std;

void PPU::initSDL() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_CreateWindowAndRenderer(160, 144, 0, &window, &renderer);
    SDL_SetWindowSize(window, 480, 432);
    SDL_SetWindowResizable(window, SDL_FALSE);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 160, 144);
}

void PPU::cycleSDL(memory& mem) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            exit(0);
        } else if (event.type == SDL_KEYDOWN) {
            if (event.key.repeat == 0) {

                switch (event.key.keysym.sym) {
                    case SDLK_w:
                    cout << "UP\n";
                    mem.joy.up = true;
                    break;

                    case SDLK_a:
                    cout << "LEFT\n";
                    mem.joy.left = true;
                    break;

                    case SDLK_s:
                    cout << "DOWN\n";
                    mem.joy.down = true;
                    break;

                    case SDLK_d:
                    cout << "RIGHT\n";
                    mem.joy.right = true;
                    break;

                    case SDLK_o:
                    cout << "A\n";
                    mem.joy.A = true;
                    break;

                    case SDLK_p:
                    cout << "B\n";
                    mem.joy.B = true;
                    break;

                    case SDLK_k:
                    cout << "SELECT\n";
                    mem.joy.select = true;
                    break;

                    case SDLK_l:
                    cout << "START\n";
                    mem.joy.start = true;
                    break;
                }
            }
        } else if (event.type == SDL_KEYUP) {
            switch (event.key.keysym.sym) {
                case SDLK_w:
                cout << "UP lifted\n";
                mem.joy.up = false;
                break;

                case SDLK_a:
                cout << "LEFT lifted\n";
                mem.joy.left = false;
                break;

                case SDLK_s:
                cout << "DOWN lifted\n";
                mem.joy.down = false;
                break;

                case SDLK_d:
                cout << "RIGHT lifted\n";
                mem.joy.right = false;
                break;

                case SDLK_o:
                cout << "A lifted\n";
                mem.joy.A = false;
                break;

                case SDLK_p:
                cout << "B lifted\n";
                mem.joy.B = false;
                break;

                case SDLK_k:
                cout << "SELECT lifted\n";
                mem.joy.select = false;
                break;

                case SDLK_l:
                cout << "START lifted\n";
                mem.joy.start = false;
                break;
            }
        }
    }
}

void PPU::update(uint8_t cycles, uint8_t& IF, memory& mem) {
    /*bool LCD_enable = LCDC & 0x80;
    if (!LCD_enable) { // lcd off
        
        cycles_in_mode=0;
        LY=0;
        ppu_mode=H_BLANK;
        STAT&=0x7c;
        return;
    }*/
    cycles_in_mode+=cycles;

    switch (ppu_mode) {
        case OAM_SCAN:
        if (cycles_in_mode>=80) {
            cycles_in_mode-=80;
            ppu_mode=DRAWING_PIXELS;
        }
        break;

        case DRAWING_PIXELS:
        if (cycles_in_mode >= 172) {
            cycles_in_mode-=172;
            ppu_mode = H_BLANK;
            render_scanline(mem);
        }
        break;

        case H_BLANK:
        if (cycles_in_mode >= 204) {
            cycles_in_mode-=204;
            LY++;
            if (LY==144) { // last row
                ppu_mode = V_BLANK;
                IF = IF | 0x01; // v-blank interrupt
            } else {
                ppu_mode = OAM_SCAN;
            }
        }
        break;

        case V_BLANK:
        if (cycles_in_mode>=456) {
            cycles_in_mode-=456;
            LY++;
            if (LY>153) {
                LY=0;
                window_y=0;
                SDL_UpdateTexture(texture, NULL, framebuffer, 160*3);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
                ppu_mode = OAM_SCAN;
            }
        }
        break;
    }

    STAT = (STAT & 0xFC) | ppu_mode;
    if (LY == LYC) {
        STAT |= 0x04;
        if (STAT & 0x40) {
            IF |= 0x02; 
        }
    } else {
        STAT &= ~0x04;
    }
}

uint8_t PPU::tile_pixel_color(uint8_t x, uint8_t low_byte, uint8_t high_byte) {
    uint8_t low_bit = (low_byte >> (7-x)) & 1;
    uint8_t high_bit = (high_byte >> (7-x)) & 1;
    uint8_t palette_index = (high_bit << 1) | low_bit;
    return palette_index;

}

uint8_t PPU::get_palette_shade(uint8_t palette, uint8_t index) {
    uint8_t high_bit = (palette >> 2*index+1) & 1;
    uint8_t low_bit = (palette >> 2*index) & 1;
    uint8_t shade = (high_bit << 1) | low_bit;
    return shade;
}

vector<PPU::sprite> PPU::get_visible_sprites(array<uint8_t, 160>& OAM, uint8_t LY) {
    vector<PPU::sprite> visible{};
    for (int i=0;i<160 && visible.size() < 10;i+=4) {
        uint8_t byte0 = OAM[i]; // y
        uint8_t byte1 = OAM[i+1]; // x
        uint8_t byte2 = OAM[i+2]; // tile index
        uint8_t byte3 = OAM[i+3]; // attr/flagsb
        
        int real_y = byte0-16;
        int height = (LCDC & 0x04) ? 16 : 8;
        
        if (LY >= real_y && LY < (height + real_y)) {
            visible.push_back({byte0, byte1, byte2, byte3});
        }
        
    } 
    return visible;
}

void PPU::render_scanline(memory& mem) {


    vector<sprite> visible_sprites = get_visible_sprites(mem.OAM, LY);
    bool window_drawn_this_scanline = false;
    for (uint8_t x=0; x<160;x++) {
        uint8_t bg_x = (x + SCX) % 256;
        uint8_t bg_y = (LY + SCY) % 256;
        uint8_t tile_col = bg_x/8;
        uint8_t tile_pixel_x = bg_x%8;
        uint8_t tile_row = bg_y/8;
        uint8_t tile_pixel_y = bg_y%8;
        bool tile_map_area = (LCDC >> 3) & 1;
        uint16_t stored_address;
        if (tile_map_area) {
            stored_address = 0x9C00;
        } else {
            stored_address = 0x9800;
        }
        uint8_t tile_index = mem.read(stored_address + tile_row * 32 + tile_col);

        bool tile_data_area = (LCDC >> 4) & 1;

        uint16_t address;
        if (tile_data_area) {
            address = 0x8000 + tile_index*16;
        } else {
            address = 0x9000 + (int8_t)tile_index *16;
        }

        uint16_t row_address = address + tile_pixel_y*2;
        uint8_t color_index = tile_pixel_color(tile_pixel_x, mem.read(row_address), mem.read(row_address+1));
        uint8_t final_color_index = color_index;

        uint8_t shade = get_palette_shade(BGP, color_index);
        bool bg_enable = LCDC & 1;
        if (!bg_enable) {
            color_index = 0;
            shade = 0;
        }


        // window
        bool window_enable = (LCDC >> 5) & 1;
        if (window_enable) {
            if (LY >= WY && x >= (int)WX - 7) {
                uint8_t window_tilemap_col = (x- (WX - 7))/8;
                uint8_t pixel_col = (x - (WX - 7))%8;
                uint8_t window_tilemap_row = window_y/8;
                uint8_t pixel_row = window_y%8;
                bool window_tilemap = (LCDC >> 6) & 1;
                uint16_t starting_wind_addr;
                if (window_tilemap) {
                    starting_wind_addr = 0x9C00;
                } else {
                    starting_wind_addr = 0x9800;
                }
                uint8_t window_tile_index = mem.read(starting_wind_addr + window_tilemap_row*32 + window_tilemap_col);
                bool tile_data_area = (LCDC >> 4) & 1;

                uint16_t address;
                if (tile_data_area) {
                    address = 0x8000 + window_tile_index*16;
                } else {
                    address = 0x9000 + (int8_t)window_tile_index *16;
                }
                uint16_t row_address = address + pixel_row*2;
                uint8_t color_index = tile_pixel_color(pixel_col, mem.read(row_address), mem.read(row_address+1));
                final_color_index = color_index;
                shade = get_palette_shade(BGP, color_index);
                window_drawn_this_scanline=true;

            }
        }

        // oam
        
        for (auto it = visible_sprites.rbegin(); it != visible_sprites.rend(); ++it) {
            sprite sprt = *it;
            int real_sprt_x = (int)sprt.x-8;
            int real_sprt_y = (int)sprt.y-16;
            if (x >= real_sprt_x && x < real_sprt_x + 8) {
                uint8_t sprt_col = x - real_sprt_x;
                bool x_flip = (sprt.attr_flags >> 5) & 1;
                if (x_flip) {
                    sprt_col = 7-sprt_col;
                }
                int sprite_height = (LCDC & 0x04) ? 16 : 8;
                uint8_t sprt_row = LY - real_sprt_y;
                bool y_flip = (sprt.attr_flags >> 6) & 1;
                if (y_flip) {
                    sprt_row = (sprite_height-1) -sprt_row;
                }
                uint8_t sprt_tile_index = sprt.tile_index;
                if (sprite_height == 16) {
                    sprt_tile_index &= 0xFE;
                    if (sprt_row >= 8) {
                        sprt_tile_index |= 0x01; // Fetch the bottom tile
                    }
                }
                uint16_t sprt_tile_address = 0x8000 + sprt_tile_index*16;
                uint16_t sprt_row_address = sprt_tile_address + (sprt_row%8)*2;

                uint8_t color_index_sprt = tile_pixel_color(sprt_col, mem.read(sprt_row_address), mem.read(sprt_row_address+1));
                bool behind_bg = (sprt.attr_flags >> 7)&1;
                if (color_index_sprt!=0) {
                    bool should_draw = !behind_bg || (final_color_index == 0);
                    if (should_draw) {
                        bool palette = (sprt.attr_flags >> 4) & 1;
                        if (palette) {
                            shade = get_palette_shade(OBP1, color_index_sprt);
                        } else {
                            shade = get_palette_shade(OBP0, color_index_sprt);
                        }
                    }
                }
            }
        }




        // colors
        tuple<int, int, int> white{255,255,255};
        tuple<int, int, int> light_gray{170, 170, 170};
        tuple<int, int, int> dark_gray{85,85,85};
        tuple<int, int, int> black{0,0,0};
        int R, G, B;
        tuple<int, int, int> chosen_color;
        switch (shade) {
            case 0:
            chosen_color = white;
            break;

            case 1:
            chosen_color = light_gray;
            break; 
            
            case 2:
            chosen_color = dark_gray;
            break;

            case 3:
            chosen_color = black;
            break;
        }


        int offset = (LY*160+x)*3;

        //write to framebuffer
        framebuffer[offset]=get<0>(chosen_color);
        framebuffer[offset+1]=get<1>(chosen_color);
        framebuffer[offset+2]=get<2>(chosen_color);
    }
    if (window_drawn_this_scanline) {
        window_y++;
    }
}