# 4dapter - XInput / Analogue Pocket Dock Firmware

This alternative firmware will allow the 4dapter to appear a single XInput controller. All 4 controller inputs will share the same singular player input (i.e. use only one controller at time). 

The controller inputs have been mapped to align with the default mapping on the Analogue Pocket Dock (as of firmware 1.1).

## IMPORTANT NOTE
**Installing the XInput Library will remove the self-reset listener from the Arduino board. This means you will need to manually reset the Arduino when updating the code on the board or switching to a different firmware.**

**You can accomplish this by connecting the RST and GND pins together - a tip of a screwdriver works well since the pins are next to each other.**

### Resources Used

* https://github.com/dmadison/ArduinoXInput
* https://github.com/dmadison/ArduinoXInput_AVR

## Install Instructions

### 1. Install XInput Library from Arduino Library Manager

![Library Installer](https://user-images.githubusercontent.com/31223405/210913898-79e7c503-dfee-435b-9685-0df4602176fe.png)

### 2. Add the following URL as an Additional Board Manager URL (in File -> Preferences menu)

`https://raw.githubusercontent.com/dmadison/ArduinoXInput_Boards/master/package_dmadison_xinput_index.json`

### 3. Install XInput Boards from Arduino Boards Manager

![Board Installer](https://user-images.githubusercontent.com/31223405/210913897-90a02f39-623b-473a-b531-b47f3c0cb15b.png)

### 4. Select "XInput AVR Boards - Arduino Leonardo w/ XInput" from Boards List

![Board Selection](https://user-images.githubusercontent.com/31223405/210913895-d7c319d9-86e8-4e33-947e-3fe8e6cf977c.png)

### 5. Download project to Arduino Pro Micro board

Remember after download your Arduino Pro Micro will no longer report as a Serial device and will not appear in the "Port" list in the Arduino software.

Note: If you are replacing existing code on the 4dapter, trigger the download from the Arduino software and then trigger a reset on the Arduino Pro Micro within a few seconds of starting the download. If the download fails initially due to not finding the COM port, repeat the download/reset process and it should work the second time.