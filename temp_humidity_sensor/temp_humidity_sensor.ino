#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>

#define DHTPIN 14
#define DHTTYPE DHT11
#define NEOPIXEL_PIN 0
#define NEOPIXEL_POWER 2

const char* ssid = "wifi";
const char* password = "pw";

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);
Adafruit_NeoPixel pixel(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setPixel(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void handleRoot() {
  float humidity = dht.readHumidity();
  float tempC = dht.readTemperature();
  float tempF = dht.readTemperature(true);

  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta http-equiv='refresh' content='5'>
  <title>DHT11 Sensor</title>
  <style>
    body { font-family: Arial; text-align: center; padding: 50px; background: #1a1a2e; color: #eee; }
    .reading { font-size: 2em; margin: 20px; padding: 20px; background: #16213e; border-radius: 10px; }
    .label { color: #888; font-size: 0.6em; }
  </style>
</head>
<body>
  <h1>DHT11 Sensor Data</h1>
  <div class='reading'>
    <div class='label'>Temperature</div>
    )" + String(tempF, 1) + R"(°F / )" + String(tempC, 1) + R"(°C
  </div>
  <div class='reading'>
    <div class='label'>Humidity</div>
    )" + String(humidity, 1) + R"(%
  </div>
</body>
</html>
)";

  server.send(200, "text/html", html);
}

void handleApi() {
  float humidity = dht.readHumidity();
  float tempC = dht.readTemperature();
  float tempF = dht.readTemperature(true);

  String json = "{";
  json += "\"temperature_f\":" + String(tempF, 1) + ",";
  json += "\"temperature_c\":" + String(tempC, 1) + ",";
  json += "\"humidity\":" + String(humidity, 1) + ",";
  json += "\"uptime_ms\":" + String(millis());
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);
  
  pixel.begin();
  pixel.setBrightness(20);
  setPixel(255, 0, 0);

  IPAddress staticIP(10, 0, 0, 85);
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

  ArduinoOTA.setHostname("dht11-sensor");
  ArduinoOTA.setPassword("update123");
  
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

  server.on("/", handleRoot);
  server.on("/api", handleApi);
  server.begin();
  
  setPixel(0, 255, 0);
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  
  if (WiFi.status() != WL_CONNECTED) {
    setPixel(255, 0, 0);
  } else {
    setPixel(0, 255, 0);
  }
}