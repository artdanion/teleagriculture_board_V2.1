#include <Arduino.h>
#include <time_functions.h>
#include <init_Board.h>
#include <def_Board.h>

#include <driver/rtc_io.h>
#include "driver/timer.h"
#include <sys/time.h>
#include <time.h>
#include <esp_sntp.h>
#include <RTClib.h>

#include <WiFiClientSecure.h>

time_t prevDisplay = 0;

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

RTC_DS3231 rtc;

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

String get_header()
{
   WiFiClientSecure client;
   String header = "";
   String dateLine = "NO DATE";

   client.setCACertBundle(rootca_bundle_crt_start); // falls Zertifikat-Bundle vorhanden
   // client.setInsecure(); // nur zum Testen, wenn das Zertifikat-Bundle abgelaufen ist

   Serial.println("[HTTPS] Connecting...");
   if (!client.connect(GET_Time_SERVER, SSL_PORT))
   {
      Serial.println("[HTTPS] Connection failed");
      return "NO CONNECTION";
   }

   // HTTP GET Request senden
   client.println("GET " + GET_Time_Address + " HTTP/1.1");
   client.print("Host: ");
   client.println(GET_Time_SERVER);
   client.println("Connection: close");
   client.println();

   unsigned long timeNow = millis();
   bool headerComplete = false;

   while (millis() - timeNow < TIMEOUT)
   {
      while (client.available())
      {
         String line = client.readStringUntil('\n');
         line.trim(); // \r\n -> \n

         if (line.length() == 0)
         {
            headerComplete = true; // leere Zeile => Header zu Ende
            break;
         }

         header += line + "\n";

         // Date-Zeile extrahieren
         if (line.startsWith("Date: "))
         {
            dateLine = line.substring(6); // alles nach "Date: "
         }
      }

      if (headerComplete)
         break;
   }

   client.stop();
   
#if DEBUG_PRINT
   Serial.println("[HTTPS] Received header:");
   Serial.println(header);

   Serial.println("Date from header: " + dateLine);
#endif
   return dateLine;
}

// winter: UTC+1, summer: UTC+2
// String timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";
bool setEsp32TimeFromHeader(String dateTimeStr, String timeZone)
{
   dateTimeStr.trim();
   Serial.print("Date from header: ");
   Serial.println(dateTimeStr);

   char wkday[4], month[4], tz[4];
   int day, year, hour, minute, second;

   int parsed = sscanf(dateTimeStr.c_str(),
                       "%3s, %d %3s %d %d:%d:%d %3s",
                       wkday, &day, month, &year,
                       &hour, &minute, &second, tz);

   if (parsed < 8)
   {
      Serial.println("sscanf failed");
      return false;
   }
   int mon = monthStrToInt(month);
   if (mon < 0)
   {
      Serial.println("Invalid month");
      return false;
   }

   struct tm t = {0};
   t.tm_year = year - 1900;
   t.tm_mon = mon;
   t.tm_mday = day;
   t.tm_hour = hour;
   t.tm_min = minute;
   t.tm_sec = second;

   // first as UTC
   setenv("TZ", "GMT0", 1);
   tzset();

   time_t epoch = mktime(&t);
   if (epoch == -1)
   {
      Serial.println("mktime failed");
      return false;
   }

   struct timeval now = {.tv_sec = epoch};
   settimeofday(&now, NULL);

   // than local TimeZone set
   setenv("TZ", timeZone.c_str(), 1);
   tzset();

#if DEBUG_PRINT
   struct tm utcTime;
   gmtime_r(&epoch, &utcTime);
   Serial.printf("UTC time set to: %s", asctime(&utcTime));

   struct tm localTime;
   getLocalTime(&localTime);
   Serial.printf("Local time adjusted to: %s", asctime(&localTime));
#endif
   return true;
}

