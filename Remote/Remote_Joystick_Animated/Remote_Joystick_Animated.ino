#include <dummy.h>

#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =================== OLED PINS ===================
#define OLED_SDA 25
#define OLED_SCL 26

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// =================== JOYSTICK PINS ===================
#define VRX_PIN 34
#define VRY_PIN 35
#define JOY_SW 32     // Push button

// =================== WIFI SETTINGS ===================
const char* ssid = "ESP32_AP";
const char* password = "12345678";

WiFiUDP udp;
const int UDP_PORT = 4210;

// =================== MODE ===================
bool autoMode = false;
unsigned long lastSwitch = 0;

// =================== DRISHTI LOGOS ===================
const unsigned char PROGMEM tankBMP[] = {
  0x1C,0x3E,0x7F,0xFF,0xFF,0x7F,0x3E,0x1C
};

// ------- Show DRISHTI text ------
void showDrishtiLogo() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.print("DRISHTI");

  display.setTextSize(1);
  display.setCursor(22, 35);
  display.print("REMOTE SYSTEM");

  display.display();
  delay(1200);
}

// ------- Tank Animation ------
void tankAnimation() {
  for (int x = 0; x < 110; x += 6) {
    display.clearDisplay();
    display.drawBitmap(x, 40, tankBMP, 8, 8, WHITE);
    display.display();
    delay(50);
  }
}

// ------- Laser Animation ------
void laserAnimation() {
  int x = 64;
  for (int y = 50; y > 5; y -= 6) {
    display.clearDisplay();
    display.drawBitmap(60, 55, tankBMP, 8, 8, WHITE);
    display.drawLine(x, 50, x, y, WHITE);
    display.display();
    delay(40);
  }
  delay(200);
}

void setup() {
  Serial.begin(115200);

  pinMode(JOY_SW, INPUT_PULLUP);

  // OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED FAILED");
    while (1);
  }

  // Startup animation
  showDrishtiLogo();
  tankAnimation();
  laserAnimation();

  // WiFi connect to turret AP
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to Turret AP");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  udp.begin(UDP_PORT);
}

void loop() {
  int xVal = analogRead(VRX_PIN);
  int yVal = analogRead(VRY_PIN);
  int swVal = digitalRead(JOY_SW);

  // Toggle Auto / Manual
  if (swVal == LOW && millis() - lastSwitch > 350) {
    autoMode = !autoMode;
    lastSwitch = millis();

    if (autoMode)
      udp.beginPacket("192.168.4.1", UDP_PORT), udp.print("MODE:AUTO"), udp.endPacket();
    else
      udp.beginPacket("192.168.4.1", UDP_PORT), udp.print("MODE:MANUAL"), udp.endPacket();
  }

  // Convert joystick ADC → 0–100
  int xMapped = map(xVal, 0, 4095, 0, 100);
  int yMapped = map(yVal, 0, 4095, 0, 100);

  // Send joystick values
  udp.beginPacket("192.168.4.1", UDP_PORT);
  udp.printf("%d,%d", xMapped, yMapped);
  udp.endPacket();

  // ================= SERIAL =================
  Serial.print("X:");
  Serial.print(xMapped);
  Serial.print(" | Y:");
  Serial.print(yMapped);
  Serial.print(" | Mode:");
  Serial.println(autoMode ? "AUTO" : "MANUAL");

  // ================= OLED DISPLAY =================
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("DRISHTI Remote");

  display.setCursor(0, 15);
  display.print("Mode: ");
  display.print(autoMode ? "AUTO" : "MANUAL");

  // direction
  String dir = "CENTER";
  if (xMapped < 30) dir = "LEFT";
  else if (xMapped > 70) dir = "RIGHT";
  if (yMapped < 30) dir += " UP";
  else if (yMapped > 70) dir += " DOWN";

  display.setCursor(0, 30);
  display.print("Joy: ");
  display.print(dir);

  display.setCursor(0, 50);
  display.print("X:");
  display.print(xMapped);
  display.print(" Y:");
  display.print(yMapped);

  display.display();
}
