/*\
 *
 * TeleAgriCulture Board Firmware
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
 *


//TODO: prepaired but disabled in ConfigPortal -->Cert read and set  ---> String user_CA ---> to usable const char[]
//      WPA Enterprise works without cert and anonym identity at the moment
//TODO: WiFISecure Cert check maybe dissable? server cert may change... client.insecure() not avaiable in new espidf

//TODO: Battery optimation    ---> uses around 3mA without gas sensor and without display
//TODO: Sensor test

//TODO: find a I2C Address solution


/*
 *
 * For defines, GPIOs and implemented Sensors, see sensor_Board.hpp (BoardID, API_KEY and LORA credentials)
 *
 *
 * Config Portal Access Point:   SSID: TeleAgriCulture Board
 *                               pasword: enter123
 *
 * ------> Config Portal opens after double reset or holding BooT button for >5sec
 *
 * !! to build this project, take care that board_credentials.h is in the include folder (gets ignored by git)
 *
 * main() handles Config Accesspoint, WiFi, LoRa, load, save, time and display UI
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
#include <Ticker.h>

#include <driver/spi_master.h>
#include "driver/timer.h"
#include <driver/rtc_io.h>
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_wpa2.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include <BLEDevice.h>

#include <WiFi.h>

#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <time.h>
#include <sys/time.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <lmic.h>
#include <hal/hal.h>
#include <LoraMessage.h>

#include <tac_logo.h>
#include <customTitle_picture.h>
#include <sensor_Board.hpp> // Board and setup defines
#include <sensor_Read.hpp>  // Sensor read handling

#define ESP_DRD_USE_SPIFFS true
#define DOUBLERESETDETECTOR_DEBUG true

#include <ESP_DoubleResetDetector.h>
#include <Button.h>
#include <servers.h>
#include <WiFiManager.h>
#include <WiFiManagerTz.h> // Setup Page html rendering and input handling

#include <Wire.h>
#include <SPI.h>
#include "driver/spi_master.h"
#include "driver/i2c.h"

#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Adafruit_ST7735.h>

// ----- Deep Sleep related -----//
#define BUTTON_PIN_BITMASK 0x1   // GPIO 0
#define uS_TO_S_FACTOR 1000000UL /* Conversion factor for micro seconds to seconds */

RTC_DATA_ATTR int bootCount = 0;

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
void configModeCallback(WiFiManager *myWiFiManager);
int countMeasurements(const std::vector<Sensor> &sensors);
void checkButton(void);
void toggleLED(void);
void startBlinking(void);
void stopBlinking(void);

// file and storage functions
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
void do_send(osjob_t *j);
void onEvent(ev_t ev);
void convertTo_LSB_EUI(String input, uint8_t *output);
void convertTo_MSB_APPKEY(String input, uint8_t *output);

// debug fuctions
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void checkLoadedStuff(void);
void printConnectors(ConnectorType typ);
void printProtoSensors(void);
void printMeassurments(void);
void printSensors(void);
void wifi_sendData(void);
void printHex2(unsigned v);

// ----- Function declaration -----//

// ----- LoRa related -----//

static const int spiClk = 1000000; // 10 MHz

// https://www.loratools.nl/#/airtime
const unsigned TX_INTERVAL = 3580;
bool loraJoinFailed = false;
bool loraDataTransmitted = false;

// ----- LoRa related -----//

// ----- Initialize TFT ----- //
#define ST7735_TFTWIDTH 128
#define ST7735_TFTHEIGHT 160
#define background_color 0x07FF

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

int backlight_pwm = 250;
bool displayRefresh = true;
// ----- Initialize TFT ----- //

// ----- WiFiManager section ---- //
#define WM_NOHELP 1
WiFiManager wifiManager;
WiFiClientSecure client;
// ----- WiFiManager section ---- //

// ----- globals and defines --------

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 5

// RTC Memory Address for the DoubleResetDetector to use just for ESP8266
#define DRD_ADDRESS 0

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

bool sendDataWifi = false;
bool sendDataLoRa = false;

// Define a variable to hold the last hour value
int currentHour;
int lastHour = -1;
unsigned long lastExecutionTime = 0;
int seconds_to_wait = 0;

