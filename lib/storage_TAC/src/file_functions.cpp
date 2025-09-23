#include <file_functions.h>
#include "board_credentials.h"
#include <init_Board.h>
#include <debug_functions.h>
#include <time_functions.h>

#include <ArduinoJson.h>

// file and storage functions

void SD_sendData()
{
   DynamicJsonDocument docMeasures(2000);

   // Add timestamp
   String timestamp;
   if (rtcEnabled && rtc.begin())
   {
      DateTime now = rtc.now();
      char buffer[25];
      sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
              now.year(), now.month(), now.day(),
              now.hour(), now.minute(), now.second());
      timestamp = String(buffer);
   }
   else
   {
      // Use system time (if available) or millis() as fallback
      time_t now = time(nullptr);
      if (now > 0)
      {
         struct tm *timeinfo = localtime(&now);
         char buffer[25];
         sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                 timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
         timestamp = String(buffer);
      }
      else
      {
         timestamp = String(millis()); // fallback to millis if time() not available
      }
   }

   docMeasures["timestamp"] = timestamp;

   // Collect sensor data
   for (int i = 0; i < sensorVector.size(); ++i)
   {
      for (int j = 0; j < sensorVector[i].returnCount; j++)
      {
         if (!isnan(sensorVector[i].measurements[j].value))
         {
            docMeasures[sensorVector[i].measurements[j].data_name] =
                static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0);
         }
      }
   }

   String output;
   serializeJson(docMeasures, output);

   // Ensure the output is not empty
   if (output.length() > 2) // "{}" is 2 chars
   {
      String path = "/Teleagriculture_SensorKit_id_" + String(boardID);
      File dataFile;

      if (SD.exists(path.c_str()))
      {
         dataFile = SD.open(path.c_str(), FILE_APPEND);
      }
      else
      {
         dataFile = SD.open(path.c_str(), FILE_WRITE);
      }

      if (dataFile)
      {
         dataFile.println(output);
         dataFile.close();
         Serial.println("Data written to SD card: " + output);
      }
      else
      {
         Serial.println("Failed to open file for writing");
      }
   }
   else
   {
      Serial.println("No valid data to write");
   }
}

void initFiles()
{
   // print SPIFF files for debugging
   if (!SPIFFS.begin(true))
   {
      Serial.println("Failed to mount SPIFFS file system");
      return;
   }
   listDir(SPIFFS, "/", 0);
   Serial.println();
}

void load_Sensors()
{
   DynamicJsonDocument doc(JSON_BUFFER);

   DeserializationError error = deserializeJson(doc, proto_sensors);

   // Check for parsing errors
   if (error)
   {
      Serial.print(F("Failed to parse JSON: "));
      Serial.println(error.c_str());
      forceConfig = true;
      return;
   }

   for (size_t i = 0; i < SENSORS_NUM; i++)
   {
      JsonObject sensorObj = doc[i];

      allSensors[i].sensor_id = NO;
      allSensors[i].sensor_name = " ";

      allSensors[i].sensor_id = sensorObj["sensor-id"].as<SensorsImplemented>();
      allSensors[i].sensor_name = sensorObj["name"].as<String>();
      allSensors[i].con_typ = sensorObj["con_typ"].as<String>();
      allSensors[i].returnCount = sensorObj["returnCount"].as<int>();

      if (sensorObj.containsKey("measurements"))
      {
         JsonArray measurementsArr = sensorObj["measurements"].as<JsonArray>();
         size_t numMeasurements = measurementsArr.size();

         for (size_t j = 0; j < numMeasurements; j++)
         {
            JsonObject measurementObj = measurementsArr[j].as<JsonObject>();
            if (!measurementObj.containsKey("data_name") || !measurementObj.containsKey("value") || !measurementObj.containsKey("unit"))
            {
               Serial.println("Missing 'data_name', 'value', or 'unit' key in measurement object");
               continue;
            }

            allSensors[i].measurements[j].data_name = measurementObj["data_name"].as<String>();
            allSensors[i].measurements[j].value = measurementObj["value"].as<float>();
            allSensors[i].measurements[j].valueOrder = getValueOrderFromString(measurementObj["valueOrder"].as<String>());
            allSensors[i].measurements[j].unit = measurementObj["unit"].as<String>();
         }
      }
      if (sensorObj.containsKey("addr_num"))
      {
         allSensors[i].addr_num = sensorObj["addr_num"].as<int>();
      }

      if (sensorObj.containsKey("possible_i2c_add"))
      {
         JsonObject i2c_alt_Obj = sensorObj["possible_i2c_add"].as<JsonObject>();

         if (i2c_alt_Obj.containsKey("default"))
         {
            allSensors[i].possible_i2c_add[0] = i2c_alt_Obj["default"].as<String>();
            // Serial.println("Loaded I2C default address: " + allSensors[i].possible_i2c_add[0]);
         }

         if (i2c_alt_Obj.containsKey("alt_1"))
         {
            allSensors[i].possible_i2c_add[1] = i2c_alt_Obj["alt_1"].as<String>();
            // Serial.println("Loaded I2C alternative address 1: " + allSensors[i].possible_i2c_add[1]);
         }

         if (i2c_alt_Obj.containsKey("alt_2"))
         {
            allSensors[i].possible_i2c_add[2] = i2c_alt_Obj["alt_2"].as<String>();
            // Serial.println("Loaded I2C alternative address 2: " + allSensors[i].possible_i2c_add[2]);
         }

         if (i2c_alt_Obj.containsKey("alt_3"))
         {
            allSensors[i].possible_i2c_add[3] = i2c_alt_Obj["alt_3"].as<String>();
            // Serial.println("Loaded I2C alternative address 3: " + allSensors[i].possible_i2c_add[3]);
         }
      }
   }
   Serial.printf("JSON verwendet %zu von %zu Bytes\n",
                 doc.memoryUsage(), doc.capacity());
   Serial.println();

   DynamicJsonDocument deallocate(doc);
}

