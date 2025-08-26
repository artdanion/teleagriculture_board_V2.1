#include <Arduino.h>
#include <time_functions.h>
#include <init_Board.h>
#include <def_Board.h>
#include <time.h>
#include <RTClib.h>
#include <WiFiClientSecure.h>

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

   setenv("TZ", timeZone.c_str(), 1);
   tzset();

   getLocalTime(&timeInfo);
   Serial.printf("\nSetting time based on GET: %s", asctime(&timeInfo));

   currentDay = timeInfo.tm_mday;
   lastDay = currentDay;
}

void setEsp32TimeRTC()
{
   if (rtcEnabled)
   {
      RTC_DS3231 rtc;

      rtc.begin();
      DateTime now = rtc.now();
      if (now.year() < 2024)
      {
         Serial.println("RTC not set, set to compile time");
         rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      }
      Serial.print("Current RTC time: ");
      Serial.print(now.year());
      Serial.print("-");
      Serial.print(now.month());
      Serial.print("-");
      Serial.print(now.day());
      Serial.print(" ");
      Serial.print(now.hour());
      Serial.print(":");
      Serial.print(now.minute());
      Serial.print(":");
      Serial.print(now.second());
      Serial.println();

      struct tm timeinfo;
      timeinfo.tm_year = now.year() - 1900;
      timeinfo.tm_mon = now.month() - 1;
      timeinfo.tm_mday = now.day();
      timeinfo.tm_hour = now.hour();
      timeinfo.tm_min = now.minute();
      timeinfo.tm_sec = now.second();

      struct timeval tv;
      tv.tv_sec = mktime(&timeinfo);
      tv.tv_usec = 0;

      if (settimeofday(&tv, nullptr) == 0)
      {
         Serial.println("System time updated");
      }
      else
      {
         Serial.println("Failed to set system time");
      }
   }
}