unsigned long upButtonsMillis = 0;
unsigned long previousMillis = 0;
unsigned long previousMillis_long = 0;
const long interval = 200000;   // screen backlight darken time 20s
const long interval2 = 3000000; // screen goes black after 5min

// Battery levels
double vs[101]; //   Copyright (c) 2019 Pangodream   	https://github.com/pangodream/18650CL

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
   pinMode(SW_3V3, OUTPUT);
   pinMode(SW_5V, OUTPUT);

   delay(500);               // for debugging in screen
   Serial.setTxTimeoutMs(5); // set USB CDC Time TX
   Serial.begin(115200);     // start Serial for debuging

   // Initialize the NVS (non-volatile storage) for saving and restoring the keys
   nvs_flash_init();

   // Initialize SPIFFS
   if (!SPIFFS.begin(true))
   {
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
   }

   // Print the wakeup reason for ESP32
   doubleReset_wakeup_reason();
   GPIO_wake_up();
   esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

   delay(100);

   // Increment boot number and print it every reboot
   ++bootCount;
   Serial.println("\nBoot number: " + String(bootCount));

   upButton.begin();
   downButton.begin();

   // calibrate ADC1
   esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_2_5, ADC_WIDTH_BIT_12, 0, &adc_cal);
   adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_2_5); // BATSENS
   adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_2_5); // ADC1_CON
   adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_2_5); // ADC2_CON
   adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_2_5); // ADC3_CON

   digitalWrite(SW_3V3, HIGH);
   digitalWrite(LORA_CS, HIGH);

   /* ------------  Test DATA for connected Sensors  --------------------------

      ---> comes from Web Config normaly or out of SPIFF

  I2C_con_table[0] = NO;
  I2C_con_table[1] = NO;
  I2C_con_table[2] = NO;
  I2C_con_table[3] = NO;
  ADC_con_table[0] = CAP_SOIL;
  ADC_con_table[1] = TDS;
  ADC_con_table[2] = TDS;
  OneWire_con_table[0] = DHT_22;
  OneWire_con_table[1] = DHT_22;
  OneWire_con_table[2] = DHT_22;
  SPI_con_table[0] = NO;
  I2C_5V_con_table[0] = MULTIGAS_V1;
  EXTRA_con_table[0] = NO;
  EXTRA_con_table[1] = NO;

  save_Connectors(); // <------------------ called normaly after Web Config

  Test DATA for connected Sensors ---> come from Web Config normaly

  ---------------------------------------------------------------------------------*/

   // print SPIFF files for debugging
   // if (!SPIFFS.begin(true))
   // {
   //    Serial.println("Failed to mount SPIFFS file system");
   //    return;
   // }
   // listDir(SPIFFS, "/", 0);
   // Serial.println();

   load_Sensors();    // Prototypes get loaded
   load_Connectors(); // Connectors lookup table
   load_Config();     // loade config Data

   Wire.setPins(I2C_SDA, I2C_SCL);

   // checkLoadedStuff();

   if (useDisplay || forceConfig)
   {
      // ----- Initiate the TFT display and Start Image----- //
      tft.initR(INITR_GREENTAB); // work around to set protected offset values
      tft.initR(INITR_BLACKTAB); // change the colormode back, offset values stay as "green display"

      tft.cp437(true);
      tft.setCursor(0, 0);
      tft.setRotation(3);
   }

   if (useBattery && useDisplay)
   {
      analogWrite(TFT_BL, 25); // turn TFT Backlight on
      tft.fillScreen(ST7735_BLACK);
   }

   if (!useBattery && useDisplay)
   {
      analogWrite(TFT_BL, backlight_pwm); // turn TFT Backlight on
      tft.fillScreen(background_color);
      tft.drawRGBBitmap(0, 0, tac_logo, 160, 128);
   }
   // ----- Initiate the TFT display  and Start Image----- //

   if (upload == "WIFI" && !forceConfig)
   {

      // Cert field in ConfigPortal deactivated --> do not know if this would work, can not test it now
      // https://github.com/martinius96/ESP32-eduroam
      // WiFiManager handles esp_wpa2.h connection, username and password
      if (useEnterpriseWPA && !forceConfig)
      {
         WiFi.disconnect(true);
         WiFi.mode(WIFI_STA);

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
      }

      if ((WiFi.status() == WL_CONNECTED) && useNTP)
      {
         WiFiManagerNS::configTime();
         WiFiManagerNS::NTP::onTimeAvailable(&on_time_available);
      }

      stopBlinking();
   }

   if (upload == "WIFI")
      sendDataWifi = true;

   if (upload == "LORA")
   {
      WiFiManagerNS::TZ::loadPrefs();

      digitalWrite(SW_3V3, HIGH);

      sendDataLoRa = true;
   }
}

