#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

// ==== Wi-Fi Credentials (your ESP32 is in AP mode) ====
const char* ssid = "ESP32_AP";
const char* password = "12345678";

// ==== UDP Settings ====
WiFiUDP udp;
const int UDP_PORT = 4210;
char incomingPacket[255];

// ==== Servo Setup ====
Servo servoX;  // horizontal
Servo servoY;  // vertical
const int servoXPin = 26;
const int servoYPin = 27;

// ==== Mode Handling ====
enum ControlMode { AUTO, MANUAL };
ControlMode currentMode = AUTO;

void setup() {
  Serial.begin(115200);

  // Start Wi-Fi in AP mode
  WiFi.softAP(ssid, password);
  Serial.println("ESP32 started in Access Point mode");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Bind UDP
  udp.begin(UDP_PORT);
  Serial.printf("📡 Listening for UDP packets on port %d...\n", UDP_PORT);

  // Attach servos
  servoX.attach(servoXPin);
  servoY.attach(servoYPin);
  servoX.write(90);  // center
  servoY.write(90);  // center
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    int len = udp.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = '\0';  // Null-terminate

      Serial.print("📥 Received: ");
      Serial.println(incomingPacket);

      // Check for mode change
      if (strcmp(incomingPacket, "MODE:AUTO") == 0) {
        currentMode = AUTO;
        Serial.println(" Switched to AUTO mode (OpenCV)");
      } else if (strcmp(incomingPacket, "MODE:MANUAL") == 0) {
        currentMode = MANUAL;
        Serial.println(" Switched to MANUAL mode (MPU6050)");
      }

      // If it's coordinates and mode is correct
      else {
        int x = -1, y = -1;
        if (sscanf(incomingPacket, "%d,%d", &x, &y) == 2) {
          // Clamp and map
          x = constrain(x, 0, 100);
          y = constrain(y, 0, 100);
          int angleX = map(x, 0, 100, 00, 180);
          int angleY = map(y, 0, 100, 30, 120);

          // Only move if sender is in charge of current mode
          if (currentMode == AUTO) {
            Serial.println(" AUTO: Moving servos from OpenCV input");
            servoX.write(angleX);
            servoY.write(angleY);
          } else if (currentMode == MANUAL) {
            Serial.println("🎮 MANUAL: Moving servos from MPU6050 input");
            servoX.write(angleX);
            servoY.write(angleY);
          }

          Serial.printf("→ Servo X: %d°, Y: %d°\n", angleX, angleY);
        } else {
          Serial.println("⚠️ Invalid data. Use 'x,y' or 'MODE:...'");
        }
      }
    }
  }
}
