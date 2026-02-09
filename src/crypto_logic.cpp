#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "crypto_logic.h"
#include "ui_app.h"

// --- FUNKTION 1: Aktuelle Preise (USD, EUR, CHF) ---
void update_btc_price() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Crypto] WiFi not connected, skipping price update.");
        return;
    }

    HTTPClient http;
    http.setConnectTimeout(5000); // 5s Timeout
    // Wir nutzen CoinGecko für stabilere API-Abrufe (Free Tier)
    http.begin("https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd,eur,chf");
    
    int httpCode = http.GET();
    if (httpCode == 200) {
        String payload = http.getString();
        JsonDocument doc; 
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            float usd = doc["bitcoin"]["usd"];
            float eur = doc["bitcoin"]["eur"];
            float chf = doc["bitcoin"]["chf"];
            
            // Nur updaten wenn wir gültige Daten haben (nicht 0)
            if (usd > 0) {
                ui_set_crypto_prices(usd, eur, chf);
                Serial.printf("[Crypto] CoinGecko: $%.2f | €%.2f | CHF%.2f\n", usd, eur, chf);
            } else {
                Serial.println("[Crypto] Received 0 or invalid values from CoinGecko.");
            }
        } else {
            Serial.printf("[Crypto] JSON Parse Error: %s\n", error.c_str());
            Serial.println("[Crypto] Payload was: " + payload);
        }
    } else {
        Serial.printf("[Crypto] API Error: %d\n", httpCode);
        if (httpCode < 0) {
             Serial.printf("[Crypto] HTTP Connection error: %s\n", http.errorToString(httpCode).c_str());
        }
    }
    http.end();
}

// --- FUNKTION 2: Historische Daten für das Chart (1h bis 5y) ---
void update_btc_chart(int timeframe) {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    String url;
    
    switch(timeframe) {
        case 0: // 1h
            url = "https://min-api.cryptocompare.com/data/v2/histominute?fsym=BTC&tsym=USD&limit=60";
            break;
        case 1: // 24h
            url = "https://min-api.cryptocompare.com/data/v2/histohour?fsym=BTC&tsym=USD&limit=24";
            break;
        case 2: // 7d
            url = "https://min-api.cryptocompare.com/data/v2/histoday?fsym=BTC&tsym=USD&limit=7";
            break;
        case 3: // 1m
            url = "https://min-api.cryptocompare.com/data/v2/histoday?fsym=BTC&tsym=USD&limit=30";
            break;
        case 4: // 1y
            url = "https://min-api.cryptocompare.com/data/v2/histoday?fsym=BTC&tsym=USD&limit=365";
            break;
        case 5: // 5y (KORREKTUR: histoday mit aggregate)
            url = "https://min-api.cryptocompare.com/data/v2/histoday?fsym=BTC&tsym=USD&limit=60&aggregate=30";
            break;
        default: return;
    }

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == 200) {
        // ArduinoJson V7: JsonDocument passt sich automatisch an
        JsonDocument doc; 
        DeserializationError error = deserializeJson(doc, http.getString());
        
        if (!error) {
            JsonArray data = doc["Data"]["Data"];
            float min_p = 2000000.0, max_p = 0.0;
            static float prices[370]; 
            int count = 0;

            for (JsonObject obj : data) {
                float p = obj["close"];
                if (p <= 0) continue; 
                prices[count] = p;
                if (p < min_p) min_p = p;
                if (p > max_p) max_p = p;
                count++;
                if (count >= 370) break;
            }
            ui_update_chart(prices, count, min_p, max_p, timeframe);
        }
    } else {
        // FALLBACK: Wenn API Fehler, UI wieder aufwecken
        Serial.printf("API Error: %d\n", httpCode);
        // Damit wir nicht bei "FETCHING" hängen bleiben:
        update_btc_price(); // Einfach nur Preis updaten um UI zu triggern
    }
    http.end();
}