void loop()
{
   drd->loop();
   analogWrite(TFT_BL, backlight_pwm); // turn TFT Backlight on

   time_t rawtime;
   time(&rawtime);
   localtime_r(&rawtime, &timeInfo);

   currentHour = timeInfo.tm_hour;

   if (currentHour != lastHour)
   {
      if (upload == "WIFI")
         sendDataWifi = true;
      if (upload == "LORA")
         sendDataLoRa = true;
      // The hour has changed since the last execution of this block
      // Set the last hour variable to the current hour to avoid triggering the function multiple times in the same hour
      lastHour = currentHour;
   }

   if (forceConfig)
   {
      startBlinking();

      setUPWiFi();

      // wifiManager.setAPCallback(configModeCallback);

      if (!wifiManager.startConfigPortal("TeleAgriCulture Board", "enter123"))
      {
         Serial.println("failed to connect and hit timeout");
         delay(3000);
         // reset and try again, or maybe put it to deep sleep
         ESP.restart();
      }
      ESP.restart();
      stopBlinking();
   }

   if (sendDataWifi)
   {
      sensorRead();
      // Serial.println("\nPrint Measurements: ");
      // printMeassurments();
      // Serial.println("\nPrint Sensors connected: ");
      // printSensors();
      // Serial.println();
      wifi_sendData();

      sendDataWifi = false;

      if (!(WiFiManagerNS::NTPEnabled))
      {
         String header = get_header();
         delay(1000);
         String Time1 = getDateTime(header);

         setenv("TZ", "GMT0", 1); // set Timezone to GMT
         tzset();

         setEsp32Time(Time1.c_str());
      }

      setUploadTime();

      if ((!useBattery || !gotoSleep) && useDisplay)
      {
         renderPage(currentPage);
      }
   }

   if (sendDataLoRa)
   {
      sensorRead();

      loraDataTransmitted = false;
      sendDataLoRa = false;
      gotoSleep = false;

      lora_sendData();

      if ((!useBattery || !gotoSleep) && useDisplay)
      {
         renderPage(currentPage);
      }
   }

   unsigned long currentMillis = millis();
   unsigned long currentMillis_long = millis();

   if (currentMillis - previousMillis >= interval)
   {
      backlight_pwm = 5; // turns Backlight down
      previousMillis = currentMillis;

      if (upload == "WIFI" && useBattery)
      {
         gotoSleep = true;
      }

      if (upload == "LORA" && useBattery && loraDataTransmitted)
      {
         gotoSleep = true;
      }
   }

   if (currentMillis_long - previousMillis_long >= interval2)
   {
      backlight_pwm = 0; // turns Backlight off
      tft.fillScreen(ST7735_BLACK);
      previousMillis_long = currentMillis_long;
      tft.enableSleep(true);
   }

   num_pages = NUM_PAGES + total_measurement_pages;

   if (downButton.pressed())
   {
      currentPage = (currentPage + 1) % num_pages;
      backlight_pwm = 250;
      tft.enableSleep(false);
      gotoSleep = false;
   }

   if (upButton.pressed())
   {
      currentPage = (currentPage - 1 + num_pages) % num_pages;
      backlight_pwm = 250;
      tft.enableSleep(false);
      upButtonsMillis = millis();
      gotoSleep = false;
   }

   if (upButton.released())
   {
      if (millis() - upButtonsMillis > 5000)
      {
         startBlinking();
         if (upload != "WIFI")
         {
            setUPWiFi();
         }

         if (!wifiManager.startConfigPortal("TeleAgriCulture Board", "enter123"))
         {
            Serial.println("failed to connect and hit timeout");
            delay(3000);
            // reset and try again, or maybe put it to deep sleep
            ESP.restart();
            delay(5000);
         }
      }
      upButtonsMillis = 0;
      stopBlinking();
   }

   if ((useBattery && gotoSleep) || (!useDisplay))
   {
      // Stop Lora befor sleep
      if (upload == "LORA")
      {
         LMIC_shutdown();
         esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ALL_LOW);
         esp_sleep_enable_timer_wakeup((3585) * uS_TO_S_FACTOR);
         Serial.println("Setup ESP32 to sleep for 3600 Seconds");
      }

      // Stop WiFi befor sleep
      if (upload == "WIFI")
      {
         wifiManager.disconnect();
         WiFi.mode(WIFI_OFF);

         esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ALL_LOW);
         esp_sleep_enable_timer_wakeup((seconds_to_next_hour() - 15) * uS_TO_S_FACTOR);
         Serial.println("Setup ESP32 to sleep for " + String(seconds_to_next_hour()) + " Seconds");
      }

      // Stop all events
      blinker.detach();

      // Stop I2C
      Wire.end();

      // Stop SPI
      tft.enableSleep(true);
      // spi_bus_free(SPI1_HOST);
      spi_bus_free(SPI2_HOST);

      // reset Pins
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

      delay(100);

      esp_deep_sleep_start();
   }

   // Only refresh the screen if the current page has changed
   if ((currentPage != lastPage) || displayRefresh)
   {
      lastPage = currentPage;
      if (useDisplay)
      {
         renderPage(currentPage);
         displayRefresh = false;
      }
   }

   if (currentPage == 0)
   {
      if ((now() != prevDisplay) && upload == "WIFI")
      { // update the display only if time has changed
         prevDisplay = now();
         if (useDisplay && (!useBattery || !gotoSleep))
         {
            digitalClockDisplay(5, 95, false);
         }
      }
   }

   if (upload == "LORA")
   {
      if (!loraDataTransmitted)
      {
         os_runloop_once();
      }
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
   tft.setTextSize(1);
   tft.setCursor(x, y);
   tft.setTextColor(ST7735_WHITE);

   tft.fillRect(x - 5, y - 2, 60, 10, ST7735_BLACK);

   tft.print(timeInfo.tm_hour);
   printDigits(timeInfo.tm_min);
   printDigits(timeInfo.tm_sec);

   if (date)
   {
      tft.print("   ");
      tft.print(timeInfo.tm_mday);
      tft.print(" ");
      tft.print(timeInfo.tm_mon + 1);
      tft.print(" ");
      tft.print(timeInfo.tm_year - 100 + 2000);
      tft.println();
   }
}

