// SmallTV-style desk display firmware for ESP8266 + 240x240 ST7789.
// Pages: clock, weather (Open-Meteo), status, GIF.  Web UI, OTA, MQTT + Home Assistant discovery.
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include "wifisetup.h"
#include <PubSubClient.h>
#include <TFT_eSPI.h>
#include <AnimatedGIF.h>
#include <time.h>
#include <strings.h>
#include <new>
#include "settings.h"
#include "webui.h"
#include "fonts.h"
#include "icons.h"

#define C565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

// ------------------------------------------------------------------ globals
Settings cfg;
TFT_eSPI tft;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiClient netClient;
PubSubClient mqtt(netClient);
AnimatedGIF* gif = nullptr;

uint16_t cBg, cPanel, cText, cDim, cTrack, cAccent;
char apName[24];

struct Weather {
  bool valid = false; float temp = 0, hi = 0, lo = 0, wind = 0; int code = 0, hum = 0; bool isDay = true;
  int dcode[3] = {0, 0, 0}; float dhi[3] = {0, 0, 0}, dlo[3] = {0, 0, 0};     // today + next two days
} wx;
uint32_t wxVer = 0, lastWxVer = 0xFFFFFFFF;                                  // bumped on every weather update
const char* wxErr = nullptr;
bool wxDirty = true;
uint32_t nextWeather = 0;

uint8_t page = PAGE_CLOCK;
bool fullRedraw = true, gifOk = false;
bool fsOk = false, recoveryMode = false, healthyMarked = false;
uint32_t bootCount = 0;
uint32_t pageSince = 0, lastSlow = 0, lastBars = 0, gifNext = 0;
int lastMin = -1, lastSec = -1, gifOffX = 0, gifOffY = 0;
int lastDay = -1;

// mqtt
char tStatus[64], tState[64], tBrt[64], tBrtSet[64], tPageSet[64], tGifNext[64];
uint32_t mqttNext = 0, mqttPub = 0;

void setPage(uint8_t p);
bool validGifName(const String& n);

// ------------------------------------------------------------------ crash-loop guard
// Counts boots in RTC memory (survives crashes, watchdog and reset-button, not power loss).
// A boot only counts as good after HEALTHY_MS of uptime. RECOVERY_BOOTS bad boots in a row
// start a minimal WiFi + /update mode so a bad firmware can always be replaced over WiFi.
struct RtcBoot { uint32_t magic, build, count; };
static const uint32_t RTC_MAGIC = 0x534D5456;   // "SMTV"

uint32_t buildId() {
  uint32_t h = 2166136261u;
  for (const char* p = FW_VERSION __DATE__ __TIME__; *p; p++) { h ^= (uint8_t)*p; h *= 16777619u; }
  return h;
}
void markHealthy() {
  RtcBoot r = {RTC_MAGIC, buildId(), 0};
  ESP.rtcUserMemoryWrite(0, (uint32_t*)&r, sizeof r);
}
void bootGuard() {
  RtcBoot r;
  bool ok = ESP.rtcUserMemoryRead(0, (uint32_t*)&r, sizeof r);
  bool fresh = !ok || r.magic != RTC_MAGIC || r.build != buildId() ||
               ESP.getResetInfoPtr()->reason == REASON_DEFAULT_RST;   // power-on or new firmware
  if (fresh) r = {RTC_MAGIC, buildId(), 0};
  r.count++;
  ESP.rtcUserMemoryWrite(0, (uint32_t*)&r, sizeof r);
  bootCount = r.count;
  recoveryMode = r.count >= RECOVERY_BOOTS;
}

// ------------------------------------------------------------------ helpers
uint16_t hexTo565(const char* h) {
  if (!h || h[0] != '#' || strlen(h) != 7) return tft.color565(45, 212, 191);
  long v = strtol(h + 1, nullptr, 16);
  return tft.color565((v >> 16) & 255, (v >> 8) & 255, v & 255);
}

void rebuildColors() {
  switch (cfg.theme) {
    case 1:  // midnight
      cBg = C565(6, 12, 30); cPanel = C565(22, 34, 66); cText = C565(235, 240, 255); cDim = C565(135, 150, 190); cTrack = C565(44, 58, 96); break;
    case 2:  // graphite
      cBg = C565(24, 24, 27); cPanel = C565(44, 44, 48); cText = C565(245, 245, 247); cDim = C565(150, 150, 158); cTrack = C565(70, 70, 76); break;
    case 3:  // light
      cBg = C565(242, 242, 247); cPanel = C565(255, 255, 255); cText = C565(20, 20, 24); cDim = C565(110, 110, 118); cTrack = C565(210, 210, 216); break;
    default: // black
      cBg = C565(0, 0, 0); cPanel = C565(28, 28, 30); cText = C565(255, 255, 255); cDim = C565(142, 142, 147); cTrack = C565(58, 58, 60); break;
  }
  cAccent = hexTo565(cfg.accent);
}

bool timeValid() { return time(nullptr) > 1700000000; }

bool nightActive() {
  if (!cfg.nightEnabled || !timeValid() || cfg.nightStart == cfg.nightEnd) return false;
  time_t n = time(nullptr); struct tm t; localtime_r(&n, &t);
  int h = t.tm_hour;
  return cfg.nightStart < cfg.nightEnd ? (h >= cfg.nightStart && h < cfg.nightEnd)
                                       : (h >= cfg.nightStart || h < cfg.nightEnd);
}

void setBacklight(uint8_t pct) {
  uint16_t duty = map(pct, 0, 100, 0, 1023);
  if (BL_ACTIVE_LOW) duty = 1023 - duty;
  analogWrite(BL_PIN, duty);
}
void applyBrightness() { setBacklight(nightActive() ? cfg.nightBright : cfg.brightness); }

bool resolveGif(const char* name, char* out, size_t n) {
  if (!fsOk || !name || !name[0]) return false;
  snprintf(out, n, "%s/%s", GIF_DIR, name);
  if (LittleFS.exists(out)) return true;
  snprintf(out, n, "%s/%s", GIF_DIR2, name);
  return LittleFS.exists(out);
}
void refreshGifOk() { char p[48]; gifOk = resolveGif(cfg.gif, p, sizeof p); }
bool pageUsable(uint8_t p) {
  if (p >= PAGE_COUNT || !((cfg.pages >> p) & 1)) return false;
  return p != PAGE_GIF || gifOk;
}
uint8_t usableCount() { uint8_t n = 0; for (uint8_t p = 0; p < PAGE_COUNT; p++) n += pageUsable(p); return n; }
bool rotating() { return cfg.mode == 0 && usableCount() > 1; }
uint8_t nextPageFrom(uint8_t cur) {
  for (uint8_t i = 1; i <= PAGE_COUNT; i++) { uint8_t p = (cur + i) % PAGE_COUNT; if (pageUsable(p)) return p; }
  return PAGE_CLOCK;
}

// ------------------------------------------------------------------ weather
int wxType(int c) {   // 0 clear 1 partly 2 cloud 3 rain 4 snow 5 storm 6 fog
  if (c == 0) return 0;
  if (c <= 2) return 1;
  if (c == 3) return 2;
  if (c == 45 || c == 48) return 6;
  if ((c >= 51 && c <= 67) || (c >= 80 && c <= 82)) return 3;
  if ((c >= 71 && c <= 77) || c == 85 || c == 86) return 4;
  if (c >= 95) return 5;
  return 2;
}
const char* wxText(int c) {
  switch (wxType(c)) {
    case 0: return "Clear";   case 1: return "Partly cloudy"; case 2: return "Cloudy";
    case 3: return "Rain";    case 4: return "Snow";          case 5: return "Thunderstorm";
    default: return "Fog";
  }
}

