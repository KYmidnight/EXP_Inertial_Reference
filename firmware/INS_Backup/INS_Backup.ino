/*
 * Aircraft Inertial Reference Navigation Backup
 * Firmware v0.3 (Breadboard / Bring-up + core features)
 *
 * Hardware:
 * - ESP32-S3-DevKitC N16R8 (dual USB-C)
 * - DFRobot Gravity 10DOF (BMI323 + BMM350 + BMP581)
 * - HiLetgo 2.42" SSD1309 OLED (SPI)
 * - Deegoo NEO-M8N GPS (UART)
 * - Two tactile buttons
 *
 * Required libraries (Arduino Library Manager or GitHub ZIP):
 * - U8g2 (olikraus)
 * - TinyGPSPlus (Mikal Hart)
 * - DFRobot_Multi_DOF_IMU (+ DFRobot_RTU dependency)
 *
 * Features in this version:
 * - Tilt-compensated magnetic heading
 * - GPS health gating (sat count + HDOP) with GPS / INS mode indicator
 * - Basic pure-INS distance stub (accel integration along heading)
 * - Units toggle (m/ft, km/mi)
 * - Persistent display invert and units via NVS
 * - Proper SSD1309 invert (0xA6 / 0xA7)
 * - Two-button UI with long-press behaviors
 * - Menu skeleton (units, invert)
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
bool useMetric = true;          // true = m/km, false = ft/mi

float originLat = 0, originLon = 0;
bool originSet = false;

float headingDeg = 0;
float altitudeM = 0;
float distanceM = 0;

// Simple INS dead-reckoning state
float velocityMps = 0;          // along-track speed (m/s)
float insDistanceM = 0;         // distance accumulated in pure INS mode

bool gpsHealthy = false;
bool inMenu = false;
int menuIndex = 0;
const int MENU_ITEMS = 3;       // Units, Invert, Exit

// -------------------- Helpers --------------------
float toDisplayDist(float meters) {
  if (useMetric) return meters;
  return meters * 3.28084f;     // feet
}

const char* distUnit() {
  return useMetric ? "m" : "ft";
}

float toDisplayAlt(float meters) {
  return useMetric ? meters : meters * 3.28084f;
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println("\nINS Backup v0.3");

  pinMode(BTN_MENU, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);

  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.setContrast(255);

  prefs.begin("ins", false);
  displayInverted = prefs.getBool("invert", false);
  useMetric = prefs.getBool("metric", true);
  applyDisplayMode();

  showSplash();

  Wire.begin(IMU_SDA, IMU_SCL);
  Serial.print("IMU... ");
  if (!imu.begin()) {
    Serial.println("FAILED");
    showError("IMU FAIL");
  } else {
    Serial.println("OK");
    imu.setSensorMode(DFRobot_Multi_DOF_IMU::eNormalMode);
    imu.setAccelRange(DFRobot_Multi_DOF_IMU::eAccelRange2G);
    imu.setGyroRange(DFRobot_Multi_DOF_IMU::eGyroRange250DPS);
  }

  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("GPS serial started");

  resetMillis = millis();
}

// -------------------- Loop --------------------
void loop() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 100) {
    lastUpdate = millis();
    if (!inMenu) {
      readSensors();
      updateDisplay();
    }
  }

  handleButtons();
}

// -------------------- Display --------------------
void showSplash() {
  u8g2.clearBuffer();
  u8g2.drawStr(10, 20, "INS BACKUP");
  u8g2.drawStr(10, 35, "v0.3 Breadboard");
  u8g2.drawStr(10, 50, "Starting...");
  u8g2.sendBuffer();
  delay(1200);
}

void showError(const char* msg) {
  u8g2.clearBuffer();
  u8g2.drawStr(10, 30, msg);
  u8g2.sendBuffer();
  delay(2000);
}

void applyDisplayMode() {
  // Correct SSD1309 invert: 0xA6 normal, 0xA7 inverted
  // Do NOT use setContrast(0) — it blanks the display
  u8g2.sendF("c", displayInverted ? 0xA7 : 0xA6);
}

void updateDisplay() {
  u8g2.clearBuffer();
  char buf[32];

  // Time
  if (gps.time.isValid() && gpsHealthy) {
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

  // Heading
  snprintf(buf, sizeof(buf), "HDG %3.0f", headingDeg);
  u8g2.drawStr(0, 28, buf);

  // Mode indicator
  u8g2.drawStr(90, 28, gpsHealthy ? "GPS" : "INS");

  // Distance
  float showDist = toDisplayDist(distanceM);
  snprintf(buf, sizeof(buf), "DIST %5.0f %s", showDist, distUnit());
  u8g2.drawStr(0, 42, buf);

  // Altitude
  float showAlt = toDisplayAlt(altitudeM);
  snprintf(buf, sizeof(buf), "ALT %5.0f %s", showAlt, useMetric ? "m" : "ft");
  u8g2.drawStr(0, 56, buf);

  // Satellites
  if (gps.satellites.isValid()) {
    snprintf(buf, sizeof(buf), "SAT %d", gps.satellites.value());
    u8g2.drawStr(90, 56, buf);
  }

  u8g2.sendBuffer();
}

void drawMenu() {
  u8g2.clearBuffer();
  u8g2.drawStr(0, 12, "MENU");

  const char* items[MENU_ITEMS] = {
    useMetric ? "Units: METRIC" : "Units: IMPERIAL",
    displayInverted ? "Invert: ON" : "Invert: OFF",
    "Exit"
  };

  for (int i = 0; i < MENU_ITEMS; i++) {
    if (i == menuIndex) {
      u8g2.drawStr(0, 28 + i * 14, ">");
    }
    u8g2.drawStr(10, 28 + i * 14, items[i]);
  }
  u8g2.sendBuffer();
}

// -------------------- Sensors --------------------
void computeTiltCompensatedHeading(float ax, float ay, float az,
                                   float mx, float my, float mz) {
  // Simple tilt compensation from accelerometer
  float roll  = atan2(ay, az);
  float pitch = atan2(-ax, sqrt(ay * ay + az * az));

  float mx2 = mx * cos(pitch) + mz * sin(pitch);
  float my2 = mx * sin(roll) * sin(pitch) + my * cos(roll) - mz * sin(roll) * cos(pitch);

  headingDeg = atan2(my2, mx2) * 180.0f / M_PI;
  if (headingDeg < 0) headingDeg += 360.0f;
}

void readSensors() {
  DFRobot_Multi_DOF_IMU::sSensorData_t accel, gyro, mag;
  float pressureAlt = 0;

  bool imuOk = imu.get10dofData(&accel, &gyro, &mag, &pressureAlt, true);

  if (imuOk) {
    altitudeM = pressureAlt;
    computeTiltCompensatedHeading(accel.x, accel.y, accel.z,
                                  mag.x, mag.y, mag.z);
  }

  // GPS health
  bool satsOk = gps.satellites.isValid() && gps.satellites.value() >= 4;
  bool hdopOk = gps.hdop.isValid() && gps.hdop.hdop() < 2.5f;
  bool locOk  = gps.location.isValid();
  gpsHealthy = satsOk && hdopOk && locOk;

  if (gpsHealthy) {
    if (!originSet) {
      originLat = gps.location.lat();
      originLon = gps.location.lng();
      originSet = true;
      Serial.println("Origin set from GPS");
    }
    distanceM = TinyGPSPlus::distanceBetween(
      originLat, originLon,
      gps.location.lat(), gps.location.lng()
    );
    // Reset INS velocity when GPS is good (simple aiding)
    velocityMps = gps.speed.isValid() ? gps.speed.mps() : 0;
    insDistanceM = distanceM;
  } else {
    // Pure INS mode — basic dead-reckoning stub
    // Integrate forward acceleration along heading (very rough)
    if (imuOk) {
      // Project horizontal acceleration onto heading
      // This is a placeholder; full double-integration comes later
      float headingRad = headingDeg * M_PI / 180.0f;
      // Assume accel.x/y are in g; rough conversion
      float ax = accel.x * 9.81f;
      float ay = accel.y * 9.81f;
      float along = ax * sin(headingRad) + ay * cos(headingRad);
      velocityMps += along * 0.1f;   // dt = 0.1 s
      // Light damping to limit runaway
      velocityMps *= 0.98f;
      insDistanceM += velocityMps * 0.1f;
      if (insDistanceM < 0) insDistanceM = 0;
      distanceM = insDistanceM;
    }
  }
}

// -------------------- Buttons --------------------
void handleButtons() {
  static unsigned long menuPress = 0;
  static unsigned long resetPress = 0;
  static bool menuLongDone = false;
  static bool resetLongDone = false;

  bool menuDown = (digitalRead(BTN_MENU) == LOW);
  bool resetDown = (digitalRead(BTN_RESET) == LOW);

  // Both buttons held → soft power-off message
  if (menuDown && resetDown) {
    u8g2.clearBuffer();
    u8g2.drawStr(15, 30, "POWER OFF");
    u8g2.drawStr(10, 45, "(hold 2s)");
    u8g2.sendBuffer();
    delay(500);
    return;
  }

  // Menu button
  if (menuDown) {
    if (menuPress == 0) {
      menuPress = millis();
      menuLongDone = false;
    }
    if (!menuLongDone && millis() - menuPress > 5000) {
      menuLongDone = true;
      inMenu = !inMenu;
      menuIndex = 0;
      if (inMenu) drawMenu();
      Serial.println(inMenu ? "Entered menu" : "Exited menu");
    }
  } else {
    if (menuPress > 0 && !menuLongDone && millis() - menuPress < 1000) {
      // Short press
      if (inMenu) {
        // Cycle menu or toggle
        if (menuIndex == 0) {
          useMetric = !useMetric;
          prefs.putBool("metric", useMetric);
        } else if (menuIndex == 1) {
          displayInverted = !displayInverted;
          prefs.putBool("invert", displayInverted);
          applyDisplayMode();
        } else if (menuIndex == 2) {
          inMenu = false;
        }
        if (inMenu) drawMenu();
      } else {
        // Outside menu: short press cycles brightness conceptually (stub)
        // Could later cycle contrast levels
      }
    }
    menuPress = 0;
  }

  // Reset button
  if (resetDown) {
    if (resetPress == 0) {
      resetPress = millis();
      resetLongDone = false;
    }
    if (!resetLongDone && millis() - resetPress > 5000) {
      resetLongDone = true;
      if (inMenu) {
        // Commit / advance menu selection
        menuIndex = (menuIndex + 1) % MENU_ITEMS;
        drawMenu();
      } else {
        // Reset origin and timer
        originSet = false;
        distanceM = 0;
        insDistanceM = 0;
        velocityMps = 0;
        resetMillis = millis();
        Serial.println("Origin / timer reset");
      }
    }
  } else {
    resetPress = 0;
  }
}
