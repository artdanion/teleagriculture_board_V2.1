/*\
 *
 * TeleAgriCulture Board Sensor Read
 *
 * Copyright (c) 2023 artdanion
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
\*/

// TODO: a system for multible simular measurments, NAMING

#include <Arduino.h>
#include <esp_adc_cal.h>
#include <soc/adc_channel.h>
#include <unordered_map>
#include <map>

#include <Adafruit_Sensor.h>
#include <BMP280.h>
#include <Adafruit_BME280.h>
#include "Adafruit_VEML7700.h"
#include <Multichannel_Gas_GMXXX.h>
#include <MiCS6814-I2C.h>
#include <DHT.h>
#include <OneWire.h>
#include "OneWireNg_CurrentPlatform.h"
#include "drivers/DSTherm.h"
#include "utils/Placeholder.h"
#include <GravityTDS.h>
#include <Adafruit_ADS1X15.h>

#define VREF 3.3 // analog reference voltage(Volt) of the ADC
#define ADC_RES 4095
#define DHTTYPE DHT22
#define SEALEVELPRESSURE_HPA (1013.25)

// ----- Sensor section ----- //

/*

        TODO: optimizing the read process ... mostly timing stuff

        TODO: implementing sensor_Calibrate ... just a few have to be calibrated

*/

Sensor allSensors[SENSORS_NUM];
Measurement measurements[MEASURMENT_NUM];

// Global vector to store connected Sensor data
std::vector<Sensor> sensorVector;
std::vector<Measurement> show_measurements;

esp_adc_cal_characteristics_t adc_cal;

// flag for saving Connector data
bool shouldSaveConfig = false;

// ----- Sensor section ----- //

Adafruit_BME280 bme;
BMP280 bmp280;
Adafruit_VEML7700 veml = Adafruit_VEML7700();

GAS_GMXXX<TwoWire> gas;
MiCS6814 multiGasV1;

GravityTDS gravityTds;
Adafruit_ADS1115 ads;

float temperature = 22, tdsValue = 0;

bool parseI2CAddress(const String &addressString, uint8_t *addressValue);
void readI2C_Connectors();
void readADC_Connectors();
void readOneWire_Connectors();
void readI2C_5V_Connector();
void readSPI_Connector();
void readEXTRA_Connectors();
float getBatteryVoltage();
void updateDataNames(std::vector<Sensor> &sensorVector);

// LevelSensor
#define NO_TOUCH 0xFE
#define THRESHOLD 100
#define ATTINY1_HIGH_ADDR 0x78
#define ATTINY2_LOW_ADDR 0x77
unsigned char low_data[8] = {0};
unsigned char high_data[12] = {0};
void getHigh12SectionValue(void);
void getLow8SectionValue(void);
// Levelsensor

// TDS Sensor
#define SCOUNT 30 // sum of sample point
int getMedianNum(int bArray[], int iFilterLen);
// TDS Sensor

// OneWireNG
static bool printId(const OneWireNg::Id &id);
static void printScratchpad(const DSTherm::Scratchpad &scrpd);
// OneWireNg

// calibration values for: Gravity: Dissolved Oxygen Probe
#define TWO_POINT_CALIBRATION 1
// Single point calibration needs to be filled CAL1_V and CAL1_T
#define CAL1_V (1600) // mv
#define CAL1_T (25)   // ℃
// Two-point calibration needs to be filled CAL2_V and CAL2_T
// CAL1 High temperature point, CAL2 Low temperature point
#define CAL2_V (1300) // mv
#define CAL2_T (15)   // ℃

const uint16_t DO_Table[41] = {
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};
// calibration values for: Gravity: Dissolved Oxygen Probe

// calibration values for: Gravity: Analog Electrical Conductivity Sensor
#define RES2 820.0
#define ECREF 200.0
float kvalue = 1.0;
float kvalueLow = 1.02;
float kvalueHigh = 1.22;
// calibration values for: Gravity: Analog Electrical Conductivity Sensor

