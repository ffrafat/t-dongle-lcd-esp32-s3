
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
    // Final mapping for correct landscape orientation in rotation 3
    tft.drawPixel(text_x_offset + x, text_y_offset + y, current_text_color);
}

// WiFi Credentials
const char* ssid     = "bangla.gov.bd";
const char* password = "11223344";

// NTP Settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 21600; // Bangladesh Time (UTC+6)
const int   daylightOffset_sec = 0;

// Premium Colors
#define COLOR_BG      0x0000 // Black
#define COLOR_ACCENT  0xFD20 // Gold/Orange
#define COLOR_TEXT    0xFFFF // White
#define COLOR_DAY     0x7BEF // Grey/Blue

// Bangla Translation Helpers
String banglaDigits[] = {"০", "১", "২", "৩", "৪", "৫", "৬", "৭", "৮", "৯"};
String banglaMonths[] = {"বৈশাখ", "জ্যৈষ্ঠ", "আষাঢ়", "শ্রাবণ", "ভাদ্র", "আশ্বিন", "কার্তিক", "অগ্রহায়ণ", "পৌষ", "মাঘ", "ফাল্গুন", "চৈত্র"};

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

// ------------------------------------------------------------------------------------

void draw_premium_ui() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return;
  }

  tft.fillScreen(COLOR_BG);
  
  // Draw Background Accent (Glassmorphism effect)
  tft.drawRoundRect(5, 5, 150, 70, 8, COLOR_ACCENT);
  tft.drawRoundRect(6, 6, 148, 68, 7, COLOR_ACCENT);

  // Draw Month Name (e.g., APRIL)
  char monthStringDisplay[15];
  strftime(monthStringDisplay, 15, "%B", &timeinfo);
  for(int i = 0; monthStringDisplay[i]; i++) monthStringDisplay[i] = toupper(monthStringDisplay[i]);
  
  tft.setFont(&FreeSansBold12pt7b);
  tft.setTextColor(COLOR_TEXT);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(monthStringDisplay, 0, 0, &x1, &y1, &w, &h);
  
  // If text is too wide for the screen, fall back to a smaller bold font
  if (w > 150) {
    tft.setFont(&FreeSans9pt7b);
    tft.getTextBounds(monthStringDisplay, 0, 0, &x1, &y1, &w, &h);
  }
  
  tft.setCursor((160 - w) / 2, 30);
  tft.print(monthStringDisplay);

  // Draw Date Number (e.g., 26)
  char dateString[3];
  strftime(dateString, 3, "%d", &timeinfo);
  
  tft.setFont(&FreeSansBold18pt7b);
  tft.setTextColor(COLOR_ACCENT);
  tft.getTextBounds(dateString, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((160 - w) / 2, 65);
  tft.print(dateString);

  tft.fillRect(157, 0, 3, 80, ST77XX_BLACK); // Mask glitch
}

void draw_bangla_premium_ui() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return;

  tft.fillScreen(COLOR_BG);
  tft.drawRoundRect(5, 5, 150, 70, 8, COLOR_ACCENT);
  tft.drawRoundRect(6, 6, 148, 68, 7, COLOR_ACCENT);

  BanglaDate bd = getBanglaDate(timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
  
  // 1. Draw Bangla Month (Top)
  String banMonth = banglaMonths[bd.month];
  BTTextSize size = renderer->getTextSize(banMonth.c_str());
  current_text_width = size.width;
  current_text_height = size.height;
  
  text_x_offset = (160 - size.width) / 2;
  text_y_offset = 30; // Matches premium date layout
  current_text_color = COLOR_TEXT;
  renderer->renderText(banMonth.c_str(), draw_pixel_callback);

  // 2. Draw Bangla Date (Bottom)
  String banDate = toBanglaDigits(bd.day);
  size = renderer->getTextSize(banDate.c_str());
  current_text_width = size.width;
  current_text_height = size.height;

  text_x_offset = (160 - size.width) / 2;
  text_y_offset = 65; 
  current_text_color = COLOR_ACCENT;
  renderer->renderText(banDate.c_str(), draw_pixel_callback);

  tft.fillRect(157, 0, 3, 80, ST77XX_BLACK); // Mask glitch
}

void showMarquee(String text, uint16_t color) {
  tft.setFont(&FreeSansBold12pt7b); // Match Time Slide Font
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text.c_str(), 0, 0, &x1, &y1, &w, &h);
  
  if (w <= 140) {
    tft.fillScreen(COLOR_BG);
    tft.drawRoundRect(5, 5, 150, 70, 8, COLOR_ACCENT);
    tft.setCursor((160 - w) / 2, (80 + h) / 2 - 2); 
    tft.setTextColor(color);
    tft.print(text);
    delay(6000); // Display for 6 seconds
    return;
  }

  // Scroll infinitely but bound by a 10-second slide time
  unsigned long startTime = millis();
  int x = 160;
  while (millis() - startTime < 10000) { 
    tft.fillScreen(COLOR_BG);
    tft.drawRoundRect(5, 5, 150, 70, 8, COLOR_ACCENT);
    tft.setCursor(x, (80 + h) / 2 - 2);
    tft.setTextColor(color);
    tft.print(text);
    
    tft.fillRect(157, 0, 3, 80, COLOR_BG); // Mask edge glitch
    
    x -= 4;
    if (x < -(int)w - 20) x = 160; // Wrap around
    delay(15);
  }
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

// ------------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  
  SPI.begin(TFT_SCL_PIN, -1, TFT_SDA_PIN, TFT_CS_PIN);
  
  tft.initR(INITR_MINI160x80);
  tft.invertDisplay(true);
  tft.setRotation(3);
  tft.fillScreen(COLOR_BG);
  
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
    draw_premium_ui();
  } else {
    tft.fillScreen(COLOR_BG);
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
  
  // 1. Show Time for 5 seconds with blinking colon
  for(int i=0; i<5; i++) {
    if(getLocalTime(&timeinfo)) {
      char timeBuf[10];
      // Blink effect: toggle between colon and space
      if (i % 2 == 0) strftime(timeBuf, sizeof(timeBuf), "%H:%M", &timeinfo);
      else strftime(timeBuf, sizeof(timeBuf), "%H %M", &timeinfo);
      
      tft.fillScreen(COLOR_BG);
      tft.drawRoundRect(5, 5, 150, 70, 8, COLOR_ACCENT);
      tft.setFont(&FreeSansBold18pt7b); // Bigger font
      tft.setTextColor(COLOR_TEXT);
      
      int16_t x1, y1;
      uint16_t w, h;
      tft.getTextBounds(timeBuf, 0, 0, &x1, &y1, &w, &h);
      tft.setCursor((160 - w) / 2, (80 + h) / 2 + 5); // Centered
      tft.print(timeBuf);
      tft.fillRect(157, 0, 3, 80, COLOR_BG); // Mask glitch
    }
    delay(1000);
  }

  // 2. Show Premium Date UI (Month + Date)
  if(getLocalTime(&timeinfo)) {
    draw_premium_ui();
    delay(3000);
  }

  // 3. Show Bangla Premium Date UI
  if(getLocalTime(&timeinfo)) {
    draw_bangla_premium_ui();
    delay(3000);
  }

  // 4. Show International Day if any
  if(getLocalTime(&timeinfo)) {
    const char* unDay = getUNEvent(timeinfo.tm_mon + 1, timeinfo.tm_mday);
    if (unDay != NULL) {
      showMarquee(String(unDay), COLOR_TEXT);
    }
  }
}
