# ESP32-AMOLED-Sensorbase (Long Range BLE Edition)

Waveshare ESP32-S3 with 1.91" AMOLED touchscreen and real-time sensor telemetry via high-performance **Bluetooth 5.0 LE Coded PHY (Long Range)**.

## Features of Bluetooth
Standard Bluetooth (1M PHY) often fails over water surfaces due to multipath interference and signal reflections, losing connection after 20-30 meters.
This implementation uses **LE Coded PHY (S=8)**:
- **Redundant Modulation:** Every bit is sent 8 times, allowing Forward Error Correction (FEC).
- **Link Budget:** Increases range by ~12 dB without higher power consumption.
- **Range:** Stable telemetry (Speed, G-Force) up to **200m+** on open water.
- **Smartphone Sync:** Fully optimized for high-end devices like the Samsung Galaxy S25 Ultra using the nRF Connect app.

##  Key Features
- **Display:** 1.91" AMOLED (Waveshare ESP32-S3 AMOLED) with LVGL 8.3.9.
- **BLE Long Range Telemetry:**
  - Real-time **Speed** and **G-Force** (Aggregate).
  - **Environment:** Lux, AQI (VOC/NOx), Temperature, Humidity, Pressure.
  - **mmWave Radar:** 24GHz Presence & Distance tracking.
- **Smart UI:** 
  - **AMOLED "INFO" Tab:** Real-time BLE status, RSSI (smoothed), and Peer Address.
  - **Web Dashboard:** Responsive UI with high-speed (250ms) RSSI history graphs.
- **OTA UI-Updates:** Decoupled Web-UI updates via GitHub (LittleFS) using GZIP compression.

## Software Stack
- **Library:** `h2zero/NimBLE-Arduino` (Optimized for memory and speed).
- **Framework:** Arduino / ESP-IDF.
- **Web-Server:** ESPAsyncWebServer (serving GZIP assets from LittleFS).

## Setup & Usage
1.  **Clone** this repository.
2.  Rename `include/secrets.h.example` to `include/secrets.h` and enter your WiFi credentials.
3.  **Flash Web-UI:** In PlatformIO, run **"Upload Filesystem Image"** to flash `data/` to LittleFS.
4.  **Flash Firmware:** Build and Upload the code.
5.  **Connect:** Open **nRF Connect** on your smartphone, find `S3-AMOLED-LR`, and select **"Connect with Preferred PHY"** -> **LE Coded**.

## Telemetry Analysis
The Web-UI provides a precision RSSI graph (-120 dBm to +30 dBm) to analyze signal stability during field tests. The ESP32 implements **exponential smoothing** to ensure readable values even in harsh RF environments.

Developed for high-performance telemetry on open water.
---

## UI Documentation & Visuals
1. Main Telemetry Dashboard
Real-time sensor data including Speed, G-Force, and environmental metrics displayed on the AMOLED panel.


![20260217_210533](https://github.com/user-attachments/assets/8270c394-6042-43ee-8a11-7e67026d5cc9)

2. Wireless Info & Connectivity Status
The "INFO" tab displays critical BLE Long Range parameters, connection status, and the smoothed RSSI signal strength.

![20260217_210522](https://github.com/user-attachments/assets/1356e972-1e89-484a-92be-2445b54c96fa)

3. Web Dashboard Access
Scan the generated QR code to access the high-speed Web-UI (ensure your device is in the same WiFi network).
![20260217_210850](https://github.com/user-attachments/assets/621f29a5-f7cc-45d2-b99c-e992ac8cc6e6)

4. You will endup on the Web Dashboard:
<img width="1469" height="1219" alt="image" src="https://github.com/user-attachments/assets/cc339a1f-e0d8-4d20-a55e-84150cb70315" />
<img width="1526" height="806" alt="image" src="https://github.com/user-attachments/assets/2b42d55f-bef6-46e3-978b-5b341e86bd30" />


