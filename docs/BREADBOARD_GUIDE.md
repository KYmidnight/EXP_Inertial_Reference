# Aircraft Inertial Reference Navigation Backup – Breadboard Guide
## Initial Prototype (ESP32-S3 + 10DOF + OLED + GPS)

**Version:** 0.1 (Breadboard / Bring-up)  
**Date:** October 2026  
**Target:** ESP32-S3-DevKitC N16R8 + DFRobot Gravity 10DOF + HiLetgo 2.42" SSD1309 SPI + Deegoo NEO-M8N

This guide covers the breadboard stage. Start with USB power. Aircraft 12/24 V power, LiPo backup, and the final case come after basic functionality is verified.

---

## 1. Required Libraries (Arduino IDE)

Install via Library Manager or GitHub ZIP:

1. **U8g2** (by olikraus) – for the SSD1309 OLED  
2. **TinyGPSPlus** (by Mikal Hart) – GPS NMEA parsing  
3. **DFRobot_Multi_DOF_IMU** – main library for the Gravity 10DOF  
   - GitHub: https://github.com/DFRobot/DFRobot_Multi_DOF_IMU  
4. **DFRobot_RTU** – dependency for the Multi_DOF library  
   - Install this first if the Multi_DOF library complains  

Also install the ESP32 board package (Espressif Systems) if you have not already.

---

## 2. Pin Assignments (Safe for N16R8)

These avoid PSRAM pins (35–37), USB pins (19/20), and strapping pins.

| Function              | ESP32-S3 GPIO | Notes                          |
|-----------------------|---------------|--------------------------------|
| OLED SPI SCK          | 12            |                                |
| OLED SPI MOSI         | 11            |                                |
| OLED SPI CS           | 10            |                                |
| OLED SPI DC           | 13            |                                |
| OLED SPI RST          | 14            |                                |
| IMU I2C SDA           | 8             | Default Wire pins work too     |
| IMU I2C SCL           | 9             |                                |
| GPS UART RX (from GPS)| 16            | GPS TX → ESP RX                |
| GPS UART TX (to GPS)  | 17            | ESP TX → GPS RX (optional)     |
| Button 1 (Menu)       | 4             | To GND, internal pull-up       |
| Button 2 (Reset)      | 5             | To GND, internal pull-up       |

**Power:** All modules → 3.3 V or 5 V (most of these modules accept either). Use the ESP32’s 3.3 V pin for logic and the 5 V pin if the OLED or GPS prefers it. Common ground is mandatory.

---

## 3. Wiring Instructions

Refer to the accompanying wiring diagram image (`wiring_diagram.jpg`).

### OLED (SSD1309 SPI version)
- VCC → 3.3 V or 5 V  
- GND → GND  
- SCK → GPIO 12  
- MOSI / DIN → GPIO 11  
- CS → GPIO 10  
- DC → GPIO 13  
- RST → GPIO 14  

### DFRobot Gravity 10DOF
- Set DIP switches: I2C mode, address 0x4A (A0=0, A1=0 – factory default)  
- + → 3.3 V  
- − → GND  
- SDA → GPIO 8  
- SCL → GPIO 9  

### NEO-M8N GPS (external housing)
- VCC → 5 V (preferred) or 3.3 V  
- GND → GND  
- TX (from GPS) → GPIO 16 (ESP RX)  
- RX (to GPS) → GPIO 17 (optional for configuration)  

### Buttons
- Button 1 (Menu): One side to GPIO 4, other side to GND  
- Button 2 (Reset): One side to GPIO 5, other side to GND  
(Internal pull-ups are enabled in code.)

---

## 4. Arduino IDE Setup

1. Board: **ESP32S3 Dev Module** (or ESP32-S3-DevKitC-1)  
2. Upload Speed: 921600  
3. USB CDC On Boot: Enabled (if available)  
4. PSRAM: Enabled (OPI PSRAM)  
5. Partition Scheme: Default or larger app if needed  

---

## 5. First Flash & Verification

1. Wire everything on the breadboard.  
2. Open `INS_Backup_Initial.ino`.  
3. Select the correct COM port and upload.  
4. Open Serial Monitor at 115200 baud.  
5. You should see initialization messages, then live data on the OLED:  
   - Time (from GPS when available, otherwise uptime)  
   - Heading  
   - Distance from origin  
   - Altitude  
   - Time since reset  

**Button behavior (initial version):**
- Short press Button 1 → cycle display brightness / invert (stub)  
- Long press Button 1 (5 s) → enter menu (stub)  
- Long press Button 2 (5 s) → reset origin and timer  
- Both buttons held → soft power-off message (full power control comes with the battery circuit)

---

## 6. Next Steps After Bring-up

Once the OLED, IMU, GPS, and buttons work:
- Add full menu system  
- Implement simple dead-reckoning distance  
- Add GPS integrity checks and fallback  
- Compass calibration routine  
- Persistent settings  
- Then move to the 3-part 3D-printed case

---

## 7. Notes & Known Limitations

- Pure inertial distance will drift. This is a VFR reorientation aid, not a precision navigation system.  
- Magnetic heading needs a basic calibration in the aircraft.  
- GPS is used for time, altitude reference, and occasional position correction when healthy.  
- Power architecture (buck + TP4056 + LiPo + 12/24 V auto-start) is separate from this breadboard stage.

Questions or issues during bring-up → continue in the project chat.
