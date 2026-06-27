# Chocoboy
A Game Boy (DMG) emulator written in C++.

## Screenshots
![Legend of Zelda, Link's Awakening](https://media.discordapp.net/attachments/1484875525288759376/1520538629943070922/image.png?ex=6a418f73&is=6a403df3&hm=c5d696a043ef55917c02351036fb020cdcca0c25f08169345cc81f63de52af69&=&format=webp&quality=lossless&width=1468&height=1322)

![Kirby's Dream Land](https://media.discordapp.net/attachments/1484875525288759376/1520539970656276601/image.png?ex=6a4190b3&is=6a403f33&hm=fdb0a57a59db56cbdf666643e95693eeda8002421e4f4d1d8a9beae73aa6a9ed&=&format=webp&quality=lossless&width=1468&height=1322)


![Tetris](https://cdn.discordapp.com/attachments/1484875525288759376/1520548972199153704/image.png?ex=6a419915&is=6a404795&hm=4b1831bba15b51b2515336230c700c25659775d287713723af579f895a497b1e&)

## Notes
- Passes most of Blargg's test roms
- Passes all SM83 single step tests
- Passes dmg-acid2
- Does not support audio yet
- Joypad works (as far as tested)
- Does not support saving yet

## Technicals
- I used SDL2 to display the framebuffer on screen.
- Really unoptimised at the moment.

## Plan
After I get audio output working, I plan to 3D print a Game Boy (DMG) shell and design a custom PCB with a microcontroller, an LCD, speakers, and an SD card slot (?) for putting game cartridges in.

I also think I might expand the scope of this to include Game Boy (CGB) as well.

