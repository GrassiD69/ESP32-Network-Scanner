# ESP32-Network-Scanner – build/flash without the Arduino IDE
#
# Wraps arduino-cli. It reuses the same board package and sketchbook
# libraries (~/Documents/Arduino/libraries) the IDE uses, so the pinned
# LilyGo/lvgl/SensorLib/XPowersLib versions already in place are picked up.
#
# Quick start:
#   make deps        # one-time: install the esp32 board core
#   make build       # compile
#   make flash       # compile + upload to the board
#   make monitor     # open the serial monitor
#   make PORT=/dev/cu.usbmodem1101 flash monitor

SKETCH    := ESP32-Network-Scanner.ino
CLI       := arduino-cli
BUILD_DIR := build

# ESP32-S3 dev module + the LilyGO AMOLED Plus settings (mirror of the IDE).
# Verify option ids/values with:  arduino-cli board details -b esp32:esp32:esp32s3
FQBN := esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,UploadSpeed=921600

# Auto-pick the first USB serial port; override with `make PORT=... flash`.
PORT ?= $(shell ls /dev/cu.usbmodem* /dev/cu.wchusbserial* 2>/dev/null | head -n1)
BAUD ?= 115200

.PHONY: all deps build upload flash monitor clean list port

all: build

## Install the ESP32 board core (one-time). Run `make index` first if the core isn't found.
deps:
	$(CLI) core update-index
	$(CLI) core install esp32:esp32

index:
	$(CLI) core update-index

## Compile into ./build
build:
	$(CLI) compile --fqbn "$(FQBN)" --build-path "$(BUILD_DIR)" .

## Upload an already-built sketch to the board
upload: require-port
	$(CLI) upload --fqbn "$(FQBN)" --port "$(PORT)" --input-dir "$(BUILD_DIR)" .

## Compile and upload in one step
flash: require-port
	$(CLI) compile --fqbn "$(FQBN)" --build-path "$(BUILD_DIR)" --upload --port "$(PORT)" .

## Open the serial monitor (Ctrl-C to quit)
monitor: require-port
	$(CLI) monitor --port "$(PORT)" --config baudrate=$(BAUD)

## List attached boards (find your port here)
list:
	$(CLI) board list

## Print the port that will be used
port:
	@echo "PORT = $(PORT)"

clean:
	rm -rf "$(BUILD_DIR)"

# Fail with a clear message if no port was detected/supplied.
require-port:
	@if [ -z "$(PORT)" ]; then \
		echo "No serial port found. Plug in the board and retry, or pass one:"; \
		echo "  make PORT=/dev/cu.usbmodemXXXX $(MAKECMDGOALS)"; \
		echo "Run 'make list' to see attached boards."; \
		exit 1; \
	fi
