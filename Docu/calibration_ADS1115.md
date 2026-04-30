# ADS1115 Water Quality Sensors — Calibration Guide

> **Status:** Advanced sensors — web calibration wizard in active development.  
> Factory defaults are used on first boot if no calibration file exists.

---

## Hardware Setup

The optional **ADS1115** (16-bit I²C ADC) expands the board with four analog water-quality inputs.

| Channel | Sensor | Product |
|---------|--------|---------|
| ADC0 | ORP / Redox | [DFRobot SEN0464](https://wiki.dfrobot.com/Gravity_Analog_ORP_Sensor_PRO_SKU_SEN0464) |
| ADC1 | DO / Dissolved Oxygen | [DFRobot SEN0237](https://wiki.dfrobot.com/Gravity__Analog_Dissolved_Oxygen_Sensor_SKU_SEN0237) |
| ADC2 | EC / Electrical Conductivity | [DFRobot DFR0300 (K=1)](https://wiki.dfrobot.com/Gravity__Analog_Electrical_Conductivity_Sensor___Meter_V2__K=1__SKU_DFR0300) |
| ADC3 | pH | [DFRobot SEN0161-V2](https://wiki.dfrobot.com/Gravity__Analog_pH_Sensor_Meter_Kit_V2_SKU_SEN0161-V2) |

- I²C address: `0x48` (ADDR pin to GND), connected to the 3.3 V I²C rail  
- Gain: `GAIN_ONE` → ±4.096 V full scale, **1 bit = 0.125 mV**  
- Temperature reference: DS18B20 on ONEWIRE_1 (GPIO 39) is recommended.  
  Fallback order: BME/BMP reading → hardcoded 22 °C.

---

## Calibration Methods

### A) Web Calibration Wizard *(recommended)*

1. Connect to the board's WiFi, then navigate to `http://<board-ip>/calibrate`
2. Select a sensor — ADS1115 sensors are marked **Advanced · ADS1115**
3. Follow the step-by-step wizard. Live voltage readings update every 2 seconds directly from the ADS1115 — no multimeter needed.
4. Click **Set Point** when the reading is stable, then **Save**.
5. Values are written to `/board_cal.json` on SPIFFS and reloaded on every boot.

### B) Standalone Sketch *(legacy / advanced)*

The file [`cal_ADS1115.cpp`](cal_ADS1115.cpp) is a standalone PlatformIO sketch for bench calibration without WiFi. Flash it as a separate project, use the LEFT button to step through states, and read calibration values from the Serial Monitor (115200 baud).

```
States:  START → READ_ORP → DO_FIRST → DO_SECOND → EC → PH → FINISHED
```

Take note of the printed values, then enter them in the web wizard and save — or edit `cal_values.h` directly.

---

## Calibration Variables

These are defined in `include/cal_values.h` and stored in `/board_cal.json`:

| Variable | Default | Description |
|----------|---------|-------------|
| `cal_pH_neutral` | `1455.0 mV` | ADC reading in pH 7.0 buffer |
| `cal_pH_acid` | `2032.44 mV` | ADC reading in pH 4.0 buffer |
| `cal_EC_kvalue` | `1.0` | EC probe K factor |
| `cal_DO_sat_mV` | `1600.0 mV` | DO voltage in air at cal temperature |
| `cal_DO_sat_T` | `25.0 °C` | Temperature during DO calibration |
| `cal_ORP_offset` | `-1989.0 mV` | ORP ADC offset (3.3 V supply) |
| `cal_soil_air` | `2963` | Soil probe ADC reading in dry air |
| `cal_soil_water` | `1044` | Soil probe ADC reading in water |

---

## pH Calibration — 2-Point

**Required:** pH 4.0 buffer · pH 7.0 buffer · distilled water for rinsing

1. Rinse probe with distilled water, shake off excess.
2. Place in **pH 7.0** buffer. Wait 30–60 s until stable.  
   → Record as `cal_pH_neutral` *(typical: ~1455 mV at 25 °C)*
3. Rinse, place in **pH 4.0** buffer. Wait until stable.  
   → Record as `cal_pH_acid` *(typical: ~2032 mV at 25 °C)*

**Firmware formula:**
```
slope     = (7.0 - 4.0) / ((cal_pH_neutral - 1500) / 3 - (cal_pH_acid - 1500) / 3)
intercept = 7.0 - slope * (cal_pH_neutral - 1500) / 3
pH        = slope * (raw_mV - 1500) / 3 + intercept
```

Detection windows used by standalone sketch:
- pH 7.0 buffer: 1322–1678 mV
- pH 4.0 buffer: 1854–2210 mV

> **Note:** pH probes drift over time — re-calibrate every 1–4 weeks.  
> Store the probe in KCl storage solution, not distilled water.

---

## EC Calibration — K Value

**Required:** 1413 µS/cm standard solution *(or 12.88 mS/cm for high-range)*

1. Rinse probe, submerge in standard solution. Wait until stable.
2. Web wizard reads raw voltage from ADC2.
3. Enter the known EC value → wizard computes K:
```
rawEC  = raw_mV / 820 / 200
kvalue = known_EC_mS_cm / rawEC
```
4. Save.

**Firmware formula:**
```
rawEC = volts_mV / RES2 / ECREF    (RES2 = 820, ECREF = 200)
EC    = rawEC * cal_EC_kvalue / (1 + 0.0185 * (temperature - 25))
```

> EC is temperature-compensated using the DS18B20 reading (or fallback temperature).  
> Use 1413 µS/cm for freshwater / hydroponics; 12.88 mS/cm for saltwater / aquaponics.

---

## DO Calibration — Air Saturation

**Required:** Nothing — calibrate in open air. DS18B20 recommended for accurate temperature.

1. Expose the membrane tip to open air (or air-saturated DI water).
2. Enter current water temperature in the wizard.
3. Wait at least 60 s until voltage is stable.  
   → Records `cal_DO_sat_mV` and `cal_DO_sat_T`
4. Save.

**Firmware formula:**
```
V_sat = cal_DO_sat_mV + 35 * (temperature - cal_DO_sat_T)
DO    = volts1 * DO_Table[temp_int] / V_sat              (result in mg/L)
```

`DO_Table[41]` contains theoretical O₂ saturation values for 0–40 °C.

> The membrane must be wetted before calibration.  
> At altitudes above 500 m, dissolved oxygen saturation is lower — apply an altitude correction factor.

---

## ORP Calibration — Offset

**Required:** Certified ORP buffer solution  
*(e.g. Zobell solution = +228 mV at 25 °C, or Quinhydrone in pH 4.0 = +268 mV)*

1. Rinse probe, submerge in buffer. Wait until stable (allow ~60 s).
2. Enter the expected ORP value in mV.
3. Wizard computes:
```
cal_ORP_offset = expected_mV - raw_adc_mV
```
4. Save.

**Firmware formula:**
```
ORP_mV = (adc0 * 0.125) + cal_ORP_offset
```

Default offset `−1989.0 mV` is pre-calibrated for a 3.3 V supply where the ADC mid-point sits at ~2217 mV (= 0 mV ORP).

---

## Soil Moisture Calibration — 2-Point

**Required:** A cup of water. No chemicals.

1. Hold probe in open air (completely dry). Wait until stable.  
   → Record ADC as `cal_soil_air` *(typical: ~2963)*
2. Submerge probe in water up to the marked line. Wait until stable.  
   → Record ADC as `cal_soil_water` *(typical: ~1044)*
3. Save.

**Firmware formula:**
```
moisture_% = map(analogRead(pin), cal_soil_air, cal_soil_water, 0, 100)
```

> Capacitive probes read *lower* ADC values when wet.  
> Re-calibrate after replacing a probe — values vary between units.

---

## Troubleshooting

| Symptom | Likely cause |
|---------|-------------|
| `/calread` returns `"error"` | ADS1115 not found at 0x48 — check wiring, I²C address, and SW_3V3 power |
| pH reads far off | Probe dry or aging — soak 30 min in KCl solution before calibrating |
| EC K-value out of range (0.5–1.5) | Wrong standard solution, or probe not fully submerged |
| DO reads 0 mg/L | Membrane fouled or dry — clean and wet probe, then re-calibrate |
| ORP drifts slowly | Normal — allow 60 s stabilisation before setting the cal point |
| Soil moisture stuck at 0 % or 100 % | `cal_soil_air` and `cal_soil_water` are swapped or too close together |
