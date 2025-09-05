#include "def_Sensors.h"

Sensor::Sensor() : sensor_id(), sensor_name(), con_typ(), returnCount(0), addr_num(0)
{
  for (int i = 0; i < 8; ++i)
  {
    measurements[i] = Measurement();
  }
  for (int i = 0; i < 4; ++i)
  {
    possible_i2c_add[i] = "";
  }
}

Sensor::Sensor(const Sensor &s)
    : sensor_id(s.sensor_id), sensor_name(s.sensor_name), con_typ(s.con_typ),
      returnCount(s.returnCount), addr_num(s.addr_num)
{
  for (int i = 0; i < 8; ++i)
  {
    measurements[i] = s.measurements[i];
  }
  for (int i = 0; i < 4; ++i)
  {
    possible_i2c_add[i] = s.possible_i2c_add[i];
  }
}

/* ENUM ValueOrder

  NOT = -1, VOLT, TEMP, HUMIDITY, PRESSURE, DISTANCE,
  TDSv, MOIS, LUX, AMBIENT, H2v, COv, CO2v, NO2v,
  NH3v, C4H10v, C3H8v, CH4v, C2H5OHv, ALTITUDE,
  MV, MGL, MSCM, PH, DBA, DEPTH, UV_I, RGB, ANGLE, KOHM
*/

