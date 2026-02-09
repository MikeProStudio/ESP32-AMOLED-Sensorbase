#include "web_ui_updater.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// URLs zu deinem GitHub Repo (Anpassen an deine Struktur)
const char* WebUIUpdater::GITHUB_VERSION_URL = "https://raw.githubusercontent.com/MikeProStudio/HW_ESP32_AMOLED-1.91/main/web/version.json";
const char* WebUIUpdater::GITHUB_ASSET_URL = "https://raw.githubusercontent.com/MikeProStudio/HW_ESP32_AMOLED-1.91/main/data/www/index.html.gz";

void WebUIUpdater::checkForUpdates() {
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.println("[Updater] Checking for UI updates...");
    
    WiFiClientSecure client;
    client.setInsecure(); // Für GitHub raw content meist ausreichend, falls Root-CA nicht hinterlegt

    HTTPClient http;
    if (http.begin(client, GITHUB_VERSION_URL)) {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            JsonDocument doc;
            deserializeJson(doc, payload);
            
            int remoteVersion = doc["version"] | 0;
            // Hier würde der Vergleich mit der lokalen version.json stattfinden
            Serial.printf("[Updater] Remote Version: %d\n", remoteVersion);
            
            // Trigger update if needed
            // performUpdate(GITHUB_ASSET_URL);
        }
        http.end();
    }
}

bool WebUIUpdater::performUpdate(const String& url) {
    WiFiClientSecure client;
    client.setInsecure();
    
    HTTPClient http;
    http.begin(client, url);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        File f = LittleFS.open("/www/index.html.gz.tmp", "w");
        if (f) {
            http.writeToStream(&f);
            f.close();
            
            // Atomic Swap
            LittleFS.remove("/www/index.html.gz");
            LittleFS.rename("/www/index.html.gz.tmp", "/www/index.html.gz");
            Serial.println("[Updater] Update successful");
            return true;
        }
    }
    return false;
}
