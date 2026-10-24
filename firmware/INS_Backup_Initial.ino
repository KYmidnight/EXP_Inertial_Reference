/*
 * Aircraft Inertial Reference Navigation Backup
 * Initial Breadboard Firmware v0.1
 * 
 * Hardware:
 * - ESP32-S3-DevKitC N16R8
 * - DFRobot Gravity 10DOF (BMI323+BMM350+BMP581)
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
// Software SPI for flexible pin choice
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

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nINS Backup – Initial Bring-up");

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
  u8g2.drawStr(10, 35, "Bring-up v0.1");
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
  // U8g2 invert via setPowerSave or contrast + software invert for mono
  // For simple black/white swap we use contrast and font color conceptually
  // Full invert can be done by drawing inverted later if needed
  u8g2.setContrast(displayInverted ? 0 : 255);
}

void readSensors() {
  DFRobot_Multi_DOF_IMU::sSensorData_t accel, gyro, mag;
  float pressureAlt;

  if (imu.get10dofData(&accel, &gyro, &mag, &pressureAlt, true)) {
    // Simple heading from magnetometer (X/Y) – needs calibration later
    headingDeg = atan2(mag.y, mag.x) * 180.0 / PI;
    if (headingDeg < 0) headingDeg += 360.0;

    altitudeM = pressureAlt;
  }

  // GPS time / position when valid
  if (gps.location.isValid() && gps.satellites.value() >= 4 && gps.hdop.hdop() < 2.5) {
    if (!originSet) {
      originLat = gps.location.lat();
      originLon = gps.location.lng();
      originSet = true;
      resetMillis = millis();
      Serial.println("Origin set from GPS");
    }
    // Simple distance (Haversine stub – replace with better later)
    distanceM = TinyGPSPlus::distanceBetween(
      originLat, originLon,
      gps.location.lat(), gps.location.lng()
    );
  }
}

void updateDisplay() {
  u8g2.clearBuffer();

  char buf[32];

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

  // Heading
  snprintf(buf, sizeof(buf), "HDG %3.0f", headingDeg);
  u8g2.drawStr(0, 28, buf);

  // Distance
  snprintf(buf, sizeof(buf), "DIST %5.0f m", distanceM);
  u8g2.drawStr(0, 42, buf);

  // Altitude
  snprintf(buf, sizeof(buf), "ALT %5.0f m", altitudeM);
  u8g2.drawStr(0, 56, buf);

  // Status
  if (gps.satellites.isValid()) {
    snprintf(buf, sizeof(buf), "SAT %d", gps.satellites.value());
    u8g2.drawStr(80, 56, buf);
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
      // Enter menu stub
      u8g2.clearBuffer();
      u8g2.drawStr(20, 30, "MENU STUB");
      u8g2.sendBuffer();
      delay(1000);
      menuPress = 0;
    }
  } else {
    if (menuPress > 0 && millis() - menuPress < 1000) {
      // Short press – toggle invert
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
