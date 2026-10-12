// SmallTV installer: a tiny firmware whose only job is to be small enough to install over the stock
// firmware's update page, and then accept the full firmware (or the original one) at /update.
// It only READS the stock filesystem (to borrow the WiFi login) and never writes or formats it.
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "wifisetup.h"

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

void importStockWifi() {
  File f = LittleFS.open("/wifi.json", "r");               // stock format: {"a":"<ssid>","p":"<password>"}
  if (!f) return;
  JsonDocument d;
  bool bad = (bool)deserializeJson(d, f);
  f.close();
  if (bad) return;
  const char* ssid = d["a"].as<const char*>();
  const char* pass = d["p"].as<const char*>();
  if (!ssid || !ssid[0]) return;
  WiFi.persistent(true);
  WiFi.mode(WIFI_STA);
  WiFi.hostname("smalltv-installer");
  WiFi.begin(ssid, pass ? pass : "");
  waitWifi(15000);
}

void handleRoot() {
  uint32_t maxKB = ((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000) / 1024;
  String h = F("<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'><title>SmallTV installer</title>"
               "<body style='font-family:system-ui,sans-serif;max-width:34rem;margin:2rem auto;padding:0 1rem;line-height:1.5'>"
               "<h1>SmallTV installer</h1><p>This small program only exists to install other firmware.</p>"
               "<p style='font-size:1.3rem'><a href='/update'><b>Open the upload page</b></a></p>"
               "<p>Choose <b>SmallTV-Modern.bin</b> (or the original firmware to go back). Do not unplug the device while it updates.</p>"
               "<p style='color:#667'>Largest file accepted now: ");
  h += maxKB;
  h += F(" KB. Address: ");
  h += WiFi.localIP().toString();
  h += F("</p>");
  server.send(200, "text/html", h);
}

void setup() {
  Serial.begin(115200);
  LittleFSConfig fsc; fsc.setAutoFormat(false); LittleFS.setConfig(fsc);   // never format
  if (LittleFS.begin()) { importStockWifi(); LittleFS.end(); }
  char ap[32]; snprintf(ap, sizeof ap, "SmallTV-Installer-%06X", ESP.getChipId());
  if (!wifiConnectOrPortal(ap, "smalltv-installer", nullptr, 300)) ESP.restart();
  server.on("/", HTTP_GET, handleRoot);
  httpUpdater.setup(&server, "/update");
  server.begin();
}

void loop() { server.handleClient(); yield(); }
