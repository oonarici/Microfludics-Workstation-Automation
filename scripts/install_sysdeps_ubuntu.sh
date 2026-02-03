#!/usr/bin/env bash
set -e
sudo apt update
sudo apt install -y \
  libx11-dev libx11-xcb-dev libfontenc-dev libice-dev libsm-dev libxau-dev libxaw7-dev \
  libxkbfile-dev libxmu-dev libxmuu-dev libxpm-dev libxres-dev libxss-dev libxv-dev \
  libxxf86vm-dev libxcb-glx0-dev libxcb-randr0-dev libxcb-shape0-dev libxcb-sync-dev \
  libxcb-xfixes0-dev libxcb-dri2-0-dev libxcb-dri3-dev libxcb-present-dev \
  libxcb-composite0-dev libxcb-ewmh-dev libxcb-res0-dev libxcb-cursor-dev \
  ca-certificates
