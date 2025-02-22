# 4dapter - HID Firmware (Single) for MiSTer / PC

The default HID firmware will allow the 4dapter to appear as a single controller with all 4 inputs merged into one.

## MiSTer - Define Joystick Buttons (Mapping)

### Manual Mapping
If manually mapping in the MiSTer main menu, use the N64 controller using the following steps and NES / SNES / Genesis will be appropriately mapped for their cores:

```
DPAD Test: Press RIGHT     ---  D-Right
Stick 1 Test: Tilt RIGHT   ---  Analog Stick Right
Stick 1 Test: Tilt DOWN    ---  Analog Stick Down
Stick 2 Test: Tilt RIGHT   ---  Undefine (User / Space to Skip)
Press: RIGHT               ---  Analog Stick Right
Press: LEFT                ---  Analog Stick Left
Press: DOWN                ---  Analog Stick Down
Press: UP                  ---  Analog Stick Up
Press: A                   ---  A Button
Press: B                   ---  B Button
Press: X                   ---  C-Down Button
Press: Y                   ---  C-Left Button
Press: L                   ---  Left Bumper Button
Press: R                   ---  Right Bumper Button
Press: Select              ---  C-Right Button
Press: Start               ---  Start Button
Press: Mouse Move RIGHT    ---  Undefine (User / Space to Skip)
Press: Mouse Move LEFT     ---  Undefine (User / Space to Skip)
Press: Mouse Move DOWN     ---  Undefine (User / Space to Skip)
Press: Mouse Move UP       ---  Undefine (User / Space to Skip)
Press: Mouse Btn Left      ---  Undefine (User / Space to Skip)
Press: Mouse Btn Right     ---  Undefine (User / Space to Skip)
Press: Mouse Btn Middle    ---  Undefine (User / Space to Skip)
Press: Mouse Emu/Sniper    ---  Undefine (User / Space to Skip)
Press: Menu                ---  C-Right Button + Analog Stick Down
Note: C-Right Button + Analog Stick Down makes Select/Mode + Down work for NES/SNES/GEN
      This may cause menu to appear in some gameplay, so adapt as needed.
Press: Menu: OK            ---  A Button
Press: Menu: Back          ---  B Button
Stick 1: Tilt RIGHT        ---  Analog Stick Right
Stick 1: Tilt DOWN         ---  Analog Stick Down
```

## Controller Button Mapping

```
     NES    PowerPad  SNES     GEN(normal)  GEN(MiSTer)  N64
-------------------------------------------------------------------
X    U/D    N/A       U/D      U/D          U/D          Stick U/D
Y    L/R    N/A       L/R      L/R          L/R          Stick L/R
01   B      Pad 01    B        B            A            B
02   A      Pad 02    A        A            B            A
03   N/A    Pad 03    Y        Y            X            C-Left
04   N/A    Pad 04    X        X            Y            C-Down
05   N/A    Pad 05    L        Z            Z            L
06   N/A    Pad 06    R        C            C            R
07   SELECT Pad 07    SELECT   MODE         MODE         C-Right
08   START  Pad 08    START    START        START        START
09   N/A    Pad 09    NTT 0    HOME(8BitDo) [SPECIAL]    Z
10   N/A    Pad 10    NTT 1    N/A          N/A          D-Up
11   N/A    Pad 11    NTT 2    N/A          N/A          D-Down
12   N/A    Pad 12    NTT 3    N/A          N/A          D-Left
13   N/A    N/A       NTT 4    N/A          N/A          D-Right
14   N/A    N/A       NTT 5    N/A          N/A          C-Up
15   N/A    N/A       NTT 6    N/A          N/A          N/A
16   N/A    N/A       NTT 7    N/A          N/A          N/A
17   N/A    N/A       NTT 8    N/A          N/A          N/A
18   N/A    N/A       NTT 9    N/A          N/A          N/A
19   N/A    N/A       NTT *    N/A          N/A          N/A
20   N/A    N/A       NTT #    N/A          N/A          N/A
21   N/A    N/A       NTT .    N/A          N/A          N/A
22   N/A    N/A       NTT C    N/A          N/A          N/A
23   N/A    N/A       N/A      N/A          N/A          N/A
24   N/A    N/A       NTT End  N/A          N/A          N/A

* GENESIS(MiSTer): Mode will send Select + Down
* N64: [SPECIAL] is trigged by L + R + C-Down at the same time and sends Button 07 + Analog Down
```

## MiSTer Home Menu Suggestion
* **NES:** SELECT + DOWN
* **SNES:** SELECT + DOWN
* **GENESIS:** MODE + DOWN

*Note: SELECT + DOWN = HOME on 8BitDo N30*

## MiSTer mode on 8bitdo M30 controller

MiSTer mode can be toggled on and off with "HOME + Z" (you have to press HOME first, Z second). This setting is saved to EEPROM and preserved across power cycles. The default is "normal mode".

When the Genesis controller is in MiSTer mode:

- The HOME button sends "DOWN + MODE" (This is because no equivalent of the HOME button exists on the other controller ports)

- Buttons are swapped: A with B and X with Y. This is such that the position of the buttons is consistent between SNES and Genesis.

## Install Instructions

### 1. Select "Arduino AVR Boards - Arduino Leonardo" from Boards List

### 2. Download project to Arduino Pro Micro board

## Restoring Firmware After Alternative Firmware Downloads

If you previously used the Analogue Pocket or Nintendo Switch firmware versions, your Arduino Pro Micro will no longer report as a Serial device and will not appear in the "Port" list in the Arduino software. Using the default 3dapter firmware does not suffer from this issue.

To restore the default firmware for your Arduino Pro Micro, you will need to manually reset the Arduino Pro Micro by shorting the Reset + Ground pins together to trigger a reset. 

1. Open the default firmware project in the Arduino software.
2. Connect your Arduino Pro Micro to your computer.
3. Trigger the download from the Arduino software. The Arduino software will first compile the project before beginning the download.
4. Once the "compiling" step has finished, trigger a reset on the Arduino Pro Micro by briefly touching the Reset (RST) and Ground (GND) pins together. This should make the Arduino forcefully switch into bootloader mode and allow the download to complete. If the download fails initially due to not finding the COM port, repeat the download/reset process and it should work the second time.
