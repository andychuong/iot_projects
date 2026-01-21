/*
 * PIR Motion Sensor
 *
 * WiFi-enabled room occupancy sensor using ESP32 Feather V2
 * and a PIR motion sensor.
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

#define PIR_SENSOR_PIN 14
#define NEOPIXEL_PIN 0
#define NEOPIXEL_POWER 2

// Time before room is considered unoccupied (5 minutes)
#define OCCUPANCY_TIMEOUT_MS 300000

const char* ssid = SECRET_WIFI_SSID;
const char* password = SECRET_WIFI_PASS;

WebServer server(80);
Adafruit_NeoPixel pixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Motion state tracking
bool motionDetected = false;
bool roomOccupied = false;
unsigned long lastMotionTime = 0;
unsigned long motionCount = 0;

void setPixel(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void handleRoot() {
  String status = roomOccupied ? "OCCUPIED" : "EMPTY";
  String statusColor = roomOccupied ? "#e74c3c" : "#27ae60";
  unsigned long timeSinceMotion = (millis() - lastMotionTime) / 1000;

  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta http-equiv='refresh' content='2'>
  <title>Room Occupancy</title>
  <style>
    body { font-family: Arial; text-align: center; padding: 50px; background: #1a1a2e; color: #eee; }
    .status { font-size: 3em; margin: 30px; padding: 30px; border-radius: 15px; }
    .info { font-size: 1.2em; margin: 15px; padding: 15px; background: #16213e; border-radius: 10px; }
    .label { color: #888; font-size: 0.7em; }
  </style>
</head>
<body>
  <h1>Room Occupancy Sensor</h1>
  <div class='status' style='background: )" + statusColor + R"(;'>
    )" + status + R"(
  </div>
  <div class='info'>
    <div class='label'>Time Since Last Motion</div>
    )" + String(timeSinceMotion) + R"( seconds
  </div>
  <div class='info'>
    <div class='label'>Motion Events Today</div>
    )" + String(motionCount) + R"(
  </div>
</body>
</html>
)";

  server.send(200, "text/html", html);
}

void handleApi() {
  unsigned long timeSinceMotion = (millis() - lastMotionTime) / 1000;

  String json = "{";
  json += "\"motion_detected\":" + String(motionDetected ? "true" : "false") + ",";
  json += "\"room_occupied\":" + String(roomOccupied ? "true" : "false") + ",";
  json += "\"status\":\"" + String(roomOccupied ? "occupied" : "empty") + "\",";
  json += "\"seconds_since_motion\":" + String(timeSinceMotion) + ",";
  json += "\"motion_count\":" + String(motionCount) + ",";
  json += "\"uptime_ms\":" + String(millis());
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  // PIR sensor input
  pinMode(PIR_SENSOR_PIN, INPUT);

  // NeoPixel setup
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);
  pixel.begin();
  pixel.setBrightness(20);
  setPixel(255, 0, 0);

  // WiFi
  IPAddress staticIP(10, 0, 0, 87);
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
  ArduinoOTA.setHostname("pir-sensor");
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

  // Allow PIR sensor to stabilize (typically 30-60 seconds)
  Serial.println("Waiting for PIR sensor to stabilize...");
  delay(2000);

  lastMotionTime = millis();

  Serial.println("PIR sensor ready");
  updateStatusLED();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  // Check PIR sensor
  bool currentMotion = (digitalRead(PIR_SENSOR_PIN) == HIGH);

  if (currentMotion && !motionDetected) {
    // New motion detected
    motionDetected = true;
    lastMotionTime = millis();
    motionCount++;

    if (!roomOccupied) {
      roomOccupied = true;
      Serial.println("Room OCCUPIED - motion detected");
    } else {
      Serial.println("Motion detected");
    }

    updateStatusLED();
  } else if (!currentMotion && motionDetected) {
    // Motion ended
    motionDetected = false;
    Serial.println("Motion ended");
  }

  // Check occupancy timeout
  if (roomOccupied && (millis() - lastMotionTime > OCCUPANCY_TIMEOUT_MS)) {
    roomOccupied = false;
    Serial.println("Room EMPTY - no motion for timeout period");
    updateStatusLED();
  }

  delay(100);
}

void updateStatusLED() {
  if (WiFi.status() != WL_CONNECTED) {
    setPixel(255, 0, 0);  // Red: no WiFi
  } else if (roomOccupied) {
    setPixel(255, 165, 0);  // Orange: room occupied
  } else {
    setPixel(0, 255, 0);  // Green: room empty
  }
}
