#ifndef BLE_LONG_RANGE_H
#define BLE_LONG_RANGE_H

#include <NimBLEDevice.h>

void init_ble_long_range();
void update_ble_telemetry(float speed, float g_force_x, float g_force_y, float g_force_z,
                          float lux, int aqi, float temp, float hum, float pres, float radar_dist, int bat_pct,
                          float pitch, float roll);

bool is_ble_connected();
int get_ble_rssi();
const char* get_ble_client_name();

#endif // BLE_LONG_RANGE_H
