# EXP Inertial Reference

A self-contained inertial reference navigation backup for experimental aircraft (VFR).

Provides approximate magnetic heading, distance from a known origin, altitude, and time so a pilot can reorient on a paper sectional if GPS fails or is spoofed.

**This is not a precision navigation system.** Inertial drift is expected. It is a “good enough” reorientation aid for visual flight rules.

## Status

**v0.3 – Breadboard / Bring-up**

- Hardware selected
- Firmware with tilt-compensated heading, GPS health gating, basic INS distance stub, units toggle, persistent settings, and two-button UI
- Corrected wiring diagram and power architecture
- Next: physical bring-up, full dead-reckoning, magnetometer calibration, 3D-printed case

## Hardware

| Component | Part | Notes |
|-----------|------|-------|
| Controller | ESP32-S3-DevKitC N16R8 (dual USB-C) | 16 MB flash + 8 MB PSRAM |
| IMU + Baro | DFRobot Gravity 10DOF (BMI323 + BMM350 + BMP581) | I2C, address 0x4A |
| Display | HiLetgo 2.42" SSD1309 128x64 OLED | SPI (7-pin) |
| GPS | Deegoo NEO-M8N (external housing) | UART NMEA |
| Buttons | 6×6 mm tactile switches | Menu + Reset |
| Power | MP1584EN buck + TP4056 + protected 1S LiPo | 12/24 V aircraft + battery backup |

See `docs/WIRING_GUIDE.md` and `images/wiring_diagram.jpg`.

## Quick Start

1. Install Arduino IDE and the ESP32 board package (Espressif Systems).
2. Install libraries:
   - U8g2
   - TinyGPSPlus
   - DFRobot_Multi_DOF_IMU (and DFRobot_RTU if required)
3. Open `firmware/INS_Backup/INS_Backup.ino`.
4. Board: **ESP32S3 Dev Module**, PSRAM enabled, USB CDC On Boot enabled if using native USB.
5. Upload and open Serial Monitor at 115200.

## Features (v0.3)

- Tilt-compensated magnetic heading
- GPS health check (satellites + HDOP) with GPS / INS mode indicator
- Basic pure-INS distance accumulation when GPS is unhealthy
- Units toggle (metric / imperial)
- Persistent display invert and units (NVS)
- Correct SSD1309 invert (0xA6 / 0xA7)
- Two-button UI:
  - Long press Menu (5 s) → enter/exit menu
  - Short press Menu in menu → toggle current option
  - Long press Reset (5 s) → reset origin/timer (or advance menu)
  - Both buttons → soft power-off message

## Project Goals

- Cheap and buildable by experimental aircraft enthusiasts
- Clear monochrome OLED
- Explicit GPS integrity monitoring with clean fallback to inertial mode
- Persistent settings and future compass calibration
- 3D-printable high-temperature case for 3 1/8" panel
- Fully open source

## License

MIT License — see [LICENSE](LICENSE).

---

*Designed for the experimental aircraft community. Use at your own risk. This is a backup reorientation aid, not a certified navigation instrument.*
