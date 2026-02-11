#include "ui_app.h"
#include <stdio.h>
#include <Arduino.h>
#include "ble_long_range.h"

static lv_obj_t * label_btc_usd = NULL;
static lv_obj_t * label_btc_eur = NULL;
static lv_obj_t * label_btc_chf = NULL;
static lv_obj_t * label_chart_info = NULL;
static lv_obj_t * label_chart_status = NULL; 
lv_obj_t * label_wifi_info = NULL;
static lv_obj_t * qr_obj = NULL;

lv_obj_t * label_lux = NULL;
lv_obj_t * label_gyro = NULL;
lv_obj_t * label_aqi = NULL;
lv_obj_t * label_voc_nox = NULL;
lv_obj_t * label_temp = NULL;
lv_obj_t * label_mmwave = NULL;
static lv_obj_t * chart = NULL;
static lv_chart_series_t * ser_temp = NULL; 

extern int request_timeframe_update; 
static int current_tf_internal = 0;
static uint32_t last_click_time = 0;

static lv_obj_t * tab_sensors = NULL;
static lv_obj_t * tab_sensor_charts = NULL;
static lv_obj_t * tab_btc_chart = NULL;
static lv_obj_t * tab_btc_prices = NULL;
static lv_obj_t * tab_info = NULL;
static lv_obj_t * tab_qr = NULL;

static lv_obj_t * sensor_chart_lux = NULL;
static lv_obj_t * sensor_chart_env = NULL;
static lv_chart_series_t * ser_lux = NULL;
static lv_chart_series_t * ser_env_temp = NULL;
static lv_chart_series_t * ser_env_hum = NULL;
static lv_chart_series_t * ser_env_pres = NULL;

static lv_obj_t* create_card(lv_obj_t* parent, const char* title, lv_color_t border_color) {
    lv_obj_t * card = lv_obj_create(parent);
    lv_obj_set_size(card, 122, 165); 
    lv_obj_set_style_bg_color(card, lv_color_hex(0x101010), 0);
    lv_obj_set_style_border_color(card, border_color, 0);
    lv_obj_set_style_border_width(card, 3, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 4, 0);
    lv_obj_set_style_pad_top(card, 8, 0);
    lv_obj_set_style_pad_gap(card, 8, 0);

    lv_obj_t * t = lv_label_create(card);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_font(t, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(t, lv_color_hex(0xAAAAAA), 0);
    return card;
}

static void chart_event_cb(lv_event_t * e) {
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if(millis() - last_click_time < 500) return;
        last_click_time = millis();
        current_tf_internal++;
        if(current_tf_internal > 5) current_tf_internal = 0; 
        request_timeframe_update = current_tf_internal;
        if(chart && ser_temp) {
            lv_chart_set_all_value(chart, ser_temp, LV_CHART_POINT_NONE);
            lv_obj_set_style_opa(chart, 150, 0); 
            lv_chart_refresh(chart); 
        }
        if(label_chart_status) lv_label_set_text(label_chart_status, "FETCHING...");
    }
}

static void create_sensor_tab(lv_obj_t * tab) {
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x000000), 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(tab, 10, 0);

    lv_obj_t * card1 = create_card(tab, "LIGHT", lv_color_hex(0xFFFF00));
    label_lux = lv_label_create(card1);
    lv_label_set_text(label_lux, "-- Lx");
    lv_obj_set_style_text_font(label_lux, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label_lux, lv_color_white(), 0);

    label_gyro = lv_label_create(card1);
    lv_label_set_long_mode(label_gyro, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label_gyro, 115);
    lv_obj_set_style_text_align(label_gyro, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label_gyro, "P: 0.0°\nR: 0.0°");
    lv_obj_set_style_text_font(label_gyro, &lv_font_montserrat_24, 0); 
    lv_obj_set_style_text_color(label_gyro, lv_palette_main(LV_PALETTE_ORANGE), 0);

    lv_obj_t * card2 = create_card(tab, "AIR", lv_color_hex(0x00FF00));
    label_aqi = lv_label_create(card2);
    lv_label_set_text(label_aqi, "--");
    lv_obj_set_style_text_font(label_aqi, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label_aqi, lv_color_white(), 0);

    label_voc_nox = lv_label_create(card2);
    lv_label_set_long_mode(label_voc_nox, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label_voc_nox, 115);
    lv_obj_set_style_text_align(label_voc_nox, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label_voc_nox, "VOC: 0\nNOx: 0");
    lv_obj_set_style_text_font(label_voc_nox, &lv_font_montserrat_28, 0); 
    lv_obj_set_style_text_color(label_voc_nox, lv_color_white(), 0);

    lv_obj_t * card3 = create_card(tab, "ENV", lv_color_hex(0xFF0000));
    label_temp = lv_label_create(card3);
    lv_label_set_long_mode(label_temp, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label_temp, 115);
    lv_obj_set_style_text_align(label_temp, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label_temp, "0.0°C\n0.0bar\n0%");
    lv_obj_set_style_text_font(label_temp, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label_temp, lv_color_white(), 0);

    lv_obj_t * card4 = create_card(tab, "mmWave", lv_color_hex(0x800080));
    label_mmwave = lv_label_create(card4);
    lv_label_set_long_mode(label_mmwave, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label_mmwave, 115);
    lv_obj_set_style_text_align(label_mmwave, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label_mmwave, "-- cm\nClear");
    lv_obj_set_style_text_font(label_mmwave, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_mmwave, lv_color_white(), 0);
}

