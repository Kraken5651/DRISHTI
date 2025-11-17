#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// === WiFi Settings ===
const char* ssid = "ESP32_AP";
const char* password = "12345678";

// === UDP Settings ===
WiFiUDP udp;
const char* receiverIP = "192.168.4.1";  // ESP32 receiver IP
const int receiverPort = 4210;

// === MPU6050 Setup ===
MPU6050 mpu(Wire);
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 333;  // milliseconds

// === Button Setup ===
const int buttonPin = 4;
bool currentModeManual = true;
bool lastButtonStableState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;  // milliseconds

// === OLED Setup ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// === WiFi Connect Function ===
void connectToWiFi() {
  Serial.print("🔌 Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected!");
}

// === Show Logo Function ===
void showLogo() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(INVERSE);
  display.setCursor(0, 0);
  display.print("DRISHTI");

  display.setTextSize(1);
  display.setCursor(0, 18);
  display.print("Remote Aiming System");

  display.display();
  delay(5000);  // Show logo for 5 seconds
  display.clearDisplay();
  display.display();
}

// === OLED Update Function ===
void updateOLED(String mode, int x, int y) {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("MODE: ");
  display.println(mode);

  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print("X: ");
  display.println(x);
  display.setCursor(0, 40);
  display.print("Y: ");
  display.println(y);

  display.display();
}

// === Send Mode via UDP ===
void sendMode(const char* modeStr) {
  udp.beginPacket(receiverIP, receiverPort);
  udp.write((const uint8_t*)modeStr, strlen(modeStr));
  udp.endPacket();
}

void setup() {
  Serial.begin(115200);

  // Button
  pinMode(buttonPin, INPUT_PULLUP);

  // I2C and OLED
  Wire.begin(21, 22);  // SDA, SCL pins for your ESP32
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED not found!");
    while (true);
  }
  display.clearDisplay();
  display.display();

  showLogo();

  // MPU6050 init
  mpu.begin();
  delay(250);
  mpu.calcOffsets(true, true);

  connectToWiFi();

  udp.begin(WiFi.localIP(), receiverPort);
}

void loop() {
  mpu.update();

  // Button debounce
  bool reading = digitalRead(buttonPin);
  if (reading != lastButtonStableState && (millis() - lastDebounceTime > debounceDelay)) {
    lastDebounceTime = millis();
    if (reading == LOW) {
      currentModeManual = !currentModeManual;
      if (currentModeManual) {
        sendMode("MODE:MANUAL");
        Serial.println("🕹 Switched to MANUAL mode");
      } else {
        sendMode("MODE:AUTO");
        Serial.println("🎯 Switched to AUTO mode");
      }
    }
    lastButtonStableState = reading;
  }

  // Read MPU6050 angles (pitch = X, roll = Y)
  float pitch = mpu.getAngleX();  // Up/down tilt
  float roll = mpu.getAngleY();   // Left/right tilt

  // Optional: smooth values with simple low-pass filter
  static float smoothPitch = pitch;
  static float smoothRoll = roll;
  float smoothingFactor = 0.1;
  smoothPitch = smoothPitch * (1 - smoothingFactor) + pitch * smoothingFactor;
  smoothRoll = smoothRoll * (1 - smoothingFactor) + roll * smoothingFactor;

  // Constrain MPU angles to your desired physical limits
  float constrainedRoll = constrain(smoothRoll, -45, 45);   // horizontal tilt
  float constrainedPitch = constrain(smoothPitch, -45, 45); // vertical tilt

  // Map constrained angles to servo range 0 - 180 degrees
  int servoX = map(constrainedRoll, -45, 45, 0, 180);
  int servoY = map(constrainedPitch, -45, 45, 0, 180);

  // Map constrained angles to OLED scale 0 - 100 for display clarity
  int oledX = map(constrainedRoll, -45, 45, 0, 100);
  int oledY = map(constrainedPitch, -45, 45, 0, 100);

  // Update OLED
  updateOLED(currentModeManual ? "MANUAL" : "AUTO", oledX, oledY);

  // Send coordinates only in MANUAL mode and at intervals
  if (currentModeManual && millis() - lastSendTime > sendInterval) {
    lastSendTime = millis();

    char msg[20];
    sprintf(msg, "%d,%d", servoX, servoY);

    udp.beginPacket(receiverIP, receiverPort);
    udp.write((const uint8_t*)msg, strlen(msg));
    udp.endPacket();

    Serial.printf("📤 Sent coords: %s\n", msg);
  }
}
