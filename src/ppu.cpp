#include <iostream>
#include <SDL.h>
#include "ppu.h"
using namespace std;

void PPU::initSDL() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    SDL_CreateWindowAndRenderer(160, 144, 0, &window, &renderer);
    SDL_SetWindowSize(window, 480, 432);
    SDL_SetWindowResizable(window, SDL_TRUE);
}

void PPU::update(uint8_t cycles, uint8_t& IF) {
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
            // render_scanline();
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
                ppu_mode = OAM_SCAN;
            }
        }
        break;
    }

    STAT = (STAT & 0xFC) | ppu_mode;
}