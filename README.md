# SmallTV modern firmware (ESP8266, 240x240 ST7789)

**Status: written but NOT compiled for the device or tested on hardware.** The drawing and screen code was compiled and run on a
computer against a fake display (that is what `ui-preview.png` shows), but the rest still needs a real build. Expect to fix small
build errors, and check the pin table below.

## Why two files?
The original firmware's updater only has room for a new firmware of about **524 KB**. So **SmallTV-Installer.bin** (small) goes on
first through the original update page, then **SmallTV-Modern.bin** (full) goes on through the installer's page, which has more room.

## Memory safety ("ERROR[4] Not Enough Space")
The build checks the size of each file against the exact limit of the page it will be uploaded through, and **fails with a clear
message instead of producing a file that would not fit**. The log shows e.g. `SmallTV-Modern.bin: 612345 bytes. Limit ... (80 KB to
spare)`. The installer page also shows "Largest file accepted now: X KB". Fonts and icons are stored compressed (~24 KB in total),
and the unused OTA library and built-in fonts were removed, so this version is smaller than the previous one.

## Get the files
Upload this folder to GitHub (keep `.github`). The **Actions** tab builds both files; download the artifact `SmallTV-bins`.
Or run `pio run` locally.

## Install (WiFi only, no wires)
1. Open `http://<device-ip>/update` (original firmware). Under *Firmware* (not FileSystem) pick **SmallTV-Installer.bin**, press Update.
   Wait about a minute. Do not unplug. The screen may go blank or white: normal at this step.
2. The installer borrows the original firmware's WiFi login. Look for `smalltv-installer` in your router's list. If it can't join, it
   opens a WiFi network `SmallTV-Installer-XXXXXX`: join it from your phone, open `192.168.4.1`, and enter your WiFi.
3. Open the installer's address, click **Open the upload page**, pick **SmallTV-Modern.bin** under *Firmware*, press Update.
4. Open the address shown on the screen (Status page) or `http://smalltv.local`, set location, time zone and style.

## The look (web UI, Display tab)
- **Clock styles:** Classic (white), Accent (your accent color), Cards (rounded cards). Optional info blocks under the clock:
  outside temperature plus humidity, wind, or today's high. Optional date, seconds line, and 12/24 h.
- **Weather styles:** Forecast (now + next three days, like a phone widget) and Card (big temperature with pills).
- **Color themes:** Black, Midnight, Graphite, Light, plus your own accent color.
- **Display mode:** rotate through pages or keep one page on screen. No header bar; optional page dots.
- Text is drawn with a custom rounded font (Outfit Bold, SIL Open Font License) and flat icons. Everything is painted once and then
  only changed parts are repainted, so there is no flicker or lag.
- **GIFs:** loops now keep the GIF's own pace, and the first frame is drawn in one streamed pass, so the pause at the loop point is
  much shorter. (A short repaint at the loop is unavoidable with this chip when the first frame is a full picture.)

## Updating later
The full firmware is too big to replace itself: open its `/update`, upload **SmallTV-Installer.bin**, wait, open the installer's
address, **Open the upload page**, upload the new **SmallTV-Modern.bin**. Settings and GIFs are kept.

## Going back to the original
Same steps, but send the original `FW-Smalltv-Ultra-V9_0_50.bin` from the installer's upload page. This firmware never formats the
original's files; it uses its own settings file (`smalltv_cfg.json`).

## Safety net
- **Recovery mode:** 3 restarts in a row without running for 3 minutes starts a minimal mode (WiFi + upload page, backlight
  blinking). Press reset 3 times quickly to trigger it by hand. Upload SmallTV-Installer.bin there.
- Still possible: power loss during an update's final copy can corrupt the image (needs a serial flash); a crash only after 3+
  minutes of uptime does not trigger recovery mode; no password on the web pages.
- A wrong pin setting is harmless: the screen may be blank, but WiFi and the web page still work.

## Hardware values to verify (unverified guesses)
| Item | Used | Where |
|---|---|---|
| Display | ST7789 240x240 | `platformio.ini` |
| MOSI / SCLK | GPIO13 / GPIO14 | fixed on ESP8266 |
| DC / RST | GPIO0 / GPIO2 | `platformio.ini` |
| CS | none (tied low) | |
| Backlight | GPIO5, active low | `src/settings.h` |

Swapped red/blue: add `-DTFT_RGB_ORDER=TFT_BGR`. Inverted colors: add `-DTFT_INVERSION_ON`.
Garbled GIF colors: change `LITTLE_ENDIAN_PIXELS` to `BIG_ENDIAN_PIXELS` in `startGif()`.

## Notes
- Weather (Open-Meteo, no key) uses plain HTTP to save RAM. If it says "Needs HTTPS", uncomment `-DWEATHER_HTTPS=1`.
- The original firmware's GIFs are read from `/image` (and `/gif`). The stock WiFi file format (`{"a":ssid,"p":password}`) was inferred
  from the binary, so the automatic import is best-effort.
- The web UI is embedded in `src/webui.h`. `src/fonts.h` and `src/icons.h` are generated files (compressed 4-bit alpha).
- Font: Outfit Bold, copyright The Outfit Project Authors, SIL Open Font License 1.1 (rounded and resized for the display).
