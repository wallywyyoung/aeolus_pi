#!/bin/bash

set -e

echo "Setting up environment..."

echo "Generating WiFi settings. NOTE: wpa_supplicant.conf is gitignored and should never be committed!"
WIFI_CONF="buildroot-ext/overlay/etc/wpa_supplicant.conf"

read -p "Enter WiFi SSID: " WIFI_SSID
read -sp "Enter WiFi Password (NOTE: This will be stored in wpa_supplicant.conf hashed.): " WIFI_PASS
echo ""

if [ -n "$WIFI_SSID" ] && [ -n "$WIFI_PASS" ]; then
    if [ -f "$WIFI_CONF" ]; then
        echo "Found existing WiFi config. Deleting for a fresh start..."
        rm "$WIFI_CONF"
    fi
    echo "Generating hashed wpa_supplicant config using Python..."
    mkdir -p "$(dirname "$WIFI_CONF")"
    
    # Calculate the hashed PSK using Python (no extra packages needed)
    HASHED_PSK=$(python3 -c "import hashlib, binascii; dk = hashlib.pbkdf2_hmac('sha1', '$WIFI_PASS'.encode(), '$WIFI_SSID'.encode(), 4096, 32); print(binascii.hexlify(dk).decode())")

    # Write the complete config file
    cat <<EOF > "$WIFI_CONF"
ctrl_interface=/var/run/wpa_supplicant
ctrl_interface_group=0
update_config=0
country=US
ap_scan=1

network={
    ssid="$WIFI_SSID"
    psk=$HASHED_PSK
}
EOF
    
    chmod 600 "$WIFI_CONF"
    echo "WiFi config generated at $WIFI_CONF"
else
    echo "Error: SSID and Password are required. Skipping WiFi config..."
fi

echo "Installing required packages..."
if command -v sudo >/dev/null 2>&1; then
    SUDO="sudo"
else
    SUDO=""
fi

$SUDO apt update
$SUDO apt install -y build-essential binutils sed make which bash \
    patch gzip bzip2 perl tar cpio unzip rsync file bc wget ncurses-dev git curl zip unzip tar

echo "Initalizing submodules..."
git submodule update --init --recursive

# Change $(pwd)/vcpkg to $HOME/vcpkg
VCPKG_ROOT="${VCPKG_ROOT:-$HOME/vcpkg}"

if [ ! -d "$VCPKG_ROOT" ]; then
    echo "vcpkg not found. Cloning into $VCPKG_ROOT..."
    git clone --depth=1 https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT"
    "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics
else
	echo "vcpkg found. Using $VCPKG_ROOT..."
fi

echo "Done!"
