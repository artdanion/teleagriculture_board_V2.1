#pragma once
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// Global MQTT objects (defined once in a .cpp)
extern WiFiClient _mqttNet;
extern WiFiClientSecure _mqttNetSecure;
extern PubSubClient mqtt;

// Retry bookkeeping
extern unsigned long _mqttRetryAt;
extern uint8_t _mqttAttempts;

// Helpers you want to call from main.cpp
bool mqttUseTLS();          // returns true for 8883/443
void selectMqttNetClient(); // picks secure/plain and binds to `mqtt`
bool ensureMqttConnected(bool immediate = false);
void mqttLoop();
void publishLiveTopics();
String makeClientId();
bool mqttConnectNow();
bool setMqttServerFromString(const String& host, uint16_t port);
bool mqttPreflightCheck(const String& host, uint16_t port);