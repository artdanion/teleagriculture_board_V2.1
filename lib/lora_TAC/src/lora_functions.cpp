#include <Arduino.h>
#include <lora_functions.h>


// Lora functions
void os_getArtEui(u1_t *buf)
{
   std::copy(app_eui, app_eui + 8, buf);
}
void os_getDevEui(u1_t *buf)
{
   std::copy(dev_eui, dev_eui + 8, buf);
}
void os_getDevKey(u1_t *buf)
{
   std::copy(app_key, app_key + 16, buf);
}

void lora_sendData(void)
{
   Serial.print("\nDEVEUI[8]={ ");
   for (int i = 0; i < 8; i++)
   {
      Serial.print("0x");
      Serial.print(dev_eui[i], HEX);
      if (i < 7)
      {
         Serial.print(", ");
      }
   }
   Serial.println(" };");

   Serial.print("APPEUI[8]={ ");
   for (int i = 0; i < 8; i++)
   {
      Serial.print("0x");
      Serial.print(app_eui[i], HEX);
      if (i < 7)
      {
         Serial.print(", ");
      }
   }
   Serial.println(" };");

   Serial.print("APPKEY[16]={ ");
   for (int i = 0; i < 16; i++)
   {
      Serial.print("0x");
      Serial.print(app_key[i], HEX);
      if (i < 15)
      {
         Serial.print(", ");
      }
   }
   Serial.println(" };\n");

   // https://github.com/thesolarnomad/lora-serialization
   // JS decoder example online

   LoraMessage message;

   for (int i = 0; i < sensorVector.size(); ++i)
   {
      for (int j = 0; j < sensorVector[i].returnCount; j++)
      {
         int k = static_cast<uint8_t>((sensorVector[i].measurements[j].valueOrder));

         if (!isnan(sensorVector[i].measurements[j].value))
         {
            message.addUint8(k);

            switch (k) // send Measurment values as different packeges
            {
            case VOLT:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case TEMP:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case HUMIDITY:
               message.addHumidity(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case PRESSURE:
               message.addRawFloat(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case DISTANCE:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case TDSv:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case MOIS:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case LUX:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case AMBIENT:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case H2v:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case COv:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case CO2v:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case NO2v:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case NH3v:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case C4H10v:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case C3H8v:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case CH4v:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case C2H5OHv:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case ALTITUDE:
               message.addUint16(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<uint16_t>(round(sensorVector[i].measurements[j].value)));
               break;
            case MV:
               message.addRawFloat(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case MGL:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case MSCM:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case PH:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            case DBA:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;

            case DEPTH:
               message.addRawFloat(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;

            case UV_I:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            
            case ANGLE:
               message.addTemperature(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;

            case KOHM:
               message.addUint16(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               Serial.print(sensorVector[i].measurements[j].data_name);
               Serial.print(": #");
               Serial.print(k);
               Serial.print(" Value: ");
               Serial.println(static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0));
               break;
            }
         }
      }
   }

   if (message.getLength() > 2)
   {
      Serial.println("\nSend Lora Data: ");
      do_send(message);
   }
}

void convertTo_LSB_EUI(String input, uint8_t *output)
{
   // Check if input matches pattern
   if (input.length() != 16)
   {
      Serial.println("Input must be 16 characters long.");
      return;
   }
   for (int i = 0; i < 16; i++)
   {
      if (!isxdigit(input.charAt(i)))
      {
         Serial.println("Input must be in hexadecimal format.");
         return;
      }
   }

   // Convert string to byte array
   for (int i = 0; i < 8; i++)
   {
      String byteStr = input.substring(i * 2, i * 2 + 2);
      output[7 - i] = strtol(byteStr.c_str(), NULL, 16);
   }
}

void convertTo_MSB_APPKEY(String input, uint8_t *output)
{
   // Check if input matches pattern
   if (input.length() != 32)
   {
      Serial.println("Input must be 16 characters long.");
      return;
   }
   for (int i = 0; i < 32; i++)
   {
      if (!isxdigit(input.charAt(i)))
      {
         Serial.println("Input must be in hexadecimal format.");
         return;
      }
   }

   // Convert string to byte array
   for (int i = 0; i < 16; i++)
   {
      String byteStr = input.substring(i * 2, i * 2 + 2);
      output[i] = strtol(byteStr.c_str(), NULL, 16);
   }
}

void onEvent(ev_t ev)
{
   Serial.print(os_getTime());
   Serial.print(": ");
   switch (ev)
   {
   case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
   case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
   case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
   case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
   case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
   case EV_JOINED:
   {
      Serial.println(F("EV_JOINED"));

      u4_t netid = 0;
      devaddr_t devaddr = 0;
      u1_t nwkKey[16];
      u1_t artKey[16];
      LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
      Serial.print("netid: ");
      Serial.println(netid, DEC);
      Serial.print("devaddr: ");
      Serial.println(devaddr, HEX);
      Serial.print("artKey: ");
      for (size_t i = 0; i < sizeof(artKey); ++i)
      {
         Serial.print(artKey[i], HEX);
      }
      Serial.println("");
      Serial.print("nwkKey: ");
      for (size_t i = 0; i < sizeof(nwkKey); ++i)
      {
         Serial.print(nwkKey[i], HEX);
      }
      Serial.println("\n");

      loraJoined = true;
      loraJoinFailed = false;
      displayRefresh = true;

      saveLORA_State();

      // Disable link check validation (automatically enabled
      // during join, but because slow data rates change max TX
      // size, we don't use it in this example.
      if (lora_ADR)
      {
         Serial.println("\nuse ADR for LORA (for mobile Nodes)");
         LMIC_setLinkCheckMode(0);
      }
      else
      {
         Serial.println("\nuse ADR for LORA (for mobile Nodes)");
      }
   }
   break;
   /*
       || This event is defined but not used in the code. No
       || point in wasting codespace on it.
       ||
       || case EV_RFU1:
       ||     Serial.println(F("EV_RFU1"));
       ||     break;
       */
   case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      loraJoinFailed = true;
      // upload="WIFI"; // Fallback WIFI
      // save_Config();
      ESP.restart();
      break;
   case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
   case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
      {
         Serial.println(F("Received ack"));
      }
      if (LMIC.dataLen)
      {
         Serial.print(F("Received "));
         Serial.print(LMIC.dataLen);
         Serial.println(F(" bytes of payload"));
      }
      if (!userWakeup)
      {
         gotoSleep = true;
      }
      break;
   case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
   case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
   case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
   case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
   case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
   /*
       || This event is defined but not used in the code. No
       || point in wasting codespace on it.
       ||
       || case EV_SCAN_FOUND:
       ||    Serial.println(F("EV_SCAN_FOUND"));
       ||    break;
       */
   case EV_TXSTART:
      Serial.println(F("EV_TXSTART"));
      break;
   case EV_TXCANCELED:
      Serial.println(F("EV_TXCANCELED"));
      break;
   case EV_RXSTART:
      /* do not print anything -- it wrecks timing */
      break;
   case EV_JOIN_TXCOMPLETE:
      Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
      break;
   default:
      Serial.print(F("Unknown event: "));
      Serial.println((unsigned)ev);
      break;
   }
}

void do_send(LoraMessage &message)
{
   if (message.getLength() > 1)
   {
      // Prepare upstream data transmission at the next possible time.
      LMIC_setTxData2(1, message.getBytes(), message.getLength(), 0);
      // Serial.println(F("Packet queued\n"));
   }
}

void LoraWANPrintLMICOpmode(void)
{
   Serial.print(F("LMIC.opmode: "));
   if (LMIC.opmode & OP_NONE)
   {
      Serial.print(F("OP_NONE "));
   }
   if (LMIC.opmode & OP_SCAN)
   {
      Serial.print(F("OP_SCAN "));
   }
   if (LMIC.opmode & OP_TRACK)
   {
      Serial.print(F("OP_TRACK "));
   }
   if (LMIC.opmode & OP_JOINING)
   {
      Serial.print(F("OP_JOINING "));
   }
   if (LMIC.opmode & OP_TXDATA)
   {
      Serial.print(F("OP_TXDATA "));
   }
   if (LMIC.opmode & OP_POLL)
   {
      Serial.print(F("OP_POLL "));
   }
   if (LMIC.opmode & OP_REJOIN)
   {
      Serial.print(F("OP_REJOIN "));
   }
   if (LMIC.opmode & OP_SHUTDOWN)
   {
      Serial.print(F("OP_SHUTDOWN "));
   }
   if (LMIC.opmode & OP_TXRXPEND)
   {
      Serial.print(F("OP_TXRXPEND "));
   }
   if (LMIC.opmode & OP_RNDTX)
   {
      Serial.print(F("OP_RNDTX "));
   }
   if (LMIC.opmode & OP_PINGINI)
   {
      Serial.print(F("OP_PINGINI "));
   }
   if (LMIC.opmode & OP_PINGABLE)
   {
      Serial.print(F("OP_PINGABLE "));
   }
   if (LMIC.opmode & OP_NEXTCHNL)
   {
      Serial.print(F("OP_NEXTCHNL "));
   }
   if (LMIC.opmode & OP_LINKDEAD)
   {
      Serial.print(F("OP_LINKDEAD "));
   }
   if (LMIC.opmode & OP_LINKDEAD)
   {
      Serial.print(F("OP_LINKDEAD "));
   }
   if (LMIC.opmode & OP_TESTMODE)
   {
      Serial.print(F("OP_TESTMODE "));
   }
   if (LMIC.opmode & OP_UNJOIN)
   {
      Serial.print(F("OP_UNJOIN "));
   }
}

void saveLMICToRTC(int deepsleep_sec)
{
   Serial.println(F("Save LMIC to RTC"));
   RTC_LMIC = LMIC;

   // ESP32 can't track millis during DeepSleep and no option to advanced millis after DeepSleep.
   // Therefore reset DutyCyles

   unsigned long now = millis();

   // EU Like Bands
#if defined(CFG_eu868)
   Serial.println(F("Reset CFG_LMIC_EU_like band avail"));
   for (int i = 0; i < MAX_BANDS; i++)
   {
      ostime_t correctedAvail = RTC_LMIC.bands[i].avail - ((now / 1000.0 + deepsleep_sec) * OSTICKS_PER_SEC);
      if (correctedAvail < 0)
      {
         correctedAvail = 0;
      }
      RTC_LMIC.bands[i].avail = correctedAvail;
   }

   RTC_LMIC.globalDutyAvail = RTC_LMIC.globalDutyAvail - ((now / 1000.0 + deepsleep_sec) * OSTICKS_PER_SEC);
   if (RTC_LMIC.globalDutyAvail < 0)
   {
      RTC_LMIC.globalDutyAvail = 0;
   }
#else
   Serial.println(F("No DutyCycle recalculation function!"));
#endif
}

void loadLMICFromRTC()
{
   Serial.println(F("Load LMIC from RTC"));
   LMIC = RTC_LMIC;
}

// Function to store LMIC configuration to RTC Memory
void saveLORA_State(void)
{
   Serial.println(F("Save LMIC to RTC ..."));
   RTC_LORAWAN_netid = LMIC.netid;
   RTC_LORAWAN_devaddr = LMIC.devaddr;
   memcpy(RTC_LORAWAN_nwkKey, LMIC.nwkKey, 16);
   memcpy(RTC_LORAWAN_artKey, LMIC.artKey, 16);
   RTC_LORAWAN_dn2Dr = LMIC.dn2Dr;
   RTC_LORAWAN_dnConf = LMIC.dnConf;
   RTC_LORAWAN_seqnoDn = LMIC.seqnoDn;
   RTC_LORAWAN_seqnoUp = LMIC.seqnoUp;
   RTC_LORAWAN_adrTxPow = LMIC.adrTxPow;
   RTC_LORAWAN_txChnl = LMIC.txChnl;
   RTC_LORAWAN_datarate = LMIC.datarate;
   RTC_LORAWAN_adrAckReq = LMIC.adrAckReq;
   RTC_LORAWAN_rx1DrOffset = LMIC.rx1DrOffset;
   RTC_LORAWAN_rxDelay = LMIC.rxDelay;

#if (CFG_eu868)
   memcpy(RTC_LORAWAN_channelFreq, LMIC.channelFreq, MAX_CHANNELS * sizeof(u4_t));
   memcpy(RTC_LORAWAN_channelDrMap, LMIC.channelDrMap, MAX_CHANNELS * sizeof(u2_t));
   memcpy(RTC_LORAWAN_channelDlFreq, LMIC.channelDlFreq, MAX_CHANNELS * sizeof(u4_t));
   memcpy(RTC_LORAWAN_bands, LMIC.bands, MAX_BANDS * sizeof(band_t));
   RTC_LORAWAN_channelMap = LMIC.channelMap;
#endif

   Serial.println("LMIC configuration stored in RTC Memory.");
}

// Function to reload LMIC configuration from RTC Memory
void loadLORA_State()
{
   Serial.println(F("Load LMIC State from RTC ..."));

   LMIC_setSession(RTC_LORAWAN_netid, RTC_LORAWAN_devaddr, RTC_LORAWAN_nwkKey, RTC_LORAWAN_artKey);
   LMIC_setSeqnoUp(RTC_LORAWAN_seqnoUp);
   LMIC_setDrTxpow(RTC_LORAWAN_datarate, RTC_LORAWAN_adrTxPow);
   LMIC.seqnoDn = RTC_LORAWAN_seqnoDn;
   LMIC.dnConf = RTC_LORAWAN_dnConf;
   LMIC.adrAckReq = RTC_LORAWAN_adrAckReq;
   LMIC.dn2Dr = RTC_LORAWAN_dn2Dr;
   LMIC.rx1DrOffset = RTC_LORAWAN_rx1DrOffset;
   LMIC.rxDelay = RTC_LORAWAN_rxDelay;
   LMIC.txChnl = RTC_LORAWAN_txChnl;

#if (CFG_eu868)
   memcpy(LMIC.bands, RTC_LORAWAN_bands, MAX_BANDS * sizeof(band_t));
   memcpy(LMIC.channelFreq, RTC_LORAWAN_channelFreq, MAX_CHANNELS * sizeof(u4_t));
   memcpy(LMIC.channelDlFreq, RTC_LORAWAN_channelDlFreq, MAX_CHANNELS * sizeof(u4_t));
   memcpy(LMIC.channelDrMap, RTC_LORAWAN_channelDrMap, MAX_CHANNELS * sizeof(u2_t));
   LMIC.channelMap = RTC_LORAWAN_channelMap;
#endif
   delay(200);
   Serial.println("LMIC configuration reloaded from RTC Memory.");
}

