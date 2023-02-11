# Wallpaper Clock for Atari Portfolio

This program shows a clock with optional animation.

![Screenshot_1](sshots/sshot_1-bw.bmp)

![Screenshot_2](sshots/sshot_2-bw.bmp)

_Note_: Specific Portfolio's BIOS calls are being used by the program, it won't run on PC BIOS compatible system.

## Installation

The program size is *33KiB*. It can run from the RAM disk only without need for external memory card. Format the drive to at least *"40"*:

`c>fdisk 40`

Transfer the program and execute it:

`c>pfwallcl`

Tested with BIOS version *1.052*. Try with "untested" if you have different version:

`c>pfwallcl untested`

## Keyboard shortcuts

| Key                         | Action                                |
| ---------------------------:|:------------------------------------- |
| <kbd>a</kbd>                | Toggle animation                      |
| <kbd>1</kbd> - <kbd>9</kbd> | Set power-off delay override in hours |
| <kbd>0</kbd>                | Reset power-off delay override        |
| <kbd>f</kbd>                | Fast timer tick toggle                |
| <kbd>Space</kbd>            | Rearrange windows                     |
| <kbd>o</kbd>                | Power off now                         |

## INI File

Automatic power on / off time(s) can be specified via optional .INI file created in the same directory as the program and named *PFWALLCL.INI*.

```
[Timer]
TriggerPowerOnAt=8:20
TriggerPowerOffAt=22:35
;PowerOffDelayKbhit=2:00
```

Available options are namely power on time (**TriggerPowerOnAt**), power off time (**TriggerPowerOffAt**) and/or power off delay on keyboard hit (**PowerOffDelayKbhit**) applied during the "off" period.

See [PFWALLCL.INI](PFWALLCL.INI?raw=true) example.

## Source code compilation

The source code compiles with *Borland C++ Version 3.1 (1992)*. To compile it, add Borland's installation directory to *PATH* and to makefile *PFWALLCL.MAK*. Then type:

`make -fpfwallcl.mak`

## Font used in program

Noto (Noto Fonts)\
<sub>_Copyright 2018 The Noto Project Authors (github.com/googlei18n/noto-fonts)_</sub>\
<sub>_Fonts license: [LICENSE.noto-fonts](LICENSE.noto-fonts)_</sub>