void fetchWeather() {
  nextWeather = millis() + (wx.valid ? 15UL * 60000UL : 60000UL);
  if (WiFi.status() != WL_CONNECTED) return;
#ifdef WEATHER_HTTPS
  const char* scheme = "https";
#else
  const char* scheme = "http";
#endif
  char url[420];
  snprintf(url, sizeof url,
           "%s://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,relative_humidity_2m,weather_code,is_day,wind_speed_10m"
           "&daily=weather_code,temperature_2m_max,temperature_2m_min&forecast_days=3&timezone=auto"
           "&temperature_unit=%s&wind_speed_unit=%s",
           scheme, cfg.lat, cfg.lon, cfg.fahrenheit ? "fahrenheit" : "celsius", cfg.fahrenheit ? "mph" : "kmh");
  HTTPClient http;
#ifdef WEATHER_HTTPS
  BearSSL::WiFiClientSecure client; client.setInsecure(); client.setBufferSizes(1024, 512);
#else
  WiFiClient client;
#endif
  http.begin(client, url);
  http.useHTTP10(true);       // avoids chunked encoding so we can stream-parse
  http.setTimeout(8000);
  int code = http.GET();
  if (code == 200) {
    JsonDocument filter;
    filter["current"]["temperature_2m"] = true;
    filter["current"]["relative_humidity_2m"] = true;
    filter["current"]["weather_code"] = true;
    filter["current"]["is_day"] = true;
    filter["current"]["wind_speed_10m"] = true;
    filter["daily"]["weather_code"][0] = true;
    filter["daily"]["temperature_2m_max"][0] = true;
    filter["daily"]["temperature_2m_min"][0] = true;
    JsonDocument doc;
    if (!deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter))) {
      wx.temp = doc["current"]["temperature_2m"] | 0.0f;
      wx.hum = doc["current"]["relative_humidity_2m"] | 0;
      wx.wind = doc["current"]["wind_speed_10m"] | 0.0f;
      wx.code = doc["current"]["weather_code"] | 0;
      wx.isDay = (doc["current"]["is_day"] | 1) == 1;
      for (int i = 0; i < 3; i++) {
        wx.dcode[i] = doc["daily"]["weather_code"][i] | 0;
        wx.dhi[i] = doc["daily"]["temperature_2m_max"][i] | 0.0f;
        wx.dlo[i] = doc["daily"]["temperature_2m_min"][i] | 0.0f;
      }
      wx.hi = wx.dhi[0]; wx.lo = wx.dlo[0];
      wx.valid = true; wxErr = nullptr; wxDirty = true; wxVer++;
    } else { wxErr = "Weather data error"; wxDirty = true; wxVer++; }
  } else {
    wxErr = (code == 301 || code == 302 || code == 308) ? "Needs HTTPS (see README)" : "Weather unreachable";
    wxDirty = true; wxVer++;
  }
  http.end();
}

// ------------------------------------------------------------------ drawing engine
// Text, icons and rounded boxes are drawn row by row straight into the display window: no frame
// buffer, no flicker, and almost no RAM. Fonts and icons live in flash as run-length encoded 4-bit alpha.
enum { AL_L = 0, AL_C = 1, AL_R = 2 };

struct Rle { const uint8_t* p; uint16_t left; uint8_t v; };
static inline uint8_t rleNext(Rle& d) {
  if (d.left == 0) {
    uint8_t b = pgm_read_byte(d.p++);
    d.v = b >> 4;
    uint8_t n = b & 15;
    d.left = (n < 15) ? n + 1 : 16 + pgm_read_byte(d.p++);
  }
  d.left--;
  return d.v;
}

static uint16_t rowBuf[240];

static inline uint16_t mix565(uint16_t bg, uint16_t fg, uint8_t a) {   // a = 0..15 (fg amount)
  if (a == 0) return bg;
  if (a >= 15) return fg;
  uint32_t r = (((bg >> 11) & 31) * (15 - a) + ((fg >> 11) & 31) * a + 7) / 15;
  uint32_t g = (((bg >> 5) & 63) * (15 - a) + ((fg >> 5) & 63) * a + 7) / 15;
  uint32_t b = ((bg & 31) * (15 - a) + (fg & 31) * a + 7) / 15;
  return (r << 11) | (g << 5) | b;
}
static uint16_t blendPct(uint16_t a, uint16_t b, int pct) {            // pct % of a, the rest of b
  int r = (((a >> 11) & 31) * pct + ((b >> 11) & 31) * (100 - pct)) / 100;
  int g = (((a >> 5) & 63) * pct + ((b >> 5) & 63) * (100 - pct)) / 100;
  int bl = ((a & 31) * pct + (b & 31) * (100 - pct)) / 100;
  return (r << 11) | (g << 5) | bl;
}

static bool findGlyph(const Font& f, uint8_t code, Glyph& out) {
  for (uint8_t i = 0; i < f.n; i++) {
    memcpy_P(&out, &f.g[i], sizeof(Glyph));
    if (out.code == code) return true;
  }
  return false;
}

// Draws a string with its baseline at 'base'. Returns the text width. padW clears a wider box.
static int drawText(const Font& f, const char* s, int x, int base, uint8_t al, uint16_t fg, uint16_t bg, int padW = 0) {
  Glyph gl[24]; int gx[24]; Rle dec[24]; int ng = 0, w = 0;
  for (const uint8_t* p = (const uint8_t*)s; *p && ng < 24; p++) {
    if (*p == ' ') { w += f.space; continue; }
    Glyph g;
    if (!findGlyph(f, *p, g)) continue;
    gl[ng] = g; gx[ng] = w + g.dx;
    dec[ng].p = f.bmp + g.off; dec[ng].left = 0; dec[ng].v = 0;
    ng++; w += g.adv;
  }
  int box = max(w, padW);
  if (box > 240) box = 240;
  if (box < 1) return w;
  int x0 = (al == AL_L) ? x : (al == AL_C) ? x - box / 2 : x - box;
  int t0 = (al == AL_L) ? 0 : (al == AL_C) ? (box - w) / 2 : box - w;
  if (x0 < 0) x0 = 0;
  if (x0 + box > 240) x0 = 240 - box;
  int top = base - f.asc, H = f.asc + f.desc;
  int rs = max(0, -top), re = min(H, 240 - top);
  if (re <= rs) return w;
  uint16_t lut[16];
  for (int a = 0; a < 16; a++) lut[a] = mix565(bg, fg, a);
  tft.startWrite();
  tft.setAddrWindow(x0, top + rs, box, re - rs);
  for (int r = 0; r < H; r++) {
    bool vis = (r >= rs && r < re);
    if (vis) for (int i = 0; i < box; i++) rowBuf[i] = bg;
    for (int k = 0; k < ng; k++) {
      int gr = r - (f.asc - gl[k].dy);
      if (gr < 0 || gr >= gl[k].h) continue;
      for (int c = 0; c < gl[k].w; c++) {
        uint8_t a = rleNext(dec[k]);            // every glyph row is decoded exactly once, in order
        if (a && vis) { int px = t0 + gx[k] + c; if (px >= 0 && px < box) rowBuf[px] = lut[a]; }
      }
    }
    if (vis) tft.pushPixels(rowBuf, box);
  }
  tft.endWrite();
  return w;
}

// Draws an icon centered at cx,cy. size 64 = full, 32 = half (smoothly reduced); 16-px icons are drawn 1:1.
static void drawIcon(const Icon& ic, int cx, int cy, int size, uint16_t bg, int32_t over = -1) {
  static uint8_t src[3][2][64];
  const int S = ic.size, f = S / size;
  int x0 = cx - size / 2, y0 = cy - size / 2;
  if (x0 < 0 || y0 < 0 || x0 + size > 240 || y0 + size > 240 || f < 1 || f > 2) return;
  Rle dec[3];
  for (int l = 0; l < ic.layers; l++) { dec[l].p = ic.bmp + ic.off[l]; dec[l].left = 0; dec[l].v = 0; }
  tft.startWrite();
  tft.setAddrWindow(x0, y0, size, size);
  for (int r = 0; r < size; r++) {
    for (int l = 0; l < ic.layers; l++)
      for (int k = 0; k < f; k++)
        for (int c = 0; c < S; c++) src[l][k][c] = rleNext(dec[l]);
    for (int x = 0; x < size; x++) {
      uint16_t px = bg;
      for (int l = 0; l < ic.layers; l++) {
        uint16_t sum = 0;
        for (int k = 0; k < f; k++) for (int j = 0; j < f; j++) sum += src[l][k][x * f + j];
        uint8_t a = (f == 1) ? (uint8_t)sum : (uint8_t)min(15, (int)(sum * 6) / (f * f * 5));
        px = mix565(px, (over >= 0 && l == 0) ? (uint16_t)over : ic.color[l], a);
      }
      rowBuf[x] = px;
    }
    tft.pushPixels(rowBuf, size);
  }
  tft.endWrite();
}

