/*            web server args
saved by function:

void save_Config(void);
void save_Connectors(void);

Listing directory: /
  FILE: connectors.json SIZE: 174
  FILE: board_config.json       SIZE: 554

values:

BoardID: 1000
useBattery: 0
useDisplay: 1
saveDataSDCard: 0
useEnterpriseWPA: 0
useCustomNTP: 0
useNTP: 1
API_KEY: xxxxxxxxxxxxxxXXXXXxxxxxxxxxxxxx
upload: WIFI
upload_interval: 2
anonym: anonymus@example.com
user_CA: -----BEGIN CERTIFICATE----- optional -----END CERTIFICATE-----
OTAA_DEVEUI: 0000000000000000
OTAA_APPEUI: 0000000000000000
OTAA_APPKEY: 00000000000000000000000000000000
lora_ADR: 0
apn: 0000
gprs_user: XXXXX
gprs_pass: XXXXX
i2c_1: -1
i2c_3: -1
i2c_2: -1
i2c_4: -1
I2C_5V: -1
adc_1: -1
onewire_1: 6
adc_2: -1
onewire_2: -1
adc_3: -1
onewire_3: 7
upload_interval: 60
system-time: 1970-01-01T01:07
timezone: 810
set-time: 2023-04-01T09:45
use-ntp-server: 1
enable-dst: 1
custom_ntp_enable: 1
custom_ntp: 129.6.15.28
ntp-server: 3
ntp-server-interval: 60
*/

#pragma once

#include "prefs.hpp"
#include "NTP.hpp"
#include "TZ.hpp"
#include <servers.h>

void save_Connectors();
void save_Config();
void setEsp32Time(const char *timeStr);

namespace WiFiManagerNS
{

#include "strings_en.h"

  bool NTPEnabled = false; // overriden by prefs
  bool DSTEnabled = true;
  String TimeConfHTML;
  char systime[64];

  constexpr const char *menuhtml = "<form action='/custom' method='get'><button>Setup Board</button></form><br/>\n";

  WiFiManager *_wifiManager;

  String generateDropdown(String con_typ, int sensor_id);

  void bindServerCallback();

  void init(WiFiManager *manager)
  {
    _wifiManager = manager;
    _wifiManager->setWebServerCallback(bindServerCallback);
    _wifiManager->setCustomMenuHTML(menuhtml);
    _wifiManager->setClass("invert");
    // _wifiManager->wm.setDarkMode(true);
    TZ::loadPrefs();
    prefs::getBool("NTPEnabled", &NTPEnabled, false);
    if (NTPEnabled)
    {
      NTP::loadPrefs();
    }
  }

  void configTime()
  {
    const char *tz = TZ::getTzByLocation(TZ::tzName);
    Serial.printf("\nSetting up time: NTPServer=%s, TZ-Name=%s, TZ=%s\n", NTP::server(), TZ::tzName, tz);
    String tempServer = NTP::server();
    char char_array[tempServer.length() + 1];
    tempServer.toCharArray(char_array, tempServer.length() + 1);
    delay(100);
    TZ::configTimeWithTz(tz, char_array);
  }

  enum Element_t
  {
    HTML_HEAD_START,
    HTML_STYLE,
    HTML_SCRIPT,
    HTML_HEAD_END,
    HTML_PORTAL_OPTIONS,
    HTML_ITEM,
    HTML_FORM_START,
    HTML_FORM_PARAM,
    HTML_FORM_END,
    HTML_SCAN_LINK,
    HTML_SAVED,
    HTML_END,
  };

  constexpr const size_t count = sizeof(Element_t);

  struct TemplateSet_t
  {
    Element_t element;
    const char *content;
  };

  TemplateSet_t templates[] =
      {
          {HTML_HEAD_START, HTTP_HEAD_START},
          {HTML_STYLE, HTTP_STYLE},
          {HTML_SCRIPT, HTTP_SCRIPT},
          {HTML_HEAD_END, HTTP_HEAD_END},
          {HTML_PORTAL_OPTIONS, HTTP_PORTAL_OPTIONS},
          {HTML_ITEM, HTTP_ITEM},
          {HTML_FORM_START, HTTP_FORM_START},
          {HTML_FORM_PARAM, HTTP_FORM_PARAM},
          {HTML_FORM_END, HTTP_FORM_END},
          {HTML_SCAN_LINK, HTTP_SCAN_LINK},
          {HTML_SAVED, HTTP_SAVED},
          {HTML_END, HTTP_END},
  };

