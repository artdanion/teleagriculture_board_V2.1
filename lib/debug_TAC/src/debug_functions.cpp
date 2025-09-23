
#include <Arduino.h>
#include <debug_functions.h>
#include <init_Board.h>
#include <file_functions.h>
#include <board_credentials.h>


void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
   Serial.printf("\nListing directory: %s\r\n", dirname);

   File root = fs.open(dirname);
   if (!root)
   {
      Serial.println("- failed to open directory");
      return;
   }
   if (!root.isDirectory())
   {
      Serial.println("- not a directory");
      return;
   }

   File file = root.openNextFile();
   while (file)
   {
      if (file.isDirectory())
      {
         Serial.print("  DIR : ");
         Serial.println(file.name());
         if (levels)
         {
            listDir(fs, file.name(), levels - 1);
         }
      }
      else
      {
         Serial.print("  FILE: ");
         Serial.print(file.name());
         Serial.print("\tSIZE: ");
         Serial.println(file.size());
      }
      file = root.openNextFile();
   }
}

void checkLoadedStuff(void)
{
   Serial.println();
   Serial.println("---------------Connector table loaded -----------");
   printConnectors(ConnectorType::I2C);
   printConnectors(ConnectorType::ADC);
   printConnectors(ConnectorType::ONE_WIRE);
   printConnectors(ConnectorType::I2C_5V);
   printConnectors(ConnectorType::SPI_CON);
   printConnectors(ConnectorType::EXTRA);

   Serial.println();
   Serial.println("---------------Config loaded -----------");
   Serial.println("Configuration Values:");

   // Order matches load_Config()
   Serial.printf("BoardID: %d\n", boardID);
   Serial.printf("useBattery: %d\n", useBattery);
   Serial.printf("useDisplay: %d\n", useDisplay);
   Serial.printf("saveDataSDCard: %d\n", saveDataSDCard);

   Serial.printf("useEnterpriseWPA: %d\n", useEnterpriseWPA);
   Serial.printf("useCustomNTP: %d\n", useCustomNTP);
   Serial.printf("useNTP: %d\n", useNTP);
   Serial.printf("rtcEnabled: %d\n", rtcEnabled);

   Serial.printf("API_KEY: %s\n", API_KEY.c_str());
   Serial.printf("upload: %s\n", upload.c_str());
   Serial.printf("upInterval (upload_interval): %d\n", upload_interval);

   Serial.printf("anonym: %s\n", anonym.c_str());
   Serial.printf("user_CA: %s\n", user_CA.c_str());
   Serial.printf("customNTPaddress: %s\n", customNTPaddress.c_str());
   Serial.printf("timeZone: %s\n", timeZone.c_str());

   Serial.printf("OTAA_DEVEUI: %s\n", OTAA_DEVEUI.c_str());
   Serial.printf("OTAA_APPEUI: %s\n", OTAA_APPEUI.c_str());
   Serial.printf("OTAA_APPKEY: %s\n", OTAA_APPKEY.c_str());
   Serial.printf("lora_ADR: %d\n", lora_ADR);

   Serial.printf("apn: %s\n", apn.c_str());
   Serial.printf("gprs_user: %s\n", gprs_user.c_str());
   Serial.printf("gprs_pass: %s\n", gprs_pass.c_str());

   // NEW: LIVE / MQTT / OSC
   Serial.printf("live_mode: %s\n", live_mode.c_str());
   Serial.printf("mqtt_server_ip: %s\n", mqtt_server_ip.c_str());
   Serial.printf("mqtt_topic: %s\n", mqtt_topic.c_str());
   Serial.printf("mqtt port: %d\n", mqtt_port);
   Serial.printf("osc_ip: %s\n", osc_ip.c_str());
   Serial.printf("osc_port: %d\n", static_cast<int>(osc_port));

   Serial.println("\n---------------DEBUG END -----------\n");
}

