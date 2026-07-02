# Chocoboy
A Game Boy (DMG) emulator written in C++ (WIP)

## Screenshots
<img src="https://user-cdn.hackclub-assets.com/019f2400-d86a-7b08-8b4c-46dfee348e63/image.png" alt="The Legend of Zelda: Link's Awakening" width="400" />
<img src="https://cdn.hackclub.com/019f2400-db4b-76c8-b9e8-b18ad09fbcf8/image.png" alt="Kirby's Dream Land" width="400" />
<img src="https://cdn.hackclub.com/019f2400-ddd0-754e-ad2e-f8b6e62ce12e/image.png" alt="Tetris" width="400" />




## Notes
- Passes most of Blargg's test roms
- Passes all SM83 single step tests
- Passes dmg-acid2
- Does not support audio yet
- Joypad works (as far as tested)
- Does not support saving yet
- Not every game works, mostly because of MBC.

## Technicals
- I used SDL2 to display the framebuffer on screen.
- Really unoptimised at the moment.

## Plan
After I get audio output working, I plan to 3D print a Game Boy (DMG) shell and design a custom PCB with a microcontroller, an LCD, speakers, and an SD card slot (?) for putting game cartridges in.

I also think I might expand the scope of this to include Game Boy (CGB) as well.