static void create_sensor_charts_tab(lv_obj_t * tab) {
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x000000), 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(tab, 20, 0); 
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

    sensor_chart_lux = lv_chart_create(tab);
    lv_obj_set_size(sensor_chart_lux, 240, 160);
    lv_obj_set_style_bg_color(sensor_chart_lux, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(sensor_chart_lux, 1, 0);
    lv_obj_set_style_border_color(sensor_chart_lux, lv_color_hex(0x333333), 0);
    lv_chart_set_type(sensor_chart_lux, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(sensor_chart_lux, 50); 
    lv_chart_set_range(sensor_chart_lux, LV_CHART_AXIS_PRIMARY_Y, 0, 1000);
    lv_chart_set_axis_tick(sensor_chart_lux, LV_CHART_AXIS_PRIMARY_Y, 5, 2, 5, 2, true, 45);
    ser_lux = lv_chart_add_series(sensor_chart_lux, lv_palette_main(LV_PALETTE_ORANGE), LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_line_width(sensor_chart_lux, 2, LV_PART_ITEMS);

    sensor_chart_env = lv_chart_create(tab);
    lv_obj_set_size(sensor_chart_env, 240, 160);
    lv_obj_set_style_bg_color(sensor_chart_env, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(sensor_chart_env, 1, 0);
    lv_obj_set_style_border_color(sensor_chart_env, lv_color_hex(0x333333), 0);
    lv_chart_set_type(sensor_chart_env, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(sensor_chart_env, 50);
    lv_chart_set_range(sensor_chart_env, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_axis_tick(sensor_chart_env, LV_CHART_AXIS_PRIMARY_Y, 5, 2, 5, 2, true, 45);
    ser_env_temp = lv_chart_add_series(sensor_chart_env, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    ser_env_pres = lv_chart_add_series(sensor_chart_env, lv_palette_main(LV_PALETTE_YELLOW), LV_CHART_AXIS_PRIMARY_Y);
    ser_env_hum  = lv_chart_add_series(sensor_chart_env, lv_palette_main(LV_PALETTE_CYAN), LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_line_width(sensor_chart_env, 2, LV_PART_ITEMS);
}

static void create_chart_tab(lv_obj_t * tab) {
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x000000), 0);
    lv_obj_set_style_pad_all(tab, 0, 0); 
    lv_obj_add_flag(tab, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tab, chart_event_cb, LV_EVENT_CLICKED, NULL);
    label_chart_info = lv_label_create(tab);
    lv_obj_set_style_text_font(label_chart_info, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(label_chart_info, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(label_chart_info, LV_ALIGN_TOP_LEFT, 20, 5);
    lv_label_set_text(label_chart_info, "BTC/USD");
    label_chart_status = lv_label_create(tab);
    lv_obj_set_style_text_font(label_chart_status, &lv_font_montserrat_18, 0);
    lv_obj_align(label_chart_status, LV_ALIGN_TOP_RIGHT, -20, 5);
    lv_label_set_text(label_chart_status, "Loading...");
    chart = lv_chart_create(tab);
    lv_obj_set_size(chart, 510, 130);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x050505), 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    ser_temp = lv_chart_add_series(chart, lv_color_hex(0xFF9500), LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS);
    lv_obj_add_flag(chart, LV_OBJ_FLAG_EVENT_BUBBLE);
}

static void create_crypto_tab(lv_obj_t * tab) {
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x000000), 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(tab, 10, 0);
    label_btc_usd = lv_label_create(tab);
    lv_obj_set_style_text_font(label_btc_usd, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(label_btc_usd, lv_color_hex(0xFF9500), 0);
    label_btc_eur = lv_label_create(tab);
    lv_obj_set_style_text_font(label_btc_eur, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(label_btc_eur, lv_color_white(), 0);
    label_btc_chf = lv_label_create(tab);
    lv_obj_set_style_text_font(label_btc_chf, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(label_btc_chf, lv_color_white(), 0);
}

static void create_info_tab(lv_obj_t * tab) {
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x000000), 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    
    label_wifi_info = lv_label_create(tab);
    lv_obj_set_style_text_font(label_wifi_info, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(label_wifi_info, lv_color_white(), 0);
    lv_obj_align(label_wifi_info, LV_ALIGN_TOP_LEFT, 20, 5);
    lv_label_set_text(label_wifi_info, "WLAN Status\nChecking...");
}

static void create_qr_tab(lv_obj_t * tab) {
    lv_obj_set_style_bg_color(tab, lv_color_hex(0x000000), 0);
    lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // QR Code wird dynamisch in ui_update_qr_code() erstellt, sobald IP da ist.
    // Wir erstellen hier einen Container oder ein Label als Platzhalter/Titel
    lv_obj_t * label = lv_label_create(tab);
    lv_label_set_text(label, "Scan for Web Dashboard");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_pad_bottom(label, 10, 0);
}

extern "C" {
    void ui_update_wifi_info(const char* ssid, int rssi_pct, float down_speed, float up_speed, int freq, int channel, const char* status, const char* ip, const char* uptime, float bat_v, int bat_pct) {
        if (!label_wifi_info) return;
        char buf[1024];

        // Bluetooth Status holen
        bool ble_conn = is_ble_connected();
        int ble_rssi = get_ble_rssi();
        const char* ble_client = get_ble_client_name();

        snprintf(buf, sizeof(buf),
            "Status: %s (IP: %s)\n"
            "SSID: %s | Sig: %d%%\n"
            "Spd: %s%.1f %s%.1f Mbit/s\n"
            "Freq: %d MHz (Ch %d)\n"
            "-----------------------------\n"
            "BLE: %s (%d dBm)\n"
            "Client: %s\n"
            "Mode: LE Coded PHY (S=8)\n"
            "-----------------------------\n"
            "Uptime: %s\n"
            "Battery: %.2fV (%d%%) %s",
            status, ip, ssid, rssi_pct, LV_SYMBOL_DOWNLOAD, down_speed, LV_SYMBOL_UPLOAD, up_speed, freq, channel,
            ble_conn ? "CONNECTED" : "ADVERTISING", ble_rssi, ble_client, uptime, bat_v, bat_pct, LV_SYMBOL_CHARGE);
        lv_label_set_text(label_wifi_info, buf);
    }

    void ui_update_qr_code(const char* ip_address) {
        if (!tab_qr) return;
        if (qr_obj) return; // QR Code schon erstellt
        if(strlen(ip_address) < 7) return; 

        char url[64];
        snprintf(url, sizeof(url), "http://%s", ip_address);
        
        // QR Code erstellen: Größe 150px
        qr_obj = lv_qrcode_create(tab_qr, 150, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
        lv_qrcode_update(qr_obj, url, strlen(url));
        
        // Style: Weißer Rahmen
        lv_obj_set_style_border_color(qr_obj, lv_color_white(), 0);
        lv_obj_set_style_border_width(qr_obj, 5, 0);
        lv_obj_set_style_radius(qr_obj, 5, 0);
    }

    void ui_init(void) {
        lv_obj_t * scr = lv_scr_act();
        lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
        lv_obj_t * tv = lv_tabview_create(scr, LV_DIR_TOP, 50);
        lv_obj_set_style_bg_color(tv, lv_color_hex(0x000000), 0);
        
        lv_obj_t * tab_btns = lv_tabview_get_tab_btns(tv);
        lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0x1A1A1A), 0);
        lv_obj_set_style_text_font(tab_btns, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(tab_btns, lv_color_hex(0x666666), 0);
        lv_obj_set_style_text_color(tab_btns, lv_color_white(), LV_STATE_CHECKED);

        tab_sensors = lv_tabview_add_tab(tv, "SENSORS");
        tab_sensor_charts = lv_tabview_add_tab(tv, "CHART");
        tab_btc_chart = lv_tabview_add_tab(tv, "CRYPTO");
        tab_btc_prices = lv_tabview_add_tab(tv, "PRICE");
        tab_info = lv_tabview_add_tab(tv, "INFO");
        tab_qr = lv_tabview_add_tab(tv, "QR"); // Neuer Tab
        
        create_sensor_tab(tab_sensors);
        create_sensor_charts_tab(tab_sensor_charts);
        create_chart_tab(tab_btc_chart);
        create_crypto_tab(tab_btc_prices);
        create_info_tab(tab_info);
        create_qr_tab(tab_qr);
    }

    void ui_update_sensor_charts(float lux, float temp, float hum, float pres) {
        if(sensor_chart_lux && ser_lux) lv_chart_set_next_value(sensor_chart_lux, ser_lux, (lv_coord_t)lux);
        if(sensor_chart_env) {
            lv_chart_set_next_value(sensor_chart_env, ser_env_temp, (lv_coord_t)temp);
            lv_chart_set_next_value(sensor_chart_env, ser_env_hum, (lv_coord_t)hum);
            lv_chart_set_next_value(sensor_chart_env, ser_env_pres, (lv_coord_t)(pres * 100));
        }
    }

    void ui_set_sensor_data(float lux, int aqi, int voc, int nox, float temp, float hum, float pres, float gx, float gy, float gz, float pitch, float roll, float radar_dist, bool person_detected) {
        char buf[128]; 
        if(label_lux) { snprintf(buf, sizeof(buf), "%.0f Lx", lux); lv_label_set_text(label_lux, buf); }
        if(label_gyro) { 
            snprintf(buf, sizeof(buf), "P: %.1f°\nR: %.1f°", pitch, roll); 
            lv_label_set_text(label_gyro, buf); 
            lv_obj_set_style_text_font(label_gyro, &lv_font_montserrat_24, 0);
        }
        if(label_aqi) {
            if(aqi <= 0) { lv_label_set_text(label_aqi, "--"); lv_obj_set_style_text_color(label_aqi, lv_palette_main(LV_PALETTE_GREY), 0); }
            else {
                snprintf(buf, sizeof(buf), "%d", aqi); lv_label_set_text(label_aqi, buf);
                lv_color_t c = (aqi < 200) ? lv_palette_main(LV_PALETTE_GREEN) : (aqi < 350) ? lv_palette_main(LV_PALETTE_YELLOW) : (aqi < 450) ? lv_palette_main(LV_PALETTE_ORANGE) : lv_palette_main(LV_PALETTE_RED);
                lv_obj_set_style_text_color(label_aqi, c, 0);
                lv_obj_set_style_border_color(lv_obj_get_parent(label_aqi), c, 0);
            }
        }
        if(label_voc_nox) { 
            snprintf(buf, sizeof(buf), "VOC: %d\nNOx: %d", voc, nox); 
            lv_label_set_text(label_voc_nox, buf); 
            lv_obj_set_style_text_font(label_voc_nox, &lv_font_montserrat_24, 0); 
        }
        if(label_temp) { 
            snprintf(buf, sizeof(buf), "%.1f°C\n%.2fbar\n%.0f%%", temp, pres, hum); 
            lv_label_set_text(label_temp, buf); 
            lv_obj_set_style_text_font(label_temp, &lv_font_montserrat_24, 0);
        }
        if(label_mmwave) {
            snprintf(buf, sizeof(buf), "%.0f cm\n%s", radar_dist, person_detected ? "Detected" : "Clear");
            lv_label_set_text(label_mmwave, buf);
            lv_obj_set_style_text_font(label_mmwave, &lv_font_montserrat_24, 0);
            lv_obj_set_style_border_color(lv_obj_get_parent(label_mmwave), person_detected ? lv_color_hex(0x800080) : lv_palette_main(LV_PALETTE_GREY), 0);
        }
    }

    void ui_set_crypto_prices(float usd, float eur, float chf) {
        char buf[32];
        if(label_btc_usd) { snprintf(buf, sizeof(buf), "$ %.2f", usd); lv_label_set_text(label_btc_usd, buf); }
        if(label_btc_eur) { snprintf(buf, sizeof(buf), "EUR %.2f", eur); lv_label_set_text(label_btc_eur, buf); }
        if(label_btc_chf) { snprintf(buf, sizeof(buf), "CHF %.2f", chf); lv_label_set_text(label_btc_chf, buf); }
    }

    void ui_update_chart(float *prices, int count, float min_p, float max_p, int tf) {
        if (chart == NULL || ser_temp == NULL || count < 2) return;
        lv_obj_set_style_opa(chart, 255, 0);
        float diff = prices[count-1] - prices[0];
        float pct = (diff / prices[0]) * 100.0;
        lv_color_t trend_color = (diff >= 0) ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000);
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 1000);
        lv_chart_set_point_count(chart, count);
        float range = max_p - min_p; if (range < 0.1) range = 1.0; 
        for (int i = 0; i < count; i++) {
            float scaled = (prices[i] - min_p) / range * 1000.0f;
            lv_chart_set_value_by_id(chart, ser_temp, i, (lv_coord_t)scaled);
        }
        lv_chart_set_series_color(chart, ser_temp, trend_color);
        if(label_chart_status) {
            char status_buf[64]; lv_obj_set_style_text_color(label_chart_status, trend_color, 0);
            snprintf(status_buf, sizeof(status_buf), "$ %.0f (%s%.2f%%)", prices[count-1], (diff >= 0) ? "+" : "", pct);
            lv_label_set_text(label_chart_status, status_buf);
        }
        if(label_chart_info) {
            const char* tfs[] = {"BTC 1h", "BTC 24h", "BTC 7d", "BTC 1m", "BTC 1y", "BTC 5y"};
            lv_label_set_text(label_chart_info, tfs[tf]);
        }
        lv_chart_refresh(chart);
    }
}