void printConnectors(ConnectorType typ)
{
   switch (typ)
   {
   case ConnectorType::I2C:
   {
      Serial.println("---- I2C ----");
      for (int i = 0; i < I2C_NUM; i++)
      {
         Serial.print("I2C_");
         Serial.print(i + 1);

         if (I2C_con_table[i].sensorIndex == NO)
         {
            Serial.println("  NO");
         }
         else
         {
            Serial.print("  ");
            Serial.print(allSensors[I2C_con_table[i].sensorIndex].sensor_name);
            Serial.print(" (");
            Serial.print(allSensors[I2C_con_table[i].sensorIndex].con_typ);
            Serial.print(") - ");

            Serial.print("I2C Address: ");
            Serial.print(allSensors[I2C_con_table[i].sensorIndex].possible_i2c_add[I2C_con_table[i].addrIndex]);
            Serial.println();
            Serial.print("       ");

            for (int j = 0; j < allSensors[I2C_con_table[i].sensorIndex].returnCount; j++)
            {
               Serial.print(allSensors[I2C_con_table[i].sensorIndex].measurements[j].data_name);
               Serial.print(": ");
               Serial.print(allSensors[I2C_con_table[i].sensorIndex].measurements[j].value);
               Serial.print(allSensors[I2C_con_table[i].sensorIndex].measurements[j].unit);
               Serial.print(allSensors[I2C_con_table[i].sensorIndex].addr_num);

               Serial.print(", ");
            }

            Serial.println();
         }
      }
   }
   break;

   case ConnectorType::ADC:
   {

      Serial.println("---- ADC ----");
      for (int i = 0; i < ADC_NUM; i++)
      {
         Serial.print("ADC_");
         Serial.print(i + 1);

         if (ADC_con_table[i] == NO)
         {
            Serial.println("  NO");
         }
         else
         {
            Serial.print("  ");
            Serial.print(allSensors[ADC_con_table[i]].sensor_name);
            Serial.print(" (");
            Serial.print(allSensors[ADC_con_table[i]].con_typ);
            Serial.print(") - ");
            for (int j = 0; j < allSensors[ADC_con_table[i]].returnCount; j++)
            {
               Serial.print(allSensors[ADC_con_table[i]].measurements[j].data_name);
               Serial.print(": ");
               Serial.print(allSensors[ADC_con_table[i]].measurements[j].value);
               Serial.print(allSensors[ADC_con_table[i]].measurements[j].unit);
               Serial.print(", ");
            }
            Serial.println();
         }
      }
   }
   break;

   case ConnectorType::ONE_WIRE:
   {
      Serial.println("---- 1-Wire ----");
      for (int i = 0; i < ONEWIRE_NUM; i++)
      {
         Serial.print("1-Wire_");
         Serial.print(i + 1);

         if (OneWire_con_table[i] == NO)
         {
            Serial.println("  NO");
         }
         else
         {
            Serial.print("  ");
            Serial.print(allSensors[OneWire_con_table[i]].sensor_name);
            Serial.print(" (");
            Serial.print(allSensors[OneWire_con_table[i]].con_typ);
            Serial.print(") - ");
            for (int j = 0; j < allSensors[OneWire_con_table[i]].returnCount; j++)
            {
               Serial.print(allSensors[OneWire_con_table[i]].measurements[j].data_name);
               Serial.print(": ");
               Serial.print(allSensors[OneWire_con_table[i]].measurements[j].value);
               Serial.print(allSensors[OneWire_con_table[i]].measurements[j].unit);
               Serial.print(", ");
            }
            Serial.println();
         }
      }
   }
   break;

   case ConnectorType::I2C_5V:
   {
      Serial.println("---- I2C_5V ----");
      Serial.print("I2C_5V");

      if (I2C_5V_con_table[0].sensorIndex == NO)
      {
         Serial.println("  NO");
      }
      else
      {
         Serial.print("  ");
         Serial.print(allSensors[I2C_5V_con_table[0].sensorIndex].sensor_name);
         Serial.print(" (");
         Serial.print(allSensors[I2C_5V_con_table[0].sensorIndex].con_typ);
         Serial.print(") - ");

         Serial.print("I2C Address: ");
         Serial.print(allSensors[I2C_5V_con_table[0].sensorIndex].possible_i2c_add[I2C_5V_con_table[0].addrIndex]);
         Serial.println();
         Serial.print("        ");

         for (int j = 0; j < allSensors[I2C_5V_con_table[0].sensorIndex].returnCount; j++)
         {
            Serial.print(allSensors[I2C_5V_con_table[0].sensorIndex].measurements[j].data_name);
            Serial.print(": ");
            Serial.print(allSensors[I2C_5V_con_table[0].sensorIndex].measurements[j].value);
            Serial.print(allSensors[I2C_5V_con_table[0].sensorIndex].measurements[j].unit);
            Serial.print(", ");
         }
         Serial.println();
      }
   }
   break;

   case ConnectorType::SPI_CON:
   {
      Serial.println("---- SPI CON ----");
      Serial.print("SPI");

      if (SPI_con_table[0] == NO)
      {
         Serial.println("  NO");
      }
      else
      {
         Serial.print("  ");
         Serial.print(allSensors[SPI_con_table[0]].sensor_name);
         Serial.print(" (");
         Serial.print(allSensors[SPI_con_table[0]].con_typ);
         Serial.print(") - ");

         for (int j = 0; j < allSensors[SPI_con_table[0]].returnCount; j++)
         {
            Serial.print(allSensors[SPI_con_table[0]].measurements[j].data_name);
            Serial.print(": ");
            Serial.print(allSensors[SPI_con_table[0]].measurements[j].value);
            Serial.print(allSensors[SPI_con_table[0]].measurements[j].unit);
            Serial.print(", ");
         }
         Serial.println();
      }
   }
   break;

   case ConnectorType::EXTRA:
   {
      Serial.println("---- EXTRA CON ----");
      for (int i = 0; i < EXTRA_NUM; i++)
      {
         Serial.print("EXTRA_");
         Serial.print(i + 1);

         if (EXTRA_con_table[i] == NO)
         {
            Serial.println("  NO");
         }
         else
         {
            Serial.print("  ");
            Serial.print(allSensors[EXTRA_con_table[i]].sensor_name);
            Serial.print(" (");
            Serial.print(allSensors[EXTRA_con_table[i]].con_typ);
            Serial.print(") - ");
            for (int j = 0; j < allSensors[EXTRA_con_table[i]].returnCount; j++)
            {
               Serial.print(allSensors[EXTRA_con_table[i]].measurements[j].data_name);
               Serial.print(": ");
               Serial.print(allSensors[EXTRA_con_table[i]].measurements[j].value);
               Serial.print(allSensors[EXTRA_con_table[i]].measurements[j].unit);
               Serial.print(", ");
            }
            Serial.println();
         }
      }
   }
   break;

   default:
      break;
   }
}

