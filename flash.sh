#!/bin/bash

# Source ESP-IDF environment
. /Users/hvo/esp/esp-idf/export.sh

# Flash to device
idf.py -p /dev/cu.usbmodem* flash monitor
