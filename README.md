# Chocoboy
A Game Boy (DMG) emulator written in C++ (WIP)

## Usage
Download the latest release from the [Releases page](https://github.com/v-pun215/chocoboy/releases/latest).

Obtain a boot ROM from [here](https://gbdev.gg8.se/files/roms/bootroms/) (dmg_boot.bin).

Run a ROM using
`./path/to/chocoboy path/to/boot_rom path/to/rom.gb`

Make sure to use a valid boot rom when running chocoboy, otherwise it WILL crash.

## Building manually
Make sure you have libSDL installed (through brew on macOS or through your favorite package manager on Linux).

Build with `make`. Depending on your system, you may need to edit the Makefile (as I haven't thoroughly tested the Makefile).

## Controls
|  Joypad  |   Input  |
| -------- | -------- |
| DPAD UP  | W  |
| DPAD LEFT  | A  |
| DPAD DOWN  | S  |
| DPAD RIGHT  | D  |
| A  | O  |
| B  | P  |
| SELECT  | K  |
| START  | L  |



## Notes
- Passes most of Blargg's test roms
- Passes all SM83 single step tests
- Passes dmg-acid2
- Does not support audio yet
- Joypad works (as far as tested)
- Does not support saving yet
- Not every game works, mostly because of MBC.
- Instruction-accurate, not cycle-accurate at the moment.
- Synced to audio, although there are some audio glitches (pops and other things) at the moment
- Supports saving, RTC with MBC3 also works (for games like Pokemon Red!)
- Stereo audio (Channel 1,2,3,4) implemented, albeit with some bugs.

## Games tested
- Super Mario Land
- Super Mario Land 2
- The Legend of Zelda: Link's Awakening
- Pokemon Red
- Tetris
- Metroid II
- Donkey Kong
- Dr. Mario
- Final Fantasy Adventure ~~(audio messed up)~~ fixed

## Known Issues
- Audio is still rough - there are still *some* bugs.
- ~~In Final Fantasy Adventure (1991), there is a loud ringing noise during the first boss fight and cutscenes.~~ fixed!

## Technicals
- I used SDL2 to display the framebuffer on screen.
- There *is* a "debugger" implemented, but its commented out at the moment.

## Plan
I plan to 3D print a Game Boy (DMG) shell and design a custom PCB with a microcontroller, an LCD, speakers, and an SD card slot (?) for putting game cartridges in.

I also think I might expand the scope of this to include Game Boy (CGB) as well.

## Credits
[Pan Docs](https://gbdev.io/pandocs/) for technical reference.

[SameBoy](https://sameboy.github.io/) for comparing emulator output and debugging.


## Screenshots
<img src="https://user-cdn.hackclub-assets.com/019f55ae-18f2-75c9-b2f7-9f418b591d78/image.png" alt="Super Mario Land" width="300" />
<img src="https://user-cdn.hackclub-assets.com/019f2400-d86a-7b08-8b4c-46dfee348e63/image.png" alt="The Legend of Zelda: Link's Awakening" width="300" />
<img src="https://cdn.hackclub.com/019f2400-db4b-76c8-b9e8-b18ad09fbcf8/image.png" alt="Kirby's Dream Land" width="300" />
<img src="https://cdn.hackclub.com/019f2400-ddd0-754e-ad2e-f8b6e62ce12e/image.png" alt="Tetris" width="300" />
