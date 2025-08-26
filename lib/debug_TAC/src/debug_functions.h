#pragma once

#include <Arduino.h>
#include <init_Board.h>

// debug fuctions
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void checkLoadedStuff(void);
void printConnectors(ConnectorType typ);
void printProtoSensors(void);
void printMeassurments(void);
void printSensors(void);
void printHex2(unsigned v);
void scanI2CDevices(void);