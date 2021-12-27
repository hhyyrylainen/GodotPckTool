#!/usr/bin/env sh
# Build script to be ran in a container
echo "Running in container build script"

set -e

mkdir /build

cd /build

cmake /src -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/out
make install -j $(nproc --all)
