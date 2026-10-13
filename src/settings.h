#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// ---- Hardware (verify against your board!) ----
#define BL_PIN        5      // backlight, PWM
#define BL_ACTIVE_LOW 1      // 1 = LOW turns the backlight on
#define FW_VERSION    "1.5.0"
#define CONFIG_PATH   "/smalltv_cfg.json"   // own name: never touches the stock firmware's /config.json
#define GIF_DIR       "/image"   // same folder the stock firmware uses for GIFs
#define GIF_DIR2      "/gif"     // also searched, in case stock GIFs live here
#define RECOVERY_BOOTS 3          // this many unfinished boots in a row -> recovery mode
#define HEALTHY_MS    180000UL    // uptime after which a boot counts as good

enum Page : uint8_t { PAGE_CLOCK = 0, PAGE_WEATHER = 1, PAGE_STATS = 2, PAGE_GIF = 3, PAGE_COUNT = 4 };

struct Settings {
  char     hostname[24]  = "smalltv";
  char     tz[40]        = "UTC0";          // POSIX TZ string
  bool     h24           = true;
  bool     flip          = false;           // rotate display 180 degrees
  float    lat           = 51.5072f;
  float    lon           = -0.1276f;
  bool     fahrenheit    = false;
  uint8_t  brightness    = 70;              // percent
  bool     nightEnabled  = false;
  uint8_t  nightStart    = 22;              // hour
  uint8_t  nightEnd      = 7;               // hour
  uint8_t  nightBright   = 10;              // percent
  uint8_t  pages         = 0x07;            // bit mask: 1 clock, 2 weather, 4 stats, 8 gif
  uint16_t rotateSec     = 10;
  char     accent[8]     = "#2DD4BF";
  char     gif[32]       = "";
  bool     mqttEnabled   = false;
  char     mqttHost[48]  = "";
  uint16_t mqttPort      = 1883;
  char     mqttUser[32]  = "";
  char     mqttPass[48]  = "";
  char     mqttTopic[32] = "smalltv";
  bool     haDiscovery   = true;
  uint8_t  mode          = 0;               // 0 = rotate pages, 1..4 = keep one page (clock, weather, status, GIF)
  uint8_t  clockStyle    = 0;               // 0 classic, 1 accent, 2 cards, 3 stacked, 4 day progress, 5 glance
  uint8_t  weatherStyle  = 0;               // 0 forecast, 1 card, 2 tiles, 3 range
  uint8_t  theme         = 0;               // 0 black, 1 midnight, 2 graphite, 3 light
  bool     showSeconds   = false;
  bool     showDate      = true;
  bool     showInfo      = true;            // clock: the two info blocks at the bottom
  uint8_t  infoRight     = 0;               // 0 humidity, 1 wind, 2 high today
  bool     pageDots      = false;           // small dots showing the current page while rotating
  bool     gifAuto       = false;           // play every GIF in turn instead of looping one
  uint16_t gifSecs       = 20;              // ... switching after this many seconds (at the end of a loop)
};

inline void copyStr(char* dst, size_t n, JsonVariantConst v) {
  const char* p = v.as<const char*>();
  if (p) strlcpy(dst, p, n);
}

