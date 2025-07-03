[<img src="https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/vscode.svg" alt="VS Code logo" width="50" height="50">](https://code.visualstudio.com)   &nbsp;   [<img src="https://cdn.platformio.org/images/platformio-logo-xs.fd6e881d.png" alt="PlatformIO logo" height="50">](https://platformio.org) &nbsp; [<img src="https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/ESP32-S3.png" alt="PlatformIO logo" height="50">](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)

>Firmware is based on this VS Code / Platformio ESP32-S3 Project

>find the current compiled firmware for OTA upload (Board Setup on the Config Portal) here:
>[current Firmware](https://gitlab.com/teleagriculture/community/-/tree/main/teleAgriCulture%20Board%20V2.1/Firmware) version 1.03
>
>**If you are using LORA please choose the right Firmware for your location / lora module**
>and you have to enter your credentials for LoRa on the Config Portal

# TeleAgriCulture Board V2.1

![TeleAgriCulture Board V2.1](https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/tac_board_V2_1_02.JPG){width=50% height=50%}

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
- the board creates a WiFi Access Point ( SSID: <mark style="background-color: lightblue">TeleAgriCulture Board</mark> / Password: <mark style="background-color: lightblue">enter123</mark>)
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

### AP Mode
When the board is not battery-powered, it operates in AP Mode. You can connect to the board's WiFi with the following credentials:
- **SSID**: Teleagriculture DB
- **Password**: enter123

Once connected, you can view a dashboard with live sensor data.

### SD Card Logging
The board supports SD card logging for data storage. Below are the instructions to connect an SD card reader to the SPI connector.

#### Connecting an SD Card Reader
1. **Identify the SPI Connector**: Refer to the image below for the location of the SPI connector on the board.
   ![SPI Connector](https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/SPI_CON.jpeg){width=50% height=50%}

2. **Identify the SD Card Reader Module**: Refer to the image below for the SD card reader module.
   ![SD Card Reader Module](https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/SD_CARD-Modul.jpeg){width=50% height=50%}

   I used this SD Card Module: [External Link](https://www.amazon.de/dp/B09YYG6BT3?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1), but you can use any other SD Card Module with SPI connection and 3.3V.

3. **Connections**:
   - **VCC** to **VCC**
   - **GND** to **GND**
   - **MOSI** to **MOSI**
   - **MISO** to **MISO**
   - **CS** to **CS**

4. **SD Card Format**: Ensure the SD card is formatted to **FAT32** for compatibility.

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