// Minimal WiFi setup: join the saved network, or open an access point with a one-page form.
#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

inline String htmlEsc(const String& s) {
  String o; o.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') o += F("&amp;");
    else if (c == '<') o += F("&lt;");
    else if (c == '>') o += F("&gt;");
    else if (c == '"') o += F("&quot;");
    else o += c;
  }
  return o;
}

inline bool waitWifi(uint32_t ms) {
  for (uint32_t t = millis(); WiFi.status() != WL_CONNECTED && millis() - t < ms;) delay(100);
  return WiFi.status() == WL_CONNECTED;
}

// Returns true when connected. If not, opens "apName" and serves a form at 192.168.4.1 for portalSec seconds.
inline bool wifiConnectOrPortal(const char* apName, const char* hostname, void (*onPortal)(const char*), uint32_t portalSec) {
  WiFi.persistent(true);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostname);
  if (WiFi.status() != WL_CONNECTED) { WiFi.begin(); waitWifi(15000); }
  if (WiFi.status() == WL_CONNECTED) return true;

  String options;
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n && i < 20; i++) options += "<option value=\"" + htmlEsc(WiFi.SSID(i)) + "\">";
  if (onPortal) onPortal(apName);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName);

  ESP8266WebServer portal(80);
  bool saved = false;
  String ssid, pass;
  portal.on("/", HTTP_GET, [&]() {
    String h = F("<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
                 "<body style='font-family:system-ui,sans-serif;max-width:28rem;margin:2rem auto;padding:0 1rem'>"
                 "<h2>Connect to WiFi</h2><form method=POST action=/save><p>Network<br>"
                 "<input name=s list=n autocapitalize=off style='width:100%;padding:.5rem'><datalist id=n>");
    h += options;
    h += F("</datalist><p>Password<br><input name=p type=password style='width:100%;padding:.5rem'>"
           "<p><button style='padding:.6rem 1rem'>Connect</button></form>");
    portal.send(200, "text/html", h);
  });
  portal.on("/save", HTTP_POST, [&]() {
    ssid = portal.arg("s"); pass = portal.arg("p");
    saved = ssid.length() > 0;
    portal.send(200, "text/html", F("<meta name=viewport content='width=device-width,initial-scale=1'>Saved. The device is connecting. You can close this page."));
  });
  portal.onNotFound([&]() { portal.sendHeader("Location", "http://192.168.4.1/"); portal.send(302, "text/plain", ""); });
  portal.begin();
  for (uint32_t t = millis(); !saved && millis() - t < portalSec * 1000UL;) { portal.handleClient(); delay(5); }
  delay(500);
  portal.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  if (!saved) return false;
  WiFi.begin(ssid.c_str(), pass.c_str());
  return waitWifi(20000);
}
