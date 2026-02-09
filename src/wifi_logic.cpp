#include "wifi_logic.h"
#include "ui_app.h"
#include <Arduino.h>
#include "esp_wifi.h"
#ifdef __has_include
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#endif

static uint32_t connection_start_ms = 0;
static bool is_smart_config_active = false;
static Preferences preferences;

// Globale Variablen für Metriken (statisch im Modul)
static int g_rssi = 0;
static int g_rssi_pct = 0;
static float g_down_speed = 0;
static float g_up_speed = 0;
static int g_freq = 0;
static int g_channel = 0;

void init_wifi() {
    preferences.begin("wifi", false);
    String ssid = preferences.getString("ssid", "");
    String pass = preferences.getString("pass", "");
    preferences.end();

    WiFi.mode(WIFI_AP_STA);

    if (ssid == "" || ssid == "N/A") {
        Serial.println("No NVS credentials found.");
#ifdef WIFI_SSID
        Serial.println("Trying secrets.h fallback...");
        ssid = WIFI_SSID;
        pass = WIFI_PASS;
#else
        Serial.println("No secrets.h fallback available.");
        ssid = "N/A";
        pass = "";
#endif
    }

    Serial.printf("Connecting to WiFi: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED && counter < 20) {
        delay(500);
        Serial.print(".");
        counter++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nCONNECTED!");
        connection_start_ms = millis();
    } else {
        Serial.println("\nFAILED (Timeout). Starting SmartConfig...");
        is_smart_config_active = true;
        WiFi.beginSmartConfig();

        while (is_smart_config_active) {
            delay(500);
            Serial.print("#");
            if (WiFi.smartConfigDone()) {
                Serial.println("\nSmartConfig Done!");
                is_smart_config_active = false;
                
                // Neue Credentials speichern
                preferences.begin("wifi", false);
                preferences.putString("ssid", WiFi.SSID());
                preferences.putString("pass", WiFi.psk());
                preferences.end();
                
                connection_start_ms = millis();
                break;
            }
        }
    }
}

void update_wifi_status_logic() {
    static uint32_t last_update = 0;
    if (millis() - last_update < 1000) return;
    last_update = millis();

    char ssid[33];
    const char* status = "Disconnected";
    char ip_str[20] = "0.0.0.0";
    char uptime_str[16] = "00:00:00";

    if (WiFi.status() == WL_CONNECTED) {
        status = "Connected";
        strncpy(ssid, WiFi.SSID().c_str(), 32);
        ssid[32] = '\0';
        
        strncpy(ip_str, WiFi.localIP().toString().c_str(), 19);
        ip_str[19] = '\0';

        g_rssi = WiFi.RSSI();
        if (g_rssi <= -100) g_rssi_pct = 0;
        else if (g_rssi >= -50) g_rssi_pct = 100;
        else g_rssi_pct = 2 * (g_rssi + 100);

        wifi_config_t config;
        esp_wifi_get_config(WIFI_IF_STA, &config);
        
        g_down_speed = (float)g_rssi_pct * 0.72f; 
        g_up_speed = g_down_speed * 0.8f;

        g_channel = WiFi.channel();
        g_freq = (g_channel <= 14) ? 2400 : 5000;

        uint32_t diff = (millis() - connection_start_ms) / 1000;
        int h = diff / 3600;
        int m = (diff % 3600) / 60;
        int s = diff % 60;
        snprintf(uptime_str, sizeof(uptime_str), "%02d:%02d:%02d", h, m, s);
    } else if (is_smart_config_active) {
        status = "SmartConfig Mode";
        strcpy(ssid, "Waiting...");
    } else {
        status = "Connecting...";
        strcpy(ssid, "N/A");
    }

    // Battery Measurement (GPIO 1)
    static int battery_pin = 1;
    analogReadResolution(12); // 4096 levels
    int raw_adc = analogRead(battery_pin);
    float avg_mv = (float)raw_adc * (3300.0f / 4095.0f); // Convert to mV
    float voltage = (avg_mv * 2.0f) / 1000.0f; // Voltage Divider correction
    
    int bat_pct = (int)((voltage - 3.2f) * 100.0f / (3.7f - 3.2f));
    if (bat_pct > 100) bat_pct = 100;
    if (bat_pct < 0) bat_pct = 0;

    ui_update_wifi_info(ssid, g_rssi_pct, g_down_speed, g_up_speed, g_freq, g_channel, status, ip_str, uptime_str, voltage, bat_pct);
    ui_update_qr_code(ip_str);
}

// Implementierung der Getter
int get_wifi_rssi() { return g_rssi; }
int get_wifi_rssi_pct() { return g_rssi_pct; }
float get_wifi_down_speed() { return g_down_speed; }
float get_wifi_up_speed() { return g_up_speed; }
int get_wifi_freq() { return g_freq; }
int get_wifi_channel() { return g_channel; }
