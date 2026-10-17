#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// ---- Hardware (verify against your board!) ----
#define BL_PIN        5      // backlight, PWM
#define BL_ACTIVE_LOW 1      // 1 = LOW turns the backlight on
#define FW_VERSION    "1.9.0"
#define CONFIG_PATH   "/smalltv_cfg.json"   // own name: never touches the stock firmware's /config.json
#define GIF_DIR       "/image"   // same folder the stock firmware uses for GIFs
#define GIF_DIR2      "/gif"     // also searched, in case stock GIFs live here
#define RECOVERY_BOOTS 3          // this many unfinished boots in a row -> recovery mode
#define HEALTHY_MS    180000UL    // uptime after which a boot counts as good

#define MAX_SLOTS 10
#define MAX_PL 4                        // number of playlists
struct Slot { uint8_t page, style; uint16_t secs; };   // one playlist item: which page, which face, how long

enum Page : uint8_t { PAGE_CLOCK = 0, PAGE_WEATHER = 1, PAGE_STATS = 2, PAGE_GIF = 3, PAGE_CAL = 4, PAGE_COUNT = 5 };

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
  uint8_t  pages         = 0x07;            // bit mask: 1 clock, 2 weather, 4 stats, 8 gif, 16 calendar
  uint16_t rotateSec     = 10;
  char     accent[8]     = "#FF6B57";
  char     timeColor[8]  = "#FFFFFF";         // white = follow the theme
  char     gif[32]       = "";
  bool     mqttEnabled   = false;
  char     mqttHost[48]  = "";
  uint16_t mqttPort      = 1883;
  char     mqttUser[32]  = "";
  char     mqttPass[48]  = "";
  char     mqttTopic[32] = "smalltv";
  bool     haDiscovery   = true;
  uint8_t  mode          = 0;               // 0 = rotate pages, 1..4 = keep one page (clock, weather, status, GIF), 5 = custom playlist, 6 = calendar only
  Slot     list[MAX_PL][MAX_SLOTS];           // custom playlists
  uint8_t  listN[MAX_PL] = {0, 0, 0, 0};
  char     plName[MAX_PL][12] = {"Playlist 1", "Playlist 2", "Playlist 3", "Playlist 4"};
  uint8_t  plActive      = 0;                 // which playlist plays in "Custom playlist" mode
  uint8_t  clockStyle    = 0;               // 0 classic, 1 accent, 2 cards, 3 stacked, 4 day progress, 5 glance, 6 week, 7 sidebar, 8 date
  uint8_t  weatherStyle  = 0;               // 0 forecast, 1 card, 2 tiles, 3 range, 4 hourly, 5 list
  uint8_t  theme         = 0;               // 0 black, 1 midnight, 2 graphite, 3 light
  bool     showSeconds   = false;
  bool     showDate      = true;
  bool     showInfo      = true;            // clock: the two info blocks at the bottom
  uint8_t  infoRight     = 0;               // 0 humidity, 1 wind, 2 high today
  bool     pageDots      = false;           // small dots showing the current page while rotating
  uint8_t  calStyle      = 0;               // calendar: 0 month, 1 tiles, 2 focus
  bool     calMon        = true;            // week starts on Monday
  bool     calWeekend    = true;            // color the weekend days
  char     weekendColor[8] = "#FF6B57";
  bool     calOther      = true;            // show the neighbouring months' days, dimmed
  bool     calClock      = true;            // small clock in the calendar header
  bool     gifAuto       = false;           // play every GIF in turn instead of looping one
  uint16_t gifSecs       = 20;
  uint16_t gifSpeed      = 100;             // percent of the GIF's own speed              // ... switching after this many seconds (at the end of a loop)
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
  copyStr(s.timeColor, sizeof(s.timeColor), v["timeColor"]);
  copyStr(s.weekendColor, sizeof(s.weekendColor), v["weekendColor"]);
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
  if (!v["pages"].isNull())        s.pages = (uint8_t)((int)v["pages"] & 0x1F);
  if (!v["rotateSec"].isNull())    s.rotateSec = constrain((int)v["rotateSec"], 3, 600);
  if (!v["mqttEnabled"].isNull())  s.mqttEnabled = v["mqttEnabled"];
  if (!v["mqttPort"].isNull())     s.mqttPort = v["mqttPort"];
  if (!v["haDiscovery"].isNull())  s.haDiscovery = v["haDiscovery"];
  if (!v["mode"].isNull())         s.mode = constrain((int)v["mode"], 0, 6);
  if (!v["clockStyle"].isNull())   s.clockStyle = constrain((int)v["clockStyle"], 0, 8);
  if (!v["weatherStyle"].isNull()) s.weatherStyle = constrain((int)v["weatherStyle"], 0, 5);
  if (!v["theme"].isNull())        s.theme = constrain((int)v["theme"], 0, 3);
  if (!v["showSeconds"].isNull())  s.showSeconds = v["showSeconds"];
  if (!v["showDate"].isNull())     s.showDate = v["showDate"];
  if (!v["showInfo"].isNull())     s.showInfo = v["showInfo"];
  if (!v["infoRight"].isNull())    s.infoRight = constrain((int)v["infoRight"], 0, 2);
  if (!v["pageDots"].isNull())     s.pageDots = v["pageDots"];
  if (!v["calStyle"].isNull())     s.calStyle = constrain((int)v["calStyle"], 0, 2);
  if (!v["calMon"].isNull())       s.calMon = v["calMon"];
  if (!v["calWeekend"].isNull())   s.calWeekend = v["calWeekend"];
  if (!v["calOther"].isNull())     s.calOther = v["calOther"];
  if (!v["calClock"].isNull())     s.calClock = v["calClock"];
  if (!v["gifAuto"].isNull())      s.gifAuto = v["gifAuto"];
  if (!v["gifSecs"].isNull())      s.gifSecs = constrain((int)v["gifSecs"], 3, 3600);
  if (!v["gifSpeed"].isNull())     s.gifSpeed = constrain((int)v["gifSpeed"], 25, 300);
  // playlists: {"pl":[{"n":"name","l":[{"p":page,"s":face,"t":seconds},...]},...],"plActive":0}
  JsonArrayConst pls = v["pl"].as<JsonArrayConst>();
  if (!pls.isNull()) {
    int pi = 0;
    for (JsonVariantConst q : pls) {
      if (pi >= MAX_PL) break;
      copyStr(s.plName[pi], sizeof(s.plName[pi]), q["n"]);
      JsonArrayConst items = q["l"].as<JsonArrayConst>();
      s.listN[pi] = 0;
      for (JsonVariantConst o : items) {
        if (s.listN[pi] >= MAX_SLOTS) break;
        int p = o["p"] | 0, f = o["s"] | 0, t = o["t"] | 10;
        Slot& sl = s.list[pi][s.listN[pi]++];
        sl.page = constrain(p, 0, 4); sl.style = constrain(f, 0, 8); sl.secs = constrain(t, 1, 600);
      }
      pi++;
    }
  }
  if (!v["plActive"].isNull()) s.plActive = constrain((int)v["plActive"], 0, MAX_PL - 1);
  JsonArrayConst legacy = v["list"].as<JsonArrayConst>();      // settings saved by older versions: one playlist
  if (!legacy.isNull() && v["pl"].isNull()) {
    s.listN[0] = 0;
    for (JsonVariantConst o : legacy) {
      if (s.listN[0] >= MAX_SLOTS) break;
      int p = o["p"] | 0, f = o["s"] | 0, t = o["t"] | 10;
      Slot& sl = s.list[0][s.listN[0]++];
      sl.page = constrain(p, 0, 4); sl.style = constrain(f, 0, 8); sl.secs = constrain(t, 1, 600);
    }
  }
  if (s.pages == 0) s.pages = 0x01;
}

