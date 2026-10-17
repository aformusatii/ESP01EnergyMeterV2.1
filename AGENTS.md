# AGENTS.md

Guidance for coding agents working on this repository.

## Project

ESP-01S (ESP8266, 1 MB flash) firmware that reads a Peacefair PZEM-004T v3.0 energy meter over
the hardware UART and exposes the values over HTTP. PlatformIO project (Arduino framework,
`espressif8266@4.2.1`, core 3.1.2), migrated from an Eclipse Sloeber project
(`sloeber_arduino_eclipse/` is legacy, do not build from it).

- Device: static IP `192.168.100.23`
- Logs: JSON over UDP to `192.168.100.134:5000` / `:5010` (`lib/Logger`)
- HTTP API: `GET /` (info + upload form), `GET /data` (readings), `GET /health`, `POST /update` (OTA)
- MQTT: energy per hour/day/ISO week/month to Mosquitto `192.168.100.3:1883`, topics
  `energy-meter/...`, local time Europe/Chisinau from NTP. Contract: [docs/MQTT.md](docs/MQTT.md)

```
src/main.cpp          application: HTTP routes, PZEM readout
lib/WiFiHelper        WiFi connect, static IP, status LED, reconnect watchdog
lib/ServerHelper      index page / 404 handler
lib/Logger            JSON UDP logger
lib/EnergyPeriods     PZEM counter -> per-period records + RAM queue (no Arduino deps, host-tested)
lib/MqttPublisher     MQTT connection with backoff, drains the queue with QoS 1
include/AppConfig.h   MQTT broker, topic, timezone, NTP servers
include/WiFiConfig.h  local WiFi credentials (git-ignored; copy from WiFiConfig.example.h)
scripts/ota_upload.py PlatformIO custom upload: POSTs firmware.bin to /update
scripts/mqtt_monitor.py  prints the MQTT topics (pip install paho-mqtt)
test/host/            host unit test for lib/EnergyPeriods
platformio.ini        envs: esp01_1m (serial, COM5) and esp01_1m_ota (WiFi)
```

Hardware constraints to keep in mind:
- UART (GPIO1/3) belongs to the PZEM. Do not use `Serial` for debug output; log via `Logger` (UDP).
- The blue LED is GPIO2 (`-DLED_BUILTIN=2` in `platformio.ini`).
- Flash is 1 MB with no filesystem (`eagle.flash.1m.ld`). All free space is reserved for OTA.
- RTC user memory: blocks 0-31 belong to the OTA bootloader; the energy tracker state lives at block 64.

## Build

The PlatformIO CLI is at `~/.platformio/penv/Scripts/pio` and is on PATH.

```sh
pio run -e esp01_1m_ota          # build only (the OTA env refuses placeholder WiFi credentials)
```

Host test for the period logic (needs g++ with POSIX TZ support, e.g. Cygwin at `C:/cygwin64/bin`):

```sh
g++ -std=gnu++17 -Wall -I lib/EnergyPeriods/src test/host/test_energy_periods.cpp     lib/EnergyPeriods/src/EnergyPeriods.cpp -o .pio/test_energy_periods && .pio/test_energy_periods
```

## OTA upload

```sh
pio run -e esp01_1m_ota -t upload
```

This builds the firmware and POSTs `.pio/build/esp01_1m_ota/firmware.bin` to
`http://192.168.100.23/update` (`custom_ota_url` in `platformio.ini`). The updater answers
HTTP 200 even when flashing fails, so the script checks the reply for `Update Success`.

A successful run ends with:

```
OTA update successful, device is rebooting
```

After uploading, confirm that the device came back. It takes a few seconds to reboot and rejoin WiFi:

```sh
for i in 1 2 3 4 5 6; do curl -s -m 5 -o /dev/null -w "%{http_code}\n" http://192.168.100.23/health && break; done
curl -s http://192.168.100.23/data
```

If OTA fails and the device is unreachable, the only recovery is serial flashing on COM5
(`pio run -e esp01_1m -t upload`), which needs physical access. Do not take OTA risks.

## Firmware size limit for OTA (check before every upload)

OTA on the ESP8266 is not A/B. The new image is written into the free flash **above the running
sketch**, then copied over it on reboot. So the running image and the new image must fit in
flash at the same time.

From `eagle.flash.1m.ld` and the core (`ESP8266HTTPUpdateServer`, `Updater.cpp`):

- The sketch region ends at `_FS_start` = `0xFB000` = **1,028,096 bytes**. EEPROM, RF cal
  and WiFi config take up the last 20 KB.
- `/update` calls `Update.begin((ESP.getFreeSketchSpace() - 0x1000) & ~0xFFF)`, which reserves
  one more 4 KB sector.
- Usable OTA region = `1,024,000 - round_up_4k(running sketch size)`.

The "Flash: 1023984 bytes" figure in the build output is the **serial-flash** limit. It is
**not** the OTA limit.

### Rules

1. **Hard limit (this upload):** `round_up_4k(running) + round_up_4k(new) <= 1,024,000`.
   If the new image is larger than the running one, use the size of the running one.
   The running firmware is 352,672 bytes as of 2026-10-18 (MQTT reports), so the new
   `firmware.bin` must be at most **667,648 bytes**.
2. **Safe limit (every future upload):** `firmware.bin <= 512,000 bytes` (125 sectors).
   Below this size, any firmware can be replaced over OTA by any other firmware of the same
   size. Firmware above 512,000 bytes may flash once but then cannot be OTA-updated, so the
   next update would need serial access. **Treat 512,000 bytes as the limit.**

### Check to run after every build, before `-t upload`

```sh
BIN=.pio/build/esp01_1m_ota/firmware.bin
SIZE=$(stat -c %s "$BIN")
LIMIT=512000
echo "firmware.bin = $SIZE bytes, OTA safe limit = $LIMIT bytes, headroom = $((LIMIT - SIZE))"
[ "$SIZE" -le "$LIMIT" ] && echo "OK: OTA-safe" || echo "STOP: too large for safe OTA, do not upload"
```

- If the check fails, **do not upload**. Report the size to the user and reduce the firmware.
  Possible fixes: remove unused features or libraries, move strings into `F()`/PROGMEM, or
  avoid heavy ArduinoJson usage.
- In your report, include the firmware size and the remaining headroom so that size growth is visible.
- After a successful OTA, update the "running firmware" figure in Rule 1 if the size changed noticeably.

## Conventions

- Keep the board settings in `platformio.ini` (160 MHz, DOUT 40 MHz, lwIP2 low memory,
  `eagle.flash.1m.ld`) unless the user asks to change them. In particular, do not add a filesystem
  partition, because it would take space away from OTA.
- Never commit `include/WiFiConfig.h`.
