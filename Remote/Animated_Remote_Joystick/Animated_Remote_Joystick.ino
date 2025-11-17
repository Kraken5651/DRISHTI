#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =================== OLED ===================
#define OLED_SDA 25
#define OLED_SCL 26
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// =================== JOYSTICK ===================
#define VRX_PIN 34
#define VRY_PIN 35
#define JOY_SW 32

// =================== BUTTON ===================
#define REMOTE_SW 22

// =================== BUZZER ===================
#define BUZZER 23

// =================== WIFI ===================
const char* ssid = "ESP32_AP";
const char* password = "12345678";

WiFiUDP udp;
const int UDP_PORT = 4210;

bool remoteON = false;
bool autoMode = false;

unsigned long lastButtonPress = 0;
unsigned long lastModePress = 0;

// ===== Smooth ADC =====
int smoothRead(int pin) {
  long sum = 0;
  for (int i = 0; i < 6; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  return sum / 6;
}

int applyDeadzone(int value, int center, int threshold = 4) {
  if (abs(value - center) <= threshold) return center;
  return value;
}

// ===== BUZZER =====
void toneBeep(int t) {
  digitalWrite(BUZZER, HIGH);
  delay(t);
  digitalWrite(BUZZER, LOW);
}

void beepStartup() {
  toneBeep(200);
}

void beepRemoteON() {
  toneBeep(140);
}

void beepRemoteOFF() {
  toneBeep(140);
  delay(80);
  toneBeep(140);
}

void beepAuto() {
  toneBeep(170);
}

void beepManual() {
  toneBeep(170);
  delay(100);
  toneBeep(170);
}

// ===== INTRO =====
void showIntro() {
  display.clearDisplay();
  display.setTextColor(WHITE);

  // Title + Subtitle
  display.setTextSize(2);
  display.setCursor(10, 10);
  display.print("DRISHTI");

  display.setTextSize(1);
  display.setCursor(22, 45);
  display.print("Remote  Control");

  display.display();
  beepStartup();
  delay(5000);   // Show for 5 seconds

  // ===== Connecting Animation =====
  const char spin[4] = {'|', '/', '-', '\\'};
  int dotCount = 0;

  for (int i = 0; i < 15; i++) {
    display.clearDisplay();

    display.setTextSize(2);
    display.setCursor(20, 8);
    display.print("DRISHTI");

    display.setTextSize(1);
    display.setCursor(10, 40);
    display.print("Connecting ");

    // Dots animation
    for (int j = 0; j < dotCount; j++) {
      display.print(".");
    }

    // Spinner
    display.setCursor(105, 40);
    display.print(spin[i % 4]);

    display.display();
    delay(180);

    dotCount = (dotCount + 1) % 4;
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  pinMode(JOY_SW, INPUT_PULLUP);
  pinMode(REMOTE_SW, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  showIntro();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(150);
  }

  udp.begin(UDP_PORT);
}

// ===== LOOP =====
void loop() {

  // ===== Remote ON/OFF =====
  if (digitalRead(REMOTE_SW) == LOW && millis() - lastButtonPress > 350) {
    remoteON = !remoteON;
    lastButtonPress = millis();
    if (remoteON) beepRemoteON();
    else beepRemoteOFF();
  }

  // ===== Mode Switch =====
  if (digitalRead(JOY_SW) == LOW && millis() - lastModePress > 350) {
    autoMode = !autoMode;
    lastModePress = millis();
    if (autoMode) beepAuto();
    else beepManual();
  }

  // ===== Joystick Read =====
  int xVal = smoothRead(VRX_PIN);
  int yVal = smoothRead(VRY_PIN);

  int X = map(xVal, 0, 4095, 0, 100);
  int Y = map(yVal, 0, 4095, 0, 100);

  X = applyDeadzone(X, 50, 6);
  Y = applyDeadzone(Y, 50, 6);

  Serial.print("X: ");
  Serial.print(X);
  Serial.print("   Y: ");
  Serial.println(Y);

  if (remoteON) {
    udp.beginPacket("192.168.4.1", UDP_PORT);
    udp.printf("%d,%d", X, Y);
    udp.endPacket();
  }

  // ===== DISPLAY =====
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(14, 0);
  display.print("DRISHTI");

  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print("Remote: ");
  display.print(remoteON ? "ON" : "OFF");

  display.setCursor(0, 32);
  display.print("Mode: ");
  display.print(autoMode ? "AUTO" : "MANUAL");

  display.setCursor(0, 45);
  display.print("X:");
  display.print(X);
  display.print("  Y:");
  display.print(Y);

  display.display();
}
