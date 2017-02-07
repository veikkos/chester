# Chester - veikkos' Game Boy emulator

Game Boy emulator written in C language with minimal dependencies.

## Important ##

Chester is probably NOT suitable for playing games, as weird as it may sound. It is my attempt to learn emulator programming and is probably not accurate and stable enough for real gaming use.

## What it does ##

Chester does play several ROM-only, MBC1 and MBC3 games pretty well. It supports in-game saving for battery backed MBC1/MBC3 cartridges. Its accurate CPU instruction implementation passes Blargg's CPU instruction tests.

## What is it missing ##

Sounds, (very) accurate instruction and memory timing, (very) accurate timers, configurable buttons, full support for many cartridge types, window resize, full GPU accuracy... And many other things.

## Dependencies ##

SDL2 - https://www.libsdl.org/

## Getting started ##

Make sure you have C compiler installed. Emulator is tested with, and defaults to, GCC but also compiles with clang.

Install SDL2
```
$ sudo apt-get install libsdl2-dev
```

Build
```
$ make release
```

Run
```
$ ./gbe path/to/rom.gb
```

Optionally bootloader can be used by providing "DMG_ROM.bin" in working directory.

## Tested platforms ##

* Ubuntu 14.04 LTS
* Windows 10 (MinGW)

## Thanks to ##

This emulator has been influenced by at least
* Imran Nazar's JavaScript article series
* Cinoop emulator by CTurt

And its development was eased by debugging with following great emulators
* NO$GMB
* BGB

Tested with ROMs from
* Blargg

Also thanks to numerous people over various emulator forums for their great research and help for others.

## Status

[![Build Status](https://api.travis-ci.org/veikkos/chester.svg?branch=public)](https://travis-ci.org/veikkos/chester)
