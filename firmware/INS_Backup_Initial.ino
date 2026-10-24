/*
 * Aircraft Inertial Reference Navigation Backup
 * Breadboard Firmware v0.2
 *
 * Improvements over v0.1:
 * - Proper SSD1309 invert (0xA6 / 0xA7) instead of contrast=0
 * - Tilt-compensated magnetic heading (roll/pitch from accel)
 * - Clearer GPS health indication and mode (GPS / INS)
 * - Safer sensor read structure
 * - Labeled HDG (MAG)
 *
 * Hardware:
 * - ESP32-S3-DevKitC N16R8
 * - DFRobot Gravity 10DOF (BMI323 + BMM350 + BMP581)
 * - HiLetgo 2.42" SSD1309 OLED (SPI)
 * - NEO-M8N GPS
 * - Two tactile buttons
 *
 * Required libraries:
 * - U8g2
 * - TinyGPSPlus
 * - DFRobot_Multi_DOF_IMU (+ DFRobot_RTU)
 */

#include <U8g2lib.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include "DFRobot_Multi_DOF_IMU.h"
#include <Preferences.h>
#include <math.h>

// -------------------- Pins --------------------
#define OLED_SCK   12
#define OLED_MOSI  11
#define OLED_CS    10
#define OLED_DC    13
#define OLED_RST   14

#define IMU_SDA    8
#define IMU_SCL    9

#define GPS_RX     16   // ESP RX ← GPS TX
#define GPS_TX     17   // ESP TX → GPS RX

#define BTN_MENU   4
#define BTN_RESET  5

// -------------------- Objects --------------------
U8G2_SSD1309_128X64_NONAME2_F_4W_SW_SPI u8g2(
  U8G2_R0,
  OLED_SCK, OLED_MOSI, OLED_CS, OLED_DC, OLED_RST
);

TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

const uint8_t IMU_ADDR = 0x4A;
DFRobot_Multi_DOF_IMU_I2C imu(DFRobot_Multi_DOF_IMU::eSensorModel10DOF, &Wire, IMU_ADDR);

Preferences prefs;

// -------------------- State --------------------
unsigned long resetMillis = 0;
bool displayInverted = false;

float originLat = 0, originLon = 0;
bool originSet = false;

float headingDeg = 0;
float altitudeM = 0;
float distanceM = 0;

bool gpsHealthy = false;
bool insMode = true;          // true = pure INS / no healthy GPS

// Simple GPS health thresholds (tune later)
const int   MIN_SATS = 4;
const float MAX_HDOP = 2.5;

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nINS Backup – v0.2");

  // Buttons
  pinMode(BTN_MENU, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);

  // Display
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.setContrast(255);
  showSplash();

  // IMU
  Wire.begin(IMU_SDA, IMU_SCL);
  Serial.print("Init IMU... ");
  if (!imu.begin()) {
    Serial.println("FAILED");
    showError("IMU FAIL");
  } else {
    Serial.println("OK");
    imu.setSensorMode(DFRobot_Multi_DOF_IMU::eNormalMode);
    imu.setAccelRange(DFRobot_Multi_DOF_IMU::eAccelRange2G);
    imu.setGyroRange(DFRobot_Multi_DOF_IMU::eGyroRange250DPS);
    // Optional: calibrateAltitude(knownAltitudeMeters) if you want absolute baro later
  }

  // GPS
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("GPS serial started");

  resetMillis = millis();
  prefs.begin("ins", false);
  displayInverted = prefs.getBool("invert", false);
  applyDisplayMode();
}

// -------------------- Loop --------------------
void loop() {
  // Feed GPS
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // Read sensors ~10 Hz
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 100) {
    lastUpdate = millis();
    readSensors();
    updateDisplay();
  }

  handleButtons();
}

// -------------------- Functions --------------------
void showSplash() {
  u8g2.clearBuffer();
  u8g2.drawStr(10, 20, "INS BACKUP");
  u8g2.drawStr(10, 35, "v0.2 Bring-up");
  u8g2.drawStr(10, 50, "Waiting...");
  u8g2.sendBuffer();
  delay(1500);
}

void showError(const char* msg) {
  u8g2.clearBuffer();
  u8g2.drawStr(10, 30, msg);
  u8g2.sendBuffer();
  delay(2000);
}

void applyDisplayMode() {
  // Proper SSD1309 invert (0xA6 = normal, 0xA7 = inverted)
  // Do NOT use setContrast(0) — that blanks the display
  if (displayInverted) {
    u8g2.sendF("c", 0xA7);
  } else {
    u8g2.sendF("c", 0xA6);
  }
  u8g2.setContrast(255);
}

