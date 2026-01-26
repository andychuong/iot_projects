/*
 * Pomodoro Timer
 *
 * WiFi-enabled Pomodoro timer using ESP32 Feather V2 with
 * LCD display, NeoPixel progress bar, and web dashboard.
 *
 * Endpoints:
 *   /      - Web dashboard
 *   /api   - JSON API
 *   /start - Start timer
 *   /pause - Pause timer
 *   /reset - Reset timer
 *   /mode  - Set mode (?m=work|short_break|long_break)
 */

#include "arduino_secrets.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>

// Pin definitions
#define BTN_START_PAUSE 32
#define BTN_RESET       33
#define BTN_MODE        27
#define BUZZER_PIN      26
#define LED_WORK        12
#define LED_BREAK       14
#define NEOPIXEL_PIN    15
#define NEOPIXEL_COUNT  8

// Timer durations (in seconds)
#define WORK_DURATION       (25 * 60)  // 25 minutes
#define SHORT_BREAK_DURATION (5 * 60)  // 5 minutes
#define LONG_BREAK_DURATION (15 * 60)  // 15 minutes
#define CYCLES_BEFORE_LONG  4          // Long break after 4 work sessions

// Timer modes
enum TimerMode {
  MODE_WORK,
  MODE_SHORT_BREAK,
  MODE_LONG_BREAK
};

const char* ssid = SECRET_WIFI_SSID;
const char* password = SECRET_WIFI_PASS;

WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address 0x27, 16 cols, 2 rows
Adafruit_NeoPixel strip(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Timer state
TimerMode currentMode = MODE_WORK;
bool timerRunning = false;
unsigned long timerStartMillis = 0;
unsigned long timerPausedAt = 0;
unsigned long timeRemainingSec = WORK_DURATION;
int currentCycle = 1;
int workSessionsCompleted = 0;

// Button debouncing
unsigned long lastButtonPress = 0;
#define DEBOUNCE_MS 200

// Display update throttling
unsigned long lastDisplayUpdate = 0;
#define DISPLAY_UPDATE_MS 250

void setup() {
  Serial.begin(115200);

  // Button inputs
  pinMode(BTN_START_PAUSE, INPUT);
  pinMode(BTN_RESET, INPUT);
  pinMode(BTN_MODE, INPUT);

  // Outputs
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_WORK, OUTPUT);
  pinMode(LED_BREAK, OUTPUT);

  // Initialize LCD
  Wire.begin(23, 22);  // SDA, SCL
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Pomodoro Timer");
  lcd.setCursor(0, 1);
  lcd.print("Connecting...");

  // Initialize NeoPixel strip
  strip.begin();
  strip.setBrightness(50);
  strip.show();

  // WiFi
  IPAddress staticIP(10, 0, 0, 88);
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
  ArduinoOTA.setHostname("pomodoro-timer");
  ArduinoOTA.setPassword(SECRET_OTA_PASS);

  ArduinoOTA.onStart([]() {
    lcd.clear();
    lcd.print("OTA Update...");
  });
  ArduinoOTA.onEnd([]() {
    lcd.clear();
    lcd.print("Update complete!");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    lcd.clear();
    lcd.print("OTA Error!");
  });

  ArduinoOTA.begin();

  // Web server routes
  server.on("/", handleRoot);
  server.on("/api", HTTP_GET, handleApi);
  server.on("/start", HTTP_POST, handleStart);
  server.on("/pause", HTTP_POST, handlePause);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/mode", HTTP_POST, handleSetMode);
  server.begin();

  // Initial state
  updateDisplay();
  updateLEDs();
  updateProgressBar();

  Serial.println("Pomodoro Timer ready");
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  // Handle buttons
  handleButtons();

  // Update timer if running
  if (timerRunning) {
    unsigned long elapsed = (millis() - timerStartMillis) / 1000;
    unsigned long totalDuration = getDurationForMode(currentMode);

    if (elapsed >= totalDuration) {
      // Timer completed
      timerComplete();
    } else {
      timeRemainingSec = totalDuration - elapsed;
    }
  }

  // Update display periodically
  if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_MS) {
    updateDisplay();
    updateProgressBar();
    lastDisplayUpdate = millis();
  }

  delay(10);
}

void handleButtons() {
  if (millis() - lastButtonPress < DEBOUNCE_MS) return;

  if (digitalRead(BTN_START_PAUSE) == HIGH) {
    lastButtonPress = millis();
    if (timerRunning) {
      pauseTimer();
    } else {
      startTimer();
    }
  }

  if (digitalRead(BTN_RESET) == HIGH) {
    lastButtonPress = millis();
    resetTimer();
  }

  if (digitalRead(BTN_MODE) == HIGH) {
    lastButtonPress = millis();
    cycleMode();
  }
}

