#pragma once
#include <SDL.h>
#include <vector>
#include <array>
#include <cstdint>

using namespace std;

struct memory;
struct cpu;
struct PPU {
    // SDL stuff
    SDL_Renderer* renderer;
    SDL_Window* window;
    SDL_Texture* texture;
    //void render_debugger(memory& mem, cpu& cpu, bool& paused, bool&s);
    void cycleSDL(memory& mem);
    void initSDL();


    uint8_t LCDC = 0; // LCD control
    uint8_t frame_cntr = 0;
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
    bool stat_line = false;

    uint8_t window_y = 0;

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
    void set_mode(uint8_t mode, uint8_t& IF);
    void check_lyc(memory& mem);
    void update(uint8_t cycle, memory& mem, cpu& cpu, bool& paused, bool& step);
    uint8_t tile_pixel_color(uint8_t x, uint8_t low, uint8_t high);
    uint8_t get_palette_shade(uint8_t palette, uint8_t index) ;

    struct sprite {
        uint8_t y;
        uint8_t x;
        uint8_t tile_index;
        uint8_t attr_flags;
    };

    vector<PPU::sprite> get_visible_sprites(array<uint8_t, 160>& OAM, uint8_t LY);
    void render_scanline(memory& mem);
};