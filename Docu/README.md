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
- SOUND: Gravity analog Sound level meter

- ADS1115: for KlimaOasis aquaponic system a 4-channel ADC is added
- - ADC0: https://wiki.dfrobot.com/Gravity_Analog_ORP_Sensor_PRO_SKU_SEN0464
- - ADC1: https://wiki.dfrobot.com/Gravity__Analog_Dissolved_Oxygen_Sensor_SKU_SEN0237
- - ADC2: https://wiki.dfrobot.com/Gravity__Analog_Electrical_Conductivity_Sensor___Meter_V2__K=1__SKU_DFR0300
- - ADC3: https://wiki.dfrobot.com/Gravity__Analog_pH_Sensor_Meter_Kit_V2_SKU_SEN0161-V2



Each sensor has its own library and reading code in the `include/sensor_Read.hpp` file. The data from the sensors is stored in the `sensorVector` and can be accessed using the `sensorVector[i].measurements[j].value` syntax.

New sensors can be added by following the steps outlined in the [Adding new sensors](#adding-new-sensors) section.

## Adding New Sensors

To add new sensors, follow these steps:

1. In `sensor_Board.hpp`:
    - Add new `SensorName` to `ENUM SensorsImplemented` (use a name that has no conflicts with your sensor library).
    - Add new sensor to `json` styled `proto_sensors` (use the same format).
    - Add new `ENUM ValueOrder` if necessary. This also has to be added to `lora_sendData()` and `getValueOrderFromString()`. (Lora data gets sent in the format `<ValueOrder ENUM><Value>` for encoding/decoding).
2. In `sensor_Read.hpp`:
    - Include the SENSOR library.
    - Add implementation to the following functions corresponding to your sensor type with a case statement using your sensor ENUM:
        - `readI2C_Connectors()`
        - `readADC_Connectors()`
        - `readOneWire_Connectors()`
        - `readI2C_5V_Connector()`
        - `readSPI_Connector()`
        - `readEXTRA_Connectors()`
3. Create a new sensor object at the end of your implementation like: `Sensor newSensor = allSensors[ENUM_SENSOR];` (using the prototype sensor)
4. Add sensor measurement to the newSensor object like: `newSensor.measurements[0].value = value;`
5. Push the new sensor object to the global sensorVector like: `sensorVector.push_back(newSensor);`
6. Increase `SENSORS_NUM` in `include/sensor_Board.hpp` by 1.
7. <mark>Share your success with the community :-) <mark>


## Wishlist

- [x] WPA2 enterprise support
- [x] LoRa regions
- [x] deep sleep
- [x] custom NTP server option
- [x] include CA root certificates
- [ ] support for alternative I2C adresses
- [x] data upload intervall based on UI
- [ ] powermanagment field tests
- [ ] implement DS3231 RTC
- [ ] implement WS2812 LED function and control logic
- [ ] implement Servo function and control logic
- [ ] sensor calibration option

### Hardware

- [ ] LiPo Charger circuit on board
- [ ] Grove I2C Connector

 