void printProtoSensors(void)
{
   Serial.println("---------------Prototype Sensors loaded -----------");
   for (int z = 0; z < SENSORS_NUM; z++)
   {
      Serial.println();
      Serial.print(allSensors[z].sensor_id);
      Serial.print("  ");
      Serial.print(allSensors[z].sensor_name);
      Serial.print("  ");
      Serial.println(allSensors[z].con_typ);
      Serial.print("         ");
      Serial.print("Measurements: ");
      Serial.println(allSensors[z].returnCount);

      for (size_t j = 0; j < allSensors[z].returnCount; j++)
      {
         Serial.print("         ");
         Serial.print(allSensors[z].measurements[j].data_name);
         Serial.print("  ");
         Serial.print(allSensors[z].measurements[j].value);
         Serial.println(allSensors[z].measurements[j].unit);
      }

      if (allSensors[z].con_typ == "I2C" || allSensors[z].con_typ == "I2C_5V")
      {
         Serial.print("         ");
         Serial.print("Number of I2C Addresses: ");
         Serial.println(allSensors[z].addr_num);

         Serial.print("         ");
         for (size_t k = 0; k <= allSensors[z].addr_num; k++)
         {
            Serial.print(allSensors[z].possible_i2c_add[k]);
            Serial.print("  ");
         }
      }
      Serial.println();
   }
   Serial.println();
}

void printSensors()
{
   for (int i = 0; i < sensorVector.size(); ++i)
   {
      Serial.print("Sensor ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(sensorVector[i].sensor_name);
   }
}

void printMeassurments()
{
   // Loop through all the elements in the vector
   for (int i = 0; i < sensorVector.size(); ++i)
   {
      for (int j = 0; j < sensorVector[i].returnCount; j++)
      {
         Serial.print(sensorVector[i].measurements[j].data_name);
         Serial.print(":  ");
         Serial.println(sensorVector[i].measurements[j].value);
      }
   }
   Serial.print("\nVector elements: ");
   Serial.print(sensorVector.size() + 1);
   Serial.print("\nSize of Vector: ");
   Serial.println(sizeof(sensorVector));
}

void printHex2(unsigned v)
{
   v &= 0xff;
   if (v < 16)
      Serial.print('0');
   Serial.print(v, HEX);
}

void scanI2CDevices(void)
{
   byte error, address;
   int nDevices;

   digitalWrite(SW_3V3, HIGH);
   digitalWrite(SW_5V, HIGH);

   Serial.println("Scanning I2C bus...");
   Wire.begin(I2C_SDA, I2C_SCL, I2C_FREQ);
   nDevices = 0;
   for (address = 1; address < 127; address++)
   {
      Wire.beginTransmission(address);
      error = Wire.endTransmission();

      if (error == 0)
      {
         Serial.print("I2C device found at address 0x");
         if (address < 16)
            Serial.print("0");
         Serial.print(address, HEX);
         Serial.println("  !");

         nDevices++;
      }
      else if (error == 4)
      {
         Serial.print("Unknown error at address 0x");
         if (address < 16)
            Serial.print("0");
         Serial.println(address, HEX);
      }
   }
   if (nDevices == 0)
      Serial.println("No I2C devices found\n");
   else
      Serial.println("I2C scan complete\n");

   digitalWrite(SW_3V3, LOW);
   digitalWrite(SW_5V, LOW);
}