#include "theme.h"

// UI theme colors (hex "#rrggbb"), overridable via KEY=VALUE lines in
// WIFI_CFG_PATH (see THEME_appendConfigLines/THEME_tryParseConfigLine below).
static String st_colorBg          = "#11151c";   // page background
static String st_colorCardBg      = "#1b212b";   // card/panel background
static String st_colorBorder      = "#232b37";   // card/table borders
static String st_colorText        = "#e6e9ef";   // main body text
static String st_colorMuted       = "#8b93a3";   // secondary/muted text
static String st_colorAccent      = "#ff5a3c";   // primary button / active nav background
static String st_colorAccentHover = "#ff6f55";   // primary button hover
static String st_colorLink        = "#ff7a5c";   // hyperlink color
static String st_colorBtn         = "#2a3340";   // secondary button background
static String st_colorBtnHover    = "#37424f";   // secondary button hover

struct ColorEntry { const char* key; const char* cssVar; String* value; };
static ColorEntry st_colorEntries[] = {
    { "COLOR_BG",           "bg",           &st_colorBg },
    { "COLOR_CARD_BG",      "card-bg",      &st_colorCardBg },
    { "COLOR_BORDER",       "border",       &st_colorBorder },
    { "COLOR_TEXT",         "text",         &st_colorText },
    { "COLOR_MUTED",        "muted",        &st_colorMuted },
    { "COLOR_ACCENT",       "accent",       &st_colorAccent },
    { "COLOR_ACCENT_HOVER", "accent-hover", &st_colorAccentHover },
    { "COLOR_LINK",         "link",         &st_colorLink },
    { "COLOR_BTN",          "btn",          &st_colorBtn },
    { "COLOR_BTN_HOVER",    "btn-hover",    &st_colorBtnHover },
};
static const int NUM_COLOR_ENTRIES = sizeof(st_colorEntries) / sizeof(st_colorEntries[0]);

// "dark" or "light" - which preset below is currently active (persisted as THEME=).
static String st_theme = "dark";

// Preset color values, in the same order as st_colorEntries above.
static const char* DARK_PRESET[]  = { "#11151c", "#1b212b", "#232b37", "#e6e9ef", "#8b93a3",
                                       "#ff5a3c", "#ff6f55", "#ff7a5c", "#2a3340", "#37424f" };
static const char* LIGHT_PRESET[] = { "#f4f5f7", "#ffffff", "#d8dce2", "#1b212b", "#6b7280",
                                       "#ff5a3c", "#ff7a5c", "#d9480f", "#e9ecef", "#d8dde3" };

String THEME_getCSSVars(void) {
    String s;
    for (int i = 0; i < NUM_COLOR_ENTRIES; i++) {
        s += String("--") + st_colorEntries[i].cssVar + ":" + *st_colorEntries[i].value + ";";
    }
    return s;
}

void THEME_toggle(void) {
    bool toLight = (st_theme != "light");
    st_theme = toLight ? "light" : "dark";
    const char** preset = toLight ? LIGHT_PRESET : DARK_PRESET;
    for (int i = 0; i < NUM_COLOR_ENTRIES; i++) {
        *st_colorEntries[i].value = preset[i];
    }
}

void THEME_appendConfigLines(String& content) {
    content += "THEME=" + st_theme + "\n";
    content += "# --- UI theme colors (hex #rrggbb) ---\n";
    for (int i = 0; i < NUM_COLOR_ENTRIES; i++) {
        content += String(st_colorEntries[i].key) + "=" + *st_colorEntries[i].value + "\n";
    }
}

bool THEME_tryParseConfigLine(const String& key, const String& val) {
    if (key == "THEME") {
        if (val == "light" || val == "dark") {
            st_theme = val;
        }
        return true;
    }
    if (val.length() != 7 || val[0] != '#') {
        return false;
    }
    for (int i = 0; i < NUM_COLOR_ENTRIES; i++) {
        if (key == st_colorEntries[i].key) {
            *st_colorEntries[i].value = val;
            return true;
        }
    }
    return false;
}
