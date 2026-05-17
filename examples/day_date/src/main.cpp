
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
#include <time.h>
#include <BanglaText.h>
#include "BanglaMNRegular_33pt.h"
#include "pin_config.h"
#include "un_days.h"

// Hardware Objects
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS_PIN, TFT_DC_PIN, TFT_RES_PIN);
CRGB leds;
OneButton button(BTN_PIN, true);

// Bangla Rendering
BTfont font = BanglaMNRegular_33pt;
BanglaTextRenderer *renderer;
int16_t text_x_offset = 0;
int16_t text_y_offset = 0;
uint16_t current_text_color = ST77XX_WHITE;
int16_t current_text_width = 0;
int16_t current_text_height = 0;

void draw_pixel_callback(int16_t x, int16_t y) {
    tft.drawPixel(text_x_offset + x, text_y_offset + y, current_text_color);
}

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

// Bangla Translation Helpers
String banglaDigits[] = {"০", "১", "২", "৩", "৪", "৫", "৬", "৭", "৮", "৯"};
String banglaMonths[] = {"বৈশাখ", "জ্যৈষ্ঠ", "আষাঢ়", "শ্রাবণ", "ভাদ্র", "আশ্বিন", "কার্তিক", "অগ্রহায়ণ", "পৌষ", "মাঘ", "ফাল্গুন", "চৈত্র"};
String banglaDays[] = {"রবিবার", "সোমবার", "মঙ্গলবার", "বুধবার", "বৃহস্পতিবার", "শুক্রবার", "শনিবার"};

String toBanglaDigits(int num) {
    String s = String(num);
    String res = "";
    for(int i=0; i<s.length(); i++) {
        if(s[i] >= '0' && s[i] <= '9') res += banglaDigits[s[i]-'0'];
        else res += s[i];
    }
    return res;
}

struct BanglaDate {
    int day;
    int month;
    int year;
};

BanglaDate getBanglaDate(int d, int m, int y) {
    BanglaDate bd;
    bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    if (m > 4 || (m == 4 && d >= 14)) bd.year = y - 593;
    else bd.year = y - 594;

    int sm[] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 1, 2, 3};
    int sd[] = {14, 15, 15, 16, 16, 16, 16, 15, 15, 14, (leap ? 14 : 13), 15};

    int monthIdx = -1;
    for (int i = 0; i < 12; i++) {
        int startM = sm[i];
        int startD = sd[i];
        int nextM = sm[(i + 1) % 12];
        int nextD = sd[(i + 1) % 12];
        bool isCurrent = false;
        if (startM <= nextM) {
            if ((m > startM || (m == startM && d >= startD)) && (m < nextM || (m == nextM && d < nextD))) isCurrent = true;
        } else {
            if ((m == startM && d >= startD) || (m == nextM && d < nextD) || (m > startM) || (m < nextM)) {
                if (startM == 12 && nextM == 1) {
                    if ((m == 12 && d >= 15) || (m == 1 && d < 14)) isCurrent = true;
                } else if (startM == 3 && nextM == 4) {
                     if ((m == 3 && d >= 15) || (m == 4 && d < 14)) isCurrent = true;
                }
            }
        }
        if (isCurrent) { monthIdx = i; break; }
    }
    if (monthIdx == -1) monthIdx = 11; 
    bd.month = monthIdx;

    if (m == sm[monthIdx]) bd.day = d - sd[monthIdx] + 1;
    else {
        int prevMonthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (leap) prevMonthDays[1] = 29;
        int pMonth = (m == 1) ? 12 : m - 1;
        bd.day = (prevMonthDays[pMonth-1] - sd[monthIdx] + 1) + d;
    }
    return bd;
}

// Screen state management
enum ScreenMode {
    SCREEN_TIME,
    SCREEN_DATE_EN,
    SCREEN_DATE_BN,
    SCREEN_EVENT
};

ScreenMode currentScreen = SCREEN_TIME;
int screenIndex = 0;

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

