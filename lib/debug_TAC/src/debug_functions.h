#pragma once

#include <Arduino.h>
#include <init_Board.h>


/*
 * Globale Log-Stufe per build_flags:
 *   -DTAC_LOG_LEVEL=4     // 0..5 (0=aus, 1=E, 2=W, 3=I, 4=D, 5=V)
 *
 * Optional per file:
 *   #define TAC_LOG_LEVEL_LOCAL 3
 *   #define LOG_TAG "MAIN"
 *   #include <debug_functions.h>
 */

#ifndef TAC_LOG_LEVEL
#define TAC_LOG_LEVEL 4
#endif

#ifndef TAC_LOG_LEVEL_LOCAL
#define TAC_LOG_LEVEL_LOCAL TAC_LOG_LEVEL
#endif

#ifndef LOG_TAG
#define LOG_TAG "APP"
#endif


inline void _tac_log_prefix(const char* lvl, const char* tag) {
  // with timestamp: Serial.printf("[%10lu %s/%s] ", millis(), lvl, tag);
  Serial.printf("[%s/%s] ", lvl, tag);
}

#if TAC_LOG_LEVEL_LOCAL >= 5
  #define LOGV(fmt, ...) do { _tac_log_prefix("V", LOG_TAG); Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } while (0)
#else
  #define LOGV(...)      do {} while (0)
#endif

#if TAC_LOG_LEVEL_LOCAL >= 4
  #define LOGD(fmt, ...) do { _tac_log_prefix("D", LOG_TAG); Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } while (0)
#else
  #define LOGD(...)      do {} while (0)
#endif

#if TAC_LOG_LEVEL_LOCAL >= 3
  #define LOGI(fmt, ...) do { _tac_log_prefix("I", LOG_TAG); Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } while (0)
#else
  #define LOGI(...)      do {} while (0)
#endif

#if TAC_LOG_LEVEL_LOCAL >= 2
  #define LOGW(fmt, ...) do { _tac_log_prefix("W", LOG_TAG); Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } while (0)
#else
  #define LOGW(...)      do {} while (0)
#endif

#if TAC_LOG_LEVEL_LOCAL >= 1
  #define LOGE(fmt, ...) do { _tac_log_prefix("E", LOG_TAG); Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); } while (0)
#else
  #define LOGE(...)      do {} while (0)
#endif

// ---- Kompatibilität zu vorhandenen Makros ----
#ifndef DBG
  #define DBG(fmt, ...)   LOGD(fmt, ##__VA_ARGS__)
#endif
#ifndef DBGLN
  #define DBGLN(fmt, ...) LOGD(fmt, ##__VA_ARGS__)
#endif
#ifndef WDBG
  #define WDBG(fmt, ...)  LOGD(fmt, ##__VA_ARGS__)
#endif
#ifndef WDBGLN
  #define WDBGLN(fmt, ...) LOGD(fmt, ##__VA_ARGS__)
#endif


// C-Array-lenght at compiletime
template <typename T, size_t N>
constexpr int arrlen(const T (&)[N]) { return static_cast<int>(N); }

// (float/double overloads)
static inline float  round2f(float  x) { return roundf(x * 100.0f) / 100.0f; }
static inline double round2f(double x) { return ::round(x * 100.0) / 100.0; }

inline void LOG_HEX(const void* data, size_t len) {
  const uint8_t* p = static_cast<const uint8_t*>(data);
  for (size_t i = 0; i < len; ++i) {
    if (i && (i % 16 == 0)) Serial.println();
    if (p[i] < 16) Serial.print('0');
    Serial.print(p[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
}


// debug fuctions
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void checkLoadedStuff(void);
void printConnectors(ConnectorType typ);
void printProtoSensors(void);
void printMeassurments(void);
void printSensors(void);
void printHex2(unsigned v);
void scanI2CDevices(void);