# 4dapter - 4 Player HID Firmware variation for MiSTer / PC

This firmware reports **four controller ports as separate USB HID gamepads** (four players on one device):

- **Gamepad 0:** NES  
- **Gamepad 1:** SNES  
- **Gamepad 2:** Genesis  
- **Gamepad 3:** N64  

To get all four endpoints, **CDC (USB serial) must be disabled** at build time. See [Build setup (4 HID endpoints)](#build-setup-4-hid-endpoints) below.

---

## Build Setup (4 HID Endpoints)

The ATmega32U4 has limited USB endpoints. With serial (CDC) enabled, only three HID gamepads are available and the N64 port will not appear. To get all four gamepads you must build the Arduino AVR core with `CDC_DISABLED`.

### 1. Copy `platform.local.txt` into the AVR platform folder

This folder includes **`platform.local.txt`**. Copy it into the **same folder as the AVR platform** your Arduino IDE uses.

To find that folder: enable **File → Preferences → Show verbose output during compilation**, then compile once and look for a line like **“Using core ... from platform in folder: ...”**. The path is the folder that should contain `platform.local.txt`.

Example (your path may differ by OS or Arduino version):

- **macOS:** `~/Library/Arduino15/packages/arduino/hardware/avr/1.8.7/`
- **Windows:** `%LOCALAPPDATA%\Arduino15\packages\arduino\hardware\avr\1.8.7\`
- **Linux:** `~/.arduino15/packages/arduino/hardware/avr/1.8.7/`

Copy the **`platform.local.txt`** from this folder into that AVR folder (so the file sits next to `platform.txt`).

### 2. Clear the build cache and recompile

The IDE may still use an old core built without `-DCDC_DISABLED`. After adding `platform.local.txt`:

1. Quit the Arduino IDE.
2. Delete the build cache folder (its path appears in the verbose compile output as **“-build-cache ...”**).
3. Reopen the IDE, open this sketch, and run **Sketch → Verify/Compile**.

In the verbose log you should see **`-DCDC_DISABLED`** when the core is compiled. Then all four HID gamepads (including N64) will register.

### 3. Uploading (no serial port)

With CDC disabled, the board no longer appears as a serial port. To upload:

1. Start upload from the IDE.
2. When it says “Uploading…”, **press the reset button** on the 4dapter PCB. The board enters the bootloader and the upload completes.

---

## MiSTer - Define Joystick Buttons (Mapping)

In the MiSTer main menu, use the N64 controller for the steps below; NES / SNES / Genesis will then align for their cores:

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

---

## Controller Button Mapping

To keep button mapping consistent on MiSTer, map the SNES controller in the MiSTer main menu; NES and Genesis will then align to their core defaults.

```
     NES    PowerPad  SNES     GEN(normal)  N64
-------------------------------------------------------
X    U/D    N/A       U/D      U/D          Stick U/D
Y    L/R    N/A       L/R      L/R          Stick L/R
01   B      Pad 01    B        B            B
02   A      Pad 02    A        A            A
03   N/A    Pad 03    Y        Y            C-Left
04   N/A    Pad 04    X        X            C-Down
05   N/A    Pad 05    L        Z            L
06   N/A    Pad 06    R        C            R
07   SELECT Pad 07    SELECT   MODE         C-Right
08   START  Pad 08    START    START        START
09   N/A    Pad 09    NTT 0    HOME(8BitDo) Z
10   N/A    Pad 10    NTT 1    N/A          D-Up
11   N/A    Pad 11    NTT 2    N/A          D-Down
12   N/A    Pad 12    NTT 3    N/A          D-Left
13   N/A    N/A       NTT 4    N/A          D-Right
14   N/A    N/A       NTT 5    N/A          C-Up
15   N/A    N/A       NTT 6    N/A          N/A 
16   N/A    N/A       NTT 7    N/A          N/A
17   N/A    N/A       NTT 8    N/A          N/A
18   N/A    N/A       NTT 9    N/A          N/A
19   N/A    N/A       NTT *    N/A          N/A
20   N/A    N/A       NTT #    N/A          N/A
21   N/A    N/A       NTT .    N/A          N/A
22   N/A    N/A       NTT C    N/A          N/A
23   N/A    N/A       N/A      N/A          N/A
24   N/A    N/A       NTT End  N/A          N/A
```

---

## MiSTer Home Menu Suggestion

If you use the [mapping steps](#mister---define-joystick-buttons-mapping) above (with Menu → C-Right Button + Analog Stick Down for N64), then:

* **N64:** C-Right Button + Analog Stick Down  
* **NES:** SELECT + DOWN  
* **SNES:** SELECT + DOWN  
* **GENESIS:** MODE + DOWN  

*Note: SELECT + DOWN = HOME on 8BitDo N30*

---

## RetroArch Port Binding

The 4dapter appears as one USB device with four controller indices. In RetroArch, set **Settings → Input → Port 1 Binds → Device Index** and choose the index for the port you want (e.g. #4 for N64, #1–#3 for NES/SNES/Genesis). Save; it applies per game, core, or content directory.

- https://retropie.org.uk/forum/topic/26681/port-binds/
- https://retropie.org.uk/docs/RetroArch-Configuration/#core-input-remapping

---

## Install Instructions

### 1. Complete build setup (4 HID endpoints)

Follow [Build Setup (4 HID endpoints)](#build-setup-4-hid-endpoints) above: copy **`platform.local.txt`** into the AVR platform folder and clear the build cache.

### 2. Board and upload

1. In the Arduino IDE, select **Tools → Board → Arduino AVR Boards → Arduino Leonardo**.
2. Open this sketch and compile (Sketch → Verify/Compile).
3. Upload using the reset-button method described in step 3 of Build Setup.

---

## Restoring Firmware After Alternative Firmware

If you previously flashed Analogue Pocket or Nintendo Switch firmware, the board may no longer show as a serial port. The 4dapter HID firmware (with CDC disabled) also does not use serial.

To upload or restore firmware:

1. Open this project in the Arduino IDE.
2. Connect the 4dapter via USB.
3. Start upload (Sketch → Upload).
4. When the IDE says “Uploading…”, **press the reset button** on the 4dapter PCB. The board enters the bootloader and the upload should complete. If the port is not found, try the upload and reset sequence again.
