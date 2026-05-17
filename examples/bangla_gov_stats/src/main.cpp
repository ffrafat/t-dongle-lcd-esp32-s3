#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <OneButton.h>
#include <FastLED.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include "pin_config.h"

// Hardware Objects
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS_PIN, TFT_DC_PIN, TFT_RES_PIN);
CRGB leds;
OneButton button(BTN_PIN, true);

// WiFi Credentials
const char* ssid     = "bangla.gov.bd";
const char* password = "11223344";

// NTP Settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 21600; // Bangladesh Time (UTC+6)
const int   daylightOffset_sec = 0;

// Dynamic Theme System
#define COLOR_BG_NIGHT      0x0000 // Black
#define COLOR_BG_MORNING    0x0010 // Dark Blue
#define COLOR_BG_DAY        0x0020 // Deep Blue  
#define COLOR_BG_EVENING    0x2010 // Dark Purple

#define COLOR_ACCENT_GOLD   0xFD20 // Gold/Orange
#define COLOR_ACCENT_CYAN   0x07FF // Cyan
#define COLOR_ACCENT_ROSE   0xF9F0 // Rose Pink
#define COLOR_ACCENT_EMERALD 0x46A0 // Emerald Green

#define COLOR_TEXT          0xFFFF // White
#define COLOR_DIM           0x6B4D // Dim Grey

uint16_t COLOR_BG = COLOR_BG_NIGHT;
uint16_t COLOR_ACCENT = COLOR_ACCENT_GOLD;

// Screen state management
enum ScreenMode {
    SCREEN_WELCOME,
    SCREEN_JULY_STATS,
    SCREEN_PURNO_STATS,
    SCREEN_SYS_STATS
};

ScreenMode currentScreen = SCREEN_WELCOME;
int screenIndex = 0;

// API Stats Variables (Volatile & Thread-Safe)
volatile int july_downloads = -1;
volatile int purno_downloads = -1;
volatile bool stats_loading = true;
volatile bool stats_error = false;
volatile bool trigger_immediate_fetch = false;

// Dynamic theme based on time of day
void updateThemeForTime(int hour) {
    if (hour >= 5 && hour < 9) {
        COLOR_BG = COLOR_BG_MORNING;
        COLOR_ACCENT = COLOR_ACCENT_GOLD;
    } else if (hour >= 9 && hour < 17) {
        COLOR_BG = COLOR_BG_DAY;
        COLOR_ACCENT = COLOR_ACCENT_CYAN;
    } else if (hour >= 17 && hour < 20) {
        COLOR_BG = COLOR_BG_EVENING;
        COLOR_ACCENT = COLOR_ACCENT_ROSE;
    } else {
        COLOR_BG = COLOR_BG_NIGHT;
        COLOR_ACCENT = COLOR_ACCENT_EMERALD;
    }
}

// WiFi signal strength indicator
void drawWiFiIndicator(int rssi) {
    int bars = 0;
    if (rssi > -50) bars = 4;
    else if (rssi > -60) bars = 3;
    else if (rssi > -70) bars = 2;
    else if (rssi > -80) bars = 1;
    
    int x = 145;
    int y = 2;
    for (int i = 0; i < 4; i++) {
        int barHeight = 3 + (i * 2);
        if (i < bars) {
            tft.fillRect(x + (i * 4), y + (10 - barHeight), 2, barHeight, COLOR_ACCENT);
        } else {
            tft.fillRect(x + (i * 4), y + (10 - barHeight), 2, barHeight, COLOR_DIM);
        }
    }
}

// Progress indicator (4 dots)
void drawProgressIndicator(int activeIndex) {
    int startX = 64;
    int y = 75;
    for (int i = 0; i < 4; i++) {
        if (i == activeIndex) {
            tft.fillCircle(startX + (i * 9), y, 3, COLOR_ACCENT);
        } else {
            tft.drawCircle(startX + (i * 9), y, 3, COLOR_DIM);
        }
    }
}

// Stream-based parser to scan massive 60KB JSON with 0 bytes overhead
int fetchDownloadCountFromAPI(const char* target_id) {
    WiFiClientSecure client;
    client.setInsecure(); // Bypass cert validation for simplicity and longevity
    client.setTimeout(8000); // 8 seconds timeout
    
    HTTPClient http;
    // Bangla government apps portal API URL
    http.begin(client, "https://bangla.gov.bd/api/bangla-gov-bd/bangla-gov-bd/");
    
    int httpCode = http.GET();
    int count = -1;
    
    if (httpCode == HTTP_CODE_OK) {
        WiFiClient* stream = http.getStreamPtr();
        String idStr = String("\"id\":\"") + target_id + "\"";
        String searchField = "\"downloaded_file_count\":";
        
        // Find the specific item ID in stream
        if (stream->find(idStr.c_str())) {
            // Find the downloaded_file_count key within this item
            if (stream->find(searchField.c_str())) {
                String valStr = "";
                while (stream->connected() && stream->available()) {
                    char c = stream->read();
                    if (c >= '0' && c <= '9') {
                        valStr += c;
                    } else if (valStr.length() > 0) {
                        break; // End of digits
                    }
                }
                if (valStr.length() > 0) {
                    count = valStr.toInt();
                }
            }
        }
    }
    http.end();
    return count;
}

