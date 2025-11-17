#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Pins
#define OLED_SDA 25
#define OLED_SCL 26

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Joystick Pins
#define VRX_PIN 34
#define VRY_PIN 35
#define JOY_SW 32

// Modes
bool autoMode = false;
unsigned long lastPress = 0;

// Data structure
typedef struct struct_message {
  int x;
  int y;
  bool autoMode;
} struct_message;

struct_message controlData;

// Receiver MAC
uint8_t receiverAddress[] = {0x24, 0x0A, 0xC4, 0x11, 0x22, 0x33};

// ✅ ESP-NOW CALLBACK
void OnSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);

  pinMode(JOY_SW, INPUT_PULLUP);

  // INIT I2C for OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("Remote Booting...");
  display.display();
  delay(600);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init fail");
    ESP.restart();
  }

  esp_now_register_send_cb(OnSent);

  esp_now_peer_info_t peerInfo{};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
  }
}

void loop() {

  int xVal = analogRead(VRX_PIN);
  int yVal = analogRead(VRY_PIN);
  int swVal = digitalRead(JOY_SW);

  // Toggle auto/manual
  if (swVal == LOW && millis() - lastPress > 400) {
    autoMode = !autoMode;
    lastPress = millis();
  }

  // Send data
  controlData.x = xVal;
  controlData.y = yVal;
  controlData.autoMode = autoMode;

  esp_now_send(receiverAddress, (uint8_t*)&controlData, sizeof(controlData));

  // SERIAL OUTPUT
  Serial.print("X: "); Serial.print(xVal);
  Serial.print(" | Y: "); Serial.print(yVal);
  Serial.print(" | Mode: ");
  Serial.println(autoMode ? "AUTO" : "MANUAL");

  // Direction
  String direction = "";

  if (xVal < 1400) direction = "LEFT";
  else if (xVal > 2700) direction = "RIGHT";

  if (yVal < 1400) direction += " UP";
  else if (yVal > 2700) direction += " DOWN";

  if (direction == "") direction = "CENTER";

  // OLED Display
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Turret Remote");

  display.setCursor(0, 15);
  display.print("Mode: ");
  display.print(autoMode ? "AUTO" : "MANUAL");

  display.setCursor(0, 30);
  display.print("Joystick: ");
  display.print(direction);

  display.setCursor(0, 50);
  display.print("X:");
  display.print(xVal);
  display.print("  Y:");
  display.print(yVal);

  display.display();

  delay(40);
}
