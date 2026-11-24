#!/bin/sh
# Installs Hutch on the Pi. Build first, and put WOL_ADDR in .env.
set -eu
cd "$(dirname "$0")/.."

id -u warren >/dev/null 2>&1 ||
  sudo useradd --system --no-create-home --shell /usr/sbin/nologin warren
sudo install -Dm600 .env /etc/warren/hutch.env
sudo install -Dm755 build/hutch/hutch /usr/local/bin/hutch
sudo install -Dm644 deploy/hutch.service /etc/systemd/system/hutch.service
sudo systemctl daemon-reload
sudo systemctl enable hutch
sudo systemctl restart hutch
