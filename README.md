# 4dapter
### Open Source Controller to Nintendo Switch Adapter

This software will allow the 4dapter to appear as a single Switch controller that maps all controller inputs as a single combined controller. The controller inputs have been mapped to align to map to the Nintendo Switch Virtual NES, SNES, and Genesis applications. You will only be able to use one different input at a time (although all controllers can be plugged in at the same time).

<img src="https://github.com/timville85/4dapter/assets/31223405/ef0abcfb-1a11-40ba-a59b-c8b400074be6" width=30% height=30%>
<img src="https://github.com/timville85/4dapter/assets/31223405/1e0bf1f6-3932-423e-93d6-e7c35eedb27a" width=25% height=25%>
<img src="https://github.com/timville85/4dapter/assets/31223405/64f502cb-03b1-4d93-b38f-2e75e7a82c00" width=25% height=25%>

This project uses code from the **`LUFA Arduino Board & Library`** from [CrazyRedMachine](https://github.com/CrazyRedMachine/A
rduino-Lufa) and **`Switch-Fightstick`** from [progmem](https://github.com/progmem/Switch-Fightstick).

## 4dapter Bill of Materials (BOM)

4dapter was designed with all components to be through-hole soldered to make the project as accessible as possible.

* **1x 4dapter Main Board**

* **1x 4dapter N64 Daugher Board**

* **1x SNES Socket (90 deg):** [AliExpress - Gamers Zone Store](https://www.aliexpress.com/item/32838396935.html) 

* **1x NES Socket:** [AliExpress - Gamers Zone Store](https://www.aliexpress.com/item/1005003699734963.html)

* **1x Genesis Socket:** [AliExpress - Gamers Zone Store](https://www.aliexpress.com/item/1005003699497865.html)

* **1X N64 Socket:** [AliExpress - Gamers Zone Store](https://www.aliexpress.us/item/3256803534347876.html)

* **1x Arduino Pro Micro (USB-C):** [AliExpress](https://www.aliexpress.com/item/32888212119.html)

* **1x 1x3 90 Degree Male Header:** [Digi-Key](https://www.digikey.com/en/products/detail/sullins-connector-solutions/PREC003SBAN-M71RC/2774931)

* **1x 3.3v Regulator (MCP1700-3302E/TO):** [Digi-Key](https://www.digikey.com/en/products/detail/microchip-technology/MCP1700-3302E-TO/652680)

* **2x 1uF Capacitor:** [Digi-Key](https://www.digikey.com/en/products/detail/tdk-corporation/FG18X5R1H105KRT06/5802871) 

* **1x 6mm x 6mm x 5mm Push Button:** [Digi-Key](https://www.digikey.com/en/products/detail/te-connectivity-alcoswitch-switches/1825910-6/1632536)

* **2x 1x12 Female Header:** [Digi-Key](https://www.digikey.com/en/products/detail/sullins-connector-solutions/PPTC121LFBN-RC/807231)

* **5x M2x10mm Screws for 3D Case**

<img src="https://github.com/timville85/4dapter/assets/31223405/98f82b4f-083f-4438-bafa-7ce2acf0fb60" width=25% height=25%>
<img src="https://github.com/timville85/4dapter/assets/31223405/7358a154-84c5-4bab-9e5e-43bf4f743833" width=25% height=25%>


## 4dapter Special Buttons

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

## Arduino Code Install Instructions

**PLEASE NOTE: Installing this sketch will remove the self-reset listener functionality from the Arduino board. This means you will need to manually trigger a "reset" on the Arduino to replace the code on the board. A reset button has been included on the PCB to hanlde this situation.**

**Note: If your board does not have a reset button, you can manually reset the Arduino by connecting the RST and GND pins together briefly.**

1. Add the following URL as an Additional Board Manager URL (in File -> Preferences menu).
`https://github.com/CrazyRedMachine/Arduino-Lufa/raw/master/package_arduino-lufa_index.json`

2. Select and Install **`LUFA AVR Boards`** from the Arduino Board Manager.

3. Select **`Arduino LUFA AVR Boards -> Arduino Leonardo (LUFA)`** as your board type.

4. Download the project to the Arduino Pro Micro as normal. Remember after download your Arduino Pro Micro will no longer report as a serial device and will not appear in the "Port" list in the Arduino software.

Note: If you are replacing existing code on the 4dapter, trigger the download from the Arduino software and then press the reset on the 4dapter within 1-2 seconds of starting the download. If the download fails initially due to not finding the COM port, repeat the download + reset process and it should work the second time.

5. Your 4dapter will now show up as a Switch Controller. Remember that all controller inputs will report for the same player, so multiple players need multiple 4dapter units!
