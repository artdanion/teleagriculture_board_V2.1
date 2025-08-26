#include <Arduino.h>
#include <init_Board.h>

#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_wpa2.h"
#include "esp_sntp.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "driver/timer.h"
#include <sys/time.h>
#include <time.h>

#include <ArduinoJson.h>

#include <Wire.h>
#include <SPI.h>
#include "driver/i2c.h"
#include <driver/spi_master.h>
#include <SD.h>

#include <lmic.h>
#include <hal/hal.h>
#include <LoraMessage.h>

#include <FS.h>
#include "SPIFFS.h"
#include <vector>
#include <WString.h>

#include <driver/rtc_io.h>
#include <RTClib.h>

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


// ------------------- RTC Variablen -------------------
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool loraJoined = false;

RTC_DATA_ATTR u4_t RTC_LORAWAN_netid = 0;
RTC_DATA_ATTR devaddr_t RTC_LORAWAN_devaddr = 0;
RTC_DATA_ATTR u1_t RTC_LORAWAN_nwkKey[16];
RTC_DATA_ATTR u1_t RTC_LORAWAN_artKey[16];
RTC_DATA_ATTR u4_t RTC_LORAWAN_seqnoUp = 0;
RTC_DATA_ATTR u4_t RTC_LORAWAN_seqnoDn = 0;
RTC_DATA_ATTR u1_t RTC_LORAWAN_dn2Dr = 0;
RTC_DATA_ATTR u1_t RTC_LORAWAN_dnConf = 0;
RTC_DATA_ATTR s1_t RTC_LORAWAN_adrTxPow = 0;
RTC_DATA_ATTR u1_t RTC_LORAWAN_txChnl = 0;
RTC_DATA_ATTR s1_t RTC_LORAWAN_datarate = 0;
RTC_DATA_ATTR u2_t RTC_LORAWAN_channelMap = 0;
RTC_DATA_ATTR s2_t RTC_LORAWAN_adrAckReq = 0;
RTC_DATA_ATTR u1_t RTC_LORAWAN_rx1DrOffset = 0;
RTC_DATA_ATTR u1_t RTC_LORAWAN_rxDelay = 0;

#if (CFG_eu868)
RTC_DATA_ATTR u4_t RTC_LORAWAN_channelFreq[MAX_CHANNELS];
RTC_DATA_ATTR u2_t RTC_LORAWAN_channelDrMap[MAX_CHANNELS];
RTC_DATA_ATTR u4_t RTC_LORAWAN_channelDlFreq[MAX_CHANNELS];
RTC_DATA_ATTR band_t RTC_LORAWAN_bands[MAX_BANDS];
#endif

RTC_DATA_ATTR lmic_t RTC_LMIC;

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

// ------------------- Globale Objekte -------------------
esp_adc_cal_characteristics_t adc_cal;

Sensor allSensors[SENSORS_NUM];
Measurement measurements[MEASURMENT_NUM];

// Global vector to store connected Sensor data
std::vector<Sensor> sensorVector;
std::vector<Measurement> show_measurements;

DoubleResetDetector *drd = nullptr;
Adafruit_ST7735 *tft = nullptr;
SPIClass *spi = nullptr;

WiFiManager wifiManager;
WiFiClientSecure client;
WebServer server(80);

Ticker blinker;
Button upButton(LEFT_BUTTON_PIN);
Button downButton(RIGHT_BUTTON_PIN);

RTC_DS3231 rtc;

// ------------------- Statusvariablen -------------------
int backlight_pwm = 250;
bool displayRefresh = true;

const unsigned TX_INTERVAL = 30U;
bool loraJoinFailed = false;
bool loraDataTransmitted = false;

bool portalRunning = false;
bool _enteredConfigMode = false;
bool connectorsSaved = false;
bool configSaved = false;
int total_measurement_pages = 1;
int currentPage = 0;
int lastPage = -1;
time_t prevDisplay = 0;
int num_pages = NUM_PAGES;

struct tm timeInfo = {};
uint32_t userUTCTime = 0;
String lastUpload;
bool initialState = false;
bool ledState = false;
bool gotoSleep = true;
bool userWakeup = false;
bool forceConfig = false;
bool freshBoot = true;

bool sendDataWifi = false;
bool sendDataLoRa = false;
bool no_upload = false;
bool useSDCard = false;

int currentDay = 0;
int lastDay = -1;
unsigned long lastExecutionTime = 0;
int seconds_to_wait = 0;