// Stats fetching FreeRTOS task running on Core 0 (background)
void fetch_stats_task(void *param) {
    int timer = 0;
    while (1) {
        // Run fetch immediately on boot, every 60s, or on manual trigger
        if (timer == 0 || trigger_immediate_fetch) {
            trigger_immediate_fetch = false;
            
            if (WiFi.status() == WL_CONNECTED) {
                stats_loading = true;
                stats_error = false;
                
                // Cyan status LED for "Connecting/Loading"
                FastLED.showColor(CRGB::DarkCyan);
                
                Serial.println("[StatsTask] Fetching July font stats...");
                int july_count = fetchDownloadCountFromAPI("0e0389ae-7e84-4e45-9329-77f39fa5f94c");
                
                Serial.println("[StatsTask] Fetching Purno font stats...");
                int purno_count = fetchDownloadCountFromAPI("02b6b237-2875-44a4-80ea-e720f8d7d488");
                
                if (july_count != -1 && purno_count != -1) {
                    july_downloads = july_count;
                    purno_downloads = purno_count;
                    stats_loading = false;
                    stats_error = false;
                    
                    // Green status LED for "Success"
                    FastLED.showColor(CRGB::DarkGreen);
                    delay(500);
                    FastLED.showColor(CRGB::Black);
                } else {
                    stats_error = true;
                    stats_loading = false;
                    
                    // Red status LED for "Error"
                    FastLED.showColor(CRGB::DarkRed);
                    delay(1000);
                    FastLED.showColor(CRGB::Black);
                }
            } else {
                stats_error = true;
                stats_loading = false;
            }
            timer = 60; // Reset 60s timer
        }
        
        delay(1000);
        if (timer > 0) timer--;
    }
}

// Helper to draw centered text with custom font
void drawCenteredText(const char* text, const GFXfont* font, int16_t y, uint16_t color) {
    int16_t x1, y1;
    uint16_t w, h;
    tft.setFont(font);
    tft.setTextColor(color);
    tft.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
    tft.setCursor((160 - w) / 2, y);
    tft.print(text);
}

// Screen 1: Dashboard Cover / Welcome
void draw_screen_welcome() {
    tft.fillScreen(COLOR_BG);
    drawWiFiIndicator(WiFi.RSSI());
    
    tft.drawRoundRect(5, 5, 150, 68, 8, COLOR_ACCENT);
    tft.drawRoundRect(6, 6, 148, 66, 7, COLOR_ACCENT);
    
    // Title Line 1: "BANGLA.GOV"
    drawCenteredText("BANGLA.GOV", &FreeSansBold12pt7b, 26, COLOR_TEXT);
    
    // Title Line 2: "PORTAL STATS"
    drawCenteredText("PORTAL STATS", &FreeSansBold12pt7b, 46, COLOR_ACCENT);
    
    // English URL footer
    drawCenteredText("bangla.gov.bd", &FreeSans9pt7b, 64, COLOR_DIM);
    
    drawProgressIndicator(screenIndex);
}

// Screen 2: July Font Downloads
void draw_screen_july() {
    tft.fillScreen(COLOR_BG);
    drawWiFiIndicator(WiFi.RSSI());
    
    tft.drawRoundRect(5, 5, 150, 68, 8, COLOR_ACCENT);
    tft.drawRoundRect(6, 6, 148, 66, 7, COLOR_ACCENT);
    
    // Title: "JULY FONT"
    drawCenteredText("JULY FONT", &FreeSansBold12pt7b, 22, COLOR_TEXT);
    
    // Value: Download Count or Connection Status
    if (stats_loading && july_downloads == -1) {
        drawCenteredText("Loading...", &FreeSansBold12pt7b, 48, COLOR_DIM);
    } else if (stats_error && july_downloads == -1) {
        drawCenteredText("Conn Error", &FreeSansBold12pt7b, 48, ST77XX_RED);
    } else {
        char countBuf[16];
        snprintf(countBuf, sizeof(countBuf), "%d", july_downloads);
        drawCenteredText(countBuf, &FreeSansBold18pt7b, 48, COLOR_ACCENT);
    }
    
    // Footer: "Downloads"
    drawCenteredText("Downloads", &FreeSans9pt7b, 66, COLOR_DIM);
    
    drawProgressIndicator(screenIndex);
}

