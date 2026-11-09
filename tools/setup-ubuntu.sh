#!/bin/bash
# Installs emulators + toolchain build deps on Ubuntu 24.04 and fetches submodules.
set -e
cd "$(dirname "$0")/.."
git submodule update --init --depth 1
sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
  xvfb xdotool imagemagick ffmpeg mednafen fs-uae mame mame-tools \
  make wget git gcc g++ lhasa libgmp-dev libmpfr-dev libmpc-dev flex bison \
  gettext texinfo libncurses-dev autoconf rsync libreadline-dev
echo "Optional, ~30 min: (cd third_party/amiga/amiga-gcc && make all -j\$(nproc) PREFIX=/opt/amiga)"