// calibration values for: Gravity: Analog pH Sensor / Meter Kit V2
float phValue = 7.0;
float acidVoltage = 2032.44;   // buffer solution 4.0 at 22C
float neutralVoltage = 1455.0; // buffer solution 7.0 at 22C
// calibration values for: Gravity: Analog pH Sensor / Meter Kit V2

void sensorRead()
{
    digitalWrite(LED, HIGH);
    pinMode(SW_3V3, OUTPUT);
    pinMode(SW_5V, OUTPUT);

    digitalWrite(SW_3V3, HIGH);
    digitalWrite(SW_5V, HIGH);

    sensorVector.clear();

    // TwoWire I2CCON = TwoWire(0);
    // I2CCON.begin(I2C_SDA, I2C_SCL);

    Wire.setPins(I2C_SDA, I2C_SCL);

    if (I2C_5V_con_table[0] == MULTIGAS_V1)
    {
        // Connect to sensor using default I2C address (0x04)
        // Alternatively the address can be passed to begin(addr)

        if (multiGasV1.begin(0x04) == true)
        {
            // Turn heater element on
            multiGasV1.powerOn();
            delay(20000); // heat up 20sek
        }
        else
        {
            // Print error message on failed connection
            Serial.println("MultiChannel Gas Sensor V1 not found on this address");
        }
    }

    Sensor newSensor = allSensors[BATTERY];
    newSensor.measurements->value = getBatteryVoltage();
    sensorVector.push_back(newSensor);

    Serial.println("SensorRead.....");
    Serial.println();

    readOneWire_Connectors();
    readADC_Connectors();
    readI2C_Connectors();
    readI2C_5V_Connector();
    readEXTRA_Connectors();

    // digitalWrite(SW_3V3, LOW);
    digitalWrite(SW_5V, LOW);
    digitalWrite(LED, LOW);

    updateDataNames(sensorVector); // adding # to sensor data_name (e.g. temp, temp1, temp2 ....)
}

