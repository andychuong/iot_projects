# Pomodoro Timer

WiFi-enabled Pomodoro timer using ESP32 Feather V2 with LCD display, NeoPixel progress bar, and web dashboard.

## Features

- **25/5/15 Timer** — Standard Pomodoro: 25min work, 5min short break, 15min long break (every 4 cycles)
- **LCD Display** — Shows current mode, time remaining, and cycle count
- **NeoPixel Progress Bar** — Visual countdown with color-coded status
- **Web Dashboard** — Monitor and control timer remotely
- **JSON API** — Timer state, mode, and statistics
- **OTA Updates**
- **Buzzer Alarm** — Audio alert when timer completes
- **Mode LEDs** — Red (work), Green (break)

## Hardware

| Part | Description |
|------|-------------|
| Adafruit ESP32 Feather V2 | Main microcontroller |
| LCD 16x2 with I2C backpack | Timer display |
| NeoPixel/WS2812B Strip (8 LEDs) | Visual progress bar |
| Piezo Buzzer | Alarm sound |
| Push Button x3 | Start/Pause, Reset, Mode |
| Red LED | Work indicator |
| Green LED | Break indicator |
| 220Ω Resistor x2 | LED current limiting |
| 10kΩ Resistor x3 | Button pull-down |

## Wiring

### LCD (I2C)

| Connection | From | To |
|------------|------|-----|
| SDA | LCD SDA | ESP32 GPIO 23 (SDA) |
| SCL | LCD SCL | ESP32 GPIO 22 (SCL) |
| VCC | LCD VCC | ESP32 3.3V |
| GND | LCD GND | ESP32 GND |

### NeoPixel Strip

| Connection | From | To |
|------------|------|-----|
| Data In | Strip DIN | ESP32 GPIO 15 |
| Power | Strip VCC | ESP32 5V (USB) |
| Ground | Strip GND | ESP32 GND |

### Buttons (Active HIGH with pull-down resistors)

| Connection | From | To |
|------------|------|-----|
| Start/Pause | Button 1 | ESP32 GPIO 32 |
| Reset | Button 2 | ESP32 GPIO 33 |
| Mode | Button 3 | ESP32 GPIO 27 |

> Connect each button: one leg to 3.3V, other leg to GPIO pin AND to GND through 10kΩ resistor.

### Buzzer

| Connection | From | To |
|------------|------|-----|
| Positive | Buzzer + | ESP32 GPIO 26 |
| Negative | Buzzer - | ESP32 GND |

### Status LEDs

| Connection | From | To |
|------------|------|-----|
| Work LED | Red LED + | ESP32 GPIO 12 (via 220Ω) |
| Break LED | Green LED + | ESP32 GPIO 14 (via 220Ω) |
| Ground | LED - | ESP32 GND |

## Setup

1. Copy `arduino_secrets.h` and fill in your credentials:
   ```cpp
   #define SECRET_WIFI_SSID "your_wifi"
   #define SECRET_WIFI_PASS "your_password"
   #define SECRET_OTA_PASS "your_ota_password"
   ```

2. Install required libraries in Arduino IDE:
   - LiquidCrystal_I2C
   - Adafruit_NeoPixel

3. Upload to ESP32 Feather V2

4. Access the dashboard at `http://10.0.0.88` (or check Serial for IP)

## Usage

| Button | Action |
|--------|--------|
| Start/Pause | Start or pause the timer |
| Reset | Reset current timer to beginning |
| Mode | Cycle through: Work → Short Break → Long Break |

## API

**Endpoint:** `GET /api`

**Response:**
```json
{
  "mode": "work",
  "running": true,
  "time_remaining_sec": 1234,
  "time_total_sec": 1500,
  "cycle": 2,
  "total_cycles": 4,
  "work_sessions_completed": 5
}
```

**Control Endpoints:**
- `POST /start` — Start timer
- `POST /pause` — Pause timer
- `POST /reset` — Reset current timer
- `POST /mode?m=work|short_break|long_break` — Set mode
