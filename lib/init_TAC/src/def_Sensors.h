#pragma once
#include <Arduino.h>

// Sensor settings
#define SENSORS_NUM 26
#define MEASURMENT_NUM 8
#define MAX_I2C_ADDRESSES 4
#define JSON_BUFFER 15000

// ---- Classes and Enum ---- //

enum class ConnectorType : uint8_t {
  I2C, ONE_WIRE, ADC, I2C_5V, SPI_CON, EXTRA
};

enum ValueOrder {
  NOT = -1, VOLT, TEMP, HUMIDITY, PRESSURE, DISTANCE,
  TDSv, MOIS, LUX, AMBIENT, H2v, COv, CO2v, NO2v,
  NH3v, C4H10v, C3H8v, CH4v, C2H5OHv, ALTITUDE,
  MV, MGL, MSCM, PH, DBA, DEPTH, UV_I, RGB, ANGLE, KOHM
};

enum SensorsImplemented {
  NO = -1, BMP_280, LEVEL, VEML7700, TDS, CAP_SOIL,
  CAP_GROOVE, DHT_22, DS18B20, MULTIGAS, MULTIGAS_V1,
  RTCDS3231, BATTERY, WS2812, SERVO, BME_280, ADS1115,
  SOUND, PRE_LVL, UV_DFR, LIGHT_DFR, DFR_LM35,
  DFR_FLAME, DHT_11, BMP_680, BH_1750, SHT_21
};

class Measurement {
public:
  double value;
  ValueOrder valueOrder;
  String unit;
  String data_name;
};

class Sensor {
public:
  SensorsImplemented sensor_id;
  String sensor_name;
  String con_typ;
  int returnCount;
  Measurement measurements[8];
  int addr_num;
  String possible_i2c_add[4];

  Sensor();
  Sensor(const Sensor &s);
};

// Funktionen
ValueOrder getValueOrderFromString(String str);

// Proto JSON
extern const char *proto_sensors;
