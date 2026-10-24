# ESP32-Network-Scanner

A WiFi and Bluetooth scanning application to flash onto ESP32's.

Pick a mode on startup — **WiFi** or **Bluetooth** — scan for nearby signals,
browse the results on the display, and open any one for a detail page
(signal strength, approximate distance, channel/encryption or BLE advertisement
data).

This is a **passive scanner**: it only reads what devices already broadcast
(SSIDs, signal strength, BLE advertisements). It does not connect to,
deauthenticate, jam, or interfere with anything.

---

## Hardware

| | |
|---|---|
| **Board** | LilyGO T-Display-S3 AMOLED **Plus** 1.91" |
| **MCU** | ESP32-S3 (dual-core, WiFi 2.4 GHz + Bluetooth LE) |
| **Display** | 1.91" AMOLED, 536 × 240, SPI (`LILYGO_AMOLED_191_SPI` variant) — **no touch** |
| **Input** | 2 buttons: BOOT (GPIO0) and user button (GPIO21) |
| **Connection** | USB-C |

The "Plus" is the SPI variant (it has a power-management chip, SD slot and RTC).
Navigation is button-driven because this display has no touch.

---

## Software requirements

- **arduino-cli** (command-line build — no IDE needed). Install with Homebrew:
  ```sh
  brew install arduino-cli
  ```
  (Or use the Arduino IDE instead — see below.)
- **ESP32 board core** `esp32:esp32` (tested with 3.3.12). Installed via `make deps`.
- **Libraries** — the project depends on, with these known-good pinned versions:

  | Library | Version | Notes |
  |---|---|---|
  | LilyGo-AMOLED-Series | (vendor) | board driver + `LV_Helper` |
  | lvgl | **8.4.0** | UI — do **not** use 9.x (API breaks) |
  | SensorLib | **0.2.4** | newer breaks the build (`isPressed() override`) |
  | XPowersLib | **0.2.7** | power-management chip |
  | GFX Library for Arduino | 1.6.9 | colors are `RGB565_*` |

  arduino-cli uses the same sketchbook folder as the IDE, so if these are
  already in `~/Documents/Arduino/libraries` they are picked up automatically.
  The versions matter — pin them and don't auto-update.

---

## Build & flash (Makefile)

From the project folder:

```sh
make deps        # one-time: install the esp32 board core
make build       # compile into ./build
make flash       # compile + upload to the board
make monitor     # open serial monitor at 115200 baud
make list        # list attached boards / find your port
make clean       # remove ./build
```

The Makefile auto-detects a `/dev/cu.usbmodem*` port. If detection fails (or you
have several boards), pass it explicitly:

```sh
make PORT=/dev/cu.usbmodem1101 flash monitor
```

### Board settings

The Makefile bakes in the LilyGO AMOLED Plus settings via the FQBN:

```
esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi
```

These mirror the Arduino IDE **Tools** menu: ESP32S3 Dev Module · USB CDC On Boot
**Enabled** · Flash Size **16MB** · Partition **16M Flash (3MB APP/9.9MB FATFS)** ·
PSRAM **OPI**. If a build complains about an unknown option, check the exact
ids/values for your core version with:

```sh
arduino-cli board details -b esp32:esp32:esp32s3
```

### If upload fails to connect

Put the board in bootloader mode: hold **BOOT**, tap **RST**, release BOOT, then
run `make flash` again.

---

## Build with the Arduino IDE (alternative)

Open `ESP32-Network-Scanner.ino`, select **ESP32S3 Dev Module**, set the Tools
menu to match the board settings above, and click Upload.

---

## Status

Early scaffold — board + LVGL boilerplate and the screen/state skeleton are in
place; scan and display logic is in progress. See the `TODO` markers in
`ESP32-Network-Scanner.ino`.

## License

MIT — see [LICENSE](LICENSE).
