
#include <Arduino.h>
#include <web_functions.h>

void handleRoot()
{
   String temp;
   int sec = millis() / 1000;
   int min = sec / 60;
   int hr = min / 60;

   temp = "<!DOCTYPE html><html>\
           <head>\
             <meta http-equiv='refresh' content='15'/>\
             <title>ESP32 Demo</title>\
             <meta name='viewport' content='width=device-width, initial-scale=1'>\
             <style>\
               html { font-family: Helvetica; min-height: 100%; text-align: center; background: linear-gradient(to bottom, #00CC99, #009999); color: #fff;}\
               table { margin: 10px auto; width: 100%; max-width: 600px; border-spacing: 10px; border-collapse: collapse; box-shadow: 0 0 8px rgba(0, 0, 0, 0.3); background-color: rgba(255, 255, 255, 0.4); }\
               th, td { padding: 8px; text-align: left; }\
               tr:not(:last-child) td { border-bottom: 1px solid #d9d9d9; }\
               th { background-color: #206040; color: #fff; }\
               tr:nth-child(even) { background-color: #00b386; }\
               tr:nth-child(odd) { background-color: #00cc99; }\
             </style>\
           </head>\
           <body>\
             <h1>TeleAgriCulture Board 2.1</h1>\
             <p>Uptime: " +
          String(hr) + ":" + String(min % 60) + ":" + String(sec % 60) + "</p>";
   temp += version;
   temp += "<br>BoardID: ";
   temp += boardID;
   temp += "<br>Upload: ";
   temp += upload;
   temp += measurementsTable();
   temp += connectorTable();
   temp += "</body></html>";

   server.send(200, "text/html", temp.c_str());
}

void handleNotFound()
{
   String message = "File Not Found\n\n";
   message += "URI: ";
   message += server.uri();
   message += "\nMethod: ";
   message += (server.method() == HTTP_GET) ? "GET" : "POST";
   message += "\nArguments: ";
   message += server.args();
   message += "\n";

   for (uint8_t i = 0; i < server.args(); i++)
   {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
   }

   server.send(404, "text/plain", message);
}

void drawGraph()
{
   String out = "";
   char temp[100];
   out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
   out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
   out += "<g stroke=\"black\">\n";
   int y = rand() % 130;
   for (int x = 10; x < 390; x += 10)
   {
      int y2 = rand() % 130;
      sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
      out += temp;
      y = y2;
   }
   out += "</g>\n</svg>\n";

   server.send(200, "image/svg+xml", out);
}

String measurementsTable()
{
   String table = "<h2>Measurements</h2><table>";
   table += "<tr><th>Data Name</th><th>Value</th><th>Unit</th></tr>";

   for (int i = 0; i < show_measurements.size(); i++)
   {
      table += "<tr>";
      table += "<td>" + show_measurements[i].data_name + "</td>";
      table += "<td>";

      if (!isnan(show_measurements[i].value))
      {
         table += String(show_measurements[i].value);
      }
      else
      {
         table += "NAN";
      }

      table += "</td>";
      table += "<td>";

      if (!(show_measurements[i].unit == "°C"))
      {
         table += show_measurements[i].unit;
      }
      else
      {
         table += "&deg;C";
      }

      table += "</td>";
      table += "</tr>";
   }

   table += "</table>";

   return table;
}

String connectorTable()
{
   String table = "<h2>Connectors</h2><table>";

   // I2C Connectors
   for (int i = 0; i < I2C_NUM; i++)
   {
      table += "<tr><td style='text-align: center;'>I2C_" + String(i + 1) + "</td>";

      if (I2C_con_table[i].sensorIndex != NO)
      {
         table += "<td>" + allSensors[I2C_con_table[i].sensorIndex].sensor_name + "</td>";
      }
      else
      {
         table += "<td>NO</td>";
      }

      table += "</tr>";
   }

   // ADC Connectors
   for (int i = 0; i < ADC_NUM; i++)
   {
      table += "<tr><td style='text-align: center;'>ADC_" + String(i + 1) + "</td>";

      if (ADC_con_table[i] != NO)
      {
         table += "<td>" + allSensors[ADC_con_table[i]].sensor_name + "</td>";
      }
      else
      {
         table += "<td>NO</td>";
      }

      table += "</tr>";
   }

   // 1-Wire Connectors
   for (int i = 0; i < ONEWIRE_NUM; i++)
   {
      table += "<tr><td style='text-align: center;'>1-Wire_" + String(i + 1) + "</td>";

      if (OneWire_con_table[i] != NO)
      {
         table += "<td>" + allSensors[OneWire_con_table[i]].sensor_name + "</td>";
      }
      else
      {
         table += "<td>NO</td>";
      }

      table += "</tr>";
   }

   table += "</table>";
   return table;
}
