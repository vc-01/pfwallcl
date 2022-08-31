# Wallpaper Clock for Atari Portfolio

Oh, the time flies! It's been decades since Atari Portfolio was released in 1989! Mine was sitting in the boxes somewhere forgotten for years, which was itching me. I dug it up finally, plugged it in, and... boom! It works like new.

And what happens next? You write a program for it! And what program can make the machine useful again, let it showcase itself, make it a decoration one can put on the shelf? Nothing revolutionary; a clock with optional animation. And that's exactly what this program does:

![Screenshot_1](sshots/sshot_1-bw.bmp)

![Screenshot_2](sshots/sshot_2-bw.bmp)

## Installation

The program size is 36KiB. It can run from RAM disk without a need for memory card. Format the disk to at least '40':

`c>fdisk 40`

Transfer the program and execute it:

`c>pfwallcl.exe`

The required Bios version is '1.052'. It could possibly work on other versions, but you'd need to physically remove the check from the source (function `PFBios::check_bioscompat()`). I don't have the means to test against different versions.

## Keyboard shortcuts

| Key                         | Action                           |
| ---------------------------:|:-------------------------------- |
| <kbd>a</kbd>                | Toggle animation                 |
| <kbd>1</kbd> - <kbd>9</kbd> | Set power-off delay in hours     |
| <kbd>0</kbd>                | Set power-off delay to 4 minutes |
| <kbd>f</kbd>                | Fast-tick toggle                 |
| <kbd>Space</kbd>            | Rearrange windows                |
| <kbd>o</kbd>                | Power off now                    |

## Compilation

Source code compiles with Borland C++ Version 3.1 (1992). To compile it add Borland installation directory to path as well as to the makefile and type:

`make -fpfwallcl.mak`

## Font used in program

Noto (Noto Fonts)\
<sub>_Copyright 2018 The Noto Project Authors (github.com/googlei18n/noto-fonts)_</sub>\
<sub>_Fonts license: [LICENSE.noto-fonts](LICENSE.noto-fonts)_</sub>
