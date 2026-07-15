#ifndef THEME_H
#define THEME_H

#include <Arduino.h>

// Web UI theme colors (hex "#rrggbb"), overridable via KEY=VALUE lines in
// WIFI_CFG_PATH. Persistence itself (reading/writing that file) is owned by
// wifi_connection.cpp, since it's one shared file with the WiFi credentials
// and brightness - these two functions are its hook into the theme's part
// of that file's content.
extern void THEME_appendConfigLines(String& content);       // appends this module's KEY=VALUE lines
extern bool THEME_tryParseConfigLine(const String& key, const String& val);  // true if recognized/handled

extern String THEME_getCSSVars(void);   // ":root" CSS custom properties for the UI theme colors
extern void THEME_toggle(void);         // flips between the dark/light color presets (RAM only - caller persists)

#endif