// Anti-aliased rounded rectangle; 'bg' is the color behind the corners.
static void drawRRect(int x, int y, int w, int h, int r, uint16_t color, uint16_t bg) {
  if (w > 240) w = 240;
  if (w < 2 || h < 2) return;
  tft.startWrite();
  tft.setAddrWindow(x, y, w, h);
  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) rowBuf[j] = color;
    if (i < r || i >= h - r) {
      float cy = (i < r) ? r : h - r;
      for (int j = 0; j < w; j++) {
        if (j >= r && j < w - r) continue;
        float cx = (j < r) ? r : w - r;
        float dx = j + 0.5f - cx, dy = i + 0.5f - cy;
        float cov = r - sqrtf(dx * dx + dy * dy) + 0.5f;
        rowBuf[j] = (cov <= 0) ? bg : (cov >= 1) ? color : mix565(bg, color, (uint8_t)(cov * 15 + 0.5f));
      }
    }
    tft.pushPixels(rowBuf, w);
  }
  tft.endWrite();
}

static int textW(const Font& f, const char* s) {
  int w = 0; Glyph g;
  for (const uint8_t* p = (const uint8_t*)s; *p; p++) {
    if (*p == ' ') w += f.space; else if (findGlyph(f, *p, g)) w += g.adv;
  }
  return w;
}

static void drawDots() {                                // optional page indicator (rotating mode only)
  if (!cfg.pageDots || !rotating()) return;
  int n = usableCount(), x = 120 - (n - 1) * 6;
  for (uint8_t p = 0; p < PAGE_COUNT; p++) {
    if (!pageUsable(p)) continue;
    tft.fillCircle(x, 235, 2, p == page ? cAccent : cTrack);
    x += 12;
  }
}
static void clearScreen() { tft.fillScreen(cBg); drawDots(); }

