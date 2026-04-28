[<img src="https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/vscode.svg" alt="VS Code logo" width="50" height="50">](https://code.visualstudio.com)   &nbsp;   [<img src="https://cdn.platformio.org/images/platformio-logo-xs.fd6e881d.png" alt="PlatformIO logo" height="50">](https://platformio.org) &nbsp; [<img src="https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/ESP32-S3.png" alt="PlatformIO logo" height="50">](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)

>Firmware is based on this VS Code / Platformio ESP32-S3 Project

## Known Issue: PlatformIO Build Error (intelhex module)

If you encounter a `ModuleNotFoundError: No module named 'intelhex'` during compilation, install the missing module:
```bash
C:\Users\YOUR_USERNAME\.platformio\penv\Scripts\python.exe -m pip install intelhex
```

On Linux/Mac use: `~/.platformio/penv/bin/python -m pip install intelhex`


----

>find the current compiled firmware for OTA upload (Board Setup on the Config Portal) here:
>[current Firmware](https://github.com/artdanion/teleagriculture_board_V2.1/tree/main/Firmware) version 1.80

>
>**If you are using LORA please choose the right Firmware for your location / lora module**
>and you have to enter your credentials for LoRa on the Config Portal

# TeleAgriCulture Board V2.1

![TeleAgriCulture Board V2.1](https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/tac_board_V2_1_02.JPG)

_Picture by Daniel Fischer 2024_


This is a repository for the TeleAgriCulture Board V2.1, a modular and open-source hardware platform for urban agriculture and ecology projects.

## What is TeleAgriCulture?

TeleAgriCulture is a community platform that provides a crowd/cloud data exchange network to promote a more sustainable engagement with food, agriculture and ecology, through creative innovation, community engagement and scientific education.

TeleAgriCulture aims to facilitate access to environmental sensing technologies and data visualization tools for anyone interested in exploring the interactions between plants, animals, humans and their environments.

## What can you do with the TeleAgriCulture Board V2.1?

The TeleAgriCulture Board V2.1 is a versatile and customizable board that can be used for various applications such as:

- Monitoring soil moisture, temperature, humidity, light intensity and other parameters of your plants or crops
- Controlling irrigation systems, LED lights, fans or other actuators based on sensor data or user input
- Connecting to the Internet via WiFi or LoRaWAN and sending/receiving data to/from the TeleAgriCulture Platform or other services
- Creating interactive art installations or educational projects that use environmental data as input

## How to get started with the TeleAgriCulture Board V2.1?

To get started with the TeleAgriCulture Board V2.1, you will need:

- A TeleAgriCulture Board V2.1 kit
- A micro USB cable
- Some sensors and actuators of your choice (optional)

The boards are flashed with the actual firmware at shipping, and ready to use... (or update via OverTheAirUpdate in Config Portal)

Follow these steps: [here a version with pictures](https://gitlab.com/teleagriculture/community/-/blob/main/teleAgriCulture%20Board%20V2.1/Docu/setup.md)

- power the board ( via Micro USB or Waveshare Solar Power Manager board)
- the board creates a WiFi Access Point with a **unique SSID per board**:
  - **Firmware v1.80+**: SSID: `TAC-XXXXXX` (e.g. `TAC-A1B2C3`) — last 6 hex digits of the board's MAC address
  - **Firmware v1.75 and earlier**: SSID: `TeleAgriCulture Board` (static, same on all boards)
- **Password**: `enter123` (same on all versions)
- connect to this Wifi Network with your mobilephone or laptop
- you will be redirected in your browser to the Config Portal of the board ( IP: 192.168.4.1 )
- go through all setup pages and choose your settings
- save the settings

 # Board Functions

## Navigation
- **Navigate through pages**: Use the **BOOT BUTTON** and **SELECT BUTTON** to move between different pages.
- **Wakeup from Deep Sleep**: Press the **BOOT BUTTON** to wake the board from deep sleep mode.

## Configuration Mode
To enter the Configuration Mode (with open Config Portal), you can use one of the following methods:
1. **Long Press**: Press and hold the **BOOT BUTTON** for more than 5 seconds.
2. **Double Reset**: Perform a double reset by resetting the board, waiting for 1 second, then resetting it again.

## Serial Monitor
The Serial Monitor is available with a baud rate of **115200**.

## Additional Features

### LoRa Upload
The board supports LoRa upload for long-range, low-power data transmission.

### No Upload
The board can operate in a mode where no data is uploaded, useful for local data processing or storage.

### LIVE Mode (MQTT / OSC)
The board can stream sensor data in real time over the local network — without uploading to the TAC server. Two protocols are supported:
- **MQTT** — compatible with Home Assistant, Node-RED, and similar tools
- **OSC** (Open Sound Control) — UDP-based, popular in Max/MSP, TouchDesigner, Pure Data

The send rate is capped at 20 per second to avoid flooding the network.

### AP Mode
When the board is not battery-powered, it operates in AP Mode. You can connect to the board's WiFi with the following credentials:
- **SSID**: Teleagriculture DB
- **Password**: enter123

Once connected, you can view a dashboard with live sensor data.

### SD Card Logging
The board supports SD card logging for data storage. Below are the instructions to connect an SD card reader to the SPI connector.

#### Connecting an SD Card Reader
1. **Identify the SPI Connector**: Refer to the image below for the location of the SPI connector on the board.
   ![SPI Connector](https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/SPI_CON.jpeg)

_Picture by Daniel Fischer 2024_


1. **Identify the SD Card Reader Module**: Refer to the image below for the SD card reader module.
   ![SD Card Reader Module](https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/SD_CARD-Modul.jpeg)

_Picture by Daniel Fischer 2024_


   I used this SD Card Module: [External Link](https://www.amazon.de/dp/B09YYG6BT3?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1), but you can use any other SD Card Module with SPI connection and 3.3V.

1. **Connections**:
   - **VCC** to **VCC**
   - **GND** to **GND**
   - **MOSI** to **MOSI**
   - **MISO** to **MISO**
   - **CS** to **CS**

2. **SD Card Format**: Ensure the SD card is formatted to **FAT32** for compatibility.

## Additional Information
- **Battery Mode**: When Battery Mode is enabled, certain features may be limited to conserve power.
- **WiFi Connectivity**: Ensure that your device is within range of the board's WiFi signal for optimal performance.
- **Sensor Data**: The sensor data displayed at **192.168.4.1** includes real-time readings from various sensors connected to the board.

## Troubleshooting
- **Connection Issues**: If you encounter difficulties connecting to the board's WiFi, try resetting the board or moving closer to the device.
- **Serial Monitor**: If the Serial Monitor is not displaying data correctly, ensure that the baud rate is set to **115200**.
- **SD Card Issues**: If the SD card is not recognized, ensure it is properly formatted to **FAT32** and correctly connected.

For more information on how to use the board, please join our community at https://www.teleagriculture.org

---> see how to get data to teleagriculture.org you need an ID and API token
Use of the API to get data out : https://gitlab.com/teleagriculture/community/-/blob/e54ab443139c24a1889cc83034cda92408a1d88b/teleAgriCulture%20Board%20V2.1/Docu/API.md

<span style="background-color: #32C8C8">We would love to hear about your experiances with the board and share your projects with us all...</span>

>**for more info about the source code and if you would like to contribute in the coding goto:**  [Docu Page](https://gitlab.com/teleagriculture/community/-/tree/main/teleAgriCulture%20Board%20V2.1/Docu)
there is also a section about integrated sensors.

## Implemented Sensors

- **ADS1115** — 4-channel 16-bit ADC (I2C 0x48), used for aquaponic probes: ORP, dissolved oxygen, EC, pH
- **BATTERY** — battery voltage read from onboard BATSENS pin
- **BH_1745** — ROHM BH1745NUC RGBC color sensor, outputs Red / Green / Blue / Clear counts (I2C 0x38 / 0x39)
- **BH_1750** — ambient light sensor, lux output (I2C 0x23 / 0x5C)
- **BME_280** — temperature, humidity and barometric pressure (I2C)
- **BMP_280** — temperature and barometric pressure (I2C)
- **BMP_680** — temperature, humidity, pressure and VOC air quality index (I2C)
- **CAP_SOIL** — capacitive soil moisture sensor (analog)
- **CAP_GROOVE** — capacitive groove soil moisture sensor (analog)
- **DFR FLAME** — DFRobot analog flame detection sensor
- **DFR LIGHT** — DFRobot analog ambient light sensor
- **DFR LM35** — DFRobot LM35 analog linear temperature sensor
- **DHT11** — digital temperature and humidity sensor
- **DHT22** — digital temperature and humidity sensor (higher accuracy than DHT11)
- **DS18B20** — 1-Wire waterproof temperature sensor, supports multiple sensors on one bus
- **LEVEL** — analog water level sensor
- **LTR_390** — UV index and ambient light sensor (I2C 0x53)
- **MultiGasV1** — Grove Multichannel Gas Sensor V1 (I2C 0x04) — H₂, CO, NO₂, NH₃, C₄H₁₀, C₃H₈, CH₄, C₂H₅OH
- **MultiGasV2** — Grove Multichannel Gas Sensor V2 (I2C 0x08) — same gas channels, updated hardware
- **PRE_LVL** — DFRobot throw-in liquid level transmitter (analog pressure-based depth sensor)
- **RTCDS3231** — DS3231 real-time clock module with onboard temperature sensor (I2C 0x68)
- **SERVO** — servo motor output *(control logic not yet implemented)*
- **SHT_21** — temperature and humidity sensor (I2C 0x40)
- **SOUND** — DFRobot Gravity analog sound level meter (dB)
- **SPF_WINDVANE** — SparkFun Weather Meter Kit wind vane, 16-direction ADC lookup, output in degrees
- **SPF_ANEMOMETER** — SparkFun Weather Meter Kit anemometer, pulse counting over 5 s, output in km/h
- **TDS** — analog total dissolved solids sensor (water quality, ppm)
- **UV_DFR** — DFRobot ML8511 analog UV sensor, outputs UV index
- **VEML7700** — high-accuracy ambient light sensor, lux and white light (I2C 0x10)
- **WS2812** — RGB LED strip output *(control logic not yet implemented)*

