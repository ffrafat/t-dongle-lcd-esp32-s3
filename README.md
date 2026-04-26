# ESP32-S3 Pocket Dongle LCD (N16R8)

This project is configured for the ESP32-S3 Development Board (Pocket Dongle) with a 0.96-inch ST7735 LCD.

## Hardware Specifications
- **MCU:** ESP32-S3 (Dual-core, WiFi/BT)
- **Memory:** 16MB Flash (N16), 8MB PSRAM (R8)
- **Display:** 0.96" IPS LCD (ST7735)
- **Features:** SD Card Slot, BOOT Button (GPIO 0), RGB LED (check schematic).

## Pinout Mapping

### LCD (SPI)
| Function | GPIO |
|----------|------|
| SCLK     | 10   |
| MOSI     | 11   |
| CS       | 12   |
| DC       | 13   |
| RST      | 14   |
| BL (Backlight) | 38 |

### SD Card (Full Pinout)
| Function | GPIO |
|----------|------|
| CLK      | 17   |
| CMD      | 18   |
| D0 (MISO)| 16   |
| D1       | 15   |
| D2       | 48   |
| D3 (CS)  | 47   |

### Buttons
- **BOOT:** GPIO 0

## How to Flash
1. Install [PlatformIO](https://platformio.org/).
2. Open this folder in VS Code.
3. Connect the dongle via USB.
4. If it's not recognized, hold the **BOOT** button while plugging it in or pressing Reset.
5. Click **Upload** in PlatformIO.

## Library Used
- `TFT_eSPI` by Bodmer. Configuration is handled via `build_flags` in `platformio.ini`.
