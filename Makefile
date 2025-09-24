#!/usr/bin/make -f

# Project configuration
SKETCH ?= perci.ino
CHIP   ?= esp32
# Default board (override with BOARD=esp32:esp32:esp32s3 for S3)
BOARD  ?= esp32:esp32:esp32

# Optional: serial port for flash target (e.g., COM9 or /dev/ttyUSB0)
# UPLOAD_PORT ?= COM9

# Additional build flags (optional)
# BUILD_EXTRA_FLAGS += -DCORE_DEBUG_LEVEL=0

# Locate makeEspArduino
ifeq ($(MAKEESPARDUINO),)
  ifneq ($(wildcard tools/makeEspArduino/makeEspArduino.mk),)
    MAKEESPARDUINO := tools/makeEspArduino/makeEspArduino.mk
  else ifneq ($(wildcard $(HOME)/makeEspArduino/makeEspArduino.mk),)
    MAKEESPARDUINO := $(HOME)/makeEspArduino/makeEspArduino.mk
  else
    $(error Set MAKEESPARDUINO to path of makeEspArduino.mk or clone it to tools/makeEspArduino/)
  endif
endif

include $(MAKEESPARDUINO)