unsigned long upButtonsMillis = 0;
unsigned long previousMillis = 0;
unsigned long previousMillis_long = 0;
unsigned long previousMillis_upload = 0;
const long interval = 1 * mS_TO_MIN_FACTOR;
const long interval2 = 5 * mS_TO_MIN_FACTOR;

double vs[101] = {0};


// ------------------- Funktionen -------------------

void initBoard()
{
    // reset Pins after holding them during deep sleep
   gpio_reset_pin(GPIO_NUM_0);
   gpio_reset_pin((gpio_num_t)SW_3V3);
   gpio_reset_pin((gpio_num_t)SW_5V);
   gpio_reset_pin((gpio_num_t)TFT_BL);

   gpio_hold_dis((gpio_num_t)SW_3V3);
   gpio_hold_dis((gpio_num_t)SW_5V);
   gpio_hold_dis((gpio_num_t)TFT_BL);
   gpio_deep_sleep_hold_dis();

   pinMode(BATSENS, INPUT_PULLDOWN);
   pinMode(TFT_BL, OUTPUT);
   pinMode(LED, OUTPUT);
   pinMode(LORA_CS, OUTPUT);
   pinMode(SPI_CON_CS, OUTPUT);
   pinMode(SW_3V3, OUTPUT);
   pinMode(SW_5V, OUTPUT);

   delay(1000); // for debugging in screen

   Serial.setTxTimeoutMs(5); // set USB CDC Time TX
   Serial.begin(115200);     // start Serial for debuging

   spi = new SPIClass(HSPI);
   spi->begin(TFT_SCLK, TFT_MISO, TFT_MOSI, SPI_CON_CS);

   analogWrite(TFT_BL, 0); // turn off TFT Backlight

   // Print the wakeup reason for ESP32
   doubleReset_wakeup_reason();
   GPIO_wake_up();
   esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

   delay(100);

   // Increment boot number and print it every reboot
   ++bootCount;
   Serial.println("\nBoot number: " + String(bootCount));
   Serial.printf("LoRa has joined: %s\n", loraJoined ? "true" : "false");

   if (bootCount == 1 || (bootCount % 720) == 0) // new join once every 24h
      freshBoot = true;
   if (bootCount > 60480) // bootCount resets every 84 days
      bootCount = 0;

   upButton.begin();
   downButton.begin();

   // calibrate ADC1

   esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_2_5, ADC_WIDTH_BIT_12, 0, &adc_cal);
   adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_2_5); // BATSENS
   adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_2_5); // ADC1_CON
   adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_2_5); // ADC2_CON
   adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_2_5); // ADC3_CON

   initVoltsArray();

   digitalWrite(SW_3V3, HIGH);
   digitalWrite(LORA_CS, HIGH);

   Wire.setPins(I2C_SDA, I2C_SCL);
}

void doubleReset_wakeup_reason()
{
   esp_sleep_wakeup_cause_t wakeup_reason;

   wakeup_reason = esp_sleep_get_wakeup_cause();
   Serial.println();

   switch (wakeup_reason)
   {
   case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Wakeup caused by external signal using RTC_IO");
      break;
   case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      break;
   case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Wakeup caused by timer");
      break;
   case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial.println("Wakeup caused by touchpad");
      break;
   case ESP_SLEEP_WAKEUP_ULP:
      Serial.println("Wakeup caused by ULP program");
      break;
   default:
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
      break;
   }

   drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);

   // Double Reset detected caused not by deep sleep timer
   if (drd->detectDoubleReset() && (wakeup_reason != ESP_SLEEP_WAKEUP_TIMER))
   {
      Serial.println(F("\nForcing config mode as there was a Double reset detected"));
      forceConfig = true;
   }
}

void GPIO_wake_up()
{
   esp_sleep_wakeup_cause_t wakeup_reason;
   wakeup_reason = esp_sleep_get_wakeup_cause();

   uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
   Serial.print("\nGPIO that triggered the wake up: GPIO ");
   Serial.println((log(GPIO_reason)) / log(2), 0);

   if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1)
   {
      userWakeup = true;
      freshBoot = false;

      if (useDisplay)
         gotoSleep = false;
   }

   if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
   {
      freshBoot = false;
   }
}

void initDisplay()
{
      tft = new Adafruit_ST7735(spi, TFT_CS, TFT_DC, TFT_RST);
      tft->setSPISpeed(1000000);

      // ----- Initiate the TFT display and Start Image----- //
      tft->initR(INITR_GREENTAB); // work around to set protected offset values
      tft->initR(INITR_BLACKTAB); // change the colormode back, offset values stay as "green display"

      tft->cp437(true);
      tft->setCursor(0, 0);
      tft->setRotation(3);
}

