# GlitzBoy
A fast cross platform Gamboy(DMG) emulator written in C. \
GlitzBoy supports sound, several hardware types, and RTC

Implemented
-----------

* CPU
  - All instructions correct
  - All timings correct
  - Double speed mode
* GPU
  - Normal mode
  - Color mode
* Audio
* Joypad
* Timer
* MMU
  - MBC-less
  - MBC1
  - MBC3 (with RTC)
  - MBC5
  - save games
* Progress Saving




## Installation
First make sure SDL2 is installed on your platform
```
git clone https://github.com/glitzflitz/GlitzBoy.git
cd GlitzBoy
make
./GlitzBoy <Path to rom>
```

## Emulation Accuracy
Currently, GlitzBoy passes [Blargg's](http://gbdev.gg8.se/files/roms/blargg-gb-tests/) CPU instruction test cases and and CPU instruction timing test cases.
