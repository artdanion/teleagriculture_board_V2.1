#pragma once

// Calibration globals — defined in file_functions.cpp, loaded from /board_cal.json on boot.

extern float cal_pH_neutral;   // mV in pH 7.0 buffer  (default 1455.0)
extern float cal_pH_acid;      // mV in pH 4.0 buffer  (default 2032.44)
extern float cal_EC_kvalue;    // EC probe K value      (default 1.0)
extern float cal_DO_sat_mV;    // DO voltage in air-sat (default 1600.0)
extern float cal_DO_sat_T;     // Temperature at DO cal (default 25.0)
extern float cal_ORP_offset;   // ORP ADC offset in mV  (default -1989.0)
extern int   cal_soil_air;     // Soil ADC dry/air      (default 2963)
extern int   cal_soil_water;   // Soil ADC wet/water    (default 1044)