void drawBattery(int x, int y)
{
   int bat = getBatteryChargeLevel();
   tft.setCursor(x, y);
   tft.setTextColor(0xCED7);
   tft.print("Bat: ");
   if (bat < 1)
   {
      tft.setTextColor(ST7735_WHITE);
      tft.print("NO");
      return;
   }
   if (bat > 1 && bat < 20)
   {
      tft.setTextColor(ST7735_RED);
   }
   if (bat >= 20 && bat < 40)
   {
      tft.setTextColor(ST7735_ORANGE);
   }
   if (bat >= 40 && bat < 60)
   {
      tft.setTextColor(ST7735_YELLOW);
   }
   if (bat >= 60 && bat < 80)
   {
      tft.setTextColor(0xAEB2);
   }
   if (bat >= 80)
   {
      tft.setTextColor(ST7735_GREEN);
   }
   tft.print(bat);
   tft.print(" %");
}

void printDigits(int digits)
{
   // utility for digital clock display: prints preceding colon and leading 0
   tft.print(":");
   if (digits < 10)
      tft.print('0');
   tft.print(digits);
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
      if (useDisplay)
         gotoSleep = false;
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
   tft.fillScreen(ST7735_BLACK);
   tft.setTextColor(0x9E6F);
   tft.setFont(&FreeSans9pt7b);
   tft.setTextSize(1);
   tft.setCursor(5, 17);
   tft.print("TeleAgriCulture");
   tft.setCursor(5, 38);
   tft.print("Board V2.1");

   tft.setTextColor(ST7735_BLUE);
   tft.setFont();
   tft.setTextSize(1);
   tft.setCursor(5, 45);
   tft.print("Board ID: ");
   tft.print(boardID);
   tft.setCursor(5, 55);
   tft.print(version);
   tft.setTextColor(0xCED7);
   tft.setCursor(5, 65);
   tft.print("WiFi: ");
   tft.setTextColor(ST7735_WHITE);
   tft.print(WiFi.SSID());
   tft.setCursor(5, 75);
   tft.setTextColor(0xCED7);
   tft.print("IP: ");
   tft.setTextColor(ST7735_WHITE);
   tft.print(WiFi.localIP());
   tft.setCursor(5, 85);
   tft.setTextColor(0xCED7);
   tft.print("MAC: ");
   tft.setTextColor(ST7735_WHITE);
   tft.print(WiFi.macAddress());

   if (upload == "WIFI")
   {
      digitalClockDisplay(5, 95, true);

      tft.setCursor(5, 105);
      tft.print("last data UPLOAD: ");
      tft.setTextColor(ST7735_ORANGE);
      tft.print(upload);

      tft.setTextColor(ST7735_ORANGE);
      tft.setCursor(5, 115);
      tft.print(lastUpload);
   }
   else
   {
      tft.setTextColor(ST7735_ORANGE);
      tft.setCursor(5, 95);
      tft.print("TTN no time sync");
      tft.setTextColor(ST7735_RED);
      tft.setCursor(5, 105);
      tft.print(lora_fqz);
      tft.print("  ");
      tft.setTextColor(ST7735_ORANGE);
      tft.print(upload);

      if (loraJoinFailed)
      {
         tft.setTextColor(ST7735_RED);
         tft.setCursor(5, 115);
         tft.print("JOIN FAILED");
      }
      else
      {
         if (loraDataTransmitted)
         {
            tft.setTextColor(ST7735_GREEN);
            tft.setCursor(5, 115);
            tft.print("packet ACK");
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
   tft.fillScreen(ST7735_BLACK);
   tft.setTextSize(2);
   tft.setCursor(10, 10);
   tft.setTextColor(ST7735_WHITE);
   tft.print("I2C Con");

   tft.setTextSize(1);
   int cursor_y = 35;

   for (int i = 0; i < I2C_NUM; i++)
   {
      tft.setCursor(5, cursor_y);
      tft.setTextColor(ST7735_YELLOW);

      tft.print("I2C_");
      tft.print(i + 1);
      tft.setCursor(80, cursor_y);
      if (I2C_con_table[i] == -1)
      {
         tft.setTextColor(ST7735_RED);
         tft.print("NO");
      }
      else
      {
         tft.setTextColor(ST7735_GREEN);
         tft.print(allSensors[I2C_con_table[i]].sensor_name);
      }
      cursor_y += 10;
   }
   cursor_y += 10;
   tft.setTextSize(2);
   tft.setCursor(10, cursor_y);
   tft.setTextColor(ST7735_WHITE);
   tft.print("I2C_5V Con");

   cursor_y += 25;

   tft.setTextSize(1);
   tft.setCursor(5, cursor_y);
   tft.setTextColor(ST7735_YELLOW);

   tft.print("I2C_5V");

   tft.setCursor(80, cursor_y);
   if (I2C_5V_con_table[0] == -1)
   {
      tft.setTextColor(ST7735_RED);
      tft.print("NO");
   }
   else
   {
      tft.setTextColor(ST7735_GREEN);
      tft.print(allSensors[I2C_5V_con_table[0]].sensor_name);
   }
}

void ADC_ConnectorPage()
{
   tft.fillScreen(ST7735_BLACK);
   tft.fillScreen(ST7735_BLACK);
   tft.setTextSize(2);
   tft.setCursor(10, 10);
   tft.setTextColor(ST7735_WHITE);
   tft.print("ADC Con");

   tft.setTextSize(1);
   int cursor_y = 45;

   for (int i = 0; i < ADC_NUM; i++)
   {
      tft.setCursor(5, cursor_y);
      tft.setTextColor(ST7735_YELLOW);

      tft.print("ADC_");
      tft.print(i + 1);
      tft.setCursor(80, cursor_y);
      if (ADC_con_table[i] == -1)
      {
         tft.setTextColor(ST7735_RED);
         tft.print("NO");
      }
      else
      {
         tft.setTextColor(ST7735_GREEN);
         tft.print(allSensors[ADC_con_table[i]].sensor_name);
      }
      cursor_y += 10;
   }
}

void OneWire_ConnectorPage()
{
   tft.fillScreen(ST7735_BLACK);

   tft.fillScreen(ST7735_BLACK);
   tft.setTextSize(2);
   tft.setCursor(10, 10);
   tft.setTextColor(ST7735_WHITE);
   tft.print("OneWire Con");

   tft.setTextSize(1);
   int cursor_y = 45;

   for (int i = 0; i < ONEWIRE_NUM; i++)
   {
      tft.setCursor(5, cursor_y);
      tft.setTextColor(ST7735_YELLOW);

      tft.print("1-Wire_");
      tft.print(i + 1);
      tft.setCursor(80, cursor_y);
      if (OneWire_con_table[i] == -1)
      {
         tft.setTextColor(ST7735_RED);
         tft.print("NO");
      }
      else
      {
         tft.setTextColor(ST7735_GREEN);
         tft.print(allSensors[OneWire_con_table[i]].sensor_name);
      }
      cursor_y += 10;
   }
}

void measurementsPage(int page)
{
   tft.fillScreen(ST7735_BLACK);
   tft.setTextSize(2);
   tft.setCursor(10, 10);
   tft.setTextColor(ST7735_WHITE);
   tft.print("Sensor Data");

   tft.setTextSize(1);

   int cursor_y = 35;

   int startIndex = (page - NUM_PAGES) * NUM_PERPAGE;
   int endIndex = startIndex + NUM_PERPAGE;
   if (endIndex > show_measurements.size())
   {
      endIndex = show_measurements.size();
   }

   for (int i = startIndex; i < endIndex; i++)
   {
      tft.setCursor(5, cursor_y);
      tft.setTextColor(ST7735_BLUE);
      tft.print(show_measurements[i].data_name);
      tft.print(": ");

      tft.setCursor(60, cursor_y);
      tft.setTextColor(ST7735_YELLOW);

      if (!isnan(show_measurements[i].value))
      {
         tft.print(show_measurements[i].value);
      }
      else
      {
         tft.print("NAN");
      }

      tft.print(" ");

      if (!(show_measurements[i].unit == "°C"))
      {
         tft.print(show_measurements[i].unit);
      }
      else
      {
         tft.drawChar(tft.getCursorX(), tft.getCursorY(), 0xF8, ST7735_YELLOW, ST7735_BLACK, 1);
         tft.setCursor(tft.getCursorX() + 7, tft.getCursorY());
         tft.print("C");
      }
      cursor_y += 10;
   }
}

void checkLoadedStuff(void)
{
   Serial.println();
   Serial.println("---------------Prototype Sensors loaded -----------");
   printProtoSensors();
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

   Serial.print("\nBoard ID: ");
   Serial.println(boardID);
   Serial.print("Battery powered: ");
   Serial.println(useBattery);
   Serial.print("Display: ");
   Serial.println(useDisplay);
   Serial.print("use Enterprise WPA: ");
   Serial.println(useEnterpriseWPA);
   Serial.print("use Custom NTP: ");
   Serial.println(useCustomNTP);
   Serial.print("use NTP: ");
   Serial.println(useNTP);
   Serial.print("API Key: ");
   Serial.println(API_KEY);
   Serial.print("Timezone: ");
   Serial.println(timeZone);
   Serial.print("Upload: ");
   Serial.println(upload);
   Serial.print("Anonym ID: ");
   Serial.println(anonym);
   Serial.print("\nUser CA: ");
   Serial.println(user_CA);
   Serial.print("Custom NTP Address: ");
   Serial.println(customNTPaddress);
   Serial.print("Lora Frequency: ");
   Serial.println(lora_fqz);
   Serial.print("OTAA DEVEUI: ");
   Serial.println(OTAA_DEVEUI);
   Serial.print("OTAA APPEUI: ");
   Serial.println(OTAA_APPEUI);
   Serial.print("OTAA APPKEY: ");
   Serial.println(OTAA_APPKEY);

   Serial.println("\n---------------DEBUGG END -----------");

   Serial.println();
}

void save_Config(void)
{
   StaticJsonDocument<2500> doc;

   doc["BoardID"] = boardID;
   doc["useBattery"] = useBattery;
   doc["useDisplay"] = useDisplay;
   doc["useEnterpriseWPA"] = useEnterpriseWPA;
   doc["useCustomNTP"] = useCustomNTP;
   doc["useNTP"] = useNTP;
   doc["API_KEY"] = API_KEY;
   doc["upload"] = upload;
   doc["anonym"] = anonym;
   doc["user_CA"] = user_CA;
   doc["customNTPadress"] = customNTPaddress;
   doc["timeZone"] = timeZone;
   doc["OTAA_DEVEUI"] = OTAA_DEVEUI;
   doc["OTAA_APPEUI"] = OTAA_APPEUI;
   doc["OTAA_APPKEY"] = OTAA_APPKEY;

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
               useEnterpriseWPA = doc["useEnzerpriseWPA"];
               useCustomNTP = doc["useCustomNTP"];
               useNTP = doc["useNTP"];
               API_KEY = doc["API_KEY"].as<String>();
               upload = doc["upload"].as<String>();
               anonym = doc["anonym"].as<String>();
               user_CA = doc["user_CA"].as<String>();
               customNTPaddress = doc["customNTPadress"].as<String>();
               timeZone = doc["timeZone"].as<String>();
               OTAA_DEVEUI = doc["OTAA_DEVEUI"].as<String>();
               OTAA_APPEUI = doc["OTAA_APPEUI"].as<String>();
               OTAA_APPKEY = doc["OTAA_APPKEY"].as<String>();
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
         WiFiClientSecure client;
         client.setCACert(kits_ca);

         HTTPClient https;

         // https://gitlab.com/teleagriculture/community/-/blob/main/API.md

         // python example
         // https://gitlab.com/teleagriculture/community/-/blob/main/RPI/tacserial.py

         // should show up there:
         // https://kits.teleagriculture.org/kits/1003
         // Sensor: test

         // https.begin(client, "https://kits.teleagriculture.org/api/kits/1003/measurements");

         String serverName = "https://kits.teleagriculture.org/api/kits/" + String(boardID) + "/measurements";
         String api_Bearer = "Bearer " + API_KEY;

         https.begin(client, serverName);

         https.addHeader("Content-Type", "application/json");
         https.addHeader("Authorization", api_Bearer);

         int httpResponseCode = https.POST(output);

         Serial.print("\nHTTP Response code: ");
         Serial.println(httpResponseCode);
         Serial.println();
         Serial.println(serverName);
         Serial.println(api_Bearer);

         // Free resources
         https.end();
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
   convertTo_LSB_EUI(OTAA_DEVEUI, dev_eui);
   convertTo_LSB_EUI(OTAA_APPEUI, app_eui);
   convertTo_MSB_APPKEY(OTAA_APPKEY, app_key);

   // LMIC init
   os_init();

   // Reset the MAC state. Session and pending data transfers will be discarded.
   LMIC_reset();

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
            }
         }
      }
   }

   if (message.getLength() > 2)
   {
      Serial.println("\nSend Lora Data: ");

      if (LMIC.opmode & OP_TXRXPEND)
      {
         Serial.println(F("OP_TXRXPEND, not sending"));
      }
      else
      {
         // Prepare upstream data transmission at the next possible time.
         LMIC_setTxData2(1, message.getBytes(), message.getLength(), 1);
         Serial.println(F("Packet queued\n"));
      }
      // Next TX is scheduled after TX_COMPLETE event.
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
      tft.fillScreen(ST7735_BLACK);
      tft.setTextColor(ST7735_WHITE);
      tft.setFont(&FreeSans9pt7b);
      tft.setTextSize(1);
      tft.setCursor(5, 17);
      tft.print("TeleAgriCulture");
      tft.setCursor(5, 38);
      tft.print("Board V2.1");

      tft.setTextColor(ST7735_RED);
      tft.setFont();
      tft.setTextSize(2);
      tft.setCursor(5, 50);
      tft.print("Config MODE");
      tft.setTextColor(ST7735_WHITE);
      tft.setTextSize(1);
      tft.setCursor(5, 73);
      tft.print("SSID:");
      tft.setCursor(5, 85);
      tft.print(myWiFiManager->getConfigPortalSSID());
      tft.setCursor(5, 95);
      tft.print("IP: ");
      tft.print(WiFi.softAPIP());
      tft.setCursor(5, 108);
      tft.print("MAC: ");
      tft.print(WiFi.macAddress());
      tft.setCursor(5, 117);
      tft.setTextColor(ST7735_BLUE);
      tft.print(version);
   }
}

