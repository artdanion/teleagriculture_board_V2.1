#include "osc_support.h"
#include <WiFi.h>
#include <ArduinoOSCWiFi.h>
#include "board_credentials.h"
#include "def_Sensors.h"
#include "init_Board.h"
#include "debug_functions.h"
#include <vector>

#define LOG_TAG "OSC"
// ---- externs from your project ----
extern int boardID;

// Consider declaring these in board_credentials.h and defining them ONCE in a .cpp
extern String osc_ip; // e.g. "192.168.1.200" or hostname
extern int osc_port;  // e.g. 5000
extern bool sendDataOSC;
extern String osc_topic; // e.g. "/TAC/"

// ---- module state ----
static unsigned long s_retryAt = 0;
static uint8_t s_attempts = 0;
static bool s_destReady = false; // result of last resolve

// ---- helpers ----
static inline float round2(float x) { return roundf(x * 100.0f) / 100.0f; }

static bool resolveDest(IPAddress &ipOut, const String &host)
{
  if (host.isEmpty())
    return false;
  if (ipOut.fromString(host.c_str()))
    return true; // dotted quad
  if (WiFi.status() != WL_CONNECTED)
    return false;
  int ok = WiFi.hostByName(host.c_str(), ipOut);
  return ok == 1;
}

static String makeOscBaseTopic()
{
  // Build "/<osc_topic>/<boardID>/"
  String base = osc_topic;
  if (base.isEmpty())
    base = "/";
  if (base[0] != '/')
    base = "/" + base;
  if (base[base.length() - 1] != '/')
    base += '/';
  base += String(boardID);
  base += '/';
  return base;
}

static bool ensureOscDestination(bool immediate)
{
  if (osc_port <= 0 || osc_port > 65535)
  {
    LOGI("[OSC] invalid port: %d", osc_port);
    return false;
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    LOGI("[OSC] WiFi not connected");
    return false;
  }
  if (immediate)
  {
    IPAddress ip;
    s_destReady = resolveDest(ip, osc_ip);
    LOGI("[OSC] resolve (immediate) %s -> %s", osc_ip.c_str(), s_destReady ? ip.toString().c_str() : "FAIL");
    return s_destReady;
  }

  unsigned long now = millis();
  if (now < s_retryAt)
    return s_destReady;

  IPAddress ip;
  s_destReady = resolveDest(ip, osc_ip);
  if (!s_destReady)
  {
    s_attempts++;
    const unsigned long delayMs = 1000UL * min<unsigned long>(30, (1UL << min<uint8_t>(s_attempts, 5)));
    s_retryAt = now + delayMs;
    LOGI("[OSC] resolve fail (%s), retry in %lu ms", osc_ip.c_str(), delayMs);
  }
  else
  {
    s_attempts = 0;
    s_retryAt = now + 5000;
    LOGI("[OSC] resolve ok: %s -> %s", osc_ip.c_str(), ip.toString().c_str());
  }
  return s_destReady;
}

// Optional inbound/update loop. Keep disabled unless you actually subscribe.
void oscLoop()
{
  // LOGI("[OSC] update()");
  // OscWiFi.update();
}

// ---- publishing ----
static bool sendOneFloat(const char *addr, float v)
{
  if (!addr || !*addr)
  {
    LOGE("[OSC] skip: empty address");
    return false;
  }
  if (!isfinite(v))
  {
    LOGE("[OSC] skip: NaN/Inf at %s", addr);
    return false;
  }

  LOGD("[OSC] -> %s : %.2f", addr, v);

  OscWiFi.send(osc_ip.c_str(), osc_port, addr, v);

  LOGD("sent");
  return true;
}

void publishSensorVectorAsOSC(const String &baseAddress)
{
  if (baseAddress.isEmpty()) {
    LOGE("[OSC] baseAddress empty, abort");
    return;
  }

  // Count how many values we intend to send (sum of clamped returnCounts)
  size_t totalMeas = 0;
  for (const auto &s : sensorVector) {
    const int cap   = arrlen(s.measurements);           // == 8
    int nTake       = s.returnCount;
    if (nTake < 0)  nTake = 0;
    if (nTake > cap) nTake = cap;
    totalMeas += static_cast<size_t>(nTake);
  }

  LOGI("[OSC] publish start: sensors=%u, totalMeasurements=%u, base='%s'",
        (unsigned)sensorVector.size(), (unsigned)totalMeas, baseAddress.c_str());

  // Send values
  for (size_t i = 0; i < sensorVector.size(); ++i)
  {
    const Sensor &s = sensorVector[i];

    const int cap = arrlen(s.measurements); // 8
    int nTake     = s.returnCount;
    if (nTake < 0)  nTake = 0;
    if (nTake > cap) {
      LOGI("[OSC] sensor[%u]: returnCount=%d > cap=%d, clamping",
            (unsigned)i, s.returnCount, cap);
      nTake = cap;
    }

    if (nTake == 0) {
      LOGE("[OSC] sensor[%u]: no values to send", (unsigned)i);
      continue;
    }

    for (int j = 0; j < nTake; ++j)
    {
      const Measurement &m = s.measurements[j];

      if (m.data_name.isEmpty()) {
        LOGE("[OSC] sensor[%u].m[%d]: empty name, skip", (unsigned)i, j);
        continue;
      }
      if (isnan(m.value)) {
        LOGE("[OSC] sensor[%u].m[%d]: value NaN, skip", (unsigned)i, j);
        continue;
      }

      String addr = baseAddress; // e.g. "/TAC/<boardID>/"
      addr += m.data_name;

      // measurement.value is double; round to 2 decimals and send as float
      const float v = round2(static_cast<float>(m.value));
      sendOneFloat(addr.c_str(), v);
    }
  }
  LOGI("[OSC] publish done");
}

void publishLiveOsc()
{
  if (!sendDataOSC)
  {
    LOGI("[OSC] sendDataOSC=false, skip");
    return;
  }
  if (!ensureOscDestination(false))
  {
    LOGI("[OSC] destination not ready (WiFi/DNS), skip");
    return;
  }
  const String base = makeOscBaseTopic();
  LOGI("[OSC] begin publish base='%s' host='%s' port=%d",
        base.c_str(), osc_ip.c_str(), osc_port);
  publishSensorVectorAsOSC(base);
}

bool oscConnectNow()
{
  if (osc_port <= 0 || osc_port > 65535)
  {
#if OSC_DBG
    Serial.printf("[OSC] invalid port: %d\n", osc_port);
#endif
    return false;
  }
  if (WiFi.status() != WL_CONNECTED)
  {
#if OSC_DBG
    Serial.println("[OSC] WiFi not connected");
#endif
    return false;
  }

  IPAddress ip;
  bool ok = ip.fromString(osc_ip.c_str()) ||
            (WiFi.hostByName(osc_ip.c_str(), ip) == 1);

#if OSC_DBG
  Serial.printf("[OSC] connectNow %s -> %s\n",
                osc_ip.c_str(), ok ? ip.toString().c_str() : "FAIL");
#endif
  return ok;
}