  void setTemplate(Element_t element, const char *content)
  {
    templates[element].content = content;
  }

  const char *getTemplate(Element_t element)
  {
    return templates[element].content;
  }

  void handleFavicon()
  {
    _wifiManager->server->send_P(200, "text/html", favicon.c_str(), sizeof(favicon));
  }

  String getSystimeStr()
  {
    static String systimeStr;
    struct timeval tv;
    time_t nowtime;
    struct tm *nowtm;
    gettimeofday(&tv, NULL);
    nowtime = tv.tv_sec;
    nowtm = localtime(&nowtime);
    strftime(systime, sizeof systime, "%Y-%m-%dT%H:%M", nowtm);
    systimeStr = String(systime);
    return systimeStr;
  }

  void handleRoute()
  {
    Serial.println("[HTTP] handle route Custom");

    if (useCustomNTP)
    {
      if (NTP::NTP_Servers.size() == NUM_PREDIFINED_NTP)
      {
        const std::string constStr = customNTPaddress.c_str();
        NTP::NTP_Server newServer = {"Custom NTP Server", constStr};
        NTP::NTP_Servers.push_back(newServer);
      }
    }

    TimeConfHTML = "";
    TimeConfHTML += getTemplate(HTML_HEAD_START);
    TimeConfHTML.replace(FPSTR(T_v), "TeleAgriCulture Board Setup");
    TimeConfHTML += custom_Title_Html;

    TimeConfHTML += "<script>";
    TimeConfHTML += "window.addEventListener('load', function() { var now = new Date(); var offset = now.getTimezoneOffset() * 60000; var adjustedDate = new Date(now.getTime() - offset);";
    TimeConfHTML += "document.getElementById('set-time').value = adjustedDate.toISOString().substring(0,16); });";
    TimeConfHTML += "</script>";

    TimeConfHTML += "<script>";
    TimeConfHTML += "function chooseUploade() { var checked = document.querySelector('input[name=upload]:checked'); var div = document.getElementById('Lora'); var divNTP = document.getElementById('use_NTP'); var divNoNTP = document.getElementById('no_NTP'); var divGSM = document.getElementById('GSM');";
    TimeConfHTML += "if (checked && checked.value == 'LORA') { div.style.display = 'block'; var checkbox = document.getElementById('use-WPA_enterprise'); checkbox.checked = false; divNTP.style.display = 'none'; divNoNTP.style.display = 'block'; divGSM.style.display = 'none'; }";
    TimeConfHTML += "else if (checked && checked.value == 'NO') { div.style.display = 'none'; divNTP.style.display = 'none'; divNoNTP.style.display = 'none'; divGSM.style.display = 'none'; }";
    TimeConfHTML += "else if (checked && checked.value == 'GSM') { div.style.display = 'none'; divNTP.style.display = 'none'; divNoNTP.style.display = 'none'; divGSM.style.display = 'block'; }";
    TimeConfHTML += "else { div.style.display = 'none'; divNTP.style.display = 'block'; divNoNTP.style.display = 'none'; divGSM.style.display = 'none'; } }";
    TimeConfHTML += "</script>";

    TimeConfHTML += "<script type='text/javascript'>";
    TimeConfHTML += "function chooseCustomNTP() {var customNtp = document.getElementById('custom_ntp'); var ntpList = document.getElementById('ntp_list');var checkBox = document.getElementById('custom_ntp_enable');";
    TimeConfHTML += "if (checkBox.checked == true) {customNtp.style.display = 'block'; ntpList.style.display = 'none';}";
    TimeConfHTML += "else {customNtp.style.display = 'none'; ntpList.style.display = 'block';}}";
    TimeConfHTML += "</script>";

    TimeConfHTML += "<script type='text/javascript'>";
    TimeConfHTML += "function chooseNTP() {var useNTP = document.getElementById('ntp_Settings');var noNTP = document.getElementById('no_NTP');var checkBox = document.getElementById('use-ntp-server');";
    TimeConfHTML += "if (checkBox.checked == true) {useNTP.style.display = 'block';noNTP.style.display = 'none';}";
    TimeConfHTML += "else {useNTP.style.display = 'none';noNTP.style.display = 'block';}}";
    TimeConfHTML += "</script>";
    TimeConfHTML += "<script type='text/javascript'>";
    TimeConfHTML += "document.addEventListener('DOMContentLoaded', function () { chooseNTP();});";
    TimeConfHTML += "</script>";
    TimeConfHTML += "<style>strong {color:red;}</style>";

    TimeConfHTML += getTemplate(HTML_STYLE);

    TimeConfHTML += getTemplate(HTML_HEAD_END);
    TimeConfHTML.replace(FPSTR(T_c), "invert"); // add class str

    TimeConfHTML += "<h2>Board Setup</h2>";
    TimeConfHTML += version;
    TimeConfHTML += "<BR><BR>";
    TimeConfHTML += "Board MAC Address: " + WiFi.macAddress();
    TimeConfHTML += "<BR><BR>";
    TimeConfHTML += "<iframe name='dummyframe' id='dummyframe' style='display: none;'></iframe>";

    TimeConfHTML += "<div><form action='/save-tz' target='dummyframe' method='POST'><legend>Please select your data upload method:</legend>";
    TimeConfHTML += "<table style='width:100%'><tr>";
    TimeConfHTML += "<td><input type='radio' id='wificheck' name='upload' value='WIFI' onchange='chooseUploade()' checked /><label for='upload1'> WiFi</label></td>";
    TimeConfHTML += "<td><input type='radio' id='loracheck' name='upload' value='LORA' onchange='chooseUploade()' /><label for='upload2'> LoRa</label></td>";
    TimeConfHTML += "<td><input type='radio' id='nouploadcheck' name='upload' value='NO' onchange='chooseUploade()' /><label for='upload3'> NO upload</label></td>";
    //TimeConfHTML += "<td><input type='radio' id='gsmcheck' name='upload' value='GSM' onchange='chooseUploade()' /><label for='upload4'> GSM</label></td>";
    TimeConfHTML += "</tr></table><br>";

    if (useBattery)
    {
      TimeConfHTML += "<input type='checkbox' id='battery' name='battery' value='1' checked /><label for='battery'> powerd by battery</label><br>";
    }
    else
    {
      TimeConfHTML += "<input type='checkbox' id='battery' name='battery' value='1'/><label for='battery'> powerd by battery</label><br>";
    }

    if (useDisplay)
    {
      TimeConfHTML += "<input type='checkbox' id='display' name='display' value='1' checked /><label for='display'> show display</label><br>";
    }
    else
    {
      TimeConfHTML += "<input type='checkbox' id='display' name='display' value='1'/><label for='display'> show display</label><br>";
    }

    if (saveDataSDCard)
    {
      TimeConfHTML += "<input type='checkbox' id='logtosd' name='logtosd' value='1' checked /><label for='logtosd'> Log to SD Card</label><br>";
    }
    else
    {
      TimeConfHTML += "<input type='checkbox' id='logtosd' name='logtosd' value='1'/><label for='logtosd'> Log to SD Card</label><br>";
    }

    TimeConfHTML += "<BR><BR><label for='up_interval'>Upload Interval:</label>";
    TimeConfHTML += "<select id='up_interval' name='up_interval'>";
    TimeConfHTML += "<option value=2>2 min</option>";
    TimeConfHTML += "<option value=10>10 min</option>";
    TimeConfHTML += "<option value=30>30 min</option>";
    TimeConfHTML += "<option value=60>60 min</option>";
    TimeConfHTML += "</select><br>";

    TimeConfHTML += "</div><BR><div><BR>";

    TimeConfHTML += "<b>WiFi Upload Data</b>";
    TimeConfHTML += "<div><label for='BoardID'>Board ID:</label><input type='text' id='BoardID' name='BoardID' pattern='^(1[0-9]{3}|199[0-9])$' title='Enter 4 digit Board ID' value=" + String(boardID) + " required>";
    TimeConfHTML += "<label for='API_KEY'>API KEY:</label><input type='text' name='API_KEY' pattern='^[A-Za-z0-9]{32}$' title=' Enter Bearer token' value=" + API_KEY + " required>";

    TimeConfHTML += "<div id='Lora' style='display:none'><br><BR><b>LoRa TTN Data</b>";

    TimeConfHTML += "<BR><strong>" + lora_fqz + "</strong><BR><BR>";

    TimeConfHTML += "<label for='OTAA_APPEUI'>OTAA_APPEUI:</label><input type='text' id='OTAA_APPEUI' name='OTAA_APPEUI' pattern='^[0-9A-F]{16}$' title='Enter 8 hexadecimal digits without any prefix or separator' value=" + OTAA_APPEUI + " required>";
    TimeConfHTML += "<label for='OTAA_DEVEUI'>OTAA_DEVEUI:</label><input type='text' id='OTAA_DEVEUI' name='OTAA_DEVEUI' pattern='^[0-9A-F]{16}$' title='Enter 8 hexadecimal digits without any prefix or separator' value=" + OTAA_DEVEUI + " required>";
    TimeConfHTML += "<label for='OTAA_APPKEY'>OTAA_APPKEY:</label><input type='text' id='OTAA_APPKEY' name='OTAA_APPKEY' pattern='^[0-9A-F]{32}$' title='Enter 16 hexadecimal digits without any prefix or separator' value=" + OTAA_APPKEY + " required>";
    TimeConfHTML += "<BR><input type='checkbox' id='ADR' name='ADR' value='1'/><label for='ADR'> use ADR</label>";
    TimeConfHTML += "</div><BR>";

    TimeConfHTML += "<div id='GSM' style='display:none'><br><BR><b>GSM Data</b>";
    TimeConfHTML += "<label for='apn'>APN:</label><input type='text' id='apn' name='apn' value=" + apn + " required>";
    TimeConfHTML += "<label for='gprs_user'>GPRS User:</label><input type='text' id='gprs_user' name='gprs_user' value=" + gprs_user + " required>";
    TimeConfHTML += "<label for='gprs_pass'>GPRS Password:</label><input type='password' id='gprs_pass' name='gprs_pass' value=" + gprs_pass + " required>";
    TimeConfHTML += "</div><BR>";

    //------------- Start Connectors ------- //

    TimeConfHTML += "<BR><h2>Connectors:</h2>";

    TimeConfHTML += "<table style='width:100%'><tbody><tr><td colspan='2'><h3>I2C Connectors</h3></td></tr><tr>";

    TimeConfHTML += "<td><label for='i2c_1'>I2C_1</label>";

    TimeConfHTML += "<select id='I2C_1' name='i2c_1'>";

    TimeConfHTML += generateDropdown("I2C", I2C_con_table[0]);

    TimeConfHTML += "<td><label for='i2c_3'>I2C_3</label>";

    TimeConfHTML += "<select id='I2C_3' name='i2c_3'>";

    TimeConfHTML += generateDropdown("I2C", I2C_con_table[1]);

    TimeConfHTML += "</tr><tr>";

    TimeConfHTML += "<td><label for='i2c_2'>I2C_2</label>";

    TimeConfHTML += "<select id='I2C_2' name='i2c_2'>";

    TimeConfHTML += generateDropdown("I2C", I2C_con_table[2]);

    TimeConfHTML += "<td><label for='i2c_4'>I2C_4</label>";

    TimeConfHTML += "<select id='I2C_4' name='i2c_4'>";

    TimeConfHTML += generateDropdown("I2C", I2C_con_table[3]);

    TimeConfHTML += "</tr></tbody></table>";

    TimeConfHTML += "<table style='width:100%'><tbody><tr><td><h3>ADC Connectors</h3></td>";
    TimeConfHTML += "<td><h3>1-Wire Connectors</h3></td></tr><tr>";

    TimeConfHTML += "<td><label for='adc_1'>ADC_1</label>";

    TimeConfHTML += "<select id='ADC_1' name='adc_1'>";

    TimeConfHTML += generateDropdown("ADC", ADC_con_table[0]);

    TimeConfHTML += "<td><label for='onewire_1'>1-Wire_1</label>";

    TimeConfHTML += "<select id='onewire_1' name='onewire_1'>";

    TimeConfHTML += generateDropdown("ONE_WIRE", OneWire_con_table[0]);

    TimeConfHTML += "</tr><tr><td><label for='adc_2'>ADC_2</label>";

    TimeConfHTML += "<select id='ADC_2' name='adc_2'>";

    TimeConfHTML += generateDropdown("ADC", ADC_con_table[1]);

    TimeConfHTML += "<td><label for='onewire_2'>1-Wire_2</label>";

    TimeConfHTML += "<select id='onewire_2' name='onewire_2'>";

    TimeConfHTML += generateDropdown("ONE_WIRE", OneWire_con_table[1]);

    TimeConfHTML += "</tr><tr><td><label for='adc_3'>ADC_3</label>";

    TimeConfHTML += "<select id='ADC_3' name='adc_3'>";

    TimeConfHTML += generateDropdown("ADC", ADC_con_table[2]);

    TimeConfHTML += "<td><label for='onewire_3'>1-Wire_3</label>";

    TimeConfHTML += "<select id='onewire_3' name='onewire_3'>";

    TimeConfHTML += generateDropdown("ONE_WIRE", OneWire_con_table[2]);
    TimeConfHTML += "</tr></tbody><label for='I2C_5V'>I2C_5V Con</label><select id='I2C_5V' name='I2C_5V'>";
    TimeConfHTML += generateDropdown("I2C_5V", I2C_5V_con_table[0]);
    TimeConfHTML += " </table><BR>";

    TimeConfHTML += "<BR><h2>Time Settings</h2>";

    String systimeStr = getSystimeStr();

    TimeConfHTML += "<label for='ntp-server'>System Time ";

    TimeConfHTML += "<input readonly style=width:auto name='system-time' type='datetime-local' value='" + systimeStr + "'>";
    TimeConfHTML += " <button onclick=location.reload() style=width:auto type=button> Refresh </button></label><br>";

    TimeConfHTML += "<label for='timezone'>Time Zone</label>";
    TimeConfHTML += "<select id='timezone' name='timezone'>";
    for (int i = 0; i < TZ::zones(); i += 2)
    {
      bool selected = (strcmp(TZ::tzName, TZ::timezones[i]) == 0);
      TimeConfHTML += "<option value='" + String(i) + "'" + String(selected ? "selected" : "") + ">" + String(TZ::timezones[i]) + "</option>";
    }
    TimeConfHTML += "</select><br>";

    TimeConfHTML += "<div id='no_NTP'>";
    TimeConfHTML += "<label for='set-time'>Set Time ";
    TimeConfHTML += "<input style=width:auto name='set-time' id='set-time' type='datetime-local' value='" + systimeStr + "'>";
    TimeConfHTML += "</div>";

    TimeConfHTML += "<div id='use_NTP'>";
    TimeConfHTML += "<label for='use-ntp-server'>Enable NTP Client</label> ";
    TimeConfHTML += "<input value='1' type='checkbox' name='use-ntp-server' id='use-ntp-server' " + String(NTPEnabled ? "checked" : "") + " onchange='chooseNTP()'>";
    TimeConfHTML += "<br>";

    TimeConfHTML += "<div id='ntp_Settings' style='display:none;'><h2>NTP Client Setup</h2><label for='enable-dst'>Auto-adjust clock for DST </label>";
    TimeConfHTML += "<input value='1' type='checkbox' name='enable-dst' id='enable-dst'" + String(DSTEnabled ? "checked" : "") + "><br>";
    TimeConfHTML += "<label for='custom_ntp_enable'>Custom NTP Server </label>";
    TimeConfHTML += "<input value='1' type='checkbox' name='custom_ntp_enable' id='custom_ntp_enable' onchange='chooseCustomNTP()'><br><br>";

    TimeConfHTML += "<label for='ntp-server'>Server:</label>";

    TimeConfHTML += "<div id='custom_ntp' style='display:none;'>please enter a valide NTP Server IP Address:<br><input type='text' pattern='[A-Za-z0-9\\\\.]{1,15}'";
    TimeConfHTML += "title='Please enter a valid NTP server address' name='custom_ntp' value='" + customNTPaddress + "'></div>";

    TimeConfHTML += "<div id='ntp_list'><select id='ntp-server-list' name='ntp-server'>";
    size_t servers_count = NTP::NTP_Servers.size();
    uint8_t server_id = NTP::getServerId();
    for (int i = 0; i < servers_count; i++)
    {
      TimeConfHTML += "<option value='" + String(i) + "'" + String(i == server_id ? "selected" : "") + ">";
      TimeConfHTML += NTP::NTP_Servers[i].name.c_str();
      TimeConfHTML += "(";
      TimeConfHTML += NTP::NTP_Servers[i].addr.c_str();
      TimeConfHTML += ")</option>";
    }
    TimeConfHTML += "</select></div><br>";

    TimeConfHTML += "<label for='ntp-server-interval'>Sync interval:</label>";
    TimeConfHTML += "<select id='ntp-server-interval' name='ntp-server-interval'>";
    TimeConfHTML += "<option value=60>Hourly</option>";
    TimeConfHTML += "<option value=14400>Daily</option>";
    TimeConfHTML += "<option value=10080>Weekly</option>";
    TimeConfHTML += "</select></div><br>";

    TimeConfHTML += "</div>";

    TimeConfHTML += "<button type=submit>Submit</button>";
    TimeConfHTML += "</form></div><BR>";
    TimeConfHTML += getTemplate(HTML_END);

    _wifiManager->server->send_P(200, "text/html", TimeConfHTML.c_str(), TimeConfHTML.length());

    // Serial.print(TimeConfHTML); // debug HTML output

    TimeConfHTML = String();
  }

