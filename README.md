# GlitzBoy
[![Build Status](https://travis-ci.org/glitzflitz/GlitzBoy.svg?branch=master)](https://travis-ci.org/glitzflitz/GlitzBoy)\
A fast cross platform Gamboy(DMG) emulator written in C. \
GlitzBoy supports sound, several hardware types, and RTC

## Screenshots
![Screenshot 1](https://raw.githubusercontent.com/glitzflitz/GlitzBoy/master/Screenshots/zelda.png)
![Screenshot 2](https://raw.githubusercontent.com/glitzflitz/GlitzBoy/master/Screenshots/donkeykong.png)
![Screenshot 3](https://raw.githubusercontent.com/glitzflitz/GlitzBoy/master/Screenshots/dragonheart.png)
![Screenshot 4](https://raw.githubusercontent.com/glitzflitz/GlitzBoy/master/Screenshots/pokemonblue.png)


Implemented
-----------

* CPU
  - All instructions correct
  - All timings correct
  - Double, triple, quadruple speed mode
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




## Installation
First make sure SDL2 is installed on your platform
```
git clone https://github.com/glitzflitz/GlitzBoy.git
cd GlitzBoy
make
./GlitzBoy <Path to rom>
```

## Emulation Accuracy
Although the goal of this emulator is speed over accuracy, GlitzBoy passes [Blargg's](http://gbdev.gg8.se/files/roms/blargg-gb-tests/) CPU instruction test cases and and CPU instruction timing test cases.
![CPU_Test](https://raw.githubusercontent.com/glitzflitz/GlitzBoy/master/Screenshots/cpu_test.png)
