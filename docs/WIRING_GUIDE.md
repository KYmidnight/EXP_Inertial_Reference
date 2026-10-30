# EXP Inertial Reference – Accurate Wiring Diagram & Power Guide
**Version:** 1.0 (Corrected for dual-USB ESP32-S3 + full power circuit)  
**Date:** October 2026

This replaces earlier diagrams that incorrectly associated GPIOs with the USB ports and omitted the charging/power path.

## 1. ESP32-S3-DevKitC-1 N16R8 Dual USB-C

- **Two USB-C ports** are at the **bottom** of the board:
  - **UART USB** → onboard USB-serial (GPIO43/44) for flashing & Serial Monitor
  - **Native USB** → GPIO19/20 (do not use these pins as GPIO)
- **All signal connections** use the **side header pins** (J1 left, J3 right).  
  The USB ports are only for power/programming during development. They are **not** the GPIO connections.

### Left header (J1) – top to bottom (USB at bottom)
| Pin | Function | Used for |
|-----|----------|----------|
| 3V3 | 3.3 V | Optional logic power |
| RST | Reset | — |
| GPIO4 | — | **BTN_MENU** |
| GPIO5 | — | **BTN_RESET** |
| GPIO15–18 | — | free |
| GPIO16 | — | **GPS RX** (← GPS TX) |
| GPIO17 | — | **GPS TX** (→ GPS RX) |
| GPIO8 | — | **IMU SDA** |
| GPIO9 | — | **IMU SCL** |
| GPIO10 | — | **OLED CS** |
| GPIO11 | — | **OLED MOSI / SDA** |
| GPIO12 | — | **OLED SCK** |
| GPIO13 | — | **OLED DC** |
| GPIO14 | — | **OLED RES** |
| 5V | 5 V input | **System power from buck/battery** |
| GND | Ground | Common GND |

Avoid: GPIO19/20 (native USB), GPIO35–37 (octal PSRAM on N16R8), strapping pins 0/3/45/46 during boot.

## 2. Module Connections

### DFRobot Gravity 10DOF (4-wire Gravity connector)
Set DIP switches to **I2C**, address **0x4A** (A0=0, A1=0).

| Wire | Function | Connect to |
|------|----------|------------|
| + | VCC (3.3–5 V) | 3.3 V or 5 V |
| – | GND | GND |
| C/R | SCL | **GPIO 9** |
| D/T | SDA | **GPIO 8** |

### HiLetgo SSD1309 OLED (7-pin SPI)
| Pin | Function | Connect to |
|-----|----------|------------|
| GND | Ground | GND |
| VCC | 3.3–5 V | 3.3 V or 5 V |
| SCK | Clock | **GPIO 12** |
| SDA | MOSI | **GPIO 11** |
| RES | Reset | **GPIO 14** |
| DC | Data/Command | **GPIO 13** |
| CS | Chip Select | **GPIO 10** |

### Deegoo NEO-M8N GPS (6-wire external module)
Only four wires are required. The extra two (often PPS and an unused/NC line) can float.

| Wire | Function | Connect to |
|------|----------|------------|
| VCC | 5 V preferred | System 5 V |
| GND | Ground | GND |
| TX | GPS data out | **GPIO 16** (ESP RX) |
| RX | GPS data in | **GPIO 17** (ESP TX, optional) |
| PPS | Pulse-per-second | leave open |
| (6th) | NC / other | leave open |

### Buttons
- Button Menu: one side → **GPIO 4**, other side → GND (internal pull-up)
- Button Reset: one side → **GPIO 5**, other side → GND

## 3. Power Architecture (12/24 V + LiPo Backup)

```
Aircraft 12/24 V
    │
    ├── Fuse (1–2 A)
    ├── Reverse-polarity diode (e.g. 1N5408 or ideal diode)
    │
    ▼
MP1584EN Buck module
    IN+ / IN–   ← aircraft power
    OUT+ set to **5.0 V** (verify with multimeter before connecting!)
    OUT–
    │
    ├── Schottky diode → System 5 V rail
    │
    ▼
TP4056 Charger module (protected version preferred)
    IN+ ← Buck 5 V
    IN–
    B+ / BAT+ → 1S LiPo positive
    B– / BAT– → LiPo negative / GND
    OUT+ (load-sharing versions) → System 5 V rail (via diode if needed)
    OUT–
    │
    ▼
System 5 V rail  →  ESP32 **5 V pin**
                 →  OLED VCC
                 →  IMU +
                 →  GPS VCC
All grounds common.
```

### Behavior
- **Aircraft power present** → Buck supplies 5 V, TP4056 charges LiPo, system runs from buck.
- **Aircraft power lost** → LiPo (via TP4056 OUT or boost) seamlessly powers the system.
- **Software** can start a 120 s countdown and then drive a MOSFET (or enable pin) to cut battery power for clean shutdown.
- Use a **protected 1S LiPo**. Never connect LiPo directly without protection.

### Breadboard stage
Start with USB power only. Add the buck + TP4056 + LiPo only after OLED, IMU, GPS, and buttons are verified.

## 4. Files
- `wiring_diagram_accurate.svg` – vector schematic (open in browser or Inkscape)
- This document

## 5. Safety Notes for Aircraft Use
- Verify buck output is exactly 5.0 V before connecting to ESP32 or LiPo charger.
- Fuse the aircraft power input.
- Secure all wiring against vibration.
- The device is a VFR reorientation aid only; pure inertial drift is expected and acceptable for sectional reorientation.

## 5. Power-Source Sensing

To detect when aircraft power is lost and the system switches to the LiPo:

1. Build a voltage divider on the MP1584EN **OUT+** (5 V):
   - 10 kΩ from OUT+ to the sense node
   - 10 kΩ from the sense node to GND
2. Connect the sense node to **ESP32 GPIO 1** (ADC1_CH0 on the right header).
3. Firmware reads this pin. When the voltage drops below the threshold, it sets `onBattery = true` and starts the 120-second shutdown countdown.

All other power connections remain as described above.
