/*\
 *
 * TeleAgriCulture Board Definitions
 *
 * Copyright (c) 2023 artdanion
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
\*/

#include <Arduino.h>
#include <board_credentials.h>
#include <WString.h>

#define SENSORS_NUM 15      // Number of Sensors implemeted
#define MEASURMENT_NUM 8    // max. Sensor values / Sensor (Multi Gas Sensor V1 produces 8 measures to send)
#define MAX_I2C_ADDRESSES 3 // max. stored I2C addresses / Sensor
#define I2C_NUM 4
#define ADC_NUM 3
#define ONEWIRE_NUM 3
#define SPI_NUM 1
#define I2C_5V_NUM 1
#define EXTRA_NUM 2
#define JSON_BUFFER 7000 // json buffer for prototype Sensors class ( const char *sensors = R"([.....])" )

// ----- Declare Connectors ----- //

int I2C_con_table[I2C_NUM];
int ADC_con_table[ADC_NUM];
int OneWire_con_table[ONEWIRE_NUM];
int SPI_con_table[SPI_NUM];
int I2C_5V_con_table[I2C_5V_NUM];
int EXTRA_con_table[EXTRA_NUM];

// ----- Declare Connectors ----- //

// ***  initial values  ***  // will be overwritten by config file / user input at board setup

String customNTPaddress = "129.6.15.28";

bool useBattery = false;
bool useDisplay = true;
bool useEnterpriseWPA = false;
bool useNTP = false;
bool useCustomNTP = false;
bool loraChanged = false;

String upload = "WIFI";
String anonym = "anonymus@example.com";
String user_CA = "-----BEGIN CERTIFICATE----- optional -----END CERTIFICATE-----";
String setTime_value = "";
String timeZone = "";

String hostname = "TeleAgriCulture Board";

const char GET_Time_SERVER[30] = "www.teleagriculture.org";
String GET_Time_Address = "https://www.teleagriculture.org";
const int SSL_PORT = 443;
const unsigned long TIMEOUT = 2500;

static osjob_t sendjob;
uint8_t dev_eui[8]={ 0, 0, 0, 0, 0, 0, 0, 0 };
uint8_t app_eui[8]={ 0, 0, 0, 0, 0, 0, 0, 0 };
uint8_t app_key[16]={ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// Saves the LMIC structure during DeepSleep
RTC_DATA_ATTR lmic_t RTC_LMIC;

// ----- Define Pins ----- //
#define I2C_SDA 8 // on teleAgriCulture Board V2.0 I2C_5V SDA is GPIO 15
#define I2C_SCL 9 // on teleAgriCulture Board V2.0 I2C_5V SCL is GPIO 16

#define TFT_SCLK 36
#define TFT_MISO 37
#define TFT_MOSI 35
#define SPI_CON_CS 38
#define TFT_CS 15 // on teleAgriCulture Board V2.0 it is I2C_5V SDA pin  CHANGE THIS TO 38(SPI CON CS)
#define TFT_RST 1 // on teleAgriCulture Board V2.0 it is part of J3 CON
#define TFT_DC 2  // on teleAgriCulture Board V2.0 it is part of J3 CON
#define TFT_BL 48 // on teleAgriCulture Board V2.0 it is part of J4 CON

#define LORA_SPI_HOST SPI2_HOST
#define LORA_SPI_DMA_CHAN SPI_DMA_DISABLED
#define LORA_CS 10
#define LORA_MOSI 11
#define LORA_SCLK 12
#define LORA_MISO 13
#define LORA_RST 17
#define LORA_DI0 18
#define LORA_DI1 14 // on teleAgriCulture Board V2.0 it has to be briged to the LORA Module Connector!
#define UNUSED_PIN 0xFF
#define LORA_PIN_RXTX UNUSED_PIN

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = LORA_CS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LORA_RST,
    .dio = {LORA_DI0, LORA_DI1, LMIC_UNUSED_PIN},
    .rxtx_rx_active = 0,
    .rssi_cal = 14,
    .spi_freq = 1000000,
};

// opmode def
// https://github.com/mcci-catena/arduino-lmic/blob/89c28c5888338f8fc851851bb64968f2a493462f/src/lmic/lmic.h#L233

#define ONEWIRE_1 39
#define ONEWIRE_2 40
#define ONEWIRE_3 41

#define ANALOG1 5 // on teleAgriCulture Board V2.0 it is GPIO 4
#define ANALOG2 6 // on teleAgriCulture Board V2.0 it is GPIO 5
#define ANALOG3 7 // on teleAgriCulture Board V2.0 it is GPIO 6
#define BATSENS 4 // on teleAgriCulture Board V2.0 it is GPIO 7

#define SW_3V3 42
#define SW_5V 47

#define LEFT_BUTTON_PIN 0
#define RIGHT_BUTTON_PIN 16 // on teleAgriCulture Board V2.0 it is I2C_5V SCL pin

#define LED 21

// ----- Define Pins ----- //

// ---- Classes and Enum ---- //

enum class ConnectorType : uint8_t
{
  I2C,
  ONE_WIRE,
  ADC,
  I2C_5V,
  SPI_CON,
  EXTRA
};