// Update only the keys that are present. Used for both file load and web POST.
inline void settingsFromJson(Settings& s, JsonVariantConst v) {
  copyStr(s.hostname, sizeof(s.hostname), v["hostname"]);
  copyStr(s.tz, sizeof(s.tz), v["tz"]);
  copyStr(s.accent, sizeof(s.accent), v["accent"]);
  copyStr(s.gif, sizeof(s.gif), v["gif"]);
  copyStr(s.mqttHost, sizeof(s.mqttHost), v["mqttHost"]);
  copyStr(s.mqttUser, sizeof(s.mqttUser), v["mqttUser"]);
  copyStr(s.mqttTopic, sizeof(s.mqttTopic), v["mqttTopic"]);
  const char* pw = v["mqttPass"].as<const char*>();
  if (pw && pw[0]) strlcpy(s.mqttPass, pw, sizeof(s.mqttPass));   // empty = keep old
  if (!v["h24"].isNull())          s.h24 = v["h24"];
  if (!v["flip"].isNull())         s.flip = v["flip"];
  if (!v["lat"].isNull())          s.lat = v["lat"];
  if (!v["lon"].isNull())          s.lon = v["lon"];
  if (!v["fahrenheit"].isNull())   s.fahrenheit = v["fahrenheit"];
  if (!v["brightness"].isNull())   s.brightness = constrain((int)v["brightness"], 0, 100);
  if (!v["nightEnabled"].isNull()) s.nightEnabled = v["nightEnabled"];
  if (!v["nightStart"].isNull())   s.nightStart = constrain((int)v["nightStart"], 0, 23);
  if (!v["nightEnd"].isNull())     s.nightEnd = constrain((int)v["nightEnd"], 0, 23);
  if (!v["nightBright"].isNull())  s.nightBright = constrain((int)v["nightBright"], 0, 100);
  if (!v["pages"].isNull())        s.pages = (uint8_t)((int)v["pages"] & 0x0F);
  if (!v["rotateSec"].isNull())    s.rotateSec = constrain((int)v["rotateSec"], 3, 600);
  if (!v["mqttEnabled"].isNull())  s.mqttEnabled = v["mqttEnabled"];
  if (!v["mqttPort"].isNull())     s.mqttPort = v["mqttPort"];
  if (!v["haDiscovery"].isNull())  s.haDiscovery = v["haDiscovery"];
  if (!v["mode"].isNull())         s.mode = constrain((int)v["mode"], 0, 4);
  if (!v["clockStyle"].isNull())   s.clockStyle = constrain((int)v["clockStyle"], 0, 5);
  if (!v["weatherStyle"].isNull()) s.weatherStyle = constrain((int)v["weatherStyle"], 0, 3);
  if (!v["theme"].isNull())        s.theme = constrain((int)v["theme"], 0, 3);
  if (!v["showSeconds"].isNull())  s.showSeconds = v["showSeconds"];
  if (!v["showDate"].isNull())     s.showDate = v["showDate"];
  if (!v["showInfo"].isNull())     s.showInfo = v["showInfo"];
  if (!v["infoRight"].isNull())    s.infoRight = constrain((int)v["infoRight"], 0, 2);
  if (!v["pageDots"].isNull())     s.pageDots = v["pageDots"];
  if (!v["gifAuto"].isNull())      s.gifAuto = v["gifAuto"];
  if (!v["gifSecs"].isNull())      s.gifSecs = constrain((int)v["gifSecs"], 3, 3600);
  if (s.pages == 0) s.pages = 0x01;
}

inline void settingsToJson(const Settings& s, JsonDocument& d, bool includeSecrets) {
  d["hostname"] = s.hostname;   d["tz"] = s.tz;           d["h24"] = s.h24;
  d["flip"] = s.flip;           d["lat"] = s.lat;         d["lon"] = s.lon;
  d["fahrenheit"] = s.fahrenheit;   d["brightness"] = s.brightness;
  d["nightEnabled"] = s.nightEnabled;   d["nightStart"] = s.nightStart;
  d["nightEnd"] = s.nightEnd;   d["nightBright"] = s.nightBright;
  d["pages"] = s.pages;         d["rotateSec"] = s.rotateSec;
  d["accent"] = s.accent;       d["gif"] = s.gif;
  d["mqttEnabled"] = s.mqttEnabled;     d["mqttHost"] = s.mqttHost;
  d["mqttPort"] = s.mqttPort;   d["mqttUser"] = s.mqttUser;
  d["mqttTopic"] = s.mqttTopic; d["haDiscovery"] = s.haDiscovery;
  d["mode"] = s.mode;           d["clockStyle"] = s.clockStyle;
  d["weatherStyle"] = s.weatherStyle;   d["theme"] = s.theme;
  d["showSeconds"] = s.showSeconds;     d["showDate"] = s.showDate;
  d["showInfo"] = s.showInfo;           d["infoRight"] = s.infoRight;
  d["pageDots"] = s.pageDots;
  d["gifAuto"] = s.gifAuto;             d["gifSecs"] = s.gifSecs;
  d["mqttPass"] = includeSecrets ? s.mqttPass : "";
}

inline void loadSettings(Settings& s) {
  File f = LittleFS.open(CONFIG_PATH, "r");
  if (!f) return;
  JsonDocument d;
  if (!deserializeJson(d, f)) settingsFromJson(s, d.as<JsonVariantConst>());
  f.close();
}

inline bool saveSettings(const Settings& s) {
  File f = LittleFS.open(CONFIG_PATH, "w");
  if (!f) return false;
  JsonDocument d;
  settingsToJson(s, d, true);
  serializeJson(d, f);
  f.close();
  return true;
}
