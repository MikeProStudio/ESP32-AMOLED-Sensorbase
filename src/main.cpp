#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include "pins_config.h"
#include "sd_card_bsp.h"
#include "sd_logger.h"
#include "lvgl.h"
#include "rm67162.h"
#include "FT3168.h"
#include "ui_app.h" 
#include "wifi_logic.h"
#include "crypto_logic.h" 
#include "data_manager.h" 
#include "web_server.h"
#include "ble_long_range.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

SemaphoreHandle_t wireMutex = NULL;

#define TOUCH_SDA 40
#define TOUCH_SCL 39
#define TOUCH_INT 41
#define TOUCH_RST -1 

FT3168 touch(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT);



static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf; 
volatile int request_timeframe_update = 0;



void update_ui_and_brightness(float lux, int aqi, int voc, int nox, float temp, float hum, float pres, float gx, float gy, float gz, float pitch, float roll, float radar_dist, bool person_detected);
void set_amoled_brightness(uint8_t level) {
    lcd_set_brightness(level);
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    lcd_address_set(area->x1, area->y1, area->x2, area->y2);
    lcd_PushColors((uint16_t *)color_p, w * h);
    lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data) {
    uint16_t x, y;
    bool touched = false;
    if (xSemaphoreTake(wireMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        touched = touch.getTouch(&x, &y, NULL);
        xSemaphoreGive(wireMutex);
    }
    if (touched) {
        data->point.x = y; 
        data->point.y = 240 - x; 
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void lvgl_loop_task(void* p) {
    Serial.println("[Task] LVGL Task gestartet...");
    ui_init(); 
    uint32_t last_tick = millis();
    while(1) {
        uint32_t now = millis();
        lv_tick_inc(now - last_tick);
        last_tick = now;
        lv_timer_handler(); 
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void run_i2c_scanner() {
    byte error, address;
    int nDevices = 0;
    Serial.println("\n--- START I2C0 SCANNER ---");
    for(address = 1; address < 127; address++ ) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0) {
            Serial.printf("! I2C0 GERÄT GEFUNDEN: 0x%02X\n", address);
            nDevices++;
        }
    }
    if (nDevices == 0) Serial.println("Kein I2C0 Gerät gefunden.");
}

void run_i2c_scanner_i2c1() {
    byte error, address;
    int nDevices = 0;
    Serial.println("\n--- START I2C1 SCANNER ---");
    for(address = 1; address < 127; address++ ) {
        Wire1.beginTransmission(address);
        error = Wire1.endTransmission();
        if (error == 0) {
            Serial.printf("! I2C1 GERÄT GEFUNDEN: 0x%02X\n", address);
            nDevices++;
        }
    }
    if (nDevices == 0) Serial.println("Kein I2C1 Gerät gefunden.");
}

void setup() {
    Serial.begin(115200);
    

    wireMutex = xSemaphoreCreateMutex();
    delay(2000); 

    // DISABLE BROWNOUT DETECTOR
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.println("\n\n--- SAFE BOOT START ---");
    Wire.begin(40, 39);
    Wire.setClock(100000); 
    run_i2c_scanner(); 
    if (!touch.begin(0x38)) Serial.println("FATAL: FT3168 Touch Controller nicht gefunden!");
    Wire1.begin(3, 2); 
    Wire1.setClock(100000); 
    run_i2c_scanner_i2c1(); 
    xTaskCreatePinnedToCore(radar_task, "radar", 4096, NULL, 1, NULL, 0);

    rm67162_init(); 
    lcd_setRotation(1);
    lcd_fill(0, 0, 536, 240, 0x001F); 
    set_amoled_brightness(20); // START WITH LOW BRIGHTNESS TO PREVENT BROWNOUT
    delay(200);

    lv_init();
    size_t buffer_size_pixels = 536 * 40; 
    buf = (lv_color_t *)ps_malloc(buffer_size_pixels * sizeof(lv_color_t));
    if (!buf) { while(1) delay(1000); }
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, buffer_size_pixels);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 536; disp_drv.ver_res = 240;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    xTaskCreatePinnedToCore(lvgl_loop_task, "lvgl", 20000, NULL, 5, NULL, 1);
    delay(200); // SPREAD OUT INITIALIZATION SPIKES

    init_wifi(); 
    delay(200); // SPREAD OUT INITIALIZATION SPIKES
    
    /* Webserver initialization is now handled in wifi_logic.cpp upon connection
    // Webserver starten
    if(WiFi.status() == WL_CONNECTED) {
        init_webserver();
        Serial.print("Webserver IP: ");
        Serial.println(WiFi.localIP());
    }
    */

    delay(200); // SPREAD OUT INITIALIZATION SPIKES

    // BLE Long Range initialisieren
    init_ble_long_range();
    delay(200); // SPREAD OUT INITIALIZATION SPIKES

    request_timeframe_update = 0; 
    // update_btc_price(); -> Removed from setup to avoid potential sync issues at boot
    
    // SD Karte deaktiviert
    // SD_card_Init();
    // init_logger();

    pinMode(1, INPUT); //Battery Voltage ADC Reading
    pinMode(4, OUTPUT); //Radar LED Alarm Pin
    analogReadResolution(12); //12Bit ADC
    // Erstelle den Radar Task auf Core 0 (LVGL läuft auf Core 1)
    
    // RESTORE DISPLAY BRIGHTNESS FOR REGULAR OPERATION
    set_amoled_brightness(255);
}



void loop() {
    static uint32_t last_btc_price_update = 0;
    static uint32_t last_chart_auto_update = 0;
    static uint32_t last_log_time = 0;
    static uint32_t last_print_time = 0; 
    static bool last_person_in_range = false;
    float lux = 0, temp = 0, hum = 0, pres = 0, gx = 0, gy = 0, gz = 0, pitch = 0, roll = 0;
    int aqi = -1, voc = 0, nox = 0;
    get_sensor_readings(&lux, &aqi, &voc, &nox, &temp, &hum, &pres, &gx, &gy, &gz, &pitch, &roll);
    update_ui_and_brightness(lux, aqi, voc, nox, temp, hum, pres, gx, gy, gz, pitch, roll, radar_distance, person_in_range);
    update_wifi_status_logic();

    // Berechne Battery Percentage für BLE (falls nicht schon vorhanden)
    int raw_adc = analogRead(1);
    float avg_mv = (float)raw_adc * (3300.0f / 4095.0f);
    float bat_v = (avg_mv * 2.0f) / 1000.0f;
    int bat_pct = (int)((bat_v - 3.2f) * 100.0f / (3.7f - 3.2f));
    if (bat_pct > 100) bat_pct = 100; if (bat_pct < 0) bat_pct = 0;

    // BLE Telemetrie aktualisieren (Long Range) mit allen Sensorwerten inkl. Gyro (Pitch/Roll) & Battery
    update_ble_telemetry(0.0f, gx, gy, gz, lux, aqi, temp, hum, pres, radar_distance, bat_pct, pitch, roll);

    if (person_in_range != last_person_in_range) {
        last_person_in_range = person_in_range; // Zuerst speichern!
        
        Serial.println(">>> ZUSTAND GEÄNDERT <<<");
        Serial.printf("Status: %s | Distanz: %.1f cm\n", 
                      person_in_range ? "BETRETEN" : "VERLASSEN", 
                      radar_distance);
    }
    
    // Webserver Update alle 1s (statt 10s logging)
    if (millis() - last_log_time >= 1000) {
        int raw_adc = analogRead(1);
        float avg_mv = (float)raw_adc * (3300.0f / 4095.0f);
        float bat_v = (avg_mv * 2.0f) / 1000.0f;
        int bat_pct = (int)((bat_v - 3.2f) * 100.0f / (3.7f - 3.2f));
        if (bat_pct > 100) bat_pct = 100; if (bat_pct < 0) bat_pct = 0;
        
        update_webserver_data(lux, pitch, roll, aqi, voc, nox, temp, pres, hum, bat_v, bat_pct,
                               get_wifi_rssi(), get_wifi_rssi_pct(), get_wifi_down_speed(), get_wifi_up_speed(), get_wifi_freq(), get_wifi_channel(),
                               radar_distance, person_in_range);
        last_log_time = millis();
    }
    
    if (request_timeframe_update != -1) {
        int tf = request_timeframe_update; request_timeframe_update = -1;
        update_btc_chart(tf); last_chart_auto_update = millis();
    }
    if (millis() - last_btc_price_update > 60000) {
        if (WiFi.status() == WL_CONNECTED) update_btc_price();
        last_btc_price_update = millis();
    }
    if (millis() - last_chart_auto_update > 600000) {
        request_timeframe_update = 0; last_chart_auto_update = millis();
    }

    if (millis() - last_print_time > 5000) {
        //Serial.printf("Info: Aktuelle Distanz: %.1f cm (Detected: %s)\n", radar_distance, person_in_range ? "JA" : "NEIN");
        last_print_time = millis();
    }
    vTaskDelay(pdMS_TO_TICKS(1)); 
}
