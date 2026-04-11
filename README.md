# Smart Glove Car 🤚🚗

A gesture-controlled car built with an **ESP32-C3** microcontroller and **MPU6050** IMU. Hand tilt movements are captured in real-time, processed on the glove-side ESP32-C3, and transmitted wirelessly as directional commands to a receiver unit mounted on the car.

---

## Table of Contents

- [System Architecture](#system-architecture)
- [Hardware Components](#hardware-components)
- [Circuit Connections](#circuit-connections)
- [Gesture-to-Command Mapping](#gesture-to-command-mapping)
- [Software Stack](#software-stack)
- [Project Structure](#project-structure)
- [Setup & Flashing](#setup--flashing)
- [How It Works](#how-it-works)
- [Future Improvements](#future-improvements)

---

## System Architecture

```
┌─────────────────────────────┐          Wireless (ESP-NOW / Wi-Fi)          ┌──────────────────────────────┐
│        GLOVE UNIT           │ ─────────────────────────────────────────────▶│         CAR UNIT             │
│                             │                                               │                              │
│  MPU6050 (Accelerometer +   │                                               │  ESP32-C3 (Receiver)         │
│  Gyroscope)                 │                                               │  L298N / L293D Motor Driver  │
│       │                     │                                               │  DC Motors (x4 or x2)        │
│       ▼                     │                                               │  LiPo / 18650 Battery Pack   │
│  ESP32-C3 (Transmitter)     │                                               │                              │
│  USB-C / LiPo powered       │                                               │                              │
└─────────────────────────────┘                                               └──────────────────────────────┘
```

---

## Hardware Components

### Glove Unit (Transmitter)

| Component | Specification | Purpose |
|-----------|--------------|---------|
| ESP32-C3 | 160 MHz RISC-V, Wi-Fi/BLE | Main MCU + wireless transmitter |
| MPU6050 | 6-axis IMU (3-axis accel + 3-axis gyro), I2C | Motion / tilt sensing |
| LiPo Battery | 3.7V 500–1000 mAh | Portable power for glove |
| 3.3V LDO Regulator | AMS1117 or similar | Regulate battery voltage to 3.3V |

### Car Unit (Receiver)

| Component | Specification | Purpose |
|-----------|--------------|---------|
| ESP32-C3 | 160 MHz RISC-V, Wi-Fi/BLE | Wireless receiver + motor control |
| L298N Motor Driver | Dual H-Bridge, up to 2A per channel | Drive DC motors |
| DC Gear Motors | 3–6V, TT or N20 | Wheel propulsion |
| LiPo / 18650 Pack | 7.4V or 2S LiPo | Power motors and MCU |

---

## Circuit Connections

### Glove Side — MPU6050 → ESP32-C3

| MPU6050 Pin | ESP32-C3 Pin | Description |
|-------------|-------------|-------------|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| SDA | GPIO6 | I2C Data |
| SCL | GPIO7 | I2C Clock |
| AD0 | GND | I2C address = 0x68 |
| INT | GPIO4 (optional) | Data-ready interrupt |

### Car Side — ESP32-C3 → L298N

| ESP32-C3 Pin | L298N Pin | Description |
|-------------|----------|-------------|
| GPIO0 | IN1 | Motor A direction |
| GPIO1 | IN2 | Motor A direction |
| GPIO2 | IN3 | Motor B direction |
| GPIO3 | IN4 | Motor B direction |
| GPIO8 | ENA (PWM) | Motor A speed |
| GPIO9 | ENB (PWM) | Motor B speed |

> **Note:** L298N 12V input → motor battery. L298N 5V output can power ESP32-C3 via Vin if jumper is set. Verify current ratings.

---

## Gesture-to-Command Mapping

The MPU6050 outputs raw pitch and roll values which are thresholded into discrete commands.

| Gesture | Pitch / Roll Threshold | Car Action |
|---------|----------------------|------------|
| Tilt Forward | Pitch < -20° | Move Forward |
| Tilt Backward | Pitch > +20° | Move Backward |
| Tilt Left | Roll < -20° | Turn Left |
| Tilt Right | Roll > +20° | Turn Right |
| Level / Flat | −20° ≤ P,R ≤ +20° | Stop |

> Thresholds can be tuned in `src/main.cpp` via the `PITCH_THRESHOLD` and `ROLL_THRESHOLD` defines.

---

## Software Stack

- **Framework:** Arduino (via PlatformIO)
- **Platform:** Espressif ESP32-C3
- **Key Libraries:**
  - `Wire.h` — I2C communication with MPU6050
  - `MPU6050.h` / `Adafruit MPU6050` — IMU driver & DMP
  - `esp_now.h` — Low-latency peer-to-peer ESP-NOW protocol
  - `WiFi.h` — Required for ESP-NOW channel initialization

---

## Project Structure

```
smart-glove-car/
├── src/
│   └── main.cpp           # Main application logic (transmitter or receiver)
├── include/
│   └── config.h           # Pin definitions, thresholds, MAC addresses
├── lib/                   # Local/vendored libraries (if any)
├── test/                  # Unit tests
├── platformio.ini         # PlatformIO build configuration
└── .vscode/               # Editor settings
```

---

## Setup & Flashing

### Prerequisites

1. Install [PlatformIO](https://platformio.org/) (VS Code extension or CLI).
2. Clone this repository:
   ```bash
   git clone https://github.com/Divija2612/smart-glove-car.git
   cd smart-glove-car
   ```

### Step 1 — Get Receiver MAC Address

Before flashing the transmitter, you need the car-side ESP32-C3's MAC address.

Flash a simple MAC-printer sketch to the receiver ESP32-C3:
```cpp
#include <WiFi.h>
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.println(WiFi.macAddress());
}
void loop() {}
```
Note the printed MAC (e.g., `AA:BB:CC:DD:EE:FF`).

### Step 2 — Configure MAC Address

In `include/config.h`, set:
```cpp
uint8_t receiverMAC[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
```

### Step 3 — Flash Receiver (Car Unit)

```bash
# Set the build environment to 'receiver' in platformio.ini, then:
pio run -e receiver -t upload
```

### Step 4 — Flash Transmitter (Glove Unit)

```bash
pio run -e transmitter -t upload
```

### Step 5 — Monitor Serial Output

```bash
pio device monitor --baud 115200
```

---

## How It Works

1. **Sensor Acquisition:** The MPU6050 samples accelerometer and gyroscope data at ~100 Hz over I2C.
2. **Angle Computation:** Raw accelerometer values are converted to pitch and roll angles using `atan2()`. A complementary filter (or the MPU6050 DMP) fuses gyroscope data to reduce noise.
3. **Command Encoding:** Angles are compared against thresholds to produce a `Command` enum value (`FORWARD`, `BACKWARD`, `LEFT`, `RIGHT`, `STOP`).
4. **Wireless Transmission:** The command is packed into a small struct and sent via ESP-NOW to the receiver's MAC address. Latency is typically under 5 ms.
5. **Motor Control:** The receiver decodes the command and drives the L298N IN1–IN4 pins accordingly. PWM on ENA/ENB controls speed.


---

## Future Improvements

- [ ] Add PID control for smoother speed transitions
- [ ] Implement gyroscope-based complementary filter (currently accelerometer-only)
- [ ] Add OLED display on glove for real-time angle readout
- [ ] Support servo-based steering (single rear motor + front servo)
- [ ] OTA firmware updates over Wi-Fi
- [ ] Mobile app dashboard via BLE for telemetry

