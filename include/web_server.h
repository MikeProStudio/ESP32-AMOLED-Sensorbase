#ifndef WEB_SERVER_H
#define WEB_SERVER_H

//#include <Arduino.h> // Sicher ist sicher für ESP32 spezifische Typen

/**
 * Initialisiert den AsyncWebServer auf Port 80
 */
void init_webserver();

/**
 * Übergibt die aktuellen Sensorwerte an den Webserver-Buffer
 */
void update_webserver_data(
    float lux, float pitch, float roll,
    int aqi, int voc, int nox,
    float temp, float pres, float hum,
    float bat_v, int bat_pct,
    int rssi, int rssi_pct,
    float down, float up,
    int freq, int channel,
    float radar_dist, bool person_detected
);

/**
 * Sendet Radardaten per WebSocket an alle verbundenen Clients
 */
void broadcast_radar_data(float dist, bool detected);

#endif