/*\
 *
 * TeleAgriCulture Board Firmware
 *
 * Copyright (c) 2025 artdanion
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
 *


//TODO: prepaired but disabled in ConfigPortal -->Cert read and set  ---> String user_CA ---> to usable const char[]
//      WPA Enterprise works without cert and anonym identity at the moment

//TODO: Battery optimation    ---> uses around 3mA without gas sensor and without display
//TODO: Sensor test --> ongoing

//TODO: Sensor calibration (hardcoded at the moment)

//TODO: find a I2C Address solution
//TODO: back channel or Webinterface for LEDs / RGB

/*********************************** VERSION 1.09 ****************************
/*
 *
 * For defines, GPIOs and implemented Sensors, see /include/sensor_Board.hpp
 * board credentials are in /include/board_credentials.h  (BoardID, API_KEY and LORA credentials)
 *
 *
 * ------> Config Portal opens after double reset or holding BooT button for > 5sec
 *____________________________________________________________
 *
 * Config Portal Access Point:   SSID: TeleAgriCulture Board
 *                               pasword: enter123
 *____________________________________________________________
 *
 *
 * !! to build this project, take care that board_credentials.h is in the include folder (gets ignored by git)
 * !! for using LoRa set the right frequency for your region in platformio.ini
 *
 * ___________________________________________________________
 *
 * main() handles Config Accesspoint, WiFi, LoRa, load, save, time and display UI
 * sensorRead() in /include/sensor_Read.hpp takes car of the sensor reading
 * HTML rendering for Config Portal is done here: /lib/WiFiManagerTz/src/WiFiManagerTz.h
 *
 * Global vector to store connected Sensor data:
 * std::vector<Sensor> sensorVector;
 *
 * Sensor Data Name: sensorVector[i].measurements[j].data_name    --> in order of apperiance (temp, temp1, temp2 ...)
 * Sensor Value:     sensorVector[i].measurements[j].value
 *
/*/

/*
   to add new Sensors
   -> sensor_Board.hpp
   -> add new SensorName to ENUM SensorsImplemented (use Name that has no conflicts with your sensor library)
   -> add new Sensor to json styled proto_sensors (use same format)
   -> add new ENUM ValueOrder if necessary -> this has also to be added to lora_sendData() and getValueOrderFromString()
      (Lora Data gets send in the format <ValueOrder ENUM><Value> for encoding/decoding)

   -> include SENSOR library to sensor_Read.hpp
   -> add implementation to   readI2C_Connectors()
                              readADC_Connectors()
                              readOneWire_Connectors()
                              readI2C_5V_Connector()
                              readSPI_Connector()
                              readEXTRA_Connectors()
      corresponding to your Sensortype with case statement using your Sensor ENUM
   -> increase SENSORS_NUM by 1
*/

#include <Arduino.h>
#include <FS.h>
#include "SPIFFS.h"
#include <vector>
#include <WString.h>

#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_wpa2.h"
#include "esp_sntp.h"

#include "driver/timer.h"
#include <driver/rtc_io.h>
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

#include <tac_logo.h>
#include <customTitle_picture.h>

#include <sensor_Board.hpp> // Board and setup defines
#include <sensor_Read.hpp>  // Sensor read handling

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>

#include <servers.h>
#include <WiFiManager.h>
#include <WiFiManagerTz.h> // Setup Page html rendering and input handling
#include <WebServer.h>

#define ESP_DRD_USE_SPIFFS true
#define DOUBLERESETDETECTOR_DEBUG true
#define DRD_ADDRESS 0

#define DEBUG_PRINT false

#include <ESP_DoubleResetDetector.h>
#include <Button.h>
#include <Ticker.h>

#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Adafruit_ST7735.h>

// #define DEBUG_PRINT false

// ----- Deep Sleep related -----//
#define BUTTON_PIN_BITMASK 0x1      // GPIO 0
#define uS_TO_S_FACTOR 1000000UL    /* Conversion factor for micro seconds to seconds */
#define uS_TO_MIN_FACTOR 60000000UL /* Conversion factor for micro seconds to minutes */
#define mS_TO_MIN_FACTOR 60000UL    /* Conversion factor for milli seconds to minutes */

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool loraJoined = false;

// ----- Deep Sleep LORA related -----//
RTC_DATA_ATTR u4_t RTC_LORAWAN_netid = 0;
RTC_DATA_ATTR devaddr_t RTC_LORAWAN_devaddr = 0;
RTC_DATA_ATTR u1_t RTC_LORAWAN_nwkKey[16];
RTC_DATA_ATTR u1_t RTC_LORAWAN_artKey[16];
RTC_DATA_ATTR u4_t RTC_LORAWAN_seqnoUp = 0;
RTC_DATA_ATTR u4_t RTC_LORAWAN_seqnoDn;
RTC_DATA_ATTR u1_t RTC_LORAWAN_dn2Dr;
RTC_DATA_ATTR u1_t RTC_LORAWAN_dnConf;
RTC_DATA_ATTR s1_t RTC_LORAWAN_adrTxPow;
RTC_DATA_ATTR u1_t RTC_LORAWAN_txChnl;
RTC_DATA_ATTR s1_t RTC_LORAWAN_datarate;
RTC_DATA_ATTR u2_t RTC_LORAWAN_channelMap;
RTC_DATA_ATTR s2_t RTC_LORAWAN_adrAckReq;
RTC_DATA_ATTR u1_t RTC_LORAWAN_rx1DrOffset;
RTC_DATA_ATTR u1_t RTC_LORAWAN_rxDelay;

#if (CFG_eu868)
RTC_DATA_ATTR u4_t RTC_LORAWAN_channelFreq[MAX_CHANNELS];
RTC_DATA_ATTR u2_t RTC_LORAWAN_channelDrMap[MAX_CHANNELS];
RTC_DATA_ATTR u4_t RTC_LORAWAN_channelDlFreq[MAX_CHANNELS];
RTC_DATA_ATTR band_t RTC_LORAWAN_bands[MAX_BANDS];
#endif

void doubleReset_wakeup_reason();
void GPIO_wake_up();
// ----- Deep Sleep related -----//

// ----- LED Timer related -----//
#define TIMER_DIVIDER 16
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER)
#define BLINK_INTERVAL_MS 500
// ----- LED Timer related -----//

// ----- Function declaration -----//
void setUPWiFi();
void wifi_sendData(void);
void configModeCallback(WiFiManager *myWiFiManager);
int countMeasurements(const std::vector<Sensor> &sensors);
void checkButton(void);
void toggleLED(void);
void startBlinking(void);
void stopBlinking(void);
void openConfig(void);
void openConfig(void);

// file and storage functions
void SD_sendData(void);
void load_Sensors(void);
void load_Connectors(void);
void save_Connectors(void);
void load_Config(void);
void save_Config(void);
ValueOrder getValueOrderFromString(String str);

// UI functions
void renderPage(int page);
void mainPage(void);
void I2C_ConnectorPage(void);
void ADC_ConnectorPage(void);
void OneWire_ConnectorPage(void);
void measurementsPage(int page);
void deepsleepPage();
void digitalClockDisplay(int x, int y, bool date);
void printDigits(int digits);
void setUploadTime();
void initVoltsArray();            //   Copyright (c) 2019 Pangodream   	https://github.com/pangodream/18650CL
int getBatteryChargeLevel();      //   Copyright (c) 2019 Pangodream   	https://github.com/pangodream/18650CL
int getChargeLevel(double volts); //   Copyright (c) 2019 Pangodream   	https://github.com/pangodream/18650CL
void drawBattery(int x, int y);

void handleRoot(); // WEBSERVER
void handleNotFound();
void drawGraph();
String measurementsTable();
String connectorTable();

// time functions
int seconds_to_next_hour();
void on_time_available(struct timeval *t);
String get_header();
String getDateTime(String header);
time_t convertDateTime(String dateTimeStr);
void get_time_in_timezone(const char *timezone);
void setEsp32Time(const char *timeStr);

// Lora functions
void os_getArtEui(u1_t *buf)
{
   std::copy(app_eui, app_eui + 8, buf);
}
void os_getDevEui(u1_t *buf)
{
   std::copy(dev_eui, dev_eui + 8, buf);
}
void os_getDevKey(u1_t *buf)
{
   std::copy(app_key, app_key + 16, buf);
}

void lora_sendData(void);
void do_send(LoraMessage &message);
void onEvent(ev_t ev);
void convertTo_LSB_EUI(String input, uint8_t *output);
void convertTo_MSB_APPKEY(String input, uint8_t *output);
void saveLORA_State(void);
void loadLORA_State(void);
void LoraWANPrintLMICOpmode(void);
void saveLMICToRTC(int deepsleep_sec);
void loadLMICFromRTC();

// debug fuctions
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void checkLoadedStuff(void);
void printConnectors(ConnectorType typ);
void printProtoSensors(void);
void printMeassurments(void);
void printSensors(void);
void printHex2(unsigned v);

// ----- Function declaration -----//

// ----- SPI related -----//
static const int spiClk = 1000000; // 10 MHz
SPIClass *spi = NULL;

// ----- LoRa related -----//

// https://www.loratools.nl/#/airtime
// Saves the LMIC structure during DeepSleep
RTC_DATA_ATTR lmic_t RTC_LMIC;
const unsigned TX_INTERVAL = 30U;
bool loraJoinFailed = false;
bool loraDataTransmitted = false;
// ----- LoRa related -----//

// ----- Initialize TFT ----- //
#define ST7735_TFTWIDTH 128
#define ST7735_TFTHEIGHT 160
#define background_color 0x07FF

Adafruit_ST7735 *tft = NULL;

// Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

int backlight_pwm = 250;
bool displayRefresh = true;
// ----- Initialize TFT ----- //

// ----- WiFiManager section ---- //
#define WM_NOHELP 1
WiFiManager wifiManager;
WiFiClientSecure client;

extern const uint8_t x509_crt_bundle_start[] asm("_binary_src_x509_crt_bundle_start");
extern const uint8_t x509_crt_bundle_end[] asm("_binary_src_x509_crt_bundle_end");
extern const uint8_t rootca_bundle_crt_start[] asm("_binary_data_cert_x509_crt_bundle_bin_start");
// ----- WiFiManager section ---- //

// ----- globals and defines --------

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 5

DoubleResetDetector *drd;
Ticker blinker;
Button upButton(LEFT_BUTTON_PIN);
Button downButton(RIGHT_BUTTON_PIN);

#define NUM_PAGES 4   // pages with out the measurement pages
#define NUM_PERPAGE 9 // max measurement values per page

bool portalRunning = false;
bool _enteredConfigMode = false;
bool connectorsSaved = false;
bool configSaved = false;
int total_measurement_pages = 1;

int currentPage = 0;
int lastPage = -1;      // Store the last page to avoid refreshing unnecessarily
time_t prevDisplay = 0; // when the digital clock was displayed
int num_pages = NUM_PAGES;

struct tm timeInfo;   // time from NTP or setEsp32Time
uint32_t userUTCTime; // Seconds since the UTC epoch
String lastUpload;
bool initialState; // state of the LED
bool ledState = false;
bool gotoSleep = true;
bool userWakeup = false;
bool forceConfig = false; // if no config file or DoubleReset detected
bool freshBoot = true;    // fresh start - not loading LMIC Config

bool sendDataWifi = false;
bool sendDataLoRa = false;
bool no_upload = false;
bool useSDCard = false;

// Define a variable to hold the last hour value
int currentDay;
int lastDay = -1;
unsigned long lastExecutionTime = 0;
int seconds_to_wait = 0;

unsigned long upButtonsMillis = 0;
unsigned long previousMillis = 0;
unsigned long previousMillis_long = 0;
unsigned long previousMillis_upload = 0;
const long interval = 1 * mS_TO_MIN_FACTOR;  // screen backlight darken time after 1 min
const long interval2 = 5 * mS_TO_MIN_FACTOR; // screen goes black after 5 min

// Battery levels
double vs[101]; //   Copyright (c) 2019 Pangodream   	https://github.com/pangodream/18650CL

WebServer server(80);

void setup()
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
   Serial.printf("LoRa has joined: %s\n", loraJoined ? "true" : "false");

   if (bootCount == 1 || (bootCount % 720) == 0) // new join once every 24h
      freshBoot = true;
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

   // print SPIFF files for debugging
   if (!SPIFFS.begin(true))
   {
      Serial.println("Failed to mount SPIFFS file system");
      return;
   }
   // listDir(SPIFFS, "/", 0);
   // Serial.println();

   load_Sensors();    // Prototypes get loaded
   load_Connectors(); // Connectors lookup table
   load_Config();     // load config Data

#if DEBUG_PRINT
   delay(2000);
   checkLoadedStuff();
