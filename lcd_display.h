#ifndef LCDDISPLAY_H
#define LCDDISPLAY_H

#define C_BLACK 0x0000
#define C_WHITE 0xFFFF
#define C_RED 0xF800
#define C_GREEN 0x07E0
#define C_BLUE 0x001F
#define C_CYAN 0x07FF
#define C_MAGENTA 0xF81F
#define C_YELLOW 0xFFE0
#define C_ORANGE 0xFC00

enum FontStyle {
  Font9pt,
  Font12pt,
  Font18pt,
  Font24pt
};

extern void LCD_init(void);
extern void LCD_clear(void);
extern void LCD_textSize(int txtSize);
extern void LCD_color(uint16_t c);
extern void LCD_write(String msg);
extern void LCD_setFont(FontStyle id);
extern void LCD_setX(int x);
extern int LCD_getX(void);
extern int LCD_getY(void);
extern void LCD_setCursor(int x, int y);
extern void LCD_writeCentered(String msg, int y);
extern void LCD_clearStringArea(String msg);
extern uint16_t LCD_getBgColor(void);
extern void LCD_setBgColor(uint16_t color);
extern uint16_t LCD_getFgColor(void);
extern void LCD_setFgColor(uint16_t color);
extern void LCD_setInverted(bool inverted);
extern void LCD_rotate180(void);
extern bool LCD_isRotated180(void);
// Persistence hook into wifi_connection.cpp's shared WIFI_CFG_PATH content,
// same pattern as theme.h/storage_mode.h.
extern void LCD_appendConfigLines(String& content);
extern bool LCD_tryParseConfigLine(const String& key, const String& val);
extern void LCD_setBacklight(uint8_t percent);
extern void LCD_flashBacklight(void);
extern void LCD_busRelease(void);
extern void LCD_busAcquire(void);
extern void LCD_imageBegin(void);
extern void LCD_imagePush(const uint16_t *buf, uint32_t n);
extern void LCD_imageEnd(void);
extern void LCD_process(void);
extern void LCD_showProgress(uint8_t percent);  // draw/update a thin bar at the screen bottom
extern void LCD_hideProgress(void);             // erase it (overwrite with background)

#endif
