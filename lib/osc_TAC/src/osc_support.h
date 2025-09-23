#pragma once
#include <Arduino.h>
#include <IPAddress.h>

// runtime flag toggled in setupOSCIfNeeded()
extern bool sendDataOSC;

// connectivity helpers
bool oscPreflightCheck(const String& host, uint16_t port);
bool oscConnectNow();
bool ensureOscReady(bool immediate = false);
void oscLoop();        // (no-op unless you enable inbound)

// outbound publishing
void publishLiveOsc(); // uses your mqtt_topic + boardID to build the OSC address base
void publishSensorVectorAsOSC(const String& baseAddress);
