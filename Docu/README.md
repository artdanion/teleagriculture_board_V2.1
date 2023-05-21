# teleAgriCulture Board V2.1

[<img src="https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/vscode.svg" alt="VS Code logo" width="50" height="50">](https://code.visualstudio.com)   &nbsp;   [<img src="https://cdn.platformio.org/images/platformio-logo-xs.fd6e881d.png" alt="PlatformIO logo" height="50">](https://platformio.org) &nbsp; [<img src="https://gitlab.com/teleagriculture/community/-/raw/main/teleAgriCulture%20Board%20V2.1/Docu/pictures/ESP32-S3.png" alt="PlatformIO logo" height="50">](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)

To build the teleAgriCulture Board V2.1 project follow these steps:

1. Clone the repository to your local machine.
2. Open the project in [VS Code](https://code.visualstudio.com) with the [PlatformIO](https://platformio.org) extension installed.
3. Update the board credentials in `/include/board_credentials.h` with your own BoardID, API_KEY and LORA credentials.
4. make sure that you choose the right lora-region build-flag in `platformio.ini` file
5. Build and upload the project to your ESP32-S3 board.

Once the project is uploaded to the board, you can access the Config Portal Access Point with the SSID `TeleAgriCulture Board` and password `enter123`. From there, you can configure the board and view sensor data.
 
<mark>!! to build this project, take care that board_credentials.h is in the include folder (gets ignored by git)</mark>

## Main program

The `main()` function in the `src/main.cpp` file handles the Config Access Point, WiFi, LoRa, load, save, time and display UI. It initializes the board and its sensors and starts the main loop.

The global vector `sensorVector` is used to store connected sensor data. Each element in the vector represents a sensor and contains its measurements. The data name and value for each measurement can be accessed using `sensorVector[i].measurements[j].data_name` and `sensorVector[i].measurements[j].value`, respectively.

In the main loop, the program reads sensor data, updates the display UI and sends data to the server at regular intervals. The board can also enter deep sleep mode to conserve power.

Global vector to store connected Sensor data:
`std::vector<Sensor> sensorVector;`
 
Sensor Data Name: `sensorVector[i].measurements[j].data_name`    --> in order of apperiance (temp, temp1, temp2 ...)
Sensor Value:     `sensorVector[i].measurements[j].value`

## Sensors

The teleAgriCulture Board V2.1 supports a variety of sensors through its connectors. The currently implemented sensors are listed in the `SensorsImplemented` enum in the `include/sensor_Board.hpp` file:

- BMP_280: A temperature and pressure sensor. <mark>BMP280_DEFAULT_ADDRESS 0x77 is used<mark>
- LEVEL: A water level sensor.
- VEML7700: An ambient light sensor. <mark>VEML7700_I2CADDR_DEFAULT 0x10 is used<mark>
- TDS: A total dissolved solids sensor.
- CAP_SOIL: A capacitive soil moisture sensor.
- CAP_GROOVE: A capacitive groove moisture sensor.
- DHT_22: A temperature and humidity sensor.
- DS18B20: A temperature sensor.
- MULTIGAS: A multi-gas sensor. <mark>`gas.begin(Wire, 0x08);` I2C Address is used<mark>
- MULTIGAS_V1: An older version of the multi-gas sensor. <mark>`multiGasV1.begin(0x04);` I2C Address is used<mark>
- DS3231: A real-time clock module - <mark>not implemented now.<mark>
- BATTERY: Battery level sensed on BATSENS Pin.
- WS2812: An RGB LED strip - <mark>not implemented now.<mark>
- SERVO: A servo motor -<mark>not implemented now.<mark>
- BME_280: A temperature, humidity and pressure sensor. <mark>BME280_ADDRESS_ALTERNATE (0x76) is used<mark>

Each sensor has its own library and reading code in the `include/sensor_Read.hpp` file. The data from the sensors is stored in the `sensorVector` and can be accessed using the `sensorVector[i].measurements[j].value` syntax.

New sensors can be added by following the steps outlined in the [Adding new sensors](#adding-new-sensors) section.

## Adding new sensors

To add a new sensor to the teleAgriCulture Board V2.1, follow these steps:

1. Connect the sensor to the board according to its datasheet and the board's pinout.
2. Include the library for the sensor in the `include/sensor_Read.hpp` file.
3. Add the sensor to the `const char *proto_sensors` prototype class in JSON format in the `include/sensor_Board.hpp` file.
4. Add the sensor to the `SensorsImplemented` enum in `include/sensor_Board.hpp`.
5. Add the sensor reading code to one of the following functions in the `include/sensor_Read.hpp` file: `readI2C_Connectors()`, `readADC_Connectors()`, `readOneWire_Connectors()`, `readI2C_5V_Connector()`, or `readEXTRA_Connectors()` according to the sensor type.
6. Create a new Sensor Object `Sensor newSensor = allSensors[SENSOR_ENUM];`
7. Update the `newSensor.measurements[0].value` with the sensor data.
8. Push the `newSensor` object to the `sensorVector` by `sensorVector.push_back(newSensor);`


## Wishlist

- [x] WPA2 enterprise support
- [x] LoRa regions
- [x] deep sleep
- [x] custom NTP server option
- [x] include CA root certificates
- [ ] support for alternative I2C adresses
- [ ] data upload intervall based on UI
- [ ] powermanagment field tests
- [ ] implement DS3231 RTC
- [ ] implement WS2812 LED function and control logic
- [ ] implement Servo function and control logic
 