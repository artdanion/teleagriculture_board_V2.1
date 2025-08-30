#pragma once

#include <Arduino.h>

#include <driver/rtc_io.h>
#include "driver/timer.h"
#include <sys/time.h>
#include <time.h>
#include <esp_sntp.h>
#include <RTClib.h>

extern RTC_DS3231 rtc;   // external RTC object

extern time_t prevDisplay;
extern struct tm timeInfo;
extern uint32_t userUTCTime;
extern String lastUpload;

extern int currentDay;
extern int lastDay;
extern unsigned long lastExecutionTime;
extern int seconds_to_wait;

extern unsigned long upButtonsMillis;
extern unsigned long previousMillis;
extern unsigned long previousMillis_long;
extern unsigned long previousMillis_upload;
extern const long interval;
extern const long interval2;

void setUploadTime();
String get_header();
bool setEsp32TimeFromHeader(String header, String timeZone);
void setRTCfromConfigPortal(const String& setTime_value, const String& timeZone);
void setESP32timeFromRTC(const char* tz);

void printRTCInfo();
void printLocalTime();

int seconds_to_next_hour();
int monthStrToInt(const String &month);
void get_time_in_timezone(const char *timezone);
time_t convertDateTime(String dateTimeStr);