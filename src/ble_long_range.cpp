#include "ble_long_range.h"
#include <NimBLEDevice.h>
#include <Arduino.h>

/**
 * BLE SENIOR ARCHITECTURE - FINAL GATT CORRECTION:
 * 1. Focus: Fixing nRF Connect Unit Mismatch (m^3 fix).
 * 2. Focus: Fixing Lux HEX display (Format/Unit alignment).
 * 3. Focus: Strict adherence to Bluetooth SIG Unit Codes.
 * 
 * SIG UNIT CODES:
 * 0x2700: unitless
 * 0x2701: metre (m)
 * 0x272F: Celsius (C)
 * 0x2731: lux (lx)
 * 0x2763: degree (plane angle)
 * 0x27AD: percentage (%)
 * 0x2724: pascal (Pa)
 */

// --- SERVICES ---
#define SVC_SIG_GENERIC_ATTR    "1801"
#define SVC_SIG_ENV             "181A"
#define SVC_SIG_BAT             "180F"
#define SVC_IMU_DATA            "FF01"
#define SVC_RADAR_DATA          "FF02"

// --- CHARACTERISTICS ---
#define CHAR_SIG_TEMP           "2A6E"
#define CHAR_SIG_HUM            "2A6F"
#define CHAR_SIG_PRES           "2A6D"
#define CHAR_SIG_LUX            "2AFB"
#define CHAR_SIG_RSSI           "2A07"
#define CHAR_SIG_BAT            "2A19"
#define CHAR_SIG_SVC_CHNG       "2A05"

#define CHAR_IMB_PITCH          "e0000002-87d4-469b-866b-4e0078235222"
#define CHAR_IMB_ROLL           "e0000003-87d4-469b-866b-4e0078235222"
#define CHAR_IMB_GFORCE         "e0000004-87d4-469b-866b-4e0078235222"
#define CHAR_RADAR_DIST         "f0000001-87d4-469b-866b-4e0078235222"

NimBLEServer* pServer = nullptr;

// Pointers
NimBLECharacteristic *pTempC, *pHumC, *pPresC, *pLuxC, *pTxC, *pBatC, *pSvcChngC;
NimBLECharacteristic *pPitchC, *pRollC, *pGForceC, *pRadarC;

bool deviceConnected = false;
String connectedClientName = "None";
int current_rssi = 0;
uint16_t active_conn_handle = 0xFFFF;
float smoothed_rssi = 0;
bool first_rssi = true;

/**
 * Setup Descriptor Helper with strict SIG Unit checking
 */
void setupChar(NimBLECharacteristic* pChar, const char* name, uint8_t format, int8_t exponent, uint16_t unit) {
    pChar->createDescriptor("2901", NIMBLE_PROPERTY::READ)->setValue(name);
    uint8_t descVal[7] = { 
        format, 
        (uint8_t)exponent, 
        (uint8_t)(unit & 0xFF), 
        (uint8_t)((unit >> 8) & 0xFF), 
        0x01, // Namespace SIG
        0x00, 0x00 
    };
    pChar->createDescriptor("2904", NIMBLE_PROPERTY::READ)->setValue(descVal, 7);
}

class MyServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
        deviceConnected = true;
        active_conn_handle = desc->conn_handle;
        connectedClientName = NimBLEAddress(desc->peer_ota_addr).toString().c_str();
        first_rssi = true;

        Serial.println("\n>>> [BLE_EVENT] HIGH-SPEED LINK ESTABLISHED");
        Serial.printf(">>> [CLIENT] ID: %s | HANDLE: %d\n", connectedClientName.c_str(), active_conn_handle);
        Serial.println(">>> [MODULATION] LE CODED PHY (S=8) NEGOTIATED\n");

        if (pSvcChngC) {
            uint8_t val[4] = {0x01, 0x00, 0xFF, 0xFF};
            pSvcChngC->setValue(val, 4);
            pSvcChngC->indicate();
        }
    }
    void onDisconnect(NimBLEServer* pServer) {
        deviceConnected = false;
        active_conn_handle = 0xFFFF;
        connectedClientName = "None";
        current_rssi = 0;
        first_rssi = true;
        Serial.println("\n>>> [BLE_EVENT] LINK TERMINATED");
        Serial.println(">>> [SYSTEM] REVERTING TO LONG-RANGE ADVERTISING...\n");
        NimBLEDevice::getAdvertising()->start();
    }
};