ValueOrder getValueOrderFromString(String str)
{
  if (str == "VOLT")
    return VOLT;
  if (str == "TEMPERATURE")
    return TEMP;
  if (str == "HUMIDITY")
    return HUMIDITY;
  if (str == "PRESSURE")
    return PRESSURE;
  if (str == "DISTANCE")
    return DISTANCE;
  if (str == "TDSv")
    return TDSv;
  if (str == "MOIS")
    return MOIS;
  if (str == "LUX")
    return LUX;
  if (str == "AMBIENT")
    return AMBIENT;
  if (str == "H2v")
    return H2v;
  if (str == "COv")
    return COv;
  if (str == "CO2v")
    return CO2v;
  if (str == "NO2v")
    return NO2v;
  if (str == "NH3v")
    return NH3v;
  if (str == "C4H10v")
    return C4H10v;
  if (str == "C3H8v")
    return C3H8v;
  if (str == "CH4v")
    return CH4v;
  if (str == "C2H5OHv")
    return C2H5OHv;
  if (str == "ALTITUDE")
    return ALTITUDE;
  if (str == "MV")
    return MV;
  if (str == "MGL")
    return MGL;
  if (str == "MSCM")
    return MSCM;
  if (str == "PH")
    return PH;
  if (str == "DBA")
    return DBA;
  if (str == "DEPTH")
    return DEPTH;
  if (str == "UV_I")
    return UV_I;
  if (str == "RGB")
    return RGB;
  if (str == "ANGLE")
    return ANGLE;
  if (str == "KOHM")
    return KOHM;

  return NOT;
}

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
        "valueOrder": "TEMPERATURE",
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
    "addr_num": 2,
    "possible_i2c_add": {
      "default": "0x76",
      "alt_1": "0x77"
    }
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
    "addr_num": 2,
    "possible_i2c_add": {
      "default": "0x78",
      "alt_1": "0x77"
    }
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
    "addr_num": 1,
    "possible_i2c_add": {
      "default": "0x10"
    }
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
        "value": 80,
        "valueOrder": "MOIS",
        "unit": "%",
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
        "unit": "raw",
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
        "valueOrder": "TEMPERATURE",
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
        "valueOrder": "TEMPERATURE",
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
    "addr_num": 2,
    "possible_i2c_add": {
      "default": "0x08",
      "alt_1": "0x55"
    }
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
    "addr_num": 2,
    "possible_i2c_add": {
      "default": "0x04",
      "alt_1": "0x19"
    }
  },
  {
    "sensor-id": 11,
    "name": "RTCDS3231",
    "con_typ": "I2C",
    "returnCount": 1,
    "measurements": [
      {
        "value": 20,
        "valueOrder": "TEMPERATURE",
        "unit": "°C",
        "data_name": "temp"
      }
    ],
    "addr_num": 1,
    "possible_i2c_add": {
      "default": "0x68"
    }
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
        "valueOrder": "TEMPERATURE",
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
    "addr_num": 2,
    "possible_i2c_add": {
      "default": "0x77",
      "alt_1": "0x76"
    }
  },
  {
    "sensor-id": 16,
    "name": "ADS1115",
    "con_typ": "I2C",
    "returnCount": 4,
    "measurements": [
      {
        "value": 56,
        "valueOrder": "MV",
        "unit": "mV",
        "data_name": "ORP"
      },
      {
        "value": 20.3,
        "valueOrder": "MGL",
        "unit": "mg/L",
        "data_name": "DO"
      },
      {
        "value": 1000.5,
        "valueOrder": "MSCM",
        "unit": "mS/cm",
        "data_name": "EC"
      },
      {
        "value": 100,
        "valueOrder": "PH",
        "unit": "pH",
        "data_name": "PH"
      }
    ],
    "addr_num": 2,
    "possible_i2c_add": {
      "default": "0x48",
      "alt_1": "0x49"
    }
  },
  {
    "sensor-id": 17,
    "name": "Sound LVL",
    "con_typ": "ADC",
    "returnCount": 1,
    "measurements": [
      {
        "value": 20,
        "valueOrder": "DBA",
        "unit": "dBA",
        "data_name": "Sound lvl"
      }
    ]
  },
  {
    "sensor-id": 18,
    "name": "Pressure LVL",
    "con_typ": "ADC",
    "returnCount": 1,
    "measurements": [
      {
        "value": 20,
        "valueOrder": "DEPTH",
        "unit": "mm",
        "data_name": "Pressure lvl"
      }
    ]
  },
  {
    "sensor-id": 19,
    "name": "DFRobot UV",
    "con_typ": "ADC",
    "returnCount": 1,
    "measurements": [
      {
        "value": 0.2,
        "valueOrder": "UV_I",
        "unit": "mW/cm^2",
        "data_name": "UV Int."
      }
    ]
  },
  {
    "sensor-id": 20,
    "name": "DFR LIGHT",
    "con_typ": "ADC",
    "returnCount": 1,
    "measurements": [
      {
        "value": 0.2,
        "valueOrder": "LUX",
        "unit": "lx",
        "data_name": "Light int."
      }
    ]
  },
  {
    "sensor-id": 21,
    "name": "DFR LM35",
    "con_typ": "ADC",
    "returnCount": 1,
    "measurements": [
      {
        "value": 0.2,
        "valueOrder": "TEMPERATURE",
        "unit": "°C",
        "data_name": "temp"
      }
    ]
  },
  {
    "sensor-id": 22,
    "name": "DFR FLAME",
    "con_typ": "ADC",
    "returnCount": 1,
    "measurements": [
      {
        "value": 0.2,
        "valueOrder": "VOLT",
        "unit": "V",
        "data_name": "flame"
      }
    ]
  },
  {
    "sensor-id": 23,
    "name": "DHT11",
    "con_typ": "ONE_WIRE",
    "returnCount": 2,
    "measurements": [
      {
        "value": 20.2,
        "valueOrder": "TEMPERATURE",
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
    "sensor-id": 24,
    "name": "BMP_680",
    "con_typ": "I2C",
    "returnCount": 5,
    "measurements": [
      {
        "value": 20,
        "valueOrder": "TEMPERATURE",
        "unit": "°C",
        "data_name": "temp"
      },
      {
        "value": 40.2,
        "valueOrder": "HUMIDITY",
        "unit": "%",
        "data_name": "hum"
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
      },
      {
        "value": 22.5,
        "valueOrder": "KOHM",
        "unit": "kΩ",
        "data_name": "resist"
      }
    ],
    "addr_num": 2,
    "possible_i2c_add": {
      "default": "0x77",
      "alt_1": "0x76"
    }
  },
  {
    "sensor-id": 25,
    "name": "BH_1750",
    "con_typ": "I2C",
    "returnCount": 1,
    "measurements": [
      {
        "value": 100.5,
        "valueOrder": "LUX",
        "unit": "lx",
        "data_name": "lux"
      }
    ],
    "addr_num": 2,
    "possible_i2c_add": {
      "default": "0x23",
      "alt_1": "0x5C"
    }
  },
  {
    "sensor-id": 26,
    "name": "SHT_21",
    "con_typ": "I2C",
    "returnCount": 2,
    "measurements": [
      {
        "value": 20.5,
        "valueOrder": "TEMPERATURE",
        "unit": "°C",
        "data_name": "temp"
      },
      {
        "value": 50.5,
        "valueOrder": "HUMIDITY",
        "unit": "%",
        "data_name": "hum"
      }
    ],
    "addr_num": 1,
    "possible_i2c_add": {
      "default": "0x40"
    }
  },
  {
    "sensor-id": 26,
    "name": "SHT_21",
    "con_typ": "I2C",
    "returnCount": 2,
    "measurements": [
      {
        "value": 20.5,
        "valueOrder": "TEMPERATURE",
        "unit": "°C",
        "data_name": "temp"
      },
      {
        "value": 50.5,
        "valueOrder": "HUMIDITY",
        "unit": "%",
        "data_name": "hum"
      }
    ],
    "addr_num": 1,
    "possible_i2c_add": {
      "default": "0x40"
    }
  },
  {
    "sensor-id": 27,
    "name": "LTR_390",
    "con_typ": "I2C",
    "returnCount": 2,
    "measurements": [
      {
        "value": 100.5,
        "valueOrder": "LUX",
        "unit": "lx",
        "data_name": "lux"
      },
      {
        "value": 5.5,
        "valueOrder": "UV_I",
        "unit": "mW/cm^2",
        "data_name": "UV Index"
      }
    ],
    "addr_num": 1,
    "possible_i2c_add": {
      "default": "0x53"
    }
  }
])";
