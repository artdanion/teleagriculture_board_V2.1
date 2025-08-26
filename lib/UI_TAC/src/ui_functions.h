#pragma once

#include <Arduino.h>
#include <vector>
#include <init_Board.h>
#include <board_credentials.h>

// UI functions
void renderPage(int page);
void mainPage(void);
void I2C_ConnectorPage(void);
void ADC_ConnectorPage(void);
void OneWire_ConnectorPage(void);
int countMeasurements(const std::vector<Sensor> &sensors);
void measurementsPage(int page);
void deepsleepPage();
void digitalClockDisplay(int x, int y, bool date);
void printDigits(int digits);
int seconds_to_next_hour();
void setUploadTime();
void drawBattery(int x, int y);
