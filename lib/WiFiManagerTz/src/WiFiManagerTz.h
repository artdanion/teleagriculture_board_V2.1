/*            web server args
saved by function:

void save_Config(void);
void save_Connectors(void);

Listing directory: /
  FILE: connectors.json SIZE: 174
  FILE: board_config.json       SIZE: 554

Server Arguments
=== RECEIVED FORM DATA ===
Arg 0: upload = 'LIVE'
Arg 1: display = '1'
Arg 2: up_interval = '2'
Arg 3: BoardID = '1000'
Arg 4: API_KEY = 'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx'
Arg 5: OTAA_APPEUI = '0000000000000000'
Arg 6: OTAA_DEVEUI = '0000000000000000'
Arg 7: OTAA_APPKEY = '00000000000000000000000000000000'
Arg 8: apn = '0000'
Arg 9: gprs_user = 'XXXXX'
Arg 10: gprs_pass = 'XXXXX'
Arg 11: live_mqtt = '1'
Arg 12: instant_upload = '1'
Arg 13: mqtt_server_ip = '158.255.212.248'
Arg 14: mqtt_topic = '/TAC/'
Arg 15: mqtt_port = '1883'
Arg 16: osc_ip = '192.168.242.120'
Arg 17: osc_port = '5000'
Arg 18: i2c_1 = '0'
Arg 19: i2c_1_addr = '0'
Arg 20: i2c_3 = '24'
Arg 21: i2c_3_addr = '0'
Arg 22: i2c_2 = '23'
Arg 23: i2c_2_addr = '0'
Arg 24: i2c_4 = '10'
Arg 25: i2c_4_addr = '0'
Arg 26: I2C_5V = '-1'
Arg 27: adc_1 = '-1'
Arg 28: onewire_1 = '22'
Arg 29: adc_2 = '-1'
Arg 30: onewire_2 = '-1'
Arg 31: adc_3 = '-1'
Arg 32: onewire_3 = '-1'
Arg 33: system-time = '2025-09-24T12:34'
Arg 34: timezone = '810'
Arg 35: set-time = '2025-09-24T12:34'
Arg 36: enable-dst = '1'
Arg 37: custom_ntp = '129.6.15.28'
Arg 38: ntp-server = '0'
Arg 39: ntp-server-interval = '60'
=== END FORM DATA ===
*/

#pragma once

#include "prefs.hpp"
#include "NTP.hpp"
#include "TZ.hpp"
#include <servers.h>
#include <time_functions.h>

#define MAX_FOUND_I2C 16
#define DEBUG_PRINT false

void save_Connectors();
void save_Config();

namespace WiFiManagerNS
{

#include "strings_en.h"

  bool NTPEnabled = false; // overriden by prefs
  bool DSTEnabled = true;
  String TimeConfHTML;
  char systime[64];

  String foundI2C[MAX_FOUND_I2C];
  int foundCount = 0;

  constexpr const char *menuhtml = "<form action='/custom' method='get'><button>Setup Board</button></form><br/>\n";

  WiFiManager *_wifiManager;

  String generateI2CTable();
  String generateDropdown(const String &con_typ, const int &conf, const String &formFieldName);

  void handleTimezoneSettings();
  void handleBoardSettings();
  void handleI2CSettings();
  void handleADCSettings();
  void handleOneWireSettings();

  void handlePeripheralSettings();
  void handleUploadSettings();
  void handleLoraSettings();
  void handleTimeSetting();
  void updateRtcStatus();
  void handleLiveSettings();

  void handleSavedPage();
  void handleReboot();

  void bindServerCallback();

  static inline const Sensor *findSensorByTypeIndex(const String &con_typ, int selectedIndex);
  static void appendSensorCard(String &html, const String &title,
                               const String &con_typ, int selectedIndex, int addrIndex = -1);

  bool isFound(String addr);
  bool getBoolArg(const char *name);
  int getIntArg(const char *name, int defaultValue = 0);
  String getStringArg(const char *name, const String &defaultValue = "");

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
    _wifiManager->server->send_P(200, "text/html", favicon.c_str(), favicon.length());
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

  static void beginSensorsCard(String &html)
  {
    html += F(
        "<div class='card'>"
        "<div class='card-h'>Saved Sensors</div>"
        "<div class='card-b'>"
        "<table class='tbl'>"
        "<thead><tr>"
        "<th>CON</th><th>Typ</th><th>Sensor</th><th>I2C-Adr</th><th>Status</th>"
        "</tr></thead><tbody>");
  }

  // einmal nach allen Zeilen aufrufen
  static void endSensorsCard(String &html)
  {
    html += F(
        "</tbody></table>"
        "</div>"
        "</div>");
  }

