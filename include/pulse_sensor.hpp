/*\
 *
 * TeleAgriCulture Board - PulseSensor beat detection
 *
 * Continuous analog pulse sampling + beat detection running in its own
 * FreeRTOS task at a stable 500 Hz. This decouples the (timing critical) beat
 * detection from the slow main loop / sensorRead() cadence (~20 Hz), which is
 * far too slow for a pulse waveform.
 *
 * Only meaningful in LIVE mode: the board stays awake and streams continuously
 * (MQTT/OSC). In WiFi/LoRa/SD modes the board deep-sleeps between interval
 * reads, so continuous sampling can't work there and the task is not started.
 *
 * The beat detection is the canonical "PulseSensor Amped" algorithm by World
 * Famous Electronics (the same math their PulseSensorPlayground library runs in
 * its 2 ms ISR). It is implemented inline here instead of pulling in the
 * library, because that library hard-codes the ESP32 hardware-timer API of
 * Arduino-core 3.x and does not compile on the core 2.0.x this project pins.
 *
 * Algorithm constants are tuned for a 0..1023 (10-bit) signal, so the raw
 * 12-bit ADC reading is mapped down with >> 2.
 *
\*/

#pragma once

#include <Arduino.h>
#include <def_Board.h>   // ANALOG1/2/3, ADC_NUM, ADC_con_table
#include <def_Sensors.h> // HEART_RATE

// Drop the reading if no beat was seen for this long (finger removed / no signal).
#ifndef PULSE_SIGNAL_TIMEOUT_MS
#define PULSE_SIGNAL_TIMEOUT_MS 3000
#endif

// Periodic serial debug of the raw signal range / threshold / BPM (every 1 s).
// Set to 0 once the sensor is brought up and tuned.
#ifndef PULSE_DEBUG
#define PULSE_DEBUG 1
#endif

// Shared state written by the task, read by sensorRead().
inline volatile int           pulseBPM        = 0;
inline volatile unsigned long pulseLastBeatMs = 0;
inline TaskHandle_t           pulseTaskHandle = nullptr;

// Returns the ANALOG GPIO a HEART_RATE sensor is configured on, or -1 if none.
inline int pulseSensorConfiguredPin()
{
   const int adcPins[ADC_NUM] = {ANALOG1, ANALOG2, ANALOG3};
   for (int i = 0; i < ADC_NUM; i++)
      if (ADC_con_table[i] == HEART_RATE)
         return adcPins[i];
   return -1;
}

// Latest valid BPM, or NAN when the task isn't running or no recent beat was
// detected. Called from the HEART_RATE case in sensorRead().
inline double pulseSensorBPM()
{
   if (pulseTaskHandle == nullptr)
      return NAN;
   if (millis() - pulseLastBeatMs > PULSE_SIGNAL_TIMEOUT_MS)
      return NAN;
   return (double)pulseBPM;
}

