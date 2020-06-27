#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#include "Display.h"


//PIN RELAY CONTROLS 32,15,33,27
// PIN MODE INDICATOR 14 

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

int backLightPin ;

void initDisplay(int rotation, int blPin) {
  
  backLightPin = blPin ;
  pinMode(backLightPin, OUTPUT);

  tft.begin();
  setBackLight(HIGH) ;
  
    
  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 
  tft.setRotation(rotation);

}

void setBackLight(int val) {
   digitalWrite(backLightPin, val);
}
void displayClear() {
    tft.fillScreen(ILI9341_WHITE);
    setTextSize(2);
}
void setTextSize(int sz) {
     tft.setTextSize(sz);
}
void displayMessage(char *str, int x, int y) {
    tft.setCursor(x, y);
    tft.setTextColor(ILI9341_BLACK);  
    tft.print(str);
}
void displayMessage(char *str, int x, int y, int col) {
    tft.setCursor(x, y);
    tft.setTextColor(col);  
    tft.print(str);
}
void statusmessage(char *str) {
    displayClear() ;
    displayMessage(str, 0, 0) ;
}

unsigned long testText() {
  tft.fillScreen(ILI9341_BLACK);
  unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(ILI9341_YELLOW); tft.setTextSize(2);
  tft.println(1234.56);
  tft.setTextColor(ILI9341_RED);    tft.setTextSize(3);
  tft.println(0xDEADBEEF, HEX);
  tft.println();
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(5);
  tft.println("Groop");
  tft.setTextSize(2);
  tft.println("I implore thee,");
  tft.setTextSize(1);
  tft.println("my foonting turlingdromes.");
  tft.println("And hooptiously drangle me");
  tft.println("with crinkly bindlewurdles,");
  tft.println("Or I will rend thee");
  tft.println("in the gobberwarts");
  tft.println("with my blurglecruncheon,");
  tft.println("see if I don't!");
  return micros() - start;
}
