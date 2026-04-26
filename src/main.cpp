#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// SD Card Pins (for future use)
// CLK: 17, CMD: 18, D0: 16, D3 (CS): 47

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32-S3 Dongle LCD Test");

    // Initialize TFT
    tft.init();
    tft.setRotation(1); // Landscape
    
    // Backlight control (GPIO 38 is common for this board)
    // If the screen stays black, try GPIO 38 or check if hardwired
    #ifdef TFT_BL
        pinMode(TFT_BL, OUTPUT);
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
    #endif

    tft.fillScreen(TFT_BLACK);

    // Draw some UI elements
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("ESP32-S3 Dongle", 80, 10, 2);
    
    tft.drawRect(0, 0, 160, 80, TFT_BLUE);
    
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawCentreString("N16R8 + ST7735", 80, 35, 2);

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawCentreString("Antigravity Ready", 80, 60, 1);

    Serial.println("Display Initialized");
}

void loop() {
    static uint32_t lastMillis = 0;
    if (millis() - lastMillis > 100) { // Check more frequently
        lastMillis = millis();
        
        // Blink a small circle to show life
        static bool state = false;
        tft.fillCircle(150, 10, 3, state ? TFT_RED : TFT_BLACK);
        state = !state;

        // Check BOOT button (GPIO 0)
        if (digitalRead(0) == LOW) {
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.drawCentreString("BOOT PRESSED", 80, 45, 1);
            Serial.println("BOOT Button Pressed!");
        } else {
            // Clear the message if not pressed
            tft.fillRect(40, 45, 80, 10, TFT_BLACK);
        }
    }
}
