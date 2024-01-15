# 4dapter - XInput / Analogue Pocket Dock Firmware

This alternative firmware will allow the 4dapter to appear a single XInput controller. All 4 controller inputs will share the same singular player input (i.e. use only one controller at time). 

The controller inputs have been mapped to align with the default mapping on the Analogue Pocket Dock (as of firmware 1.1).

## IMPORTANT NOTE
**Installing the XInput Library will remove the self-reset listener from the Arduino board. This means you will need to manually reset the Arduino when updating the code on the board or switching to a different firmware.**

**You can accomplish this by connecting the RST and GND pins together - a tip of a screwdriver works well since the pins are next to each other.**

<img src="https://github.com/timville85/TripleController/assets/31223405/b407d6e9-23bf-4204-840a-c814300fc317" width=30% height=30%>

## IMPORTANT NOTE
**Installing the XInput Library will remove the self-reset listener from the Arduino board. This means you will need to manually reset the Arduino when updating the code on the board or switching to a different firmware.**

**You can accomplish this by pressing the reset button on the 4dapter (the small hole on the top of the case)**

### Resources Used

* https://github.com/dmadison/ArduinoXInput
* https://github.com/dmadison/ArduinoXInput_AVR

## Install Instructions

**Instructions tested with Arduino 1.8.19 - not fully tested with Arduino 2.X!**

### 1. Install XInput Library (tested with 1.2.6) from Arduino Library Manager

<img src="https://github.com/timville85/TripleController/assets/31223405/38a2bc0b-d369-4d84-97ce-102e0bcb07e5" width=70% height=70%>

### 2. Add the following URL as an Additional Board Manager URL (in File -> Preferences menu)

`https://raw.githubusercontent.com/dmadison/ArduinoXInput_Boards/master/package_dmadison_xinput_index.json`

<img src="https://github.com/timville85/TripleController/assets/31223405/1cba8973-0a96-4ce4-bb16-88dc7bfd06eb" width=70% height=70%>

### 3. Install XInput AVR Boards (tested with 1.0.5) from Arduino Boards Manager

_Note: Choose the AVR version, not Sparkfun_

<img src="https://github.com/timville85/TripleController/assets/31223405/c6feadd6-1a90-4994-b593-7eb0eabc6d5f" width=70% height=70%>

### 4. Select "XInput AVR Boards - Arduino Leonardo w/ XInput" from Boards List

<img src="https://github.com/timville85/TripleController/assets/31223405/e22e636c-30b9-4465-9161-914a4f8581e1" width=70% height=70%>

### 5. Download project to Arduino Pro Micro board

Remember after download your Arduino Pro Micro will no longer report as a Serial device and will not appear in the "Port" list in the Arduino software.

Note: If you are replacing existing code on the 4dapter, trigger the download from the Arduino software and then trigger a reset on the Arduino Pro Micro within a few seconds of starting the download. If the download fails initially due to not finding the COM port, repeat the download/reset process and it should work the second time.
