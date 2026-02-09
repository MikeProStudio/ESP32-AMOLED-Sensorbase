#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include <Arduino.h>

void init_logger();
void log_data_to_sd(float lux, float pitch, float roll, int aqi, int voc, int nox, float temp, float pres, float hum, float down, float up, float bat_v, int bat_pct);

#endif
