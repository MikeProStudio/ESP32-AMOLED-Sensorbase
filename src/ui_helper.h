#ifndef UI_HELPER_H
#define UI_HELPER_H

#include <Arduino.h>

// Deklaration der Funktion, damit sie in der main.cpp aufgerufen werden kann
void update_ui_and_brightness(float lux, int aqi, int voc, int nox, float temp, float hum, float pres, float gx, float gy, float gz, float pitch, float roll, float radar_dist, bool person_detected);

// Falls du weitere Funktionen in der ui_helper.cpp hast, kommen sie auch hier rein
// void update_status_bar(bool wifi_connected); 

#endif