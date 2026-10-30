# Operator Instructions
## EXP Inertial Reference Navigation Backup

**Version:** 0.3.4  
**Intended use:** VFR reorientation aid only. Not a certified navigation instrument.

---

## 1. What This Device Does

It gives you:
- Magnetic heading
- Distance from a known origin
- Altitude
- Time (GPS when available, otherwise uptime)
- Time since last origin reset

It is a backup for when GPS fails or is unreliable. It is **not** a precision navigation system. Inertial distance will drift. Use it to reorient on a paper sectional and continue visual navigation.

---

## 2. Power

- The unit powers on automatically when aircraft power (12/24 V) is present.
- A small LiPo provides backup if aircraft power is lost.
- If aircraft power is lost:
  - **BATT** and a countdown appear (blinking) in the top-right.
  - If power returns before 120 seconds, the countdown cancels and the unit continues without reset.
  - If power stays off for the full 120 seconds, the unit goes to deep sleep to conserve the battery.
  - Restoring aircraft power after sleep wakes the unit automatically.

---

## 3. Display Layout

```
[Time]          [RST mm:ss]   or   [BATT xxxs] (blinking)
HDG xxx         GPS / INS     or   GPS FAIL (blinking)
DIST xxxxx m/ft
ALT  xxxxx m/ft          SAT x
```

- **Time** — GPS time when healthy, otherwise uptime.
- **RST** — minutes:seconds since last origin reset.
- **HDG** — magnetic heading (tilt-compensated).
- **GPS / INS** — current navigation mode.
- **GPS FAIL** — blinks when GPS is unhealthy (poor satellite count or HDOP).
- **DIST** — distance from origin.
- **ALT** — barometric altitude.
- **SAT** — number of GPS satellites (when available).
- **BATT** — blinks with remaining seconds when running on battery.

Alerts blink at 1 second on / 1 second off and do not permanently cover the primary data.

---

## 4. Buttons

Two buttons: **Menu** (GPIO 4) and **Reset** (GPIO 5).

| Action | Button | Result |
|--------|--------|--------|
| Long press (5 s) | Menu | Enter or exit menu |
| Short press | Menu (in menu) | Toggle the selected option |
| Long press (5 s) | Reset | Reset origin and timer (outside menu) or advance menu selection |
| Both buttons held | — | Soft power-off message (full power control is automatic) |

### Menu Options
- **Units** — Metric (m) or Imperial (ft)
- **Invert** — Normal or inverted display
- **Exit** — Leave menu

Settings are saved and restored on the next power-up.

---

## 5. Setting the Origin

- On first GPS lock after power-up (or after a reset), the current position becomes the origin.
- Long-press **Reset** (5 s) to clear the origin and timer. The next healthy GPS fix will set a new origin.
- Distance is measured from that origin.

---

## 6. GPS vs Inertial Mode

- **GPS mode** — Healthy GPS (≥4 satellites and good HDOP). Distance comes from GPS.
- **INS mode** — GPS unhealthy. Distance is estimated by simple inertial integration. Drift is expected.
- **GPS FAIL** blinks when the unit has switched to (or is in) inertial mode because of poor GPS.

Use the heading and approximate distance to reorient on a sectional. Do not treat the inertial distance as precise.

---

## 7. Before Flight

1. Confirm the unit powers on with aircraft power.
2. Wait for GPS lock (SAT count appears and “GPS” shows).
3. Optionally long-press Reset to set a fresh origin at the ramp or runway.
4. Verify heading looks reasonable (point the aircraft in a known direction).
5. Confirm units (m/ft) are what you prefer.

---

## 8. In Flight – GPS Failure

1. Note the **GPS FAIL** blink and the mode change to **INS**.
2. Use the displayed heading and distance-from-origin together with your last known position and a sectional.
3. Continue under visual flight rules.
4. When GPS recovers, the unit will automatically return to GPS mode and the FAIL alert will stop.

---

## 9. Limitations

- Pure inertial distance drifts. This is a reorientation aid, not a substitute for visual navigation or a certified instrument.
- Magnetic heading requires the unit to be mounted reasonably level and calibrated for the aircraft (full calibration is a future feature).
- Battery backup is intended for short interruptions, not extended operation without aircraft power.
- Not approved for IFR or as a primary navigation source.

---

## 10. Troubleshooting

| Symptom | Check |
|---------|-------|
| Blank display | Power and OLED wiring; try Invert in menu |
| No GPS | Antenna view of sky; wait for lock; check SAT count |
| Heading wrong | Mounting orientation; future calibration routine |
| Won’t wake after sleep | Confirm 12 V is reaching the buck and the sense divider |
| Stuck in menu | Long-press Menu to exit, or power cycle |

---

*Use at your own risk. This is an experimental backup aid for VFR reorientation only.*
