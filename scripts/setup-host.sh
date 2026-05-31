#!/bin/bash
# Bruce MacKinnon KC1FSZ 31-May-2026
#
# A setup script used successfully on Raspberry Pi and other 
# Debian-style machines. Installs the required packages and 
# makes some configuration changes related to the CM108 devices.

# Required packages
apt install -y wget net-tools libcurl4-gnutls-dev pulseaudio-utils

# Setup permissions to access HID interfaces on any CM108s
cat << 'EOF' > /etc/udev/rules.d/99-mydevice.rules
# The C-Media vendor ID
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="0d8c", MODE="0666", TAG+="uaccess"
EOF
udevadm control --reload-rules
udevadm trigger

# Remove the keyboard mapping for the volume down control on the CM108
cat << 'EOF' > /etc/udev/hwdb.d/50-cm108.hwdb
# Ignore HID input events from CM108 GPIOs. NOTE: Exactly one space
# before the "KEYBOARD_KEY ..." line.
evdev:input:b*v0D8Cp0012*
 KEYBOARD_KEY_c00ea=reserved
EOF
systemd-hwdb update
udevadm trigger

# Set profile to off for each C-Media sound card
pactl list cards short | awk '$2 ~ /C-Media/' | awk '{print $1}' | xargs -I {} pactl set-card-profile {} off
