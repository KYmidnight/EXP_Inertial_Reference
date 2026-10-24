# Design Notes – EXP_Inertial_Reference v0.2

Short notes capturing early review feedback so they stay visible in the repo.

## 1. Display Invert
- Do **not** use `setContrast(0)`. That blanks the SSD1309.
- Correct method: send controller command  
  - `0xA6` = normal  
  - `0xA7` = inverted  
- Implemented via `u8g2.sendF("c", ...)` in `applyDisplayMode()`.

## 2. Magnetic Heading
- Raw `atan2(mag.y, mag.x)` is only valid when the board is level.
- v0.2 adds simple tilt compensation using roll/pitch derived from the accelerometer.
- Output is labeled **HDG (MAG)** because magnetic heading is what is wanted against a sectional.
- Full hard-iron / soft-iron calibration (ellipse fit) is still required for in-aircraft use. That is a later milestone.

## 3. DFRobot 10DOF Library
- `imu.get10dofData(&accel, &gyro, &mag, &pressureAlt, true)` is the documented call.
- When the last argument is `true` the float is altitude in meters.
- Optional: call `imu.calibrateAltitude(knownMeters)` once at startup if you want absolute barometric altitude.
- If the call fails on first bring-up, fall back to reading pressure and converting manually.

## 4. GPS Health and Failover
- Current health check: ≥4 satellites and HDOP < 2.5.
- When GPS is healthy the distance is updated from GPS and the display shows “GPS”.
- When GPS is unhealthy the display shows “INS” and distance stops being updated from GPS (it will later be driven by pure inertial integration).
- Full spoofing detection (position jump, altitude cross-check vs baro, etc.) and clean pure-INS dead-reckoning are still next steps.

## 5. Magnetometer Calibration (Future)
- A simple offset is not sufficient inside an aircraft.
- Plan a multi-point / figure-8 calibration routine that can be performed in the airframe.
- Store the resulting calibration matrix in NVS.
- Document the procedure in a future `CALIBRATION.md`.

## 6. Known Limitations (still true)
- Pure inertial distance will drift. This is a VFR reorientation aid, not a precision navigation system.
- Heading accuracy depends on magnetic calibration and board orientation relative to the aircraft.
- Power architecture (buck + TP4056 + LiPo + 12/24 V) is still separate from this breadboard stage.
