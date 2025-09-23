#pragma once
#include <Arduino.h>
#include <WString.h>

// HW Settings
#define I2C_NUM 4
#define ADC_NUM 3
#define ONEWIRE_NUM 3
#define SPI_NUM 1
#define I2C_5V_NUM 1
#define EXTRA_NUM 2


struct I2CConnectorConf {
  int sensorIndex;   // -1 = NO, otherwise zero-based
  int addrIndex;     // Index in possible_i2c_add[]
};

// Connector tables
extern I2CConnectorConf I2C_con_table[I2C_NUM];
extern int ADC_con_table[ADC_NUM];
extern int OneWire_con_table[ONEWIRE_NUM];
extern int SPI_con_table[SPI_NUM];
extern I2CConnectorConf I2C_5V_con_table[I2C_5V_NUM];
extern int EXTRA_con_table[EXTRA_NUM];


// Board settings
// ***  initial values  ***  // will be overwritten by config file / user input at board setup

extern String customNTPaddress;

extern bool useBattery;
extern bool useDisplay;
extern bool saveDataSDCard;
extern bool useEnterpriseWPA;
extern bool useNTP;
extern bool rtcEnabled;
extern bool useCustomNTP;
extern bool loraChanged;
extern bool webpage;
extern bool lora_ADR;

extern int upload_interval;

extern String upload;
extern String live_mode;
extern String anonym;
extern String user_CA;
extern String setTime_value;
extern String timeZone;

extern String hostname;

extern const char GET_Time_SERVER[30];
extern String GET_Time_Address;
extern const int SSL_PORT;
extern const unsigned long TIMEOUT;

// static osjob_t sendjob;
extern uint8_t dev_eui[8];
extern uint8_t app_eui[8];
extern uint8_t app_key[16];

// ----- Define Pins ----- //
#define I2C_SDA 8 // on teleAgriCulture Board V2.0 I2C_5V SDA is GPIO 15
#define I2C_SCL 9 // on teleAgriCulture Board V2.0 I2C_5V SCL is GPIO 16
#define I2C_FREQ 80000 //100000 //400000

#define SPI_CON_CS 38 // used for SD Card

#define TFT_SCLK 36
#define TFT_MISO 37
#define TFT_MOSI 35
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