  void handleRoute()
  {
    Serial.println("[HTTP] handle route Custom / Setup Page");

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

    // Script to set initial time
    TimeConfHTML += "<script>";
    TimeConfHTML += "window.addEventListener('load', function(){"
                    "var now=new Date();var offset=now.getTimezoneOffset()*60000;"
                    "var adjustedDate=new Date(now.getTime()-offset);"
                    "var el=document.getElementById('set-time'); if(el){el.value=adjustedDate.toISOString().substring(0,16);}"
                    "});";
    TimeConfHTML += "</script>";

    // ===== Unified UI logic (show/hide WIFI/LORA/GSM/LIVE + LIVE sub-forms) =====
    TimeConfHTML += "<script>";
    TimeConfHTML += "function show(e){if(e)e.style.display='block'};";
    TimeConfHTML += "function hide(e){if(e)e.style.display='none'};";
    TimeConfHTML += "function setLiveRequired(mq,os){"
                    "var mi=document.getElementById('mqtt_server_ip');"
                    "var mt=document.getElementById('mqtt_topic');"
                    "var mp=document.getElementById('mqtt_port');"
                    "var oi=document.getElementById('osc_ip');"
                    "var op=document.getElementById('osc_port');"
                    "if(mi)mi.required=!!mq; if(mt)mt.required=!!mq;"
                    "if(mp) mp.required=!!mq;"
                    "if(oi)oi.required=!!os; if(op)op.required=!!os;"
                    "}";
    TimeConfHTML += "function chooseLive(){"
                    "var cbM=document.getElementById('live_mqtt');"
                    "var cbO=document.getElementById('live_osc');"
                    "var m=document.getElementById('LIVE_MQTT');"
                    "var o=document.getElementById('LIVE_OSC');"
                    "if(this&&this.id==='live_mqtt'&&cbM&&cbM.checked){if(cbO)cbO.checked=false;}"
                    "if(this&&this.id==='live_osc' &&cbO&&cbO.checked){if(cbM)cbM.checked=false;}"
                    "if(m)m.style.display=(cbM&&cbM.checked)?'block':'none';"
                    "if(o)o.style.display=(cbO&&cbO.checked)?'block':'none';"
                    "setLiveRequired(cbM&&cbM.checked, cbO&&cbO.checked);"
                    "}";
    TimeConfHTML += "function chooseUploade(){"
                    "var sel=document.querySelector('input[name=upload]:checked');"
                    "var WIFI=document.getElementById('WIFI');"
                    "var Lora=document.getElementById('Lora');"
                    "var GSM=document.getElementById('GSM');"
                    "var LIVE=document.getElementById('LIVE');"
                    "var useNTP=document.getElementById('use_NTP');"
                    "var noNTP=document.getElementById('no_NTP');"
                    "hide(WIFI);hide(Lora);hide(GSM);hide(LIVE);"
                    "if(!sel){hide(useNTP);hide(noNTP);return;}"
                    "switch(sel.value){"
                    " case 'WIFI': show(WIFI); show(useNTP); hide(noNTP); break;"
                    " case 'LORA': show(Lora); hide(useNTP); show(noNTP);"
                    "              var cb=document.getElementById('use-ntp-server'); if(cb)cb.checked=false;"
                    "              break;"
                    " case 'GSM' : show(GSM);  hide(useNTP); hide(noNTP); break;"
                    " case 'LIVE': show(LIVE); show(useNTP); hide(noNTP); break;"
                    " case 'NO'  : hide(useNTP); hide(noNTP); break;"
                    " default    : show(useNTP); hide(noNTP);"
                    "}"
                    "if(sel.value!=='LIVE'){ setLiveRequired(false,false); }"
                    "}";
    TimeConfHTML += "document.addEventListener('DOMContentLoaded',function(){"
                    "var cbM=document.getElementById('live_mqtt');"
                    "var cbO=document.getElementById('live_osc');"
                    "if(cbM)cbM.addEventListener('change',chooseLive);"
                    "if(cbO)cbO.addEventListener('change',chooseLive);"
                    "chooseUploade();"
                    "chooseNTP();"
                    "chooseLive();"
                    "});";
    TimeConfHTML += "</script>";

    // Script to choose custom NTP settings
    TimeConfHTML += "<script type='text/javascript'>"
                    "function chooseCustomNTP(){"
                    "var customNtp=document.getElementById('custom_ntp');"
                    "var ntpList=document.getElementById('ntp_list');"
                    "var checkBox=document.getElementById('custom_ntp_enable');"
                    "if(checkBox&&checkBox.checked){customNtp.style.display='block';ntpList.style.display='none';}"
                    "else{customNtp.style.display='none';ntpList.style.display='block';}"
                    "}"
                    "</script>";

    // Script to choose NTP settings
    TimeConfHTML += "<script type='text/javascript'>"
                    "function chooseNTP(){"
                    "var useNTP=document.getElementById('ntp_Settings');"
                    "var noNTP=document.getElementById('no_NTP');"
                    "var checkBox=document.getElementById('use-ntp-server');"
                    "if(checkBox&&checkBox.checked){useNTP.style.display='block';noNTP.style.display='none';}"
                    "else{useNTP.style.display='none';noNTP.style.display='block';}"
                    "}"
                    "</script>";

    // Address select helper
    TimeConfHTML += "<script>"
                    "function updateAddrSelect(sensorSelect,addrSelectId){"
                    "let opt=sensorSelect.options[sensorSelect.selectedIndex];"
                    "let addrs=opt.dataset.addrs?opt.dataset.addrs.split(','):[];"
                    "let addrSelect=document.getElementById(addrSelectId);"
                    "if(!addrSelect)return;"
                    "let preselect=addrSelect.dataset.selectedAddr?parseInt(addrSelect.dataset.selectedAddr):-1;"
                    "addrSelect.innerHTML='';"
                    "addrs.forEach((a,idx)=>{var option=document.createElement('option');"
                    "option.value=idx; option.text=a; if(idx===preselect)option.selected=true; addrSelect.add(option);});"
                    "}"
                    "document.addEventListener('DOMContentLoaded',()=>{"
                    "document.querySelectorAll(\"select[id$='_addr']\").forEach(addrSelect=>{"
                    "let sensorSelectId=addrSelect.id.replace('_addr','');"
                    "let sensorSelect=document.getElementById(sensorSelectId);"
                    "if(sensorSelect){updateAddrSelect(sensorSelect,addrSelect.id);}"
                    "});"
                    "});"
                    "</script>";

    TimeConfHTML += "<style>strong{color:red;}</style>";
    TimeConfHTML += getTemplate(HTML_STYLE);
    TimeConfHTML += getTemplate(HTML_HEAD_END);
    TimeConfHTML.replace(FPSTR(T_c), "invert");

    TimeConfHTML += "<h2>Board Setup</h2>";
    TimeConfHTML += version;
    TimeConfHTML += "<BR><BR>";
    TimeConfHTML += "Board MAC Address: " + WiFi.macAddress();
    TimeConfHTML += "<BR><BR>";

    TimeConfHTML += "<div><form action='/save-tz' method='POST'> <legend>Please select your data upload method:</legend>";
    TimeConfHTML += "<table style='width:100%'><tr>";

    TimeConfHTML += "<td><input type='radio' id='wificheck' name='upload' value='WIFI' onchange='chooseUploade()' ";
    TimeConfHTML += (upload == "WIFI") ? "checked " : "";
    TimeConfHTML += "/><label for='wificheck'> WiFi</label></td>";

    TimeConfHTML += "<td><input type='radio' id='loracheck' name='upload' value='LORA' onchange='chooseUploade()' ";
    TimeConfHTML += (upload == "LORA") ? "checked " : "";
    TimeConfHTML += "/><label for='loracheck'> LoRa</label></td>";

    TimeConfHTML += "<td><input type='radio' id='nouploadcheck' name='upload' value='NO' onchange='chooseUploade()' ";
    TimeConfHTML += (upload == "NO") ? "checked " : "";
    TimeConfHTML += "/><label for='nouploadcheck'> NO upload</label></td>";

    TimeConfHTML += "<td><input type='radio' id='livecheck' name='upload' value='LIVE' onchange='chooseUploade()' ";
    TimeConfHTML += (upload == "LIVE") ? "checked " : "";
    TimeConfHTML += "/><label for='livecheck'> LIVE</label></td>";

    // TimeConfHTML += "<td><input type='radio' id='gsmcheck' name='upload' value='GSM' onchange='chooseUploade()' /><label for='gsmcheck'> GSM</label></td>";
    TimeConfHTML += "</tr></table><br>";

    // power/display/log options
    TimeConfHTML += (useBattery ? "<input type='checkbox' id='battery' name='battery' value='1' checked />"
                                : "<input type='checkbox' id='battery' name='battery' value='1'/>");
    TimeConfHTML += "<label for='battery'> powerd by battery</label><br>";

    TimeConfHTML += (useDisplay ? "<input type='checkbox' id='display' name='display' value='1' checked />"
                                : "<input type='checkbox' id='display' name='display' value='1'/>");
    TimeConfHTML += "<label for='display'> show display</label><br>";

    TimeConfHTML += (saveDataSDCard ? "<input type='checkbox' id='logtosd' name='logtosd' value='1' checked />"
                                    : "<input type='checkbox' id='logtosd' name='logtosd' value='1'/>");
    TimeConfHTML += "<label for='logtosd'> Log to SD Card</label><br>";

    TimeConfHTML += "<BR><label for='up_interval'>Upload Interval:</label>";
    TimeConfHTML += "<select id='up_interval' name='up_interval'>"
                    "<option value=2>2 min</option>"
                    "<option value=10>10 min</option>"
                    "<option value=30>30 min</option>"
                    "<option value=60>60 min</option>"
                    "</select>";

    TimeConfHTML += "</div><div>";

    // ===== WIFI (wrapped so we can hide it) =====
    TimeConfHTML += "<div id='WIFI' style='display:none'><b>WiFi Upload Data</b>";
    TimeConfHTML += "<div><label for='BoardID'>Board ID:</label>"
                    "<input type='text' id='BoardID' name='BoardID' pattern='^(1[0-9]{3}|199[0-9])$' "
                    "title='Enter 4 digit Board ID' value=" +
                    String(boardID) + " required>";
    TimeConfHTML += "<label for='API_KEY'>API KEY:</label>"
                    "<input type='text' id='API_KEY' name='API_KEY' pattern='^[A-Za-z0-9]{32}$' "
                    "title=' Enter Bearer token' value=" +
                    API_KEY + " required></div></div>";

    // ===== LORA =====
    TimeConfHTML += "<div id='Lora' style='display:none'><BR><b>LoRa TTN Data</b>";
    TimeConfHTML += "<BR><strong>" + lora_fqz + "</strong><BR>";
    TimeConfHTML += "<label for='OTAA_APPEUI'>OTAA_APPEUI:</label><input type='text' id='OTAA_APPEUI' name='OTAA_APPEUI' pattern='^[0-9A-F]{16}$' title='Enter 8 hexadecimal digits without any prefix or separator' value=" + OTAA_APPEUI + " required>";
    TimeConfHTML += "<label for='OTAA_DEVEUI'>OTAA_DEVEUI:</label><input type='text' id='OTAA_DEVEUI' name='OTAA_DEVEUI' pattern='^[0-9A-F]{16}$' title='Enter 8 hexadecimal digits without any prefix or separator' value=" + OTAA_DEVEUI + " required>";
    TimeConfHTML += "<label for='OTAA_APPKEY'>OTAA_APPKEY:</label><input type='text' id='OTAA_APPKEY' name='OTAA_APPKEY' pattern='^[0-9A-F]{32}$' title='Enter 16 hexadecimal digits without any prefix or separator' value=" + OTAA_APPKEY + " required>";
    TimeConfHTML += "<BR><input type='checkbox' id='ADR' name='ADR' value='1'/><label for='ADR'> use ADR</label>";
    TimeConfHTML += "</div><BR>";

    // ===== GSM =====
    TimeConfHTML += "<div id='GSM' style='display:none'><BR><b>GSM Data</b>";
    TimeConfHTML += "<label for='apn'>APN:</label><input type='text' id='apn' name='apn' value=" + apn + " required>";
    TimeConfHTML += "<label for='gprs_user'>GPRS User:</label><input type='text' id='gprs_user' name='gprs_user' value=" + gprs_user + " required>";
    TimeConfHTML += "<label for='gprs_pass'>GPRS Password:</label><input type='password' id='gprs_pass' name='gprs_pass' value=" + gprs_pass + " required>";
    TimeConfHTML += "</div><BR>";

    // ===== LIVE (with MQTT / OSC) =====
    TimeConfHTML += "<div id='LIVE' style='display:none'><b>Live Output</b><br>";

    TimeConfHTML += "<input type='checkbox' id='live_mqtt' name='live_mqtt' value='1' ";
    TimeConfHTML += (upload == "LIVE" && live_mode == "MQTT") ? "checked " : "";
    TimeConfHTML += "><label for='live_mqtt'> MQTT</label>&nbsp;&nbsp;";

    TimeConfHTML += "<input type='checkbox' id='live_osc' name='live_osc' value='1' ";
    TimeConfHTML += (upload == "LIVE" && live_mode == "OSC") ? "checked " : "";
    TimeConfHTML += "><label for='live_osc'> OSC</label><br>";

    TimeConfHTML += (instant_upload
                         ? "<input type='checkbox' id='instant_upload' name='instant_upload' value='1' checked />"
                         : "<input type='checkbox' id='instant_upload' name='instant_upload' value='1' />");
    TimeConfHTML += "<label for='instant_upload'> instant upload (based on sensor rate)</label><br>";

    // --- LIVE/MQTT details ---
    TimeConfHTML +=
        "<div id='LIVE_MQTT' style='display:none'>"
        "<label for='mqtt_server_ip'>MQTT server IP:</label>"
        "<input type='text' id='mqtt_server_ip' name='mqtt_server_ip' "
        "pattern='^(?:\\d{1,3}\\.){3}\\d{1,3}$' title='IPv4 address' "
        "value='" +
        mqtt_server_ip + "' required><br>"

                         "<label for='mqtt_topic'>MQTT topic:</label>"
                         "<input type='text' id='mqtt_topic' name='mqtt_topic' "
                         "pattern='^[A-Za-z0-9_\\-/]{1,128}$' title='Topic (A–Z a–z 0–9 _ - /)' "
                         "value='" +
        mqtt_topic + "' required><br>"

                     "<label for='mqtt_port'>MQTT port:</label>"
                     "<select id='mqtt_port_preset' "
                     "onchange='var ip=document.getElementById(\"mqtt_port\");"
                     "if(this.value!=\"custom\"&&ip){ip.value=this.value;}'>"
                     "<option value='1883' " +
        String(mqtt_port == 1883 ? "selected" : "") + ">1883 (Plain)</option>"
                                                      "<option value='443' " +
        String(mqtt_port == 443 ? "selected" : "") + ">443 (TLS/WebSockets)</option>"
                                                     "<option value='custom' " +
        String((mqtt_port != 1883 && mqtt_port != 443) ? "selected" : "") + ">custom</option>"
                                                                            "</select> "
                                                                            "<input type='number' id='mqtt_port' name='mqtt_port' min='1' max='65535' "
                                                                            "value='" +
        String(mqtt_port) + "' required>"
                            "<div class='muted'>Common: 1883 (plain MQTT), 443 (MQTT over WSS/TLS).</div>"
                            "</div>";

    // --- LIVE/OSC details ---
    TimeConfHTML +=
        "<div id='LIVE_OSC' style='display:none'>"
        "<label for='osc_ip'>OSC IP:</label>"
        "<input type='text' id='osc_ip' name='osc_ip' "
        "pattern='^(?:\\d{1,3}\\.){3}\\d{1,3}$' title='IPv4 address' "
        "value='" +
        osc_ip + "' required><br>"

                 "<label for='osc_port'>OSC port:</label>"
                 "<input type='number' id='osc_port' name='osc_port' min='1' max='65535' "
                 "value='" +
        String(osc_port) + "' required>"
                           "</div>";

    TimeConfHTML += "</div>";

    // ------------- Start Connectors -------
    TimeConfHTML += generateI2CTable();

    TimeConfHTML += "<h2>Connectors:</h2>";

    TimeConfHTML += "<table style='width:100%'><tbody><tr><td colspan='2'><h3>I2C Connectors</h3></td></tr><tr>";
    TimeConfHTML += "<td><label for='i2c_1'>I2C_1</label>";
    TimeConfHTML += generateDropdown("I2C", int(I2C_con_table[0].sensorIndex), "i2c_1");
    TimeConfHTML += "<td><label for='i2c_3'>I2C_3</label>";
    TimeConfHTML += generateDropdown("I2C", int(I2C_con_table[1].sensorIndex), "i2c_3");
    TimeConfHTML += "</tr><tr>";
    TimeConfHTML += "<td><label for='i2c_2'>I2C_2</label>";
    TimeConfHTML += generateDropdown("I2C", int(I2C_con_table[2].sensorIndex), "i2c_2");
    TimeConfHTML += "<td><label for='i2c_4'>I2C_4</label>";
    TimeConfHTML += generateDropdown("I2C", int(I2C_con_table[3].sensorIndex), "i2c_4");
    TimeConfHTML += "</tr></tbody></table>";

    TimeConfHTML += "<table style='width:100%'><tbody><tr><td><h3>ADC Connectors</h3></td>";
    TimeConfHTML += "<td><h3>1-Wire Connectors</h3></td></tr><tr>";
    TimeConfHTML += "<td><label for='adc_1'>ADC_1</label>";
    TimeConfHTML += generateDropdown("ADC", ADC_con_table[0], "adc_1");
    TimeConfHTML += "<td><label for='onewire_1'>1-Wire_1</label>";
    TimeConfHTML += generateDropdown("ONE_WIRE", OneWire_con_table[0], "onewire_1");
    TimeConfHTML += "</tr><tr><td><label for='adc_2'>ADC_2</label>";
    TimeConfHTML += generateDropdown("ADC", ADC_con_table[1], "adc_2");
    TimeConfHTML += "<td><label for='onewire_2'>1-Wire_2</label>";
    TimeConfHTML += generateDropdown("ONE_WIRE", OneWire_con_table[1], "onewire_2");
    TimeConfHTML += "</tr><tr><td><label for='adc_3'>ADC_3</label>";
    TimeConfHTML += generateDropdown("ADC", ADC_con_table[2], "adc_3");
    TimeConfHTML += "<td><label for='onewire_3'>1-Wire_3</label>";
    TimeConfHTML += generateDropdown("ONE_WIRE", OneWire_con_table[2], "onewire_3");
    TimeConfHTML += "</tr></tbody><label for='I2C_5V'>I2C_5V Con</label>";
    TimeConfHTML += generateDropdown("I2C_5V", int(I2C_5V_con_table[0].sensorIndex), "I2C_5V");
    TimeConfHTML += " </table><BR>";

    // start time section
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

    TimeConfHTML += "<div id='custom_ntp' style='display:none;'>please enter a valide NTP Server IP Address:<br><input type='text' pattern='[A-Za-z0-9\\\\.]{1,15}'"
                    "title='Please enter a valid NTP server address' name='custom_ntp' value='" +
                    customNTPaddress + "'></div>";

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
    TimeConfHTML += "<select id='ntp-server-interval' name='ntp-server-interval'>"
                    "<option value=60>Hourly</option>"
                    "<option value=14400>Daily</option>"
                    "<option value=10080>Weekly</option>"
                    "</select></div><br>";

    TimeConfHTML += "</div>"; // use_NTP

    TimeConfHTML += "<button type=submit>Submit</button>";
    TimeConfHTML += "</form></div><BR>";
    TimeConfHTML += getTemplate(HTML_END);

    _wifiManager->server->send_P(200, "text/html", TimeConfHTML.c_str(), TimeConfHTML.length());
    TimeConfHTML = String();
  }

