#include "ble_long_range.h"
#include <NimBLEDevice.h>
#include <Arduino.h>

// Service und Characteristic UUIDs
#define SERVICE_UUID           "181A" 
#define SPEED_CHAR_UUID        "2A67" 
#define GFORCE_CHAR_UUID       "2A5A" 
#define LUX_CHAR_UUID          "2A77" 
#define AQI_CHAR_UUID          "2BED" 
#define TEMP_HUM_PRES_CHAR_UUID "2A1C" 
#define RADAR_CHAR_UUID        "2A58" 

NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pSpeedChar = nullptr;
NimBLECharacteristic* pGForceChar = nullptr;
NimBLECharacteristic* pLuxChar = nullptr;
NimBLECharacteristic* pAQIChar = nullptr;
NimBLECharacteristic* pTempHumPresChar = nullptr;
NimBLECharacteristic* pRadarChar = nullptr;

bool deviceConnected = false;
String connectedClientName = "None";
int current_rssi = 0;
uint16_t active_conn_handle = 0xFFFF;
float smoothed_rssi = 0;
bool first_rssi = true;

class MyServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
        deviceConnected = true;
        active_conn_handle = desc->conn_handle;
        connectedClientName = NimBLEAddress(desc->peer_ota_addr).toString().c_str();
        first_rssi = true;
        Serial.printf("BLE Client connected: %s\n", connectedClientName.c_str());
    }

    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
        active_conn_handle = 0xFFFF;
        connectedClientName = "None";
        current_rssi = 0;
        first_rssi = true;
        Serial.println("BLE Client disconnected");
        NimBLEDevice::getAdvertising()->start();
    }
};

void init_ble_long_range() {
    Serial.println("Initializing NimBLE (Full Sensor Suite)...");

    NimBLEDevice::init("S3-AMOLED-LR");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); 

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    NimBLEService* pService = pServer->createService(SERVICE_UUID);

    pSpeedChar = pService->createCharacteristic(SPEED_CHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pGForceChar = pService->createCharacteristic(GFORCE_CHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pLuxChar = pService->createCharacteristic(LUX_CHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pAQIChar = pService->createCharacteristic(AQI_CHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pTempHumPresChar = pService->createCharacteristic(TEMP_HUM_PRES_CHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pRadarChar = pService->createCharacteristic(RADAR_CHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

    pService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    
    if (pAdvertising->start()) {
        Serial.println("BLE Advertising started.");
    }
}

void update_ble_telemetry(float speed, float g_force_x, float g_force_y, float g_force_z, 
                          float lux, int aqi, float temp, float hum, float pres, float radar_dist) {
    if (!deviceConnected) return;

    // RSSI FIX & Glättung
    if (active_conn_handle != 0xFFFF) {
        int8_t rssi = 0;
        if (ble_gap_conn_rssi(active_conn_handle, &rssi) == 0) {
            if (first_rssi) {
                smoothed_rssi = (float)rssi;
                first_rssi = false;
            } else {
                smoothed_rssi = (smoothed_rssi * 0.8f) + ((float)rssi * 0.2f);
            }
            current_rssi = (int)smoothed_rssi;
        }
    }

    // Telemetrie-Daten updaten
    uint16_t s = (uint16_t)(speed * 100); 
    pSpeedChar->setValue((uint8_t*)&s, 2);
    pSpeedChar->notify();

    float g_total = sqrt(g_force_x * g_force_x + g_force_y * g_force_y + g_force_z * g_force_z);
    pGForceChar->setValue((uint8_t*)&g_total, 4);
    pGForceChar->notify();

    uint16_t l = (uint16_t)lux;
    pLuxChar->setValue((uint8_t*)&l, 2);
    pLuxChar->notify();

    int16_t a = (int16_t)aqi;
    pAQIChar->setValue((uint8_t*)&a, 2);
    pAQIChar->notify();

    pRadarChar->setValue((uint8_t*)&radar_dist, 4);
    pRadarChar->notify();

    pTempHumPresChar->setValue((uint8_t*)&temp, 4);
    pTempHumPresChar->notify();
}

bool is_ble_connected() {
    return deviceConnected;
}

int get_ble_rssi() {
    return current_rssi;
}

const char* get_ble_client_name() {
    return connectedClientName.c_str();
}
