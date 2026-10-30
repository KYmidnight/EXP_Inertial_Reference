# Breadboard Guide – EXP Inertial Reference v0.3

**Target:** ESP32-S3-DevKitC N16R8 + DFRobot Gravity 10DOF + HiLetgo SSD1309 SPI + Deegoo NEO-M8N

Start with USB power. Aircraft 12/24 V power, LiPo backup, and the final case come after basic functionality is verified.

## 1. Required Libraries

1. **U8g2** (olikraus)
2. **TinyGPSPlus** (Mikal Hart)
3. **DFRobot_Multi_DOF_IMU** — https://github.com/DFRobot/DFRobot_Multi_DOF_IMU
4. **DFRobot_RTU** (dependency if required)

Also install the ESP32 board package.

## 2. Pin Assignments

| Function | GPIO | Notes |
|----------|------|-------|
| OLED SPI SCK | 12 | |
| OLED SPI MOSI | 11 | |
| OLED SPI CS | 10 | |
| OLED SPI DC | 13 | |
| OLED SPI RST | 14 | |
| IMU I2C SDA | 8 | Wire.begin(8, 9) |
| IMU I2C SCL | 9 | |
| GPS UART RX (← GPS TX) | 16 | Serial1 |
| GPS UART TX (→ GPS RX) | 17 | Optional |
| Button Menu | 4 | INPUT_PULLUP to GND |
| Button Reset | 5 | INPUT_PULLUP to GND |

All modules share GND. Power from 3.3 V or 5 V as appropriate.

## 3. Wiring

See `images/wiring_diagram.jpg` and `docs/WIRING_GUIDE.md` for the full diagram including the power circuit.

### OLED (SSD1309 SPI)
- VCC → 3.3 V or 5 V
- GND → GND
- SCK → GPIO 12
- MOSI / DIN → GPIO 11
- CS → GPIO 10
- DC → GPIO 13
- RST → GPIO 14

### DFRobot Gravity 10DOF
- DIP: I2C mode, address 0x4A
- + → 3.3 V
- – → GND
- SDA (D/T) → GPIO 8
- SCL (C/R) → GPIO 9

### NEO-M8N GPS
- VCC → 5 V (preferred)
- GND → GND
- TX → GPIO 16 (ESP RX)
- RX → GPIO 17 (optional)

### Buttons
- Menu: GPIO 4 to GND
- Reset: GPIO 5 to GND

## 4. Arduino IDE Setup

- Board: **ESP32S3 Dev Module**
- Upload Speed: 921600
- USB CDC On Boot: Enabled (if using native USB)
- PSRAM: Enabled (OPI PSRAM)
- Partition Scheme: Default or larger app

## 5. First Flash

1. Wire on breadboard.
2. Open `firmware/INS_Backup/INS_Backup.ino`.
3. Upload.
4. Serial Monitor 115200.
5. OLED should show time, heading, distance, altitude, and GPS/INS mode.

## 6. Button Behavior (v0.3)

- Long press Menu (5 s) → enter/exit menu
- Short press Menu in menu → toggle selected option (units or invert)
- Long press Reset (5 s) → reset origin and timer (or advance menu selection)
- Both buttons → soft power-off message

## 7. Next Steps

- Full dead-reckoning integration
- Magnetometer calibration routine
- GPS spoofing detection
- Power shutdown countdown
- 3-part high-temp 3D-printed case