  void handleValues()
  {
    bool success = true;
    bool _NTPEnabled = NTPEnabled;

#if DEBUG_PRINT
    Serial.println("=== RECEIVED FORM DATA ===");
    for (int i = 0; i < _wifiManager->server->args(); i++)
    {
      Serial.println("Arg " + String(i) + ": " + _wifiManager->server->argName(i) + " = '" + _wifiManager->server->arg(i) + "'");
    }
    Serial.println("=== END FORM DATA ===");
#endif

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
        case 60: // 1 hour
          serverInterval = 60;
          break;
        case 1440: // 24h
          serverInterval = 1440;
          break;
        case 10080: // 1 week
          serverInterval = 10080;
          break;
        default:
          serverInterval = 1440;
        }
        NTP::setSyncDelay(serverInterval);
      }
    }

    if (_wifiManager->server->hasArg("instant_upload"))
    {
      instant_upload = getBoolArg("instant_upload");
    }
    else
    {
      instant_upload = false;
    }

    handleTimezoneSettings();

    handleBoardSettings();

    handleI2CSettings();

    handleADCSettings();

    handleOneWireSettings();

    handlePeripheralSettings();

    handleLoraSettings();

    handleLiveSettings();

    SPI_con_table[0] = NO;
    EXTRA_con_table[0] = NO;
    EXTRA_con_table[1] = NO;

    handleUploadSettings();

    updateRtcStatus();
    handleTimeSetting();
    delay(50);

    save_Connectors();
    save_Config();
    delay(200);

    _wifiManager->server->sendHeader("Location", "/saved", true);
    _wifiManager->server->send(303, "text/plain", "");
  }

  void bindServerCallback()
  {
    _wifiManager->server->on("/custom", handleRoute);
    _wifiManager->server->on("/save-tz", handleValues);
    _wifiManager->server->on("/favicon.ico", handleFavicon); // changed to imbedded png/base64 link

    _wifiManager->server->on("/saved", HTTP_GET, handleSavedPage);
    _wifiManager->server->on("/reboot", HTTP_POST, handleReboot);
  }

  // --- saved settings page ---
  void handleSavedPage()
  {
    String html;
    html.reserve(8192);

    html += WiFiManagerNS::getTemplate(WiFiManagerNS::HTML_HEAD_START);
    html.replace(FPSTR(T_v), "Board Settings Saved");
    html += WiFiManagerNS::getTemplate(WiFiManagerNS::HTML_STYLE);
    html += F(
        "<style>"
        "body{background:#111;color:#eaeaea}"
        ".wrap{max-width:900px;margin:36px auto;padding:0 16px}"
        ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(240px,1fr));gap:14px}"
        ".card{background:#1b1b1b;border:1px solid #2b2b2b;border-radius:12px;overflow:hidden}"
        ".card-h{font-weight:600;padding:10px 12px;border-bottom:1px solid #2b2b2b}"
        ".card-b{padding:10px 12px}"
        ".kv{display:flex;justify-content:space-between;gap:12px;margin:6px 0}"
        "code{background:#0e0e0e;border:1px solid #2b2b2b;padding:2px 6px;border-radius:6px}"
        ".tag{border:1px solid #444;padding:2px 8px;border-radius:999px;font-size:12px;margin-left:6px}"
        ".tag.on{border-color:#2ea043}"
        ".tag.off{border-color:#a12d2f}"
        ".btns{display:flex;gap:12px;flex-wrap:wrap;margin-top:16px}"
        ".btn{appearance:none;border:1px solid #3b82f6;background:#2563eb;color:#fff;"
        "padding:10px 14px;border-radius:10px;cursor:pointer;text-decoration:none;display:inline-block}"
        ".btn.secondary{background:#0b0b0b;border-color:#434343;color:#e5e5e5}"
        "pre{white-space:pre-wrap;background:#0e0e0e;border:1px solid #2b2b2b;padding:12px;border-radius:8px;"
        "max-height:300px;overflow:auto}"
        "h1{font-size:20px;margin:0 0 10px}"
        "h2{font-size:16px;margin:22px 0 10px}"
        ".muted{font-size:12px;color:#9aa;margin-top:8px}"
        ".tbl{width:100%;border-collapse:collapse;font-size:13px;}"
        ".tbl th,.tbl td{padding:4px 6px;text-align:left;border-bottom:1px solid #2b2b2b;}"
        ".tbl th{font-weight:600;color:#ccc;}"
        "</style>");

    html += WiFiManagerNS::getTemplate(WiFiManagerNS::HTML_HEAD_END);
    html.replace(FPSTR(T_c), "invert");
    html += custom_Title_Html;
    html += F("<div class='wrap'>");
    html += F("<h1>Settings applied</h1>");
    html += F("<p>reboot or back to main menu</p>");

    html += F("<h2>Data stored:</h2><pre>");
    html += "BoardID = " + String(boardID) + "\n";
    html += "upload = " + upload + "\n";

    if (upload == "WIFI")
      html += "API Key = " + API_KEY + "\n";

    if (upload == "LORA")
    {
      html += "OTAA_APPEUI = " + OTAA_APPEUI + "\n";
      html += "OTAA_DEVEUI = " + OTAA_DEVEUI + "\n";
      html += "OTAA_APPKEY = " + OTAA_APPKEY + "\n";
    }

    if (upload == "LIVE")
    {
      html += "live_mode = " + live_mode + "\n";
      if (live_mode == "MQTT")
      {
        html += "mqtt_server_ip = " + mqtt_server_ip + "\n";
        html += "mqtt_topic = " + mqtt_topic + "\n";
        html += "mqtt_port = " + String(mqtt_port) + "\n";
      }
      else if (live_mode == "OSC")
      {
        html += "osc_ip = " + osc_ip + "\n";
        html += "osc_port = " + String(osc_port) + "\n";
      }
      else
      {
        html += "live_mode = (none)\n";
      }
    }

    html += "display = " + String(useDisplay ? "yes" : "no") + "\n";
    html += "battery = " + String(useBattery ? "yes" : "no") + "\n";
    html += "SD Card = " + String(useSDCard ? "yes" : "no") + "\n";
    html += "upload interval = " + String(upload_interval) + " min\n";
    html += "instand upload = " + String(instant_upload ? "yes" : "no") + "\n\n";

    html += "RTC found = " + String(rtcEnabled ? "yes" : "no") + "\n";
    html += "NTP = " + String(WiFiManagerNS::NTPEnabled ? "yes" : "no") + "\n";
    html += "TZ = " + String(TZ::tzName) + "\n";
    html += "</pre>";

    beginSensorsCard(html);

    // I2C_1..4
    for (int i = 0; i < 4; ++i)
    {
      if (I2C_con_table[i].sensorIndex >= 0)
      {
        appendSensorCard(html, "I2C_" + String(i + 1), "I2C",
                         I2C_con_table[i].sensorIndex,
                         I2C_con_table[i].addrIndex);
      }
    }

    // I2C_5V
    if (I2C_5V_con_table[0].sensorIndex >= 0)
    {
      appendSensorCard(html, "I2C_5V", "I2C_5V",
                       I2C_5V_con_table[0].sensorIndex,
                       I2C_5V_con_table[0].addrIndex);
    }

    // ADC_1..3
    for (int i = 0; i < 3; ++i)
    {
      if (ADC_con_table[i] >= 0)
      {
        appendSensorCard(html, "ADC_" + String(i + 1), "ADC",
                         ADC_con_table[i], -1);
      }
    }

    // 1-Wire_1..3
    for (int i = 0; i < 3; ++i)
    {
      if (OneWire_con_table[i] >= 0)
      {
        appendSensorCard(html, "1-W_" + String(i + 1), "ONE_WIRE",
                         OneWire_con_table[i], -1);
      }
    }

    endSensorsCard(html);

    // Buttons
    html += F(
        "<div class='btns'>"
        "<form action='/reboot' method='POST' style='margin:0'>"
        "<button class='btn' type='submit'>Reboot</button>"
        "</form>"
        "<a class='btn secondary' href='/'>Back to Main Menu</a>"
        "</div>"
        "<div class='muted'>"
        "I2C-Status shows if address was found"
        "</div>");

    html += F("</div>"); // .wrap
    html += WiFiManagerNS::getTemplate(WiFiManagerNS::HTML_END);

    WiFiManagerNS::_wifiManager->server->send_P(200, "text/html", html.c_str(), html.length());
  }

  // --- Reboot-Handler ---
  void handleReboot()
  {
    WiFiManagerNS::_wifiManager->server->send(200, "text/plain", "Rebooting...");
    WiFiManagerNS::_wifiManager->server->client().flush();
    delay(250);
    ESP.restart();
  }

  static inline const Sensor *findSensorByTypeIndex(const String &con_typ, int selectedIndex)
  {
    if (selectedIndex < 0)
      return nullptr;
    for (int i = 0; i < SENSORS_NUM; ++i)
    {
      // In generateDropdown: optVal = sensor_id - 1  -> wir matchen genauso
      if (allSensors[i].con_typ == con_typ && (int(allSensors[i].sensor_id) - 1) == selectedIndex)
      {
        return &allSensors[i];
      }
    }
    return nullptr;
  }

  static void appendSensorCard(String &html, const String &title,
                               const String &con_typ, int selectedIndex, int addrIndex)
  {
    const auto *s = findSensorByTypeIndex(con_typ, selectedIndex);
    if (!s)
      return;

    // Schönere Typanzeige
    String typ = con_typ;
    if (con_typ == "ONE_WIRE")
      typ = "1-Wire";
    else if (con_typ == "I2C_5V")
      typ = "I2C";

    String addr = "";
    String status = "&ndash;";

    if ((con_typ == "I2C" || con_typ == "I2C_5V") && addrIndex >= 0)
    {
      // bis zu 2 mögliche I2C-Adressen
      for (int j = 0, k = 0; j < 2; ++j)
      {
        if (s->possible_i2c_add[j].length() > 0)
        {
          if (k == addrIndex)
          {
            addr = s->possible_i2c_add[j];
            break;
          }
          ++k;
        }
      }
      if (addr.length() == 0)
        addr = "?";
      bool present = WiFiManagerNS::isFound(addr);
      status = String("<span class='tag ") + (present ? "on" : "off") + "'>" + (present ? "online" : "offline") + "</span>";
    }

    html += "<tr>";
    html += "<td>" + title + "</td>";
    html += "<td>" + typ + "</td>";
    html += "<td>" + s->sensor_name + "</td>";
    html += "<td>";
    html += (addr.length() ? "<code>" + addr + "</code>" : "&ndash;");
    html += "</td>";
    html += "<td>" + status + "</td>";
    html += "</tr>";
  }

  static inline bool isValidIPv4(const String &s)
  {
    int dot = 0, part = -1, val = 0;
    for (size_t i = 0; i < s.length(); ++i)
    {
      char c = s[i];
      if (c == '.')
      {
        if (part < 0)
          return false; // keine Ziffer vor dem Punkt
        if (val > 255)
          return false;
        ++dot;
        part = -1;
        val = 0;
        if (dot > 3)
          return false;
        continue;
      }
      if (c < '0' || c > '9')
        return false;
      if (part < 0)
        part = 0;
      val = val * 10 + (c - '0');
      if (val > 999)
        return false; // leichtes Fast-Fail
    }
    if (dot != 3 || part < 0 || val > 255)
      return false;
    return true;
  }

  static inline bool isValidTopic(const String &s)
  {
    if (s.length() < 1 || s.length() > 128)
      return false;
    for (size_t i = 0; i < s.length(); ++i)
    {
      char c = s[i];
      // Erlaubt: Buchstaben, Ziffern, '/', '-', '_'
      if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '/' || c == '-' || c == '_'))
        return false;
    }
    return true;
  }

  // --- Settings Handlers ---

  void handleTimezoneSettings()
  {
    if (!_wifiManager->server->hasArg("timezone"))
      return;

    String timezoneStr = getStringArg("timezone");
    size_t tzidx = timezoneStr.toInt();
    String timezone = TZ::defaultTzName;

    if (tzidx < TZ::zones())
    {
      timezone = String(TZ::timezones[tzidx]);
    }

    TZ::setTzName(timezone.c_str());
    const char *tz = TZ::getTzByLocation(TZ::tzName);

    String tempServer = NTP::server();
    std::vector<char> buf(tempServer.length() + 1);
    tempServer.toCharArray(buf.data(), buf.size());

    TZ::configTimeWithTz(tz, buf.data());
    timeZone = tz;
  }

  void handleBoardSettings()
  {
    boardID = getIntArg("BoardID");
  }

  void handleI2CSettings()
  {
    for (int i = 0; i < 4; i++)
    {
      String base = "i2c_" + String(i + 1);
      if (_wifiManager->server->hasArg(base))
      {
        I2C_con_table[i].sensorIndex = getIntArg(base.c_str());
      }
      if (_wifiManager->server->hasArg(base + "_addr"))
      {
        I2C_con_table[i].addrIndex = getIntArg((base + "_addr").c_str());
      }
    }

    if (_wifiManager->server->hasArg("I2C_5V"))
    {
      I2C_5V_con_table[0].sensorIndex = getIntArg("I2C_5V");
    }
    if (_wifiManager->server->hasArg("I2C_5V_addr"))
    {
      I2C_5V_con_table[0].addrIndex = getIntArg("I2C_5V_addr");
    }
  }

  void handleADCSettings()
  {
    for (int i = 0; i < 3; i++)
    {
      String arg = "adc_" + String(i + 1);
      if (_wifiManager->server->hasArg(arg))
      {
        ADC_con_table[i] = getIntArg(arg.c_str());
      }
    }
  }

  void handleOneWireSettings()
  {
    for (int i = 0; i < 3; i++)
    {
      String arg = "onewire_" + String(i + 1);
      if (_wifiManager->server->hasArg(arg))
      {
        OneWire_con_table[i] = getIntArg(arg.c_str());
      }
    }
  }

  void handlePeripheralSettings()
  {
    useBattery = getBoolArg("battery");
    useDisplay = getBoolArg("display");
    saveDataSDCard = getBoolArg("logtosd");
  }

  void handleUploadSettings()
  {
    if (_wifiManager->server->hasArg("up_interval"))
    {
      upload_interval = getIntArg("up_interval");
    }

    if (_wifiManager->server->hasArg("upload"))
    {
      upload = getStringArg("upload");
    }

    if (_wifiManager->server->hasArg("API_KEY"))
    {
      API_KEY = getStringArg("API_KEY").c_str();
    }
    if (_wifiManager->server->hasArg("anonym"))
    {
      anonym = getStringArg("anonym").c_str();
    }

    if (_wifiManager->server->hasArg("apn"))
    {
      apn = getStringArg("apn").c_str();
    }

    if (_wifiManager->server->hasArg("gprs_user"))
    {
      gprs_user = getStringArg("gprs_user").c_str();
    }

    if (_wifiManager->server->hasArg("gprs_pass"))
    {
      gprs_pass = getStringArg("gprs_pass").c_str();
    }

    Serial.println("Upload Settings done");
  }

  void handleLoraSettings()
  {
    lora_ADR = getBoolArg("ADR");

    Serial.println("DEVEUI Settings handle");

    if (_wifiManager->server->hasArg("OTAA_DEVEUI"))
    {
      OTAA_DEVEUI = getStringArg("OTAA_DEVEUI");
      loraChanged = true;
    }
    Serial.println("APPEUI Settings handle");

    if (_wifiManager->server->hasArg("OTAA_APPEUI"))
    {
      OTAA_APPEUI = getStringArg("OTAA_APPEUI");
      loraChanged = true;
    }
    Serial.println("APPKEY Settings handle");

    if (_wifiManager->server->hasArg("OTAA_APPKEY"))
    {
      OTAA_APPKEY = getStringArg("OTAA_APPKEY");
      loraChanged = true;
    }
  }

  void handleTimeSetting()
  {
    if (_wifiManager->server->hasArg("set-time"))
    {
      String sv = getStringArg("set-time");
      if (sv.length() >= 16)
      { // "YYYY-MM-DDTHH:MM"
        setRTCfromConfigPortal(sv, timeZone);
      }
    }
  }

  void handleLiveSettings()
  {
    // Default: keine Auswahl
    String mode;

    const bool m_checked = getBoolArg("live_mqtt");
    const bool o_checked = getBoolArg("live_osc");

    // Mutual exclusivity: MQTT priorisieren, falls beide gesetzt
    if (m_checked)
      mode = "MQTT";
    else if (o_checked)
      mode = "OSC";
    else
    {
      return;
    }

    if (mode == "MQTT")
    {
      const String ip = getStringArg("mqtt_server_ip", mqtt_server_ip);
      const String topic = getStringArg("mqtt_topic", mqtt_topic);

      int m_port = _wifiManager->server->hasArg("mqtt_port")
                       ? _wifiManager->server->arg("mqtt_port").toInt()
                       : static_cast<int>(mqtt_port);

      if (!WiFiManagerNS::isValidIPv4(ip))
      {
        Serial.println("[LIVE] Invalid MQTT IP, keeping previous");
      }
      else
      {
        mqtt_server_ip = ip;
      }

      if (!WiFiManagerNS::isValidTopic(topic))
      {
        Serial.println("[LIVE] Invalid MQTT topic, keeping previous");
      }
      else
      {
        mqtt_topic = topic;
      }

      if (m_port < 1 || m_port > 65535)
      {
        Serial.println("[LIVE] Invalid MQTT port, keeping previous");
      }
      else
      {
        mqtt_port = static_cast<uint16_t>(m_port);
      }

      live_mode = "MQTT";
      return;
    }

    // == OSC ==
    const String ipStr = getStringArg("osc_ip", osc_ip);

    int port = _wifiManager->server->hasArg("osc_port")
                   ? _wifiManager->server->arg("osc_port").toInt()
                   : static_cast<int>(osc_port);

    if (!WiFiManagerNS::isValidIPv4(ipStr))
    {
      Serial.println("[LIVE] Invalid OSC IP, keeping previous");
    }
    if (port < 1 || port > 65535)
    {
      Serial.println("[LIVE] Invalid OSC port, keeping previous");
    }
    else
    {
      osc_port = static_cast<uint16_t>(port);
      osc_ip = ipStr;
    }

    live_mode = "OSC";
  }

  /*******************DropDown******************************************************************* */
  String generateDropdown(const String &con_typ, const int &conf, const String &formFieldName)
  {
    String dropdown;

    // --- Sensor selection ---
    dropdown += "<select id='" + formFieldName + "' name='" + formFieldName +
                "' onchange='updateAddrSelect(this, \"" + formFieldName + "_addr\")'>";

    dropdown += "<option value='-1'";
    if (conf < 0)
      dropdown += " selected";
    dropdown += ">NO</option>";

    for (int i = 0; i < SENSORS_NUM; i++)
    {
      if (allSensors[i].con_typ == con_typ)
      {
        int optVal = int(allSensors[i].sensor_id) - 1;

        // possible_i2c_add added as data-addrs attribute for JS
        String dataAttr = "";
        if (con_typ == "I2C" || con_typ == "I2C_5V")
        {
          dataAttr = " data-addrs='";
          bool first = true;
          for (int j = 0; j < 2; j++)
          {
            if (allSensors[i].possible_i2c_add[j].length() > 0)
            {
              if (!first)
                dataAttr += ",";
              dataAttr += allSensors[i].possible_i2c_add[j];
              first = false;
            }
          }
          dataAttr += "'";
        }

        dropdown += "<option value='" + String(optVal) + "'" + dataAttr;
        if (conf == optVal)
          dropdown += " selected";
        dropdown += ">" + allSensors[i].sensor_name + "</option>";
      }
    }

    dropdown += "</select>";

    // --- Address CHoosen (filled by JS) ---
    if (con_typ == "I2C" || con_typ == "I2C_5V")
    {
      dropdown += "<br><label for='" + formFieldName + "_addr'>I2C Addr</label>";

      int savedAddrIndex = -1;
      if (con_typ == "I2C")
      {
        int idx = formFieldName.substring(4).toInt() - 1; // "i2c_1" → 0
        if (idx >= 0 && idx < 4)
          savedAddrIndex = I2C_con_table[idx].addrIndex;
      }
      else if (con_typ == "I2C_5V")
      {
        savedAddrIndex = I2C_5V_con_table[0].addrIndex;
      }

      dropdown += "<select id='" + formFieldName + "_addr' name='" + formFieldName +
                  "_addr' data-selected-addr='" + String(savedAddrIndex) + "'></select>";
    }

    dropdown += "</td>";
    return dropdown;
  }

  String generateI2CTable()
  {
    String i2c_html = "<h3 style='text-align:center;'>I2C Scan</h3>";
    i2c_html += "<table border='1' style='border-collapse:collapse; width:100%; text-align:center;'>";

    Serial.println("[I2C] Scanning I2C bus...");

    byte error, address;
    int count = 0;
    int col = 0;

    foundCount = 0; // Reset global list
    i2c_html += "<tr>";

    digitalWrite(SW_3V3, HIGH);
    digitalWrite(SW_5V, HIGH);

    for (address = 1; address < 127; address++)
    {
      Wire.begin(I2C_SDA, I2C_SCL, I2C_FREQ);
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
      if (error == 0)
      {
        // save addresses
        if (foundCount < MAX_FOUND_I2C)
        {
          String addrStr = "0x";
          if (address < 16)
            addrStr += "0";
          addrStr += String(address, HEX);
          foundI2C[foundCount++] = addrStr;
        }

        // HTML-Ausgabe
        count++;
        i2c_html += "<td>0x";
        if (address < 16)
          i2c_html += "0";
        i2c_html += String(address, HEX);
        i2c_html += "</td>";

        col++;
        if (col >= 6)
        {
          i2c_html += "</tr><tr>";
          col = 0;
        }
      }
    }
    Wire.endTransmission();
    // digitalWrite(SW_3V3, LOW);
    // digitalWrite(SW_5V, LOW);

    if (count == 0)
    {
      i2c_html += "<td>No I2C-Devices found</td>";
      Serial.println("[I2C] No I2C devices found.");
    }

    i2c_html += "</tr></table>";

    i2c_html += "<p style='text-align:center; font-weight:bold; margin-top:8px;'>";
    i2c_html += String(count);
    i2c_html += " I2C ";
    i2c_html += (count == 1 ? " address found" : " addresses found");
    i2c_html += "</p><BR>";

    Serial.println("[I2C] Scan complete.");
    Serial.print("[I2C] addresses found ");
    Serial.println(count);

    return i2c_html;
  }

  /****Helper functions */

  bool getBoolArg(const char *name)
  {
    if (!_wifiManager->server->hasArg(name))
      return false;
    const String &val = _wifiManager->server->arg(name);
    return (val == "1" || val.equalsIgnoreCase("true") || val == "on");
  }

  int getIntArg(const char *name, int defaultValue)
  {
    return _wifiManager->server->hasArg(name)
               ? atoi(_wifiManager->server->arg(name).c_str())
               : defaultValue;
  }

  String getStringArg(const char *name, const String &defaultValue)
  {
    return _wifiManager->server->hasArg(name)
               ? _wifiManager->server->arg(name)
               : defaultValue;
  }

  bool isFound(String addr)
  {
    for (int i = 0; i < foundCount; i++)
    {
      if (foundI2C[i].equalsIgnoreCase(addr))
        return true;
    }
    return false;
  }

  void updateRtcStatus()
  {
    rtcEnabled = false;

    // check all I2C connections
    for (int i = 0; i < 4; i++)
    {
      if (I2C_con_table[i].sensorIndex == RTCDS3231)
      {
        rtcEnabled = true;
        return;
      }
    }

    // check 5V connection
    if (I2C_5V_con_table[0].sensorIndex == RTCDS3231)
    {
      rtcEnabled = true;
    }
  }
}