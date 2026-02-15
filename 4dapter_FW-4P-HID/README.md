# 4dapter - 4 Player HID Firmware variation for MiSTer / PC

This firmware reports **four controller ports as separate USB HID gamepads** (four players on one device):

- **Gamepad 0:** NES  
- **Gamepad 1:** SNES  
- **Gamepad 2:** Genesis  
- **Gamepad 3:** N64  

**Recommended for MiSTer:** This 4-endpoint firmware is designed and optimized for MiSTer FPGA devices, where all four controller ports are correctly recognized. For PC or browser use, fewer endpoints may appear (see [Windows](#windows-only-3-controllers-visible-same-firmware-shows-4-on-maclinux) and [browser](#browser-gamepad-api) limitations below).

To get all four endpoints, **CDC (USB serial) must be disabled** at build time. See [Build setup (4 HID endpoints)](#build-setup-4-hid-endpoints) below.

**Windows:** On Windows, only 3 controller endpoints may appear even with correct firmware—this may be a [Windows limitation](#windows-only-3-controllers-visible-same-firmware-shows-4-on-maclinux).

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

**If you build on multiple computers (e.g. Mac and Windows), you must add `platform.local.txt` and clear the build cache on each machine.** Otherwise you may see 4 controller endpoints on one and only 3 on the other, because the firmware was compiled with different options (CDC on vs off).

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

## Windows: only 3 controllers visible (same firmware shows 4 on Mac/Linux)

If the **same 4dapter hardware** (with 4-endpoint firmware already flashed) shows **4 controller endpoints on macOS or Linux** but only **3 on Windows**, this appears to be a **Windows limitation**, not a firmware bug.

The device correctly reports 4 HID interfaces and 4 endpoints (verified with USBView: `bNumInterfaces = 4`, `wTotalLength` validated, all 4 interface and endpoint descriptors present). Windows’ USB composite parent driver (Usbccgp.sys) and HID stack only create 3 child devices for the 4 interfaces. IADs were tried but reverted—`bInterfaceCount=1` is invalid per USB spec and did not resolve the issue. The firmware uses plain interface descriptors and is spec-compliant.

**Things to try on Windows (may or may not help):**

1. **Device Manager** — Expand **Human Interface Devices** and **Universal Serial Bus devices**. Check how many 4dapter entries appear and whether any show a yellow warning.
2. **USB port** — Plug directly into a **motherboard USB port** (not a hub).
3. **Driver** — If an interface appears under **Other devices** or with a warning, try **Update driver** → **Let me pick** → **HID-compliant game controller** or **USB Input Device**.
4. **Verify descriptor** — Use [USBView](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usbview-example) or [USB Device Tree Viewer](https://www.uwe-sieber.de/usbtreeview.html). If `bNumInterfaces = 4` and all 4 interfaces/endpoints are present with no errors, the firmware is correct; the limitation is in Windows’ enumeration.

### Browser (Gamepad API)

In browser gamepad testers (e.g. [Hardware Tester](https://hardwaretester.com/gamepad)) using the Gamepad API:

- **Chrome on macOS:** Only **1** controller may appear. The 4dapter is one USB composite device; Chrome on Mac often exposes a single gamepad slot per device, so only one of the four ports is visible.
- **Chrome on Windows:** **3** controllers typically appear, matching the 3 HID interfaces that Windows enumerates for this device.

This is browser/OS behavior, not a firmware bug. Click on the page (user gesture) and press a button on a connected port so the browser can detect the controller.

### Custom descriptor with IADs (reference)

IADs group **two or more** interfaces into one function; `bInterfaceCount` must be &gt; 1 per USB spec. With one interface per gamepad, IAD cannot be used correctly—each IAD would need `bInterfaceCount=1`, which USBView flags as an error. This section documents the format for reference only. Each interface remains **HID** (`bInterfaceClass = 0x03`). Each interface keeps `bInterfaceClass = 0x03` (HID); the IAD is an extra descriptor that *precedes* each interface and tells the host “the following interface(s) form one function.” That helps Windows’ composite driver create one PDO per gamepad. The host still loads the standard HID driver for each interface; games and apps still see four HID game controllers.

**Current layout (no IAD):**  
For each of the four gamepads, the configuration descriptor contains:

1. **Interface descriptor** (9 bytes) — `bInterfaceClass = 0x03` (HID), one interface, one endpoint.
2. **HID descriptor** (class-specific) — report descriptor length, etc.
3. **Endpoint descriptor** (7 bytes) — interrupt IN.

So today: `[Interface][HID][Endpoint]` × 4. No IAD.

**With IAD (custom descriptor):**  
Before each of those blocks you prepend an **IAD** (8 bytes) that describes “one function = one HID interface”:

| Offset | Field             | Value (per gamepad) | Meaning                          |
|--------|-------------------|----------------------|----------------------------------|
| 0      | bLength           | 8                    | IAD size                         |
| 1      | bDescriptorType   | 0x0B                 | INTERFACE_ASSOCIATION            |
| 2      | bFirstInterface   | 0, 1, 2, or 3        | Interface number for this pad    |
| 3      | bInterfaceCount   | 1                    | One interface in this function   |
| 4      | bFunctionClass    | 0x03                 | HID                              |
| 5      | bFunctionSubClass | 0x00                 | No subclass                      |
| 6      | bFunctionProtocol | 0x00                 | No protocol                       |
| 7      | iFunction         | 0                    | No string (or index if you add one) |

So each gamepad’s block becomes: **`[IAD][Interface][HID][Endpoint]`**. The Interface descriptor is unchanged: still class 0x03 (HID), so the device still reports as a HID device. Only the configuration descriptor gains four 8-byte IADs; the rest (HID report descriptor, endpoints, behavior) stays the same.

**Example IAD bytes for interface 0 (NES):**  
`08 0B 00 01 03 00 00 00`

- `08` = length 8  
- `0B` = IAD  
- `00` = bFirstInterface 0  
- `01` = bInterfaceCount 1  
- `03 00 00` = HID, no subclass, no protocol  
- `00` = iFunction 0  

For interfaces 1–3 you’d use `08 0B 01 01 03 00 00 00`, `08 0B 02 01 03 00 00 00`, `08 0B 03 01 03 00 00 00`.

**Optional device descriptor change:**  
Some hosts expect composite devices that use IADs to use device class **EFh** (Miscellaneous), subclass **02h** (Common Class), protocol **01h** (Interface Association). That is a device-level hint that IADs are present; the interfaces remain HID (0x03). If you implement a fully custom descriptor set, you can set `bDeviceClass=0x00` (composite) as today, or `0xEF/0x02/0x01` if you want to follow that convention.

**Summary:** A custom descriptor that adds IADs would still report four HID interfaces; only the configuration descriptor layout changes. The device would still be a standard HID gamepad device to the host and to games.

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
