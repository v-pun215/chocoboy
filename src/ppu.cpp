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
                SDL_UpdateTexture(texture, NULL, framebuffer, 160*3);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
                ppu_mode = OAM_SCAN;
            }
        }
        break;
    }

    STAT = (STAT & 0xFC) | ppu_mode;
}

uint8_t PPU::tile_pixel_color(uint8_t x, uint8_t low_byte, uint8_t high_byte) {
    uint8_t low_bit = (low_byte >> (7-x)) & 1;
    uint8_t high_bit = (high_byte >> (7-x)) & 1;
    uint8_t palette_index = (high_bit << 1) | low_bit;
    return palette_index;

}

uint8_t get_palette_shade(uint8_t palette, uint8_t index) {
    uint8_t high_bit = (palette >> 2*index+1) & 1;
    uint8_t low_bit = (palette >> 2*index) & 1;
    uint8_t shade = (high_bit << 1) | low_bit;
    return shade;
}

void PPU::render_scanline(memory& mem) {
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

        uint8_t shade = get_palette_shade(BGP, color_index);
        // colors
        tuple<int, int, int> white{255,255,255};
        tuple<int, int, int> light_gray{170, 170, 170};
        tuple<int, int, int> dark_gray{85,85,85};
        tuple<int, int, int> black{0,0,0};
        int R, G, B;
        switch (shade) {
            case 0:
            R = get<0>(white);
            G = get<1>(white);
            B = get<2>(white);
            break;

            case 1:
            R = get<0>(light_gray);
            G = get<1>(light_gray);
            B = get<2>(light_gray);
            break; 
            
            case 2:
            R = get<0>(dark_gray);
            G = get<1>(dark_gray);
            B = get<2>(dark_gray);
            break;

            case 3:
            R = get<0>(black);
            G = get<1>(black);
            B = get<2>(black);
            break;
        }   

        int offset = (LY*160+x)*3;

        //write to framebuffer
        framebuffer[offset]=R;
        framebuffer[offset+1]=G;
        framebuffer[offset+2]=B;        

    }
}