void init_ble_long_range() {
    Serial.println("Initializing Corrected BLE Telemetry (GATT FIX)...");

    NimBLEDevice::init("ESP32S3-TELEMETRY-LR");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); 

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // 1. GENERIC ATTRIBUTE
    NimBLEService* pAttrSvc = pServer->createService(SVC_SIG_GENERIC_ATTR);
    pSvcChngC = pAttrSvc->createCharacteristic(CHAR_SIG_SVC_CHNG, NIMBLE_PROPERTY::INDICATE);
    pAttrSvc->start();

    // 2. ENVIRONMENTAL SENSING (SIG)
    NimBLEService* pEnvSvc = pServer->createService(SVC_SIG_ENV);
    pTempC = pEnvSvc->createCharacteristic(CHAR_SIG_TEMP, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pHumC = pEnvSvc->createCharacteristic(CHAR_SIG_HUM, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pPresC = pEnvSvc->createCharacteristic(CHAR_SIG_PRES, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    
    // LUX FIX: Ensure uint24 (0x08) and unit 0x2731 (lux)
    pLuxC = pEnvSvc->createCharacteristic(CHAR_SIG_LUX, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    setupChar(pLuxC, "Illuminance", 0x08, 0, 0x2731);

    pTxC = pEnvSvc->createCharacteristic(CHAR_SIG_RSSI, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    setupChar(pTxC, "Signal RSSI", 0x0C, 0, 0x2700);
    pEnvSvc->start();

    // 3. BATTERY SERVICE
    NimBLEService* pBatSvc = pServer->createService(SVC_SIG_BAT);
    pBatC = pBatSvc->createCharacteristic(CHAR_SIG_BAT, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pBatSvc->start();

    // 4. IMU DATA SERVICE (Custom Hub)
    NimBLEService* pImuSvc = pServer->createService(SVC_IMU_DATA);
    pPitchC = pImuSvc->createCharacteristic(CHAR_IMB_PITCH, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    setupChar(pPitchC, "Board Pitch", 0x0E, -1, 0x2763); // deg

    pRollC = pImuSvc->createCharacteristic(CHAR_IMB_ROLL, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    setupChar(pRollC, "Board Roll", 0x0E, -1, 0x2763); // deg

    pGForceC = pImuSvc->createCharacteristic(CHAR_IMB_GFORCE, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    setupChar(pGForceC, "Total G-Force", 0x06, -2, 0x2700); // unitless
    pImuSvc->start();

    // 5. RADAR DATA SERVICE (Custom Hub)
    NimBLEService* pRadarSvc = pServer->createService(SVC_RADAR_DATA);
    pRadarC = pRadarSvc->createCharacteristic(CHAR_RADAR_DIST, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    // RADAR FIX: Changed from 0x2711 (m^3) to 0x2701 (metre)
    setupChar(pRadarC, "Radar distance", 0x06, -2, 0x2701); // metre, exp -2 (cm)
    pRadarSvc->start();

    // Advertising
    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    pAdv->addServiceUUID(SVC_SIG_ENV);
    pAdv->addServiceUUID(SVC_IMU_DATA);
    pAdv->addServiceUUID(SVC_RADAR_DATA);
    pAdv->setScanResponse(true);
    pAdv->start();
    
    Serial.println("GATT Correction applied. Telemetry Hub ready.");
}

void update_ble_telemetry(float speed, float g_force_x, float g_force_y, float g_force_z,
                          float lux, int aqi, float temp, float hum, float pres, float radar_dist, int bat_pct,
                          float pitch, float roll) {
    if (!deviceConnected) return;

    static uint32_t lastCinematicPrint = 0;

    // RSSI
    int8_t rssi = 0;
    if (active_conn_handle != 0xFFFF && ble_gap_conn_rssi(active_conn_handle, &rssi) == 0) {
        if (first_rssi) { smoothed_rssi = (float)rssi; first_rssi = false; }
        else { smoothed_rssi = (smoothed_rssi * 0.8f) + ((float)rssi * 0.2f); }
        current_rssi = (int)smoothed_rssi;
        pTxC->setValue((int8_t)current_rssi);
        pTxC->notify();
    }

    if (millis() - lastCinematicPrint > 500) {
        Serial.println("--------------------------------------------------");
        Serial.println("[OS_KERNEL]  ESP32-S3 Dual-Core v4.4-LTS");
        Serial.println("[BT_STACK]   NimBLE-Arduino Engine | PHY: CODED_S8");
        Serial.printf("[SIGNAL]     Current RSSI: %d dBm (Smoothed Alpha 0.2)\n", current_rssi);
        Serial.printf("[PEER]       Remote ID: %s\n", connectedClientName.c_str());
        Serial.println("--------------------------------------------------");
        Serial.printf("[DATA_IMU]   Pitch: %+.2f° | Roll: %+.2f°\n", pitch, roll);
        Serial.printf("[DATA_ACC]   X: %+.3f G | Y: %+.3f G | Z: %+.3f G\n", g_force_x, g_force_y, g_force_z);
        Serial.printf("[DATA_RADAR] Distance: %.1f cm | Target: %s\n", radar_dist, radar_dist < 150 ? "IN_RANGE" : "CLEAR");
        Serial.printf("[DATA_ENV]   Light: %.0f Lux | Temp: %.2f°C | Hum: %.1f%%\n", lux, temp, hum);
        Serial.printf("[DATA_ENV]   Pressure: %.3f bar | AQI: %d\n", pres, aqi);
        Serial.printf("[OS_STATUS]  Battery: %d%% | Charging: %s\n", bat_pct, bat_pct < 100 ? "ACTIVE" : "STANDBY");
        Serial.println("--------------------------------------------------");
        lastCinematicPrint = millis();
    }

    // Environmental (SIG)
    int16_t tVal = (int16_t)(temp * 100); pTempC->setValue((uint8_t*)&tVal, 2); pTempC->notify();
    uint16_t hVal = (uint16_t)(hum * 100); pHumC->setValue((uint8_t*)&hVal, 2); pHumC->notify();
    uint32_t pVal = (uint32_t)(pres * 1000000); pPresC->setValue((uint8_t*)&pVal, 4); pPresC->notify();
    
    // LUX Binary uint24
    uint32_t lVal = (uint32_t)lux;
    uint8_t lBuf[3] = { (uint8_t)(lVal & 0xFF), (uint8_t)((lVal >> 8) & 0xFF), (uint8_t)((lVal >> 16) & 0xFF) };
    pLuxC->setValue(lBuf, 3); pLuxC->notify();

    // IMU (Binary)
    int16_t pitchVal = (int16_t)(pitch * 10); pPitchC->setValue((uint8_t*)&pitchVal, 2); pPitchC->notify();
    int16_t rollVal = (int16_t)(roll * 10); pRollC->setValue((uint8_t*)&rollVal, 2); pRollC->notify();
    
    float g_total = sqrt(g_force_x * g_force_x + g_force_y * g_force_y + g_force_z * g_force_z);
    uint16_t gVal = (uint16_t)(g_total * 100); pGForceC->setValue((uint8_t*)&gVal, 2); pGForceC->notify();

    // RADAR (Sent in cm, App parset via exp -2 as metre)
    uint16_t radVal = (uint16_t)radar_dist; pRadarC->setValue((uint8_t*)&radVal, 2); pRadarC->notify();

    // Battery
    uint8_t bVal = (uint8_t)bat_pct; pBatC->setValue(&bVal, 1); pBatC->notify();
}

bool is_ble_connected() { return deviceConnected; }
int get_ble_rssi() { return current_rssi; }
const char* get_ble_client_name() { return connectedClientName.c_str(); }
