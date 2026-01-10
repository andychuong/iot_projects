# Magnetic Door Sensor

WiFi-enabled door open/close sensor using ESP32 Feather V2 and a magnetic reed switch.

## Features

- **Web Dashboard** — Real-time door status with auto-refresh
- **JSON API** — Door state, duration, and open count
- **OTA Updates**
- **Status LED** — Green (closed), Orange (open), Red (no WiFi)

## Hardware

| Part | Description |
|------|-------------|
| Adafruit ESP32 Feather V2 | Main microcontroller |
| Magnetic Reed Switch | Door open/close detection |

## Wiring

| Connection | From | To |
|------------|------|-----|
| Sensor Pin | Reed Switch (NO) | ESP32 GPIO 14 |
| Ground | Reed Switch (COM) | ESP32 GND |

> **Note:** The code uses the internal pull-up resistor. When the magnet is near (door closed), the switch closes and pulls the pin LOW. When the magnet is away (door open), the pin reads HIGH.

## Setup

1. Copy `arduino_secrets.h` and fill in your credentials:
   ```cpp
   #define SECRET_WIFI_SSID "your_wifi"
   #define SECRET_WIFI_PASS "your_password"
   #define SECRET_OTA_PASS "your_ota_password"
   ```

2. Upload to ESP32 Feather V2

3. Access the dashboard at `http://10.0.0.86` (or check Serial for IP)

## API

**Endpoint:** `GET /api`

**Response:**
```json
{
  "door_open": false,
  "status": "closed",
  "state_duration_sec": 120,
  "open_count": 5,
  "uptime_ms": 300000
}
```
