# 4dapter - Nintendo Switch Firmware

This firmware will allow the 4dapter to appear a single Switch-compatible controller. All 4 controller inputs will share the same singular player input (i.e. use only one controller at time). 

The controller inputs have been mapped to align to map to the Nintendo Switch Virtual NES, SNES, N64 and Genesis apps. Switch-specific button combos have been added to support in-game menus, home buttons, and screenshot capabities.

## IMPORTANT NOTE
**Installing the XInput Library will remove the self-reset listener from the Arduino board. This means you will need to manually reset the Arduino when updating the code on the board or switching to a different firmware.**

**You can accomplish this by connecting the RST and GND pins together - a tip of a screwdriver works well since the pins are next to each other.**

## 4dapter Special Buttons for Switch

**NES:**
* In-Game Menu ("-" Button): Select + D-Pad Down
* Home Button: Select + Start
* Screenshot: Select + Up

**SNES:**
* In-Game Menu ("-" Button): Select + D-Pad Down
* Home Button: Select + Start
* Screenshot: Select + Up

**Genesis (6-Button Mode):**
* In-Game Menu ("-" Button): Mode + Down
* Home Button: Mode + Start (or 8BitDo M30 Heart Button)
* Screenshot: Mode + D-Pad Up
* Special Mode to Swap Genesis X --> Switch RB (for SEGA Genesis Classics): Hold Mode + B for 1.5 Seconds

**Genesis (3-Button Mode):**
* In-Game Menu ("-" Button): Start + A + B + C
* Home Button: Start + A + Down
* Screenshot: Start + A + Up

**N64:**
* In-Game Menu ("-" Button): Start + D-Pad Down
* Home Button: Start + L-Button + R-Button
* Screenshot: Start + D-Pad Up
* Special Mode to Swap N64 B --> Switch X (for Super Mario 3D All-Stars): Hold Start + D-Pad Down for 1.5 Seconds

**NES / SNES / Genesis 6-Button:**
Swap Directional Pad <--> Left Analog Joystick Toggle
* Hold Select/Mode + Down for 1.5 seconds
* Reports 8-Way Input (Cardinal + Diagonal)

## Resources Used

* [LUFA Arduino Board & Library](https://github.com/CrazyRedMachine/Arduino-Lufa)
* [Switch-Fightstick](https://github.com/progmem/Switch-Fightstick).

## Install Instructions

### 1. Add the following URL as an Additional Board Manager URL (in File -> Preferences menu)
`https://github.com/CrazyRedMachine/Arduino-Lufa/raw/master/package_arduino-lufa_index.json`

### 2. Select and Install **`LUFA AVR Boards`** from the Arduino Board Manager

### 3. Select **`Arduino LUFA AVR Boards -> Arduino Leonardo (LUFA)`** from Boards list

### 4. Download project to Arduino Pro Micro board

Remember after download your Arduino Pro Micro will no longer report as a Serial device and will not appear in the "Port" list in the Arduino software.

Note: If you are replacing existing code on the 4dapter, trigger the download from the Arduino software and then trigger a reset on the Arduino Pro Micro within a few seconds of starting the download. If the download fails initially due to not finding the COM port, repeat the download/reset process and it should work the second time.
