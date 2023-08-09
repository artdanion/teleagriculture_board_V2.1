/*

    Sensor Calibration for Aquaponic Sensors (on ADS1115 4-Channel-Board)

    - ADS1115: for KlimaOasis aquaponic system a 4-channel ADC is added
    - - ADC0: https://wiki.dfrobot.com/Gravity_Analog_ORP_Sensor_PRO_SKU_SEN0464
    - - ADC1: https://wiki.dfrobot.com/Gravity__Analog_Dissolved_Oxygen_Sensor_SKU_SEN0237
    - - ADC2: https://wiki.dfrobot.com/Gravity__Analog_Electrical_Conductivity_Sensor___Meter_V2__K=1__SKU_DFR0300
    - - ADC3: https://wiki.dfrobot.com/Gravity__Analog_pH_Sensor_Meter_Kit_V2_SKU_SEN0161-V2

*/

#include <Arduino.h>
#include "esp_system.h"
#include "sdkconfig.h"
#include <esp_adc_cal.h>

#include <Wire.h>
#include "driver/i2c.h"
#include <OneWire.h>
#include "OneWireNg_CurrentPlatform.h"
#include "drivers/DSTherm.h"
#include "utils/Placeholder.h"
#include <Adafruit_ADS1X15.h>
#include <Button.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Adafruit_ST7735.h>

#define ONEWIRE_1 39
#define ONEWIRE_2 40
#define ONEWIRE_3 41

#define ANALOG1 5
#define ANALOG2 6
#define ANALOG3 7
#define BATSENS 4

esp_adc_cal_characteristics_t adc_cal;

#define SW_3V3 42
#define SW_5V 47
#define LED 21

#define VREF 3.3
#define ADC_RES 4095
#define PARASITE_POWER_ARG false

#define LEFT_BUTTON_PIN 0
#define RIGHT_BUTTON_PIN 16

#define I2C_SDA 8
#define I2C_SCL 9

#define TFT_SCLK 36
#define TFT_MISO 37
#define TFT_MOSI 35
#define SPI_CON_CS 38
#define TFT_CS 15
#define TFT_RST 1
#define TFT_DC 2
#define TFT_BL 48

// ----- Initialize TFT ----- //
#define ST7735_TFTWIDTH 128
#define ST7735_TFTHEIGHT 160
#define background_color 0x07FF

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

int backlight_pwm = 250;

// ----- Initialize Buttons ----- //
Button upButton(LEFT_BUTTON_PIN);
Button downButton(RIGHT_BUTTON_PIN);

float temp = 22.0;
Adafruit_ADS1115 ads;

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
float kvalue = 0.98;
float kvalueLow = 0.97;
float kvalueHigh = 1.0;
// calibration values for: Gravity: Analog Electrical Conductivity Sensor

// calibration values for: Gravity: Analog pH Sensor / Meter Kit V2
float phValue = 7.0;
float acidVoltage = 2032.44;   // buffer solution 4.0 at 25C
float neutralVoltage = 1500.0; // buffer solution 7.0 at 25C
// calibration values for: Gravity: Analog pH Sensor / Meter Kit V2

// OneWireNG
void readDS18B();
static bool printId(const OneWireNg::Id &id);
static void printScratchpad(const DSTherm::Scratchpad &scrpd);
// OneWireNg

// Define the states
enum State
{
  START,
  READ_ORP,
  DO_FIRST,
  DO_SECOND,
  EC,
  PH,
  FINISHED
};

State currentState = START;
bool taskCompleted = true; // Flag to track task completion status

void setup()
{
  pinMode(TFT_BL, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(SW_3V3, OUTPUT);
  pinMode(SW_5V, OUTPUT);

  // calibrate ADC1
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_2_5, ADC_WIDTH_BIT_12, 0, &adc_cal);
  adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_2_5); // BATSENS
  adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_2_5); // ADC1_CON
  adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_2_5); // ADC2_CON
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_2_5); // ADC3_CON

  digitalWrite(SW_3V3, HIGH);
  Wire.setPins(I2C_SDA, I2C_SCL);

  upButton.begin();
  downButton.begin();

  delay(500);               // for debugging in screen
  Serial.setTxTimeoutMs(5); // set USB CDC Time TX
  Serial.begin(115200);     // start Serial for debuging

  analogWrite(TFT_BL, backlight_pwm); // turn on TFT Backlight

  ads.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 0.125mV

  if (!ads.begin(0x48, &Wire))
  {
    Serial.println("Failed to initialize ADS.");
    while (1)
      ;
  }

  // ----- Initiate the TFT display and Start Image----- //
  tft.initR(INITR_GREENTAB); // work around to set protected offset values
  tft.initR(INITR_BLACKTAB); // change the colormode back, offset values stay as "green display"

  tft.cp437(true);
  tft.setCursor(0, 0);
  tft.setRotation(3);

  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(0x9E6F);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(0);

  readDS18B(); // temperatur basement for calibration
  Wire.begin(I2C_SDA, I2C_SCL);
}

