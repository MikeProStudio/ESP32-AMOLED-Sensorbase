#include <Arduino.h>
#include "lvgl.h"
#include "ui_app.h" 
#include "ui_helper.h"

// Prototypen aus der main.cpp
extern void set_amoled_brightness(uint8_t level);

void update_ui_and_brightness(float lux, int aqi, int voc, int nox, float temp, float hum, float pres, float gx, float gy, float gz, float pitch, float roll, float radar_dist, bool person_detected) {
    // 1. UI-Update über die Funktion in ui_app.c
    ui_set_sensor_data(lux, aqi, voc, nox, temp, hum, pres, gx, gy, gz, pitch, roll, radar_dist, person_detected);
    
    // 2. Charts aktualisieren
    ui_update_sensor_charts(lux, temp, hum, pres);

    // 3. Dimmen mit reduzierter Frequenz und Hysterese (gegen Freezing)
    static uint32_t last_br_update = 0;
    static uint8_t last_sent_br = 255;
    
    if (millis() - last_br_update > 500) {
        float target_f;
        if (lux < 25.0f) {
            target_f = 75.0f; 
        } else {
            // Skalierung: 25 Lux -> 75, 500 Lux -> 255
            target_f = 75.0f + ((lux - 25.0f) * (255.0f - 75.0f) / (500.0f - 25.0f));
            if (target_f > 255.0f) target_f = 255.0f;
        }
        
        uint8_t target_br = (uint8_t)target_f;

        if (abs((int)target_br - (int)last_sent_br) > 5) {
            set_amoled_brightness(target_br); 
            last_sent_br = target_br;
            last_br_update = millis();
        }
    }
}