void readI2C_Connectors()
{
    // TwoWire I2CCON = TwoWire(0);
    // I2CCON.begin(I2C_SDA, I2C_SCL);
    Wire.begin(I2C_SDA, I2C_SCL);

    for (int j = 0; j < I2C_NUM; j++)
    {
        switch (I2C_con_table[j])
        {
        case NO:
        {
            // Serial.print("\nNo Sensor attaches at ");
            // Serial.print("I2C_");
            // Serial.println(j + 1);
        }
        break;

        case BMP_280:
        {
            float pressure, altitude;
            uint8_t addressValue;

            // Parse the I2C address string
            // if (!parseI2CAddress((allSensors[j].i2c_add), &addressValue))
            // {
            //     printf("Error: Invalid I2C address %s\n", (allSensors[j].i2c_add));
            //     break;
            // }

            bmp280.begin();
            delay(100);
            bmp280.setConfigTStandby(BMP280::eConfigTStandby_t::eConfigTStandby_500);
            bmp280.setCtrlMeasSamplingTemp(BMP280::eSampling_t::eSampling_X4);
            bmp280.setCtrlMeasSamplingPress(BMP280::eSampling_t::eSampling_X4);
            delay(100);

            Sensor newSensor = allSensors[BMP_280];
            newSensor.measurements[0].value = bmp280.getTemperature();
            pressure = bmp280.getPressure();
            newSensor.measurements[1].value = (double)(pressure / 100.00F);
            newSensor.measurements[2].value = bmp280.calAltitude(pressure);

            sensorVector.push_back(newSensor);
        }
        break;

        case BME_280:
        {
            unsigned status;
            uint8_t addressValue;

            // Parse the I2C address string
            // if (!parseI2CAddress((allSensors[j].i2c_add), &addressValue))
            // {
            //     printf("Error: Invalid I2C address %s\n", (allSensors[j].i2c_add));
            //     break;
            // }

            status = bme.begin(0x76, &Wire); // addressValue, &I2CCON);
            if (!status)
            {
                Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
                Serial.print("SensorID was: 0x");
                Serial.println(bme.sensorID(), 16);
                Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
                Serial.print("        ID of 0x56-0x58 represents a BMP 280,\n");
                Serial.print("        ID of 0x60 represents a BME 280.\n");
                Serial.println("        ID of 0x61 represents a BME 680.\n");
                break;
            }
            // Serial.print(bme.readTemperature());
            // Serial.print(1.8 * bme.readTemperature() + 32);

            Sensor newSensor = allSensors[BME_280];
            newSensor.measurements[0].value = bme.readHumidity();
            newSensor.measurements[1].value = bme.readTemperature();
            newSensor.measurements[2].value = (bme.readPressure() / 100.0F);
            newSensor.measurements[3].value = bme.readAltitude(SEALEVELPRESSURE_HPA);
            sensorVector.push_back(newSensor);
        }
        break;

        case LEVEL:
        {
            //          https://github.com/SeeedDocument/Grove-Water-Level-Sensor/blob/master/water-level-sensor-demo.ino
            int sensorvalue_min = 250;
            int sensorvalue_max = 255;
            uint32_t touch_val = 0;
            uint8_t trig_section = 0;
            int low_count = 0;
            int high_count = 0;

            for (int i = 0; i < 8; i++)
            {

                if (low_data[i] >= sensorvalue_min && low_data[i] <= sensorvalue_max)
                {
                    low_count++;
                }
            }
            for (int i = 0; i < 12; i++)
            {
                if (high_data[i] >= sensorvalue_min && high_data[i] <= sensorvalue_max)
                {
                    high_count++;
                }
            }

            for (int i = 0; i < 8; i++)
            {
                if (low_data[i] > THRESHOLD)
                {
                    touch_val |= 1 << i;
                }
            }
            for (int i = 0; i < 12; i++)
            {
                if (high_data[i] > THRESHOLD)
                {
                    touch_val |= (uint32_t)1 << (8 + i);
                }
            }

            while (touch_val & 0x01)
            {
                trig_section++;
                touch_val >>= 1;
            }
            Sensor newSensor = allSensors[LEVEL];
            newSensor.measurements[0].value = trig_section * 5;

            sensorVector.push_back(newSensor);
        }
        break;

        case VEML7700:
        {
            //          See Vishy App Note "Designing the VEML7700 Into an Application"
            //          Vishay Document Number: 84323, Fig. 24 Flow Chart
            if (!veml.begin(&Wire))
            {
                Serial.println("Sensor VEML7700 not found");
                break;
            }
            // allSensors[VEML7700].measurements[0].value = veml.readLux(VEML_LUX_AUTO);

            Sensor newSensor = allSensors[VEML7700];
            newSensor.measurements[0].value = veml.readLux(VEML_LUX_AUTO);
            newSensor.measurements[1].value = veml.readALS();

            sensorVector.push_back(newSensor);
        }
        break;

        case ADS1115:
        {
            ads.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 0.125mV
            if (!ads.begin(0x48, &Wire))
            {
                Serial.println("16-Bit ADC ADS1115 not found");
                break;
            }
            delay(2000); // wait for sensors

            int16_t adc0, adc1, adc2, adc3;
            float volts0, volts1, volts2, volts3;

            adc0 = ads.readADC_SingleEnded(0);
            adc1 = ads.readADC_SingleEnded(1);
            adc2 = ads.readADC_SingleEnded(2);
            adc3 = ads.readADC_SingleEnded(3);

            volts0 = (adc0 * 0.125) - 1989; // adc*resolution 1gain - offset (-2480 on 5V)
            volts1 = (adc1 * 0.125);        // adc*resolution 1gain
            volts2 = (adc2 * 0.125);        // adc*resolution 1gain
            volts3 = (adc3 * 0.125);        // adc*resolution 1gain

            // Serial.print("\nvolt 0: ");
            // Serial.println(volts0);

            // Serial.print("\nvolt 1: ");
            // Serial.println(volts1);

            // Serial.print("\nvolt 2: ");
            // Serial.println(volts2);

            // Serial.print("\nvolt 3: ");
            // Serial.println(volts3);

            Sensor newSensor = allSensors[ADS1115];

            // ORP calc
            newSensor.measurements[0].value = volts0; // ORP in mV

            // DO calc
            if (TWO_POINT_CALIBRATION == 0)
            {
                uint16_t V_saturation = (uint32_t)CAL1_V + (uint32_t)35 * temperature - (uint32_t)CAL1_T * 35;
                newSensor.measurements[1].value = (volts1 * DO_Table[int(temperature)] / V_saturation);
            }
            else
            {
                uint16_t V_saturation = (int16_t)((int8_t)temperature - CAL2_T) * ((uint16_t)CAL1_V - CAL2_V) / ((uint8_t)CAL1_T - CAL2_T) + CAL2_V;
                newSensor.measurements[1].value = (volts1 * DO_Table[int(temperature)] / V_saturation);
            }

            // EC calc
            float value = 0, valueTemp = 0, rawEC = 0;

            rawEC = volts2 / RES2 / ECREF;
            valueTemp = rawEC * kvalue;

            if (valueTemp > 2.5)
            {
                kvalue = kvalueHigh;
            }
            else if (valueTemp < 2.0)
            {
                kvalue = kvalueLow;
            }

            value = rawEC * kvalue;                                // calculate the EC value after automatic shift
            value = value / (1.0 + 0.0185 * (temperature - 25.0)); // temperature compensation
            newSensor.measurements[2].value = value;

            // pH calc
            float slope = (7.0 - 4.0) / ((neutralVoltage - 1500.0) / 3.0 - (acidVoltage - 1500.0) / 3.0); // two point: (_neutralVoltage,7.0),(_acidVoltage,4.0)
            float intercept = 7.0 - slope * (neutralVoltage - 1500.0) / 3.0;

            phValue = slope * (volts3 - 1500.0) / 3.0 + intercept; // y = k*x + b
            newSensor.measurements[3].value = phValue;

            sensorVector.push_back(newSensor);
        }
        break;

        default:
            break;
        }
    }
}

