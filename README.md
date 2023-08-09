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

Follow these steps:

- power the board ( via Micro USB or Waveshare Solar Power Manager board)
- the board creates a WiFi Access Point ( SSID: <mark style="background-color: lightblue">TeleAgriCulture Board</mark> / Password: <mark style="background-color: lightblue">enter123</mark>)
- connect to this Wifi Network with your mobilephone or laptop
- you will be redirected in your browser to the Config Portal of the board ( IP: 192.168.4.1 )
- go through all setup pages and choose your settings
- save the settings

### Board Functions

- Navigate through pages: BOOT BUTTON and SELECT BUTTON
- Wakeup from Deep Sleep: BOOT BUTTON
- to enter the Config Mode (with open Config Portal):
- - press BOOT BUTTON > 5 sec or 
- - Doueble Reset the board (reset, 1, 2, 3, reset again)
- Serial Monitor is avaiable with 115200 boud rate

For more information on how to use the board, please join our community at https://www.teleagriculture.org

<span style="background-color: #32C8C8">We would love to hear about your experiances with the board and share your projects with us all...</span>

>**for more info about the source code and if you would like to contribute in the coding goto:**  [Docu Page](https://gitlab.com/teleagriculture/community/-/tree/main/teleAgriCulture%20Board%20V2.1/Docu)