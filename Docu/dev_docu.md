# 🌿 TeleAgriCulture Board Firmware

Firmware for a custom ESP32-S3 agriculture telemetry board with comprehensive sensor support, multiple communication protocols, and advanced power management capabilities.

---

## 📑 Table of Contents

1. [🌐 Overview & Features](#-overview--features)  
2. [📊 Project Structure & Architecture](#-project-structure--architecture)  
3. [🚀 Quick Start](#-quick-start)
4. [📡 Communication Protocols](#-communication-protocols)  
5. [🔌 Sensor Support](#-sensor-support)  
6. [💾 Configuration](#-configuration)  
7. [🔋 Power Management](#-power-management)  
8. [📺 User Interface](#-user-interface)
9. [🧩 Modular Architecture](#-modular-architecture)
10. [🔍 Debugging](#-debugging)  
11. [🏗️ Hardware & Build Configuration](#-hardware--build-configuration)
12. [📋 Build Instructions](#-build-instructions) 
13. [🤝 Contributing](#-contributing)
14. [📃 License](#-license)  

---

## 🌐 Overview & Features

The TeleAgriCulture Board is an ESP32-S3 based IoT platform designed for agricultural monitoring and telemetry. It provides a complete solution for sensor data acquisition, processing, and transmission with multiple connectivity options.

### Core Features
- **Multi-Protocol Sensor Support**: I²C, ADC, OneWire, SPI interfaces
- **Multiple Communication Protocols**: Wi‑Fi HTTP/HTTPS, LoRaWAN OTAA, MQTT, OSC (Open Sound Control)
- **Data Storage**: SD card logging with configurable intervals  
- **User Interface**: TFT display with physical button navigation
- **Power Management**: Deep sleep support with RTC persistence
- **Configuration**: Web-based configuration portal (captive portal)
- **Boot Management**: Double‑reset detection and boot‑button config trigger
- **Regional LoRa Support**: EU868, US915, AU915, AS923, JP923, KR920, IN866
- **Live Data Streaming**: Real‑time sensor data via MQTT and OSC
- **Task-Based Architecture**: FreeRTOS tasks for non‑blocking UX and reliability

---

## 📊 Project Structure & Architecture

### Modular Library Structure
The firmware has been refactored into a modular architecture with dedicated libraries:

```
lib/
├── debug_TAC/          # Debug logging & diagnostics
├── init_TAC/           # Board init, I²C, sensor definitions (def_Sensors.cpp)
├── lora_TAC/           # LoRaWAN communication (LMIC)
├── mqtt_TAC/           # MQTT client with TLS & reconnect
├── osc_TAC/            # OSC protocol (UDP)
├── storage_TAC/        # SD card logging & persistence
├── time_TAC/           # RTC + NTP + TZ handling
├── UI_TAC/             # TFT UI, pages & navigation
├── WiFiManagerTz/      # WiFiManager with timezone & custom portal
└── MiCS6814-I2C/       # Gas sensor driver
```

### Core Files
```
src/
└── main.cpp                    # Main application logic

include/
├── sensor_Read.hpp             # Per-connector sensor read logic
└── board_credentials.h         # Credentials (git-ignored)

data/
└── cert/                       # TLS certificate bundle
```

> **Tip**: Keep all sensor definitions centralized in `lib/init_TAC/src/def_Sensors.cpp` add  `lib/init_TAC/src/def_Sensors.h` for clarity.

**High‑Level Runtime Architecture**
- **Setup phase** configures hardware, filesystem, network, time, and protocols.
- **Loop phase** executes periodic sensor reads, UI updates, uploads, and power transitions.
- **Tasks** (FreeRTOS) offload long‑press/config detection and avoid blocking the loop.

---

## 🚀 Quick Start

### 1) Hardware Setup
- ESP32‑S3 DevKit‑C‑1 (V2.1)
- ST7735/ST7789 TFT display
- SX1276 LoRa module (optional)
- Sensors as per [Sensor Support](#-sensor-support)

### 2) Credentials Configuration
Create `include/board_credentials.h`:

```cpp
// Board identification
const String boardID = "YOUR_BOARD_ID";
const String API_KEY = "YOUR_API_KEY";

// LoRa credentials
const String OTAA_DEVEUI = "YOUR_DEV_EUI";
const String OTAA_APPEUI = "YOUR_APP_EUI"; 
const String OTAA_APPKEY = "YOUR_APP_KEY";

// MQTT credentials
const String mqtt_server_ip = "YOUR_MQTT_BROKER";
const int    mqtt_port      = 1883;
const String mqtt_username  = "YOUR_USERNAME";
const String mqtt_password  = "YOUR_PASSWORD";

// OSC configuration
const String osc_server_ip  = "YOUR_OSC_SERVER";
const int    osc_port       = 8000;

// Enterprise WiFi (optional)
const String anonym         = "YOUR_IDENTITY";
const String user_CA        = "YOUR_CA_CERT";
const bool   useEnterpriseWPA = false;
```

### 3) Build & Upload
```bash
# Install PlatformIO
pip install platformio

# Build (EU example)
pio run -e EU

# Upload firmware
pio run -e EU -t upload

# Serial monitor
pio device monitor -b 115200
```

### 4) First Configuration
- Connect to AP **"TeleAgriCulture Board"** (password **"enter123"**)
- Complete portal setup (Wi‑Fi/LoRa/MQTT/OSC, sensors, time, display)
- Alternatively, **double‑reset** or **hold BOOT > 5s** to enter config mode

---

## 📡 Communication Protocols

### Supported Upload Modes
| Mode        | Protocol     | Use Case                               |
|-------------|--------------|----------------------------------------|
| `WIFI`      | HTTPS POST   | Cloud connectivity with JSON payloads |
| `LORA`      | LoRaWAN OTAA | Long‑range, low‑power networks        |
| `LIVE`      | MQTT / OSC   | Real‑time streaming                   |
| `SD_CARD`   | File logging | Offline data collection               |
| `NO_UPLOAD` | —            | Display‑only local monitoring         |

#### Wi‑Fi (HTTP/HTTPS)
- **Security**: TLS with certificate bundle validation (`/data/cert/`)
- **Authentication**: Bearer token (API key)
- **Format**: JSON with dynamic sizing to prevent overflows
- **Enterprise**: WPA2/WPA3‑Enterprise (eduroam) via WiFiManagerTz
- **Endpoint**: `https://kits.teleagriculture.org/api/kits/{boardID}/measurements`

#### LoRaWAN
- **Activation**: OTAA with session persistence (RTC memory)
- **Regional Support**: EU868, US915, AU915, AS923, JP923, KR920, IN866
- **Optimization**: Binary payload encoding with `lora-serialization` library
- **Power Management**: Duty‑cycle aware with air‑time calculation
- **Session Recovery**: LMIC state saved to RTC memory across sleep cycles

#### MQTT
- **Security**: TLS encryption with certificate validation
- **Reliability**: Auto‑reconnect with exponential backoff, keep‑alive management
- **Topics**: Hierarchical structure based on `boardID`
- **Live Mode**: Real‑time streaming with `instant_upload` capability
- **Buffer Management**: 2048-byte buffer with overflow protection

#### OSC (Open Sound Control)
- **Transport**: UDP for low latency
- **Applications**: Audio/visual systems, real‑time control interfaces
- **Discovery**: Automatic server resolution and connection management
- **Format**: Native OSC message formatting for sensor data paths

---

## 🔌 Sensor Support

### Connector Types
- **I²C/I²C_5V** – Digital sensors with I²C protocol
- **ADC** – Analog signals via ESP32 ADC channels
- **OneWire** – DS18B20 temperature sensor family
- **SPI** – High‑speed digital sensors
- **EXTRA** – General-purpose GPIO‑based sensors

### Implemented Sensors
*in V 1.75*

#### Environmental Sensors
- **BME280/BMP280/BMP680**: Temperature, humidity, pressure, gas resistance
- **SHT21**: High-accuracy temperature and humidity
- **DHT11/DHT22**: Basic temperature and humidity sensors
- **DS18B20**: OneWire temperature sensors
- **RTCDS3231**: Real-time clock with temperature compensation

#### Light Sensors
- **BH1750**: Digital ambient light sensor
- **VEML7700**: High-accuracy ambient light sensor
- **LTR390**: UV light sensor with ambient light
- **DFR LIGHT**: Analog light intensity sensor

#### Gas Sensors
- **MiCS6814-I2C**: Multi-gas sensor (CO, NO2, NH3) with dedicated library
- **MultiGasV1/V2**: Multi-gas sensor modules
- **DFR FLAME**: Flame detection sensor

#### Analog Sensors
- **ADS1115**: 16-bit ADC for precision analog measurements
- **TDS**: Total dissolved solids sensor for water quality
- **DFR LM35**: Precision temperature sensor

#### Additional Components
- **WS2812**: RGB LED strip control for visual indicators

### Adding New Sensors
1. **Define in Library**: Add sensor definition to `lib/init_TAC/src/def_Sensors.cpp` (JSON entry)
2. **Update Enums**: Extend `SensorsImplemented` in `lib/init_TAC/src/def_Sensors.h` if required
3. **Update SensorNUM**: Raise `SENSORS_NUM` by 1 in `lib/init_TAC/src/def_Sensors.h`.
4. **Implement Reading**: Add reading logic in `include/sensor_Read.hpp` for the appropriate connector type
5. **Protocol Integration**: Update payload mapping for LoRa/MQTT/OSC transmission (if new Units are added)
6. **Documentation**: Update sensor count and this documentation

---

## 💾 Configuration

### Web Configuration Portal
- **Access**: WiFi AP `TeleAgriCulture Board` / password `enter123`  
- **Triggers**: Double‑reset detection, boot button > 5s, or `forceConfig` flag  
- **Timeout**: 15 minutes (900 seconds) before auto-restart
- **Features**: Dark mode UI, custom HTML forms, real-time validation

#### Configuration Categories
- **Network**: WiFi credentials (WPA2/WPA3/Enterprise), hostname settings
- **Protocols**: Upload mode selection, API endpoints, credential management
- **MQTT**: Broker settings, authentication, topic configuration  
- **OSC**: Server settings, port configuration, message formatting
- **LoRa**: Regional settings, device credentials, join parameters
- **Sensors**: Connector mappings, calibration values, enable/disable
- **Power**: Sleep intervals, battery thresholds, power optimization
- **Time**: NTP servers, timezone selection, RTC configuration
- **Display**: Backlight control, page settings, sleep behavior

### Persistent Storage (SPIFFS)
- **`/config.json`**: System configuration (upload mode, credentials, intervals)  
- **`/connectors.json`**: Sensor‑to‑connector mapping definitions  
- **Format**: JSON with ArduinoJson parsing and validation
- **Backup**: Automatic configuration backup before updates
- **Recovery**: Fallback to defaults on corruption with error logging

---

## 🔋 Power Management

### Optimization Features
- **Deep Sleep**: ESP32 deep sleep with timer and GPIO wake sources  
- **RTC Persistence**: Critical data maintained in RTC memory across sleep cycles
- **Battery Mode**: Automatic low‑power behavior via `useBattery` flag  
- **Smart Scheduling**: Interval‑based wake timing with hour alignment
- **Bus Management**: Proper I²C/SPI shutdown before sleep entry
- **GPIO Hold**: Pin state preservation during deep sleep

### Sleep/Wake Cycle Management
#### Sleep Triggers
- Upload completion for all communication protocols
- Configurable timeout periods via `gotoSleep` flag
- Battery mode activation with voltage thresholds
- Display timeout and power management

#### Wake Sources
- **Timer Wake**: Configurable intervals (15, 30, 60 minutes)
- **GPIO Wake**: Button press via `ESP_EXT1_WAKEUP_ANY_LOW`
- **External Interrupts**: Sensor-driven wake events
- **RTC Alarm**: Scheduled wake for time-critical operations

#### Session Persistence
- **LoRaWAN**: Session keys and frame counters in RTC memory
- **MQTT**: Connection state and broker information
- **Configuration**: Critical flags and system state preservation
- **Sensor Data**: Last reading cache for comparison

---

## 📺 User Interface

### TFT Display System
- **Hardware**: ST7735/ST7789 compatible displays
- **Library**: Modular UI system via `UI_TAC` library
- **Resolution**: 160x128 pixels with 16-bit color
- **Backlight**: PWM control (0-255) with automatic dimming

### Display Features
#### Multi-Page Navigation
- **Page 0**: Main status with time display (WiFi mode)
- **Sensor Pages**: Dynamic page count based on active sensors
- **Connectivity Pages**: Network status, protocol indicators, Time and Connectors

#### Power Management
- **Active Mode**: Full brightness (250 PWM) during interaction
- **Dim Mode**: Low brightness (5 PWM) after 1 minute
- **Sleep Mode**: Display off after 5 minutes with black screen
- **Wake Triggers**: Button press restores full brightness

#### Status Indicators
- **SD Card**: Icon display when SD logging active
- **Network**: WiFi, LoRa, MQTT, OSC connection indicators  
- **Battery**: Voltage display and low-power warnings
- **Time**: Real-time clock display with timezone support

### Physical Controls
- **Up Button**: Page navigation up, long-press config trigger
- **Down Button**: Page navigation down, wake from sleep
- **Debouncing**: Hardware-level debounce with software confirmation
- **Task-Based**: Dedicated FreeRTOS task for long-press detection (>4s)

---

## 🧩 Modular Architecture

### Library Organization
Each `_TAC` library encapsulates a specific subsystem for maintainability:

| Library | Primary Functions | Key Features |
|---------|-------------------|--------------|
| `debug_TAC` | `DebugLogger` with configurable levels | Replaces `Serial.*` calls, 0-5 verbosity |
| `init_TAC` | `initBoard()`, `initFiles()`, sensor management | Central sensor JSON, I²C scanning |
| `mqtt_TAC` | Broker connect/reconnect, topic management | TLS support, auto-reconnect, buffering |
| `osc_TAC` | OSC message build/send over UDP | Live streaming, server discovery |
| `lora_TAC` | LMIC setup, join management, session persistence | Regional support, duty cycle management |
| `storage_TAC` | SD write helpers, file management | CSV/JSON logging, file rotation |
| `time_TAC` | NTP + RTC bridge, timezone handling | Multi-source time sync, DST support |
| `UI_TAC` | Page renderer, navigation, power management | Multi-page system, PWM backlight |
| `WiFiManagerTz` | Captive portal with enhanced UI | Custom HTML forms, timezone selection |
| `MiCS6814-I2C` | Dedicated gas sensor driver | Multi-gas measurement, calibration |


---

## 🔍 Debugging

### Boot Process Analysis
The setup() function executes the following sequence:

1. **Double‑Reset Detection**: Check for configuration trigger via DRD
2. **Board Initialization**: GPIO setup, I²C/SPI bus configuration via `initBoard()`
3. **File System**: SPIFFS mounting and validation via `initFiles()`
4. **Sensor Discovery**: Load prototypes and connector mappings
   - `load_Sensors()`: Parse sensor definitions from def_Sensors.cpp
   - `load_Connectors()`: Build connector-to-sensor mapping
   - `load_Config()`: System configuration from SPIFFS
5. **Display Setup**: Conditional TFT initialization via `initDisplayIfNeeded()`
6. **Wake Handling**: Display appropriate wake-up screen via `handleWakeupDisplay()`
7. **Task Creation**: FreeRTOS task for configuration button monitoring
8. **Time Sync**: RTC initialization and ESP32 time setting
9. **Protocol Setup**: Conditional initialization based on upload mode

### Main Loop Operation
The loop() function processes:

- **DRD Processing**: Continuous double-reset detection via `drd->loop()`
- **UI Management**: Button handling, backlight PWM control
- **Time Operations**: Periodic tasks via `updateTimeHandle()`
- **Mode-Specific Loops**: Protocol handlers based on upload configuration
- **Display Updates**: Page rendering and content refresh
- **Web Server**: Client request handling when webpage mode active
- **Task Scheduling**: 50ms delay via `vTaskDelay()` for FreeRTOS optimization

### System Flags & Variables Reference

#### Core Control Flags
| Flag | Type | Purpose | Persistence | Default |
|------|------|---------|-------------|---------|
| `forceConfig` | boolean | Force configuration portal on next boot | Session | false |
| `gotoSleep` | boolean | Trigger deep sleep after current cycle | Session | false |
| `freshBoot` | boolean | Force new LoRa join, ignore saved session | Session | false |
| `useBattery` | boolean | Enable low‑power behavior and optimizations | SPIFFS | false |
| `useDisplay` | boolean | Enable TFT display functionality | SPIFFS | true |
| `configPortal_run` | boolean | Configuration portal currently active | Session | false |

#### Communication Control Flags  
| Flag | Type | Purpose | Persistence | Default |
|------|------|---------|-------------|---------|
| `sendDataWifi` | boolean | Trigger Wi‑Fi data upload in current cycle | Session | false |
| `sendDataLoRa` | boolean | Trigger LoRa data upload in current cycle | Session | false |
| `sendDataMQTT` | boolean | Trigger MQTT data publish in current cycle | Session | false |
| `sendDataOSC` | boolean | Trigger OSC data transmission | Session | false |
| `loraJoined` | boolean | LoRaWAN session status (joined/not joined) | RTC Memory | false |
| `useHTTPS` | boolean | Enable HTTPS for Wi‑Fi uploads | SPIFFS | false |
| `useSDCard` | boolean | Enable SD card logging functionality | SPIFFS | false |
| `saveDataSDCard` | boolean | Trigger SD card data write | Session | false |
| `newSensorDataAvailable` | boolean | New sensor data ready for instant upload trggered in `include/sensor_Read.hpp` | Session | false |

#### Upload Mode Configuration
| Variable | Type | Values | Purpose |
|----------|------|--------|---------|
| `upload` | String | `WIFI`, `LORA`, `LIVE`, `NO_UPLOAD`, `SD_CARD` | Primary communication method |
| `live_mode` | String | `MQTT`, `OSC` | Live streaming protocol selection |
| `upload_interval` | integer | 2, 15, 30, 60 (minutes) | Interval between uploads and sleep cycles |
| `instant_upload` | boolean | Enable real-time data streaming | SPIFFS | false |

#### System State Variables
| Variable | Type | Purpose | Persistence |
|----------|------|---------|-------------|
| `bootCount` | integer | Boot counter for diagnostics | RTC Memory |
| `wakeup_reason` | esp_sleep_wakeup_cause_t | Reason for wake from deep sleep | Session |
| `userWakeup` | boolean | Wake triggered by button press | Session |
| `backlight_pwm` | integer | TFT backlight PWM value (0-255) | Session |
| `currentPage` | integer | Currently displayed UI page index | Session |
| `displayRefresh` | boolean | Force display refresh on next update | Session |

#### Task Management
| Variable | Type | Purpose |
|----------|------|---------|
| `configTaskHandle` | TaskHandle_t | Handle for config button monitoring task |

### Debug System Features
- **Master Control**: `DEBUG_PRINT` macro for global debug enable/disable special functions
- **Library Logging**: Each `_TAC` library implements configurable logging levels
- **Serial Output**: 115200 baud with structured debug messages
- **Log Levels**: TAC_LOG_LEVEL 0-5 for different verbosity levels
- **Memory Monitoring**: JSON document sizing and overflow detection
- **I²C Diagnostics**: Automatic device scanning and address conflict detection

### Diagnostic Functions
| Function | Purpose | Library |
|----------|---------|---------|
| `checkLoadedStuff()` | Validate sensor and connector configuration | main |
| `scanI2CDevices()` | Scan and report I²C device addresses | init_TAC |
| `printRTCInfo()` | Display RTC status and current time | time_TAC |
| `mqttPreflightCheck()` | Test MQTT broker connectivity | mqtt_TAC |
| `oscConnectNow()` | Validate OSC server connection | osc_TAC |

---

## 🏗️ Hardware & Build Configuration

### Target Hardware Specifications
- **MCU**: ESP32‑S3 DevKit‑C‑1 (ESP32-S3-WROOM-1-N8)
- **Framework**: Arduino for ESP32 (v3.20009.0)  
- **Platform**: Espressif32 (v6.3.1)
- **Display**: ST7735/ST7789 TFT with PWM backlight control
- **Radio**: SX1276 LoRa transceiver (868/915 MHz variants)
- **Storage**: SPIFFS (internal) + optional SD card
- **Additional**: RTC module (DS3231), gas sensors, environmental sensors

### Build Environments (LoRa Regional Support)
| Environment | Frequency | Region | MQTT/OSC Support |
|-------------|-----------|--------|------------------|
| `EU` | 868 MHz | Europe | Yes |
| `US` | 915 MHz | United States/Canada | Yes |
| `AU` | 915 MHz | Australia | Yes |
| `AS` | 923 MHz | Asia | Yes |
| `JP` | 923 MHz | Japan (with country code) | Yes |
| `KR` | 920 MHz | Korea | Yes |
| `IN` | 866 MHz | India | Yes |

### Build-Time Configuration Flags (platformio.ini)
| Flag | Purpose | Default | Options |
|------|---------|---------|---------|
| `TAC_VERSION` | Firmware version string | `"1.75"` | String |
| `TAC_LOG_LEVEL` | Debug verbosity for TAC libraries | `3` | 0-5 |
| `DEBUG_PRINT` | Master debug output control | `false` | true/false |
| `ARDUINO_USB_MODE` | USB interface mode | `1` | CDC enabled |
| `ARDUINO_USB_CDC_ON_BOOT` | CDC available at boot | `1` | enabled |
| `CONFIG_TTN_SPI_FREQ` | SPI frequency for LoRa radio | `1000000` | 1MHz |
| `CFG_sx1276_radio` | LoRa radio chip selection | `1` | SX1276 |

#### Regional LoRa Configuration Examples
```ini
# EU868 Configuration
-D CFG_eu868=1
-D CUSTOM_LORA_FQZ="EU 868 MHz"

# US915 Configuration  
-D CFG_us915=1
-D CUSTOM_LORA_FQZ="US/CAN 915 MHz"

# Japan Special Configuration
-D CFG_as923=1
-D LMIC_COUNTRY_CODE=LMIC_COUNTRY_CODE_JP
```

---

## 📋 Build Instructions

### Prerequisites
- **PlatformIO**: Latest version with ESP32 platform support
- **Git**: For repository management and library dependencies
- **Python**: For PlatformIO package management
- **Complete Credentials**: All protocol credentials in `board_credentials.h`

### Installation Steps
```bash
# Clone repository
git clone https://github.com/artdanion/teleagriculture_board_V2.1.git
cd teleagriculture_board_V2.1

# Install PlatformIO
pip install platformio

# Create credentials file (copy from template)
cp include/board_credentials.h.template include/board_credentials.h
# Edit with your actual credentials

# Install dependencies
pio pkg install

# Build for your region
pio run -e EU

# Upload to device
pio run -e EU -t upload

# Monitor serial output
pio device monitor -b 115200
```

### Library Dependencies
All custom libraries are included in `/lib/` directory:
- Libraries automatically included based on build configuration
- Use `pio lib list` to verify all dependencies resolved
- Third-party libraries managed through `platformio.ini`

### Build Verification
```bash
# Check build size
pio run -e EU -t size

# Run static analysis  
pio check

# Upload filesystem data
pio run -e EU -t uploadfs
```

---

## 🤝 Contributing

### Development Guidelines
- **Logging**: Use `DebugLogger` from `debug_TAC` instead of raw `Serial.*` calls
- **Sensor Management**: Keep sensor definitions centralized in `def_Sensors.cpp` and `def_Sensors.h`
- **Documentation**: Update this documentation when adding protocols, flags, or sensors

### Adding New Features
1. **Identify Library**: Determine which `_TAC` library the feature belongs to
2. **API Design**: Follow existing patterns for function signatures and error handling  
3. **Integration**: Update main.cpp loop handlers as needed
4. **Configuration**: Add web portal fields if user-configurable
5. **Persistance Settings**: keep versions backwards compatibel (conditional load)
6. **Documentation**: Update relevant sections of this document

### Sensor Integration Process
1. **Definition**: Add to `lib/init_TAC/src/def_Sensors.cpp` JSON structure
2. **Enumeration**:  Update `SensorsImplemented` enum in sensor_Board.hpp
                     Raise SENSORS_NUM in `def_Sensors.h` by 1.
3. **Reading Logic**: Implement in appropriate connector handler in sensor_Read.hpp
4. **Protocol Support**: Add to LoRa/MQTT/OSC payload formatting (if new UNITS are used)
5. **Testing**: Verify on actual hardware with debug output

---

## 📃 License

MIT License © 2025 Daniel Fischer (artdanion)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**Third-party libraries** are covered under their respective licenses located in `/lib/` directories.