void readADC_Connectors()
{
    for (int i = 0; i < ADC_NUM; i++)
    {
        switch (ADC_con_table[i])
        {
        case NO:
        {
            // Serial.print("\nNo Sensor attaches at ");
            // Serial.print("ADC_");
            // Serial.println(i + 1);
        }
        break;

        case TDS:
        {

            int analogBuffer[SCOUNT]; // store the analog value in the array, read from ADC
            int analogBufferTemp[SCOUNT];
            int analogBufferIndex = 0, copyIndex = 0;
            float averageVoltage = 0, tdsValue = 0;

            int tdsSensorPin;

            if (i == 0)
            {
                tdsSensorPin = ANALOG1;
            }

            if (i == 1)
            {
                tdsSensorPin = ANALOG2;
            }

            if (i == 2)
            {
                tdsSensorPin = ANALOG3;
            }

            gravityTds.setPin(tdsSensorPin);
            gravityTds.setAref(3.3);      // reference voltage on ADC, default 5.0V on Arduino UNO
            gravityTds.setAdcRange(4096); // 1024 for 10bit ADC;4096 for 12bit ADC
            gravityTds.begin();           // initialization

            gravityTds.setTemperature(temperature); // set the temperature and execute temperature compensation
            gravityTds.update();                    // sample and calculate
            tdsValue = gravityTds.getTdsValue();    // then get the value

            Sensor newSensor = allSensors[TDS];
            newSensor.measurements[0].value = tdsValue;

            sensorVector.push_back(newSensor);

            // pinMode(tdsSensorPin, INPUT);

            //          www.cqrobot.wiki/index.php/TDS_(Total_Dissolved_Solids)_Meter_Sensor_SKU:_CQRSENTDS01
            // unsigned long messureTime = millis();
            // do
            // {
            //     static unsigned long analogSampleTimepoint = millis();
            //     if (millis() - analogSampleTimepoint > 40U) // every 40 milliseconds,read the analog value from the ADC
            //     {
            //         analogSampleTimepoint = millis();
            //         analogBuffer[analogBufferIndex] = analogRead(tdsSensorPin); // read the analog value and store into the buffer
            //         analogBufferIndex++;
            //         if (analogBufferIndex == SCOUNT)
            //             analogBufferIndex = 0;
            //     }
            //     static unsigned long printTimepoint = millis();
            //     if (millis() - printTimepoint > 1800U)
            //     {
            //         printTimepoint = millis();
            //         for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
            //             analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
            //         averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;                                                                                                  // read the analog value more stable by the median filtering algorithm, and convert to voltage value
            //         float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);                                                                                                               // temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
            //         float compensationVolatge = averageVoltage / compensationCoefficient;                                                                                                            // temperature compensation
            //         tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; // convert voltage value to tds value

            //         Sensor newSensor = allSensors[TDS];
            //         newSensor.measurements[0].value = tdsValue;

            //         sensorVector.push_back(newSensor);
            //     }
            // } while (millis() - messureTime < 2000U);
        }
        break;

        case CAP_SOIL:
        {
            int cap_SoilPin;
            if (i == 0)
            {
                cap_SoilPin = ANALOG1;
            }

            if (i == 1)
            {
                cap_SoilPin = ANALOG2;
            }

            if (i == 2)
            {
                cap_SoilPin = ANALOG3;
            }

            pinMode(cap_SoilPin, INPUT);

            const int AirValue = 2963;
            const int WaterValue = 1044;
            int soilmoisturepercent = 0;
            soilmoisturepercent = map(analogRead(cap_SoilPin), AirValue, WaterValue, 0, 100);

            Sensor newSensor = allSensors[CAP_SOIL];
            newSensor.measurements[0].value = soilmoisturepercent;

            sensorVector.push_back(newSensor);
        }
        break;

        case CAP_GROOVE:
        {
            int cap_GroovePin;
            bool cal_grooveConnected = false;
            if (i == 0)
            {
                cap_GroovePin = ANALOG1;
            }

            if (i == 1)
            {
                cap_GroovePin = ANALOG2;
            }

            if (i == 2)
            {
                cap_GroovePin = ANALOG3;
            }

            pinMode(cap_GroovePin, INPUT);

            Sensor newSensor = allSensors[CAP_GROOVE];
            newSensor.measurements[0].value = analogRead(cap_GroovePin);

            sensorVector.push_back(newSensor);
        }
        break;

        case SOUND:
        {
            int soundPin;
            uint32_t raw;
            uint32_t millivolts;

            if (i == 0)
            {
                soundPin = ANALOG1;
                raw = adc1_get_raw(ADC1_CHANNEL_4);
                millivolts = esp_adc_cal_raw_to_voltage(raw, &adc_cal);
            }

            if (i == 1)
            {
                soundPin = ANALOG2;
                raw = adc1_get_raw(ADC1_CHANNEL_5);
                millivolts = esp_adc_cal_raw_to_voltage(raw, &adc_cal);
            }

            if (i == 2)
            {
                soundPin = ANALOG3;
                raw = adc1_get_raw(ADC1_CHANNEL_6);
                millivolts = esp_adc_cal_raw_to_voltage(raw, &adc_cal);
            }

            pinMode(soundPin, INPUT);

            Sensor newSensor = allSensors[SOUND];
            newSensor.measurements[0].value = millivolts * 50; // mV to decibel value by *50
            sensorVector.push_back(newSensor);
        }
        break;

        default:
            break;
        }
    }
}