#endif

   /* ------------  Test DATA for connected Sensors  --------------------------

      ---> comes from Web Config normaly or out of SPIFF

  I2C_con_table[0] = NO;
  I2C_con_table[1] = NO;
  I2C_con_table[2] = NO;
  I2C_con_table[3] = NO;
  ADC_con_table[0] = CAP_SOIL;
  ADC_con_table[1] = NO;
  ADC_con_table[2] = NO;
  OneWire_con_table[0] = DS18B20;
  ADC_con_table[1] = NO;
  ADC_con_table[2] = NO;
  OneWire_con_table[0] = DS18B20;
  OneWire_con_table[1] = DHT_22;
  OneWire_con_table[2] = NO;
  OneWire_con_table[2] = NO;
  SPI_con_table[0] = NO;
  I2C_5V_con_table[0] = NO;
  I2C_5V_con_table[0] = NO;
  EXTRA_con_table[0] = NO;
  EXTRA_con_table[1] = NO;

  save_Connectors(); // <------------------ called normaly after Web Config

  Test DATA for connected Sensors ---> come from Web Config normaly

  ---------------------------------------------------------------------------------*/

   /*******************************************************************************
   ++++++++++++++++ overwrite stored values for debug   +++++++++++++++                */

   // upload = "LORA";
   // upload = "WIFI";
   // useSDCard = true;
   // useBattery = true;
   // useBattery = false;

   /*******************************************************************************
   ++++++++++++++++ overwrite stored values for debug   +++++++++++++++                */

   Wire.setPins(I2C_SDA, I2C_SCL);

   if (!useBattery)
   {
      webpage = true;
   }

   if (useDisplay || forceConfig)
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

   if (useBattery && useDisplay && (!userWakeup))
   {
      analogWrite(TFT_BL, 0); // turn off TFT Backlight
      tft->fillScreen(ST7735_BLACK);
   }

   if (!useBattery && useDisplay)
   {
      analogWrite(TFT_BL, backlight_pwm); // turn TFT Backlight on
      tft->fillScreen(background_color);
      tft->drawRGBBitmap(0, 0, tac_logo, 160, 128);
   }

   if (userWakeup && useDisplay)
   {
      analogWrite(TFT_BL, backlight_pwm); // turn TFT Backlight on
      tft->fillScreen(ST7735_BLACK);
      tft->setTextColor(0x9E6F);
      tft->setFont(&FreeSans9pt7b);
      tft->setTextSize(1);
      tft->setCursor(5, 50);
      tft->print("Wakeup by USER");
   }
   // ----- Initiate the TFT display  and Start Image----- //

   if (upload == "WIFI" && !forceConfig)
   {

      // Cert field in ConfigPortal deactivated --> do not know if this would work, can not test it now
      // https://github.com/martinius96/ESP32-eduroam
      // WiFiManager handles esp_wpa2.h connection, username and password
      if (useEnterpriseWPA && !forceConfig)
      {
         esp_wifi_sta_wpa2_ent_set_ca_cert((uint8_t *)user_CA.c_str(), strlen(user_CA.c_str()) + 1); // used by @debsahu in his example EduWiFi
         // esp_wifi_sta_wpa2_ent_set_ca_cert((uint8_t *)test_root_ca, strlen(test_root_ca)); //try without strlen +1 too if not working
         esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)anonym.c_str(), strlen(anonym.c_str()));
      }

      // reset settings - wipe stored credentials for testing
      // these are stored by the WiFiManager library
      // wifiManager.resetSettings();

      setUPWiFi();

      startBlinking();

      bool success = (wifiManager.autoConnect("TeleAgriCulture Board", "enter123"));

      if (!success)
      {
         Serial.println("Failed to connect");
         ESP.restart();
      }
      else
      {
         Serial.println("Connected");
         if (useDisplay)
         {
            analogWrite(TFT_BL, backlight_pwm); // turn TFT Backlight on
            tft->fillScreen(ST7735_BLACK);
            tft->setTextColor(ST7735_ORANGE);
            tft->setFont(&FreeSans9pt7b);
            tft->setTextSize(1);
            tft->setCursor(5, 50);
            tft->print("WiFi Connected");
         }
         if ((WiFi.status() == WL_CONNECTED) && useNTP)
         {
            WiFiManagerNS::configTime();
            WiFiManagerNS::NTP::onTimeAvailable(&on_time_available);
         }

         if (!useNTP)
         {
            String header = get_header();
            delay(500);
            String Time1 = getDateTime(header);
            setEsp32Time(Time1.c_str());
         }
         if (!useNTP)
         {
            String header = get_header();
            delay(500);
            String Time1 = getDateTime(header);
            setEsp32Time(Time1.c_str());
         }
      }

      stopBlinking();
   }

   if (forceConfig)
   {
      openConfig();
   }

   if (forceConfig)
   {
      openConfig();
   }

   if (upload == "WIFI")
      sendDataWifi = true;

   if (upload == "LORA")
   {
      WiFiManagerNS::TZ::loadPrefs();

      digitalWrite(SW_3V3, HIGH);
      delay(500);

      sendDataLoRa = true;

      convertTo_LSB_EUI(OTAA_DEVEUI, dev_eui);
      convertTo_LSB_EUI(OTAA_APPEUI, app_eui);
      convertTo_MSB_APPKEY(OTAA_APPKEY, app_key);

      // LMIC init
      os_init();

      if (loraJoined && (!freshBoot)) // new join request every 24h because freshBoot gets reseted
      {
         loadLORA_State(); // load the LMIC settings
         delay(200);

         if ((RTC_LMIC.seqnoUp != 0) && CFG_LMIC_EU_like) // just for EU
         {
            Serial.println("load air time for channels");
            delay(200);
            loadLMICFromRTC();
            delay(200);
         }
      }
      else
      {
         // Reset the MAC state. Session and pending data transfers will be discarded.
         LMIC_reset();
         LMIC_setClockError(MAX_CLOCK_ERROR * 5 / 100);
         freshBoot = false;
         loraJoined = false;
         // LMIC_startJoining();
      }
   }

   if (saveDataSDCard)
   {
      digitalWrite(SW_3V3, HIGH);
      delay(200);

      if (!SD.begin(SPI_CON_CS, *spi))
      {
         Serial.println("Card Mount Failed");
         return;
      }

      uint8_t cardType = SD.cardType();

      if (cardType == CARD_NONE)
      {
         Serial.println("No SD card attached");
         return;
      }

      Serial.print("SD Card Type: ");
      if (cardType == CARD_MMC)
      {
         Serial.println("MMC");
      }
      else if (cardType == CARD_SD)
      {
         Serial.println("SDSC");
      }
      else if (cardType == CARD_SDHC)
      {
         Serial.println("SDHC");
      }
      else
      {
         Serial.println("UNKNOWN");
      }

      uint64_t cardSize = SD.cardSize() / (1024 * 1024);
      Serial.printf("SD Card Size: %lluMB\n", cardSize);

      SD.end();

      digitalWrite(SW_3V3, LOW);

      saveDataSDCard = true;
   }

   if (upload == "NO_UPLOAD")
      no_upload = true;

   if (webpage && !forceConfig)
   {
      WiFi.mode(WIFI_AP_STA);

      WiFi.softAP("TeleAgriCulture DB", "enter123");
      IPAddress IP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(IP);

      if (MDNS.begin("esp32"))
      {
         Serial.println("MDNS responder started");
      }

      server.on("/", handleRoot);
      server.on("/test.svg", drawGraph);
      server.on("/inline", []()
                { server.send(200, "text/plain", "this works as well"); });
      server.onNotFound(handleNotFound);
      server.begin();
      Serial.println("HTTP server started");
   }
}

