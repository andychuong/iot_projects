/*
 * Magnetic Door Sensor
 *
 * WiFi-enabled door open/close sensor using ESP32 Feather V2
 * and a magnetic reed switch.
 *
 * Endpoints:
 *   /     - Web dashboard
 *   /api  - JSON API
 */

#include "arduino_secrets.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>

#define MAG_SENSOR_PIN 14
#define NEOPIXEL_PIN 0
#define NEOPIXEL_POWER 2

const char* ssid = SECRET_WIFI_SSID;
const char* password = SECRET_WIFI_PASS;

WebServer server(80);
Adafruit_NeoPixel pixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Door state tracking
bool doorOpen = false;
unsigned long lastStateChange = 0;
unsigned long openCount = 0;

void setPixel(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void handleRoot() {
  String status = doorOpen ? "OPEN" : "CLOSED";
  String statusColor = doorOpen ? "#e74c3c" : "#27ae60";
  unsigned long duration = (millis() - lastStateChange) / 1000;

  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta http-equiv='refresh' content='2'>
  <title>Door Sensor</title>
  <style>
    body { font-family: Arial; text-align: center; padding: 50px; background: #1a1a2e; color: #eee; }
    .status { font-size: 3em; margin: 30px; padding: 30px; border-radius: 15px; }
    .info { font-size: 1.2em; margin: 15px; padding: 15px; background: #16213e; border-radius: 10px; }
    .label { color: #888; font-size: 0.7em; }
  </style>
</head>
<body>
  <h1>Door Sensor</h1>
  <div class='status' style='background: )" + statusColor + R"(;'>
    )" + status + R"(
  </div>
  <div class='info'>
    <div class='label'>Current State Duration</div>
    )" + String(duration) + R"( seconds
  </div>
  <div class='info'>
    <div class='label'>Times Opened</div>
    )" + String(openCount) + R"(
  </div>
</body>
</html>
)";

  server.send(200, "text/html", html);
}

void handleApi() {
  unsigned long duration = (millis() - lastStateChange) / 1000;

  String json = "{";
  json += "\"door_open\":" + String(doorOpen ? "true" : "false") + ",";
  json += "\"status\":\"" + String(doorOpen ? "open" : "closed") + "\",";
  json += "\"state_duration_sec\":" + String(duration) + ",";
  json += "\"open_count\":" + String(openCount) + ",";
  json += "\"uptime_ms\":" + String(millis());
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  // Mag sensor with internal pull-up
  pinMode(MAG_SENSOR_PIN, INPUT_PULLUP);

  // NeoPixel setup
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);
  pixel.begin();
  pixel.setBrightness(20);
  setPixel(255, 0, 0);

  // WiFi
  IPAddress staticIP(10, 0, 0, 86);
  IPAddress gateway(10, 0, 0, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  // OTA
  ArduinoOTA.setHostname("door-sensor");
  ArduinoOTA.setPassword(SECRET_OTA_PASS);

  ArduinoOTA.onStart([]() {
    setPixel(0, 0, 255);
  });
  ArduinoOTA.onEnd([]() {
    setPixel(0, 255, 0);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    setPixel(255, 0, 0);
  });

  ArduinoOTA.begin();

  // Web server
  server.on("/", handleRoot);
  server.on("/api", handleApi);
  server.begin();

  // Initialize door state
  doorOpen = (digitalRead(MAG_SENSOR_PIN) == HIGH);
  lastStateChange = millis();

  Serial.println("Door sensor ready");
  Serial.print("Door is currently: ");
  Serial.println(doorOpen ? "OPEN" : "CLOSED");

  updateStatusLED();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  // Check door state
  bool currentState = (digitalRead(MAG_SENSOR_PIN) == HIGH);

  if (currentState != doorOpen) {
    doorOpen = currentState;
    lastStateChange = millis();

    if (doorOpen) {
      openCount++;
      Serial.println("Door OPENED");
    } else {
      Serial.println("Door CLOSED");
    }

    updateStatusLED();
  }

  delay(50);  // Debounce
}

void updateStatusLED() {
  if (WiFi.status() != WL_CONNECTED) {
    setPixel(255, 0, 0);  // Red: no WiFi
  } else if (doorOpen) {
    setPixel(255, 165, 0);  // Orange: door open
  } else {
    setPixel(0, 255, 0);  // Green: door closed
  }
}
