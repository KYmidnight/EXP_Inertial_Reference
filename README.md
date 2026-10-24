# Aircraft Inertial Reference Navigation Backup (INS Backup)

**Open-source, low-cost inertial reference navigation aid for experimental aircraft.**

A self-contained backup navigation device for VFR pilots in the event of GPS failure or spoofing. It provides approximate heading, distance from a known origin, altitude, and time so a pilot can reorient on a paper sectional and continue visual navigation.

This is **not** a precision navigation system. Inertial drift is expected. It is designed as a “good enough” reorientation aid.

## Current Status

**Breadboard / Bring-up stage (v0.1)**

- Hardware selected and ordered
- Initial firmware skeleton written
- Wiring diagram and breadboard guide complete
- Next: physical bring-up, then full menu, dead-reckoning, GPS integrity checks, calibration, and 3D-printed case

## Hardware (Breadboard)

| Component | Part | Notes |
|-----------|------|-------|
| Controller | ESP32-S3-DevKitC N16R8 | 16 MB flash + 8 MB PSRAM |
| IMU + Baro | DFRobot Gravity 10DOF (BMI323 + BMM350 + BMP581) | I2C, address 0x4A |
| Display | HiLetgo 2.42" SSD1309 128x64 OLED | SPI version |
| GPS | Deegoo NEO-M8N (external antenna housing) | UART NMEA |
| Buttons | 6x6 mm tactile switches | 2× (Menu + Reset) |
| Power (later) | MP1584EN buck + TP4056 + protected 1S LiPo | 12/24 V aircraft + battery backup |

Full wiring and pin assignments are in `docs/BREADBOARD_GUIDE.md` and `images/wiring_diagram.jpg`.

## Quick Start (Breadboard)

1. Install Arduino IDE + ESP32 board package.
2. Install libraries:
   - U8g2
   - TinyGPSPlus
   - DFRobot_Multi_DOF_IMU (and its DFRobot_RTU dependency)
3. Open `firmware/INS_Backup_Initial.ino`.
4. Select board: **ESP32S3 Dev Module** (or ESP32-S3-DevKitC-1), enable PSRAM.
5. Upload and follow the breadboard guide.

## Project Goals

- Cheap and buildable by average experimental aircraft enthusiasts
- Clear, legible monochrome OLED (white-on-black / black-on-white)
- Two-button UI with long-press behaviors
- Persistent settings and compass calibration (NVS)
- GPS used for time, altitude zeroing, and occasional position aiding **only when healthy**
- Explicit GPS integrity monitoring with clean fallback to pure inertial mode
- 3D-printable high-temperature case for standard 3 1/8" avionics panel
- Fully open source

## License

MIT License — see [LICENSE](LICENSE).

## Contributing

This is an early-stage project. Feedback, testing notes, and pull requests are welcome once the repository is public.

---

*Designed for the experimental aircraft community. Use at your own risk. This is a backup reorientation aid, not a certified navigation instrument.*
