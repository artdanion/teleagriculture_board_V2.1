#include "def_Board.h"

// Connector tables
I2CConnectorConf I2C_con_table[I2C_NUM] = {{-1, -1},
                                           {-1, -1},
                                           {-1, -1},
                                           {-1, -1}};
int ADC_con_table[ADC_NUM] = {-1, -1, -1};
int OneWire_con_table[ONEWIRE_NUM] = {-1, -1, -1};
int SPI_con_table[SPI_NUM] = {-1};
I2CConnectorConf I2C_5V_con_table[I2C_5V_NUM] = {{-1, -1}};
int EXTRA_con_table[EXTRA_NUM] = {-1, -1};

// Settings
String customNTPaddress = "129.6.15.28";

bool useBattery = false;
bool useDisplay = true;
bool saveDataSDCard = false;
bool useEnterpriseWPA = false;
bool useNTP = false;
bool rtcEnabled = false;
bool useCustomNTP = false;
bool loraChanged = false;
bool webpage = false;
bool lora_ADR = false;

int upload_interval = 60;

String upload = "WIFI";
String live_mode="MQTT";
String anonym = "anonymus@example.com";
String user_CA = "-----BEGIN CERTIFICATE----- optional -----END CERTIFICATE-----";
String setTime_value = "";
String timeZone = "";

String hostname = "TeleAgriCulture Board";

const char GET_Time_SERVER[30] = "www.teleagriculture.org";
String GET_Time_Address = "https://www.teleagriculture.org";
const int SSL_PORT = 443;
const unsigned long TIMEOUT = 3500;

// static osjob_t sendjob;
uint8_t dev_eui[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t app_eui[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t app_key[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
