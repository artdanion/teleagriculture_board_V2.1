#include <file_functions.h>
#include "board_credentials.h"
#include <init_Board.h>
#include <debug_functions.h>

#include <ArduinoJson.h>

// file and storage functions

void SD_sendData()
{
   DynamicJsonDocument docMeasures(2000);

   for (int i = 0; i < sensorVector.size(); ++i)
   {
      for (int j = 0; j < sensorVector[i].returnCount; j++)
      {
         if (!isnan(sensorVector[i].measurements[j].value))
         {
            docMeasures[sensorVector[i].measurements[j].data_name] = static_cast<float>(round(sensorVector[i].measurements[j].value * 100) / 100.0);
         }
      }
   }

   String output;
   serializeJson(docMeasures, output);

   // Ensure the output is not empty
   if (output.length() > 2) // The minimum valid JSON object would be "{}" which is 2 characters
   {
      String path = "/Teleagriculture_SensorKit_id_" + String(boardID); // File path
      File dataFile;

      // Check if the file exists
      if (SD.exists(path.c_str()))
      {
         dataFile = SD.open(path.c_str(), FILE_APPEND); // Open in append mode
      }
      else
      {
         dataFile = SD.open(path.c_str(), FILE_WRITE); // Create a new file
      }

      if (dataFile)
      {
         dataFile.println(output); // Append the JSON data as a new line
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

   StaticJsonDocument<650> doc;
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
   StaticJsonDocument<650> doc;

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
}

void load_Config(void)
{
   if (SPIFFS.begin())
   {
      if (SPIFFS.exists("/board_config.json"))
      {
         // file exists, reading and loading
         // Serial.println("reading config file");
         File configFile = SPIFFS.open("/board_config.json", "r");
         if (configFile)
         {
            // Serial.println("opened config file");
            size_t size = configFile.size();
            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);

            configFile.readBytes(buf.get(), size);

            StaticJsonDocument<650> doc;
            auto deserializeError = deserializeJson(doc, buf.get());

            if (!deserializeError)
            {
               boardID = doc["BoardID"];
               useBattery = doc["useBattery"];
               useDisplay = doc["useDisplay"];
               saveDataSDCard = doc["saveDataSDCard"];
               useEnterpriseWPA = doc["useEnzerpriseWPA"];
               useCustomNTP = doc["useCustomNTP"];
               useNTP = doc["useNTP"];
               rtcEnabled = doc["rtcEnabled"];
               API_KEY = doc["API_KEY"].as<String>();
               upload = doc["upload"].as<String>();
               upload_interval = doc["upInterval"];
               anonym = doc["anonym"].as<String>();
               user_CA = doc["user_CA"].as<String>();
               customNTPaddress = doc["customNTPadress"].as<String>();
               timeZone = doc["timeZone"].as<String>();
               OTAA_DEVEUI = doc["OTAA_DEVEUI"].as<String>();
               OTAA_APPEUI = doc["OTAA_APPEUI"].as<String>();
               OTAA_APPKEY = doc["OTAA_APPKEY"].as<String>();
               lora_ADR = doc["lora_ADR"];
               apn = doc["apn"].as<String>();
               gprs_user = doc["gprs_user"].as<String>();
               gprs_pass = doc["gprs_pass"].as<String>();
            }
         }
         else
         {
            Serial.println("failed to load json config");
            forceConfig = true;
         }
         configFile.close();
      }
   }
   else
   {
      Serial.println("failed to mount FS");
      forceConfig = true;
   }
}

void save_Config(void)
{
   StaticJsonDocument<650> doc;

   doc["BoardID"] = boardID;
   doc["useBattery"] = useBattery;
   doc["useDisplay"] = useDisplay;
   doc["saveDataSDCard"] = saveDataSDCard;
   doc["useEnterpriseWPA"] = useEnterpriseWPA;
   doc["useCustomNTP"] = useCustomNTP;
   doc["useNTP"] = useNTP;
   doc["rtcEnabled"] = rtcEnabled;
   doc["API_KEY"] = API_KEY;
   doc["upload"] = upload;
   doc["upInterval"] = upload_interval;
   doc["anonym"] = anonym;
   doc["user_CA"] = user_CA;
   doc["customNTPadress"] = customNTPaddress;
   doc["timeZone"] = timeZone;
   doc["OTAA_DEVEUI"] = OTAA_DEVEUI;
   doc["OTAA_APPEUI"] = OTAA_APPEUI;
   doc["OTAA_APPKEY"] = OTAA_APPKEY;
   doc["lora_ADR"] = lora_ADR;
   doc["apn"] = apn;
   doc["gprs_user"] = gprs_user;
   doc["gprs_pass"] = gprs_pass;

   File configFile = SPIFFS.open("/board_config.json", "w");
   if (!configFile)
   {
      Serial.println("failed to open config file for writing");
   }

   serializeJson(doc, configFile);

   configFile.close();
   // end save
}
