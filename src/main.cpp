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


/*********************************** VERSION 1.60 ****************************
/*
 *
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

#include <Arduino.h>

#include <utilities/tac_logo.h>
#include <utilities/customTitle_picture.h>
#include <board_credentials.h>

#include <init_Board.h>
#include <file_functions.h>
#include <time_functions.h>
#include <ui_functions.h>
#include <web_functions.h>
#include <lora_functions.h>
#include <debug_functions.h>
#include <sensor_Read.hpp> // Sensor read handling
#include <WiFiManagerTz.h> // Setup Page html rendering and Web UI input handling ( save_Config() and save_Connectors() gets called in /lib/WiFiManagerTz.h handleValues() )
#include <RTClib.h>
#include "mqtt_support.h"
#include "osc_support.h"
#include "esp_task_wdt.h"

#define TAC_LOG_LEVEL 3
#define LOG_TAG "MAIN"

#define DEBUG_PRINT false // full debug print
#define DOUBLERESETDETECTOR_DEBUG true
bool configPortal_run = false;

// ----- Function declaration -----//
void openConfig(void);
void handleWakeupDisplay();

void setUPWiFi();
void wifi_sendData(void);

void connectIfWifi();
void setupLoRaIfNeeded();
void setupSDCardIfNeeded();
void setupWebpageIfNeeded();

void setupMQTTIfNeeded();
void setupOSCIfNeeded();

void updateTimeHandle();
void handleButtonPresses();
void updateDisplay();

void handleLoraLoop();
void handleWiFiLoop();
void handleSDLoop();
void handleMQTTLoop();
void handleOSCLoop();

// Callbacks
void configModeCallback(WiFiManager *myWiFiManager);
void on_time_available(struct timeval *t);
void onMqttMessage(char *topic, byte *payload, unsigned int len);

// Task
TaskHandle_t configTaskHandle;
void configButtonTask(void *parameter);

void setup()
{
#if DEBUG_PRINT
   delay(2000);
#endif

   initBoard();
   initFiles();

   load_Sensors();    // Prototypes get loaded
   load_Connectors(); // Connectors lookup table
   load_Config();     // load config Data

   initDisplayIfNeeded();
   handleWakeupDisplay();

   // Start the task to monitor the config button. Runs now in its own task, since there have been some issues with it
   xTaskCreatePinnedToCore(
       configButtonTask,
       "ConfigButtonTask",
       4000, // Stack size
       NULL,
       1, // priority
       &configTaskHandle,
       0 // Core-ID: 0 (Loop runs on Core 1)
   );

   if (rtcEnabled)
      setESP32timeFromRTC(timeZone.c_str()); // Set the time on ESP32 using the RTC module

#if DEBUG_PRINT
   delay(2000);
   // printProtoSensors();
   printRTCInfo();
   Serial.println();
   checkLoadedStuff();
   scanI2CDevices();
   delay(200);
#endif

   // upload = "LORA";
   // upload = "WIFI";
   // useSDCard = true;
   // useBattery = false;

   Serial.print("\nBoard ID: ");
   Serial.println(boardID);

   if (forceConfig)
      openConfig();

   if (upload == "NO_UPLOAD")
      no_upload = true;

   if (!useBattery)
      webpage = true;

   connectIfWifi();
   setupLoRaIfNeeded();
   setupSDCardIfNeeded();
   setupMQTTIfNeeded();
   setupOSCIfNeeded();
   setupWebpageIfNeeded();
}

void loop()
{
   drd->loop(); // Process double reset detection

   analogWrite(TFT_BL, backlight_pwm); // Set the backlight brightness of the TFT display

   updateTimeHandle(); // Update time and handle periodic tasks (display & upload)
   handleButtonPresses();

   if (no_upload && !configPortal_run)
   {
      sensorRead(); // Read sensor data
      no_upload = false;

      if (rtcEnabled == true)
         setESP32timeFromRTC(timeZone.c_str());

      if ((!useBattery || !gotoSleep) && useDisplay)
      {
         renderPage(currentPage); // Render the current page on the display
      }
   }

   if (upload == "LORA" && !configPortal_run)
      handleLoraLoop();

   if (upload == "WIFI" && !configPortal_run)
      handleWiFiLoop();

   if (saveDataSDCard && !configPortal_run)
      tft->drawRGBBitmap(145, 2, sdcard_icon, 14, 19);

   if (useSDCard && !configPortal_run)
      handleSDLoop();

   if (upload == "LIVE")
   {
      if (live_mode == "MQTT" && !configPortal_run)
         handleMQTTLoop();

      if (live_mode == "OSC" && !configPortal_run)
         handleOSCLoop();
   }

   updateDisplay();

   if (webpage && !configPortal_run)
      server.handleClient(); // Handle incoming client requests

   vTaskDelay(50);
}

void openConfig()
{
   startBlinking(); // Start blinking an LED to indicate configuration mode

   backlight_pwm = 200;
   analogWrite(TFT_BL, backlight_pwm); // Turn off TFT Backlight

   setUPWiFi();

   Serial.println("Starting Config Portal");

   // set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
   wifiManager.setAPCallback(configModeCallback);
   Serial.print("Board ID: ");
   Serial.println(boardID);
   Serial.println("SSID: TeleAgriCulture Board");
   Serial.println("PW: enter123");

   wifiManager.setCleanConnect(true);

   if (!wifiManager.startConfigPortal("TeleAgriCulture Board", "enter123"))
   {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.restart(); // Failed to connect, restart ESP32
   }
   ESP.restart();
}

void handleWakeupDisplay()
{
   if (useBattery && useDisplay && !userWakeup)
   {
      analogWrite(TFT_BL, 0);
      tft->fillScreen(ST7735_BLACK);
   }

   if (!useBattery && useDisplay)
   {
      analogWrite(TFT_BL, backlight_pwm);
      tft->fillScreen(background_color);
      tft->drawRGBBitmap(0, 0, tac_logo, 160, 128);
   }

   if (userWakeup && useDisplay)
   {
      analogWrite(TFT_BL, backlight_pwm);
      tft->fillScreen(ST7735_BLACK);
      tft->setTextColor(0x9E6F);
      tft->setFont(&FreeSans9pt7b);
      tft->setCursor(5, 50);
      tft->print("Wakeup by USER");
   }
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

   // set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
   wifiManager.setAPCallback(configModeCallback);

   // attach board-setup page to the WiFi Manager
   WiFiManagerNS::init(&wifiManager);

#if DEBUG_PRINT
   wifiManager.setDebugOutput(true);
#endif

   wifiManager.setHostname(hostname.c_str());
   wifiManager.setTitle("Board Config");
   wifiManager.setCustomHeadElement(custom_Title_Html.c_str());

   Serial.println("Starting WiFi");

   wifiManager.setConnectRetries(10);
   wifiManager.setCleanConnect(true);
   wifiManager.setConfigPortalBlocking(true);
   wifiManager.setTimeout(900); // set timeout for ConfigPortal to 15min (900s)

   // /!\ make sure "custom" is listed there as it's required to pull the "Board Setup" button
   std::vector<const char *> menu = {"custom", "wifi", "info", "update", "restart", "exit"};
   wifiManager.setMenu(menu);

   wifiManager.setShowInfoUpdate(true);
   wifiManager.setDarkMode(true);
   wifiManager.setSaveConfigCallback([]()
                                     { configSaved = true; }); // restart on credentials save, ESP32 doesn't like to switch between AP/STA

   Serial.println("WiFi data initialized");
}

void wifi_sendData(void)
{
   // 1) Vorab: Wie viele Messwerte senden wir wirklich? (clamp auf measurements[8])
   size_t nItems = 0;
   size_t keyBytes = 0;

   for (size_t i = 0; i < sensorVector.size(); ++i)
   {
      const Sensor &s = sensorVector[i];
      int cap = arrlen(s.measurements); // == 8
      int nTake = s.returnCount;
      if (nTake < 0)
         nTake = 0;
      if (nTake > cap)
      {
         LOGW("sensor[%u]: returnCount=%d > cap=%d, clamping", (unsigned)i, s.returnCount, cap);
         nTake = cap;
      }
      for (int j = 0; j < nTake; ++j)
      {
         const Measurement &m = s.measurements[j];
         if (m.data_name.length() == 0)
            continue;
         if (isnan(m.value))
            continue;
         ++nItems;
         keyBytes += (size_t)m.data_name.length() + 1; // +1 minimaler Slop
      }
   }

   // 2) JSON-Dokument dimensionieren (Faustregel)
   const size_t capacity = JSON_OBJECT_SIZE(nItems) + keyBytes + nItems * 16;
   DynamicJsonDocument docMeasures(capacity);

   LOGD("build JSON: sensors=%u, items=%u, capacity=%u",
        (unsigned)sensorVector.size(), (unsigned)nItems, (unsigned)capacity);

   // 3) JSON füllen (wieder mit clamp)
   for (size_t i = 0; i < sensorVector.size(); ++i)
   {
      const Sensor &s = sensorVector[i];
      int cap = arrlen(s.measurements);
      int nTake = s.returnCount;
      if (nTake < 0)
         nTake = 0;
      if (nTake > cap)
         nTake = cap;

      for (int j = 0; j < nTake; ++j)
      {
         const Measurement &m = s.measurements[j];
         if (m.data_name.length() == 0)
         {
            LOGD("sensor[%u].m[%d]: empty name, skip", (unsigned)i, j);
            continue;
         }
         if (isnan(m.value))
         {
            LOGD("sensor[%u].m[%d]: NaN, skip", (unsigned)i, j);
            continue;
         }
         const float v = round2f(static_cast<float>(m.value)); // value ist double → float
         docMeasures[m.data_name] = v;
      }
   }

   if (docMeasures.overflowed())
   {
      LOGW("JSON overflowed (cap=%u). Ergebnis kann abgeschnitten sein!", (unsigned)capacity);
   }

   // 4) Serialisieren
   String output;
   output.reserve(capacity + 32);
   serializeJson(docMeasures, output);

   if (output.length() <= 2)
   { // "{}" == 2 Zeichen
      LOGI("kein sendbarer Inhalt (empty JSON)");
      return;
   }

   LOGD("payload bytes=%u", (unsigned)output.length());

   // 5) WiFi/TLS/HTTP – sauber & mit Logs
   if (WiFi.status() != WL_CONNECTED)
   {
      LOGW("WiFi disconnected, abort");
      return;
   }

   const String serverName = "https://kits.teleagriculture.org/api/kits/" + String(boardID) + "/measurements";
   const String api_Bearer = "Bearer " + API_KEY;

   // Token nicht im Klartext loggen – nur die letzten 6 Zeichen
   String api_tail = (API_KEY.length() > 6) ? API_KEY.substring(API_KEY.length() - 6) : API_KEY;
   LOGI("POST %s (items=%u), token[..%s]", serverName.c_str(), (unsigned)nItems, api_tail.c_str());

   WiFiClientSecure client;
   client.setCACertBundle(rootca_bundle_crt_start); // erwartet, dass dein Bundle eingebunden ist

   HTTPClient https;
   // begin() liefert bool – prüfen
   if (!https.begin(client, serverName))
   {
      LOGE("HTTPS begin() failed");
      return;
   }

   https.addHeader("Content-Type", "application/json");
   https.addHeader("Authorization", api_Bearer);
   // Optional: Timeout etwas strenger
   https.setTimeout(5000);

   const int httpCode = https.POST((uint8_t *)output.c_str(), output.length());
   if (httpCode <= 0)
   {
      LOGE("HTTP POST error: %d", httpCode);
      https.end();
      return;
   }

   LOGI("HTTP %d", httpCode);
   https.end();
   DynamicJsonDocument deallocate(docMeasures);
}

void connectIfWifi()
{
   if (upload != "WIFI")
      return;

   if (useEnterpriseWPA)
   {
      esp_wifi_sta_wpa2_ent_set_ca_cert((uint8_t *)user_CA.c_str(), strlen(user_CA.c_str()) + 1);
      esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)anonym.c_str(), strlen(anonym.c_str()));
   }

   setUPWiFi();
   startBlinking();

   // set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
   wifiManager.setAPCallback(configModeCallback);

   bool success = wifiManager.autoConnect("TeleAgriCulture Board", "enter123");

   if (!success)
   {
      Serial.println("Failed to connect");
      ESP.restart();
   }
   else
   {
      Serial.println("Connected");

      if (upload == "WIFI")
         sendDataWifi = true;

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
      else if (!useNTP && !rtcEnabled)
      {
         String header = get_header();

         if (!setEsp32TimeFromHeader(header, timeZone))
         {
            Serial.println("Failed to set time");
         }
      }
   }

   stopBlinking();
}

void setupLoRaIfNeeded()
{
   if (upload != "LORA")
      return;

   // WiFiManagerNS::TZ::loadPrefs();

   if (rtcEnabled == true)
      setESP32timeFromRTC(timeZone.c_str());

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

void setupSDCardIfNeeded()
{
   if (!saveDataSDCard)
      return;

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
   Serial.println(cardType == CARD_MMC ? "MMC" : cardType == CARD_SD ? "SDSC"
                                             : cardType == CARD_SDHC ? "SDHC"
                                                                     : "UNKNOWN");

   Serial.printf("SD Card Size: %lluMB\n", SD.cardSize() / (1024 * 1024));

   SD.end();

   saveDataSDCard = true;
   digitalWrite(SW_3V3, LOW);
}

void setupWebpageIfNeeded()
{
   if (!webpage || forceConfig)
      return;

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

void setupMQTTIfNeeded()
{
   if (upload != "LIVE" || live_mode != "MQTT" || forceConfig)
      return;

   setUPWiFi();

   wifiManager.setAPCallback(configModeCallback);
   bool success = wifiManager.autoConnect("TeleAgriCulture Board", "enter123");
   if (!success)
   {
      Serial.println("Failed to connect");
      ESP.restart();
   }

   Serial.println("WiFi connected");

   if (live_mode == "MQTT")
      sendDataMQTT = true;

   if ((WiFi.status() == WL_CONNECTED) && useNTP)
   {
      WiFiManagerNS::configTime();
      WiFiManagerNS::NTP::onTimeAvailable(&on_time_available);
   }
   else if (!useNTP && !rtcEnabled)
   {
      String header = get_header();
      if (!setEsp32TimeFromHeader(header, timeZone))
      {
         Serial.println("Failed to set time");
      }
   }

   selectMqttNetClient();

#if DEBUG_PRINT
   Serial.print("Network is ok: ");
   Serial.println(mqttPreflightCheck(mqtt_server_ip, mqtt_port));
#endif

   setMqttServerFromString(mqtt_server_ip, mqtt_port);

   mqtt.setKeepAlive(30);
   mqtt.setBufferSize(2048);

   // NEW: actually connect now
   mqttConnectNow();

   Serial.println("MQTT Setup done...");
}

void setupOSCIfNeeded()
{
   if (upload != "LIVE" || live_mode != "OSC" || forceConfig)
      return;

   setUPWiFi();
   wifiManager.setAPCallback(configModeCallback);
   if (!wifiManager.autoConnect("TeleAgriCulture Board", "enter123"))
   {
      Serial.println("Failed to connect");
      ESP.restart();
   }
   Serial.println("WiFi connected");

   sendDataOSC = true;

   if ((WiFi.status() == WL_CONNECTED) && useNTP)
   {
      WiFiManagerNS::configTime();
      WiFiManagerNS::NTP::onTimeAvailable(&on_time_available);
   }
   else if (!useNTP && !rtcEnabled)
   {
      String header = get_header();
      if (!setEsp32TimeFromHeader(header, timeZone))
      {
         Serial.println("Failed to set time");
      }
   }

   if (!oscConnectNow())
   {
      Serial.println("OSC: initial resolve failed (will retry).");
   }

   Serial.println("OSC Setup done...");
}

void updateTimeHandle()
{
   // --- time & day change handling ---
   time_t rawtime;
   time(&rawtime);
   localtime_r(&rawtime, &timeInfo);
   currentDay = timeInfo.tm_mday;

   if ((currentDay != lastDay) && (upload == "WIFI") && !WiFiManagerNS::NTPEnabled && !freshBoot)
   {
      String header = get_header();
      if (!setEsp32TimeFromHeader(header, timeZone))
      {
         Serial.println("Failed to set time");
      }
      lastDay = currentDay;
   }

   const unsigned long now = millis();

   // --- interval-based upload (default mode) ---
   if (now - previousMillis_upload >= (upload_interval * mS_TO_MIN_FACTOR))
   {
      if (upload == "NO")
      {
         no_upload = true;
      }
      if (upload == "WIFI")
      {
         sendDataWifi = true;
      }
      if (upload == "LORA" && !useBattery && loraJoined)
      {
         sendDataLoRa = true;
      }

      if (!instant_upload)
      { // instant handles LIVE separately
         if (live_mode == "MQTT")
            sendDataMQTT = true;
         if (live_mode == "OSC")
            sendDataOSC = true;
      }

      if (saveDataSDCard)
      {
         useSDCard = true;
      }

      previousMillis_upload = now;
   }

   // --- instant upload (only when new data available) ---
   if (instant_upload && newSensorDataAvailable)
   {
      static unsigned long lastInstantPush = 0;
      const uint32_t minGap = 100; // ms, prevents flooding if sensors fire too fast

      if (now - lastInstantPush >= minGap)
      {
         if (live_mode == "MQTT")
            sendDataMQTT = true;
         if (live_mode == "OSC")
            sendDataOSC = true;

         lastInstantPush = now;
         newSensorDataAvailable = false; // reset flag once handled
      }
   }

   // --- periodic UI/power tasks ---
   if (now - previousMillis >= interval) // dim TFT after 1 min
   {
      backlight_pwm = 5;
      previousMillis = now;

      if (useBattery)
      {
         if (upload == "WIFI")
            gotoSleep = true;
         if (upload == "LORA")
            gotoSleep = true;
         if (useSDCard)
            gotoSleep = true;
      }
   }

   if (now - previousMillis_long >= interval2) // turn off TFT after 5 min
   {
      backlight_pwm = 0;
      tft->fillScreen(ST7735_BLACK);
      previousMillis_long = now;
      displayRefresh = true;
      tft->enableSleep(true);
   }
}

void handleButtonPresses()
{
   // navigate with Down-Button
   if (downButton.pressed())
   {
      currentPage = (currentPage + 1) % num_pages;
      backlight_pwm = 250;
      tft->enableSleep(false);
      gotoSleep = false;
   }

   // navigate with Up-Button
   if (upButton.pressed())
   {
      currentPage = (currentPage - 1 + num_pages) % num_pages;
      backlight_pwm = 250;
      tft->enableSleep(false);
      upButtonsMillis = millis(); // start time stored
      gotoSleep = false;
   }
}

void configButtonTask(void *parameter)
{
   unsigned long pressStart = 0;
   bool pressed = false;

   for (;;)
   {
      // upButton pressed?
      if (upButton.read() == Button::PRESSED)
      {
         if (!pressed)
         {
            // first edge -> remember start time
            pressed = true;
            pressStart = millis();
         }
         // already pressed for more than 4s?
         if (pressed && (millis() - pressStart > 4000))
         {
            Serial.println("Long press detected -> Starting ConfigPortal");
            configPortal_run = true;

            startBlinking();

            if (webpage)
            {
               server.close();
            }
            if (upload != "WIFI")
            {
               setUPWiFi();
            }

            // stopping Button Task Watchdog
            esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0));
            esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(1));
            wifiManager.setCleanConnect(true);
            // start configuration portal
            if (!wifiManager.startConfigPortal("TeleAgriCulture Board", "enter123"))
            {
               Serial.println("failed to connect and hit timeout");
               delay(3000);
               ESP.restart();
               delay(5000);
            }
         }
      }
      else
      {
         // upButton released -> reset
         pressed = false;
         pressStart = 0;
      }

      vTaskDelay(50 / portTICK_PERIOD_MS); // check every 50ms
   }
}

void updateDisplay()
{
   if (useDisplay && (!useBattery || !gotoSleep))
   {
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
   }
}

void handleLoraLoop()
{
   os_runloop_once(); // Run the LoRaWAN OS run loop

   if (sendDataLoRa) // If it's time to send lora data
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
         saveLMICToRTC(TX_INTERVAL);
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

         delay(100);

         esp_deep_sleep_start(); // Enter deep sleep mode
      }
   }
}

void handleWiFiLoop()
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

void handleSDLoop()
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

      delay(100);

      esp_deep_sleep_start(); // Enter deep sleep mode
   }
}

void handleMQTTLoop()
{
   mqttLoop();

   if (sendDataMQTT)
   {
      sensorRead();
      publishLiveTopics();
      sendDataMQTT = false;
   }
}

void handleOSCLoop()
{
   oscLoop();

   if (sendDataOSC)
   {
      sensorRead();
      publishLiveOsc();
      sendDataOSC = false;
   }
}

// callback function, fired when NTP gets updated.
// Used to print the updated time or adjust an external RTC module.
void on_time_available(struct timeval *t)
{
   const char *tz = timeZone.c_str();
   setenv("TZ", tz, 1);
   tzset();

   Serial.println("NTP time updated");

   if (rtcEnabled && rtc.begin())
   {
      rtc.adjust(DateTime(t->tv_sec));
      Serial.println("RTC updated from NTP");
   }

   setESP32timeFromRTC(timeZone.c_str()); // Set the time on ESP32 using the RTC module

#if DEBUG_PRINT
   printLocalTime();
   printRTCInfo();
#endif
}

// callback function, fired when WiFiManager is in config mode
// Used to change UI on display
void configModeCallback(WiFiManager *myWiFiManager)
{
#if DEBUG_PRINT
   Serial.println("Entered Conf Mode");
   Serial.print("Config SSID: ");
   Serial.println(myWiFiManager->getConfigPortalSSID());
   Serial.print("Config IP Address: ");
   Serial.println(WiFi.softAPIP());
#endif

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

void onMqttMessage(char *topic, byte *payload, unsigned int len)
{
   // handle commands if you subscribe later
}
