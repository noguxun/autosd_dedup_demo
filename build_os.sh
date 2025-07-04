#!/usr/bin/bash

# Trace execution & Exit on error
set -xe

arch=$(arch)

automotive-image-builder --verbose \
    build \
    --distro autosd9 \
    --target qemu \
    --mode image \
    --build-dir=_build \
    --export qcow2 \
    simple-developer.aib.yml \
    simple-developer.$arch.img.qcow2
