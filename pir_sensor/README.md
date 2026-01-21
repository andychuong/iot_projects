# PIR Motion Sensor

WiFi-enabled room occupancy sensor using ESP32 Feather V2 and a PIR motion sensor.

## Features

- **Web Dashboard** — Real-time occupancy status with auto-refresh
- **JSON API** — Motion state, occupancy status, and event count
- **OTA Updates**
- **Status LED** — Green (empty), Orange (occupied), Red (no WiFi)
- **Occupancy Timeout** — Room marked empty after 5 minutes of no motion

## Hardware

| Part | Description |
|------|-------------|
| Adafruit ESP32 Feather V2 | Main microcontroller |
| PIR Motion Sensor (HC-SR501) | Motion detection |

## Wiring

| Connection | From | To |
|------------|------|-----|
| Signal | PIR OUT | ESP32 GPIO 14 |
| Power | PIR VCC | ESP32 3.3V |
| Ground | PIR GND | ESP32 GND |

> **Note:** Most PIR sensors (like HC-SR501) have adjustable sensitivity and delay potentiometers. The code uses a 5-minute software timeout for occupancy detection.

## Setup

1. Copy `arduino_secrets.h` and fill in your credentials:
   ```cpp
   #define SECRET_WIFI_SSID "your_wifi"
   #define SECRET_WIFI_PASS "your_password"
   #define SECRET_OTA_PASS "your_ota_password"
   ```

2. Upload to ESP32 Feather V2

3. Wait for PIR sensor to stabilize (~30-60 seconds on first power-up)

4. Access the dashboard at `http://10.0.0.87` (or check Serial for IP)

## API

**Endpoint:** `GET /api`

**Response:**
```json
{
  "motion_detected": false,
  "room_occupied": true,
  "status": "occupied",
  "seconds_since_motion": 45,
  "motion_count": 12,
  "uptime_ms": 300000
}
```
