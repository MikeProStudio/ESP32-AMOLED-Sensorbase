#include "web_server.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "web_ui_updater.h"
#include "ble_long_range.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

static float _lux = 0, _pitch = 0, _roll = 0, _temp = 0, _pres = 0, _hum = 0, _bat_v = 0, _wifi_down = 0, _wifi_up = 0;
static int _aqi = 0, _voc = 0, _nox = 0, _bat_pct = 0, _rssi = 0, _rssi_pct = 0, _freq = 0, _chan = 0;
static float _radar_dist = 0;
static bool _person_detected = false;

void update_webserver_data(float lux, float pitch, float roll, int aqi, int voc, int nox, float temp, float pres, float hum, float bat_v, int bat_pct, int rssi, int rssi_pct, float down, float up, int freq, int channel, float radar_dist, bool person_detected) {
    _lux = lux; _pitch = pitch; _roll = roll;
    _aqi = aqi; _voc = voc; _nox = nox;
    _temp = temp; _pres = pres; _hum = hum;
    _bat_v = bat_v; _bat_pct = bat_pct;
    _rssi = rssi; _rssi_pct = rssi_pct;
    _wifi_down = down; _wifi_up = up;
    _freq = freq; _chan = channel;
    _radar_dist = radar_dist;
    _person_detected = person_detected;
}

void broadcast_radar_data(float dist, bool detected) {
    if (ws.count() > 0) {
        JsonDocument doc;
        doc["type"] = "radar";
        doc["dist"] = dist;
        doc["person"] = detected;
        String json;
        serializeJson(doc, json);
        ws.textAll(json);
    }
}

void init_webserver() {
    // LittleFS mounten
    if (!LittleFS.begin(true)) {
        Serial.println("[WebServer] Error mounting LittleFS");
    } else {
        Serial.println("[WebServer] LittleFS mounted");
    }

    // WebSocket Handler
    server.addHandler(&ws);

    // Statische Dateien servieren (HTML von LittleFS)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        if (LittleFS.exists("/www/index.html.gz")) {
            AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/www/index.html.gz", "text/html");
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        } else if (LittleFS.exists("/www/index.html")) {
            request->send(LittleFS, "/www/index.html", "text/html");
        } else {
            request->send(404, "text/plain", "UI Files missing. Please run update.");
        }
    });

    // Sensor Daten API
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
        JsonDocument doc;
        doc["lux"] = _lux; doc["pitch"] = _pitch; doc["roll"] = _roll;
        doc["aqi"] = _aqi; doc["voc"] = _voc; doc["nox"] = _nox;
        doc["temp"] = _temp; doc["pres"] = _pres; doc["hum"] = _hum;
        doc["bat_v"] = _bat_v; doc["bat_pct"] = _bat_pct;
        doc["rssi"] = _rssi; doc["rssi_pct"] = _rssi_pct;
        doc["down"] = _wifi_down; doc["up"] = _wifi_up;
        doc["freq"] = _freq; doc["chan"] = _chan;
        doc["radar_dist"] = _radar_dist;
        doc["person"] = _person_detected;
        
        // Bluetooth Daten hinzufügen
        doc["ble_conn"] = is_ble_connected();
        doc["ble_rssi"] = get_ble_rssi();
        doc["ble_client"] = get_ble_client_name();
        doc["ble_mode"] = "LE Coded PHY (S=8)";

        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });

    // Manueller Update Trigger über API
    server.on("/update_ui", HTTP_GET, [](AsyncWebServerRequest *request){
        WebUIUpdater::checkForUpdates();
        request->send(200, "text/plain", "Update process started. Check Serial Monitor.");
    });

    server.begin();
    Serial.println("[WebServer] Started on Port 80");
}
