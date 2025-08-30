#include <Arduino.h>
#include <ui_functions.h>
#include <init_Board.h>
#include <time_functions.h>
#include <lora_functions.h>
#include <board_credentials.h>
#include <Fonts/FreeSans9pt7b.h>


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

void initDisplayIfNeeded()
{
   if (useDisplay || forceConfig)
   {
      initDisplay();
   }
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
      if (I2C_con_table[i].sensorIndex == -1)
      {
         tft->setTextColor(ST7735_RED);
         tft->print("NO");
      }
      else
      {
         tft->setTextColor(ST7735_GREEN);
         tft->print(allSensors[I2C_con_table[i].sensorIndex].sensor_name);
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
   if (I2C_5V_con_table[0].sensorIndex == -1)
   {
      tft->setTextColor(ST7735_RED);
      tft->print("NO");
   }
   else
   {
      tft->setTextColor(ST7735_GREEN);
      tft->print(allSensors[I2C_5V_con_table[0].sensorIndex].sensor_name);
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

int countMeasurements(const std::vector<Sensor> &sensors)
{
   int count = 0;
   for (const auto &sensor : sensors)
   {
      count += sensor.returnCount;
   }
   return count;
}

void measurementsPage(int page)
{
   tft->fillScreen(ST7735_BLACK);
   tft->setFont();
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

      // handle °C and kΩ separately
      if (show_measurements[i].unit == "°C")
      {
         // draw ° symbol
         tft->drawChar(tft->getCursorX(), tft->getCursorY(), 0xF8, ST7735_YELLOW, ST7735_BLACK, 1);
         tft->setCursor(tft->getCursorX() + 7, tft->getCursorY());
         tft->print("C");
      }
      else if (show_measurements[i].unit == "kΩ")
      {
         tft->print("k");
         // draw Ω symbol
         tft->drawChar(tft->getCursorX(), tft->getCursorY(), 0xEA, ST7735_YELLOW, ST7735_BLACK, 1);
      }
      else
      {
         // all other units normal
         tft->print(show_measurements[i].unit);
      }

      cursor_y += 10;
   }
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

void printDigits(int digits)
{
   // utility for digital clock display: prints preceding colon and leading 0
   tft->print(":");
   if (digits < 10)
      tft->print('0');
   tft->print(digits);
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
