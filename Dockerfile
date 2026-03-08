FROM debian:trixie-slim AS base

# - Buildroot needs: build-essential, bc, cpio, rsync, wget, python3, unzip
# - vcpkg needs: curl, git, zip, tar, g++, pkg-config
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build \
    python3 rsync bc cpio wget \
    git curl zip unzip tar ca-certificates \
    && rm -rf /var/lib/apt/lists/*

ENV VCPKG_ROOT=/opt/vcpkg
RUN git clone --depth=1 https://github.com/microsoft/vcpkg.git $VCPKG_ROOT \
    && $VCPKG_ROOT/bootstrap-vcpkg.sh -disableMetrics

WORKDIR /workspace
