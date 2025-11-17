#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// === WiFi Settings ===
const char* ssid = "ESP32_AP";
const char* password = "12345678";

// === UDP Settings ===
WiFiUDP udp;
const char* receiverIP = "192.168.4.1";  // ESP32 receiver IP
const int receiverPort = 4210;

// === Joystick ===
const int VRx = 34;
const int VRy = 35;
const int SW  = 15;   // joystick push button (optional)

// === Mode Button ===
const int buttonPin = 4;
bool currentModeManual = true;
bool lastButtonStableState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

// === Status LED ===
const int ledPin = 2;

// === OLED Setup ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_SDA 25
#define OLED_SCL 26
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// === Send Interval ===
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 100;

// ------------------------------------------------------
//              WIFI CONNECT
// ------------------------------------------------------
void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi Connected!");
}

// ------------------------------------------------------
//              STARTUP LOGO
// ------------------------------------------------------
void showLogo() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(INVERSE);
  display.setCursor(0, 0);
  display.print("DRISHTI");
  display.setTextSize(1);
  display.setCursor(0, 18);
  display.print("Remote System");
  display.display();
  delay(2000);
  display.clearDisplay();
  display.display();
}

// ------------------------------------------------------
//              OLED UPDATE
// ------------------------------------------------------
void updateOLED(String mode, int x, int y) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("MODE: ");
  display.println(mode);

  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print("X:");
  display.println(x);

  display.setCursor(0, 40);
  display.print("Y:");
  display.println(y);

  display.display();
}

// ------------------------------------------------------
//              SEND MODE CHANGE
// ------------------------------------------------------
void sendMode(const char* modeStr) {
  udp.beginPacket(receiverIP, receiverPort);
  udp.write((const uint8_t*)modeStr, strlen(modeStr));
  udp.endPacket();
}

// ------------------------------------------------------
//              SETUP
// ------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(SW, INPUT_PULLUP);

  // Init I2C, OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED NOT FOUND!");
    while (true);
  }
  display.clearDisplay();
  display.display();

  showLogo();
  connectToWiFi();

  udp.begin(WiFi.localIP(), receiverPort);
  Serial.println("✅ UDP Ready");
}

// ------------------------------------------------------
//              MAIN LOOP
// ------------------------------------------------------
void loop() {

  // Toggle Mode Button
  bool reading = digitalRead(buttonPin);
  if (reading != lastButtonStableState
      && (millis() - lastDebounceTime > debounceDelay)) {

    lastDebounceTime = millis();
    if (reading == LOW) {
      currentModeManual = !currentModeManual;
      if (currentModeManual) {
        sendMode("MODE:MANUAL");
        Serial.println("🕹 Switched to MANUAL");
        digitalWrite(ledPin, HIGH);
      } else {
        sendMode("MODE:AUTO");
        Serial.println("🎯 Switched to AUTO");
        digitalWrite(ledPin, LOW);
      }
    }
    lastButtonStableState = reading;
  }

  // Read Joystick
  int rawX = analogRead(VRx);
  int rawY = analogRead(VRy);
  int sw   = digitalRead(SW);

  // Map to 0–100
  int joyX = map(rawX, 0, 4095, 0, 100);
  int joyY = map(rawY, 0, 4095, 0, 100);

  // Update OLED
  updateOLED(currentModeManual ? "MANUAL" : "AUTO", joyX, joyY);

  // Send only in manual mode
  if (currentModeManual && millis() - lastSendTime > sendInterval) {
    lastSendTime = millis();
    char msg[20];
    sprintf(msg, "%d,%d", joyX, joyY);

    udp.beginPacket(receiverIP, receiverPort);
    udp.write((const uint8_t*)msg, strlen(msg));
    udp.endPacket();

    Serial.printf("📤 Sent coords: %s\n", msg);
  }
}
void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