String get_header()
{
   WiFiClientSecure client;

   // makes a HTTP request
   unsigned long timeNow;
   bool HeaderComplete = false;
   bool currentLineIsBlank = true;
   String header = "";
   client.stop(); // Close any connection before send a new request.  This will free the socket on the WiFi
   if (client.connect(GET_Time_SERVER, SSL_PORT))
   { // if there's a successful connection:
      client.println("GET " + GET_Time_Address + " HTTP/1.1");
      client.print("HOST: ");
      client.println(GET_Time_SERVER);
      client.println();
      timeNow = millis();
      while (millis() - timeNow < TIMEOUT)
      {
         while (client.available())
         {
            char c = client.read();
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
   if (client.connected())
   {
      client.stop();
   }
   return header;
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

   struct timeval set_Time = {.tv_sec = sec};
   settimeofday(&set_Time, NULL);

   // setTime(sec);
   // setTime(now.tm_hour, now.tm_min, now.tm_sec, now.tm_mday, now.tm_mon, now.tm_year);
   setenv("TZ", timeZone.c_str(), 1);
   tzset();

   getLocalTime(&timeInfo);
   Serial.printf("\nSetting time based on GET: %s", asctime(&timeInfo));

   currentHour = timeInfo.tm_hour;
   lastHour = currentHour;
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

   wifiManager.setDebugOutput(false);
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
   tft.fillScreen(ST7735_BLACK);
   tft.setTextSize(1);
   tft.setTextColor(ST7735_WHITE);

   int cursor_y = 10;

   for (int i = 0; i < show_measurements.size(); i++)
   {
      tft.setCursor(5, cursor_y);
      tft.setTextColor(ST7735_BLUE);
      tft.print(show_measurements[i].data_name);
      tft.print(": ");

      tft.setCursor(60, cursor_y);
      tft.setTextColor(ST7735_YELLOW);

      if (!isnan(show_measurements[i].value))
      {
         tft.print(show_measurements[i].value);
      }
      else
      {
         tft.print("NAN");
      }

      tft.print(" ");

      if (!(show_measurements[i].unit == "°C"))
      {
         tft.print(show_measurements[i].unit);
      }
      else
      {
         tft.drawChar(tft.getCursorX(), tft.getCursorY(), 0xF8, ST7735_YELLOW, ST7735_BLACK, 1);
         tft.setCursor(tft.getCursorX() + 7, tft.getCursorY());
         tft.print("C");
      }
      cursor_y += 10;
   }
   cursor_y += 10;
   tft.setCursor(5, cursor_y);
   tft.setTextColor(ST7735_ORANGE);

   tft.print("Deep Sleep for: ");
   tft.print(seconds_to_next_hour());
   // tft.print(TIME_TO_SLEEP);
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
      Serial.println(F("EV_JOINED"));
      {
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
      }
      // Disable link check validation (automatically enabled
      // during join, but because slow data rates change max TX
      // size, we don't use it in this example.
      LMIC_setLinkCheckMode(0);
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
      displayRefresh = true;
      break;
   case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
   case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
      {
         Serial.println(F("Received ack"));
         loraDataTransmitted = true;
         displayRefresh = true;

         if (!userWakeup)
            gotoSleep = true;
      }
      if (LMIC.dataLen)
      {
         Serial.print(F("Received "));
         Serial.print(LMIC.dataLen);
         Serial.println(F(" bytes of payload"));
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
