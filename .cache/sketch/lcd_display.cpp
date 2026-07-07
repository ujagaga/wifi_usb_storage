#line 1 "/home/rada/Projects/wifi_usb_storage/lcd_display.cpp"
#include "config.h"

#ifdef USE_ADAFRUIT_ST7789
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
#else
#include "ST7789_Custom.h"

ST7789_Custom tft;
#endif

#include <Fonts/FreeMonoBold9pt7b.h> 
#include <Fonts/FreeMonoBold12pt7b.h> 
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include "lcd_display.h"


static uint16_t bgColor = C_BLACK;
static uint16_t fgColor = C_YELLOW;
static bool invertedColors = false;
static uint8_t currentRotation = 0;
static uint32_t currentDuty = TFT_BL_DUTY;
static uint32_t lastBacklightChange = 0;

static uint16_t invertColor(uint16_t color) {
  return ~color;
}

#ifdef USE_ADAFRUIT_ST7789
void LCD_init()
{
  ledcAttach(TFT_BL, TFT_BL_FREQ, TFT_BL_RES_BITS);
  ledcWrite(TFT_BL, TFT_BL_DUTY);
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS); // route hardware SPI to board pins
  tft.init(SCREEN_W, SCREEN_H, SPI_MODE3);
  currentRotation = 0;
  tft.setRotation(currentRotation);

  tft.fillScreen(bgColor);
  tft.setCursor(0, 0);  
  tft.setTextSize(1);
  tft.setTextColor(fgColor);
}
#else
void LCD_init() 
{
  tft.begin();
  // Set to landscape for a wide "bar" look. Use 0 or 2 for portrait.
  currentRotation = 1;
  tft.setRotation(currentRotation);

  tft.fillScreen(bgColor);
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  tft.setTextColor(fgColor);

}
#endif



void LCD_setFont(FontStyle id){
  if(id == Font24pt){
    tft.setFont(&FreeMonoBold24pt7b);
  }else if(id == Font18pt){
    tft.setFont(&FreeMonoBold18pt7b);
  }else if(id == Font12pt){
    tft.setFont(&FreeMonoBold12pt7b);
  }else{
    tft.setFont(&FreeMonoBold9pt7b); 
  }
}

void LCD_clear()
{
  tft.fillScreen(bgColor);
  tft.setCursor(0, 0);
  tft.setTextColor(fgColor);
  tft.setTextSize(1);
  LCD_setFont(Font9pt);
}

void LCD_setX(int x){
  int y = tft.getCursorY();
  tft.setCursor(x, y);
}

int LCD_getX(void){
  return tft.getCursorX();
}

int LCD_getY(void){
  return tft.getCursorY();
}

void LCD_setCursor(int x, int y){
  tft.setCursor(x, y);
}

// Write msg horizontally centered on the current screen at baseline y.
void LCD_writeCentered(String msg, int y){
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(msg, 0, y, &x1, &y1, &w, &h);
  int x = (tft.width() - (int)w) / 2;
  tft.setCursor(x, y);
  tft.print(msg);
}

void LCD_textSize(int txtSize)
{
  tft.setTextSize(txtSize);
}

void LCD_color(uint16_t c)
{
  tft.setTextColor(c);
}

uint16_t LCD_getBgColor(void){
  return bgColor;
}

void LCD_setBgColor(uint16_t color){
  bgColor = color;
}

uint16_t LCD_getFgColor(void){
  return fgColor;
}

void LCD_setFgColor(uint16_t color){
  fgColor = color;
}

// NOTE: FreeType fonts draw on top of base line, so at coordinates 0,0 the text starts outside the screen.
// First set font size and write a new line character. This will set the cursor at the right place for first row.
void LCD_write(String msg)
{
  tft.print(msg);
}

void LCD_clearStringArea(String msg) {
  int16_t x1, y1;
  uint16_t w, h;
  
  // This calculates the bounding box of the string
  tft.getTextBounds(msg, tft.getCursorX(), tft.getCursorY(), &x1, &y1, &w, &h);
  
  // Fill that box with black
  tft.fillRect(x1, y1, w, h, C_BLACK);
}

// Set backlight brightness. percent (0..100) maps to 0..50% PWM duty, since the
// panel docs warn against sustained full brightness.
void LCD_setBacklight(uint8_t percent){
  if(percent > 100){
    percent = 100;
  }
  uint32_t maxDuty = (1u << TFT_BL_RES_BITS) - 1;
  currentDuty = ((uint32_t)percent * (maxDuty / 2)) / 100;
  ledcWrite(TFT_BL, currentDuty);
}

// Flash to full brightness (bypassing the 50% cap) for 500ms, then restore.
void LCD_flashBacklight(void){
  uint32_t maxDuty = (1u << TFT_BL_RES_BITS) - 1;
  ledcWrite(TFT_BL, maxDuty);
  lastBacklightChange = millis();  
}

// Flip the screen 180 degrees (toggle between the rotation and its opposite).
void LCD_rotate180(void){
  currentRotation ^= 2;
  tft.setRotation(currentRotation);
  tft.fillScreen(bgColor);
}

// --- Shared SPI bus + raw image blit ---
// The microSD card shares the LCD's SPI bus. These wrappers let sd_images.cpp
// release the bus for SD reads and push raw RGB565 pixels to the panel. The
// GRAM pointer auto-increments, so reads and pixel pushes can interleave.
void LCD_busRelease(void){
#ifdef USE_ADAFRUIT_ST7789
  tft.endWrite();
#else
  tft.busRelease();
#endif
}

void LCD_busAcquire(void){
#ifdef USE_ADAFRUIT_ST7789
  tft.startWrite();
#else
  tft.busAcquire();
#endif
}

void LCD_imageBegin(void){
#ifdef USE_ADAFRUIT_ST7789
  tft.startWrite();
  tft.setAddrWindow(0, 0, tft.width(), tft.height());
  tft.endWrite();
#else
  tft.imageWindow();
  tft.busRelease();
#endif
}

void LCD_imagePush(const uint16_t *buf, uint32_t n){
#ifdef USE_ADAFRUIT_ST7789
  tft.startWrite();
  tft.writePixels((uint16_t *)buf, n);
  tft.endWrite();
#else
  tft.busAcquire();
  tft.pushPixels(buf, n);
  tft.busRelease();
#endif
}

void LCD_imageEnd(void){
#ifndef USE_ADAFRUIT_ST7789
  tft.busAcquire();   // restore the LCD-owns-the-bus invariant for clock drawing
#endif
}

void LCD_setInverted(bool inverted){
  if(invertedColors != inverted){
    invertedColors = inverted;
    bgColor = invertColor(bgColor);
    fgColor = invertColor(fgColor);
  }
}

void LCD_process() {
  if(lastBacklightChange > 0 && (millis() - lastBacklightChange) > 500) {
    ledcWrite(TFT_BL, currentDuty);
    lastBacklightChange = 0;
  }
}