void loop()
{
  int16_t adc0, adc1, adc2, adc3;
  float volts0, volts1, volts2, volts3;

  // Check button press if the task is completed
  if (taskCompleted && upButton.pressed())
  {
    // Increment the current state
    currentState = static_cast<State>((currentState + 1) % (FINISHED + 1));

    // Set the task completion flag to false
    taskCompleted = false;

    // Print the current state
    Serial.print("Button pressed. Current state: ");
    Serial.println(currentState);
  }

  // Check button presses based on the current state
  switch (currentState)
  {

  case START:
  {
    Serial.println("\nPress Button to start Calibration");

    tft.fillScreen(ST7735_BLACK);
    tft.setTextSize(1);
    tft.setCursor(10, 30);
    tft.setTextColor(ST7735_WHITE);
    tft.print("Callibration");
    tft.setCursor(10, 50);
    tft.print("Press Button");
    delay(500);
    taskCompleted = true; // Set the task completion flag to true
  }
  break;

  case READ_ORP:
  {
    // Perform ORP read
    adc0 = ads.readADC_SingleEnded(0);
    volts0 = (adc0 * 0.125) - 1989;
    delay(100);
    Serial.print("ORP value: ");
    Serial.println(volts0);

    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(5, 30);
    tft.print("ORP: ");
    tft.setCursor(5, 50);
    tft.print(volts0);
    tft.print(" mV");
    tft.setCursor(5, 70);
    tft.print("temp: ");
    tft.print(temp);
    tft.drawChar(tft.getCursorX(), tft.getCursorY(), 0xF8, ST7735_YELLOW, ST7735_BLACK, 1);
    tft.setCursor(tft.getCursorX() + 7, tft.getCursorY());
    tft.print("C");

    delay(500);
    taskCompleted = true; // Set the task completion flag to true
  }
  break;

  case DO_FIRST:
  {
    // Perform DO_FIRST reading
    adc1 = ads.readADC_SingleEnded(1);
    volts1 = (adc1 * 0.125);

    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(5, 30);
    tft.print("DO 1: ");
    tft.setCursor(5, 50);
    tft.print(volts1);
    tft.print(" mV");
    tft.setCursor(5, 70);
    tft.print("temp: ");
    tft.print(temp);
    tft.drawChar(tft.getCursorX(), tft.getCursorY(), 0xF8, ST7735_YELLOW, ST7735_BLACK, 1);
    tft.setCursor(tft.getCursorX() + 7, tft.getCursorY());
    tft.print("C");

    delay(500);
    taskCompleted = true; // Set the task completion flag to true
  }
  break;

  case DO_SECOND:
  {
    // Perform DO_SECOND reading
    adc1 = ads.readADC_SingleEnded(1);
    volts1 = (adc1 * 0.125);

    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(5, 30);
    tft.print("DO 2: ");
    tft.setCursor(5, 50);
    tft.print(volts1);
    tft.print(" mV");
    tft.setCursor(5, 70);
    tft.print("temp: ");
    tft.print(temp);
    tft.drawChar(tft.getCursorX(), tft.getCursorY(), 0xF8, ST7735_YELLOW, ST7735_BLACK, 1);
    tft.setCursor(tft.getCursorX() + 7, tft.getCursorY());
    tft.print("C");

    delay(500);
    taskCompleted = true; // Set the task completion flag to true
  }
  break;

  case EC:
  {
    // Perform EC reading
    float value = 0, valueTemp = 0, rawEC = 0, compECsolution = 0, KValueTemp = 0;
    adc2 = ads.readADC_SingleEnded(2);
    volts2 = (adc2 * 0.125);

    rawEC = 1000 * volts2 / RES2 / ECREF;

    if ((rawEC > 0.9) && (rawEC < 1.9))
    {                                                          // recognize 1.413us/cm buffer solution
      compECsolution = 1.413 * (1.0 + 0.0185 * (temp - 25.0)); // temperature compensation
    }
    else if ((rawEC > 9) && (rawEC < 16.8))
    {                                                          // recognize 12.88ms/cm buffer solution
      compECsolution = 12.88 * (1.0 + 0.0185 * (temp - 25.0)); // temperature compensation
    }
    else
    {
      Serial.print(F(">>>Buffer Solution Error Try Again<<<   "));
    }

    KValueTemp = RES2 * ECREF * compECsolution / 1000.0 / volts2; // calibrate the k value
    if ((KValueTemp > 0.5) && (KValueTemp < 1.5))
    {
      Serial.println();
      Serial.print(F(">>>Successful,K:"));
      Serial.print(KValueTemp);
      Serial.println(F(", Send EXITEC to Save and Exit<<<"));
      if ((rawEC > 0.9) && (rawEC < 1.9))
      {
        kvalueLow = KValueTemp;
        Serial.print("K LOW: ");
        Serial.println(kvalueLow);

        tft.fillScreen(ST7735_BLACK);
        tft.setCursor(5, 30);
        tft.print("K LOW: ");
        tft.setCursor(5, 50);
        tft.print(kvalueLow);
        tft.setCursor(5, 70);
        tft.print("temp: ");
        tft.print(temp);
        tft.drawChar(tft.getCursorX(), tft.getCursorY(), 0xF8, ST7735_YELLOW, ST7735_BLACK, 1);
        tft.setCursor(tft.getCursorX() + 7, tft.getCursorY());
        tft.print("C");
      }
      else if ((rawEC > 9) && (rawEC < 16.8))
      {
        kvalueHigh = KValueTemp;
        Serial.print("K HIGH: ");
        Serial.println(kvalueHigh);

        tft.fillScreen(ST7735_BLACK);
        tft.setCursor(5, 30);
        tft.print("K HIGH: ");
        tft.setCursor(5, 50);
        tft.print(kvalueHigh);
        tft.setCursor(5, 70);
        tft.print("temp: ");
        tft.print(temp);
        tft.drawChar(tft.getCursorX(), tft.getCursorY(), 0xF8, ST7735_YELLOW, ST7735_BLACK, 1);
        tft.setCursor(tft.getCursorX() + 7, tft.getCursorY());
        tft.print("C");
      }
    }
    else
    {
      Serial.println();
      Serial.println(F(">>>Failed,Try Again<<<"));
      Serial.println();

      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(5, 30);
      tft.print("EC RAW: ");
      tft.print(rawEC);
      tft.setCursor(5, 50);
      tft.print("FAILD");
    }
    delay(500);
    taskCompleted = true; // Set the task completion flag to true
  }
  break;

  case PH:
  {
    // Perform PH reading
    adc3 = ads.readADC_SingleEnded(3);
    volts3 = (adc3 * 0.125);

    if ((volts3 > 1322) && (volts3 < 1678))
    { // buffer solution:7.0{
      Serial.println();
      Serial.println(F(">>>Buffer Solution:7.0"));
      neutralVoltage = volts3;
      Serial.print("neutralVoltage: ");
      Serial.print(volts3);
      Serial.println(" mv");
      Serial.println();

      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(5, 30);
      tft.print("neutralVoltage: ");
      tft.print(neutralVoltage);
      tft.print(" mV");
    }
    else if ((volts3 > 1854) && (volts3 < 2210))
    { // buffer solution:4.0
      Serial.println();
      Serial.println(F(">>>Buffer Solution:4.0"));
      acidVoltage = volts3;
      Serial.print("acidVoltage: ");
      Serial.print(volts3);
      Serial.println(" mv");
      Serial.println();

      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(5, 30);
      tft.print("acidVoltage: ");
      tft.print(acidVoltage);
      tft.print(" mV");
    }
    else
    {
      Serial.println();
      Serial.print(F(">>>Buffer Solution Error Try Again<<<"));
      Serial.println(); // not buffer solution or faulty operation

      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(5, 30);
      tft.print("Buffer Solution");
      tft.setCursor(5, 50);
      tft.print("Error Try Again");
      tft.setCursor(5, 70);
      tft.print(volts3);
      tft.print(" mV");
    }
    delay(500);
    taskCompleted = true; // Set the task completion flag to true
  }
  break;

  case FINISHED:
  {
    // Perform LAST_STATE reading
    Serial.println("\nCalibration finished");
    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(5, 30);
    tft.print("CALIBRATION");
    tft.setCursor(5, 50);
    tft.print("Finished");

    delay(500);
    taskCompleted = true; // Set the task completion flag to true
  }
  break;
  }
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

void readDS18B()
{
  static Placeholder<OneWireNg_CurrentPlatform> ow;
  new (&ow) OneWireNg_CurrentPlatform(ONEWIRE_1, false);
  DSTherm drv(ow);

#if (CONFIG_MAX_SEARCH_FILTERS > 0)
  drv.filterSupportedSlaves();
#endif

  /* convert temperature on all sensors connected... */
  drv.convertTempAll(DSTherm::MAX_CONV_TIME, PARASITE_POWER_ARG);

  /* read sensors one-by-one */
  Placeholder<DSTherm::Scratchpad> scrpd;

  /* read sensors one-by-one and print temperature */
  for (const auto &id : *ow)
  {
    if (drv.readScratchpad(id, scrpd) == OneWireNg::EC_SUCCESS)
    {
      /* get temperature and print */
      long temp = scrpd->getTemp();

      double tempTemp = static_cast<double>(temp) / 1000.0;
      temp = float(tempTemp);
      Serial.print("\nTemp used: ");
      Serial.print(temp);
    }
  }
}