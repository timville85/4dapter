# 4dapter
### Open Source Controller Adapter for Nintendo Switch / MiSTer / XInput / Batocera / RetroArch

This project combines the NES, SNES, N64 and Genesis DaemonBite Retro Controllers projects together with a custom PCB to support 4 different controllers with a single socketed Arduino Pro Micro. PCB (KiCad + Gerbers), 3D Cases, and muliple firmware versions (Arduino code) all available in this repository.

<img src="https://github.com/timville85/4dapter/assets/31223405/ef0abcfb-1a11-40ba-a59b-c8b400074be6" width=30% height=30%>
<img src="https://github.com/timville85/4dapter/assets/31223405/1e0bf1f6-3932-423e-93d6-e7c35eedb27a" width=25% height=25%>
<img src="https://github.com/timville85/4dapter/assets/31223405/64f502cb-03b1-4d93-b38f-2e75e7a82c00" width=25% height=25%>

PCB Kits + Fully Assembled units (with firmware of your choice) available on [Tindie](https://www.tindie.com/products/31785/)

3 different firmware versions are available in this repo:
* Default: Optimized for MiSTer, PC, Raspberry Pi, etc. - reports as 3 separate controllers (NES/SNES combined) and supports multiplayer from a single unit.
* Analogue Pocket: Optimized for Pocket Dock - reports as a single wired XInput device.
* Nintendo Switch: Optimized for Nintendo Switch Online NES, SNES, and Genesis collections - reports as a single wired switch controller.

**MiSTer Users - Important Info:** For maximum compatibly, install the MiSTer controller Map file found in the [MiSTer Maps Folder](https://github.com/timville85/4dapter/tree/main/MiSTer%20Maps) to your `/media/fat/config/inputs` directory on your MiSTer SD card and reboot your MiSTer. After doing this, you'll need to map the N64 controller in the N64 core for all buttons to work. The SNES / Genesis / NES cores will already be properly configured via the Map file.

## Resources and Thanks

**Special thanks to** [Dinierto Designs](https://twitter.com/DiniertoDesigns) **and** [Super Retro City X](https://twitter.com/SuperRetroCity) **for their assistance during development and testing of the 4dapter. Also thanks to** [Mechevarria](https://github.com/mechevarria) **for Batocera support and testing.**

**3D Case files were also designed by** [Dinierto Designs](https://twitter.com/DiniertoDesigns) **and are available in the 3D Cases folder.**

* [DaemonBite Retro Controllers](https://github.com/MickGyver/DaemonBite-Retro-Controllers-USB)
* https://github.com/esden/pretty-kicad-libs
* https://github.com/ddribin/nes-port-breadboard
* https://github.com/Biacco42/ProMicroKiCad
* http://www.neshq.com/hardgen/powerpad.txt
* https://www.instructables.com/Use-an-Arduino-with-an-N64-controller/

See individual README files in firmware folders for more specific instructions on button assignments and firmware configurations.

## Wiring Diagram
![3dapter _ 4dapter Controller Wiring (1)](https://github.com/timville85/4dapter/assets/31223405/84c51645-eefc-4a31-bbe4-704a30f0dd75)

Current Draw Readings from DIO Pin 16 (used for 5v supply for DB9 port):
* Krikzz Joyzz:	38mA
* 8BitDo M30 2.4G: 29mA
* OEM SEGA 3-Button Wired: 3mA
* OEM SEGA 6-Button Wired: 3mA
* Retrobit 6-Button Wired: 2mA
(Arduino DIO Max Rated Current: 40mA)

## Tested Controllers

The following controllers have been personally tested and are supported with the Triple Controller. All listed devices also fit when using the 3D Case as well.

NES:
* OEM NES Controller
* OEM NES PowerPad (Default FW Only)
* 8BitDo N30 2.4G Receiver
* 8BitDo NES Retro Receiver

SEGA / Genesis:
* OEM SEGA Master System 2-Button Controller
* OEM Genesis 3-Button Controller
* OEM Genesis 6-Button Controller
* 8BitDo M30 2.4G Receiver
* 8BitDo Genesis Retro Receiver
* Krikzz Joyzz

SNES:
* OEM SNES Controller
* OEM SFC Controller
* OEM SNES NTT Controller (Default FW Only)
* 8BitDo SN30 2.4G Receiver
* 8BitDo SNES Retro Receiver

N64:
* OEM N64 Controller
* Retro Fighters Brawler64 V1/V2
* Retro Fighters Brawler64 Wireless Edition

## 4dapter Bill of Materials (BOM)

4dapter was designed with all components to be through-hole soldered to make the project as accessible as possible.

* **1x 4dapter Main Board**

* **1x 4dapter N64 Daugher Board**

* **1x SNES Socket (90 deg):** [AliExpress - Gamers Zone Store](https://www.aliexpress.com/item/32838396935.html) 

* **1x NES Socket:** [AliExpress - Gamers Zone Store](https://www.aliexpress.com/item/1005003699734963.html)

* **1x Genesis Socket:** [AliExpress - Gamers Zone Store](https://www.aliexpress.com/item/1005003699497865.html)

* **1X N64 Socket:** [AliExpress - Gamers Zone Store](https://www.aliexpress.us/item/3256803534347876.html)

* **1x Arduino Pro Micro (USB-C 3-15V Version):** [AliExpress](https://www.aliexpress.us/item/2251832700923799.html)

* **1x 1x3 90 Degree Male Header:** [Digi-Key](https://www.digikey.com/en/products/detail/sullins-connector-solutions/PREC003SBAN-M71RC/2774931)

* **1x 3.3v Regulator (MCP1700-3302E/TO):** [Digi-Key](https://www.digikey.com/en/products/detail/microchip-technology/MCP1700-3302E-TO/652680)

* **2x 1uF Capacitor:** [Digi-Key](https://www.digikey.com/en/products/detail/tdk-corporation/FG18X5R1H105KRT06/5802871) 

* **1x 6mm x 6mm x 5mm Push Button:** [Digi-Key](https://www.digikey.com/en/products/detail/te-connectivity-alcoswitch-switches/1825910-6/1632536)

* **2x 1x12 Female Header:** [Digi-Key](https://www.digikey.com/en/products/detail/sullins-connector-solutions/PPTC121LFBN-RC/807231)

* **5x M2x10mm Screws for 3D Case**

<img src="https://github.com/timville85/4dapter/assets/31223405/98f82b4f-083f-4438-bafa-7ce2acf0fb60" width=40% height=40%>
<img src="https://github.com/timville85/4dapter/assets/31223405/7358a154-84c5-4bab-9e5e-43bf4f743833" width=25% height=25%>
