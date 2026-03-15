#!/bin/bash
set -e

TARGET_DIR="$1"

grep -q '/boot' "$TARGET_DIR/etc/fstab" || \
    echo -e "/dev/mmcblk0p1\t/boot\tvfat\tdefaults\t0\t0" >> "$TARGET_DIR/etc/fstab"