enum ValueOrder
{
  NOT = -1,
  VOLT,     // temp encoding
  TEMP,     // temp encoding
  HUMIDITY, // humidity encoding
  PRESSURE, // rawfloat encoding
  DISTANCE, // rawfloat encoding
  TDSv,     // uint16 encoding
  MOIS,     // uint16 encoding
  LUX,      // uint16 encoding
  AMBIENT,  // uint16 encoding
  H2v,      // uint16 encoding
  COv,      // uint16 encoding
  CO2v,     // uint16 encoding
  NO2v,     // uint16 encoding
  NH3v,     // uint16 encoding
  C4H10v,   // uint16 encoding
  C3H8v,    // uint16 encoding
  CH4v,     // uint16 encoding
  C2H5OHv,  // uint16 encoding
  ALTITUDE, // uint16 encoding
  RGB,
  ANGLE
};

enum SensorsImplemented
{
  NO = -1,
  BMP_280,
  LEVEL,
  VEML7700,
  TDS,
  CAP_SOIL,
  CAP_GROOVE,
  DHT_22,
  DS18B20,
  MULTIGAS,
  MULTIGAS_V1,
  DS3231,
  BATTERY,
  WS2812,
  SERVO,
  BME_280
};

class Measurement
{
public:
  double value;
  ValueOrder valueOrder;
  String unit;
  String data_name;
};

class Sensor
{
public:
  SensorsImplemented sensor_id;
  String sensor_name;
  String con_typ;
  int returnCount;
  Measurement measurements[8];
  String i2c_add;
  String possible_i2c_add[2];

public:
  Sensor() : sensor_id(), sensor_name(), con_typ(), returnCount(0), i2c_add()
  {
    for (int i = 0; i < 8; ++i)
    {
      measurements[i] = Measurement();
    }
    for (int i = 0; i < 2; ++i)
    {
      possible_i2c_add[i] = "";
    }
  }

  Sensor(const Sensor &s) : sensor_id(s.sensor_id), sensor_name(s.sensor_name), con_typ(s.con_typ), returnCount(s.returnCount), i2c_add(s.i2c_add)
  {
    for (int i = 0; i < 8; ++i)
    {
      measurements[i] = s.measurements[i];
    }
    for (int i = 0; i < 2; ++i)
    {
      possible_i2c_add[i] = s.possible_i2c_add[i];
    }
  }
};

// ---- Classes and Enum ---- //

// ---- PROTOTYPE JSON for implemented sensors (gets loaded at start)