void load_Connectors()
{
   if (!SPIFFS.begin())
   {
      Serial.println("failed to mount FS");
      forceConfig = true;
      return;
   }

   if (!SPIFFS.exists("/connectors.json"))
   {
      Serial.println("no config file found");
      forceConfig = true;
      return;
   }

   File connectorsFile = SPIFFS.open("/connectors.json", "r");
   if (!connectorsFile)
   {
      Serial.println("failed to open config file");
      forceConfig = true;
      return;
   }

   StaticJsonDocument<800> doc;
   DeserializationError error = deserializeJson(doc, connectorsFile);
   connectorsFile.close();

   if (error)
   {
      Serial.println("failed to parse config file");
      forceConfig = true;
      return;
   }

   // I2C
   JsonArray jsonI2C = doc["I2C_connectors"];
   for (int i = 0; i < I2C_NUM && i < jsonI2C.size(); i++)
   {
      if (jsonI2C[i]["sensorIndex"].isNull())
      {
         I2C_con_table[i].sensorIndex = -1; // Default value if sensorIndex is missing
         Serial.println("failed to load I2C connector");
         forceConfig = true;
      }
      else
      {
         I2C_con_table[i].sensorIndex = jsonI2C[i]["sensorIndex"];
      }

      if (jsonI2C[i]["addrIndex"].isNull())
      {
         I2C_con_table[i].addrIndex = -1; // Default value if addrIndex is missing
         Serial.println("failed to load I2C connector");
         forceConfig = true;
      }
      else
      {
         I2C_con_table[i].addrIndex = jsonI2C[i]["addrIndex"];
      }
   }

   // OneWire
   JsonArray jsonOneWire = doc["OneWire_connectors"];
   for (int i = 0; i < ONEWIRE_NUM && i < jsonOneWire.size(); i++)
   {
      OneWire_con_table[i] = jsonOneWire[i];
   }

   // ADC
   JsonArray jsonADC = doc["ADC_connectors"];
   for (int i = 0; i < ADC_NUM && i < jsonADC.size(); i++)
   {
      ADC_con_table[i] = jsonADC[i];
   }

   // SPI
   JsonArray jsonSPI = doc["SPI_connectors"];
   for (int i = 0; i < SPI_NUM && i < jsonSPI.size(); i++)
   {
      SPI_con_table[i] = jsonSPI[i];
   }

   // I2C 5V
   JsonArray jsonI2C_5V = doc["I2C_5V_connectors"];
   for (int i = 0; i < I2C_5V_NUM && i < jsonI2C_5V.size(); i++)
   {
      if (jsonI2C_5V[i]["sensorIndex"].isNull())
      {
         I2C_5V_con_table[i].sensorIndex = -1; // Default value if sensorIndex is missing
         Serial.println("failed to load I2C 5V connector");
         forceConfig = true;
      }
      else
      {
         I2C_5V_con_table[i].sensorIndex = jsonI2C_5V[i]["sensorIndex"];
      }

      if (jsonI2C_5V[i]["addrIndex"].isNull())
      {
         I2C_5V_con_table[i].addrIndex = -1; // Default value if addrIndex is missing
         Serial.println("failed to load I2C 5V connector");
         forceConfig = true;
      }
      else
      {
         I2C_5V_con_table[i].addrIndex = jsonI2C_5V[i]["addrIndex"];
      }
   }

   // EXTRA
   JsonArray jsonExtra = doc["EXTRA_connectors"];
   for (int i = 0; i < EXTRA_NUM && i < jsonExtra.size(); i++)
   {
      EXTRA_con_table[i] = jsonExtra[i];
   }
}

