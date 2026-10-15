# 📺 GeekMagic Small TV - Custom Modern Firmware

![Platform](https://img.shields.io/badge/Platform-ESP8266-blue)
![Display](https://img.shields.io/badge/Display-ST7789%20240x240-green)
![Build](https://img.shields.io/badge/Build-PlatformIO-orange)
![Status](https://img.shields.io/badge/Status-Tested%20%26%20Working-success)

A custom modern firmware specifically designed and tested for the **GeekMagic Small TV** (and Ultra variants) desktop gadgets featuring an ESP8266 processor and a 240x240 ST7789 display.

![UI Preview](ui-preview.png) 

---

## ✨ Features

### 🎨 The Look (Web UI & Display)
- **Flicker-Free Rendering:** Everything is painted once, and only changed parts are repainted. No flicker or lag.
- **Custom Typography & Icons:** Uses real fonts (Outfit Bold for numbers/labels, Poppins Medium for text) and flat icons (stored compressed to save space).
- **Color Customization:** Pick your **accent color** and **time color** via a color box or quick swatches. Choose from four base themes (Black, Midnight, Graphite, Light). Customizations include optional date, info blocks, seconds line, page dots, and 12/24h formats.

### ⏱️ Clock & Weather Styles
- **9 Clock Faces:** Classic, Accent, Cards, Stacked (hours over minutes), Day Progress, Glance (clock + one weather card), Week (clock + 7-day strip), Sidebar (stacked clock + date/weather panel), and Date (date as the hero). Every face intelligently keeps related info together.
- **6 Weather Faces:** Forecast (now + 3 days), Card, Tiles (humidity, wind, high, low), Range (where "now" sits between today's high/low), Hourly (now + next 4 hours with bars), and List (big temperature + labeled rows).
- **Smart Visuals:** The range marker and hourly bars use the exact rounded numbers printed on-screen, ensuring a completely proportional and accurate visual representation of the temperature.

### 📅 Calendar 
A dedicated month calendar page with three distinct styles: **Month Grid**, **Tiles**, and **Focus** (big day number + small month).
- **Customization:** Choose a Monday or Sunday start, pick a custom weekend color, and dim days from neighboring months. 
- **Header Clock:** Includes a small header clock so you never lose track of time. Today's date is always highlighted in your accent color. 

### ⏳ Timer & Stopwatch
Take over the screen when you need to focus or track time:
- **Timer:** Start a countdown (use presets or custom mm:ss up to 99:59) with Pause/Resume and Cancel. The screen flashes for up to a minute when the timer ends.
- **Stopwatch:** Features Start, Stop, Lap (displays the last 3 laps), and Reset. 

### 🔀 Custom Playlists (Display Modes)
Don't want to just stick to one screen? Set your Display Mode to **Custom Playlist**:
- Build a custom rotation of up to 10 items.
- Assign specific screens (Clock, Weather, Status, GIF), pick a specific face for that screen, and set a custom duration in seconds.
- *Example: Weather Hourly (5s) ➔ Weather Range (5s) ➔ Clock Glance (2s) ➔ Weather Forecast (10s).*

### 🎬 GIF Support
- **Upload & Play:** Upload as many GIFs as memory allows. Pick one to use, or auto-play all GIFs in turn, switching at the end of a loop based on your timing settings.
- **Smart Looping:** Loops keep the GIF's original pacing. The first frame is drawn in one streamed pass for minimal pause at the loop point.
- **Smart Home Integration:** Publish to `<topic>/gif/next` via Home Assistant / MQTT to dynamically skip to the next GIF.

---

## 🚀 Installation (WiFi Only, OTA)

### Why Two Files?
The original firmware's updater only has room for a new firmware of about **524 KB**. To ensure strict memory safety (avoiding the `"ERROR[4] Not Enough Space"` bug), updating is a two-step process:
1. **`SmallTV-Installer.bin`** (small) is flashed first via the original update page.
2. **`SmallTV-Modern.bin`** (full) is flashed through the new installer's page, which has more room.

### Step-by-Step Guide

1. **Get the files:** Download the latest binaries (`SmallTV-bins`) from the **Actions** tab on GitHub, or build it locally using `pio run`.
2. **Flash the Installer:** Open `http://<device-ip>/update` on your current original firmware. Under *Firmware* (not FileSystem), select **`SmallTV-Installer.bin`** and press Update. Wait ~1 minute. Do not unplug. (The screen going blank or white is normal).
3. **Connect to the Installer:** The installer tries to borrow your existing WiFi credentials. Look for `smalltv-installer` on your network. If it fails, it will broadcast an AP named `SmallTV-Installer-XXXXXX`. Connect to it, go to `192.168.4.1`, and enter your WiFi details.
4. **Flash the Custom Modern Firmware:** Open the installer's IP address, click **Open the upload page**, select **`SmallTV-Modern.bin`** under *Firmware*, and press Update.
5. **Setup:** Open the IP address shown on the device screen (or go to `http://smalltv.local`) to configure your location, time zone, and UI styles.

---

## 🔄 Updates & Maintenance

### Updating to Newer Versions
The full custom modern firmware is too large to replace itself directly. To update in the future:
1. Open `<device-ip>/update` and upload `SmallTV-Installer.bin`.
2. Go to the installer's address, click **Open the upload page**, and upload the new `SmallTV-Modern.bin`. 
*(Your settings and GIFs are retained during this process).*

### Reverting to Original Stock Firmware
Follow the same update steps, but upload the original `FW-Smalltv-Ultra-V9_0_50.bin` via the installer's upload page. This firmware uses its own config (`smalltv_cfg.json`) and never formats the original firmware's files.

### 🛡️ Safety Net & Recovery
- **Recovery Mode:** 3 restarts in a row without running for 3 minutes triggers a minimal mode (WiFi + upload page, backlight blinking). You can trigger this manually by pressing the reset button 3 times quickly. Upload `SmallTV-Installer.bin` here to recover.
- **Limitations:** Power loss during the final copy of an update can corrupt the image (requiring a serial flash). A crash that happens *after* 3+ minutes of uptime will not trigger recovery mode. 
- **Wrong Pins:** A wrong pin setting in the code is generally harmless—the screen may go blank, but WiFi and Web UI will remain functional for OTA fixes.

---

## 🛠️ Hardware Configuration

**Tested Pinout for GeekMagic Small TV:**

| Item | Hardware | Defined In | Notes |
|---|---|---|---|
| **Display** | ST7789 240x240 | `platformio.ini` | |
| **MOSI / SCLK** | GPIO13 / GPIO14 | ESP8266 Hardware SPI | Fixed pins |
| **DC / RST** | GPIO0 / GPIO2 | `platformio.ini` | |
| **CS** | None | N/A | Tied low |
| **Backlight** | GPIO5 | `src/settings.h` | Active low |

**Troubleshooting Display Issues:**
- *Swapped Red/Blue?* Add `-DTFT_RGB_ORDER=TFT_BGR` to build flags.
- *Inverted Colors?* Add `-DTFT_INVERSION_ON` to build flags.
- *Garbled GIF Colors?* Change `LITTLE_ENDIAN_PIXELS` to `BIG_ENDIAN_PIXELS` in `startGif()`.

---

## 📝 Development Notes

- **Weather API:** Uses Open-Meteo (no API key required) over plain HTTP to save RAM. If the API requires HTTPS in your region, uncomment `-DWEATHER_HTTPS=1`.
- **Data Import:** The original firmware reads GIFs from `/image` (and `/gif`). The stock WiFi file format was inferred from the original binary, so automatic credential importing is "best-effort".
- **Generated Assets:** The web UI is embedded in `src/webui.h`. Fonts (`src/fonts.h`) and icons (`src/icons.h`) are generated, compressed 4-bit alpha files.

---

## 📄 License & Credits

- **Fonts:** 
  - *Outfit Bold* by The Outfit Project Authors (SIL Open Font License 1.1)
  - *Poppins Medium* by Indian Type Foundry (SIL Open Font License 1.1)
- **Code:** [Insert Your License Here - e.g., MIT License]

Contributions and bug reports are welcome!