void initVoltsArray()
{
   vs[1] = 3.250;
   vs[2] = 3.300;
   vs[3] = 3.350;
   vs[0] = 3.200;
   vs[4] = 3.400;
   vs[5] = 3.450;
   vs[6] = 3.500;
   vs[7] = 3.550;
   vs[8] = 3.600;
   vs[9] = 3.650;
   vs[10] = 3.700;
   vs[11] = 3.703;
   vs[12] = 3.706;
   vs[13] = 3.710;
   vs[14] = 3.713;
   vs[15] = 3.716;
   vs[16] = 3.719;
   vs[17] = 3.723;
   vs[18] = 3.726;
   vs[19] = 3.729;
   vs[20] = 3.732;
   vs[21] = 3.735;
   vs[22] = 3.739;
   vs[23] = 3.742;
   vs[24] = 3.745;
   vs[25] = 3.748;
   vs[26] = 3.752;
   vs[27] = 3.755;
   vs[28] = 3.758;
   vs[29] = 3.761;
   vs[30] = 3.765;
   vs[31] = 3.768;
   vs[32] = 3.771;
   vs[33] = 3.774;
   vs[34] = 3.777;
   vs[35] = 3.781;
   vs[36] = 3.784;
   vs[37] = 3.787;
   vs[38] = 3.790;
   vs[39] = 3.794;
   vs[40] = 3.797;
   vs[41] = 3.800;
   vs[42] = 3.805;
   vs[43] = 3.811;
   vs[44] = 3.816;
   vs[45] = 3.821;
   vs[46] = 3.826;
   vs[47] = 3.832;
   vs[48] = 3.837;
   vs[49] = 3.842;
   vs[50] = 3.847;
   vs[51] = 3.853;
   vs[52] = 3.858;
   vs[53] = 3.863;
   vs[54] = 3.868;
   vs[55] = 3.874;
   vs[56] = 3.879;
   vs[57] = 3.884;
   vs[58] = 3.889;
   vs[59] = 3.895;
   vs[60] = 3.900;
   vs[61] = 3.906;
   vs[62] = 3.911;
   vs[63] = 3.917;
   vs[64] = 3.922;
   vs[65] = 3.928;
   vs[66] = 3.933;
   vs[67] = 3.939;
   vs[68] = 3.944;
   vs[69] = 3.950;
   vs[70] = 3.956;
   vs[71] = 3.961;
   vs[72] = 3.967;
   vs[73] = 3.972;
   vs[74] = 3.978;
   vs[75] = 3.983;
   vs[76] = 3.989;
   vs[77] = 3.994;
   vs[78] = 4.000;
   vs[79] = 4.008;
   vs[80] = 4.015;
   vs[81] = 4.023;
   vs[82] = 4.031;
   vs[83] = 4.038;
   vs[84] = 4.046;
   vs[85] = 4.054;
   vs[86] = 4.062;
   vs[87] = 4.069;
   vs[88] = 4.077;
   vs[89] = 4.085;
   vs[90] = 4.092;
   vs[91] = 4.100;
   vs[92] = 4.111;
   vs[93] = 4.122;
   vs[94] = 4.133;
   vs[95] = 4.144;
   vs[96] = 4.156;
   vs[97] = 4.167;
   vs[98] = 4.178;
   vs[99] = 4.189;
   vs[100] = 4.200;
}

int getBatteryChargeLevel()
{
   int chargeLevel = getChargeLevel(getBatteryVoltage());
   return chargeLevel;
}

float getBatteryVoltage()
{
    uint32_t raw = adc1_get_raw(ADC1_CHANNEL_3);
    uint32_t millivolts = esp_adc_cal_raw_to_voltage(raw, &adc_cal);
    const uint32_t upper_divider = 442;
    const uint32_t lower_divider = 160;
    return (float)(upper_divider + lower_divider) / lower_divider / 1000 * millivolts;
}

int getChargeLevel(double volts)
{
   int idx = 50;
   int prev = 0;
   int half = 0;
   if (volts >= 4.2)
   {
      return 100;
   }
   if (volts <= 3.2)
   {
      return 0;
   }
   while (true)
   {
      half = abs(idx - prev) / 2;
      prev = idx;
      if (volts >= vs[idx])
      {
         idx = idx + half;
      }
      else
      {
         idx = idx - half;
      }
      if (prev == idx)
      {
         break;
      }
   }
   return idx;
}