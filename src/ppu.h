#pragma once
#include <SDL.h>

struct memory;
struct PPU {
    // SDL stuff
    SDL_Renderer* renderer;
    SDL_Window* window;
    SDL_Texture* texture;
    void initSDL();


    uint8_t LCDC = 0; // LCD control
    uint8_t LY = 0;
    uint8_t LYC = 0;
    uint8_t STAT = 0;
    uint8_t SCY = 0;
    uint8_t SCX = 0;
    uint8_t BGP = 0;
    uint8_t OBP0 = 0;
    uint8_t OBP1 = 0;
    uint8_t WY = 0;
    uint8_t WX = 0;

    enum ppu_modes {
        H_BLANK,
        V_BLANK,
        OAM_SCAN,
        DRAWING_PIXELS
    };

    int ppu_mode = OAM_SCAN;
    int cycles_in_mode = 0;




    unsigned char framebuffer[160 * 144 *3]; // RGB24
    unsigned char framebufferA[160 * 144 * 3]; // RGBA24 needed to draw multiple framebuffers on top of each other

    void update(uint8_t cycle, uint8_t& IF, memory& mem);
    uint8_t tile_pixel_color(uint8_t x, uint8_t low, uint8_t high);
    void render_scanline(memory& mem);
};