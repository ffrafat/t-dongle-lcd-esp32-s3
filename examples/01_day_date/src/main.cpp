#include <Arduino.h>
#include <TFT_eSPI.h>
#include <OneButton.h>
#include <FastLED.h>
#include <time.h>
#include "pin_config.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img = TFT_eSprite(&tft);
CRGB leds[1];
OneButton button(BTN_PIN, true);

// Time variables
struct tm timeinfo;
unsigned long lastUpdate = 0;
bool darkMode = true;

// Colors
#define CLR_PRIMARY    0x03EF // Nice Cyan
#define CLR_ACCENT     0xFBE0 // Soft Orange
#define CLR_BG_DARK    TFT_BLACK
#define CLR_BG_LIGHT   0xE73F // Light Grayish Blue
#define CLR_TEXT_DARK  TFT_WHITE
#define CLR_TEXT_LIGHT 0x2104 // Very Dark Blue

// ------------------------------------------------------------------------------------

void updateTime() {
    unsigned long now = millis();
    if (now - lastUpdate >= 1000) {
        timeinfo.tm_sec++;
        mktime(&timeinfo); // Normalize the struct (handles overflow of secs/mins/etc)
        lastUpdate = now;
    }
}

const char* getDayName(int wday) {
    const char* days[] = {"SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"};
    return days[wday % 7];
}

const char* getMonthName(int mon) {
    const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
    return months[mon % 12];
}

void drawUI() {
    uint16_t bg = darkMode ? CLR_BG_DARK : CLR_BG_LIGHT;
    uint16_t text = darkMode ? CLR_TEXT_DARK : CLR_TEXT_LIGHT;
    uint16_t accent = CLR_PRIMARY;

    img.fillSprite(bg);

    // Draw Day Name (Top)
    img.setTextColor(accent, bg);
    img.setTextDatum(TC_DATUM);
    img.drawString(getDayName(timeinfo.tm_wday), 80, 5, 2);

    // Draw Time (Center)
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    img.setTextColor(text, bg);
    img.setTextDatum(CC_DATUM);
    img.drawString(timeStr, 80, 40, 4);

    // Draw Date (Bottom)
    char dateStr[20];
    sprintf(dateStr, "%02d %s %d", timeinfo.tm_mday, getMonthName(timeinfo.tm_mon), timeinfo.tm_year + 1900);
    img.setTextColor(darkMode ? 0x7BEF : 0x4208, bg); // Subtitle color
    img.setTextDatum(BC_DATUM);
    img.drawString(dateStr, 80, 75, 2);

    // Decorative line
    img.drawFastHLine(20, 25, 120, accent);
    img.drawFastHLine(20, 58, 120, accent);

    img.pushSprite(0, 0);
}

void button_pressed() {
    darkMode = !darkMode;
    Serial.println(darkMode ? "Switched to Dark Mode" : "Switched to Light Mode");
    
    // Quick flash to confirm press
    FastLED.showColor(CRGB::White);
    delay(50);
}

void button_task(void *param) {
    while (1) {
        button.tick();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ------------------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting Day/Date Example...");

    // Initialize Time from Build Time
    // __DATE__ is "Mmm dd yyyy", __TIME__ is "hh:mm:ss"
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char m[4];
    int d, y, hh, mm, ss;
    sscanf(__DATE__, "%s %d %d", m, &d, &y);
    sscanf(__TIME__, "%d:%d:%d", &hh, &mm, &ss);
    
    timeinfo.tm_mday = d;
    timeinfo.tm_year = y - 1900;
    timeinfo.tm_hour = hh;
    timeinfo.tm_min = mm;
    timeinfo.tm_sec = ss;
    
    for (int i = 0; i < 12; i++) {
        if (strcmp(m, months[i]) == 0) {
            timeinfo.tm_mon = i;
            break;
        }
    }
    mktime(&timeinfo); // This also calculates tm_wday correctly

    // Manual reset of the LCD
    pinMode(TFT_RES_PIN, OUTPUT);
    digitalWrite(TFT_RES_PIN, HIGH);
    delay(10);
    digitalWrite(TFT_RES_PIN, LOW);
    delay(100);
    digitalWrite(TFT_RES_PIN, HIGH);
    delay(100);

    // Initialize SPI and TFT
    SPI.begin(TFT_SCL_PIN, -1, TFT_SDA_PIN, TFT_CS_PIN);
    tft.init();
    tft.setRotation(1);
    
    // Create Sprite
    img.createSprite(160, 80);
    
    // Backlight
    pinMode(TFT_LEDA_PIN, OUTPUT);  
    digitalWrite(TFT_LEDA_PIN, HIGH);

    // LED
    FastLED.addLeds<APA102, LED_DI_PIN, LED_CI_PIN, BGR>(leds, 1);
    FastLED.setBrightness(50);

    // Button
    button.attachClick(button_pressed);
    xTaskCreatePinnedToCore(button_task, "btn", 2048, NULL, 1, NULL, 0);
}

void loop() {
    updateTime();
    drawUI();

    // LED Pulse Effect
    static uint8_t hue = 0;
    static uint8_t br = 0;
    static int8_t dir = 1;
    
    br += dir;
    if (br == 0 || br == 100) dir = -dir;
    
    CRGB color = darkMode ? CRGB::Cyan : CRGB::DeepPink;
    FastLED.showColor(color.fadeToBlackBy(255 - br));

    delay(20);
}