  void handleValues()
  {
    bool success = true;
    bool _NTPEnabled = NTPEnabled;

    Serial.println("[HTTP] handle route Values");

    if (_wifiManager->server->hasArg("use-ntp-server"))
    {
      String UseNtpServer = _wifiManager->server->arg("use-ntp-server");
      log_d("UseNtpServer: %s", UseNtpServer.c_str());
      uint8_t useNtpServer = atoi(UseNtpServer.c_str());
      NTPEnabled = useNtpServer == 1;
      useNTP = true;
    }
    else
    {
      NTPEnabled = false;
      useNTP = false;
    }

    if (_NTPEnabled != NTPEnabled)
    {
      prefs::setBool("NTPEnabled", NTPEnabled);
    }

    if (_wifiManager->server->hasArg("custom_ntp_enable"))
    {
      uint8_t useC_NTP = atoi((_wifiManager->server->arg("custom_ntp_enable")).c_str());
      useCustomNTP = useC_NTP == 1;
    }
    else
    {
      useCustomNTP = false;
    }

    if (_wifiManager->server->hasArg("custom_ntp"))
    {
      customNTPaddress = _wifiManager->server->arg("custom_ntp").c_str();
      if (NTP::NTP_Servers.size() > NUM_PREDIFINED_NTP)
      {
        NTP::NTP_Servers.pop_back();
      }
      const std::string constStr = customNTPaddress.c_str();
      NTP::NTP_Server newServer = {"Custom NTP", constStr};
      NTP::NTP_Servers.push_back(newServer);
    }

    if (_wifiManager->server->hasArg("timezone"))
    {
      String timezoneStr = _wifiManager->server->arg("timezone");
      log_d("timezoneStr: %s", timezoneStr.c_str());
      size_t tzidx = atoi(timezoneStr.c_str());
      String timezone = TZ::defaultTzName;
      if (tzidx < TZ::zones())
      {
        timezone = String(TZ::timezones[tzidx]);
        log_d("timezone: %s", timezone.c_str());
      }

      TZ::setTzName(timezone.c_str());
      const char *tz = TZ::getTzByLocation(TZ::tzName);
      String tempServer = NTP::server();
      char char_array[tempServer.length() + 1];
      tempServer.toCharArray(char_array, tempServer.length() + 1);
      TZ::configTimeWithTz(tz, char_array);
      timeZone = tz;
    }

    if (NTPEnabled)
    {
      // also collect tz/server data
      if (_wifiManager->server->hasArg("ntp-server"))
      {
        String NtpServer = _wifiManager->server->arg("ntp-server");
        log_d("NtpServer: %s", NtpServer.c_str());
        uint8_t server_id = atoi(NtpServer.c_str());
        if (useCustomNTP)
        {
          Serial.println(server_id);
          if (!NTP::setServer(CUSTOM_NTP_INDEX))
            success = false;
        }
        else
        {
          if (!NTP::setServer(server_id))
            success = false;
        }
      }

      if (_wifiManager->server->hasArg("ntp-server-interval"))
      {
        String NtpServerInterval = _wifiManager->server->arg("ntp-server-interval");
        log_d("NtpServerInterval: %s", NtpServerInterval.c_str());
        int serverInterval = atoi(NtpServerInterval.c_str());
        switch (serverInterval)
        {
        case 60:
        case 14400:
        case 10080:
          break;
        default:
          serverInterval = 14400;
        }
        NTP::setSyncDelay(serverInterval);
      }
    }

    if (_wifiManager->server->hasArg("BoardID"))
    {
      boardID = atoi(_wifiManager->server->arg("BoardID").c_str());
    }

    if (_wifiManager->server->hasArg("i2c_1"))
    {
      I2C_con_table[0] = atoi(_wifiManager->server->arg("i2c_1").c_str());
    }

    if (_wifiManager->server->hasArg("i2c_2"))
    {
      I2C_con_table[1] = atoi(_wifiManager->server->arg("i2c_2").c_str());
    }

    if (_wifiManager->server->hasArg("i2c_3"))
    {
      I2C_con_table[2] = atoi(_wifiManager->server->arg("i2c_3").c_str());
    }

    if (_wifiManager->server->hasArg("i2c_4"))
    {
      I2C_con_table[3] = atoi(_wifiManager->server->arg("i2c_4").c_str());
    }

    if (_wifiManager->server->hasArg("I2C_5V"))
    {
      I2C_5V_con_table[0] = atoi(_wifiManager->server->arg("I2C_5V").c_str());
    }

    if (_wifiManager->server->hasArg("adc_1"))
    {
      ADC_con_table[0] = atoi(_wifiManager->server->arg("adc_1").c_str());
    }

    if (_wifiManager->server->hasArg("adc_2"))
    {
      ADC_con_table[1] = atoi(_wifiManager->server->arg("adc_2").c_str());
    }

    if (_wifiManager->server->hasArg("adc_3"))
    {
      ADC_con_table[2] = atoi(_wifiManager->server->arg("adc_3").c_str());
    }

    if (_wifiManager->server->hasArg("onewire_1"))
    {
      OneWire_con_table[0] = atoi(_wifiManager->server->arg("onewire_1").c_str());
    }

    if (_wifiManager->server->hasArg("onewire_2"))
    {
      OneWire_con_table[1] = atoi(_wifiManager->server->arg("onewire_2").c_str());
    }

    if (_wifiManager->server->hasArg("onewire_3"))
    {
      OneWire_con_table[2] = atoi(_wifiManager->server->arg("onewire_3").c_str());
    }

    if (_wifiManager->server->hasArg("battery"))
    {
      uint8_t useB = atoi((_wifiManager->server->arg("battery")).c_str());
      useBattery = useB == 1;
    }
    else
    {
      useBattery = false;
    }

    if (_wifiManager->server->hasArg("display"))
    {
      uint8_t useD = atoi((_wifiManager->server->arg("display")).c_str());
      useDisplay = useD == 1;
    }
    else
    {
      useDisplay = false;
    }

    if (_wifiManager->server->hasArg("logtosd"))
    {
      uint8_t useL = atoi((_wifiManager->server->arg("logtosd")).c_str());
      saveDataSDCard = useL == 1;
    }
    else
    {
      saveDataSDCard = false;
    }

    if (_wifiManager->server->hasArg("up_interval"))
    {
      String upInterval = _wifiManager->server->arg("up_interval");
      upload_interval = atoi(upInterval.c_str());
    }

    if (_wifiManager->server->hasArg("upload"))
    {
      upload = _wifiManager->server->arg("upload").c_str();
    }

    if (_wifiManager->server->hasArg("ADR"))
    {
      uint8_t use_ADR = atoi((_wifiManager->server->arg("ADR")).c_str());
      lora_ADR = use_ADR == 1;
    }
    else
    {
      lora_ADR = false;
    }

    if (_wifiManager->server->hasArg("API_KEY"))
    {
      API_KEY = _wifiManager->server->arg("API_KEY").c_str();
    }

    if (_wifiManager->server->hasArg("ANONYMUS"))
    {
      anonym = _wifiManager->server->arg("ANONYMUS").c_str();
    }

    if (_wifiManager->server->hasArg("certificate"))
    {
      user_CA = _wifiManager->server->arg("certificate").c_str();
    }

    if (_wifiManager->server->hasArg("apn"))
    {
      apn = _wifiManager->server->arg("apn").c_str();
    }

    if (_wifiManager->server->hasArg("gprs_user"))
    {
      gprs_user = _wifiManager->server->arg("gprs_user").c_str();
    }

    if (_wifiManager->server->hasArg("gprs_pass"))
    {
      gprs_pass = _wifiManager->server->arg("gprs_pass").c_str();
    }

    if (_wifiManager->server->hasArg("OTAA_DEVEUI"))
    {
      OTAA_DEVEUI = _wifiManager->server->arg("OTAA_DEVEUI").c_str();
      loraChanged = true;
    }

    if (_wifiManager->server->hasArg("OTAA_APPEUI"))
    {
      OTAA_APPEUI = _wifiManager->server->arg("OTAA_APPEUI").c_str();
      loraChanged = true;
    }

    if (_wifiManager->server->hasArg("OTAA_APPKEY"))
    {
      OTAA_APPKEY = _wifiManager->server->arg("OTAA_APPKEY").c_str();
      loraChanged = true;
    }

    if (_wifiManager->server->hasArg("set-time"))
    {
      setTime_value = _wifiManager->server->arg("set-time").c_str();
      if (!(upload == "WIFI"))
      {
        setEsp32Time(setTime_value.c_str());
      }
    }

    SPI_con_table[0] = NO;
    EXTRA_con_table[0] = NO;
    EXTRA_con_table[1] = NO;

    save_Connectors();
    save_Config();

    delay(200);

    const char *successResp = "<script>parent.location.href = '/';</script>";
    const char *failureResp = "<script>parent.alert('fail');</script>";

    _wifiManager->server->send(200, "text/html", success ? successResp : failureResp);

    delay(200);
    ESP.restart();
  }