// Premium UI with enhancements
void draw_premium_ui() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return;

    tft.fillScreen(COLOR_BG);
    
    // WiFi indicator
    drawWiFiIndicator(WiFi.RSSI());
    
    // Draw Month Name
    char monthStringDisplay[15];
    strftime(monthStringDisplay, 15, "%B", &timeinfo);
    for(int i = 0; monthStringDisplay[i]; i++) monthStringDisplay[i] = toupper(monthStringDisplay[i]);
    
    tft.setFont(&FreeSansBold12pt7b);
    tft.setTextColor(COLOR_TEXT);
    int16_t x1, y1;
    uint16_t w, h;
    tft.getTextBounds(monthStringDisplay, 0, 0, &x1, &y1, &w, &h);
    
    if (w > 150) {
        tft.setFont(&FreeSans9pt7b);
        tft.getTextBounds(monthStringDisplay, 0, 0, &x1, &y1, &w, &h);
    }
    
    tft.setCursor((160 - w) / 2, 28);
    tft.print(monthStringDisplay);

    // Draw Date Number
    char dateString[3];
    strftime(dateString, 3, "%d", &timeinfo);
    
    tft.setFont(&FreeSansBold18pt7b);
    tft.setTextColor(COLOR_ACCENT);
    tft.getTextBounds(dateString, 0, 0, &x1, &y1, &w, &h);
    tft.setCursor((160 - w) / 2, 62);
    tft.print(dateString);

    // Animated border
    tft.drawRoundRect(5, 5, 150, 68, 8, COLOR_ACCENT);
    tft.drawRoundRect(6, 6, 148, 66, 7, COLOR_ACCENT);
    
    // Progress indicator
    drawProgressIndicator(screenIndex);
}

// Bangla Premium UI with day name - FIXED orientation
void draw_bangla_premium_ui() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return;

    tft.fillScreen(COLOR_BG);
    drawWiFiIndicator(WiFi.RSSI());
    
    BanglaDate bd = getBanglaDate(timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
    
    // Draw Bangla Day Name (Top) - smaller and higher
    String banDay = banglaDays[timeinfo.tm_wday];
    BTTextSize daySize = renderer->getTextSize(banDay.c_str());
    text_x_offset = (160 - daySize.width) / 2;
    text_y_offset = 16;
    current_text_color = COLOR_DIM;
    renderer->renderText(banDay.c_str(), draw_pixel_callback);
    
    // Draw Bangla Month (Middle) - adjusted position
    String banMonth = banglaMonths[bd.month];
    BTTextSize size = renderer->getTextSize(banMonth.c_str());
    text_x_offset = (160 - size.width) / 2;
    text_y_offset = 34;
    current_text_color = COLOR_TEXT;
    renderer->renderText(banMonth.c_str(), draw_pixel_callback);

    // Draw Bangla Date (Bottom) - larger and lower
    String banDate = toBanglaDigits(bd.day);
    size = renderer->getTextSize(banDate.c_str());
    text_x_offset = (160 - size.width) / 2;
    text_y_offset = 56;
    current_text_color = COLOR_ACCENT;
    renderer->renderText(banDate.c_str(), draw_pixel_callback);

    // Border
    tft.drawRoundRect(5, 5, 150, 68, 8, COLOR_ACCENT);
    tft.drawRoundRect(6, 6, 148, 66, 7, COLOR_ACCENT);
    
    // Progress indicator
    drawProgressIndicator(screenIndex);
}

// Draw wrapped paragraph text (smaller font, no marquee)
void showUNEvent(String text, uint16_t color) {
    tft.fillScreen(COLOR_BG);
    drawWiFiIndicator(WiFi.RSSI());
    tft.drawRoundRect(5, 5, 150, 68, 8, COLOR_ACCENT);
    tft.drawRoundRect(6, 6, 148, 66, 7, COLOR_ACCENT);
    
    // Use smaller font for paragraph style
    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(color);
    
    // Word wrap the text
    int16_t x1, y1;
    uint16_t w, h;
    int margin = 12;
    int maxWidth = 160 - (margin * 2);
    int yPos = 18;
    int lineHeight = 16;
    
    String word = "";
    String line = "";
    
    for (int i = 0; i < text.length(); i++) {
        char c = text[i];
        
        if (c == ' ' || c == '\0') {
            // Test if adding this word exceeds width
            String testLine = line + word;
            tft.getTextBounds(testLine.c_str(), 0, 0, &x1, &y1, &w, &h);
            
            if (w > maxWidth && line.length() > 0) {
                // Draw current line
                tft.getTextBounds(line.c_str(), 0, 0, &x1, &y1, &w, &h);
                tft.setCursor(margin, yPos);
                tft.print(line);
                
                // Move to next line
                line = word;
                yPos += lineHeight;
                
                // Stop if we run out of space
                if (yPos > 65) break;
            } else {
                line = testLine;
            }
            word = "";
        } else {
            word += c;
        }
    }
    
    // Draw remaining text
    if (line.length() > 0 && yPos <= 65) {
        tft.getTextBounds(line.c_str(), 0, 0, &x1, &y1, &w, &h);
        tft.setCursor(margin, yPos);
        tft.print(line);
    }
    
    drawProgressIndicator(screenIndex);
    delay(6000);
}