void loop()
{
   drd->loop(); // Process double reset detection

   analogWrite(TFT_BL, backlight_pwm); // Set the backlight brightness of the TFT display

   if (forceConfig)
   {
      openConfig();
   }

   if (forceConfig)
   {
      openConfig();
   }

   time_t rawtime;
   time(&rawtime);
   localtime_r(&rawtime, &timeInfo); // Get the current local time

   currentDay = timeInfo.tm_mday; // Update the current day

   // Check if the day has changed and perform time synchronization if required
   if ((currentDay != lastDay) && (upload == "WIFI") && !(WiFiManagerNS::NTPEnabled) && freshBoot)
   if ((currentDay != lastDay) && (upload == "WIFI") && !(WiFiManagerNS::NTPEnabled) && freshBoot)
   {
      // The day has changed since the last execution of this block
      String header = get_header();
      delay(1000);
      String Time1 = getDateTime(header);
      setEsp32Time(Time1.c_str()); // Set the time on ESP32 using the obtained time value
      lastDay = currentDay;        // Update the last day value
   }

   unsigned long currentMillis = millis();
   unsigned long currentMillis_long = millis();
   unsigned long currentMillis_upload = millis();

   // check if its time to uplooad based on interval
   if (currentMillis_upload - previousMillis_upload >= (upload_interval * mS_TO_MIN_FACTOR))
   {
      delay(100);

      if (upload == "NO")
         no_upload = true; // Set flag to indicate no upload

      if (upload == "WIFI")
         sendDataWifi = true; // Set flag to send data via WiFi

      if (upload == "LORA" && !useBattery)
      {
         if (loraJoined)
         {
            sendDataLoRa = true; // Set flag to send data via LoRa
         }
      }

      if (saveDataSDCard)
         useSDCard = true; // Set flag to send data via SDCard

      previousMillis_upload = currentMillis_upload;
   }

   // Perform periodic tasks based on intervals
   if (currentMillis - previousMillis >= interval) // lower tft brightness after 1 min
   {
      backlight_pwm = 5; // Turn down the backlight
      previousMillis = currentMillis;

      if (upload == "WIFI" && useBattery)
      {
         gotoSleep = true; // Enable sleep mode if using WiFi and battery power
      }

      if (upload == "LORA" && useBattery)
      if (upload == "LORA" && useBattery)
      {
         gotoSleep = true; // Enable sleep mode if using LoRa and battery power and data transmitted
      }

      if (useSDCard && useBattery)
      {
         gotoSleep = true; // Enable sleep mode if using SDCard and battery power and data transmitted
      }
   }

   if (currentMillis_long - previousMillis_long >= interval2) // turn tft off after 5 min
   {
      backlight_pwm = 0;             // Turn off the backlight
      tft->fillScreen(ST7735_BLACK); // Fill the TFT screen with black color
      previousMillis_long = currentMillis_long;
      displayRefresh = true;
      tft->enableSleep(true); // Enable sleep mode for the TFT display
   }

   // Check button presses to navigate through pages
   if (downButton.pressed())
   {
      currentPage = (currentPage + 1) % num_pages;
      backlight_pwm = 250;
      tft->enableSleep(false); // Disable sleep mode for the TFT display
      gotoSleep = false;
   }

   if (upButton.pressed())
   {
      currentPage = (currentPage - 1 + num_pages) % num_pages;
      backlight_pwm = 250;
      tft->enableSleep(false); // Disable sleep mode for the TFT display
      upButtonsMillis = millis();
      gotoSleep = false;
   }

   if (upButton.released())
   {
      if (millis() - upButtonsMillis > 5000)
      {
         forceConfig = true;
         startBlinking();

         if (webpage)
         {
            server.close();
         }

         if (upload != "WIFI")
         {
            setUPWiFi(); // Set up WiFi connection
         }

         if (!wifiManager.startConfigPortal("TeleAgriCulture Board", "enter123"))
         {
            Serial.println("failed to connect and hit timeout");
            delay(3000);
            ESP.restart(); // Failed to connect, restart ESP32
            delay(5000);
         }
      }
      upButtonsMillis = 0;
      stopBlinking();
   }

   if (no_upload)
   {
      sensorRead(); // Read sensor data
      no_upload = false;

      if ((!useBattery || !gotoSleep) && useDisplay)
      {
         renderPage(currentPage); // Render the current page on the display
      }
   }

   if (upload == "LORA")
   {
      os_runloop_once(); // Run the LoRaWAN OS run loop

      if (sendDataLoRa) // If ít`s time to send lora data
      {
         sensorRead(); // Read sensor data

         sendDataLoRa = false;
         gotoSleep = false;

         if (useSDCard)
         {
            if (!SD.begin(SPI_CON_CS, *spi))
            {
               Serial.println("Card Mount Failed");
               return;
            }
            SD_sendData(); // Send data to SD Card

            saveDataSDCard = false; // Reset the flag
         }

         lora_sendData(); // Send data via LoRa
      }

      if (loraJoinFailed && useDisplay)
      {
         analogWrite(TFT_BL, backlight_pwm); // turn TFT Backlight on
         tft->fillScreen(background_color);

         tft->setTextColor(0x9E6F);
         tft->setFont(&FreeSans9pt7b);
         tft->setTextSize(1);
         tft->setCursor(5, 50);
         tft->print("NO LORA GW near");
      }

      if (loraJoinFailed && useBattery)
      {
         Serial.println("NO Gateway found, restarting...");
         ESP.restart();
      }

      const bool timeCriticalJobs = os_queryTimeCriticalJobs(ms2osticksRound((TX_INTERVAL * 1000)));

      if (!timeCriticalJobs && !(LMIC.opmode & OP_TXRXPEND) && loraJoined) // if no transmission is active
      {
         if ((useBattery || !useDisplay) && gotoSleep)
         {
            Serial.print(F("Can go sleep "));

            saveLORA_State();
            saveLORA_State();
            saveLMICToRTC(TX_INTERVAL);
            delay(200);
            delay(200);

            esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_LOW);
            int time_interval = upload_interval * uS_TO_MIN_FACTOR;
            esp_sleep_enable_timer_wakeup(time_interval);

            // Stop all events
            blinker.detach();

            // Stop I2C
            Wire.end();

            tft->fillScreen(ST7735_BLACK);
            analogWrite(TFT_BL, 0); // turn TFT Backlight off
            tft->enableSleep(true);
            delay(100);

            // Stop SPI
            spi_bus_free(SPI2_HOST);

            // Reset Pins
            gpio_reset_pin((gpio_num_t)SW_3V3);
            gpio_reset_pin((gpio_num_t)SW_5V);
            gpio_reset_pin((gpio_num_t)TFT_BL);

            pinMode(SW_3V3, OUTPUT);
            digitalWrite(SW_3V3, LOW);
            pinMode(SW_5V, OUTPUT);
            digitalWrite(SW_5V, LOW);
            pinMode(TFT_BL, OUTPUT);
            digitalWrite(TFT_BL, LOW);

            gpio_hold_en((gpio_num_t)SW_3V3);
            gpio_hold_en((gpio_num_t)SW_5V);
            gpio_hold_en((gpio_num_t)TFT_BL);

            gpio_deep_sleep_hold_en();

            Serial.print("Setup ESP32 to sleep for ");
            Serial.print(upload_interval);
            Serial.println(" minutes");
            Serial.print("wakeup in: ");
            Serial.print(time_interval);
            Serial.println(" MicroSeconds");

            Serial.end();
            Serial.flush();

            Serial.print("Setup ESP32 to sleep for ");
            Serial.print(upload_interval);
            Serial.println(" minutes");
            Serial.print("wakeup in: ");
            Serial.print(time_interval);
            Serial.println(" MicroSeconds");

            Serial.end();
            Serial.flush();

            delay(100);

            esp_deep_sleep_start(); // Enter deep sleep mode
         }
      }
   }

   if (upload == "WIFI")
   {
      wifiManager.process(); // Process WiFi manager tasks

      if (sendDataWifi)
      {
         sensorRead(); // Read sensor data
         delay(1000);
         wifi_sendData(); // Send data via WiFi

         sendDataWifi = false; // Reset the flag

         setUploadTime(); // Set the next upload time

         if ((!useBattery || !gotoSleep) && useDisplay)
         {
            renderPage(currentPage); // Render the current page on the display
         }
      }
      if (!saveDataSDCard)
      {
         if ((useBattery && gotoSleep) || (!useDisplay))
         {
            // Stop WiFi before sleep
            wifiManager.disconnect();
            WiFi.mode(WIFI_OFF);

            esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_LOW);
            if (upload_interval == 60)
            {
               esp_sleep_enable_timer_wakeup((seconds_to_next_hour() - 15) * uS_TO_S_FACTOR);
               Serial.println("Setup ESP32 to sleep for " + String(seconds_to_next_hour() / 60) + " Minutes");
            }
            else
            {
               int time_interval = upload_interval * uS_TO_MIN_FACTOR;
               esp_sleep_enable_timer_wakeup(time_interval);
               Serial.print("Setup ESP32 to sleep for ");
               Serial.print(upload_interval);
               Serial.println(" minutes");
            }

            // Stop all events
            blinker.detach();

            // Stop I2C
            Wire.end();

            tft->fillScreen(ST7735_BLACK);
            analogWrite(TFT_BL, 0); // turn TFT Backlight off
            tft->enableSleep(true);

            // Stop SPI
            spi_bus_free(SPI2_HOST);

            // Reset Pins
            gpio_reset_pin((gpio_num_t)SW_3V3);
            gpio_reset_pin((gpio_num_t)SW_5V);
            gpio_reset_pin((gpio_num_t)TFT_BL);

            pinMode(SW_3V3, OUTPUT);
            digitalWrite(SW_3V3, LOW);
            pinMode(SW_5V, OUTPUT);
            digitalWrite(SW_5V, LOW);
            pinMode(TFT_BL, OUTPUT);
            digitalWrite(TFT_BL, LOW);

            gpio_hold_en((gpio_num_t)SW_3V3);
            gpio_hold_en((gpio_num_t)SW_5V);
            gpio_hold_en((gpio_num_t)TFT_BL);

            gpio_deep_sleep_hold_en();

            Serial.end();
            Serial.flush();

            delay(100);

            esp_deep_sleep_start(); // Enter deep sleep mode
         }
      }
   }

   if (saveDataSDCard)
   {
      tft->drawRGBBitmap(145, 2, sdcard_icon, 14, 19);
   }

   if (useSDCard)
   {
      digitalWrite(SW_3V3, HIGH);
      delay(200);

      if (!SD.begin(SPI_CON_CS, *spi))
      {
         Serial.println("Card Mount Failed");
         return;
      }

      sensorRead(); // Read sensor data
      delay(1000);
      SD_sendData(); // Send data to SD Card

      useSDCard = false; // Reset the flag

      setUploadTime(); // Set the next upload time

      if ((!useBattery || !gotoSleep) && useDisplay)
      {
         renderPage(currentPage); // Render the current page on the display
      }

      if ((useBattery && gotoSleep) || (!useDisplay))
      {
         // Stop WiFi before sleep
         wifiManager.disconnect();
         WiFi.mode(WIFI_OFF);

         esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_LOW);
         if (upload_interval == 60)
         {
            esp_sleep_enable_timer_wakeup((seconds_to_next_hour() - 15) * uS_TO_S_FACTOR);
            Serial.println("Setup ESP32 to sleep for " + String(seconds_to_next_hour() / 60) + " Minutes");
         }
         else
         {
            int time_interval = upload_interval * uS_TO_MIN_FACTOR;
            esp_sleep_enable_timer_wakeup(time_interval);
            Serial.print("Setup ESP32 to sleep for ");
            Serial.print(upload_interval);
            Serial.println(" minutes");
         }

         // Stop all events
         blinker.detach();

         // Stop I2C
         Wire.end();

         tft->fillScreen(ST7735_BLACK);
         analogWrite(TFT_BL, 0); // turn TFT Backlight off
         tft->enableSleep(true);

         // Stop SPI
         spi_bus_free(SPI2_HOST);

         // Reset Pins
         gpio_reset_pin((gpio_num_t)SW_3V3);
         gpio_reset_pin((gpio_num_t)SW_5V);
         gpio_reset_pin((gpio_num_t)TFT_BL);

         pinMode(SW_3V3, OUTPUT);
         digitalWrite(SW_3V3, LOW);
         pinMode(SW_5V, OUTPUT);
         digitalWrite(SW_5V, LOW);
         pinMode(TFT_BL, OUTPUT);
         digitalWrite(TFT_BL, LOW);

         gpio_hold_en((gpio_num_t)SW_3V3);
         gpio_hold_en((gpio_num_t)SW_5V);
         gpio_hold_en((gpio_num_t)TFT_BL);

         gpio_deep_sleep_hold_en();

         Serial.end();
         Serial.flush();

         Serial.end();
         Serial.flush();

         delay(100);

         esp_deep_sleep_start(); // Enter deep sleep mode
      }
   }

   num_pages = NUM_PAGES + total_measurement_pages; // Calculate the total number of pages

   // Refresh the display if the current page has changed
   if ((currentPage != lastPage) || displayRefresh)
   {
      lastPage = currentPage;
      if ((useDisplay && !useBattery) || (userWakeup && useDisplay))
      {
         analogWrite(TFT_BL, backlight_pwm); // Turn on the TFT Backlight
         renderPage(currentPage);            // Render the current page on the display
         displayRefresh = false;
      }
   }

   if (currentPage == 0)
   {
      if ((now() != prevDisplay) && upload == "WIFI")
      {
         // Update the display only if time has changed
         prevDisplay = now();
         if (useDisplay && (!useBattery || !gotoSleep))
         {
            digitalClockDisplay(5, 95, false); // Display the digital clock
         }
      }
   }

   if (webpage && !forceConfig)
   {
      server.handleClient(); // Handle incoming client requests
   }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
   Serial.printf("\nListing directory: %s\r\n", dirname);

   File root = fs.open(dirname);
   if (!root)
   {
      Serial.println("− failed to open directory");
      return;
   }
   if (!root.isDirectory())
   {
      Serial.println(" − not a directory");
      return;
   }

   File file = root.openNextFile();
   while (file)
   {
      if (file.isDirectory())
      {
         Serial.print("  DIR : ");
         Serial.println(file.name());
         if (levels)
         {
            listDir(fs, file.name(), levels - 1);
         }
      }
      else
      {
         Serial.print("  FILE: ");
         Serial.print(file.name());
         Serial.print("\tSIZE: ");
         Serial.println(file.size());
      }
      file = root.openNextFile();
   }
}