void readOneWire_Connectors()
{
    for (int OWi = 0; OWi < ONEWIRE_NUM; OWi++)
    {
        switch (OneWire_con_table[OWi])
        {
        case NO:
        {
            // Serial.print("\nNo Sensor attaches at ");
            // Serial.print("1-Wire_");
            // Serial.println(OWi + 1);
        }
        break;

        case DHT_22:
        {
            int dht22SensorPin;
            if (OWi == 0)
            {
                dht22SensorPin = ONEWIRE_1;
            }

            if (OWi == 1)
            {
                dht22SensorPin = ONEWIRE_2;
            }

            if (OWi == 2)
            {
                dht22SensorPin = ONEWIRE_3;
            }

            DHT dht(dht22SensorPin, DHTTYPE);
            dht.begin(dht22SensorPin);
            delay(2000);

            Sensor newSensor = allSensors[DHT_22];

            newSensor.measurements[0].value = dht.readTemperature();
            newSensor.measurements[1].value = dht.readHumidity();

            sensorVector.push_back(newSensor);
        }
        break;

        case DS18B20:
        {

#define PARASITE_POWER_ARG false

            int ds18b20SensorPin;
            if (OWi == 0)
            {
                ds18b20SensorPin = ONEWIRE_1;
            }

            if (OWi == 1)
            {
                ds18b20SensorPin = ONEWIRE_2;
            }

            if (OWi == 2)
            {
                ds18b20SensorPin = ONEWIRE_3;
            }

            static Placeholder<OneWireNg_CurrentPlatform> ow;
            new (&ow) OneWireNg_CurrentPlatform(ds18b20SensorPin, false);
            DSTherm drv(ow);

#if (CONFIG_MAX_SEARCH_FILTERS > 0)
            drv.filterSupportedSlaves();
#endif

            /* convert temperature on all sensors connected... */
            drv.convertTempAll(DSTherm::MAX_CONV_TIME, PARASITE_POWER_ARG);

            /* read sensors one-by-one */
            Placeholder<DSTherm::Scratchpad> scrpd;

            // for (const auto &id : *ow)
            // {
            //     if (printId(id))
            //     {
            //         if (drv.readScratchpad(id, scrpd) == OneWireNg::EC_SUCCESS)
            //             printScratchpad(scrpd);
            //         else
            //             Serial.println("  Read scratchpad error.");
            //     }
            // }

            /* read sensors one-by-one and print temperature */
            for (const auto &id : *ow)
            {
                if (drv.readScratchpad(id, scrpd) == OneWireNg::EC_SUCCESS)
                {
                    /* get temperature and print */
                    long temp = scrpd->getTemp();
                    Sensor newSensor = allSensors[DS18B20];

                    newSensor.measurements[0].value = static_cast<double>(temp) / 1000.0;
                    sensorVector.push_back(newSensor);
                    temperature = float(newSensor.measurements[0].value);
                    Serial.print("\nTemp used: ");
                    Serial.print(temperature);
                }
            }
        }
        break;

        default:
            break;
        }
    }
}

