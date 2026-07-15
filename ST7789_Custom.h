#ifndef ST7789_CUSTOM_H
#define ST7789_CUSTOM_H

#include <Adafruit_GFX.h>
#include <SPI.h>
#include "config.h"

// 172x320 panel is centered on the ST7789 240x320 GRAM -> 34px column offset.
#define X_OFFSET_DEFAULT 34
#define Y_OFFSET_DEFAULT 0

class ST7789_Custom : public Adafruit_GFX {
  public:
    ST7789_Custom() : Adafruit_GFX(SCREEN_W, SCREEN_H) {
        _x_offset = X_OFFSET_DEFAULT;
        _y_offset = Y_OFFSET_DEFAULT;
    }

    void begin() {
      pinMode(TFT_DC, OUTPUT);
      pinMode(TFT_RST, OUTPUT);
      pinMode(TFT_CS, OUTPUT);
      digitalWrite(TFT_CS, LOW);   // single SPI device, keep selected

      // Backlight via PWM (Arduino-ESP32 v3.x ledc API).
      ledcAttach(TFT_BL, TFT_BL_FREQ, TFT_BL_RES_BITS);
      ledcWrite(TFT_BL, TFT_BL_DUTY);

      // Route hardware SPI to the board pins. The LCD itself never reads, so
      // MISO is left unconnected.
      SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
      SPI.setFrequency(24000000);
      SPI.setDataMode(SPI_MODE3);

      digitalWrite(TFT_RST, HIGH); delay(50);
      digitalWrite(TFT_RST, LOW);  delay(50);
      digitalWrite(TFT_RST, HIGH); delay(150);

      sendCmd(0x01); delay(150); // Software Reset
      sendCmd(0x11); delay(150); // Exit Sleep
      sendCmd(0x3A); sendData(0x05); // 16-bit color
      sendCmd(0x21);                 // Inversion ON (required for this IPS panel)
      setRotation(0);                // Set initial orientation
      sendCmd(0x13); // Normal mode
      sendCmd(0x29); // Display on
    }

    void setRotation(uint8_t m) override {
      Adafruit_GFX::setRotation(m);
      sendCmd(0x36); // MADCTL
      switch (rotation) {
        case 0: // Portrait
          sendData(0x00);
          _width  = SCREEN_W;
          _height = SCREEN_H;
          _x_offset = X_OFFSET_DEFAULT;
          _y_offset = Y_OFFSET_DEFAULT;
          break;
        case 1: // Landscape (90 deg)
          sendData(0x60);
          _width  = SCREEN_H;
          _height = SCREEN_W;
          _x_offset = Y_OFFSET_DEFAULT;
          _y_offset = X_OFFSET_DEFAULT;
          break;
        case 2: // Portrait Inverted
          sendData(0xC0);
          _width  = SCREEN_W;
          _height = SCREEN_H;
          _x_offset = X_OFFSET_DEFAULT;
          _y_offset = Y_OFFSET_DEFAULT;
          break;
        case 3: // Landscape Inverted
          sendData(0xA0);
          _width  = SCREEN_H;
          _height = SCREEN_W;
          _x_offset = Y_OFFSET_DEFAULT;
          _y_offset = X_OFFSET_DEFAULT;
          break;
      }
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
      if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;
      setAddrWindow(x, y, 1, 1);
      SPI.write16(color);
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
      if ((x >= _width) || (y >= _height)) return;
      setAddrWindow(x, y, w, h);
      for (uint32_t i = 0; i < (uint32_t)w * h; i++) SPI.write16(color);
    }

    // --- Shared SPI bus arbitration (LCD + microSD on one bus) ---
    // The LCD is normally the only device and is kept selected. When the SD card
    // needs the bus, release it; before driving the LCD again, re-acquire it
    // (the SD library changes the SPI clock/mode, so they must be restored).
    void busRelease() { digitalWrite(TFT_CS, HIGH); }
    void busAcquire() {
      digitalWrite(TFT_CS, LOW);
      // On the ESP32-C6, setFrequency()/setDataMode() do not sync to hardware,
      // but the SD library's transactions do (cmd.update). A begin/endTransaction
      // pair forces the config to actually latch again. The panel effectively
      // runs in MODE0 (the driver's MODE3 setter never latched), so restore MODE0.
      SPI.beginTransaction(SPISettings(24000000, MSBFIRST, SPI_MODE0));
      SPI.endTransaction();
      digitalWrite(TFT_DC, HIGH);
    }

    // Open a full-screen pixel write (RAMWR). The GRAM pointer auto-increments,
    // so callers may release/re-acquire the bus between pushPixels() batches.
    void imageWindow() {
      busAcquire();
      setAddrWindow(0, 0, _width, _height);
    }
    void pushPixels(const uint16_t *buf, uint32_t n) {
      for (uint32_t i = 0; i < n; i++) SPI.write16(buf[i]);
    }

  private:
    uint16_t _x_offset, _y_offset;

    void sendCmd(uint8_t c) { digitalWrite(TFT_DC, LOW); SPI.write(c); }
    void sendData(uint8_t d) { digitalWrite(TFT_DC, HIGH); SPI.write(d); }

    void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
      uint16_t x0 = x + _x_offset, x1 = x + w - 1 + _x_offset;
      uint16_t y0 = y + _y_offset, y1 = y + h - 1 + _y_offset;
      sendCmd(0x2A); sendData(x0 >> 8); sendData(x0 & 0xFF); sendData(x1 >> 8); sendData(x1 & 0xFF);
      sendCmd(0x2B); sendData(y0 >> 8); sendData(y0 & 0xFF); sendData(y1 >> 8); sendData(y1 & 0xFF);
      sendCmd(0x2C); digitalWrite(TFT_DC, HIGH);
    }
};

#endif
