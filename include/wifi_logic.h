#ifndef WIFI_LOGIC_H
#define WIFI_LOGIC_H

#include <WiFi.h>
#include <Preferences.h>

void init_wifi();
void wifi_loop();
void update_wifi_status_logic();

// Getter-Funktionen für WiFi-Metriken (globale Variablen-Zugriff)
int get_wifi_rssi();
int get_wifi_rssi_pct();
float get_wifi_down_speed();
float get_wifi_up_speed();
int get_wifi_freq();
int get_wifi_channel();

#endif