void readI2C_5V_Connector()
{
    if (I2C_5V_con_table[0] == MULTIGAS)
    {
        gas.begin(Wire, 0x08); // use the hardware I2C
        static uint8_t recv_cmd[8] = {};
        int PPM = 0;
        double lgPPM;

        delay(200);

        /* Multichannel sensor Grove V2, by vcoder 2021
            CO range 0 - 1000 PPM
            Calibrated according calibration curve by Winsen: 0 - 150 PPM

        RS/R0       PPM
        1           0
        0.77        1
        0.6         3
        0.53        5
        0.4         10
        0.29        20
        0.21        50
        0.17        100
        0.15        150
        */

        uint8_t len = 0;
        uint8_t addr = 0;
        uint8_t i;
        uint32_t val = 0;

        float sensor_volt;
        float RS_gas;
        float R0;

        float ratio;
        unsigned long messureTime = millis();

        float sensorValue = 0;

        sensorValue = gas.measure_CO();

        sensor_volt = (sensorValue / 1024) * 3.3;
        RS_gas = (3.3 - sensor_volt) / sensor_volt;

        R0 = 3.21; // measured on ambient air
        ratio = RS_gas / R0;
        // ratio = 1; //it is for tests of the calibration curve

        lgPPM = (log10(ratio) * -2.82) - 0.12; //- 3.82) - 0.66; - default      - 2.82) - 0.12; - best for range up to 150 ppm

        PPM = pow(10, lgPPM);

        Sensor newSensor = allSensors[MULTIGAS];
        newSensor.measurements[0].value = PPM;

        delay(200);

        /* Multichannel sensor Grove V2, by vcoder 2021
            NO2 range 0 - 10 PPM
            Calibrated according calibration curve by Winsen: 0 - 10 PPM

        RS/R0       PPM
        1           0
        1.4         1
        1.8         2
        2.25        3
        2.7         4
        3.1         5
        3.4         6
        3.8         7
        4.2         8
        4.4         9
        4.7         10
        */

        sensorValue = 0;

        sensorValue = gas.measure_NO2();

        sensor_volt = (sensorValue / 1024) * 3.3;
        RS_gas = (3.3 - sensor_volt) / sensor_volt;

        R0 = 1.07; // measured on ambient air
        ratio = RS_gas / R0;
        // ratio = 4.7; //for tests of the calibration curve

        lgPPM = (log10(ratio) * +1.9) - 0.2; //+2   -0.3

        PPM = pow(10, lgPPM);

        delay(10);

        newSensor.measurements[1].value = PPM;

        sensorVector.push_back(newSensor);

        Wire.end();
    }
    else if (I2C_5V_con_table[0] == MULTIGAS_V1)
    {
        MiCS6814 multiGasV1;
        // Connect to sensor using default I2C address (0x04)
        // Alternatively the address can be passed to begin(addr)

        multiGasV1.begin(0x04);
        multiGasV1.powerOn();

        delay(200);

        Sensor newSensor = allSensors[MULTIGAS_V1];

        newSensor.measurements[0].value = multiGasV1.measureCO();
        newSensor.measurements[1].value = multiGasV1.measureNO2();
        newSensor.measurements[2].value = multiGasV1.measureNH3();
        newSensor.measurements[3].value = multiGasV1.measureC3H8();
        newSensor.measurements[4].value = multiGasV1.measureC4H10();
        newSensor.measurements[5].value = multiGasV1.measureCH4();
        newSensor.measurements[6].value = multiGasV1.measureH2();
        newSensor.measurements[7].value = multiGasV1.measureC2H5OH();

        sensorVector.push_back(newSensor);

        multiGasV1.powerOff();
    }
    else
    {
        // Serial.print("\nNo Sensor attaches at ");
        // Serial.println("I2C_5V");
    }
}