// Optional callback function, fired when NTP gets updated.
// Used to print the updated time or adjust an external RTC module.
void on_time_available(struct timeval *t)
{
   Serial.println("Received time adjustment from NTP");
   getLocalTime(&timeInfo, 1000);
   Serial.println(&timeInfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
}

void digitalClockDisplay(int x, int y, bool date)
{
   tft->setTextSize(1);
   tft->setCursor(x, y);
   tft->setTextColor(ST7735_WHITE);

   tft->fillRect(x - 5, y - 2, 60, 10, ST7735_BLACK);

   tft->print(timeInfo.tm_hour);
   printDigits(timeInfo.tm_min);
   printDigits(timeInfo.tm_sec);

   if (date)
   {
      tft->print("   ");
      tft->print(timeInfo.tm_mday);
      tft->print(" ");
      tft->print(timeInfo.tm_mon + 1);
      tft->print(" ");
      tft->print(timeInfo.tm_year - 100 + 2000);
      tft->println();
   }
}

void drawBattery(int x, int y)
{
   int bat = getBatteryChargeLevel();
   tft->setCursor(x, y);
   tft->setTextColor(0xCED7);
   tft->print("Bat: ");
   if (bat < 1)
   {
      tft->setTextColor(ST7735_WHITE);
      tft->print("NO");
      return;
   }
   if (bat > 1 && bat < 20)
   {
      tft->setTextColor(ST7735_RED);
   }
   if (bat >= 20 && bat < 40)
   {
      tft->setTextColor(ST7735_ORANGE);
   }
   if (bat >= 40 && bat < 60)
   {
      tft->setTextColor(ST7735_YELLOW);
   }
   if (bat >= 60 && bat < 80)
   {
      tft->setTextColor(0xAEB2);
   }
   if (bat >= 80)
   {
      tft->setTextColor(ST7735_GREEN);
   }
   tft->print(bat);
   tft->print(" %");
}

void printDigits(int digits)
{
   // utility for digital clock display: prints preceding colon and leading 0
   tft->print(":");
   if (digits < 10)
      tft->print('0');
   tft->print(digits);
}

void checkButton()
{
   // is auto timeout portal running
   if (portalRunning)
   {
      wifiManager.process();
   }

   // is configuration portal requested?
   if (digitalRead(LEFT_BUTTON_PIN) == LOW)
   {
      delay(50);
      if (digitalRead(LEFT_BUTTON_PIN) == LOW)
      {
         if (!portalRunning)
         {
            Serial.println("Button Pressed, Starting Portal");
            WiFi.mode(WIFI_STA);
            wifiManager.startWebPortal();
            portalRunning = true;
         }
         else
         {
            Serial.println("Button Pressed, Stopping Portal");
            wifiManager.stopWebPortal();
            portalRunning = false;
         }
      }
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
      freshBoot = false;

      if (useDisplay)
         gotoSleep = false;
   }

   if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
   {
      freshBoot = false;
      freshBoot = false;
   }
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

void load_Sensors()
{
   DynamicJsonDocument doc(JSON_BUFFER);

   DeserializationError error = deserializeJson(doc, proto_sensors);

   // Check for parsing errors
   if (error)
   {
      Serial.print(F("Failed to parse JSON: "));
      Serial.println(error.c_str());
      return;
   }

   for (size_t i = 0; i < SENSORS_NUM; i++)
   {
      JsonObject sensorObj = doc[i];

      allSensors[i].sensor_id = NO;
      allSensors[i].sensor_name = " ";

      allSensors[i].sensor_id = sensorObj["sensor-id"].as<SensorsImplemented>();
      allSensors[i].sensor_name = sensorObj["name"].as<String>();
      allSensors[i].con_typ = sensorObj["con_typ"].as<String>();
      allSensors[i].returnCount = sensorObj["returnCount"].as<int>();

      if (sensorObj.containsKey("measurements"))
      {
         JsonArray measurementsArr = sensorObj["measurements"].as<JsonArray>();
         size_t numMeasurements = measurementsArr.size();

         for (size_t j = 0; j < numMeasurements; j++)
         {
            JsonObject measurementObj = measurementsArr[j].as<JsonObject>();
            if (!measurementObj.containsKey("data_name") || !measurementObj.containsKey("value") || !measurementObj.containsKey("unit"))
            {
               // The measurement object is missing one or more required fields, handle the error here...
               continue;
            }

            allSensors[i].measurements[j].data_name = measurementObj["data_name"].as<String>();
            allSensors[i].measurements[j].value = measurementObj["value"].as<float>();
            allSensors[i].measurements[j].valueOrder = getValueOrderFromString(measurementObj["valueOrder"].as<String>());
            allSensors[i].measurements[j].unit = measurementObj["unit"].as<String>();
         }
      }
      if (sensorObj.containsKey("i2c_add"))
      {
         allSensors[i].i2c_add = sensorObj["i2c_add"].as<String>();
      }
      if (sensorObj.containsKey("possible_i2c_add"))
      {
         JsonArray i2c_alt_Arr = sensorObj["possible_i2c_add"].as<JsonArray>();
         size_t numI2Cadd = i2c_alt_Arr.size();

         for (size_t j = 0; j < numI2Cadd; j++)
         {
            JsonObject i2c_alt_Obj = i2c_alt_Arr[j].as<JsonObject>();
            if (!i2c_alt_Obj.containsKey("standard"))
            {
               // The measurement object is missing one or more required fields, handle the error here...
               continue;
            }

            if (i2c_alt_Obj.containsKey("standard"))
            {
               allSensors[i].possible_i2c_add[0] = i2c_alt_Obj["standard"].as<String>();
            }

            if (i2c_alt_Obj.containsKey("alt_1"))
            {
               allSensors[i].possible_i2c_add[1] = i2c_alt_Obj["alt_1"].as<String>();
            }
            if (i2c_alt_Obj.containsKey("alt_2"))
            {
               allSensors[i].possible_i2c_add[2] = i2c_alt_Obj["alt_2"].as<String>();
            }
         }
      }
   }

   DynamicJsonDocument deallocate(doc);
}

void load_Connectors()
{
   if (SPIFFS.begin())
   {
      // Serial.println("mounted file system");
      if (SPIFFS.exists("/connectors.json"))
      {
         // file exists, reading and loading
         // Serial.println("reading config file");
         File connectorsFile = SPIFFS.open("/connectors.json", "r");
         if (connectorsFile)
         {
            // Serial.println("opened config file");
            size_t size = connectorsFile.size();
            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);

            connectorsFile.readBytes(buf.get(), size);

            // DynamicJsonDocument json(CON_BUFFER);
            StaticJsonDocument<800> doc;
            auto deserializeError = deserializeJson(doc, buf.get());
            // serializeJson(doc, Serial);
            if (!deserializeError)
            {
               // Serial.println("\nparsed json");

               JsonArray jsonI2C_connectors = doc[0]["I2C_connectors"];
               for (int i = 0; i < I2C_NUM; i++)
               {
                  I2C_con_table[i] = jsonI2C_connectors[i];
               }

               JsonArray jsonOneWire_connectors = doc[1]["OneWire_connectors"];
               for (int i = 0; i < ONEWIRE_NUM; i++)
               {
                  OneWire_con_table[i] = jsonOneWire_connectors[i];
               }

               JsonArray jsonADC_connectors = doc[2]["ADC_connectors"];
               for (int i = 0; i < ADC_NUM; i++)
               {
                  ADC_con_table[i] = jsonADC_connectors[i];
               }

               SPI_con_table[0] = doc[3]["SPI_connectors"][0];

               I2C_5V_con_table[0] = doc[4]["I2C_5V_connectors"][0];

               EXTRA_con_table[0] = doc[5]["EXTRA_connectors"][0];
               EXTRA_con_table[1] = doc[5]["EXTRA_connectors"][1];
            }
            else
            {
               Serial.println("failed to load json config");
               forceConfig = true;
            }
            connectorsFile.close();
         }
      }
   }
   else
   {
      Serial.println("failed to mount FS");
      forceConfig = true;
   }
   // end read
}

void save_Connectors()
{
   // save the custom parameters to FS

   StaticJsonDocument<800> doc;

   JsonArray jsonI2C_connectors = doc[0].createNestedArray("I2C_connectors");
   for (int j = 0; j < I2C_NUM; j++)
   {
      jsonI2C_connectors.add(I2C_con_table[j]);
   }

   JsonArray jsonOneWire_connectors = doc[1].createNestedArray("OneWire_connectors");
   for (int i = 0; i < ONEWIRE_NUM; i++)
   {
      jsonOneWire_connectors.add(OneWire_con_table[i]);
   }
   JsonArray jsonADC_connectors = doc[2].createNestedArray("ADC_connectors");
   for (int i = 0; i < ADC_NUM; i++)
   {
      jsonADC_connectors.add(ADC_con_table[i]);
   }

   doc[3]["SPI_connectors"][0] = SPI_con_table[0];
   doc[4]["I2C_5V_connectors"][0] = I2C_5V_con_table[0];

   JsonArray jsonEXTRA_connectors = doc[5].createNestedArray("EXTRA_connectors");
   jsonEXTRA_connectors.add(EXTRA_con_table[0]);
   jsonEXTRA_connectors.add(EXTRA_con_table[1]);

   File connectorsFile = SPIFFS.open("/connectors.json", "w");
   if (!connectorsFile)
   {
      Serial.println("failed to open config file for writing");
   }
   serializeJson(doc, connectorsFile);

   connectorsFile.close();
   // end save
}

void renderPage(int page)
{
   // collect measurments for rendering MeasurmentPage
   show_measurements.clear();

   for (int i = 0; i < sensorVector.size(); i++)
   {
      for (int j = 0; j < sensorVector[i].returnCount; j++)
      {
         show_measurements.push_back(sensorVector[i].measurements[j]);
      }
   }
   total_measurement_pages = ceil(show_measurements.size() / NUM_PERPAGE) + 1;

   // Update the TFT display with content for the specified page
   if (page == 0)
   {
      mainPage();
   }
   if (page == 1)
   {
      I2C_ConnectorPage();
   }
   if (page == 2)
   {
      ADC_ConnectorPage();
   }
   if (page == 3)
   {
      OneWire_ConnectorPage();
   }
   if (page > NUM_PAGES - 1)
   {
      measurementsPage(page);
   }
}

int countMeasurements(const std::vector<Sensor> &sensors)
{
   int count = 0;
   for (const auto &sensor : sensors)
   {
      count += sensor.returnCount;
   }
   return count;
}

void mainPage()
{
   tft->fillScreen(ST7735_BLACK);
   tft->setTextColor(0x9E6F);
   tft->setFont(&FreeSans9pt7b);
   tft->setTextSize(1);
   tft->setCursor(5, 17);
   tft->print("TeleAgriCulture");
   tft->setCursor(5, 38);
   tft->print("Board V2.1");

   tft->setTextColor(ST7735_BLUE);
   tft->setFont();
   tft->setTextSize(1);
   tft->setCursor(5, 45);
   tft->print("Board ID: ");
   tft->print(boardID);
   tft->setCursor(5, 55);
   tft->print(version);

   if (upload == "WIFI")
   {
      digitalClockDisplay(5, 95, true);

      tft->setTextColor(0xCED7);
      tft->setCursor(5, 65);
      tft->print("WiFi: ");
      tft->setTextColor(ST7735_WHITE);
      tft->print(WiFi.SSID());
      tft->setCursor(5, 75);
      tft->setTextColor(0xCED7);
      tft->print("IP: ");
      tft->setTextColor(ST7735_WHITE);
      tft->print(WiFi.localIP());
      tft->setCursor(5, 85);
      tft->print("MAC: ");
      tft->setTextColor(ST7735_WHITE);
      tft->print(WiFi.macAddress());
      tft->setTextColor(0xCED7);
      tft->setCursor(5, 105);
      tft->print("last data UPLOAD: ");
      tft->setTextColor(ST7735_ORANGE);
      tft->print(upload);

      tft->setTextColor(ST7735_ORANGE);
      tft->setCursor(5, 115);
      tft->print(lastUpload);
   }
   else if (upload == "NO")
   {
      tft->setTextColor(0xCED7);
      tft->setCursor(5, 65);
      tft->print("WiFi: ");
      tft->setTextColor(ST7735_WHITE);
      tft->print(WiFi.softAPSSID());
      tft->setCursor(5, 75);
      tft->setTextColor(0xCED7);
      tft->print("IP: ");
      tft->setTextColor(ST7735_WHITE);
      tft->print(WiFi.softAPIP());
      tft->setCursor(5, 85);
      tft->print("MAC: ");
      tft->setTextColor(ST7735_WHITE);
      tft->print(WiFi.softAPmacAddress());
      tft->setCursor(5, 95);
      tft->print("no time sync");
      tft->setTextColor(ST7735_RED);
      tft->setCursor(5, 105);
      tft->print("up load:");
      tft->print("  ");
      tft->setTextColor(ST7735_ORANGE);
      tft->print(upload);
      tft->setTextColor(ST7735_ORANGE);
      tft->setCursor(5, 115);
      tft->print(lastUpload);
   }
   else if (upload == "LORA")
   {
      tft->setTextColor(0xCED7);
      tft->setCursor(5, 65);
      tft->print("WiFi: ");
      tft->setTextColor(ST7735_WHITE);
      tft->print(WiFi.softAPSSID());
      tft->setCursor(5, 75);
      tft->setTextColor(0xCED7);
      tft->print("IP: ");
      tft->setTextColor(ST7735_WHITE);
      tft->print(WiFi.softAPIP());
      tft->setCursor(5, 85);
      tft->print("MAC: ");
      tft->setTextColor(ST7735_WHITE);
      tft->print(WiFi.softAPmacAddress());
      tft->setTextColor(ST7735_ORANGE);
      tft->setCursor(5, 95);
      tft->print("TTN no time sync");
      tft->setTextColor(ST7735_RED);
      tft->setCursor(5, 105);
      tft->print(lora_fqz);
      tft->print("  ");
      tft->setTextColor(ST7735_ORANGE);
      tft->print(upload);

      if (loraJoinFailed)
      {
         tft->setTextColor(ST7735_RED);
         tft->setCursor(5, 115);
         tft->print("JOIN FAILED");
      }
      else
      {
         if (loraJoined)
         {
            tft->setTextColor(ST7735_GREEN);
            tft->setCursor(5, 115);
            tft->print("GW joined");
         }
      }
   }

   drawBattery(83, 115);
}

void printConnectors(ConnectorType typ)
{
   switch (typ)
   {
   case ConnectorType::I2C:
   {
      Serial.println("---- I2C ----");
      for (int i = 0; i < I2C_NUM; i++)
      {
         Serial.print("I2C_");
         Serial.print(i + 1);

         if (I2C_con_table[i] == NO)
         {
            Serial.println("  NO");
         }
         else
         {
            Serial.print("  ");
            Serial.print(allSensors[I2C_con_table[i]].sensor_name);
            Serial.print(" (");
            Serial.print(allSensors[I2C_con_table[i]].con_typ);
            Serial.print(") - ");
            for (int j = 0; j < allSensors[I2C_con_table[i]].returnCount; j++)
            {
               Serial.print(allSensors[I2C_con_table[i]].measurements[j].data_name);
               Serial.print(": ");
               Serial.print(allSensors[I2C_con_table[i]].measurements[j].value);
               Serial.print(allSensors[I2C_con_table[i]].measurements[j].unit);
               Serial.print(", ");
            }
            Serial.println();
         }
      }
   }
   break;

   case ConnectorType::ADC:
   {

      Serial.println("---- ADC ----");
      for (int i = 0; i < ADC_NUM; i++)
      {
         Serial.print("ADC_");
         Serial.print(i + 1);

         if (ADC_con_table[i] == NO)
         {
            Serial.println("  NO");
         }
         else
         {
            Serial.print("  ");
            Serial.print(allSensors[ADC_con_table[i]].sensor_name);
            Serial.print(" (");
            Serial.print(allSensors[ADC_con_table[i]].con_typ);
            Serial.print(") - ");
            for (int j = 0; j < allSensors[ADC_con_table[i]].returnCount; j++)
            {
               Serial.print(allSensors[ADC_con_table[i]].measurements[j].data_name);
               Serial.print(": ");
               Serial.print(allSensors[ADC_con_table[i]].measurements[j].value);
               Serial.print(allSensors[ADC_con_table[i]].measurements[j].unit);
               Serial.print(", ");
            }
            Serial.println();
         }
      }
   }
   break;

   case ConnectorType::ONE_WIRE:
   {
      Serial.println("---- 1-Wire ----");
      for (int i = 0; i < ONEWIRE_NUM; i++)
      {
         Serial.print("1-Wire_");
         Serial.print(i + 1);

         if (OneWire_con_table[i] == NO)
         {
            Serial.println("  NO");
         }
         else
         {
            Serial.print("  ");
            Serial.print(allSensors[OneWire_con_table[i]].sensor_name);
            Serial.print(" (");
            Serial.print(allSensors[OneWire_con_table[i]].con_typ);
            Serial.print(") - ");
            for (int j = 0; j < allSensors[OneWire_con_table[i]].returnCount; j++)
            {
               Serial.print(allSensors[OneWire_con_table[i]].measurements[j].data_name);
               Serial.print(": ");
               Serial.print(allSensors[OneWire_con_table[i]].measurements[j].value);
               Serial.print(allSensors[OneWire_con_table[i]].measurements[j].unit);
               Serial.print(", ");
            }
            Serial.println();
         }
      }
   }
   break;

   case ConnectorType::I2C_5V:
   {
      Serial.println("---- I2C_5V ----");
      Serial.print("I2C_5V");

      if (I2C_5V_con_table[0] == NO)
      {
         Serial.println("  NO");
      }
      else
      {
         Serial.print("  ");
         Serial.print(allSensors[I2C_5V_con_table[0]].sensor_name);
         Serial.print(" (");
         Serial.print(allSensors[I2C_5V_con_table[0]].con_typ);
         Serial.print(") - ");

         for (int j = 0; j < allSensors[I2C_5V_con_table[0]].returnCount; j++)
         {
            Serial.print(allSensors[I2C_5V_con_table[0]].measurements[j].data_name);
            Serial.print(": ");
            Serial.print(allSensors[I2C_5V_con_table[0]].measurements[j].value);
            Serial.print(allSensors[I2C_5V_con_table[0]].measurements[j].unit);
            Serial.print(", ");
         }
         Serial.println();
      }
   }
   break;

   case ConnectorType::SPI_CON:
   {
      Serial.println("---- SPI CON ----");
      Serial.print("SPI");

      if (SPI_con_table[0] == NO)
      {
         Serial.println("  NO");
      }
      else
      {
         Serial.print("  ");
         Serial.print(allSensors[SPI_con_table[0]].sensor_name);
         Serial.print(" (");
         Serial.print(allSensors[SPI_con_table[0]].con_typ);
         Serial.print(") - ");

         for (int j = 0; j < allSensors[SPI_con_table[0]].returnCount; j++)
         {
            Serial.print(allSensors[SPI_con_table[0]].measurements[j].data_name);
            Serial.print(": ");
            Serial.print(allSensors[SPI_con_table[0]].measurements[j].value);
            Serial.print(allSensors[SPI_con_table[0]].measurements[j].unit);
            Serial.print(", ");
         }
         Serial.println();
      }
   }
   break;

   case ConnectorType::EXTRA:
   {
      Serial.println("---- EXTRA CON ----");
      for (int i = 0; i < EXTRA_NUM; i++)
      {
         Serial.print("EXTRA_");
         Serial.print(i + 1);

         if (EXTRA_con_table[i] == NO)
         {
            Serial.println("  NO");
         }
         else
         {
            Serial.print("  ");
            Serial.print(allSensors[EXTRA_con_table[i]].sensor_name);
            Serial.print(" (");
            Serial.print(allSensors[EXTRA_con_table[i]].con_typ);
            Serial.print(") - ");
            for (int j = 0; j < allSensors[EXTRA_con_table[i]].returnCount; j++)
            {
               Serial.print(allSensors[EXTRA_con_table[i]].measurements[j].data_name);
               Serial.print(": ");
               Serial.print(allSensors[EXTRA_con_table[i]].measurements[j].value);
               Serial.print(allSensors[EXTRA_con_table[i]].measurements[j].unit);
               Serial.print(", ");
            }
            Serial.println();
         }
      }
   }
   break;

   default:
      break;
   }
}

void printProtoSensors(void)
{
   for (int z = 0; z < SENSORS_NUM; z++)
   {
      Serial.print(allSensors[z].sensor_id);
      Serial.print("  ");
      Serial.print(allSensors[z].sensor_name);
      Serial.print("  ");
      Serial.print(allSensors[z].con_typ);
      Serial.print(" returnCount:  ");
      Serial.println(allSensors[z].returnCount);

      for (size_t j = 0; j < allSensors[z].returnCount; j++)
      {
         Serial.print("         ");
         Serial.print(allSensors[z].measurements[j].data_name);
         Serial.print("  ");
         Serial.print(allSensors[z].measurements[j].value);
         Serial.println(allSensors[z].measurements[j].unit);
      }
   }
   Serial.println();
}

void printSensors()
{
   for (int i = 0; i < sensorVector.size(); ++i)
   {
      Serial.print("Sensor ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(sensorVector[i].sensor_name);
   }
}

void I2C_ConnectorPage()
{
   tft->fillScreen(ST7735_BLACK);
   tft->setTextSize(2);
   tft->setCursor(10, 10);
   tft->setTextColor(ST7735_WHITE);
   tft->print("I2C Con");

   tft->setTextSize(1);
   int cursor_y = 35;

   for (int i = 0; i < I2C_NUM; i++)
   {
      tft->setCursor(5, cursor_y);
      tft->setTextColor(ST7735_YELLOW);

      tft->print("I2C_");
      tft->print(i + 1);
      tft->setCursor(80, cursor_y);
      if (I2C_con_table[i] == -1)
      {
         tft->setTextColor(ST7735_RED);
         tft->print("NO");
      }
      else
      {
         tft->setTextColor(ST7735_GREEN);
         tft->print(allSensors[I2C_con_table[i]].sensor_name);
      }
      cursor_y += 10;
   }
   cursor_y += 10;
   tft->setTextSize(2);
   tft->setCursor(10, cursor_y);
   tft->setTextColor(ST7735_WHITE);
   tft->print("I2C_5V Con");

   cursor_y += 25;

   tft->setTextSize(1);
   tft->setCursor(5, cursor_y);
   tft->setTextColor(ST7735_YELLOW);

   tft->print("I2C_5V");

   tft->setCursor(80, cursor_y);
   if (I2C_5V_con_table[0] == -1)
   {
      tft->setTextColor(ST7735_RED);
      tft->print("NO");
   }
   else
   {
      tft->setTextColor(ST7735_GREEN);
      tft->print(allSensors[I2C_5V_con_table[0]].sensor_name);
   }
}

void ADC_ConnectorPage()
{
   tft->fillScreen(ST7735_BLACK);
   tft->fillScreen(ST7735_BLACK);
   tft->setTextSize(2);
   tft->setCursor(10, 10);
   tft->setTextColor(ST7735_WHITE);
   tft->print("ADC Con");

   tft->setTextSize(1);
   int cursor_y = 45;

   for (int i = 0; i < ADC_NUM; i++)
   {
      tft->setCursor(5, cursor_y);
      tft->setTextColor(ST7735_YELLOW);

      tft->print("ADC_");
      tft->print(i + 1);
      tft->setCursor(80, cursor_y);
      if (ADC_con_table[i] == -1)
      {
         tft->setTextColor(ST7735_RED);
         tft->print("NO");
      }
      else
      {
         tft->setTextColor(ST7735_GREEN);
         tft->print(allSensors[ADC_con_table[i]].sensor_name);
      }
      cursor_y += 10;
   }
   cursor_y += 10;

   tft->setCursor(5, cursor_y);
   tft->setTextColor(ST7735_YELLOW);
   tft->print("BootCount: ");
   tft->setTextColor(ST7735_GREEN);
   tft->print(bootCount);
}

void OneWire_ConnectorPage()
{
   tft->fillScreen(ST7735_BLACK);

   tft->fillScreen(ST7735_BLACK);
   tft->setTextSize(2);
   tft->setCursor(10, 10);
   tft->setTextColor(ST7735_WHITE);
   tft->print("OneWire Con");

   tft->setTextSize(1);
   int cursor_y = 45;

   for (int i = 0; i < ONEWIRE_NUM; i++)
   {
      tft->setCursor(5, cursor_y);
      tft->setTextColor(ST7735_YELLOW);

      tft->print("1-Wire_");
      tft->print(i + 1);
      tft->setCursor(80, cursor_y);
      if (OneWire_con_table[i] == -1)
      {
         tft->setTextColor(ST7735_RED);
         tft->print("NO");
      }
      else
      {
         tft->setTextColor(ST7735_GREEN);
         tft->print(allSensors[OneWire_con_table[i]].sensor_name);
      }
      cursor_y += 10;
   }
}

void measurementsPage(int page)
{
   tft->fillScreen(ST7735_BLACK);
   tft->setTextSize(2);
   tft->setCursor(10, 10);
   tft->setTextColor(ST7735_WHITE);
   tft->print("Sensor Data");

   tft->setTextSize(1);

   int cursor_y = 35;

   int startIndex = (page - NUM_PAGES) * NUM_PERPAGE;
   int endIndex = startIndex + NUM_PERPAGE;
   if (endIndex > show_measurements.size())
   {
      endIndex = show_measurements.size();
   }

   for (int i = startIndex; i < endIndex; i++)
   {
      tft->setCursor(5, cursor_y);
      tft->setTextColor(ST7735_BLUE);
      tft->print(show_measurements[i].data_name);
      tft->print(": ");

      tft->setCursor(65, cursor_y);
      tft->setTextColor(ST7735_YELLOW);

      if (!isnan(show_measurements[i].value))
      {
         tft->print(show_measurements[i].value);
      }
      else
      {
         tft->print("NAN");
      }

      tft->print(" ");

      if (!(show_measurements[i].unit == "°C"))
      {
         tft->print(show_measurements[i].unit);
      }
      else
      {
         tft->drawChar(tft->getCursorX(), tft->getCursorY(), 0xF8, ST7735_YELLOW, ST7735_BLACK, 1);
         tft->setCursor(tft->getCursorX() + 7, tft->getCursorY());
         tft->print("C");
      }
      cursor_y += 10;
   }
}

void checkLoadedStuff(void)
{
   // Serial.println();
   // Serial.println("---------------Prototype Sensors loaded -----------");
   // printProtoSensors();
   // Serial.println();
   // Serial.println("---------------Prototype Sensors loaded -----------");
   // printProtoSensors();
   Serial.println();
   Serial.println("---------------Connector table loaded -----------");

   printConnectors(ConnectorType::I2C);
   printConnectors(ConnectorType::ADC);
   printConnectors(ConnectorType::ONE_WIRE);
   printConnectors(ConnectorType::I2C_5V);
   printConnectors(ConnectorType::SPI_CON);
   printConnectors(ConnectorType::EXTRA);

   Serial.println();
   Serial.println("---------------Config loaded -----------");

   Serial.println("Configuration Values:");
   Serial.printf("BoardID: %d\n", boardID);
   Serial.printf("useBattery: %d\n", useBattery);
   Serial.printf("useDisplay: %d\n", useDisplay);
   Serial.printf("useEnterpriseWPA: %d\n", useEnterpriseWPA);
   Serial.printf("useCustomNTP: %d\n", useCustomNTP);
   Serial.printf("useNTP: %d\n", useNTP);
   Serial.printf("API_KEY: %s\n", API_KEY.c_str());
   Serial.printf("upload: %s\n", upload.c_str());
   Serial.printf("upload_interval: %d\n", upload_interval);
   Serial.printf("anonym: %s\n", anonym.c_str());
   Serial.printf("user_CA: %s\n", user_CA.c_str());
   Serial.printf("customNTPaddress: %s\n", customNTPaddress.c_str());
   Serial.printf("timeZone: %s\n", timeZone.c_str());
   Serial.printf("OTAA_DEVEUI: %s\n", OTAA_DEVEUI.c_str());
   Serial.printf("OTAA_APPEUI: %s\n", OTAA_APPEUI.c_str());
   Serial.printf("OTAA_APPKEY: %s\n", OTAA_APPKEY.c_str());
   Serial.printf("lora_ADR: %d\n", lora_ADR);

   Serial.println("\n---------------DEBUGG END -----------");

   Serial.println();
}

void save_Config(void)
{
   StaticJsonDocument<3000> doc;

   doc["BoardID"] = boardID;
   doc["useBattery"] = useBattery;
   doc["useDisplay"] = useDisplay;
   doc["saveDataSDCard"] = saveDataSDCard;
   doc["useEnterpriseWPA"] = useEnterpriseWPA;
   doc["useCustomNTP"] = useCustomNTP;
   doc["useNTP"] = useNTP;
   doc["API_KEY"] = API_KEY;
   doc["upload"] = upload;
   doc["upInterval"] = upload_interval;
   doc["anonym"] = anonym;
   doc["user_CA"] = user_CA;
   doc["customNTPadress"] = customNTPaddress;
   doc["timeZone"] = timeZone;
   doc["OTAA_DEVEUI"] = OTAA_DEVEUI;
   doc["OTAA_APPEUI"] = OTAA_APPEUI;
   doc["OTAA_APPKEY"] = OTAA_APPKEY;
   doc["lora_ADR"] = lora_ADR;
   doc["apn"] = apn;
   doc["gprs_user"] = gprs_user;
   doc["gprs_pass"] = gprs_pass;

   File configFile = SPIFFS.open("/board_config.json", "w");
   if (!configFile)
   {
      Serial.println("failed to open config file for writing");
   }

   serializeJson(doc, configFile);

   configFile.close();
   // end save
}

void load_Config(void)
{
   if (SPIFFS.begin())
   {
      if (SPIFFS.exists("/board_config.json"))
      {
         // file exists, reading and loading
         // Serial.println("reading config file");
         File configFile = SPIFFS.open("/board_config.json", "r");
         if (configFile)
         {
            // Serial.println("opened config file");
            size_t size = configFile.size();
            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);

            configFile.readBytes(buf.get(), size);

            StaticJsonDocument<2500> doc;
            auto deserializeError = deserializeJson(doc, buf.get());

            if (!deserializeError)
            {
               boardID = doc["BoardID"];
               useBattery = doc["useBattery"];
               useDisplay = doc["useDisplay"];
               saveDataSDCard = doc["saveDataSDCard"];
               useEnterpriseWPA = doc["useEnzerpriseWPA"];
               useCustomNTP = doc["useCustomNTP"];
               useNTP = doc["useNTP"];
               API_KEY = doc["API_KEY"].as<String>();
               upload = doc["upload"].as<String>();
               upload_interval = doc["upInterval"];
               anonym = doc["anonym"].as<String>();
               user_CA = doc["user_CA"].as<String>();
               customNTPaddress = doc["customNTPadress"].as<String>();
               timeZone = doc["timeZone"].as<String>();
               OTAA_DEVEUI = doc["OTAA_DEVEUI"].as<String>();
               OTAA_APPEUI = doc["OTAA_APPEUI"].as<String>();
               OTAA_APPKEY = doc["OTAA_APPKEY"].as<String>();
               lora_ADR = doc["lora_ADR"];
               apn = doc["apn"].as<String>();
               gprs_user = doc["gprs_user"].as<String>();
               gprs_pass = doc["gprs_pass"].as<String>();
            }
         }
         else
         {
            Serial.println("failed to load json config");
            forceConfig = true;
         }
         configFile.close();
      }
   }
   else
   {
      Serial.println("failed to mount FS");
      forceConfig = true;
   }
}

void printMeassurments()
{
   // Loop through all the elements in the vector
   for (int i = 0; i < sensorVector.size(); ++i)
   {
      for (int j = 0; j < sensorVector[i].returnCount; j++)
      {
         Serial.print(sensorVector[i].measurements[j].data_name);
         Serial.print(":  ");
         Serial.println(sensorVector[i].measurements[j].value);
      }
   }
   Serial.print("\nVector elements: ");
   Serial.print(sensorVector.size() + 1);
   Serial.print("\nSize of Vector: ");
   Serial.println(sizeof(sensorVector));
}

void wifi_sendData(void)
{
   DynamicJsonDocument docMeasures(2000);

   for (int i = 0; i < sensorVector.size(); ++i)
   {
      for (int j = 0; j < sensorVector[i].returnCount; j++)
      {
         if (!isnan(sensorVector[i].measurements[j].value))
         {
            docMeasures[sensorVector[i].measurements[j].data_name] = static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0); // sensorVector[i].measurements[j].value;
         }
      }
   }

   String output;
   serializeJson(docMeasures, output);

   if (sizeof(output) > 2)
   {
      Serial.println("\nsend Data via WIFI:");
      serializeJson(docMeasures, Serial);

      // Check WiFi connection status
      if (WiFi.status() == WL_CONNECTED)
      {
         // WiFiClient client;
         //  WiFiClientSecure client;
         //  client.setCACert(kits_ca);

         WiFiClientSecure *client = new WiFiClientSecure;

         delay(100);

         if (client)
         {

            client->setCACertBundle(rootca_bundle_crt_start);

            HTTPClient https;

            // https://gitlab.com/teleagriculture/community/-/blob/main/API.md

            // python example
            // https://gitlab.com/teleagriculture/community/-/blob/main/RPI/tacserial.py

            // should show up there:
            // https://kits.teleagriculture.org/kits/10xx

            String serverName = "https://kits.teleagriculture.org/api/kits/" + String(boardID) + "/measurements";
            String api_Bearer = "Bearer " + API_KEY;

            https.begin(*client, serverName);

            delay(100);

            https.addHeader("Content-Type", "application/json");
            https.addHeader("Authorization", api_Bearer);

            int httpResponseCode = https.POST(output);
            Serial.println();
            Serial.println(serverName);
            Serial.println(api_Bearer);

            Serial.print("\nHTTP Response code: ");
            Serial.println(httpResponseCode);
            Serial.println();

            // Free resources
            https.end();
            delete client;
         }
         else
         {
            Serial.printf("\n[HTTPS] Unable to connect\n");
         }
      }
      else
      {
         Serial.println("WiFi Disconnected");
      }

      DynamicJsonDocument deallocate(docMeasures);
   }

   Serial.println();
}

