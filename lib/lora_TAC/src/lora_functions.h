#pragma once

#include <Arduino.h>
#include <init_Board.h>
#include <def_Board.h>

void os_getArtEui(u1_t *buf);
void os_getDevEui(u1_t *buf);
void os_getDevKey(u1_t *buf);

void lora_sendData(void);

void convertTo_LSB_EUI(String input, uint8_t *output);
void convertTo_MSB_APPKEY(String input, uint8_t *output);

void onEvent(ev_t ev);
void do_send(LoraMessage &message);
void LoraWANPrintLMICOpmode(void);
void saveLMICToRTC(int deepsleep_sec);
void loadLMICFromRTC();

// Function to store LMIC configuration to RTC Memory
void saveLORA_State(void);
// Function to reload LMIC configuration from RTC Memory
void loadLORA_State();