// Screen 3: Purno Font Downloads
void draw_screen_purno() {
    tft.fillScreen(COLOR_BG);
    drawWiFiIndicator(WiFi.RSSI());
    
    tft.drawRoundRect(5, 5, 150, 68, 8, COLOR_ACCENT);
    tft.drawRoundRect(6, 6, 148, 66, 7, COLOR_ACCENT);
    
    // Title: "PURNO FONT"
    drawCenteredText("PURNO FONT", &FreeSansBold12pt7b, 22, COLOR_TEXT);
    
    // Value: Download Count or Connection Status
    if (stats_loading && purno_downloads == -1) {
        drawCenteredText("Loading...", &FreeSansBold12pt7b, 48, COLOR_DIM);
    } else if (stats_error && purno_downloads == -1) {
        drawCenteredText("Conn Error", &FreeSansBold12pt7b, 48, ST77XX_RED);
    } else {
        char countBuf[16];
        snprintf(countBuf, sizeof(countBuf), "%d", purno_downloads);
        drawCenteredText(countBuf, &FreeSansBold18pt7b, 48, COLOR_ACCENT);
    }
    
    // Footer: "Downloads"
    drawCenteredText("Downloads", &FreeSans9pt7b, 66, COLOR_DIM);
    
    drawProgressIndicator(screenIndex);
}

// Screen 4: Device & System Network Diagnostics
void draw_screen_sys_stats() {
    tft.fillScreen(COLOR_BG);
    drawWiFiIndicator(WiFi.RSSI());
    
    tft.drawRoundRect(5, 5, 150, 68, 8, COLOR_ACCENT);
    tft.drawRoundRect(6, 6, 148, 66, 7, COLOR_ACCENT);
    
    tft.setFont(&FreeSans9pt7b);
    
    // Line 1: IP Address
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(12, 22);
    tft.print("IP: ");
    tft.setTextColor(COLOR_ACCENT);
    tft.print(WiFi.localIP().toString());
    
    // Line 2: Free Heap Space
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(12, 38);
    tft.print("Heap: ");
    tft.setTextColor(COLOR_ACCENT);
    tft.print(ESP.getFreeHeap() / 1024);
    tft.print(" KB");
    
    // Line 3: Target API Server IP
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(12, 54);
    tft.print("API: ");
    tft.setTextColor(COLOR_ACCENT);
    tft.print("103.48.19.234");
    
    drawProgressIndicator(screenIndex);
}

// Button click triggers manual refresh and immediately schedules API update
void button_pressed() {
    FastLED.showColor(CRGB::Gold);
    
    // Cycle screens manually on click
    screenIndex = (screenIndex + 1) % 4;
    currentScreen = (ScreenMode)screenIndex;
    
    // Trigger immediate API fetch task update
    trigger_immediate_fetch = true;
    
    delay(200);
    FastLED.showColor(CRGB::Black);
}

void button_task(void *param) {
    while (1) {
        button.tick();
        delay(50);
    }
}

void setup() {
    Serial.begin(115200);
    
    // Initialize SPI and display
    SPI.begin(TFT_SCL_PIN, -1, TFT_SDA_PIN, TFT_CS_PIN);
    tft.initR(INITR_MINI160x80);
    tft.invertDisplay(true);
    tft.setRotation(3);
    tft.fillScreen(ST77XX_BLACK);
    
    tft.setCursor(10, 30);
    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(COLOR_TEXT);
    tft.println("Connecting WiFi...");

    if (TFT_LEDA_PIN != -1) {
        pinMode(TFT_LEDA_PIN, OUTPUT);
        digitalWrite(TFT_LEDA_PIN, 1);
    }

    // Connect to target WiFi access point
    WiFi.begin(ssid, password);
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20) {
        delay(500);
        retry++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(10, 30);
        tft.println("Syncing time...");
        
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        delay(1000);
        
        // Adjust background themes based on real NTP time
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            updateThemeForTime(timeinfo.tm_hour);
        }
    } else {
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(10, 30);
        tft.setTextColor(ST77XX_RED);
        tft.println("WiFi Error");
    }

    // Setup physical RGB Status LED
    FastLED.addLeds<APA102, LED_DI_PIN, LED_CI_PIN, BGR>(&leds, 1);
    FastLED.showColor(CRGB::Black);

    // Setup input button interrupts
    button.attachClick(button_pressed);
    
    // Pin button debounce task to Core 0 (keeps rendering perfectly smooth on Core 1)
    xTaskCreatePinnedToCore(button_task, "button_task", 2048, NULL, 1, NULL, 0);
    
    // Pin HTTPS stats parsing to Core 0 (avoids screen lagging and networking blockages)
    xTaskCreatePinnedToCore(fetch_stats_task, "fetch_stats_task", 8192, NULL, 2, NULL, 0);
}

void loop() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        updateThemeForTime(timeinfo.tm_hour);
    }
    
    // Display the current active screen mode (cycles every 6 seconds)
    switch (currentScreen) {
        case SCREEN_WELCOME:
            draw_screen_welcome();
            break;
        case SCREEN_JULY_STATS:
            draw_screen_july();
            break;
        case SCREEN_PURNO_STATS:
            draw_screen_purno();
            break;
        case SCREEN_SYS_STATS:
            draw_screen_sys_stats();
            break;
    }
    
    // Auto-advance screens every 6 seconds
    delay(6000);
    screenIndex = (screenIndex + 1) % 4;
    currentScreen = (ScreenMode)screenIndex;
}