const char *proto_sensors = R"([
  {
    "sensor-id": 1,
    "name": "BMP_280",
    "con_typ": "I2C",
    "returnCount": 3,
    "measurements": [
      {
        "value": 20.3,
        "valueOrder": "TEMP",
        "unit": "°C",
        "data_name": "temp"
      },
      {
        "value": 1000.5,
        "valueOrder": "PRESSURE",
        "unit": "hPa",
        "data_name": "press"
      },
      {
        "value": 300,
        "valueOrder": "ALTITUDE",
        "unit": "m",
        "data_name": "alt"
      }
    ],
    "i2c_add": "0x76",
    "possible_i2c_add": [
      {
        "standard": "0x77"
      },
      {
        "alt_1": "0x76"
      }
    ]
  },
  {
    "sensor-id": 2,
    "name": "Level",
    "con_typ": "I2C",
    "returnCount": 1,
    "measurements": [
      {
        "value": 14.5,
        "valueOrder": "DISTANCE",
        "unit": "mm",
        "data_name": "height"
      }
    ],
    "i2c_add": "0x78",
    "possible_i2c_add": [
      {
        "standard": "0x78"
      },
      {
        "alt_1": "0x77"
      }
    ]
  },
  {
    "sensor-id": 3,
    "name": "VELM7700",
    "con_typ": "I2C",
    "returnCount": 2,
    "measurements": [
      {
        "value": 11255,
        "valueOrder": "LUX",
        "unit": "lx",
        "data_name": "lux"
      },
      {
        "value": 55200,
        "valueOrder": "AMBIENT",
        "unit": "lx",
        "data_name": "ambient"
      }
    ],
    "i2c_add": "0x10",
    "possible_i2c_add": [
      {
        "standard": "0x10"
      }
    ]
  },
  {
    "sensor-id": 4,
    "name": "TDS",
    "con_typ": "ADC",
    "returnCount": 1,
    "measurements": [
      {
        "value": 300,
        "valueOrder": "TDSv",
        "unit": "ppm",
        "data_name": "TDS"
      }
    ]
  },
  {
    "sensor-id": 5,
    "name": "CAP Soil",
    "con_typ": "ADC",
    "returnCount": 1,
    "measurements": [
      {
        "value": 300,
        "valueOrder": "MOIS",
        "unit": ".",
        "data_name": "mois"
      }
    ]
  },
  {
    "sensor-id": 6,
    "name": "CAP Groove",
    "con_typ": "ADC",
    "returnCount": 1,
    "measurements": [
      {
        "value": 300,
        "valueOrder": "MOIS",
        "unit": ".",
        "data_name": "mois"
      }
    ]
  },
  {
    "sensor-id": 7,
    "name": "DHT22",
    "con_typ": "ONE_WIRE",
    "returnCount": 2,
    "measurements": [
      {
        "value": 20.2,
        "valueOrder": "TEMP",
        "unit": "°C",
        "data_name": "temp"
      },
      {
        "value": 55,
        "valueOrder": "HUMIDITY",
        "unit": "%",
        "data_name": "hum"
      }
    ]
  },
  {
    "sensor-id": 8,
    "name": "DS18B20",
    "con_typ": "ONE_WIRE",
    "returnCount": 1,
    "measurements": [
      {
        "value": 20.2,
        "valueOrder": "TEMP",
        "unit": "°C",
        "data_name": "temp"
      }
    ]
  },
  {
    "sensor-id": 9,
    "name": "MultiGasV2",
    "con_typ": "I2C_5V",
    "returnCount": 2,
    "measurements": [
      {
        "value": 5,
        "valueOrder": "COv",
        "unit": "ppm",
        "data_name": "CO"
      },
      {
        "value": 5,
        "valueOrder": "NO2v",
        "unit": "ppm",
        "data_name": "NO2"
      }
    ],
    "i2c_add": "0x08",
    "possible_i2c_add": [
      {
        "standard": "0x08"
      },
      {
        "alt_1": "0x55"
      }
    ]
  },
  {
    "sensor-id": 10,
    "name": "MultiGasV1",
    "con_typ": "I2C_5V",
    "returnCount": 8,
    "measurements": [
      {
        "value": 5,
        "valueOrder": "COv",
        "unit": "ppm",
        "data_name": "CO"
      },
      {
        "value": 5,
        "valueOrder": "NO2v",
        "unit": "ppm",
        "data_name": "NO2"
      },
      {
        "value": 5,
        "valueOrder": "NH3v",
        "unit": "ppm",
        "data_name": "NH3"
      },
      {
        "value": 5,
        "valueOrder": "C3H8v",
        "unit": "ppm",
        "data_name": "C3H8"
      },
      {
        "value": 5,
        "valueOrder": "C4H10v",
        "unit": "ppm",
        "data_name": "C4H10"
      },
      {
        "value": 5,
        "valueOrder": "CH4v",
        "unit": "ppm",
        "data_name": "CH4"
      },
      {
        "value": 5,
        "valueOrder": "H2v",
        "unit": "ppm",
        "data_name": "H2"
      },
      {
        "value": 5,
        "valueOrder": "C2H5oHv",
        "unit": "ppm",
        "data_name": "C2H5OH"
      }
    ],
    "i2c_add": "0x04",
    "possible_i2c_add": [
      {
        "standard": "0x04"
      },
      {
        "alt_1": "0x19"
      }
    ]
  },
  {
    "sensor-id": 11,
    "name": "RTC DS3231",
    "con_typ": "I2C",
    "returnCount": 1,
    "measurements": [
      {
        "value": 20,
        "valueOrder": "TEMP",
        "unit": "°C",
        "data_name": "Temp"
      }
    ],
    "i2c_add": "0x68",
    "possible_i2c_add": [
      {
        "standard": "0x04"
      },
      {
        "alt_1": "0x08"
      }
    ]
  },
  {
    "sensor-id": 12,
    "name": "Battery",
    "con_typ": "EXTRA",
    "returnCount": 1,
    "measurements": [
      {
        "value": 3.7,
        "valueOrder": "VOLT",
        "unit": "V",
        "data_name": "Battery"
      }
    ]
  },
  {
    "sensor-id": 13,
    "name": "WS2812",
    "con_typ": "EXTRA",
    "returnCount": 1,
    "measurements": [
      {
        "value": 1,
        "valueOrder": "RGB",
        "unit": "OK",
        "data_name": "LED"
      }
    ]
  },
  {
    "sensor-id": 14,
    "name": "Servo",
    "con_typ": "EXTRA",
    "returnCount": 1,
    "measurements": [
      {
        "value": 90,
        "valueOrder": "ANGLE",
        "unit": "°",
        "data_name": "angle"
      }
    ]
  },
   {
    "sensor-id": 15,
    "name": "BME_280",
    "con_typ": "I2C",
    "returnCount": 4,
    "measurements": [
      {
        "value": 56,
        "valueOrder": "HUMIDITY",
        "unit": "%",
        "data_name": "hum"
      },
      {
        "value": 20.3,
        "valueOrder": "TEMP",
        "unit": "°C",
        "data_name": "temp"
      },
      {
        "value": 1000.5,
        "valueOrder": "PRESSURE",
        "unit": "hPa",
        "data_name": "press"
      },
      {
        "value": 100,
        "valueOrder": "ALTITUDE",
        "unit": "m",
        "data_name": "alt"
      }
    ],
    "i2c_add": "0x76",
    "possible_i2c_add": [
      {
        "standard": "0x76"
      },
      {
        "alt_1": "0x77"
      }
    ]
  }
])";