  void bindServerCallback()
  {
    _wifiManager->server->on("/custom", handleRoute);
    _wifiManager->server->on("/save-tz", handleValues);
    _wifiManager->server->on("/favicon.ico", handleFavicon); // changed to imbedded png/base64 link
  }

  // function that takes a connector type and a sensor id as parameters and generates dropdown menu
  String generateDropdown(String con_typ, int sensor_id)
  {
    // create an empty string to store the dropdown
    String dropdown = "";

    // add the 'NO' option
    dropdown += "<option value='-1'>NO</option>";

    // loop through all sensors
    for (int i = 0; i < SENSORS_NUM; i++)
    {
      // check if the current sensor matches the connector type
      if (allSensors[i].con_typ == con_typ)
      {
        // check if the current sensor matches the sensor id
        if (int(allSensors[i].sensor_id) == (sensor_id + 1)) // id starts at 1 // enum at 0
        {
          // add the selected attribute to that option
          dropdown += "<option value='" + String(int(allSensors[i].sensor_id) - 1) + "' selected>" + allSensors[i].sensor_name + "</option>";
        }
        else
        {
          // otherwise, add a normal option without the selected attribute
          dropdown += "<option value='" + String(int(allSensors[i].sensor_id) - 1) + "'>" + allSensors[i].sensor_name + "</option>";
        }
      }
    }

    // add the closing tag of the select element and the table cell element
    dropdown += "</select></td>";

    // return the dropdown string
    return dropdown;
  }

};