void lora_sendData(void)
{
   Serial.print("\nDEVEUI[8]={ ");
   for (int i = 0; i < 8; i++)
   {
      Serial.print("0x");
      Serial.print(dev_eui[i], HEX);
      if (i < 7)
      {
         Serial.print(", ");
      }
   }
   Serial.println(" };");

   Serial.print("APPEUI[8]={ ");
   for (int i = 0; i < 8; i++)
   {
      Serial.print("0x");
      Serial.print(app_eui[i], HEX);
      if (i < 7)
      {
         Serial.print(", ");
      }
   }
   Serial.println(" };");

   Serial.print("APPKEY[16]={ ");
   for (int i = 0; i < 16; i++)
   {
      Serial.print("0x");
      Serial.print(app_key[i], HEX);
      if (i < 15)
      {
         Serial.print(", ");
      }
   }
   Serial.println(" };\n");

   // https://github.com/thesolarnomad/lora-serialization
   // JS decoder example online

   LoraMessage message;

   for (int i = 0; i < sensorVector.size(); ++i)
   {
      for (int j = 0; j < sensorVector[i].returnCount; j++)
      {
         int k = static_cast<uint8_t>((sensorVector[i].measurements[j].valueOrder));

         if (!isnan(sensorVector[i].measurements[j].value))
         {
            message.addUint8(k);

            switch (k) // send Measurment values as different packeges
            {
            case VOLT:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case TEMP:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case HUMIDITY:
               message.addHumidity(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case PRESSURE:
               message.addRawFloat(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case DISTANCE:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case TDSv:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case MOIS:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case LUX:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case AMBIENT:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case H2v:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case COv:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case CO2v:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case NO2v:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case NH3v:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case C4H10v:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case C3H8v:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case CH4v:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case C2H5OHv:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case ALTITUDE:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case MV:
               message.addRawFloat(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case MGL:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case MSCM:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case PH:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case DBA:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;

            case DEPTH:
               message.addRawFloat(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;

            case UV_I:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            }
         }
      }
   }

   if (message.getLength() > 2)
   {
      Serial.println("\nSend Lora Data: ");
      do_send(message);
   }
}

void SD_sendData()
{
   DynamicJsonDocument docMeasures(2000);

   for (int i = 0; i < sensorVector.size(); ++i)
   {
      for (int j = 0; j < sensorVector[i].returnCount; j++)
      {
         if (!isnan(sensorVector[i].measurements[j].value))
         {
            docMeasures[sensorVector[i].measurements[j].data_name] = static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0);
         }
      }
   }

   String output;
   serializeJson(docMeasures, output);

   // Ensure the output is not empty
   if (output.length() > 2) // The minimum valid JSON object would be "{}" which is 2 characters
   {
      String path = "/Teleagriculture_SensorKit_id_" + String(boardID); // File path
      File dataFile;

      // Check if the file exists
      if (SD.exists(path.c_str()))
      {
         dataFile = SD.open(path.c_str(), FILE_APPEND); // Open in append mode
      }
      else
      {
         dataFile = SD.open(path.c_str(), FILE_WRITE); // Create a new file
      }

      if (dataFile)
      {
         dataFile.println(output); // Append the JSON data as a new line
         dataFile.close();
         Serial.println("Data written to SD card: " + output);
      }
      else
      {
         Serial.println("Failed to open file for writing");
      }
   }
   else
   {
      Serial.println("No valid data to write");
   }
}

void get_time_in_timezone(const char *timezone)
{
   struct timeval tv;
   gettimeofday(&tv, NULL);
   time_t now = tv.tv_sec;
   struct tm *local = localtime(&now);
   setenv("TZ", timezone, 1);
   tzset();
   struct tm *gmt = gmtime(&now);
   int diff_hours = (local->tm_hour - gmt->tm_hour);
   int diff_minutes = (local->tm_min - gmt->tm_min);
   Serial.printf("Timezone: %s\n", timezone);
   Serial.printf("Current time: %02d:%02d\n", local->tm_hour, local->tm_min);
   Serial.printf("GMT time: %02d:%02d\n", gmt->tm_hour, gmt->tm_min);
   Serial.printf("Difference from GMT: %d:%02d\n", diff_hours, diff_minutes);
}

int seconds_to_next_hour()
{
   struct tm timeInfo;
   getLocalTime(&timeInfo);

   // time_t now = time(NULL);
   // struct tm *tm_now = localtime(&now);
   int seconds = (60 - timeInfo.tm_sec) + (59 - timeInfo.tm_min) * 60;
   return seconds;
}

void startBlinking()
{
   // Store initial state of LED
   initialState = digitalRead(LED);

   // Start Timer
   blinker.attach(1.0, toggleLED);
}

void stopBlinking()
{
   // Stop Timer
   blinker.detach();

   // Restore initial state of LED
   digitalWrite(LED, initialState);
}

void toggleLED()
{
   digitalWrite(LED, ledState);
   ledState = !ledState;
}

void configModeCallback(WiFiManager *myWiFiManager)
{
   // Serial.println("Entered Conf Mode");
   // Serial.print("Config SSID: ");
   // Serial.println(myWiFiManager->getConfigPortalSSID());
   // Serial.print("Config IP Address: ");
   // Serial.println(WiFi.softAPIP());
   if (useDisplay)
   {
      tft->fillScreen(ST7735_BLACK);
      tft->setTextColor(ST7735_WHITE);
      tft->setFont(&FreeSans9pt7b);
      tft->setTextSize(1);
      tft->setCursor(5, 17);
      tft->print("TeleAgriCulture");
      tft->setCursor(5, 38);
      tft->print("Board V2.1");

      tft->setTextColor(ST7735_RED);
      tft->setFont();
      tft->setTextSize(2);
      tft->setCursor(5, 50);
      tft->print("Config MODE");
      tft->setTextColor(ST7735_WHITE);
      tft->setTextSize(1);
      tft->setCursor(5, 73);
      tft->print("SSID:");
      tft->setCursor(5, 85);
      tft->print(myWiFiManager->getConfigPortalSSID());
      tft->setCursor(5, 95);
      tft->print("IP: ");
      tft->print(WiFi.softAPIP());
      tft->setCursor(5, 108);
      tft->print("MAC: ");
      tft->print(WiFi.macAddress());
      tft->setCursor(5, 117);
      tft->setTextColor(ST7735_BLUE);
      tft->print(version);
   }
}

String get_header()
{
   WiFiClientSecure *client = new WiFiClientSecure;
   String header = "";

   if (client)
   {
      client->setCACertBundle(rootca_bundle_crt_start);

      // makes a HTTP request
      unsigned long timeNow;
      bool HeaderComplete = false;
      bool currentLineIsBlank = true;
      client->stop(); // Close any connection before send a new request.  This will free the socket on the WiFi
      client->connect(GET_Time_SERVER, SSL_PORT, true);
      delay(100);
      if (client->connected())
      { // if there's a successful connection:
         client->println("GET " + GET_Time_Address + " HTTP/1.1");
         client->print("HOST: ");
         client->println(GET_Time_SERVER);
         client->println();
         timeNow = millis();
         while (millis() - timeNow < TIMEOUT)
         {
            while (client->available())
            {
               char c = client->read();
               if (!HeaderComplete)
               {
                  if (currentLineIsBlank && c == '\n')
                  {
                     HeaderComplete = true;
                  }
                  else
                  {
                     header = header + c;
                  }
               }
               if (c == '\n')
               {
                  currentLineIsBlank = true;
               }
               else if (c != '\r')
               {
                  currentLineIsBlank = false;
               }
            }
            if (HeaderComplete)
               break;
         }
      }
      else
      {
         Serial.println("Connection failed");
      }
      if (client->connected())
      {
         client->stop();
      }
      return header;
   }
   else
   {
      Serial.printf("\n[HTTPS] Unable to connect to GET Time (header)\n");
   }
   return "NO HEADER";
}

time_t convertDateTime(String dateTimeStr)
{
   struct tm tm;
   memset(&tm, 0, sizeof(struct tm));
   strptime(dateTimeStr.c_str(), "%a, %d %b %Y %H:%M:%S %Z", &tm);
   time_t t = mktime(&tm);
   return t;
}

String getDateTime(String header)
{
   int index = header.indexOf("Date:");
   if (index == -1)
   {
      return "";
   }
   String dateTimeStr = header.substring(index + 6, index + 31);
   time_t t = convertDateTime(dateTimeStr);
   char buf[20];
   strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M", localtime(&t));
   return String(buf);
}

void setEsp32Time(const char *timeStr)
{
   if (useNTP)
      return;

   struct tm t;
   struct tm t2;
   timeInfo;
   strptime(timeStr, "%Y-%m-%dT%H:%M", &t);
   t2.tm_sec = 0;
   t2.tm_min = t.tm_min;
   t2.tm_mday = t.tm_mday;
   t2.tm_mon = t.tm_mon;
   t2.tm_hour = t.tm_hour;
   t2.tm_year = t.tm_year;

   delay(100);

   const time_t sec = mktime(&t2); // make time_t

   setenv("TZ", "GMT0", 1); // set Timezone to GMT
   tzset();

   struct timeval set_Time = {.tv_sec = sec};
   settimeofday(&set_Time, NULL);

   // setTime(sec);
   // setTime(now.tm_hour, now.tm_min, now.tm_sec, now.tm_mday, now.tm_mon, now.tm_year);
   setenv("TZ", timeZone.c_str(), 1);
   tzset();

   getLocalTime(&timeInfo);
   Serial.printf("\nSetting time based on GET: %s", asctime(&timeInfo));

   currentDay = timeInfo.tm_mday;
   lastDay = currentDay;
}

void setUploadTime()
{
   getLocalTime(&timeInfo);

   lastUpload = "";

   if (timeInfo.tm_hour < 10)
      lastUpload += '0';
   lastUpload += String(timeInfo.tm_hour) + ':';

   if (timeInfo.tm_min < 10)
      lastUpload += '0';
   lastUpload += String(timeInfo.tm_min) + ':';

   if (timeInfo.tm_sec < 10)
      lastUpload += '0';

   lastUpload += String(timeInfo.tm_sec);
}

void printHex2(unsigned v)
{
   v &= 0xff;
   if (v < 16)
      Serial.print('0');
   Serial.print(v, HEX);
}

void setUPWiFi()
{
   if (customNTPaddress != NULL)
   {
      if (WiFiManagerNS::NTP::NTP_Servers.size() == NUM_PREDIFINED_NTP)
      {
         const std::string constStr = customNTPaddress.c_str();
         WiFiManagerNS::NTP::NTP_Server newServer = {"Custom NTP Server", constStr};
         WiFiManagerNS::NTP::NTP_Servers.push_back(newServer);
      }
   }

   if (useNTP)
   {
      // optionally attach external RTC update callback
      WiFiManagerNS::NTP::onTimeAvailable(&on_time_available);
   }

   // attach board-setup page to the WiFi Manager
   WiFiManagerNS::init(&wifiManager);

   wifiManager.setDebugOutput(true);
   wifiManager.setHostname(hostname.c_str());
   wifiManager.setTitle("Board Config");
   wifiManager.setCustomHeadElement(custom_Title_Html.c_str());

   // set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
   wifiManager.setAPCallback(configModeCallback);
   wifiManager.setConnectRetries(10);
   wifiManager.setCleanConnect(true);
   wifiManager.setConfigPortalBlocking(true);

   // /!\ make sure "custom" is listed there as it's required to pull the "Board Setup" button
   std::vector<const char *> menu = {"custom", "wifi", "info", "update", "restart", "exit"};
   wifiManager.setMenu(menu);

   // wifiManager.setShowInfoUpdate(false);
   wifiManager.setDarkMode(true);
   wifiManager.setSaveConfigCallback([]()
                                     { configSaved = true; }); // restart on credentials save, ESP32 doesn't like to switch between AP/STA
}

void convertTo_LSB_EUI(String input, uint8_t *output)
{
   // Check if input matches pattern
   if (input.length() != 16)
   {
      Serial.println("Input must be 16 characters long.");
      return;
   }
   for (int i = 0; i < 16; i++)
   {
      if (!isxdigit(input.charAt(i)))
      {
         Serial.println("Input must be in hexadecimal format.");
         return;
      }
   }

   // Convert string to byte array
   for (int i = 0; i < 8; i++)
   {
      String byteStr = input.substring(i * 2, i * 2 + 2);
      output[7 - i] = strtol(byteStr.c_str(), NULL, 16);
   }
}

void convertTo_MSB_APPKEY(String input, uint8_t *output)
{
   // Check if input matches pattern
   if (input.length() != 32)
   {
      Serial.println("Input must be 16 characters long.");
      return;
   }
   for (int i = 0; i < 32; i++)
   {
      if (!isxdigit(input.charAt(i)))
      {
         Serial.println("Input must be in hexadecimal format.");
         return;
      }
   }

   // Convert string to byte array
   for (int i = 0; i < 16; i++)
   {
      String byteStr = input.substring(i * 2, i * 2 + 2);
      output[i] = strtol(byteStr.c_str(), NULL, 16);
   }
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

int getBatteryChargeLevel()
{
   int chargeLevel = getChargeLevel(getBatteryVoltage());
   return chargeLevel;
}

void deepsleepPage()
{
   analogWrite(TFT_BL, 10); // turn TFT Backlight on
   tft->fillScreen(ST7735_BLACK);
   tft->setTextSize(1);
   tft->setTextColor(ST7735_WHITE);

   int cursor_y = 10;

   for (int i = 0; i < show_measurements.size(); i++)
   {
      tft->setCursor(5, cursor_y);
      tft->setTextColor(ST7735_BLUE);
      tft->print(show_measurements[i].data_name);
      tft->print(": ");

      tft->setCursor(60, cursor_y);
      tft->setTextColor(ST7735_YELLOW);

      if (!isnan(show_measurements[i].value))
      {
         tft->print(show_measurements[i].value);
      }
      else
      {
         tft->print("NAN");
      }

      tft->print(" ");

      if (!(show_measurements[i].unit == "°C"))
      {
         tft->print(show_measurements[i].unit);
      }
      else
      {
         tft->drawChar(tft->getCursorX(), tft->getCursorY(), 0xF8, ST7735_YELLOW, ST7735_BLACK, 1);
         tft->setCursor(tft->getCursorX() + 7, tft->getCursorY());
         tft->print("C");
      }
      cursor_y += 10;
   }
   cursor_y += 10;
   tft->setCursor(5, cursor_y);
   tft->setTextColor(ST7735_ORANGE);

   tft->print("Deep Sleep for: ");
   tft->print(seconds_to_next_hour());
   // tft->print(TIME_TO_SLEEP);
}

ValueOrder getValueOrderFromString(String str)
{
   if (str == "VOLT")
   {
      return VOLT;
   }
   else if (str == "TEMP")
   {
      return TEMP;
   }
   else if (str == "HUMIDITY")
   {
      return HUMIDITY;
   }
   else if (str == "PRESSURE")
   {
      return PRESSURE;
   }
   else if (str == "DISTANCE")
   {
      return DISTANCE;
   }
   else if (str == "MOIS")
   {
      return MOIS;
   }
   else if (str == "LUX")
   {
      return LUX;
   }
   else if (str == "AMBIENT")
   {
      return AMBIENT;
   }
   else if (str == "H2v")
   {
      return H2v;
   }
   else if (str == "COv")
   {
      return COv;
   }
   else if (str == "COv")
   {
      return COv;
   }
   else if (str == "CO2v")
   {
      return CO2v;
   }
   else if (str == "NO2v")
   {
      return NO2v;
   }
   else if (str == "NH3v")
   {
      return NH3v;
   }
   else if (str == "C4H10v")
   {
      return C4H10v;
   }
   else if (str == "C3H8v")
   {
      return C3H8v;
   }
   else if (str == "CH4v")
   {
      return CH4v;
   }
   else if (str == "C2H5OHv")
   {
      return C2H5OHv;
   }
   else if (str == "RGB")
   {
      return RGB;
   }
   else if (str == "ANGLE")
   {
      return ANGLE;
   }
   else if (str == "ALTITUDE")
   {
      return ALTITUDE;
   }
   else if (str == "MV")
   {
      return MV;
   }
   else if (str == "MGL")
   {
      return MGL;
   }
   else if (str == "MSCM")
   {
      return MSCM;
   }
   else if (str == "PH")
   {
      return PH;
   }
   else if (str == "DBA")
   {
      return DBA;
   }
   else if (str == "DEPTH")
   {
      return DEPTH;
   }
   else if (str == "UV_I")
   {
      return UV_I;
   }
   else
   {
      return NOT;
   }
}

void onEvent(ev_t ev)
{
   Serial.print(os_getTime());
   Serial.print(": ");
   switch (ev)
   {
   case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
   case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
   case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
   case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
   case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
   case EV_JOINED:
   {
      Serial.println(F("EV_JOINED"));

      u4_t netid = 0;
      devaddr_t devaddr = 0;
      u1_t nwkKey[16];
      u1_t artKey[16];
      LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
      Serial.print("netid: ");
      Serial.println(netid, DEC);
      Serial.print("devaddr: ");
      Serial.println(devaddr, HEX);
      Serial.print("artKey: ");
      for (size_t i = 0; i < sizeof(artKey); ++i)
      {
         Serial.print(artKey[i], HEX);
      }
      Serial.println("");
      Serial.print("nwkKey: ");
      for (size_t i = 0; i < sizeof(nwkKey); ++i)
      {
         Serial.print(nwkKey[i], HEX);
      }
      Serial.println("\n");

      loraJoined = true;
      loraJoinFailed = false;
      loraJoinFailed = false;
      displayRefresh = true;

      saveLORA_State();

      // Disable link check validation (automatically enabled
      // during join, but because slow data rates change max TX
      // size, we don't use it in this example.
      if (lora_ADR)
      {
         Serial.println("\nuse ADR for LORA (for mobile Nodes)");
         LMIC_setLinkCheckMode(0);
      }
      else
      {
         Serial.println("\nuse ADR for LORA (for mobile Nodes)");
      }
      if (lora_ADR)
      {
         Serial.println("\nuse ADR for LORA (for mobile Nodes)");
         LMIC_setLinkCheckMode(0);
      }
      else
      {
         Serial.println("\nuse ADR for LORA (for mobile Nodes)");
      }
   }
   break;
   /*
       || This event is defined but not used in the code. No
       || point in wasting codespace on it.
       ||
       || case EV_RFU1:
       ||     Serial.println(F("EV_RFU1"));
       ||     break;
       */
   case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      loraJoinFailed = true;
      // upload="WIFI"; // Fallback WIFI
      // save_Config();
      ESP.restart();
      break;
   case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
   case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
      {
         Serial.println(F("Received ack"));
      }
      if (LMIC.dataLen)
      {
         Serial.print(F("Received "));
         Serial.print(LMIC.dataLen);
         Serial.println(F(" bytes of payload"));
      }
      if (!userWakeup)
      {
         gotoSleep = true;
      }
      break;
   case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
   case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
   case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
   case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
   case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
   /*
       || This event is defined but not used in the code. No
       || point in wasting codespace on it.
       ||
       || case EV_SCAN_FOUND:
       ||    Serial.println(F("EV_SCAN_FOUND"));
       ||    break;
       */
   case EV_TXSTART:
      Serial.println(F("EV_TXSTART"));
      break;
   case EV_TXCANCELED:
      Serial.println(F("EV_TXCANCELED"));
      break;
   case EV_RXSTART:
      /* do not print anything -- it wrecks timing */
      break;
   case EV_JOIN_TXCOMPLETE:
      Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
      break;
   default:
      Serial.print(F("Unknown event: "));
      Serial.println((unsigned)ev);
      break;
   }
}

String measurementsTable()
{
   String table = "<h2>Measurements</h2><table>";
   table += "<tr><th>Data Name</th><th>Value</th><th>Unit</th></tr>";

   for (int i = 0; i < show_measurements.size(); i++)
   {
      table += "<tr>";
      table += "<td>" + show_measurements[i].data_name + "</td>";
      table += "<td>";

      if (!isnan(show_measurements[i].value))
      {
         table += String(show_measurements[i].value);
      }
      else
      {
         table += "NAN";
      }

      table += "</td>";
      table += "<td>";

      if (!(show_measurements[i].unit == "°C"))
      {
         table += show_measurements[i].unit;
      }
      else
      {
         table += "&deg;C";
      }

      table += "</td>";
      table += "</tr>";
   }

   table += "</table>";

   return table;
}

String connectorTable()
{
   String table = "<h2>Connectors</h2><table>";

   // I2C Connectors
   for (int i = 0; i < I2C_NUM; i++)
   {
      table += "<tr><td style='text-align: center;'>I2C_" + String(i + 1) + "</td>";

      if (I2C_con_table[i] != NO)
      {
         table += "<td>" + allSensors[I2C_con_table[i]].sensor_name + "</td>";
      }
      else
      {
         table += "<td>NO</td>";
      }

      table += "</tr>";
   }

   // ADC Connectors
   for (int i = 0; i < ADC_NUM; i++)
   {
      table += "<tr><td style='text-align: center;'>ADC_" + String(i + 1) + "</td>";

      if (ADC_con_table[i] != NO)
      {
         table += "<td>" + allSensors[ADC_con_table[i]].sensor_name + "</td>";
      }
      else
      {
         table += "<td>NO</td>";
      }

      table += "</tr>";
   }

   // 1-Wire Connectors
   for (int i = 0; i < ONEWIRE_NUM; i++)
   {
      table += "<tr><td style='text-align: center;'>1-Wire_" + String(i + 1) + "</td>";

      if (OneWire_con_table[i] != NO)
      {
         table += "<td>" + allSensors[OneWire_con_table[i]].sensor_name + "</td>";
      }
      else
      {
         table += "<td>NO</td>";
      }

      table += "</tr>";
   }

   table += "</table>";
   return table;
}

void handleRoot()
{
   String temp;
   int sec = millis() / 1000;
   int min = sec / 60;
   int hr = min / 60;

   temp = "<!DOCTYPE html><html>\
           <head>\
             <meta http-equiv='refresh' content='15'/>\
             <title>ESP32 Demo</title>\
             <meta name='viewport' content='width=device-width, initial-scale=1'>\
             <style>\
               html { font-family: Helvetica; min-height: 100%; text-align: center; background: linear-gradient(to bottom, #00CC99, #009999); color: #fff;}\
               table { margin: 10px auto; width: 100%; max-width: 600px; border-spacing: 10px; border-collapse: collapse; box-shadow: 0 0 8px rgba(0, 0, 0, 0.3); background-color: rgba(255, 255, 255, 0.4); }\
               th, td { padding: 8px; text-align: left; }\
               tr:not(:last-child) td { border-bottom: 1px solid #d9d9d9; }\
               th { background-color: #206040; color: #fff; }\
               tr:nth-child(even) { background-color: #00b386; }\
               tr:nth-child(odd) { background-color: #00cc99; }\
             </style>\
           </head>\
           <body>\
             <h1>TeleAgriCulture Board 2.1</h1>\
             <p>Uptime: " +
          String(hr) + ":" + String(min % 60) + ":" + String(sec % 60) + "</p>";
   temp += version;
   temp += "<br>BoardID: ";
   temp += boardID;
   temp += "<br>Upload: ";
   temp += upload;
   temp += measurementsTable();
   temp += connectorTable();
   temp += "</body></html>";

   server.send(200, "text/html", temp.c_str());
}

void handleNotFound()
{
   String message = "File Not Found\n\n";
   message += "URI: ";
   message += server.uri();
   message += "\nMethod: ";
   message += (server.method() == HTTP_GET) ? "GET" : "POST";
   message += "\nArguments: ";
   message += server.args();
   message += "\n";

   for (uint8_t i = 0; i < server.args(); i++)
   {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
   }

   server.send(404, "text/plain", message);
}

void drawGraph()
{
   String out = "";
   char temp[100];
   out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
   out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
   out += "<g stroke=\"black\">\n";
   int y = rand() % 130;
   for (int x = 10; x < 390; x += 10)
   {
      int y2 = rand() % 130;
      sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
      out += temp;
      y = y2;
   }
   out += "</g>\n</svg>\n";

   server.send(200, "image/svg+xml", out);
}

void do_send(LoraMessage &message)
{
   if (message.getLength() > 1)
   {
      // Prepare upstream data transmission at the next possible time.
      LMIC_setTxData2(1, message.getBytes(), message.getLength(), 0);
      // Serial.println(F("Packet queued\n"));
   }
}

void LoraWANPrintLMICOpmode(void)
{
   Serial.print(F("LMIC.opmode: "));
   if (LMIC.opmode & OP_NONE)
   {
      Serial.print(F("OP_NONE "));
   }
   if (LMIC.opmode & OP_SCAN)
   {
      Serial.print(F("OP_SCAN "));
   }
   if (LMIC.opmode & OP_TRACK)
   {
      Serial.print(F("OP_TRACK "));
   }
   if (LMIC.opmode & OP_JOINING)
   {
      Serial.print(F("OP_JOINING "));
   }
   if (LMIC.opmode & OP_TXDATA)
   {
      Serial.print(F("OP_TXDATA "));
   }
   if (LMIC.opmode & OP_POLL)
   {
      Serial.print(F("OP_POLL "));
   }
   if (LMIC.opmode & OP_REJOIN)
   {
      Serial.print(F("OP_REJOIN "));
   }
   if (LMIC.opmode & OP_SHUTDOWN)
   {
      Serial.print(F("OP_SHUTDOWN "));
   }
   if (LMIC.opmode & OP_TXRXPEND)
   {
      Serial.print(F("OP_TXRXPEND "));
   }
   if (LMIC.opmode & OP_RNDTX)
   {
      Serial.print(F("OP_RNDTX "));
   }
   if (LMIC.opmode & OP_PINGINI)
   {
      Serial.print(F("OP_PINGINI "));
   }
   if (LMIC.opmode & OP_PINGABLE)
   {
      Serial.print(F("OP_PINGABLE "));
   }
   if (LMIC.opmode & OP_NEXTCHNL)
   {
      Serial.print(F("OP_NEXTCHNL "));
   }
   if (LMIC.opmode & OP_LINKDEAD)
   {
      Serial.print(F("OP_LINKDEAD "));
   }
   if (LMIC.opmode & OP_LINKDEAD)
   {
      Serial.print(F("OP_LINKDEAD "));
   }
   if (LMIC.opmode & OP_TESTMODE)
   {
      Serial.print(F("OP_TESTMODE "));
   }
   if (LMIC.opmode & OP_UNJOIN)
   {
      Serial.print(F("OP_UNJOIN "));
   }
}

void saveLMICToRTC(int deepsleep_sec)
{
   Serial.println(F("Save LMIC to RTC"));
   RTC_LMIC = LMIC;

   // ESP32 can't track millis during DeepSleep and no option to advanced millis after DeepSleep.
   // Therefore reset DutyCyles

   unsigned long now = millis();

   // EU Like Bands
#if defined(CFG_eu868)
   Serial.println(F("Reset CFG_LMIC_EU_like band avail"));
   for (int i = 0; i < MAX_BANDS; i++)
   {
      ostime_t correctedAvail = RTC_LMIC.bands[i].avail - ((now / 1000.0 + deepsleep_sec) * OSTICKS_PER_SEC);
      if (correctedAvail < 0)
      {
         correctedAvail = 0;
      }
      RTC_LMIC.bands[i].avail = correctedAvail;
   }

   RTC_LMIC.globalDutyAvail = RTC_LMIC.globalDutyAvail - ((now / 1000.0 + deepsleep_sec) * OSTICKS_PER_SEC);
   if (RTC_LMIC.globalDutyAvail < 0)
   {
      RTC_LMIC.globalDutyAvail = 0;
   }
#else
   Serial.println(F("No DutyCycle recalculation function!"));
#endif
}

void loadLMICFromRTC()
{
   Serial.println(F("Load LMIC from RTC"));
   LMIC = RTC_LMIC;
}

// Function to store LMIC configuration to RTC Memory
void saveLORA_State(void)
{
   Serial.println(F("Save LMIC to RTC ..."));
   RTC_LORAWAN_netid = LMIC.netid;
   RTC_LORAWAN_devaddr = LMIC.devaddr;
   memcpy(RTC_LORAWAN_nwkKey, LMIC.nwkKey, 16);
   memcpy(RTC_LORAWAN_artKey, LMIC.artKey, 16);
   RTC_LORAWAN_dn2Dr = LMIC.dn2Dr;
   RTC_LORAWAN_dnConf = LMIC.dnConf;
   RTC_LORAWAN_seqnoDn = LMIC.seqnoDn;
   RTC_LORAWAN_seqnoUp = LMIC.seqnoUp;
   RTC_LORAWAN_adrTxPow = LMIC.adrTxPow;
   RTC_LORAWAN_txChnl = LMIC.txChnl;
   RTC_LORAWAN_datarate = LMIC.datarate;
   RTC_LORAWAN_adrAckReq = LMIC.adrAckReq;
   RTC_LORAWAN_rx1DrOffset = LMIC.rx1DrOffset;
   RTC_LORAWAN_rxDelay = LMIC.rxDelay;

#if (CFG_eu868)
   memcpy(RTC_LORAWAN_channelFreq, LMIC.channelFreq, MAX_CHANNELS * sizeof(u4_t));
   memcpy(RTC_LORAWAN_channelDrMap, LMIC.channelDrMap, MAX_CHANNELS * sizeof(u2_t));
   memcpy(RTC_LORAWAN_channelDlFreq, LMIC.channelDlFreq, MAX_CHANNELS * sizeof(u4_t));
   memcpy(RTC_LORAWAN_bands, LMIC.bands, MAX_BANDS * sizeof(band_t));
   RTC_LORAWAN_channelMap = LMIC.channelMap;
#endif

   Serial.println("LMIC configuration stored in RTC Memory.");
}

// Function to reload LMIC configuration from RTC Memory
void loadLORA_State()
{
   Serial.println(F("Load LMIC State from RTC ..."));
   Serial.println(F("Load LMIC State from RTC ..."));

   LMIC_setSession(RTC_LORAWAN_netid, RTC_LORAWAN_devaddr, RTC_LORAWAN_nwkKey, RTC_LORAWAN_artKey);
   LMIC_setSeqnoUp(RTC_LORAWAN_seqnoUp);
   LMIC_setDrTxpow(RTC_LORAWAN_datarate, RTC_LORAWAN_adrTxPow);
   LMIC.seqnoDn = RTC_LORAWAN_seqnoDn;
   LMIC.dnConf = RTC_LORAWAN_dnConf;
   LMIC.adrAckReq = RTC_LORAWAN_adrAckReq;
   LMIC.dn2Dr = RTC_LORAWAN_dn2Dr;
   LMIC.rx1DrOffset = RTC_LORAWAN_rx1DrOffset;
   LMIC.rxDelay = RTC_LORAWAN_rxDelay;
   LMIC.txChnl = RTC_LORAWAN_txChnl;

#if (CFG_eu868)
   memcpy(LMIC.bands, RTC_LORAWAN_bands, MAX_BANDS * sizeof(band_t));
   memcpy(LMIC.channelFreq, RTC_LORAWAN_channelFreq, MAX_CHANNELS * sizeof(u4_t));
   memcpy(LMIC.channelDlFreq, RTC_LORAWAN_channelDlFreq, MAX_CHANNELS * sizeof(u4_t));
   memcpy(LMIC.channelDrMap, RTC_LORAWAN_channelDrMap, MAX_CHANNELS * sizeof(u2_t));
   LMIC.channelMap = RTC_LORAWAN_channelMap;
#endif
   delay(200);
   delay(200);
   Serial.println("LMIC configuration reloaded from RTC Memory.");
}

void openConfig()
{
   startBlinking(); // Start blinking an LED to indicate configuration mode

   setUPWiFi(); // Set up WiFi connection

   backlight_pwm = 200;
   analogWrite(TFT_BL, backlight_pwm); // Turn off TFT Backlight

   wifiManager.setTimeout(900000); // set timeout for ConfigPortal to 15min (9000000ms)

   if (!wifiManager.startConfigPortal("TeleAgriCulture Board", "enter123"))
   {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.restart(); // Failed to connect, restart ESP32
   }
   ESP.restart();
   stopBlinking(); // Stop blinking the LED
}
