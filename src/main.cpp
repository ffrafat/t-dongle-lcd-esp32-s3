#include <Arduino.h>
#include <TFT_eSPI.h>
#include <OneButton.h>
#include <FastLED.h>
#include "pin_config.h"

TFT_eSPI tft;
CRGB leds[1];
OneButton button(BTN_PIN, true);

// ------------------------------------------------------------------------------------

void button_task(void *param) {
  while (1) {
    button.tick();
    delay(50);
  }
}

void button_pressed() {
  FastLED.showColor(CRGB::Yellow);
  tft.fillRect(0, 0, TFT_HEIGHT, TFT_WIDTH, TFT_YELLOW);
  tft.setTextColor(TFT_BLACK, TFT_YELLOW);
  tft.drawString("YELLOW", TFT_HEIGHT/2, TFT_WIDTH/2, 4);
  Serial.println("Yellow");
}

// ------------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting T-Dongle-S3...");

  // Manual reset of the LCD
  pinMode(TFT_RES_PIN, OUTPUT);
  digitalWrite(TFT_RES_PIN, HIGH);
  delay(10);
  digitalWrite(TFT_RES_PIN, LOW);
  delay(100);
  digitalWrite(TFT_RES_PIN, HIGH);
  delay(100);

  // Initialize SPI with correct pins
  SPI.begin(TFT_SCL_PIN, -1, TFT_SDA_PIN, TFT_CS_PIN);

  // Initialize TFT
  tft.init();
  tft.setRotation(1);
  tft.setTextFont(2);
  tft.setTextDatum(CC_DATUM);
  tft.fillScreen(TFT_BLACK);
  
  // Backlight control
  pinMode(TFT_LEDA_PIN, OUTPUT);  
  digitalWrite(TFT_LEDA_PIN, HIGH); // Try HIGH first

  // Attach LED
  FastLED.addLeds<APA102, LED_DI_PIN, LED_CI_PIN, BGR>(leds, 1);  
  FastLED.setBrightness(50);

  // Attach handler to the button
  button.attachClick(button_pressed);
  button.attachLongPressStart(button_pressed);

  // Create a separate task for the button to be checked. Task is pinned to core #0
  // Note that the main loop runs on core #1. Priority of the task is set to 1, which is above the iddle task (0)
  xTaskCreatePinnedToCore(button_task, "button_task", 1024, NULL, 1, NULL, 0);
}

// ------------------------------------------------------------------------------------

void loop() {
  static bool blState = true;
  
  FastLED.showColor(CRGB::Red);  
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.drawString("RED", tft.width()/2, tft.height()/2, 4);
  Serial.println("Red");
  delay(1000);

  FastLED.showColor(CRGB::Green);
  tft.fillScreen(TFT_GREEN);
  tft.setTextColor(TFT_WHITE, TFT_GREEN);
  tft.drawString("GREEN", tft.width()/2, tft.height()/2, 4);
  Serial.println("Green");
  delay(1000);
  
  FastLED.showColor(CRGB::Blue);
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.drawString("BLUE", tft.width()/2, tft.height()/2, 4);
  Serial.println("Blue");
  delay(1000);

  // Toggle backlight to verify it works
  blState = !blState;
  digitalWrite(TFT_LEDA_PIN, blState ? HIGH : LOW);
  Serial.printf("Backlight: %s\n", blState ? "ON" : "OFF");
}
