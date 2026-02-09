#include "sd_logger.h"
#include <SD.h>
#include <FS.h>
#include <time.h>

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600; // Germany/Zurich (GMT+1)
const int   daylightOffset_sec = 3600; // DST

void init_logger() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("[Logger] Time sync initialized.");
}

void log_data_to_sd(float lux, float pitch, float roll, int aqi, int voc, int nox, float temp, float pres, float hum, float down, float up, float bat_v, int bat_pct) {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("[Logger] Failed to obtain time");
        return;
    }

    char fileName[32];
    strftime(fileName, sizeof(fileName), "/sd_card/%Y-%m-%d.csv", &timeinfo);
    
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

    bool fileExists = SD.exists(fileName);
    File file = SD.open(fileName, FILE_APPEND);
    
    if(!file) {
        // Fallback: Versuche ohne /sd_card Prefix falls Mountpunkt anders ist
        strftime(fileName, sizeof(fileName), "/%Y-%m-%d.csv", &timeinfo);
        file = SD.open(fileName, FILE_APPEND);
    }

    if(!file) {
        Serial.println("[Logger] Failed to open file for logging");
        return;
    }

    // Header schreiben wenn Datei neu
    if(!fileExists || file.size() == 0) {
        file.println("Time;Lux;Pitch;Roll;AQI;VOC;NOx;Temp;Pres;Hum;WiFiDown;WiFiUp;BatV;BatPct");
    }

    // Daten zeile schreiben
    file.printf("%s;%.1f;%.1f;%.1f;%d;%d;%d;%.1f;%.2f;%.0f;%.1f;%.1f;%.2f;%d\n",
                timeStr, lux, pitch, roll, aqi, voc, nox, temp, pres, hum, down, up, bat_v, bat_pct);
    
    file.close();
    Serial.printf("[Logger] Data appended to %s\n", fileName);
}