void startTimer() {
  if (!timerRunning) {
    timerRunning = true;
    timerStartMillis = millis() - ((getDurationForMode(currentMode) - timeRemainingSec) * 1000);
    Serial.println("Timer started");
    updateLEDs();
  }
}

void pauseTimer() {
  if (timerRunning) {
    timerRunning = false;
    timerPausedAt = millis();
    Serial.println("Timer paused");
  }
}

void resetTimer() {
  timerRunning = false;
  timeRemainingSec = getDurationForMode(currentMode);
  timerStartMillis = 0;
  Serial.println("Timer reset");
  updateLEDs();
  updateDisplay();
  updateProgressBar();
}

void cycleMode() {
  timerRunning = false;

  switch (currentMode) {
    case MODE_WORK:
      currentMode = MODE_SHORT_BREAK;
      break;
    case MODE_SHORT_BREAK:
      currentMode = MODE_LONG_BREAK;
      break;
    case MODE_LONG_BREAK:
      currentMode = MODE_WORK;
      break;
  }

  timeRemainingSec = getDurationForMode(currentMode);
  Serial.print("Mode changed to: ");
  Serial.println(getModeString(currentMode));
  updateLEDs();
  updateDisplay();
  updateProgressBar();
}

void timerComplete() {
  timerRunning = false;

  // Play alarm
  playAlarm();

  // Update statistics and auto-advance
  if (currentMode == MODE_WORK) {
    workSessionsCompleted++;

    if (currentCycle >= CYCLES_BEFORE_LONG) {
      currentMode = MODE_LONG_BREAK;
      currentCycle = 1;
    } else {
      currentMode = MODE_SHORT_BREAK;
      currentCycle++;
    }
  } else {
    // After any break, go back to work
    currentMode = MODE_WORK;
  }

  timeRemainingSec = getDurationForMode(currentMode);
  updateLEDs();
  updateDisplay();
  updateProgressBar();

  Serial.print("Timer complete! Next: ");
  Serial.println(getModeString(currentMode));
}

void playAlarm() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 1000, 200);
    delay(300);
    tone(BUZZER_PIN, 1500, 200);
    delay(300);
  }
  noTone(BUZZER_PIN);
}

unsigned long getDurationForMode(TimerMode mode) {
  switch (mode) {
    case MODE_WORK:        return WORK_DURATION;
    case MODE_SHORT_BREAK: return SHORT_BREAK_DURATION;
    case MODE_LONG_BREAK:  return LONG_BREAK_DURATION;
    default:               return WORK_DURATION;
  }
}

const char* getModeString(TimerMode mode) {
  switch (mode) {
    case MODE_WORK:        return "work";
    case MODE_SHORT_BREAK: return "short_break";
    case MODE_LONG_BREAK:  return "long_break";
    default:               return "unknown";
  }
}

const char* getModeDisplayString(TimerMode mode) {
  switch (mode) {
    case MODE_WORK:        return "WORK";
    case MODE_SHORT_BREAK: return "SHORT BREAK";
    case MODE_LONG_BREAK:  return "LONG BREAK";
    default:               return "???";
  }
}

void updateDisplay() {
  unsigned int minutes = timeRemainingSec / 60;
  unsigned int seconds = timeRemainingSec % 60;

  // Line 1: Mode and status
  lcd.setCursor(0, 0);
  lcd.print("                ");  // Clear line
  lcd.setCursor(0, 0);
  lcd.print(getModeDisplayString(currentMode));

  if (!timerRunning && timeRemainingSec < getDurationForMode(currentMode)) {
    lcd.print(" [P]");  // Paused indicator
  }

  // Line 2: Time and cycle
  lcd.setCursor(0, 1);
  lcd.print("                ");  // Clear line
  lcd.setCursor(0, 1);

  char timeStr[12];
  sprintf(timeStr, "%02d:%02d", minutes, seconds);
  lcd.print(timeStr);

  lcd.setCursor(10, 1);
  lcd.print("C:");
  lcd.print(currentCycle);
  lcd.print("/");
  lcd.print(CYCLES_BEFORE_LONG);
}

void updateLEDs() {
  if (currentMode == MODE_WORK) {
    digitalWrite(LED_WORK, timerRunning ? HIGH : LOW);
    digitalWrite(LED_BREAK, LOW);
  } else {
    digitalWrite(LED_WORK, LOW);
    digitalWrite(LED_BREAK, timerRunning ? HIGH : LOW);
  }
}

