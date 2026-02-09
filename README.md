# ESP32-AMOLED-Sensorbase

Waveshare ESP32-S3 with 1.91" AMOLED-Touchscreen and Sensordata-Display Implementation with WebUI.

## Features
- **Display:** 1.91" AMOLED (Waveshare ESP32-S3 AMOLED)
- **Sensors:** 
  - Air Quality (AQI, VOC, NOx) via SGP41
  - Environmental (Temp, Humidity, Pressure) via AHTX0/BMP280
  - Radar: 24GHz mmWave Presence Detection (S3KM1110)
  - Light Intensity (BH1750)
- **Web Interface:** Responsive Dashboard with real-time charts (Chart.js)
- **OTA UI-Updates:** Decoupled Web-UI updates via GitHub (LittleFS)

## Software Stack
- **Framework:** Arduino (Espressif 32)
- **IDE:** PlatformIO
- **UI-Library:** LVGL 8.3.9
- **Web-Server:** ESPAsyncWebServer (with LittleFS serving)

## Setup
1. Clone the repository.
2. Rename `include/secrets.h.example` to `include/secrets.h` and enter your WiFi credentials.
3. In PlatformIO, run **"Upload Filesystem Image"** to flash the Web-UI to LittleFS.
4. Build and Upload the firmware.

## Web-UI Update Logic
The Web-UI is stored in `data/www/`. The device can check for updates on GitHub and download a new `index.html.gz` without requiring a firmware reflash.
- Endpoint for manual update check: `http://[DEVICE_IP]/update_ui`