static const char* const WD[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char* const WDU[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char* const MO[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static const Icon* wxIcon(int code, bool day) {
  switch (wxType(code)) {
    case 0: return day ? &IC_CLEAR_DAY : &IC_CLEAR_NIGHT;
    case 1: return day ? &IC_PARTLY_DAY : &IC_PARTLY_NIGHT;
    case 2: return &IC_CLOUD;
    case 3: return &IC_RAIN;
    case 4: return &IC_SNOW;
    case 5: return &IC_STORM;
    default: return &IC_FOG;
  }
}
static const char* wxShort(int code, bool day) {
  switch (wxType(code)) {
    case 0: return day ? "SUNNY" : "CLEAR";
    case 1: return "P. CLOUDY";
    case 2: return "CLOUDY";
    case 3: return "RAIN";
    case 4: return "SNOW";
    case 5: return "STORM";
    default: return "FOG";
  }
}

// ------------------------------------------------------------------ clock
// Styles: 0 classic, 1 accent color, 2 cards. Text is repainted once a minute, the info blocks when
// new weather arrives, and the optional seconds line every second.
static void drawInfo(uint8_t st) {
  bool card = (st == 2);
  uint16_t bg = card ? cPanel : cBg;
  uint16_t lcol = (st == 1) ? blendPct(cAccent, cBg, 75) : cDim;
  uint16_t vcol = (st == 1) ? cAccent : cText;
  uint16_t ucol = (st == 1) ? lcol : cDim;
  char unit[3] = {(char)0xB0, cfg.fahrenheit ? 'F' : 'C', 0};
  const Icon* ic2 = &IC_DROP; const char* lab2 = "HUMIDITY"; int32_t ov2 = -1;
  if (cfg.infoRight == 1) { ic2 = &IC_WIND; lab2 = "WIND"; }
  else if (cfg.infoRight == 2) { ic2 = &IC_THERMO; lab2 = "HIGH"; ov2 = C565(255, 159, 10); }
  char v1[8] = "--", v2[8] = "--"; const char* u1 = ""; const char* u2 = "";
  if (wx.valid) {
    snprintf(v1, sizeof v1, "%d", (int)roundf(wx.temp)); u1 = unit;
    if (cfg.infoRight == 1) { snprintf(v2, sizeof v2, "%d", (int)roundf(wx.wind)); u2 = cfg.fahrenheit ? "mph" : "km/h"; }
    else if (cfg.infoRight == 2) { snprintf(v2, sizeof v2, "%d", (int)roundf(wx.hi)); u2 = unit; }
    else { snprintf(v2, sizeof v2, "%d", wx.hum); u2 = "%"; }
  }
  for (int b = 0; b < 2; b++) {
    const Icon& ic = b ? *ic2 : IC_THERMO;
    const char* lab = b ? lab2 : "OUTSIDE";
    const char* val = b ? v2 : v1;
    const char* un = b ? u2 : u1;
    int32_t ov = b ? ov2 : -1;
    int bx = card ? (b ? 124 : 6) : (b ? 124 : 24);
    int icx = card ? bx + 20 : bx + 8, icy = card ? 181 : 172;
    int lx = card ? bx + 34 : bx + 22, lb = card ? 187 : 178;
    int vx = card ? bx + 14 : bx, vb = card ? 224 : 214;
    drawIcon(ic, icx, icy, 16, bg, ov);
    drawText(F_CAP, lab, lx, lb, AL_L, lcol, bg);
    int w = drawText(F_VAL, val, vx, vb, AL_L, vcol, bg, card ? 88 : 92);
    if (un[0]) drawText(F_TXT, un, vx + w + 3, vb, AL_L, ucol, bg);
  }
}

// Styles 3..5: stacked (hours over minutes), day progress, glance (weather strip + clock).
static void renderClockExtra(uint8_t st, const struct tm& t, bool full) {
  int h = t.tm_hour;
  if (!cfg.h24) { h %= 12; if (!h) h = 12; }
  bool newMin = (t.tm_min != lastMin);
  bool newWx = (full || wxVer != lastWxVer);
  char db[24], tb[8], ib[40];
  snprintf(db, sizeof db, "%s, %s %d", WD[t.tm_wday], MO[t.tm_mon], t.tm_mday);
  const char* ampm = t.tm_hour < 12 ? "AM" : "PM";
  if (st == 3) {                                              // stacked
    if (newMin) {
      lastMin = t.tm_min;
      snprintf(tb, sizeof tb, cfg.h24 ? "%02d" : "%d", h);
      drawText(F_TIME, tb, 120, 88, AL_C, cText, cBg, 150);
      snprintf(tb, sizeof tb, "%02d", t.tm_min);
      drawText(F_TIME, tb, 120, 162, AL_C, cAccent, cBg, 150);
      if (!cfg.h24) drawText(F_CAP, ampm, 222, 162, AL_R, cDim, cBg);
      if (cfg.showDate) drawText(F_TXT, db, 120, 206, AL_C, cDim, cBg, 200);
    }
    if (cfg.showInfo && newWx) {
      lastWxVer = wxVer;
      if (wx.valid) snprintf(ib, sizeof ib, "%d%c%c  %c  %d%%", (int)roundf(wx.temp), 0xB0, cfg.fahrenheit ? 'F' : 'C', 0xB7, wx.hum);
      else strcpy(ib, "--");
      drawText(F_TXT, ib, 120, 231, AL_C, cDim, cBg, 200);
    }
    return;
  }
  if (st == 4) {                                              // day progress
    if (newMin) {
      lastMin = t.tm_min;
      if (cfg.h24) snprintf(tb, sizeof tb, "%02d:%02d", h, t.tm_min); else snprintf(tb, sizeof tb, "%d:%02d", h, t.tm_min);
      drawText(F_TIME, tb, 120, 98, AL_C, cText, cBg, 212);
      if (cfg.showDate) drawText(F_TXT, db, 120, 140, AL_C, cDim, cBg, 190);
      if (!cfg.h24) drawText(F_CAP, ampm, 224, 140, AL_R, cDim, cBg);
      float frac = (t.tm_hour * 60 + t.tm_min) / 1440.0f;
      int fw = max(12, (int)(192 * frac));
      drawRRect(24, 170, 192, 12, 6, cTrack, cBg);
      drawRRect(24, 170, fw, 12, 6, cAccent, cTrack);
      drawText(F_CAP, "TODAY", 24, 210, AL_L, cDim, cBg);
      snprintf(ib, sizeof ib, "%d%%", (int)(frac * 100));
      drawText(F_TXT, ib, 216, 212, AL_R, cText, cBg, 60);
    }
    return;
  }
  // st == 5: glance
  if (newWx) {
    lastWxVer = wxVer;
    if (wx.valid) {
      drawIcon(*wxIcon(wx.code, wx.isDay), 30, 36, 32, cBg);
      snprintf(ib, sizeof ib, "%d%c", (int)roundf(wx.temp), 0xB0);
      drawText(F_VAL, ib, 54, 50, AL_L, cText, cBg, 60);
      drawText(F_TXT, wxShort(wx.code, wx.isDay), 228, 48, AL_R, cDim, cBg, 120);
      snprintf(ib, sizeof ib, "High %d%c    Low %d%c", (int)roundf(wx.hi), 0xB0, (int)roundf(wx.lo), 0xB0);
      drawText(F_TXT, ib, 120, 226, AL_C, cDim, cBg, 220);
    } else drawText(F_TXT, "--", 54, 48, AL_L, cDim, cBg, 60);
    tft.fillRect(20, 68, 200, 1, cTrack);
  }
  if (newMin) {
    lastMin = t.tm_min;
    if (cfg.h24) snprintf(tb, sizeof tb, "%02d:%02d", h, t.tm_min); else snprintf(tb, sizeof tb, "%d:%02d", h, t.tm_min);
    drawText(F_TIME, tb, 120, 140, AL_C, cText, cBg, 212);
    if (cfg.showDate) drawText(F_TXT, db, 120, 184, AL_C, cAccent, cBg, 190);
    if (!cfg.h24) drawText(F_CAP, ampm, 224, 184, AL_R, cDim, cBg);
  }
}

void renderClock() {
  static bool syncing = false;
  if (!timeValid()) {
    syncing = true;
    if (fullRedraw) { fullRedraw = false; tft.fillScreen(cBg); drawText(F_TXT, "Syncing time...", 120, 126, AL_C, cDim, cBg); }
    return;
  }
  if (syncing) { syncing = false; fullRedraw = true; }
  time_t now = time(nullptr); struct tm t; localtime_r(&now, &t);
  if (t.tm_mday != lastDay) { lastDay = t.tm_mday; fullRedraw = true; }
  uint8_t st = cfg.clockStyle > 5 ? 0 : cfg.clockStyle;
  bool cards = (st == 2), info = cfg.showInfo;
  uint16_t panel = cards ? cPanel : cBg;
  bool full = fullRedraw;
  if (full) {
    fullRedraw = false; lastMin = lastSec = -1; lastWxVer = 0xFFFFFFFF;
    clearScreen();
    if (cards) {
      drawRRect(6, 6, 228, info ? 146 : 228, 26, cPanel, cBg);
      if (info) { drawRRect(6, 158, 110, 76, 22, cPanel, cBg); drawRRect(124, 158, 110, 76, 22, cPanel, cBg); }
    }
  }
  if (st >= 3) { renderClockExtra(st, t, full); return; }
  int tBase = info ? (cards ? 88 : 98) : 130;
  int dBase = info ? (cards ? 130 : 140) : (cards ? 172 : 176);
  if (t.tm_min != lastMin) {
    lastMin = t.tm_min;
    char tb[8]; int h = t.tm_hour;
    if (!cfg.h24) { h %= 12; if (!h) h = 12; }
    snprintf(tb, sizeof tb, cfg.h24 ? "%02d:%02d" : "%d:%02d", h, t.tm_min);
    drawText(F_TIME, tb, 120, tBase, AL_C, st == 1 ? cAccent : cText, panel, 212);
    if (cfg.showDate) {
      char db[24]; snprintf(db, sizeof db, "%s, %s %d", WD[t.tm_wday], MO[t.tm_mon], t.tm_mday);
      drawText(F_TXT, db, 120, dBase, AL_C, st == 1 ? blendPct(cAccent, cBg, 80) : cDim, panel, 190);
    }
    if (!cfg.h24) drawText(F_CAP, t.tm_hour < 12 ? "AM" : "PM", 224, dBase, AL_R, cDim, panel);
  }
  if (info && (full || wxVer != lastWxVer)) { lastWxVer = wxVer; drawInfo(st); }
  if (cfg.showSeconds && t.tm_sec != lastSec) {
    lastSec = t.tm_sec;
    tft.fillRect(60, 1, 120, 3, cTrack);
    tft.fillRect(60, 1, max(3, 120 * (t.tm_sec + 1) / 60), 3, cAccent);
  }
}

// ------------------------------------------------------------------ weather
// Styles: 0 forecast (now + 3 days), 1 card. Redrawn only when new data arrives or the page appears.
// Horizontal blue-to-orange bar with rounded ends (temperature range).
static void drawGradBar(int x, int y, int w, int h, uint16_t bg) {
  int r = h / 2;
  tft.startWrite();
  tft.setAddrWindow(x, y, w, h);
  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) {
      int f = (j * 255) / (w - 1);
      uint16_t c = C565(60 + (195 * f) / 255, 150, 255 - (215 * f) / 255);
      float cov = 1;
      if (j < r || j >= w - r) {
        float cx = (j < r) ? r : w - r, dx = j + 0.5f - cx, dy = i + 0.5f - r;
        cov = r - sqrtf(dx * dx + dy * dy) + 0.5f;
      }
      rowBuf[j] = (cov <= 0) ? bg : (cov >= 1) ? c : mix565(bg, c, (uint8_t)(cov * 15 + 0.5f));
    }
    tft.pushPixels(rowBuf, w);
  }
  tft.endWrite();
}

void renderWeather() {
  if (!fullRedraw && !wxDirty) return;
  fullRedraw = false; wxDirty = false;
  clearScreen();
  if (!wx.valid) { drawText(F_TXT, wxErr ? wxErr : "Fetching weather...", 120, 126, AL_C, cDim, cBg); return; }
  char tb[24];
  if (cfg.weatherStyle == 1) {                                   // card
    drawRRect(6, 6, 228, 228, 32, cPanel, cBg);
    drawIcon(*wxIcon(wx.code, wx.isDay), 42, 44, 32, cPanel);
    drawText(F_TXT, wxShort(wx.code, wx.isDay), 70, 52, AL_L, cText, cPanel);
    snprintf(tb, sizeof tb, "%d", (int)roundf(wx.temp));
    int w = drawText(F_TIME, tb, 26, 146, AL_L, cText, cPanel);
    char u[3] = {(char)0xB0, cfg.fahrenheit ? 'F' : 'C', 0};
    drawText(F_VAL, u, 26 + w + 4, 112, AL_L, cDim, cPanel);
    char pt[3][12];
    snprintf(pt[0], 12, "H %d%c", (int)roundf(wx.hi), 0xB0);
    snprintf(pt[1], 12, "L %d%c", (int)roundf(wx.lo), 0xB0);
    snprintf(pt[2], 12, "%d%%", wx.hum);
    uint16_t pc[3] = {C565(255, 159, 10), C565(100, 210, 255), C565(10, 132, 255)};
    int pw[3], total = 12;
    for (int i = 0; i < 3; i++) { pw[i] = textW(F_TXT, pt[i]) + 18; total += pw[i]; }
    int x = (240 - total) / 2;
    for (int i = 0; i < 3; i++) {
      uint16_t pb = blendPct(pc[i], cPanel, 22);
      drawRRect(x, 172, pw[i], 32, 16, pb, cPanel);
      drawText(F_TXT, pt[i], x + 9, 194, AL_L, pc[i], pb);
      x += pw[i] + 6;
    }
    return;
  }
  if (cfg.weatherStyle == 2) {                                   // tiles
    drawText(F_TXT, wxShort(wx.code, wx.isDay), 120, 24, AL_C, cDim, cBg);
    drawIcon(*wxIcon(wx.code, wx.isDay), 50, 64, 64, cBg);
    snprintf(tb, sizeof tb, "%d%c", (int)roundf(wx.temp), 0xB0);
    if (textW(F_TIME, tb) > 146) tb[strlen(tb) - 1] = 0;
    drawText(F_TIME, tb, 160, 92, AL_C, cText, cBg);
    char unit[3] = {(char)0xB0, cfg.fahrenheit ? 'F' : 'C', 0};
    char val[4][8];
    snprintf(val[0], 8, "%d", wx.hum); snprintf(val[1], 8, "%d", (int)roundf(wx.wind));
    snprintf(val[2], 8, "%d", (int)roundf(wx.hi)); snprintf(val[3], 8, "%d", (int)roundf(wx.lo));
    const Icon* ti[4] = {&IC_DROP, &IC_WIND, &IC_THERMO, &IC_THERMO};
    int32_t tov[4] = {-1, -1, C565(255, 159, 10), C565(100, 210, 255)};
    const char* tl[4] = {"HUMIDITY", "WIND", "HIGH", "LOW"};
    const char* tu[4] = {"%", cfg.fahrenheit ? "mph" : "km/h", unit, unit};
    const char* tc[4] = {"%", cfg.fahrenheit ? "MPH" : "KM/H", unit, unit};
    for (int i = 0; i < 4; i++) {
      int x = (i & 1) ? 124 : 6, y = (i >> 1) ? 174 : 110;
      drawRRect(x, y, 110, 60, 20, cPanel, cBg);
      drawIcon(*ti[i], x + 20, y + 15, 16, cPanel, tov[i]);
      drawText(F_CAP, tl[i], x + 34, y + 19, AL_L, cDim, cPanel);
      int w = drawText(F_VAL, val[i], x + 14, y + 54, AL_L, cText, cPanel);
      bool tight = (w + 3 + textW(F_TXT, tu[i]) > 90);          // long units fall back to the small caps font
      drawText(tight ? F_CAP : F_TXT, tight ? tc[i] : tu[i], x + 14 + w + 3, y + 54, AL_L, cDim, cPanel);
    }
    return;
  }
  if (cfg.weatherStyle == 3) {                                   // range: where "now" sits between today's low and high
    drawIcon(*wxIcon(wx.code, wx.isDay), 30, 30, 32, cBg);
    drawText(F_TXT, wxShort(wx.code, wx.isDay), 54, 38, AL_L, cDim, cBg);
    snprintf(tb, sizeof tb, "%d%c", (int)roundf(wx.temp), 0xB0);
    if (textW(F_TIME, tb) > 146) tb[strlen(tb) - 1] = 0;
    drawText(F_TIME, tb, 120, 114, AL_C, cText, cBg);
    drawGradBar(24, 134, 192, 12, cBg);
    float pos = (wx.hi > wx.lo) ? (wx.temp - wx.lo) / (wx.hi - wx.lo) : 0.5f;
    pos = constrain(pos, 0.0f, 1.0f);
    int mx = 24 + (int)(pos * 192), my = 140;
    tft.fillCircle(mx, my, 9, cBg);
    drawRRect(mx - 6, my - 6, 12, 12, 6, cText, cBg);
    snprintf(tb, sizeof tb, "%d%c", (int)roundf(wx.lo), 0xB0);
    drawText(F_TXT, tb, 24, 176, AL_L, cText, cBg);
    snprintf(tb, sizeof tb, "%d%c", (int)roundf(wx.hi), 0xB0);
    drawText(F_TXT, tb, 216, 176, AL_R, cText, cBg);
    drawText(F_CAP, "LOW", 24, 196, AL_L, cDim, cBg);
    drawText(F_CAP, "HIGH", 216, 196, AL_R, cDim, cBg);
    drawIcon(IC_DROP, 34, 221, 16, cBg);
    snprintf(tb, sizeof tb, "%d%%", wx.hum);
    drawText(F_TXT, tb, 48, 228, AL_L, cText, cBg);
    drawIcon(IC_WIND, 124, 221, 16, cBg);
    snprintf(tb, sizeof tb, "%d %s", (int)roundf(wx.wind), cfg.fahrenheit ? "mph" : "km/h");
    drawText(F_TXT, tb, 138, 228, AL_L, cText, cBg);
    return;
  }
  // forecast
  snprintf(tb, sizeof tb, "%s %c %d%%", wxShort(wx.code, wx.isDay), 0xB7, wx.hum);
  drawText(F_TXT, tb, 120, 30, AL_C, cDim, cBg);
  drawIcon(*wxIcon(wx.code, wx.isDay), 48, 78, 64, cBg);
  snprintf(tb, sizeof tb, "%d%c", (int)roundf(wx.temp), 0xB0);
  if (textW(F_TIME, tb) > 146) tb[strlen(tb) - 1] = 0;           // three-digit values: drop the degree sign
  drawText(F_TIME, tb, 162, 110, AL_C, cText, cBg);
  tft.fillRect(20, 124, 200, 1, cTrack);
  int wd = -1;
  if (timeValid()) { time_t n = time(nullptr); struct tm lt; localtime_r(&n, &lt); wd = lt.tm_wday; }
  for (int i = 0; i < 3; i++) {
    int cx = 48 + 72 * i;
    if (wd >= 0) drawText(F_CAP, WDU[(wd + i) % 7], cx, 146, AL_C, cDim, cBg);
    drawIcon(*wxIcon(wx.dcode[i], true), cx, 168, 32, cBg);
    snprintf(tb, sizeof tb, "%d%c", (int)roundf(wx.dhi[i]), 0xB0);
    drawText(F_VAL, tb, cx, 208, AL_C, cText, cBg);
    snprintf(tb, sizeof tb, "%d%c", (int)roundf(wx.dlo[i]), 0xB0);
    drawText(F_TXT, tb, cx, 230, AL_C, cDim, cBg);
  }
}

// ------------------------------------------------------------------ status page
void renderStats() {
  int sec = millis() / 1000;
  static char cache[6][26];
  bool full = fullRedraw;
  if (!full && sec == lastSec) return;
  lastSec = sec;
  if (full) {
    fullRedraw = false;
    clearScreen();
    static const char* const L[6] = {"ADDRESS", "SIGNAL", "FREE MEMORY", "UPTIME", "MQTT", "FIRMWARE"};
    for (int i = 0; i < 6; i++) { cache[i][0] = 0; drawText(F_CAP, L[i], 20, 8 + 38 * i + 12, AL_L, cDim, cBg); }
  }
  char v[6][26];
  snprintf(v[0], 26, "%s", WiFi.localIP().toString().c_str());
  snprintf(v[1], 26, "%d dBm", WiFi.RSSI());
  snprintf(v[2], 26, "%u KB", (unsigned)(ESP.getFreeHeap() / 1024));
  uint32_t u = sec;
  snprintf(v[3], 26, "%ud %02u:%02u:%02u", (unsigned)(u / 86400), (unsigned)((u / 3600) % 24), (unsigned)((u / 60) % 60), (unsigned)(u % 60));
  snprintf(v[4], 26, "%s", !cfg.mqttEnabled ? "Off" : mqtt.connected() ? "Connected" : "Offline");
  snprintf(v[5], 26, "v%s", FW_VERSION);
  for (int i = 0; i < 6; i++) {
    if (strcmp(v[i], cache[i]) == 0) continue;
    strcpy(cache[i], v[i]);
    drawText(F_TXT, v[i], 20, 8 + 38 * i + 34, AL_L, cText, cBg, 200);
  }
}

// ------------------------------------------------------------------ GIF player
static File gifFile;
static uint32_t gifStartMs = 0;
static bool gifSwitchPending = false;
void* GIFOpen(const char* fname, int32_t* pSize) {
  gifFile = LittleFS.open(fname, "r");
  if (!gifFile) return nullptr;
  *pSize = gifFile.size();
  return (void*)&gifFile;
}
void GIFClose(void* h) { File* f = (File*)h; if (f) f->close(); }
int32_t GIFRead(GIFFILE* pFile, uint8_t* pBuf, int32_t iLen) {
  File* f = (File*)pFile->fHandle;
  int32_t n = iLen;
  if ((pFile->iSize - pFile->iPos) < iLen) n = pFile->iSize - pFile->iPos - 1;
  if (n <= 0) return 0;
  n = f->read(pBuf, n);
  pFile->iPos = f->position();
  return n;
}
int32_t GIFSeek(GIFFILE* pFile, int32_t pos) {
  File* f = (File*)pFile->fHandle;
  f->seek(pos);
  pFile->iPos = (int32_t)f->position();
  return pFile->iPos;
}
void GIFDraw(GIFDRAW* d) {
  static uint16_t line[240];
  static int nextY = 0;
  int y = d->y + d->iY + gifOffY;
  int x0 = d->iX + gifOffX, w = d->iWidth;
  uint8_t* s = d->pPixels;
  uint16_t* pal = d->pPalette;
  if (d->ucDisposalMethod == 2) {
    for (int x = 0; x < w; x++) if (s[x] == d->ucTransparent) s[x] = d->ucBackground;
    d->ucHasTransparency = 0;
  }
  bool fits = x0 >= 0 && x0 + w <= 240 && d->iY + gifOffY >= 0 && d->iY + gifOffY + d->iHeight <= 240;
  if (!d->ucHasTransparency && fits) {
    // Opaque frame (e.g. the first frame of the loop): open the window once, then stream every row.
    if (d->y == 0) { tft.setAddrWindow(x0, d->iY + gifOffY, w, d->iHeight); nextY = 1; }
    if (d->y == 0 || d->y == nextY) {
      if (d->y != 0) nextY++;
      for (int x = 0; x < w; x++) line[x] = pal[s[x]];
      tft.pushPixels(line, w);
      return;
    }
  }
  if (y < 0 || y >= 240 || x0 >= 240) return;
  if (x0 + w > 240) w = 240 - x0;
  if (d->ucHasTransparency) {
    int x = 0;
    while (x < w) {
      int start = x;
      while (x < w && s[x] != d->ucTransparent) { line[x - start] = pal[s[x]]; x++; }
      if (x > start) tft.pushImage(x0 + start, y, x - start, 1, line);
      while (x < w && s[x] == d->ucTransparent) x++;
    }
  } else {
    for (int x = 0; x < w; x++) line[x] = pal[s[x]];
    tft.pushImage(x0, y, w, 1, line);
  }
}

void stopGif() { if (gif) { gif->close(); delete gif; gif = nullptr; } }

bool startGif() {
  stopGif();
  if (!gifOk) return false;
  gif = new (std::nothrow) AnimatedGIF();
  if (!gif) return false;
  gif->begin(LITTLE_ENDIAN_PIXELS);      // if colors look wrong, try BIG_ENDIAN_PIXELS
  char p[48];
  if (!resolveGif(cfg.gif, p, sizeof p)) { delete gif; gif = nullptr; return false; }
  if (!gif->open(p, GIFOpen, GIFClose, GIFRead, GIFSeek, GIFDraw)) { delete gif; gif = nullptr; return false; }
  gifOffX = max(0, (240 - gif->getCanvasWidth()) / 2);
  gifOffY = max(0, (240 - gif->getCanvasHeight()) / 2);
  tft.fillScreen(TFT_BLACK);
  gifNext = 0; gifStartMs = millis(); gifSwitchPending = false;
  return true;
}

// ---- GIF playlist: all GIFs in alphabetical order; "next" wraps around.
static int listGifs(char names[][32], int maxN) {
  int n = 0;
  const char* dirs[] = {GIF_DIR, GIF_DIR2};
  for (const char* dn : dirs) {
    Dir dir = LittleFS.openDir(dn);
    while (dir.next() && n < maxN) {
      String nm = dir.fileName();
      if (!dir.isFile() || !validGifName(nm)) continue;
      bool dup = false;
      for (int i = 0; i < n; i++) if (strcasecmp(names[i], nm.c_str()) == 0) dup = true;
      if (!dup) { strlcpy(names[n], nm.c_str(), 32); n++; }
    }
  }
  for (int i = 1; i < n; i++) {                       // insertion sort
    char tmp[32]; strcpy(tmp, names[i]); int j = i - 1;
    while (j >= 0 && strcasecmp(names[j], tmp) > 0) { strcpy(names[j + 1], names[j]); j--; }
    strcpy(names[j + 1], tmp);
  }
  return n;
}

// Moves to the next GIF. Returns true if a different GIF was selected (and started, when the GIF page is showing).
bool gifAdvance(bool save) {
  if (!fsOk) return false;
  static char names[24][32];
  int n = listGifs(names, 24);
  if (n < 2) return false;
  int cur = -1;
  for (int i = 0; i < n; i++) if (strcasecmp(names[i], cfg.gif) == 0) cur = i;
  for (int k = 1; k <= n; k++) {
    int idx = (cur + k + n) % n;
    strlcpy(cfg.gif, names[idx], sizeof cfg.gif);
    refreshGifOk();
    if (page != PAGE_GIF || startGif()) { if (save) saveSettings(cfg); return true; }
  }
  gifOk = false; setPage(PAGE_CLOCK);                 // none of them could be opened
  return false;
}

void gifTick() {
  if (!gif || (int32_t)(millis() - gifNext) < 0) return;
  if (gifSwitchPending) {                             // the previous GIF finished its last frame: start the next one
    gifSwitchPending = false;
    if (gifAdvance(false)) return;
    if (!gif) return;
  }
  uint32_t start = millis();
  int d = 0;
  tft.startWrite();                                   // keep the display transaction open for the whole frame
  int r = gif->playFrame(false, &d);
  tft.endWrite();
  if (r < 0) { stopGif(); gifOk = false; setPage(nextPageFrom(page)); return; }   // bad file: skip it
  if (r == 0) {                                       // last frame shown: rewind for a seamless loop
    gif->reset();
    if (cfg.gifAuto && (uint32_t)(millis() - gifStartMs) >= (uint32_t)cfg.gifSecs * 1000UL) gifSwitchPending = true;
  }
  if (d < 10) d = 10;
  gifNext = start + d;                                // the frame's time includes its decode time, so loops keep the GIF's own pace
}

// ------------------------------------------------------------------ page control
void setPage(uint8_t p) {
  if (page == PAGE_GIF && p != PAGE_GIF) stopGif();
  page = p; pageSince = millis(); fullRedraw = true; lastSec = lastMin = lastDay = -1; wxDirty = true;
  if (p == PAGE_GIF) {
    if (startGif()) fullRedraw = false;
    else { page = PAGE_CLOCK; }
  }
}

void applyConfig() {
  rebuildColors();
  tft.setRotation(cfg.flip ? 2 : 0);
  configTime(cfg.tz, "pool.ntp.org", "time.cloudflare.com");
  refreshGifOk();
  applyBrightness();
  nextWeather = 0;
  if (mqtt.connected()) mqtt.disconnect();
  mqttNext = 0;
  snprintf(tStatus, sizeof tStatus, "%s/status", cfg.mqttTopic);
  snprintf(tState, sizeof tState, "%s/state", cfg.mqttTopic);
  snprintf(tBrt, sizeof tBrt, "%s/brightness", cfg.mqttTopic);
  snprintf(tBrtSet, sizeof tBrtSet, "%s/brightness/set", cfg.mqttTopic);
  snprintf(tPageSet, sizeof tPageSet, "%s/page/set", cfg.mqttTopic);
  snprintf(tGifNext, sizeof tGifNext, "%s/gif/next", cfg.mqttTopic);
  if (cfg.mode > 0) {                                   // one page pinned
    uint8_t t = cfg.mode - 1;
    if (t == PAGE_GIF && !gifOk) t = PAGE_CLOCK;
    setPage(t);
  } else setPage(pageUsable(page) ? page : nextPageFrom(page));
}

// ------------------------------------------------------------------ MQTT
String deviceUid() { return String(cfg.hostname) + "_" + String(ESP.getChipId(), HEX); }

void publishState() {
  if (!mqtt.connected()) return;
  JsonDocument d;
  if (wx.valid) d["temp"] = wx.temp;
  d["condition"] = wx.valid ? wxText(wx.code) : "unknown";
  d["rssi"] = WiFi.RSSI();
  d["heap"] = ESP.getFreeHeap();
  d["uptime"] = millis() / 1000;
  d["page"] = page;
  char buf[200]; size_t n = serializeJson(d, buf, sizeof buf);
  mqtt.publish(tState, (const uint8_t*)buf, n, true);
  char b[8]; snprintf(b, sizeof b, "%u", cfg.brightness);
  mqtt.publish(tBrt, b, true);
}

void discover(const char* comp, const char* key, const char* name, const char* cls, const char* unit, const char* tpl) {
  String uid = deviceUid();
  char topic[110]; snprintf(topic, sizeof topic, "homeassistant/%s/%s_%s/config", comp, uid.c_str(), key);
  JsonDocument d;
  d["name"] = name;
  d["unique_id"] = uid + "_" + key;
  d["availability_topic"] = tStatus;
  if (cls) d["device_class"] = cls;
  if (unit) d["unit_of_measurement"] = unit;
  if (!strcmp(comp, "number")) {
    d["command_topic"] = tBrtSet; d["state_topic"] = tBrt;
    d["min"] = 0; d["max"] = 100; d["icon"] = "mdi:brightness-6";
  } else {
    d["state_topic"] = tState; d["value_template"] = tpl;
  }
  JsonObject dev = d["device"].to<JsonObject>();
  dev["identifiers"][0] = uid;
  dev["name"] = cfg.hostname;
  dev["manufacturer"] = "DIY";
  dev["model"] = "SmallTV firmware";
  dev["sw_version"] = FW_VERSION;
  char buf[640]; size_t n = serializeJson(d, buf, sizeof buf);
  mqtt.publish(topic, (const uint8_t*)buf, n, true);
}

void mqttCb(char* topic, byte* pl, unsigned len) {
  char b[16]; len = min(len, 15u); memcpy(b, pl, len); b[len] = 0;
  int v = atoi(b);
  if (!strcmp(topic, tBrtSet)) { cfg.brightness = constrain(v, 0, 100); applyBrightness(); publishState(); }
  else if (!strcmp(topic, tPageSet) && pageUsable(v)) setPage(v);
  else if (!strcmp(topic, tGifNext)) { if (gifAdvance(false) && page != PAGE_GIF) setPage(PAGE_GIF); }
}

void mqttLoop(uint32_t now) {
  if (!cfg.mqttEnabled || !cfg.mqttHost[0] || WiFi.status() != WL_CONNECTED) return;
  if (mqtt.connected()) {
    mqtt.loop();
    if (now - mqttPub > 30000) { mqttPub = now; publishState(); }
    return;
  }
  if (now < mqttNext) return;
  mqttNext = now + 15000;
  mqtt.setServer(cfg.mqttHost, cfg.mqttPort);
  mqtt.setBufferSize(768);
  mqtt.setCallback(mqttCb);
  String id = deviceUid();
  bool ok = cfg.mqttUser[0]
    ? mqtt.connect(id.c_str(), cfg.mqttUser, cfg.mqttPass, tStatus, 0, true, "offline")
    : mqtt.connect(id.c_str(), tStatus, 0, true, "offline");
  if (!ok) return;
  mqtt.publish(tStatus, "online", true);
  mqtt.subscribe(tBrtSet);
  mqtt.subscribe(tPageSet);
  mqtt.subscribe(tGifNext);
  if (cfg.haDiscovery) {
    discover("sensor", "temp", "Outdoor temperature", "temperature", cfg.fahrenheit ? "\xC2\xB0" "F" : "\xC2\xB0" "C", "{{ value_json.temp }}");
    discover("sensor", "rssi", "WiFi signal", "signal_strength", "dBm", "{{ value_json.rssi }}");
    discover("number", "brightness", "Brightness", nullptr, "%", nullptr);
  }
  publishState();
}

// ------------------------------------------------------------------ web API
void sendJson(JsonDocument& d, int code = 200) {
  String out; serializeJson(d, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", out);
}
void sendOk() { server.send(200, "application/json", "{\"ok\":true}"); }

void handleRoot() {   // the web UI lives inside the firmware (webui.h), no filesystem needed
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, "text/html", WEB_UI);
}

void handleStatus() {
  JsonDocument d;
  d["version"] = FW_VERSION; d["hostname"] = cfg.hostname;
  d["ip"] = WiFi.localIP().toString(); d["rssi"] = WiFi.RSSI();
  d["heap"] = ESP.getFreeHeap(); d["uptime"] = millis() / 1000; d["page"] = page;
  if (timeValid()) {
    time_t n = time(nullptr); struct tm t; localtime_r(&n, &t);
    char b[24]; strftime(b, sizeof b, "%H:%M:%S", &t); d["time"] = b;
    strftime(b, sizeof b, "%a %d %b", &t); d["date"] = b;
  }
  if (wx.valid) { d["temp"] = wx.temp; d["condition"] = wxText(wx.code); d["hi"] = wx.hi; d["lo"] = wx.lo; }
  d["mqtt"] = !cfg.mqttEnabled ? "off" : mqtt.connected() ? "connected" : "offline";
  d["fs"] = fsOk;
  if (fsOk) { FSInfo fs; LittleFS.info(fs); d["fsUsed"] = fs.usedBytes; d["fsTotal"] = fs.totalBytes; }
  else { d["fsUsed"] = 0; d["fsTotal"] = 0; }
  sendJson(d);
}

void handleConfigGet() { JsonDocument d; settingsToJson(cfg, d, false); sendJson(d); }

void handleConfigPost() {
  JsonDocument d;
  if (deserializeJson(d, server.arg("plain"))) { server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}"); return; }
  settingsFromJson(cfg, d.as<JsonVariantConst>());
  bool saved = fsOk && saveSettings(cfg);
  server.send(200, "application/json", saved ? "{\"ok\":true,\"saved\":true}" : "{\"ok\":true,\"saved\":false}");
  applyConfig();
}

void handlePage() {
  int n = server.arg("n").toInt();
  if (n < 0 || n >= PAGE_COUNT || (n == PAGE_GIF && !gifOk)) {
    server.send(400, "application/json", "{\"error\":\"That page is not available (GIF needs a selected file)\"}"); return;
  }
  setPage(n);
  if (cfg.mode != 0) { cfg.mode = n + 1; saveSettings(cfg); }   // in single-page mode, this changes the pinned page
  sendOk();
}

void handleGifNext() {
  if (!gifAdvance(true)) { server.send(400, "application/json", "{\"error\":\"Upload at least two GIFs first\"}"); return; }
  if (page != PAGE_GIF) setPage(PAGE_GIF);
  sendOk();
}

String cleanName(String n) {
  int s = max(n.lastIndexOf('/'), n.lastIndexOf('\\'));
  if (s >= 0) n = n.substring(s + 1);
  for (size_t i = 0; i < n.length(); i++) if (!(isalnum(n[i]) || n[i] == '.' || n[i] == '-' || n[i] == '_')) n[i] = '_';
  return n;
}
bool validGifName(const String& n) {
  if (n.length() < 5 || n.length() > 31 || n[0] == '.') return false;
  String l = n; l.toLowerCase();
  return l.endsWith(".gif");
}

void handleGifList() {
  JsonDocument d;
  JsonArray a = d["files"].to<JsonArray>();
  d["used"] = 0; d["total"] = 1;
  if (fsOk) {
    const char* dirs[] = {GIF_DIR, GIF_DIR2};
    for (const char* dn : dirs) {
      Dir dir = LittleFS.openDir(dn);
      while (dir.next()) {
        String n = dir.fileName();
        if (!dir.isFile() || !validGifName(n)) continue;       // skips boot.jpg and other files
        JsonObject o = a.add<JsonObject>(); o["name"] = n; o["size"] = dir.fileSize();
      }
    }
    FSInfo fs; LittleFS.info(fs); d["used"] = fs.usedBytes; d["total"] = fs.totalBytes;
  }
  d["selected"] = cfg.gif;
  sendJson(d);
}

File upFile; String upPath; bool upOk = false; size_t upFree = 0, upWritten = 0;
void handleUpload() {
  HTTPUpload& u = server.upload();
  if (u.status == UPLOAD_FILE_START) {
    upOk = false;
    if (!fsOk) return;
    String n = cleanName(u.filename);
    if (!validGifName(n)) return;
    FSInfo fs; LittleFS.info(fs);
    upFree = fs.totalBytes - fs.usedBytes; upWritten = 0;
    stopGif();
    upPath = String(GIF_DIR) + "/" + n;
    upFile = LittleFS.open(upPath, "w");
    upOk = (bool)upFile;
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (upOk) {
      if (upWritten + u.currentSize + 8192 > upFree || upFile.write(u.buf, u.currentSize) != u.currentSize) upOk = false;
      else upWritten += u.currentSize;
    }
  } else if (u.status == UPLOAD_FILE_END || u.status == UPLOAD_FILE_ABORTED) {
    if (upFile) upFile.close();
    if (u.status == UPLOAD_FILE_ABORTED) upOk = false;
    if (!upOk && upPath.length()) LittleFS.remove(upPath);
  }
}
void handleUploadDone() {
  if (upOk) sendOk();
  else server.send(400, "application/json", "{\"error\":\"Upload failed. Use a .gif that fits in free space.\"}");
  refreshGifOk();
  if (page == PAGE_GIF) setPage(PAGE_GIF);
}

void handleGifDelete() {
  String n = cleanName(server.arg("name"));
  if (!validGifName(n)) { server.send(400, "application/json", "{\"error\":\"Bad name\"}"); return; }
  if (page == PAGE_GIF && n == cfg.gif) stopGif();
  LittleFS.remove(String(GIF_DIR) + "/" + n);
  LittleFS.remove(String(GIF_DIR2) + "/" + n);
  if (n == cfg.gif) { cfg.gif[0] = 0; saveSettings(cfg); }
  refreshGifOk();
  if (page == PAGE_GIF && !gifOk) setPage(nextPageFrom(page));
  sendOk();
}

void handleReboot() { markHealthy(); sendOk(); delay(300); ESP.restart(); }
void handleFormat() {   // only offered when storage failed to mount; never formats a working filesystem
  if (fsOk) { server.send(400, "application/json", "{\"error\":\"Storage is working, not formatting\"}"); return; }
  LittleFS.format();
  fsOk = LittleFS.begin();
  if (fsOk) LittleFS.mkdir(GIF_DIR);
  if (fsOk) sendOk(); else server.send(500, "application/json", "{\"error\":\"Format failed\"}");
}
void handleReset() {
  if (fsOk) LittleFS.remove(CONFIG_PATH);
  markHealthy();
  sendOk(); delay(300);
  ESP.eraseConfig();            // forget the saved WiFi network
  ESP.restart();
}

// ------------------------------------------------------------------ recovery mode
// Minimal and deliberately boring: no display, no weather, no MQTT, no GIFs.
// Just WiFi + /update (+ ArduinoOTA). The backlight blinks so you can tell it is active.
void recoveryRoot() {
  String h = F("<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'><title>SmallTV recovery</title>"
               "<body style='font-family:system-ui,sans-serif;max-width:34rem;margin:2rem auto;padding:0 1rem;line-height:1.5'>"
               "<h1>Recovery mode</h1><p>The firmware restarted ");
  h += bootCount;
  h += F(" times in a row without starting properly, so only WiFi and updates are running. The screen backlight is blinking.</p>"
         "<p><a href='/update'><b>Upload SmallTV-Installer.bin</b></a> &ndash; it is small enough to always fit. Then use its page to install SmallTV-Modern.bin or the original firmware.</p>"
         "<form method=POST action='/recovery/retry'><button>Try a normal start</button></form><p>"
         "<form method=POST action='/recovery/reset' onsubmit=\"return confirm('Erase SmallTV settings?')\">"
         "<button>Erase SmallTV settings and try again</button></form>"
         "<p style='color:#667'>Firmware v");
  h += FW_VERSION;
  h += F("</p>");
  server.send(200, "text/html", h);
}
void recoveryRestart() {
  markHealthy();
  server.send(200, "text/html", F("<meta http-equiv=refresh content='10;url=/'>Restarting..."));
  delay(300); ESP.restart();
}
void recoveryReset() {
  LittleFSConfig fsc; fsc.setAutoFormat(false); LittleFS.setConfig(fsc);
  if (LittleFS.begin()) LittleFS.remove(CONFIG_PATH);
  recoveryRestart();
}
void recoverySetup() {
  pinMode(BL_PIN, OUTPUT);
  snprintf(apName, sizeof apName, "SmallTV-Recovery-%06X", ESP.getChipId());
  if (!wifiConnectOrPortal(apName, "smalltv-recovery", nullptr, 180)) ESP.restart();   // stays in recovery mode
  if (MDNS.begin("smalltv-recovery")) MDNS.addService("http", "tcp", 80);
  server.on("/", HTTP_GET, recoveryRoot);
  server.on("/recovery/retry", HTTP_POST, recoveryRestart);
  server.on("/recovery/reset", HTTP_POST, recoveryReset);
  httpUpdater.setup(&server, "/update");
  server.begin();
}
void recoveryLoop() {
  server.handleClient();
  MDNS.update();
  digitalWrite(BL_PIN, (millis() / 600) & 1);
  yield();
}

// ------------------------------------------------------------------ import from the stock firmware
// The stock firmware keeps its files in the same LittleFS region this build uses (flash 0x300000, 1 MB),
// so its GIFs and WiFi login are still there after an over-the-air flash. Read-only: nothing is modified.
void importStockWifi() {
  if (!fsOk || WiFi.SSID().length()) return;          // already have a saved network
  File f = LittleFS.open("/wifi.json", "r");
  if (!f) return;
  JsonDocument d;
  bool bad = (bool)deserializeJson(d, f);
  f.close();
  if (bad) return;
  const char* ssid = d["a"].as<const char*>();         // stock format: {"a":"<ssid>","p":"<password>"}
  const char* pass = d["p"].as<const char*>();
  if (!ssid || !ssid[0]) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass ? pass : "");
  for (uint32_t t = millis(); WiFi.status() != WL_CONNECTED && millis() - t < 15000;) delay(200);
}
void importStockGif() {
  File f = LittleFS.open("/gif.json", "r");            // stock format: {"gif":"<name>"}
  if (!f) return;
  JsonDocument d;
  bool bad = (bool)deserializeJson(d, f);
  f.close();
  if (bad) return;
  const char* g = d["gif"].as<const char*>();
  if (!g) return;
  const char* b = strrchr(g, '/'); b = b ? b + 1 : g;
  char p[48];
  if (resolveGif(b, p, sizeof p)) strlcpy(cfg.gif, b, sizeof cfg.gif);
}

