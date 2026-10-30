# Design Notes – EXP Inertial Reference v0.3

## Display Invert
- Do **not** use `setContrast(0)`. That blanks the SSD1309.
- Correct method: controller commands
  - `0xA6` = normal
  - `0xA7` = inverted
- Implemented via `u8g2.sendF("c", ...)` in `applyDisplayMode()`.

## Magnetic Heading
- Raw `atan2(mag.y, mag.x)` is only valid when the board is level.
- v0.3 includes simple tilt compensation using roll/pitch from the accelerometer.
- Labeled **HDG** (magnetic). Full hard-iron / soft-iron calibration is still required for in-aircraft use.

## GPS Health and Failover
- Health: ≥4 satellites **and** HDOP < 2.5 **and** valid location.
- Healthy → distance from GPS, display shows “GPS”.
- Unhealthy → display shows “INS”, distance driven by simple along-track acceleration integration.
- Full spoofing detection (position jump, baro cross-check) and proper double-integration are next steps.

## Pure INS Distance
- Current implementation is a rough stub (projects horizontal accel along heading, integrates once with light damping).
- Acceptable for VFR reorientation demonstration; will drift.
- Later: proper velocity/position double integration, gyro aiding, and occasional GPS corrections when healthy.

## Units
- Metric (m / km) or Imperial (ft / mi) selectable in menu.
- Stored in NVS.

## Magnetometer Calibration (Future)
- Multi-point / figure-8 calibration in the airframe.
- Store calibration matrix in NVS.
- Document procedure in `CALIBRATION.md`.

## Power
- 12/24 V aircraft → MP1584EN (5.0 V) → TP4056 + system.
- Protected 1S LiPo for backup.
- Seamless switchover; software 120 s shutdown countdown planned.
- See `WIRING_GUIDE.md`.

## Known Limitations
- Pure inertial distance drifts. VFR reorientation aid only.
- Heading depends on magnetic calibration and board orientation.
- Not a certified instrument.