inline void settingsToJson(const Settings& s, JsonDocument& d, bool includeSecrets) {
  d["hostname"] = s.hostname;   d["tz"] = s.tz;           d["h24"] = s.h24;
  d["flip"] = s.flip;           d["lat"] = s.lat;         d["lon"] = s.lon;
  d["fahrenheit"] = s.fahrenheit;   d["brightness"] = s.brightness;
  d["nightEnabled"] = s.nightEnabled;   d["nightStart"] = s.nightStart;
  d["nightEnd"] = s.nightEnd;   d["nightBright"] = s.nightBright;
  d["pages"] = s.pages;         d["rotateSec"] = s.rotateSec;
  d["accent"] = s.accent;       d["timeColor"] = s.timeColor;     d["gif"] = s.gif;
  d["mqttEnabled"] = s.mqttEnabled;     d["mqttHost"] = s.mqttHost;
  d["mqttPort"] = s.mqttPort;   d["mqttUser"] = s.mqttUser;
  d["mqttTopic"] = s.mqttTopic; d["haDiscovery"] = s.haDiscovery;
  d["mode"] = s.mode;           d["clockStyle"] = s.clockStyle;
  d["weatherStyle"] = s.weatherStyle;   d["theme"] = s.theme;
  d["showSeconds"] = s.showSeconds;     d["showDate"] = s.showDate;
  d["showInfo"] = s.showInfo;           d["infoRight"] = s.infoRight;
  d["pageDots"] = s.pageDots;
  d["calStyle"] = s.calStyle;           d["calMon"] = s.calMon;           d["calWeekend"] = s.calWeekend;
  d["weekendColor"] = s.weekendColor;   d["calOther"] = s.calOther;         d["calClock"] = s.calClock;
  d["gifAuto"] = s.gifAuto;             d["gifSecs"] = s.gifSecs;         d["gifSpeed"] = s.gifSpeed;
  JsonArray pls = d["pl"].to<JsonArray>();
  for (int pi = 0; pi < MAX_PL; pi++) {
    JsonObject q = pls.add<JsonObject>();
    q["n"] = s.plName[pi];
    JsonArray items = q["l"].to<JsonArray>();
    for (int i = 0; i < s.listN[pi]; i++) { JsonObject o = items.add<JsonObject>(); o["p"] = s.list[pi][i].page; o["s"] = s.list[pi][i].style; o["t"] = s.list[pi][i].secs; }
  }
  d["plActive"] = s.plActive;
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