void updateProgressBar() {
  unsigned long totalDuration = getDurationForMode(currentMode);
  float progress = 1.0 - ((float)timeRemainingSec / (float)totalDuration);
  int ledsToLight = (int)(progress * NEOPIXEL_COUNT);

  // Color based on mode
  uint32_t color;
  if (currentMode == MODE_WORK) {
    color = strip.Color(255, 50, 0);   // Orange-red for work
  } else {
    color = strip.Color(0, 255, 50);   // Green for breaks
  }

  // Dim color for "empty" portion
  uint32_t dimColor = strip.Color(20, 20, 20);

  for (int i = 0; i < NEOPIXEL_COUNT; i++) {
    if (i < ledsToLight) {
      strip.setPixelColor(i, color);
    } else {
      strip.setPixelColor(i, dimColor);
    }
  }

  // Blink current LED if timer is running
  if (timerRunning && ledsToLight < NEOPIXEL_COUNT) {
    if ((millis() / 500) % 2 == 0) {
      strip.setPixelColor(ledsToLight, color);
    }
  }

  strip.show();
}

// Web handlers
void handleRoot() {
  unsigned int minutes = timeRemainingSec / 60;
  unsigned int seconds = timeRemainingSec % 60;
  float progress = 1.0 - ((float)timeRemainingSec / (float)getDurationForMode(currentMode));
  int progressPct = (int)(progress * 100);

  String statusColor = (currentMode == MODE_WORK) ? "#e74c3c" : "#27ae60";
  String statusText = timerRunning ? "Running" : "Paused";

  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <meta http-equiv='refresh' content='1'>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>Pomodoro Timer</title>
  <style>
    body { font-family: Arial; text-align: center; padding: 20px; background: #1a1a2e; color: #eee; }
    .timer { font-size: 4em; margin: 20px; font-family: monospace; }
    .mode { font-size: 1.5em; padding: 15px 30px; border-radius: 10px; display: inline-block; margin: 10px; }
    .progress-bar { width: 80%; max-width: 400px; height: 30px; background: #333; border-radius: 15px; margin: 20px auto; overflow: hidden; }
    .progress-fill { height: 100%; transition: width 0.3s; }
    .info { font-size: 1.1em; margin: 10px; padding: 10px; background: #16213e; border-radius: 8px; display: inline-block; }
    .btn { padding: 15px 30px; font-size: 1.2em; margin: 5px; border: none; border-radius: 8px; cursor: pointer; }
    .btn-start { background: #27ae60; color: white; }
    .btn-pause { background: #f39c12; color: white; }
    .btn-reset { background: #e74c3c; color: white; }
    .controls { margin: 20px; }
  </style>
</head>
<body>
  <h1>Pomodoro Timer</h1>
  <div class='mode' style='background: )" + statusColor + R"(;'>
    )" + String(getModeDisplayString(currentMode)) + " - " + statusText + R"(
  </div>
  <div class='timer'>)" + String(minutes < 10 ? "0" : "") + String(minutes) + ":" + String(seconds < 10 ? "0" : "") + String(seconds) + R"(</div>
  <div class='progress-bar'>
    <div class='progress-fill' style='width: )" + String(progressPct) + R"(%; background: )" + statusColor + R"(;'></div>
  </div>
  <div class='controls'>
    <button class='btn btn-start' onclick="fetch('/start',{method:'POST'}).then(()=>location.reload())">Start</button>
    <button class='btn btn-pause' onclick="fetch('/pause',{method:'POST'}).then(()=>location.reload())">Pause</button>
    <button class='btn btn-reset' onclick="fetch('/reset',{method:'POST'}).then(()=>location.reload())">Reset</button>
  </div>
  <div>
    <div class='info'>Cycle: )" + String(currentCycle) + "/" + String(CYCLES_BEFORE_LONG) + R"(</div>
    <div class='info'>Sessions: )" + String(workSessionsCompleted) + R"(</div>
  </div>
</body>
</html>
)";

  server.send(200, "text/html", html);
}

void handleApi() {
  String json = "{";
  json += "\"mode\":\"" + String(getModeString(currentMode)) + "\",";
  json += "\"running\":" + String(timerRunning ? "true" : "false") + ",";
  json += "\"time_remaining_sec\":" + String(timeRemainingSec) + ",";
  json += "\"time_total_sec\":" + String(getDurationForMode(currentMode)) + ",";
  json += "\"cycle\":" + String(currentCycle) + ",";
  json += "\"total_cycles\":" + String(CYCLES_BEFORE_LONG) + ",";
  json += "\"work_sessions_completed\":" + String(workSessionsCompleted);
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleStart() {
  startTimer();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}

void handlePause() {
  pauseTimer();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}

void handleReset() {
  resetTimer();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}

void handleSetMode() {
  if (server.hasArg("m")) {
    String mode = server.arg("m");
    timerRunning = false;

    if (mode == "work") {
      currentMode = MODE_WORK;
    } else if (mode == "short_break") {
      currentMode = MODE_SHORT_BREAK;
    } else if (mode == "long_break") {
      currentMode = MODE_LONG_BREAK;
    }

    timeRemainingSec = getDurationForMode(currentMode);
    updateLEDs();
    updateDisplay();
    updateProgressBar();
  }

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}
