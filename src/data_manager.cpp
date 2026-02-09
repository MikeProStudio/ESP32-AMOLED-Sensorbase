#include "data_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>
#include <SensirionI2CSgp41.h>
#include <VOCGasIndexAlgorithm.h>
#include <NOxGasIndexAlgorithm.h>
#include "QMI8658.h"
#include "s3km1110.h"
#include "ui_app.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t wireMutex;

// Sensor Objekte
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp(&Wire1);
BH1750 lightMeter;
SensirionI2CSgp41 sgp41;
VOCGasIndexAlgorithm voc_algorithm;
NOxGasIndexAlgorithm nox_algorithm;
QMI8658 qmi;
s3km1110 radar;

float temperature_AHT20 = 0, humidity = 0;
float temperature_BMP280 = 0, pressure = 0;

bool person_in_range = false;
float radar_distance = 0;
bool person_detected = false;
static bool last_person_in_range = false; // bleibt lokal in dieser Datei

void get_sensor_readings(float *lux, int *aqi, int *voc, int *nox, float *temp, float *hum, float *pres, float *gx, float *gy, float *gz, float *pitch, float *roll) {
    static bool sensors_initialized = false;
    static bool bh1750_found = false;      
    static bool qmi_found = false;
    static bool sgp41_conditioned = false;
    static float f_ax = 0, f_ay = 0, f_az = 0; 
    static float last_p = 0, last_r = 0;
    const float alpha = 0.075f;
    static uint32_t last_aqi_update = 0;
    static uint32_t last_qmi_update = 0;
    static uint32_t start_time = 0;

    if (!sensors_initialized) {
        if(lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire1)) bh1750_found = true;
        aht.begin(&Wire1);
        bmp.begin(0x77);
        sgp41.begin(Wire1);
        uint16_t serial[3];
        if (sgp41.getSerialNumber(serial) == 0) Serial.printf("[SGP41] Found SN: %04X%04X%04X\n", serial[0], serial[1], serial[2]);
        if(qmi.begin(Wire, 0x6A)) { qmi.enableGyro(true); qmi_found = true; }
        start_time = millis();
        sensors_initialized = true;
    }

    if (bh1750_found) {
        float l = lightMeter.readLightLevel();
        if (l >= 0) *lux = l;
    }

    sensors_event_t hum_event, temp_event;
    if (aht.getEvent(&hum_event, &temp_event)) {
        temperature_AHT20 = temp_event.temperature;
        humidity = hum_event.relative_humidity;
        *hum = humidity;
        *temp = temperature_AHT20;
    }
    
    float t_bmp = bmp.readTemperature();
    pressure = bmp.readPressure();
    *pres = pressure / 100000.0f;

    if (qmi_found && (millis() - last_qmi_update > 50)) {
        float ax, ay, az;
        float lgx = 0, lgy = 0, lgz = 0;
        bool success = false;

        if (xSemaphoreTake(wireMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (qmi.readAccelMPS2(ax, ay, az)) {
                f_ax = (alpha * ax) + ((1.0f - alpha) * f_ax);
                f_ay = (alpha * ay) + ((1.0f - alpha) * f_ay);
                f_az = (alpha * az) + ((1.0f - alpha) * f_az);
                last_p = atan2(f_ay, f_az) * 180.0f / M_PI;
                last_r = atan2(-f_ax, sqrt(f_ay * f_ay + f_az * f_az)) * 180.0f / M_PI;
                success = true;
            }
            qmi.readGyro(lgx, lgy, lgz);
            xSemaphoreGive(wireMutex);
        }

        if (success) {
            *gx = lgx; *gy = lgy; *gz = lgz;
        }
        last_qmi_update = millis();
    }
    *pitch = last_p; *roll = last_r;

    static int last_valid_aqi = -1; 
    static int last_v = 0, last_n = 0;
    if (millis() - last_aqi_update >= 1000) {
        uint16_t sraw_voc = 0, sraw_nox = 0, error = 0;
        uint16_t rh_ticks = (humidity > 1.0f) ? (uint16_t)(humidity * 655.35f) : 0x8000;
        uint16_t t_ticks = (*temp > -10.0f) ? (uint16_t)((*temp + 45.0f) * 65535.0f / 175.0f) : 0x6666;

        if (!sgp41_conditioned) {
            error = sgp41.executeConditioning(rh_ticks, t_ticks, sraw_voc);
            if (millis() - start_time > 10000) sgp41_conditioned = true;
            last_valid_aqi = 0; 
        } else {
            error = sgp41.measureRawSignals(rh_ticks, t_ticks, sraw_voc, sraw_nox);
            if (error == 0) {
                last_v = (int)voc_algorithm.process(sraw_voc);
                last_n = (int)nox_algorithm.process(sraw_nox);
                last_valid_aqi = last_v;
            }
        }
        last_aqi_update = millis();
    }
    *aqi = last_valid_aqi; *voc = last_v; *nox = last_n;
}