void save_Connectors()
{
   StaticJsonDocument<800> doc;

   // I2C
   JsonArray jsonI2C = doc.createNestedArray("I2C_connectors");
   for (int j = 0; j < I2C_NUM; j++)
   {
      JsonObject obj = jsonI2C.createNestedObject();
      obj["sensorIndex"] = I2C_con_table[j].sensorIndex;
      obj["addrIndex"] = I2C_con_table[j].addrIndex;
   }

   // OneWire
   JsonArray jsonOneWire = doc.createNestedArray("OneWire_connectors");
   for (int i = 0; i < ONEWIRE_NUM; i++)
   {
      jsonOneWire.add(OneWire_con_table[i]);
   }

   // ADC
   JsonArray jsonADC = doc.createNestedArray("ADC_connectors");
   for (int i = 0; i < ADC_NUM; i++)
   {
      jsonADC.add(ADC_con_table[i]);
   }

   // SPI
   JsonArray jsonSPI = doc.createNestedArray("SPI_connectors");
   for (int i = 0; i < SPI_NUM; i++)
   {
      jsonSPI.add(SPI_con_table[i]);
   }

   // I2C 5V
   JsonArray jsonI2C_5V = doc.createNestedArray("I2C_5V_connectors");
   for (int i = 0; i < I2C_5V_NUM; i++)
   {
      JsonObject obj = jsonI2C_5V.createNestedObject();
      obj["sensorIndex"] = I2C_5V_con_table[i].sensorIndex;
      obj["addrIndex"] = I2C_5V_con_table[i].addrIndex;
   }

   // EXTRA
   JsonArray jsonExtra = doc.createNestedArray("EXTRA_connectors");
   for (int i = 0; i < EXTRA_NUM; i++)
   {
      jsonExtra.add(EXTRA_con_table[i]);
   }

   // Speichern
   File connectorsFile = SPIFFS.open("/connectors.json", "w");
   if (!connectorsFile)
   {
      Serial.println("failed to open config file for writing");
      return;
   }
   serializeJsonPretty(doc, connectorsFile);
   connectorsFile.close();

   Serial.println("Saved Connector File");
}

void save_Config(void)
{
   Serial.println("saveConfig start");

   DynamicJsonDocument doc(1152); // heap, not stack

   doc["BoardID"] = boardID;
   doc["useBattery"] = useBattery;
   doc["useDisplay"] = useDisplay;
   doc["saveDataSDCard"] = saveDataSDCard;
   doc["useEnterpriseWPA"] = useEnterpriseWPA;
   doc["useCustomNTP"] = useCustomNTP;
   doc["useNTP"] = useNTP;
   doc["rtcEnabled"] = rtcEnabled;

   doc["API_KEY"] = API_KEY.length() ? API_KEY : "";
   doc["upload"] = upload.length() ? upload : "";
   doc["upInterval"] = upload_interval;

   doc["OTAA_DEVEUI"] = OTAA_DEVEUI.length() ? OTAA_DEVEUI : "";
   doc["OTAA_APPEUI"] = OTAA_APPEUI.length() ? OTAA_APPEUI : "";
   doc["OTAA_APPKEY"] = OTAA_APPKEY.length() ? OTAA_APPKEY : "";
   doc["lora_ADR"] = lora_ADR;

   doc["anonym"] = anonym.length() ? anonym : "";
   doc["user_CA"] = user_CA.length() ? user_CA : "";
   doc["customNTPadress"] = customNTPaddress.length() ? customNTPaddress : "";
   doc["timeZone"] = timeZone.length() ? timeZone : "";
   doc["apn"] = apn.length() ? apn : "";
   doc["gprs_user"] = gprs_user.length() ? gprs_user : "";
   doc["gprs_pass"] = gprs_pass.length() ? gprs_pass : "";

   // NEU: LIVE/MQTT/OSC
   doc["live_mode"] = live_mode.length() ? live_mode : "";
   doc["mqtt_server_ip"] = mqtt_server_ip.length() ? mqtt_server_ip : "";
   doc["mqtt_topic"] = mqtt_topic.length() ? mqtt_topic : "";
   doc["mqtt_port"] = static_cast<int>(mqtt_port);
   doc["osc_ip"] = osc_ip.length() ? osc_ip : "";
   doc["osc_port"] = static_cast<int>(osc_port);

   File configFile = SPIFFS.open("/board_config.json", "w");
   if (!configFile)
   {
      Serial.println("failed to open config file for writing");
      return;
   }
   serializeJson(doc, configFile);
   configFile.close();

   Serial.println("Saved Config File");
}

