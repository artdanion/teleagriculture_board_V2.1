#pragma once

#include <Arduino.h>
#include <init_Board.h>
#include <def_Board.h>

#include <driver/rtc_io.h>
#include <lmic.h>
#include <hal/hal.h>
#include <LoraMessage.h>

#define UNUSED_PIN 0xFF
#define LORA_PIN_RXTX UNUSED_PIN

// ------------------- RTC Persistente Variablen -------------------
extern RTC_DATA_ATTR int bootCount;
extern RTC_DATA_ATTR bool loraJoined;

extern RTC_DATA_ATTR u4_t RTC_LORAWAN_netid;
extern RTC_DATA_ATTR devaddr_t RTC_LORAWAN_devaddr;
extern RTC_DATA_ATTR u1_t RTC_LORAWAN_nwkKey[16];
extern RTC_DATA_ATTR u1_t RTC_LORAWAN_artKey[16];
extern RTC_DATA_ATTR u4_t RTC_LORAWAN_seqnoUp;
extern RTC_DATA_ATTR u4_t RTC_LORAWAN_seqnoDn;
extern RTC_DATA_ATTR u1_t RTC_LORAWAN_dn2Dr;
extern RTC_DATA_ATTR u1_t RTC_LORAWAN_dnConf;
extern RTC_DATA_ATTR s1_t RTC_LORAWAN_adrTxPow;
extern RTC_DATA_ATTR u1_t RTC_LORAWAN_txChnl;
extern RTC_DATA_ATTR s1_t RTC_LORAWAN_datarate;
extern RTC_DATA_ATTR u2_t RTC_LORAWAN_channelMap;
extern RTC_DATA_ATTR s2_t RTC_LORAWAN_adrAckReq;
extern RTC_DATA_ATTR u1_t RTC_LORAWAN_rx1DrOffset;
extern RTC_DATA_ATTR u1_t RTC_LORAWAN_rxDelay;

#if (CFG_eu868)
extern RTC_DATA_ATTR u4_t RTC_LORAWAN_channelFreq[MAX_CHANNELS];
extern RTC_DATA_ATTR u2_t RTC_LORAWAN_channelDrMap[MAX_CHANNELS];
extern RTC_DATA_ATTR u4_t RTC_LORAWAN_channelDlFreq[MAX_CHANNELS];
extern RTC_DATA_ATTR band_t RTC_LORAWAN_bands[MAX_BANDS];
#endif

extern RTC_DATA_ATTR lmic_t RTC_LMIC;
extern const unsigned TX_INTERVAL;
extern bool loraJoinFailed;
extern bool loraDataTransmitted;

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