float get_live_btc_price() {
    if(WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("https://api.api-ninjas.com/v1/cryptoprice?symbol=BTCUSD");
        http.addHeader("X-Api-Key", "j4QcSb0ZT0FMJTVwwiiajWizLs0JOBO2yLHrZDsg"); 
        int httpCode = http.GET();
        if(httpCode > 0) {
            JsonDocument doc; 
            if (!deserializeJson(doc, http.getString())) {
                float val = doc["price"].as<float>();
                http.end(); return val;
            }
        }
        http.end();
    }
    return 0;
}

void LED_Alarm(bool state) {
  static uint32_t startMillis = 0;   // Wann hat der Alarm begonnen?
  static uint32_t lastToggle = 0;    // Wann hat die LED das letzte Mal gewechselt?
  static bool ledZustand = false;    // Aktueller Status der LED
  
  const uint32_t intervall = 50;     // Geschwindigkeit: 50ms (kleiner = schneller)
  const uint32_t gesamtDauer = 3000; // 3 Sekunden

  if (!state) {
    startMillis = 0;
    ledZustand = false;
    digitalWrite(4, LOW);
    return;
  }

  // Startzeitpunkt festlegen
  if (startMillis == 0) startMillis = millis();

  // Prüfen, ob die 3 Sekunden noch nicht um sind
  if (millis() - startMillis < gesamtDauer) {
    
    // Prüfen, ob es Zeit für den nächsten Blink-Wechsel ist
    if (millis() - lastToggle >= intervall) {
      lastToggle = millis();       // Zeit merken
      ledZustand = !ledZustand;    // Zustand umkehren (true/false)
      digitalWrite(4, ledZustand); // LED schalten
    }
    
  } else {
    digitalWrite(4, LOW); // Zeit abgelaufen
  }
}


void radar_task(void* p) {
    Serial2.begin(115200, SERIAL_8N1, 11, 12);
    radar.begin(Serial2, Serial);
    float mein_threshold = 150.0; //
    
    while(1) {
        if (radar.read()) {
            person_detected = radar.isTargetDetected;
            radar_distance = (float)radar.distanceToTarget;

            if (person_detected && radar_distance > 0 && radar_distance <= mein_threshold) {
                person_in_range = true;
            } else {
                person_in_range = false;
            }

            // High-Frequency Update via WebSocket (Core 0 -> WebSocket Clients)
            static uint32_t last_ws_push = 0;
            if (millis() - last_ws_push >= 250) {
                extern void broadcast_radar_data(float dist, bool detected);
                broadcast_radar_data(radar_distance, person_in_range);
                last_ws_push = millis();
            }

            if (person_in_range != last_person_in_range) {
                last_person_in_range = person_in_range; 
                Serial.println(">>> RADAR ZUSTAND GEÄNDERT <<<");
                Serial.printf("Status: %s | Distanz: %.1f cm\n", 
                              person_in_range ? "BETRETEN" : "VERLASSEN", 
                              radar_distance);
                
            }
        }
        LED_Alarm(person_in_range);
        vTaskDelay(pdMS_TO_TICKS(20)); 
    }
}