void readSPI_Connector()
{
    if (SPI_con_table[0] != NO)
    {
        Serial.println("\nNo SPI Sensor implemented now...");
    }
    else
    {
        Serial.print("\nNo Sensor attaches at ");
        Serial.println("SPI_CON");
    }
}

void readEXTRA_Connectors()
{
}

bool parseI2CAddress(const String &addressString, uint8_t *addressValue)
{
    char buffer[5];
    addressString.toCharArray(buffer, sizeof(buffer));
    char *endPtr;
    *addressValue = strtol(buffer, &endPtr, 16);

    if (*endPtr != '\0' || endPtr == buffer ||
        *addressValue > 0xFF)
    {
        return false;
    }

    return true;
}

void getHigh12SectionValue(void)
{
    memset(high_data, 0, sizeof(high_data));
    Wire.requestFrom(ATTINY1_HIGH_ADDR, 12);
    while (12 != Wire.available())
        ;

    for (int i = 0; i < 12; i++)
    {
        high_data[i] = Wire.read();
    }
    delay(10);
}

void getLow8SectionValue(void)
{
    memset(low_data, 0, sizeof(low_data));
    Wire.requestFrom(ATTINY2_LOW_ADDR, 8);
    while (8 != Wire.available())
        ;

    for (int i = 0; i < 8; i++)
    {
        low_data[i] = Wire.read(); // receive a byte as character
    }
    delay(10);
}