// ------------------------------------------------------------------ setup / loop
void splash(const char* l1, const char* l2 = nullptr) {
  tft.fillScreen(cBg);
  drawText(F_TXT, l1, 120, l2 ? 116 : 130, AL_C, cText, cBg);
  if (l2) drawText(F_TXT, l2, 120, 148, AL_C, cDim, cBg);
}

void apCallback(const char* ap) {
  splash("Join WiFi", ap);
  drawText(F_TXT, "then open 192.168.4.1", 120, 184, AL_C, cDim, cBg);
}

void setup() {
  Serial.begin(115200);
  bootGuard();
  if (recoveryMode) { recoverySetup(); return; }
  pinMode(BL_PIN, OUTPUT);
  analogWriteRange(1023);
  setBacklight(0);
  // Never auto-format: if the stock filesystem can't be mounted we leave it untouched.
  LittleFSConfig fsc; fsc.setAutoFormat(false); LittleFS.setConfig(fsc);
  fsOk = LittleFS.begin();
  bool firstRun = false;
  if (fsOk) { firstRun = !LittleFS.exists(CONFIG_PATH); LittleFS.mkdir(GIF_DIR); loadSettings(cfg); }
  tft.init();
  tft.setSwapBytes(true);
  tft.setRotation(cfg.flip ? 2 : 0);
  rebuildColors();
  splash("SmallTV", "Starting...");
  applyBrightness();
  if (fsOk && firstRun) { importStockGif(); saveSettings(cfg); }
  splash("WiFi", "Connecting...");
  importStockWifi();

  snprintf(apName, sizeof apName, "SmallTV-%06X", ESP.getChipId());
  if (!wifiConnectOrPortal(apName, cfg.hostname, apCallback, 180)) ESP.restart();

  splash("Connected", WiFi.localIP().toString().c_str());
  if (MDNS.begin(cfg.hostname)) MDNS.addService("http", "tcp", 80);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/config", HTTP_GET, handleConfigGet);
  server.on("/api/config", HTTP_POST, handleConfigPost);
  server.on("/api/page", HTTP_POST, handlePage);
  server.on("/api/gifnext", HTTP_POST, handleGifNext);
  server.on("/api/gifs", HTTP_GET, handleGifList);
  server.on("/api/gif", HTTP_POST, handleUploadDone, handleUpload);
  server.on("/api/gif", HTTP_DELETE, handleGifDelete);
  server.on("/api/reboot", HTTP_POST, handleReboot);
  server.on("/api/reset", HTTP_POST, handleReset);
  server.on("/api/format", HTTP_POST, handleFormat);
  httpUpdater.setup(&server, "/update");
  server.begin();

  page = PAGE_COUNT;           // force a clean first draw
  applyConfig();
  delay(800);
  pageSince = millis();
}

void loop() {
  if (recoveryMode) { recoveryLoop(); return; }
  server.handleClient();
  MDNS.update();
  uint32_t now = millis();
  if (!healthyMarked && now > HEALTHY_MS) { markHealthy(); healthyMarked = true; }
  mqttLoop(now);
  if (now >= nextWeather && page != PAGE_GIF) fetchWeather();

  switch (page) {
    case PAGE_CLOCK:   renderClock(); break;
    case PAGE_WEATHER: renderWeather(); break;
    case PAGE_STATS:   renderStats(); break;
    case PAGE_GIF:     gifTick(); break;
  }

  if (rotating() && now - pageSince > (uint32_t)cfg.rotateSec * 1000UL) setPage(nextPageFrom(page));
  if (now - lastSlow >= 30000) { lastSlow = now; applyBrightness(); }
  if (page != PAGE_GIF) delay(10);
  yield();
}