inline void pulseSensorTask(void *param)
{
   const int pin = (int)(intptr_t)param;

   analogReadResolution(12); // board default; reading is mapped to 10-bit below
   pinMode(pin, INPUT);

   // --- PulseSensor "Amped" beat-detection state (canonical algorithm @ 500 Hz) ---
   int rate[10] = {0};                 // last 10 inter-beat intervals (ms)
   unsigned long sampleCounter = 0;    // ms since start (advances 2 ms / sample)
   unsigned long lastBeatTime  = 0;    // sampleCounter value at the last detected beat
   int  IBI    = 600;                  // inter-beat interval (ms)
   int  P      = 512;                  // peak of the pulse wave
   int  T      = 512;                  // trough of the pulse wave
   int  thresh = 525;                  // adaptive detection threshold
   int  amp    = 100;                  // amplitude of the pulse wave
   bool Pulse      = false;            // true while signal is above threshold (in a beat)
   bool firstBeat  = true;             // first read has no valid IBI
   bool secondBeat = false;            // second read seeds the rate[] history

#if PULSE_DEBUG
   int sigMin = 1023, sigMax = 0, beats = 0;
   unsigned long lastDbg = millis();
#endif

   TickType_t lastWake = xTaskGetTickCount();
   for (;;)
   {
      vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(2)); // stable 500 Hz cadence

      int Signal = analogRead(pin) >> 2; // 12-bit (0..4095) -> 10-bit (0..1023)
      sampleCounter += 2;

#if PULSE_DEBUG
      if (Signal < sigMin) sigMin = Signal;
      if (Signal > sigMax) sigMax = Signal;
      if (millis() - lastDbg >= 1000)
      {
         Serial.printf("[PULSE] sig=%d..%d thresh=%d amp=%d BPM=%d beats/s=%d\n",
                       sigMin, sigMax, thresh, amp, pulseBPM, beats);
         sigMin = 1023; sigMax = 0; beats = 0;
         lastDbg = millis();
      }
#endif

      int N = (int)(sampleCounter - lastBeatTime);

      // track trough, avoiding the dicrotic notch shortly after a beat
      if (Signal < thresh && N > (IBI / 5) * 3)
         if (Signal < T)
            T = Signal;

      // track peak
      if (Signal > thresh && Signal > P)
         P = Signal;

      // look for a beat: signal must rise above threshold, min 250 ms apart (<=240 BPM)
      if (N > 250)
      {
         if (Signal > thresh && !Pulse && N > (IBI / 5) * 3)
         {
            Pulse = true;
            IBI = (int)(sampleCounter - lastBeatTime);
            lastBeatTime = sampleCounter;
#if PULSE_DEBUG
            beats++;
#endif

            if (secondBeat)
            {
               secondBeat = false;
               for (int i = 0; i < 10; i++)
                  rate[i] = IBI; // seed the running average with the first solid IBI
            }
            if (firstBeat)
            {
               firstBeat = false;
               secondBeat = true;
               continue; // no valid IBI on the very first beat
            }

            // running average over the last 10 inter-beat intervals
            unsigned int runningTotal = 0;
            for (int i = 0; i < 9; i++)
            {
               rate[i] = rate[i + 1];
               runningTotal += rate[i];
            }
            rate[9] = IBI;
            runningTotal += rate[9];
            runningTotal /= 10;

            int bpm = 60000 / (int)runningTotal;
            if (bpm >= 40 && bpm <= 220) // plausibility gate
            {
               pulseBPM = bpm;
               pulseLastBeatMs = millis();
            }
         }
      }

      // beat over: signal dropped back below threshold -> recompute adaptive threshold
      if (Signal < thresh && Pulse)
      {
         Pulse = false;
         amp = P - T;
         thresh = amp / 2 + T; // threshold at 50 % of the wave amplitude
         P = thresh;
         T = thresh;
      }

      // no beat for 2.5 s -> reset the detector to defaults
      if (N > 2500)
      {
         thresh = 525;
         P = 512;
         T = 512;
         lastBeatTime = sampleCounter;
         firstBeat = true;
         secondBeat = false;
      }
   }
}

// Start the continuous pulse-sampling task. Idempotent; no-op if no HEART_RATE
// sensor is configured. Call this only in LIVE mode.
inline void startPulseSensorTask()
{
   if (pulseTaskHandle != nullptr)
      return;

   int pin = pulseSensorConfiguredPin();
   if (pin < 0)
   {
      Serial.println("PulseSensor: no HEART_RATE sensor configured, task not started");
      return;
   }

   xTaskCreatePinnedToCore(
       pulseSensorTask,
       "PulseSensorTask",
       4000, // stack size
       (void *)(intptr_t)pin,
       2, // priority (above the config-button task)
       &pulseTaskHandle,
       0 // Core-ID 0 (main loop runs on core 1)
   );

   Serial.printf("PulseSensor task started on GPIO %d\n", pin);
}
