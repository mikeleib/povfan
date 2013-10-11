
ARDUINO_DIR = /usr/share/arduino
ARDMK_DIR     = Arduino-Makefile
AVR_TOOLS_DIR = /usr

TARGET = povfan

BOARD_TAG = diecimila
ARDUINO_PORT = /dev/ttyUSB2

include Arduino-Makefile/arduino-mk/Arduino.mk
