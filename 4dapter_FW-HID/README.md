# 4dapter - HID Firmware for MiSTer / PC

The default HID firmware will allow the 4dapter to appear a multiplayer input controller with 3 controller inputs acting as it's own separate player/input (NES and SNES are combined due to Arduino USB endpoint limitations.)

## Controller Button Mapping

To maintain proper button mapping on MiSTer, it's recommended to map the SNES controller on the MiSTer Main menu and the NES / Genesis controllers will align to their core defaults properly.

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
07   SELECT Pad 07    SELECT   MODE         MODE         [SPECIAL]
08   START  Pad 08    START    START        START        START
09   N/A    Pad 09    NTT 0    HOME(8BitDo) [SPECIAL]    Z
10   N/A    Pad 10    NTT 1    N/A          N/A          D-Up
11   N/A    Pad 11    NTT 2    N/A          N/A          D-Down
12   N/A    Pad 12    NTT 3    N/A          N/A          D-Left
13   N/A    N/A       NTT 4    N/A          N/A          D-Right
14   N/A    N/A       NTT 5    N/A          N/A          C-Up
15   N/A    N/A       NTT 6    N/A          N/A          C-Right
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

The 4dapter exposes three "players" on one USB device.  Unfortunately MiSTer does not support setting keymaps per "player", MiSTer mode works around this by making sure the positional mapping is the same between all controllers.

MiSTer mode can be toggled on and off with "HOME + Z" (you have to press HOME first, Z second). This setting is saved to EEPROM and preserved across power cycles. The default is "normal mode".

When the Genesis controller is in MiSTer mode:

- The HOME button sends "DOWN + MODE" (This is because no equivalent of the HOME button exists on the other controller ports)

- Buttons are swapped: A with B and X with Y. This is such that the position of the buttons is consistent between SNES and Genesis.

## Retroarch Port Binding

Some Retroarch users have reported issues regarding setting the correct “Device Index” setting, since the 3dapter reports a single “device” to RetroArch with 3 different controllers, each as their own “index”.

In Retroarch, go to Settings → Input → Port 1 Binds → Device Index and then choose #3 for N64 and presumably #1 or #2 for NES/SNES and Genesis. Save and it sticks for that game, core or content directory.

https://retropie.org.uk/forum/topic/26681/port-binds/
https://retropie.org.uk/docs/RetroArch-Configuration/#core-input-remapping

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