# teleAgriCulture Board V2.1

## Getting started

Board Credentials are in /include/board_credentials.h (BoardID, API_KEY and LORA credentials)
For defines, GPIOs and implemented Sensors, see /include/sensor_Board.hpp
 
Config Portal Access Point:   SSID: TeleAgriCulture Board   pasword: enter123
 
<mark>!! to build this project, take care that board_credentials.h is in the include folder (gets ignored by git)</mark>
 
main() handles Config Accesspoint, WiFi, LoRa, load, save, time and display UI
 
Global vector to store connected Sensor data:
`std::vector<Sensor> sensorVector;`
 
Sensor Data Name: `sensorVector[i].measurements[j].data_name`    --> in order of apperiance (temp, temp1, temp2 ...)
Sensor Value:     `sensorVector[i].measurements[j].value`
 