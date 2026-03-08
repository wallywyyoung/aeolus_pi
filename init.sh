#!/bin/bash

set -e

echo "Setting up environment..."


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
