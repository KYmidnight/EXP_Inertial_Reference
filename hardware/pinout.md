# Pin Assignments – ESP32-S3 N16R8

Safe GPIOs only (avoids PSRAM 35–37, USB 19/20, and strapping pins).

| Function              | GPIO | Direction | Notes                    |
|-----------------------|------|-----------|--------------------------|
| OLED SPI SCK          | 12   | Out       |                          |
| OLED SPI MOSI         | 11   | Out       |                          |
| OLED SPI CS           | 10   | Out       |                          |
| OLED SPI DC           | 13   | Out       |                          |
| OLED SPI RST          | 14   | Out       |                          |
| IMU I2C SDA           | 8    | I/O       | Wire.begin(8, 9)         |
| IMU I2C SCL           | 9    | I/O       |                          |
| GPS UART RX (← GPS TX)| 16   | In        | Serial1                  |
| GPS UART TX (→ GPS RX)| 17   | Out       | Optional for config      |
| Button Menu           | 4    | In        | INPUT_PULLUP, to GND     |
| Button Reset          | 5    | In        | INPUT_PULLUP, to GND     |

All modules share GND. Power from 3.3 V or 5 V as appropriate for each module.