// Tilt-compensated magnetic heading
// Assumes accelerometer Z is "up" when level (adjust if board orientation differs)
float computeTiltCompensatedHeading(float ax, float ay, float az, float mx, float my, float mz) {
  // Normalize accelerometer
  float normA = sqrt(ax*ax + ay*ay + az*az);
  if (normA < 0.01f) return headingDeg;  // keep last
  ax /= normA; ay /= normA; az /= normA;

  // Roll and pitch from accelerometer (radians)
  float roll  = atan2(ay, az);
  float pitch = atan2(-ax, sqrt(ay*ay + az*az));

  // Tilt-compensate magnetometer
  float mx_h = mx * cos(pitch) + mz * sin(pitch);
  float my_h = mx * sin(roll) * sin(pitch) + my * cos(roll) - mz * sin(roll) * cos(pitch);

  float heading = atan2(my_h, mx_h) * 180.0f / PI;
  if (heading < 0) heading += 360.0f;
  return heading;
}

void readSensors() {
  DFRobot_Multi_DOF_IMU::sSensorData_t accel, gyro, mag;
  float pressureAlt = 0;

  // get10dofData is the documented call for this module.
  // Last argument true = return altitude (m) instead of raw pressure (Pa).
  // If this returns false on first bring-up, fall back to reading pressure and converting.
  if (imu.get10dofData(&accel, &gyro, &mag, &pressureAlt, true)) {
    // Tilt-compensated magnetic heading
    headingDeg = computeTiltCompensatedHeading(
      accel.x, accel.y, accel.z,
      mag.x, mag.y, mag.z
    );
    altitudeM = pressureAlt;
  }

  // GPS health check
  gpsHealthy = gps.location.isValid() &&
               gps.satellites.isValid() &&
               gps.satellites.value() >= MIN_SATS &&
               gps.hdop.isValid() &&
               gps.hdop.hdop() < MAX_HDOP;

  insMode = !gpsHealthy;

  if (gpsHealthy) {
    if (!originSet) {
      originLat = gps.location.lat();
      originLon = gps.location.lng();
      originSet = true;
      resetMillis = millis();
      Serial.println("Origin set from GPS");
    }
    distanceM = TinyGPSPlus::distanceBetween(
      originLat, originLon,
      gps.location.lat(), gps.location.lng()
    );
  }
  // When not healthy, distanceM is left unchanged (will be replaced by pure INS integration later)
}

void updateDisplay() {
  u8g2.clearBuffer();

  char buf[40];

  // Time
  if (gps.time.isValid()) {
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
             gps.time.hour(), gps.time.minute(), gps.time.second());
  } else {
    snprintf(buf, sizeof(buf), "UP %lus", millis() / 1000);
  }
  u8g2.drawStr(0, 12, buf);

  // Time since reset
  unsigned long since = (millis() - resetMillis) / 1000;
  snprintf(buf, sizeof(buf), "RST %02lu:%02lu", since / 60, since % 60);
  u8g2.drawStr(70, 12, buf);

  // Heading (magnetic, tilt-compensated)
  snprintf(buf, sizeof(buf), "HDG(MAG) %3.0f", headingDeg);
  u8g2.drawStr(0, 26, buf);

  // Distance + mode
  if (insMode) {
    snprintf(buf, sizeof(buf), "DIST %5.0fm INS", distanceM);
  } else {
    snprintf(buf, sizeof(buf), "DIST %5.0fm GPS", distanceM);
  }
  u8g2.drawStr(0, 40, buf);

  // Altitude
  snprintf(buf, sizeof(buf), "ALT %5.0f m", altitudeM);
  u8g2.drawStr(0, 54, buf);

  // Status
  if (gps.satellites.isValid()) {
    snprintf(buf, sizeof(buf), "SAT%d", gps.satellites.value());
    u8g2.drawStr(90, 54, buf);
  }

  u8g2.sendBuffer();
}

void handleButtons() {
  static unsigned long menuPress = 0;
  static unsigned long resetPress = 0;

  bool menuDown = (digitalRead(BTN_MENU) == LOW);
  bool resetDown = (digitalRead(BTN_RESET) == LOW);

  // Menu long press
  if (menuDown) {
    if (menuPress == 0) menuPress = millis();
    if (millis() - menuPress > 5000) {
      u8g2.clearBuffer();
      u8g2.drawStr(20, 30, "MENU STUB");
      u8g2.sendBuffer();
      delay(1000);
      menuPress = 0;
    }
  } else {
    if (menuPress > 0 && millis() - menuPress < 1000) {
      // Short press – toggle proper invert
      displayInverted = !displayInverted;
      prefs.putBool("invert", displayInverted);
      applyDisplayMode();
    }
    menuPress = 0;
  }

  // Reset long press
  if (resetDown) {
    if (resetPress == 0) resetPress = millis();
    if (millis() - resetPress > 5000) {
      originSet = false;
      distanceM = 0;
      resetMillis = millis();
      Serial.println("Origin / timer reset");
      resetPress = 0;
    }
  } else {
    resetPress = 0;
  }

  // Both buttons – soft off message
  if (menuDown && resetDown) {
    u8g2.clearBuffer();
    u8g2.drawStr(15, 30, "POWER OFF");
    u8g2.sendBuffer();
    delay(2000);
  }
}