void button_task(void *param) {
    while (1) {
        button.tick();
        delay(50);
    }
}

void button_pressed() {
    FastLED.showColor(CRGB::Gold);
    delay(200);
    FastLED.showColor(CRGB::Black);
}

void setup() {
    Serial.begin(115200);
    
    SPI.begin(TFT_SCL_PIN, -1, TFT_SDA_PIN, TFT_CS_PIN);
    
    tft.initR(INITR_MINI160x80);
    tft.invertDisplay(true);
    tft.setRotation(3);
    tft.fillScreen(ST77XX_BLACK);
    
    tft.setCursor(10, 30);
    tft.setTextColor(COLOR_TEXT);
    tft.println("Syncing Time...");

    if (TFT_LEDA_PIN != -1) {
        pinMode(TFT_LEDA_PIN, OUTPUT);
        digitalWrite(TFT_LEDA_PIN, 1);
    }

    WiFi.begin(ssid, password);
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20) {
        delay(500);
        retry++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        delay(1000);
        renderer = new BanglaTextRenderer(&font);
        
        // Initial theme
        struct tm timeinfo;
        getLocalTime(&timeinfo);
        updateThemeForTime(timeinfo.tm_hour);
        
        draw_premium_ui();
    } else {
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(10, 30);
        tft.setTextColor(ST77XX_RED);
        tft.println("WiFi Error");
    }

    FastLED.addLeds<APA102, LED_DI_PIN, LED_CI_PIN, BGR>(&leds, 1);
    FastLED.showColor(CRGB::Black);

    button.attachClick(button_pressed);
    xTaskCreatePinnedToCore(button_task, "button_task", 2048, NULL, 1, NULL, 0);
}

void loop() {
    struct tm timeinfo;
    
    // Update theme based on current hour
    getLocalTime(&timeinfo);
    updateThemeForTime(timeinfo.tm_hour);
    
    // 1. Show Time for 5 seconds with blinking colon
    currentScreen = SCREEN_TIME;
    screenIndex = 0;
    for(int i=0; i<5; i++) {
        if(getLocalTime(&timeinfo)) {
            char timeBuf[10];
            if (i % 2 == 0) strftime(timeBuf, sizeof(timeBuf), "%H:%M", &timeinfo);
            else strftime(timeBuf, sizeof(timeBuf), "%H %M", &timeinfo);
            
            tft.fillScreen(COLOR_BG);
            drawWiFiIndicator(WiFi.RSSI());
            tft.drawRoundRect(5, 5, 150, 68, 8, COLOR_ACCENT);
            tft.drawRoundRect(6, 6, 148, 66, 7, COLOR_ACCENT);
            tft.setFont(&FreeSansBold18pt7b);
            tft.setTextColor(COLOR_TEXT);
            
            int16_t x1, y1;
            uint16_t w, h;
            tft.getTextBounds(timeBuf, 0, 0, &x1, &y1, &w, &h);
            tft.setCursor((160 - w) / 2, (75 + h) / 2 + 5);
            tft.print(timeBuf);
            drawProgressIndicator(screenIndex);
        }
        delay(1000);
    }

    // 2. Show Premium Date UI (Month + Date)
    currentScreen = SCREEN_DATE_EN;
    screenIndex = 1;
    if(getLocalTime(&timeinfo)) {
        draw_premium_ui();
        delay(3000);
    }

    // 3. Show Bangla Premium Date UI with day name
    currentScreen = SCREEN_DATE_BN;
    screenIndex = 2;
    if(getLocalTime(&timeinfo)) {
        draw_bangla_premium_ui();
        delay(3000);
    }

    // 4. Show International Day as paragraph (no marquee)
    currentScreen = SCREEN_EVENT;
    screenIndex = 3;
    if(getLocalTime(&timeinfo)) {
        const char* unDay = getUNEvent(timeinfo.tm_mon + 1, timeinfo.tm_mday);
        if (unDay != NULL) {
            showUNEvent(String(unDay), COLOR_TEXT);
        }
    }
}