void setRTCfromConfigPortal(const String &setTime_value, const String &timeZone)
{
   Serial.println("Setting RTC from ConfigPortal time string: " + setTime_value);

   struct tm t = {};
   int parsed = 0;

   // Try full format with seconds
   parsed = sscanf(setTime_value.c_str(), "%d-%d-%d %d:%d:%d",
                   &t.tm_year, &t.tm_mon, &t.tm_mday,
                   &t.tm_hour, &t.tm_min, &t.tm_sec);

   // Try ISO-like without seconds
   if (parsed < 6)
   {
      t.tm_sec = 0;
      parsed = sscanf(setTime_value.c_str(), "%d-%d-%dT%d:%d",
                      &t.tm_year, &t.tm_mon, &t.tm_mday,
                      &t.tm_hour, &t.tm_min);
   }

   if (parsed >= 5)
   {
      t.tm_year -= 1900;
      t.tm_mon -= 1;

      // Tell mktime() to interpret t as **local time**
      setenv("TZ", timeZone.c_str(), 1);
      tzset();
      time_t localEpoch = mktime(&t);

      // Convert that localEpoch to UTC
      struct tm gm;
      gmtime_r(&localEpoch, &gm);
      time_t utcEpoch = mktime(&gm);

      // Apply UTC to system clock first
      struct timeval tv = {.tv_sec = utcEpoch, .tv_usec = 0};
      settimeofday(&tv, NULL);

      // Reapply TZ so getLocalTime() etc. return local
      setenv("TZ", timeZone.c_str(), 1);
      tzset();

      Serial.println("ESP32 system time updated from ConfigPortal");

      if (rtc.begin())
      {
         rtc.adjust(DateTime(utcEpoch)); // store UTC in RTC
         Serial.println("RTC updated with UTC from ConfigPortal");
      }

      // Debug output
      struct tm localNow;
      getLocalTime(&localNow);
      Serial.printf("Local system time: %s", asctime(&localNow));

      struct tm utcNow;
      gmtime_r(&utcEpoch, &utcNow);
      Serial.printf("UTC stored in RTC: %s", asctime(&utcNow));
   }
   else
   {
      Serial.println("Failed to parse setTime_value string!");
   }
}

void setESP32timeFromRTC(const char *tz)
{

   if (!rtc.begin())
   {
      Serial.println("RTC not found!");
      return;
   }

   if (rtc.lostPower())
   {
      Serial.println("RTC lost power, time not valid");
      return;
   }

   DateTime now = rtc.now();
   struct tm t;
   t.tm_year = now.year() - 1900;
   t.tm_mon = now.month() - 1;
   t.tm_mday = now.day();
   t.tm_hour = now.hour();
   t.tm_min = now.minute();
   t.tm_sec = now.second();

   // first as UTC

   setenv("TZ", "GMT0", 1);
   tzset();

   time_t epoch = mktime(&t);
   if (epoch == -1)
   {
      Serial.println("mktime failed");
      return;
   }
   
   struct timeval tv = {.tv_sec = epoch};
   settimeofday(&tv, NULL);

      setenv("TZ", tz, 1);
   tzset();

   Serial.println("ESP32 time restored from RTC");
   printLocalTime();
}

void printRTCInfo()
{
   if (rtc.begin())
   {
      DateTime now = rtc.now();
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
   }
   else
   {
      Serial.println("RTC not found!");
   }
}

void printLocalTime()
{
   struct tm timeInfo;
   if (!getLocalTime(&timeInfo))
   {
      Serial.println("Failed to obtain time");
      return;
   }

   char buffer[64];
   strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S %Z", &timeInfo);
   Serial.print("Local time: ");
   Serial.println(buffer);
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

int monthStrToInt(const String &month)
{
   if (month == "Jan")
      return 0;
   if (month == "Feb")
      return 1;
   if (month == "Mar")
      return 2;
   if (month == "Apr")
      return 3;
   if (month == "May")
      return 4;
   if (month == "Jun")
      return 5;
   if (month == "Jul")
      return 6;
   if (month == "Aug")
      return 7;
   if (month == "Sep")
      return 8;
   if (month == "Oct")
      return 9;
   if (month == "Nov")
      return 10;
   if (month == "Dec")
      return 11;
   return -1;
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

time_t convertDateTime(String dateTimeStr)
{
   struct tm tm;
   memset(&tm, 0, sizeof(struct tm));
   strptime(dateTimeStr.c_str(), "%a, %d %b %Y %H:%M:%S %Z", &tm);
   time_t t = mktime(&tm);
   return t;
}
