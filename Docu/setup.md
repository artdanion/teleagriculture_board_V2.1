## How to setup the TeleAgriCulture V2.1 board

First connect Power (via USB or Solar Board)

<img src="https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/board_power.jpg" alt="Board Power" width="400" height="400">

Switch board into Config Mode (new boards start into config mode)

- to enter the Config Mode later (with open Config Portal):
- - press BOOT BUTTON > 5 sec or 
- - Doueble Reset the board (reset, 1, 2, 3, reset again)

<img src="https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/board_config.jpg" alt="Board Config Mode" width="400" height="400">

Connect to board via WiFi (on your Mobile phone or Notebook)

<img src="https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/board_wifi.jpg" alt="Board WiFi" width="400" height="400">

Config Portal opens automatically or open IP: 192.168.4.1 in a browser (on the device you connected to the Board WiFi)

<img src="https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/board_config_page.jpg" alt="Board Config Page" width="400" height="400">

Now you can change your board settings (WiFi Network, Upload, Sensors, Timezone...)

If you use the Update Option:

>find the current compiled firmware for OTA upload (Board Setup on the Config Portal) here:
>[current Firmware](https://gitlab.com/teleagriculture/community/-/tree/main/teleAgriCulture%20Board%20V2.1/Firmware) version 1.02
>
>**If you are using LORA please choose the right Firmware for your location / lora module**
>and you have to enter your credentials for LoRa on the Config Portal

## Connect your data to the TeleAgriCulture Kits page

<img src="https://gitlab.com/teleagriculture/community/-/blob/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/teleagriculture_kits_page.jpg" alt="TeleAgriCulture Kits Page" width="400" height="400">

- first you need an account on the TeleAgiCulture page. Talk to our admin.
- log in to kits.teleagriculture.org
- click on your kit
- click on configure (Top Right)
- add Sensors, dont change the API-key or Kit-ID (we are sending data verified by this numbers)
- save your settings

**we dont send Sensor or data names. (names use a lot of bytes via Lora transmission)**
**So your Sensor names have to match whats shown on your board (e.g.: temp, temp1, temp2... hum, hum1, ...)**

>**for more info about the source code and if you would like to contribute in the coding goto:**  [Docu Page](https://gitlab.com/teleagriculture/community/-/tree/main/teleAgriCulture%20Board%20V2.1/Docu)
there is also a section about integrated sensors.


## Implemented Sensors

- ADS1115
- BH_1750
- BME_280
- BMP_280
- BMP_680
- DFR FLAME
- DFR LIGHT
- DFR LM35
- DHT11
- DHT22
- DS18B20
- LTR_390
- MultiGasV1
- MultiGasV2
- RTCDS3231
- SHT_21
- TDS
- VELM7700
- WS2812
