#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// 1. REPLACE THIS with your teammate's Car MAC Address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

Adafruit_MPU6050 mpu;

// This structure must match the Receiver's structure
typedef struct struct_message {
    char command[20];
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

void setup() {
  Serial.begin(115200);
  
  // Set Wi-Fi to Station mode for ESP-NOW
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the peer (The Car)
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // Init MPU6050 (Double check your pins: 4,5 or 8,9?)
  Wire.begin(8, 9); 
  if (!mpu.begin()) {
    Serial.println("MPU6050 missing!");
    //while (1) delay(10);
  }
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float x = a.acceleration.x;
  float y = a.acceleration.y;
  float threshold = 3.5; 

  // 8-Way Directional Logic
  if (y > threshold && x < -threshold)      strcpy(myData.command, "FL");
  else if (y > threshold && x > threshold)  strcpy(myData.command, "FR");
  else if (y < -threshold && x < -threshold) strcpy(myData.command, "BL");
  else if (y < -threshold && x > threshold)  strcpy(myData.command, "BR");
  else if (y > threshold)                   strcpy(myData.command, "FO");
  else if (y < -threshold)                  strcpy(myData.command, "BA");
  else if (x > threshold)                   strcpy(myData.command, "RI");
  else if (x < -threshold)                  strcpy(myData.command, "LE");
  else                                      strcpy(myData.command, "STOP");

  // Send the command wirelessly
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  
  Serial.print("Sending: ");
  Serial.println(myData.command);
  
  delay(500); // Fast 10Hz update rate for smooth driving
}