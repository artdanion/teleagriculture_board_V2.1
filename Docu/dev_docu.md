# 🌿 TeleAgriCulture Board Firmware

Firmware for a custom ESP32-S3 agriculture telemetry board with comprehensive sensor support, multiple communication protocols, and advanced power management capabilities.

---

## 📑 Inhaltsverzeichnis

1. [🌐 Overview & Features](#-overview--features)  
2. [📊 Project Structure & Architecture](#-project-structure--architecture)  
3. [🚀 Boot Sequence & Initialization](#-boot-sequence--initialization)  
4. [📡 Communication Protocols & Upload Modes](#-communication-protocols--upload-modes)  
5. [🔌 Sensor System Architecture](#-sensor-system-architecture)  
6. [💾 Configuration Management & Storage](#-configuration-management--storage)  
7. [🔋 Power Management & Deep Sleep](#-power-management--deep-sleep)  
8. [📺 User Interface & Navigation](#-user-interface--navigation)
9. [🌐 System Flags & Variables Reference](#-System-Flags--Variables-Reference)
10. [🔍 Debugging & Diagnostics](#-debugging--diagnostics)  
11. [🛡️ Security Considerations](#-security-considerations)  
12. [⚙️ Advanced Configuration](#-advanced-configuration)  
13. [🚀 Future Development & Roadmap](#-future-development--roadmap)  
14. [🛠️ Boot and Initialization Process (`setup()`)](#-boot-and-initialization-process-setup)  
15. [🔁 Main Program Flow (`loop()`)](#-main-program-flow-loop)  
16. [🧭 Other Functions](#-other-functions)  
17. [🏗️ Hardware Platform & Build Configuration](#-hardware-platform--build-configuration)
18. [📋 Build Instructions & Dependencies](#-build-instructions--dependencies) 
19. [📃 License](#-license)  


---

## 🌐 Overview & Features

The TeleAgriCulture Board is an ESP32-S3 based IoT platform designed for agricultural monitoring and telemetry. It provides a complete solution for sensor data acquisition, processing, and transmission with multiple connectivity options.

### Core Features
- **Multi-Protocol Sensor Support**: I2C, ADC, OneWire, SPI interfaces
- **Dual Communication**: Wi-Fi HTTP/HTTPS upload and LoRaWAN OTAA
- **Data Storage**: SD card logging with configurable intervals  
- **User Interface**: TFT display with physical button navigation
- **Power Management**: Deep sleep support for battery-powered deployments
- **Configuration**: Web-based configuration portal with captive portal
- **Boot Management**: Double reset detection and boot button configuration trigger
- **Regional LoRa Support**: EU868, US915, AU915, AS923, JP923, KR920, IN866

---

## 📊 Project Structure & Architecture

### File Organization
| Path | Purpose |
|------|---------|
| `src/main.cpp` | Main firmware logic and control flow |
| `include/sensor_Board.hpp` | GPIO definitions, sensor ENUMs, hardware abstraction |
| `include/sensor_Read.hpp` | Sensor reading logic per connector type |
| `include/board_credentials.h` | Wi-Fi, API keys, LoRa credentials (git-ignored) |
| `lib/WiFiManagerTz/` | Custom WiFiManager fork with timezone and UI support |
| `data/cert/` | SSL certificate bundle for HTTPS |

### Dependencies & Libraries
- **Display**: Adafruit ST7735/ST7789, GFX Library
- **Networking**: WiFiManager (PEAP branch), NTPClient, ESP32Time
- **LoRaWAN**: MCCI LMIC library v4.1.1
- **Sensors**: BME280, VEML7700, DHT, OneWire, ADS1X15
- **Utilities**: ArduinoJson, ESP_DoubleResetDetector, Button handling
- **Data Processing**: LoRa serialization, TDS measurement

---

## 🚀 Boot Sequence & Initialization

### Boot Process
1. **Boot Counter**: Incremented on every startup for diagnostics
2. **Wake Reason Analysis**: Determines if waking from deep sleep or cold boot
3. **Configuration Trigger Detection**:
   - Double reset within 5 seconds → Configuration portal
   - Boot button held > 5 seconds → Configuration portal
4. **Hardware Initialization**: Display, SPIFFS, I2C bus, SD card (optional)
5. **Configuration Loading**: Sensor mappings and system config from SPIFFS
6. **Communication Mode Selection**: Wi-Fi, LoRa, or local-only operation

### Configuration Triggers
- **Double Reset**: Two resets within 5-second window
- **Boot Button**: Physical button press during startup
- **Forced Config**: Software flag `forceConfig` set in previous session

---

## 📡 Communication Protocols & Upload Modes

### Upload Mode Configuration
Set via Web Configuration Portal or `config.json`:

| Mode | Behavior | Use Case |
|------|----------|----------|
| `WIFI` | HTTP/HTTPS POST to cloud endpoint | Real-time cloud connectivity |
| `LORA` | LoRaWAN OTAA with compact payloads | Long-range, low-power networks |
| `NO_UPLOAD` | Local logging only | Offline deployments |
| `SD_CARD` | Data logging to SD card | Data collection with manual retrieval |

### Wi-Fi Communication
- **Protocol**: HTTP POST (with optional HTTPS support)
- **Authentication**: Configurable API keys and endpoints
- **Data Format**: JSON payload with sensor readings and metadata
- **Connection Management**: Automatic reconnection and error handling

### LoRaWAN Communication
- **Activation**: OTAA (Over-The-Air Activation)
- **Stack**: MCCI LMIC library with regional frequency support
- **Session Persistence**: RTC memory storage for sleep/wake cycles
- **Payload Optimization**: Compact binary encoding via `getValueOrderFromString()`
- **Regional Compliance**: Automatic frequency plan selection


---

## 🔌 Sensor System Architecture

### Supported Connector Types
- **I2C / I2C_5V**: Digital sensors with I2C protocol
- **ADC**: Analog sensors via ESP32 ADC channels
- **OneWire**: Temperature sensors (DS18B20, etc.)
- **SPI**: High-speed digital sensors
- **EXTRA**: General-purpose GPIO sensors

### Sensor Configuration System
- **Dynamic Configuration**: Sensors defined in `proto_sensors[]` JSON structure
- **Per-Connector Mapping**: Each connector can host multiple sensor types
- **Runtime Detection**: Automatic sensor discovery and enumeration
- **Flexible Addressing**: Support for multiple sensors on shared buses

### Sensor Reading Pipeline
1. **Initialization**: `sensorRead()` iterates through configured `sensorVector`
   - Each includes `sensor_id`, `sensor_name`, `returnCount`, and measurements
2. **Type-Specific Handlers**: Dedicated functions per connector type
   - `readI2C_Connectors()`: I2C sensor polling
   - `readADC_Connectors()`: Analog-to-digital conversion
   - `readOneWire_Connectors()`: Temperature sensor queries
3. **Data Processing**: Calibration, filtering, and unit conversion
4. **Storage**: In-memory buffering for upload or SD card writing

### Adding New Sensors
1. **Enum Definition**: Add entry to `SensorsImplemented` in `sensor_Board.h`
2. **Metadata Registration**: Update `proto_sensors[]` JSON with sensor specifications
3. Raise `SENSORS_NUM` in `def_Sensors.h` by 1.
4. **Reading Logic**: Implement handler in appropriate `read*_Connectors()` function
5. **LoRa Integration**: Add to payload encoding via `getValueOrderFromString()`


---

## 💾 Configuration Management & Storage

### SPIFFS File System
- **Configuration Files**:
  - `/config.json`: System configuration (upload mode, credentials, intervals)
  - `/connectors.json`: Sensor-to-connector mapping definitions
- **Persistence**: Settings survive firmware updates and power cycles
- **Format**: JSON with ArduinoJson parsing library

### Configuration Categories
- **Network Settings**: Wi-Fi credentials (WPA2, WPA3, eduroam works since v1.0) managed  by WiFiManager lib (https://github.com/tzapu/WiFiManager PEAP branch for eduroam)
- **OTA Update Support**: Over-the-air firmware update capability managed by WiFiManager lib see `/lib/WiFiManagerTz`
- **Upload Settings**: Board ID, API Key, LoRa Keys... custom HTML UI by WiFiManagerTz lib
- **Sensor Mappings**: Connector assignments and sensor parameters custom HTML UI by WiFiManagerTz lib
- **Power Management**: Sleep intervals, battery thresholds custom HTML UI by WiFiManagerTz lib
- **Upload Configuration**: Protocols, intervals, data formats custom HTML UI by WiFiManagerTz lib

### Web Configuration Portal
- **Access Methods**:
  - SSID: `TeleAgriCulture Board`
  - Password: `enter123`
  - Captive portal automatically redirects browsers
- **Features**:
  - Real-time sensor configuration
  - Network credential management
  - LoRa region and key configuration
  - System diagnostics and status
- **Backend**: Custom WiFiManagerTz library with enhanced UI


---

## 🔋 Power Management & Deep Sleep

### Power Optimization Features
- **Deep Sleep Support**: ESP32 deep sleep with configurable wake sources
- **RTC Persistence**: Critical data maintained during sleep cycles
- **Battery Monitoring**: Low-power mode activation based on `useBattery` flag
- **Intelligent Scheduling**: Upload interval-based sleep timing

### Sleep/Wake Cycle
  
- **Sleep Triggers**: 
  - Upload completion
  - Timeout periods
  - Manual sleep command
  - `gotoSleep` triggers `esp_deep_sleep_start()`
- **Wake Sources**:
  - Timer-based wake (configurable intervals)
  - GPIO wake (button press)
  - External interrupt sources
- **Session Persistence**: LoRaWAN session state preserved with `RTC_DATA_ATTR`


---

## 📺 User Interface & Navigation

### TFT Display System
- **Hardware**: ST7735/ST7789 compatible TFT display
- **Library**: Adafruit GFX with ST77xx driver
- **Content**: Multi-page sensor data and system status

### Display Pages
1. **Main Status Page**: 
   - Wi-Fi/LoRa connectivity status
   - Board information and version
   - Current sensor summary
2. **Connector Pages**:
   - I2C sensor readings and status
   - ADC channel values
   - OneWire temperature sensors
3. **Detailed Sensor Pages**:
   - Individual sensor measurements
   - Historical data trends
   - Calibration status

### Physical Controls
- **Navigation Button**: Page up/down cycling
- **Boot Button**: Configuration mode entry (long press > 5s)
- **Debouncing**: Hardware debounce handling for reliable input


---

## 🌐 System Flags & Variables Reference

#### Core Control Flags
| Flag | Type | Purpose | Persistence | Default |
|------|------|---------|-------------|---------|
| `forceConfig` | boolean | Forces Configuration Portal on next boot | Session | false |
| `gotoSleep` | boolean | Triggers deep sleep entry after current cycle | Session | false |
| `freshBoot` | boolean | Forces new LoRa join sequence, ignores saved session | Session | false |
| `useBattery` | boolean | Enables low-power behavior and optimizations | SPIFFS | false |
| `useDisplay` | boolean | Enables Display | SPIFFS | true |

#### Communication Control Flags  
| Flag | Type | Purpose | Persistence | Default |
|------|------|---------|-------------|---------|
| `sendDataWifi` | boolean | Triggers Wi-Fi data upload in current cycle | Session | false |
| `sendDataLoRa` | boolean | Triggers LoRa data upload in current cycle | Session | false |
| `loraJoined` | boolean | LoRaWAN session status (joined/not joined) | RTC Memory | false |
| `useHTTPS` | boolean | Enables HTTPS instead of HTTP for Wi-Fi uploads | SPIFFS | false |
| `useSDCard` | boolean | Enables SD card logging functionality | SPIFFS | false |

#### Upload Mode Configuration
| Variable | Type | Values | Purpose |
|----------|------|--------|---------|
| `upload` | enum | `WIFI`, `LORA`, `NO_UPLOAD`, `SD_CARD` | Primary communication method selection |
| `upload_interval` | integer | Minutes (e.g., 15, 30, 60) | Interval between data uploads and sleep cycles |

#### System State Variables
| Variable | Type | Purpose | Persistence |
|----------|------|---------|-------------|
| `bootCount` | integer | Incremented on every boot, diagnostic counter | RTC Memory |
| `wakeup_reason` | esp_sleep_wakeup_cause_t | Reason for wake from deep sleep | Session |
| `sleep_duration` | uint64_t | Calculated sleep time in microseconds | Session |

#### Sensor System Variables
| Variable | Type | Purpose | Storage |
|----------|------|---------|---------|
| `sensorVector` | vector | Container for all configured sensor instances | RAM |
| `connectorConfig` | JSON | Mapping of sensors to physical connectors | SPIFFS |
| `proto_sensors[]` | JSON Array | Sensor definitions and metadata | Code/SPIFFS |

#### Configuration Variables
| Variable | Type | Purpose | Storage |
|----------|------|---------|---------|
| `wifi_ssid` | string | Wi-Fi network name | SPIFFS |
| `wifi_password` | string | Wi-Fi network password | SPIFFS |
| `api_endpoint` | string | HTTP/HTTPS upload URL | SPIFFS |
| `api_key` | string | Authentication key for cloud services | SPIFFS |

---



## 🔍 Debugging & Diagnostics

### Debug System
- **Master Control**: `DEBUG_PRINT` macro for global debug enable/disable
- **Granular Levels**: Separate debug levels for LMIC (0-2) and Core (0-5)
- **Serial Output**: 115200 baud rate for development monitoring

### Diagnostic Functions
| Function | Purpose |
|----------|---------|
| `checkLoadedStuff()` | Validates sensor and connector configuration loading |
| `printConnectors()` | Dumps complete sensor mapping per connector type |
| `listDir()` | SPIFFS filesystem contents and usage analysis |
| Boot counter | Tracks system stability and reset frequency |

### Error Handling
- **Configuration Failures**: Fallback to default configurations
- **Communication Errors**: Automatic retry mechanisms
- **Sensor Failures**: Individual sensor error isolation
- **Memory Management**: SPIFFS usage monitoring and cleanup



---

## 🛡️ Security Considerations

### Credential Management
- **Storage Location**: `board_credentials.h` (excluded from version control)
- **Wi-Fi Security**: WPA2/WPA3 with configurable credentials
- **HTTPS Support**: SSL certificate bundle embedded in firmware
- - Certificate bundle embedded from `/data/cert/x509_crt_bundle.bin`
- **LoRa Security**: Device EUI, Application EUI, and Application Key

---

## ⚙️ Advanced Configuration

### Upload Intervals & Scheduling
- **Configurable Intervals**: Upload frequency in minutes via `upload_interval`
- **Smart Scheduling**: For 60-minute intervals, aligns to full hour boundaries
- **Sleep Optimization**: Sleep duration calculated based on upload schedule

### LoRa Payload Optimization
- **Data Encoding**: Binary serialization for maximum efficiency using `lora-serialization` library ( https://github.com/thesolarnomad/lora-serialization )
- **Sensor Ordering**: Configurable sensor data sequence via `getValueOrderFromString()`
- **Regional Compliance**: Automatic duty cycle and power management per region


---

## 🚀 Future Development & Roadmap

### Planned Enhancements
- **Web-based LED/RGB Control**: Remote indicator management
- **Sensor Auto-calibration**: Automatic calibration and drift compensation  
- **I2C Address Conflict Resolution**: I2C Scan at the begin of Config Portal and drop down menu for alternative adresses

### Extensibility
- **Modular Sensor Architecture**: Easy addition of new sensor types
- **Plugin System**: Runtime sensor driver loading
- **Advanced Analytics**: On-device data processing and trend analysis
- **Multi-board Coordination**: Mesh networking capabilities


---

## 🛠️ Boot and Initialization Process (`setup()`)

During boot, the following steps are performed:

### 1. 🧹 GPIO Reset and Hold Disable
- Certain pins are reset after deep sleep (`gpio_reset_pin()`).
- Hold functions for pins are deactivated (`gpio_hold_dis()`).
- Deep sleep hold is globally deactivated.

### 2. 📌 Pin Configuration
- All used pins are set as `INPUT` or `OUTPUT`.
- Backlight (`TFT_BL`) is initially off.
- Serial console is activated (115200 baud).

### 3. ⚙️ SPI and Display Setup
- Initialization of SPI bus for TFT & SD.
- TFT backlight is deactivated.
- Display is initialized (optional when `useDisplay == true`).

### 4. 🔋 Battery Detection & ADC Calibration
- Battery monitoring is initialized and calibrated via ADC.
- Sensor values are stored in arrays preparatively.

### 5. 💾 SPIFFS / Configuration Data
- SPIFFS is mounted and checked.
- Sensor, connector, and configuration data are loaded.

### 6. 📶 Network Connection
- WiFi connection is automatically established (WPA2 or regular WLAN) when `upload == WIFI`).
- If active: NTP time is synchronized.
- Fallback: Time is set via HTTP header.

### 7. 🌐 Configuration Mode (optional)
- If `forceConfig == true`: Web configuration portal is started.
- Web server is provided in AP mode when `webpage == true`.

### 8. 📡 LoRaWAN Setup (optional)
- Initialization of LMIC (optional when `upload == LORA`).
- Session restoration from previous join.
- Alternative: New OTAA join request is prepared.

### 9. 💾 SD Card Setup (optional)
- If `saveDataSDCard == true`: SD card is initialized and checked.


---

## 🔁 Main Program Flow (`loop()`)

The `loop()` continuously processes the following functions:

### 1. 🔁 Double Reset Detection
- Detects manual restart via double-click through `drd->loop()`.

### 2. ⏰ Time-Controlled Processes
- **Upload Cycle:** Data upload via WiFi / LoRa / SD card after `upload_interval`.
- **Display Dimming:** TFT backlight is dimmed after `interval`.
- **Display Sleep:** TFT is turned off after `interval2` (backlight off, display black).

### 3. 👆 Button Control
- Navigation through pages (`upButton`, `downButton`).
- Long press of upper button opens configuration portal.

### 4. 📤 Upload Logic
Depending on `upload` mode:

#### 🔌 `NO_UPLOAD`
- Only local sensor reading and optional display on screen.

#### 📶 `WIFI`
- Data is uploaded via HTTP(S).
- WiFi is deactivated after upload.
- Optional: Set time via NTP or HTTP.
- Device may enter deep sleep (during battery operation).

#### 📡 `LORA`
- Data is transmitted via LoRaWAN.
- OTA join occurs every 24h or at restart.
- LMIC state is saved.
- Optional: Also write data to SD card.
- Device may enter deep sleep.

#### 💾 `SD_CARD`
- Data is stored locally on SD.
- Device may enter deep sleep.

### 5. 🌙 Deep Sleep
The device enters power saving mode when:
- no display is used **or**
- battery operation is detected **and**
- upload is completed.

Wake-up by:
- Button interrupt (EXT1 wakeup)
- Timer (upload interval)



---

## 🧭 Other Functions

- **Display Page Rendering:** Sensor pages and time page.
- **Web Interface:** Access to measurement data when web page is enabled.
- **Time Display:** On main page in WiFi mode.
- **Debug Output:** Via serial console.


---


## 🏗️ Hardware Platform & Build Configuration

### Target Hardware
- **MCU**: ESP32-S3 DevKit-C-1
- **Framework**: Arduino for ESP32 (v3.20009.0)
- **Platform**: Espressif32 (v6.3.1)
- **Display**: ST7735/ST7789 TFT
- **Radio**: SX1276 LoRa transceiver
- **Storage**: SPIFFS + optional SD card

### Build Environments
The firmware supports multiple regional LoRa configurations:

| Environment | Frequency | Region |
|-------------|-----------|--------|
| `EU` | 868 MHz | Europe |
| `US` | 915 MHz | United States/Canada |
| `AU` | 915 MHz | Australia |
| `AS` | 923 MHz | Asia |
| `JP` | 923 MHz | Japan (with country code) |
| `KR` | 920 MHz | Korea |
| `IN` | 866 MHz | India |

#### Build-Time Configuration Flags (platformio.ini)
| Flag | Purpose | Value/Options |
|------|---------|---------------|
| `ARDUINO_USB_MODE` | USB interface mode | 1 (CDC enabled) |
| `ARDUINO_USB_CDC_ON_BOOT` | CDC available at boot | 1 (enabled) |
| `hal_init` | LMIC HAL initialization | LMICHAL_init |
| `LMIC_DEBUG_LEVEL` | LoRa stack debug verbosity | 0-2 (0=none, 2=verbose) |
| `CORE_DEBUG_LEVEL` | ESP32 core debug level | 0-5 (0=none, 5=verbose) |
| `CONFIG_TTN_SPI_FREQ` | SPI frequency for LoRa radio | 1000000 (1MHz) |
| `CFG_sx1276_radio` | LoRa radio chip selection | 1 (SX1276) |
| `DISABLE_PING` | Disable LoRaWAN ping slots | 1 (disabled) |
| `DISABLE_BEACONS` | Disable LoRaWAN beacon reception | 0 (enabled) |
| `TAC_VERSION` | Firmware version string | "1.20" |

#### Regional LoRa Configuration Flags
| Environment | Primary Flag | Additional Flags | Frequency |
|-------------|--------------|------------------|-----------|
| EU | `CFG_eu868=1` | `CUSTOM_LORA_FQZ="EU 868 MHz"` | 868 MHz |
| US | `CFG_us915=1` | `CUSTOM_LORA_FQZ="US/CAN 915 MHz"` | 915 MHz |
| AU | `CFG_au915=1` | `CUSTOM_LORA_FQZ="AUS 915 MHz"` | 915 MHz |
| AS | `CFG_as923=1` | `CUSTOM_LORA_FQZ="ASIA 923 MHz"` | 923 MHz |
| JP | `CFG_as923=1` | `LMIC_COUNTRY_CODE=LMIC_COUNTRY_CODE_JP` | 923 MHz |
| KR | `CFG_kr920=1` | `CUSTOM_LORA_FQZ="KR 920 MHz"` | 920 MHz |
| IN | `CFG_in866=1` | `CUSTOM_LORA_FQZ="IND 866 MHz"` | 866 MHz |

#### Debug and Development Flags
| Flag | Purpose | Usage |
|------|---------|-------|
| `DEBUG_PRINT` | Master debug output control | Enable/disable all debug output |
| `LMIC_DEBUG_LEVEL` | LoRaWAN stack debugging | 0=off, 1=basic, 2=detailed |
| `CORE_DEBUG_LEVEL` | ESP32 Arduino core debugging | 0=off, 1-5=increasing verbosity |

---

## 📋 Build Instructions & Dependencies

### Prerequisites
- **PlatformIO**: Latest version with ESP32 platform support
- **Credentials File**: Create `include/board_credentials.h` with your credentials
- **Regional Selection**: Choose appropriate environment (EU, US, AU, etc.)

### Build Commands
```bash
# Build for specific region (EU example)
pio run -e EU

# Upload firmware
pio run -e EU -t upload

# Monitor serial output
pio device monitor -b 115200
```
---

## 📃 License

MIT License © 2025 artdanion  
Third-party libraries are covered under their respective licenses located in `/lib/`.

---
