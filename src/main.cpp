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


/*********************************** VERSION 1.20 ****************************
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

#define DEBUG_PRINT false // full debug print
#define DOUBLERESETDETECTOR_DEBUG true

// ----- Function declaration -----//
void openConfig(void);
void handleWakeupDisplay();

void setUPWiFi();
void wifi_sendData(void);

void connectIfWifi();
void setupLoRaIfNeeded();
void setupSDCardIfNeeded();
void setupWebpageIfNeeded();

void updateTimeHandle();
void handleButtonPresses();
void updateDisplay();

void handleLoraLoop();
void handleWiFiLoop();
void handleSDLoop();

// Callbacks
void configModeCallback(WiFiManager *myWiFiManager);
void on_time_available(struct timeval *t);

// Task
TaskHandle_t configTaskHandle;
void configButtonTask(void *parameter);

void setup()
{
   delay(1000);

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

   if (forceConfig)
      openConfig();

   if (upload == "NO_UPLOAD")
      no_upload = true;

   if (!useBattery)
      webpage = true;

   connectIfWifi();
   setupLoRaIfNeeded();
   setupSDCardIfNeeded();
   setupWebpageIfNeeded();
}

void loop()
{
   drd->loop(); // Process double reset detection

   analogWrite(TFT_BL, backlight_pwm); // Set the backlight brightness of the TFT display

   if (forceConfig)
      openConfig();

   updateTimeHandle(); // Update time and handle periodic tasks (display & upload)
   handleButtonPresses();

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
      handleLoraLoop();

   if (upload == "WIFI")
      handleWiFiLoop();

   if (saveDataSDCard)
      tft->drawRGBBitmap(145, 2, sdcard_icon, 14, 19);

   if (useSDCard)
      handleSDLoop();

   updateDisplay();

   if (webpage && !forceConfig)
      server.handleClient(); // Handle incoming client requests
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

void updateTimeHandle()
{
   time_t rawtime;
   time(&rawtime);
   localtime_r(&rawtime, &timeInfo); // Get the current local time

   currentDay = timeInfo.tm_mday; // Update the current day

   // Check if the day has changed and perform time synchronization if required
   if ((currentDay != lastDay) && (upload == "WIFI") && !(WiFiManagerNS::NTPEnabled) && !freshBoot)
   {
      // The day has changed since the last execution of this block
      String header = get_header();

      if (!setEsp32TimeFromHeader(header, timeZone))
      {
         Serial.println("Failed to set time");
      }
      lastDay = currentDay; // Update the last day value
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
                              // digitalWrite(TFT_BL, LOW);
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

            startBlinking();

            if (webpage)
            {
               server.close();
            }
            if (upload != "WIFI")
            {
               setUPWiFi();
            }

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
