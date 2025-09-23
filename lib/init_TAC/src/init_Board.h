#pragma once

#include <Arduino.h>

#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_wpa2.h"
#include "esp_sntp.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"

#include <ArduinoJson.h>

#include <Wire.h>
#include <SPI.h>
#include "driver/i2c.h"
#include <driver/spi_master.h>
#include <SD.h>

#include <FS.h>
#include "SPIFFS.h"
#include <vector>
#include <WString.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>

#include <servers.h>
#include <WiFiManager.h>
#include <WebServer.h>

#include <ESP_DoubleResetDetector.h>
#include <Button.h>
#include <Ticker.h>

#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Adafruit_ST7735.h>

#include "def_Board.h"
#include "def_Sensors.h"

// ------------------- Defines -------------------
#define ESP_DRD_USE_SPIFFS true
#define DRD_ADDRESS 0

#define BUTTON_PIN_BITMASK 0x1      // GPIO 0
#define uS_TO_S_FACTOR 1000000UL
#define uS_TO_MIN_FACTOR 60000000UL
#define mS_TO_MIN_FACTOR 60000UL

#define TIMER_DIVIDER 16
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER)
#define BLINK_INTERVAL_MS 500

#define DRD_TIMEOUT 5

#define ST7735_TFTWIDTH 128
#define ST7735_TFTHEIGHT 160
#define background_color 0x07FF

#define NUM_PAGES 4
#define NUM_PERPAGE 9


// ------------------- Zertifikate -------------------
extern const uint8_t x509_crt_bundle_start[] asm("_binary_src_x509_crt_bundle_start");
extern const uint8_t x509_crt_bundle_end[] asm("_binary_src_x509_crt_bundle_end");
extern const uint8_t rootca_bundle_crt_start[] asm("_binary_data_cert_x509_crt_bundle_bin_start");

// ------------------- Globale Objekte -------------------

extern Sensor allSensors[SENSORS_NUM];
extern Measurement measurements[MEASURMENT_NUM];

// Global vector to store connected Sensor data
extern std::vector<Sensor> sensorVector;
extern std::vector<Measurement> show_measurements;

extern esp_adc_cal_characteristics_t adc_cal;

extern DoubleResetDetector *drd;
extern Adafruit_ST7735 *tft;
extern SPIClass *spi;

extern WiFiManager wifiManager;
extern WiFiClientSecure client;
extern WebServer server;

extern Ticker blinker;
extern Button upButton;
extern Button downButton;

// ------------------- Statusvariablen -------------------
extern int backlight_pwm;
extern bool displayRefresh;

extern bool portalRunning;
extern bool _enteredConfigMode;
extern bool connectorsSaved;
extern bool configSaved;
extern int total_measurement_pages;
extern int currentPage;
extern int lastPage;
extern int num_pages;


extern bool initialState;
extern bool ledState;
extern bool gotoSleep;
extern bool userWakeup;
extern bool forceConfig;
extern bool freshBoot;

extern bool sendDataWifi;
extern bool sendDataLoRa;
extern bool sendDataMQTT;
extern bool no_upload;
extern bool useSDCard;

extern double vs[101];

// ------------------- Funktionen -------------------

void initBoard();
void doubleReset_wakeup_reason();
void GPIO_wake_up();

void checkButton(void);
void toggleLED(void);
void startBlinking(void);
void stopBlinking(void);

void initVoltsArray();            //   Copyright (c) 2019 Pangodream   	https://github.com/pangodream/18650CL
int getBatteryChargeLevel();      //   Copyright (c) 2019 Pangodream   	https://github.com/pangodream/18650CL
int getChargeLevel(double volts); //   Copyright (c) 2019 Pangodream   	https://github.com/pangodream/18650CL
float getBatteryVoltage();