void load_Config(void)
{
   if (!SPIFFS.begin())
   {
      Serial.println("failed to mount FS");
      forceConfig = true;
      return;
   }
   if (!SPIFFS.exists("/board_config.json"))
   {
      Serial.println("config file not found (using defaults)");
      return;
   }

   File configFile = SPIFFS.open("/board_config.json", "r");
   if (!configFile)
   {
      Serial.println("failed to open config file");
      forceConfig = true;
      return;
   }

   size_t size = configFile.size();
   std::unique_ptr<char[]> buf(new char[size + 1]);
   size_t nread = configFile.readBytes(buf.get(), size);
   buf[nread] = '\0';
   configFile.close();

   DynamicJsonDocument doc(1152); // heap, not stack
   DeserializationError err = deserializeJson(doc, buf.get());
   if (err)
   {
      Serial.printf("deserializeJson error: %s\n", err.c_str());
      forceConfig = true;
      return;
   }

   if (doc.containsKey("BoardID"))
      boardID = doc["BoardID"].as<int>();
   if (doc.containsKey("useBattery"))
      useBattery = doc["useBattery"];
   if (doc.containsKey("useDisplay"))
      useDisplay = doc["useDisplay"];
   if (doc.containsKey("saveDataSDCard"))
      saveDataSDCard = doc["saveDataSDCard"];

   // Abwärtskompatibel: falscher Key-Name wurde früher geschrieben
   if (doc.containsKey("useEnterpriseWPA"))
      useEnterpriseWPA = doc["useEnterpriseWPA"];
   else if (doc.containsKey("useEnzerpriseWPA"))
      useEnterpriseWPA = doc["useEnzerpriseWPA"];

   if (doc.containsKey("useCustomNTP"))
      useCustomNTP = doc["useCustomNTP"];
   if (doc.containsKey("useNTP"))
      useNTP = doc["useNTP"];
   if (doc.containsKey("rtcEnabled"))
      rtcEnabled = doc["rtcEnabled"];

   if (doc.containsKey("API_KEY"))
      API_KEY = doc["API_KEY"].as<String>();
   if (doc.containsKey("upload"))
      upload = doc["upload"].as<String>();
   if (doc.containsKey("upInterval"))
      upload_interval = doc["upInterval"].as<int>();
   if (doc.containsKey("anonym"))
      anonym = doc["anonym"].as<String>();
   if (doc.containsKey("user_CA"))
      user_CA = doc["user_CA"].as<String>();
   if (doc.containsKey("customNTPadress"))
      customNTPaddress = doc["customNTPadress"].as<String>(); // (sic)
   if (doc.containsKey("timeZone"))
      timeZone = doc["timeZone"].as<String>();
   if (doc.containsKey("OTAA_DEVEUI"))
      OTAA_DEVEUI = doc["OTAA_DEVEUI"].as<String>();
   if (doc.containsKey("OTAA_APPEUI"))
      OTAA_APPEUI = doc["OTAA_APPEUI"].as<String>();
   if (doc.containsKey("OTAA_APPKEY"))
      OTAA_APPKEY = doc["OTAA_APPKEY"].as<String>();
   if (doc.containsKey("lora_ADR"))
      lora_ADR = doc["lora_ADR"];
   if (doc.containsKey("apn"))
      apn = doc["apn"].as<String>();
   if (doc.containsKey("gprs_user"))
      gprs_user = doc["gprs_user"].as<String>();
   if (doc.containsKey("gprs_pass"))
      gprs_pass = doc["gprs_pass"].as<String>();

   // new: LIVE/MQTT/OSC
   if (doc.containsKey("live_mode"))
      live_mode = doc["live_mode"].as<String>();
   if (doc.containsKey("mqtt_server_ip"))
      mqtt_server_ip = doc["mqtt_server_ip"].as<String>();
   if (doc.containsKey("mqtt_topic"))
      mqtt_topic = doc["mqtt_topic"].as<String>();
   if (doc.containsKey("mqtt_port"))
   {
      int m = doc["mqtt_port"].as<int>();
      if (m >= 1 && m <= 65535)
         mqtt_port = static_cast<uint16_t>(m);
   }
   if (doc.containsKey("osc_ip"))
      osc_ip = doc["osc_ip"].as<String>();
   if (doc.containsKey("osc_port"))
   {
      int p = doc["osc_port"].as<int>();
      if (p >= 1 && p <= 65535)
         osc_port = static_cast<uint16_t>(p);
   }
}
