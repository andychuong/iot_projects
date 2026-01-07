# ESP32 Feather V2 + DHT11 Temperature & Humidity Monitor

A WiFi-enabled temperature and humidity monitor using the Adafruit ESP32 Feather V2 and DHT11 sensor.

## Features

- **Web Dashboard** — Real-time temperature and humidity display with auto-refresh
- **JSON API**
- **OTA Updates**
- **Status LED** — Onboard NeoPixel

## Hardware

#### Components

| Part | Description |
|------|-------------|
| Adafruit ESP32 Feather V2| Main microcontroller |
| DHT11 (3-pin breakout).  | Temperature & humidity sensor |

#### Wiring

| DHT11 Pin | ESP32 Feather V2 |
|-----------|------------------|
| VCC (+)   | 3V               |
| DATA (S)  | GPIO 14          |
| GND (-)   | GND              |

> Note: 3-pin breakout boards have a built-in pull-up resistor. If using a raw DHT11, add a 10kΩ resistor between DATA and VCC.