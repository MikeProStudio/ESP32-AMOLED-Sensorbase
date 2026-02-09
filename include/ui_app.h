#ifndef UI_APP_H
#define UI_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "src/extra/libs/qrcode/lv_qrcode.h"

void ui_init(void);
void ui_set_sensor_data(float lux, int aqi, int voc, int nox, float temp, float hum, float pres, float gx, float gy, float gz, float pitch, float roll, float radar_dist, bool person_detected);
void ui_update_sensor_charts(float lux, float temp, float hum, float pres);
void ui_update_wifi_info(const char* ssid, int rssi_pct, float down_speed, float up_speed, int freq, int channel, const char* status, const char* ip, const char* uptime, float bat_v, int bat_pct);
void ui_set_crypto_prices(float usd, float eur, float chf);
void ui_update_chart(float *prices, int count, float min_p, float max_p, int tf);
void ui_update_qr_code(const char* ip_address);

// Diese Deklarationen machen die Labels global bekannt
extern lv_obj_t * label_lux;
extern lv_obj_t * label_gyro;
extern lv_obj_t * label_aqi;
extern lv_obj_t * label_voc_nox;
extern lv_obj_t * label_temp;
extern lv_obj_t * label_mmwave;
extern lv_obj_t * label_wifi_info;

#ifdef __cplusplus
}
#endif

#endif
