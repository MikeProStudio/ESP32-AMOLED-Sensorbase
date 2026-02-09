#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <Arduino.h>

void get_sensor_readings(float *lux, int *aqi, int *voc, int *nox, float *temp, float *hum, float *pres, float *gx, float *gy, float *gz, float *pitch, float *roll);
float get_live_btc_price(void);

// --- Radar (UART) ---
// Implemented in src/data_manager.cpp
extern bool person_in_range;
extern bool person_detected;
extern float radar_distance;
void radar_task(void* p);
// Low-Level Funktionen (Portiert vom GitHub Repo)
void AHT20_begin();
void BMP280_begin();
void startMeasurementAHT20();
void checkbusyAHT20();
void getDataAHT20();
void readTemperatureBMP280();
void readPressureBMP280();

#endif

// --- Globale Variablen für Sensorwerte (für Low-Level-Code) ---
extern float temperature_AHT20;
extern float humidity;
extern float temperature_BMP280;
extern float pressure;



