# Chester - veikkos' Game Boy emulator

Game Boy emulator written in C with minimal dependencies with portable library part.

[![Screenshot](https://raw.githubusercontent.com/veikkos/chester/public/chester-dinos.png)](https://github.com/gingemonster/DinosOfflineAdventure)  
(Game shown above is Dino's Offline Adventure, see [link](https://github.com/gingemonster/DinosOfflineAdventure) for more.)

## Important ##

Chester has not been extensively tested. However, it doesn't have any major know issues besides missing sound implementation. You can help by opening an [Issue](https://github.com/veikkos/chester/issues) if you find one.

## What it does ##

Chester does play several tested ROM-only, MBC1, MBC3 and MBC5
games. It also supports Game Boy Color games and in-game saving for
battery backed cartridges. Its accurate CPU instruction implementation
passes Blargg's CPU instruction tests including timing.

## What is it missing ##

Most of the emulators have some inaccuracies but it doesn't mean they aren't usable in practise. Chester also has its limitations. It's missing sound support, (very) accurate instruction and memory timing, (very) accurate timers, full support for some cartridge types, full GPU accuracy... And probably few other things.

## Dependencies ##

Library part doesn't have external dependencies and is portable C code.

Desktop application depends on SDL2 (https://www.libsdl.org/).

## Getting started ##

### Debian-like systems ###

Make sure you have C compiler installed. Emulator has been tested with GCC and clang.

Install SDL2
```
$ sudo apt-get install libsdl2-dev
```

Build
```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

Run
```
$ ./bin/chester path/to/rom.gb
```

Optionally bootloader can be used by providing "DMG_ROM.bin" in working directory.

#### Buttons ####

| Original | Emulator   |
|----------|------------|
| Pad      | Arrow keys |
| A        | A          |
| B        | Z          |
| Start    | N          |
| Select   | M          |

F4 toggles color correction on SDL port if CGB support is enabled and playing CGB game.

### Visual Studio ###

SDL2 library can be downloaded from https://www.libsdl.org/.

For Windows you might need to specify SDL2 library path explicitly, e.g.
```
$ cmake -G "Visual Studio 15 2017 Win64" -DSDL2_PATH="C:\\<path>\\SDL2-2.0.9" ..
```

### Android ###

Needs Android SDK and NDK.

```
$ cd android/ChesterApp/
$ ./gradlew build
```

### Build options ###

|                  | Description                            | Options      |
|------------------|----------------------------------------|--------------|
| CGB              | Game Boy Color support                 | **ON** / OFF |
| COLOR_CORRECTION | Color correction by default (CGB only) | **ON** / OFF |

**Bolded** is default value.

Usage e.g.

```
$ cmake -DCGB=OFF ..
```

Color correction value can be enabled/disabled during runtime.

## Tested platforms ##

* Ubuntu 14.04 LTS
* Windows 10
* Android

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
