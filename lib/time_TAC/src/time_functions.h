#pragma once

#include <Arduino.h>
#include <time.h>

void get_time_in_timezone(const char *timezone);
String get_header();
time_t convertDateTime(String dateTimeStr);
String getDateTime(String header);
void setEsp32Time(const char *timeStr);
void setEsp32TimeRTC();
