#!/usr/bin/env bash
set -euo pipefail

if ! command -v apt-get >/dev/null 2>&1; then
  echo "This script currently supports Ubuntu or Debian based systems with apt-get." >&2
  exit 1
fi

if [[ "${EUID}" -eq 0 ]]; then
  SUDO=""
elif command -v sudo >/dev/null 2>&1; then
  SUDO="sudo"
else
  echo "sudo is required when not running as root." >&2
  exit 1
fi

APT_PACKAGES=(
  build-essential
  cmake
  curl
  docker.io
  git
  gdb
  libgtest-dev
  libopencv-dev
  make
  ninja-build
  pkg-config
  qt6-base-dev
  qt6-multimedia-dev
  qt6-tools-dev
  tar
  unzip
  zip
)

echo "Updating apt package index"
$SUDO apt-get update

echo "Installing client development packages"
$SUDO apt-get install -y "${APT_PACKAGES[@]}"

if getent group docker >/dev/null 2>&1; then
  echo "Adding $USER to docker group"
  $SUDO usermod -aG docker "$USER" || true
fi

echo "Client development environment installation completed"