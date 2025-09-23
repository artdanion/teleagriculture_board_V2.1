// mqtt_support.cpp
#include "mqtt_support.h"
#include "board_credentials.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#include "init_Board.h"

extern const uint8_t rootca_bundle_crt_start[] asm("_binary_data_cert_x509_crt_bundle_bin_start");


WiFiClient        _mqttNet;
WiFiClientSecure  _mqttNetSecure;
PubSubClient      mqtt;

unsigned long _mqttRetryAt  = 0;
uint8_t       _mqttAttempts  = 0;

extern String mqtt_server_ip;    // z.B. "158.255.212.248" oder Domain
extern String mqtt_topic;
extern int mqtt_port;       // 1883, 8883, 443 ...
extern String live_mode;
extern bool sendDataMQTT;

// -----------------------------------------------------------------------------------

static const char* mqttStateStr(int s) {
  switch (s) {
    case -4: return "CONNECTION_TIMEOUT";
    case -3: return "CONNECTION_LOST";
    case -2: return "CONNECT_FAILED";
    case -1: return "DISCONNECTED";
    case  0: return "CONNECTED";
    case  1: return "BAD_PROTOCOL";
    case  2: return "BAD_CLIENT_ID";
    case  3: return "UNAVAILABLE";
    case  4: return "BAD_CREDENTIALS";
    case  5: return "UNAUTHORIZED";
    default: return "UNKNOWN";
  }
}

bool mqttUseTLS() {
  // TLS-Ports: 8883 – standard; 443 – selten raw MQTT, häufig WebSocket (PubSubClient kann KEIN WebSocket)
  return (mqtt_port == 8883 || mqtt_port == 443);
}

// String -> IPAddress (valide IPv4)
bool toIPAddress(const String& s, IPAddress& ip) {
  if (s.length() < 7 || s.length() > 15) return false; // grober Check "x.x.x.x"
  char buf[32];
  s.toCharArray(buf, sizeof(buf));
  uint8_t oct[4];
  char* save = nullptr;
  char* tok = strtok_r(buf, ".", &save);
  int   n   = 0;
  while (tok && n < 4) {
    long v = strtol(tok, nullptr, 10);
    if (v < 0 || v > 255) return false;
    oct[n++] = (uint8_t)v;
    tok = strtok_r(nullptr, ".", &save);
  }
  if (n != 4) return false;
  ip = IPAddress(oct[0], oct[1], oct[2], oct[3]);
  return true;
}

// Schneller TCP-Reachability-Check (erkennt geblockte Ports), TLS testet mit setInsecure()
bool tcpReachable(const char* host, uint16_t port, bool tls /*=false*/, uint16_t timeoutMs /*=2000*/) {
  if (tls) {
    WiFiClientSecure s;
    s.setTimeout(timeoutMs);
    s.setInsecure();            // nur für Erreichbarkeits-Test (kein CA-Check)
    bool ok = s.connect(host, port);
    s.stop();
    return ok;
  } else {
    WiFiClient s;
    s.setTimeout(timeoutMs);
    bool ok = s.connect(host, port);
    s.stop();
    return ok;
  }
}

void selectMqttNetClient() {
  if (mqttUseTLS()) {
    // Absicherung: entweder CA-Bundle setzen oder (für Tests) unsicher
    // _mqttNetSecure.setCACertBundle(rootca_bundle_crt_start);  // falls Bundle verlinkt
    _mqttNetSecure.setInsecure();                                // zum Starten/Debuggen
    mqtt.setClient(_mqttNetSecure);
  } else {
    mqtt.setClient(_mqttNet);
  }
}

String makeClientId() {
  String cid = "tac-";
  String mac = WiFi.macAddress();       // im STA-Modus passender als softAP
  mac.replace(":", "");
  cid += mac;
  return cid;
}

bool mqttConnectNow() {
  String cid = makeClientId();
  bool ok = mqtt.connect(cid.c_str()
                         /*, user, pass */
                         /*, willTopic, willQos, willRetain, willMessage */);
  if (!ok) {
    Serial.printf("[MQTT] connect failed, state=%d (%s)\n", mqtt.state(), mqttStateStr(mqtt.state()));
  } else {
    Serial.println("[MQTT] connected");
  }
  return ok;
}

bool ensureMqttConnected(bool immediate) {
  if (mqtt.connected()) return true;

  unsigned long now = millis();
  if (!immediate && now < _mqttRetryAt) return false;

  _mqttRetryAt = now + 3000UL; // 3s Backoff
  _mqttAttempts++;

  return mqttConnectNow();
}

void mqttLoop() {
  if (mqtt.connected()) mqtt.loop();
}

// Publisht alle Messwerte als einzelne Topics
void publishLiveTopics() {
  if (!mqtt.connected()) {
    Serial.println("MQTT not connected, trying reconnect...");
    if (!mqttConnectNow()) return;
  }

  if (mqtt_topic.length() == 0) {
    Serial.println("No MQTT base topic configured");
    return;
  }

  String base = mqtt_topic;
  if (base[base.length() - 1] != '/') base += '/';
  base += String(boardID);
  base += '/';

  for (size_t i = 0; i < sensorVector.size(); ++i) {
    const Sensor &s = sensorVector[i];
    for (int j = 0; j < s.returnCount; ++j) {
      const Measurement &m = s.measurements[j];
      if (m.data_name.length() == 0) continue;
      if (isnan(m.value))            continue;

      String topic = base + m.data_name;

      // 2 Nachkommastellen
      char payload[24];
      double v = round(m.value * 100.0) / 100.0;
      snprintf(payload, sizeof(payload), "%.2f", v);

      bool ok = mqtt.publish(topic.c_str(), payload, /*retain=*/false);
      if (!ok) {
        Serial.printf("[MQTT] publish failed: %s = %s (state=%d %s)\n",
                      topic.c_str(), payload, mqtt.state(), mqttStateStr(mqtt.state()));
      } else {
        Serial.printf("[MQTT] %s => %s\n", topic.c_str(), payload);
      }
    }
  }
}

bool setMqttServerFromString(const String& host, uint16_t port) {
  IPAddress ip;
  if (toIPAddress(host, ip)) {
    mqtt.setServer(ip, port);
    return true;
  } else {
    mqtt.setServer(host.c_str(), port);
    return true; // Domain wird intern aufgelöst
  }
}

//prüfen, ob der Port im aktuellen Netz erreichbar ist
bool mqttPreflightCheck(const String& host, uint16_t port) {
  // Hinweis: Bei 443 ist es oft WebSocket – PubSubClient kann KEIN WebSocket.
  // Dieser Check sagt nur, ob TCP offen ist.
  return tcpReachable(host.c_str(), port, /*tls=*/(port==8883 || port==443), /*timeoutMs=*/2000);
}
