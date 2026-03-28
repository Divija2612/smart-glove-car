#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define F 1
#define FR 2
#define R 3
#define BR 4
#define B 5
#define BL 6
#define L 7
#define FL 8
#define S 9

// --- WiFi Settings ---
const char* ssid = "RITHOBRATHO_CAR";      
const char* password = "password123";  
const char* carIP = "192.168.4.1";     
const int udpPort = 4210;              

WiFiUDP udp;
Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  delay(1000); // Give hardware time to stabilize

  // 1. MUST INITIALIZE SENSOR FIRST (Pins 8, 9)
  Wire.begin(8, 9); 
  if (mpu.begin()) {
    Serial.println("MPU6050 missing! Check pins 8 and 9.");
    while (1) delay(10);
  }
  Serial.println("MPU6050 Found!");

  // 2. Set ranges (Standard for smooth driving)
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // 3. NOW Start WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Car WiFi!");
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float x = a.acceleration.x;
  float y = a.acceleration.y;
  float threshold = 3.5; 
  int command = S; // Default to Stop

  // 8-Way Directional Logic
  if (y > threshold && x < -threshold)      command = FL;
  else if (y > threshold && x > threshold)  command = FR;
  else if (y < -threshold && x < -threshold) command = BL;
  else if (y < -threshold && x > threshold)  command = BR;
  else if (y > threshold)                   command = F;
  else if (y < -threshold)                  command = B;
  else if (x > threshold)                   command = R;
  else if (x < -threshold)                  command = L;
  else                                      command = S;

  // 4. Send the command via UDP
  if (WiFi.status() == WL_CONNECTED) {
    udp.beginPacket(carIP, udpPort);
    udp.write(command);
    udp.endPacket();
    
    Serial.print("Sent: ");
    Serial.println(command);
  } else {
    Serial.println("WiFi Lost! Reconnecting...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  }
  delay(1000);
}