int getMedianNum(int bArray[], int iFilterLen)
{
    int bTab[iFilterLen];
    for (byte i = 0; i < iFilterLen; i++)
        bTab[i] = bArray[i];
    int i, j, bTemp;
    for (j = 0; j < iFilterLen - 1; j++)
    {
        for (i = 0; i < iFilterLen - j - 1; i++)
        {
            if (bTab[i] > bTab[i + 1])
            {
                bTemp = bTab[i];
                bTab[i] = bTab[i + 1];
                bTab[i + 1] = bTemp;
            }
        }
    }
    if ((iFilterLen & 1) > 0)
        bTemp = bTab[(iFilterLen - 1) / 2];
    else
        bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
    return bTemp;
}

/* returns false if not supported */
static bool printId(const OneWireNg::Id &id)
{
    const char *name = DSTherm::getFamilyName(id);

    Serial.print(id[0], HEX);
    for (size_t i = 1; i < sizeof(OneWireNg::Id); i++)
    {
        Serial.print(':');
        Serial.print(id[i], HEX);
    }
    if (name)
    {
        Serial.print(" -> ");
        Serial.print(name);
    }
    Serial.println();

    return (name != NULL);
}

static void printScratchpad(const DSTherm::Scratchpad &scrpd)
{
    const uint8_t *scrpd_raw = scrpd.getRaw();

    Serial.print("  Scratchpad:");
    for (size_t i = 0; i < DSTherm::Scratchpad::LENGTH; i++)
    {
        Serial.print(!i ? ' ' : ':');
        Serial.print(scrpd_raw[i], HEX);
    }

    Serial.print("; Th:");
    Serial.print(scrpd.getTh());

    Serial.print("; Tl:");
    Serial.print(scrpd.getTl());

    Serial.print("; Resolution:");
    Serial.print(9 + (int)(scrpd.getResolution() - DSTherm::RES_9_BIT));

    long temp = scrpd.getTemp();
    Serial.print("; Temp:");
    if (temp < 0)
    {
        temp = -temp;
        Serial.print('-');
    }
    Serial.print(temp / 1000);
    Serial.print('.');
    Serial.print(temp % 1000);
    Serial.print(" C");

    Serial.println();
}

float getBatteryVoltage()
{
    uint32_t raw = adc1_get_raw(ADC1_CHANNEL_3);
    uint32_t millivolts = esp_adc_cal_raw_to_voltage(raw, &adc_cal);
    const uint32_t upper_divider = 442;
    const uint32_t lower_divider = 160;
    return (float)(upper_divider + lower_divider) / lower_divider / 1000 * millivolts;
}

void updateDataNames(std::vector<Sensor> &sensorVector)
{
    std::unordered_map<ValueOrder, int> valueOrderCount;

    for (Sensor &sensor : sensorVector)
    {
        for (Measurement &measurement : sensor.measurements)
        {
            ValueOrder valueOrder = measurement.valueOrder;
            if (valueOrderCount.find(valueOrder) == valueOrderCount.end())
            {
                valueOrderCount[valueOrder] = 0;
            }
            int count = ++valueOrderCount[valueOrder];
            if (count > 1)
            {
                measurement.data_name += String(count - 1);
            }
        }
    }
}