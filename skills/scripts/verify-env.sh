#!/usr/bin/env bash
set -euo pipefail

TARGET="${1:-client}"
STATUS=0

check_command() {
  local cmd="$1"
  if command -v "$cmd" >/dev/null 2>&1; then
    echo "OK   command: $cmd"
  else
    echo "MISS command: $cmd"
    STATUS=1
  fi
}

check_pkg_config() {
  local module="$1"
  if pkg-config --exists "$module"; then
    echo "OK   pkg-config: $module"
  else
    echo "MISS pkg-config: $module"
    STATUS=1
  fi
}

verify_client() {
  check_command cmake
  check_command g++
  check_command gdb
  check_command git
  check_command ninja
  check_command pkg-config
  check_command docker

  if [[ -n "${VCPKG_ROOT:-}" ]]; then
    if [[ -x "${VCPKG_ROOT}/vcpkg" ]]; then
      echo "OK   vcpkg: ${VCPKG_ROOT}/vcpkg"
    else
      echo "MISS vcpkg: ${VCPKG_ROOT}/vcpkg"
      STATUS=1
    fi
  elif [[ -x "$HOME/.local/share/vcpkg/vcpkg" ]]; then
    echo "OK   vcpkg: $HOME/.local/share/vcpkg/vcpkg"
  else
    echo "MISS vcpkg: set VCPKG_ROOT or install under $HOME/.local/share/vcpkg"
    STATUS=1
  fi

  check_pkg_config Qt6Core
  check_pkg_config opencv4
}

case "$TARGET" in
  client)
    verify_client
    ;;
  *)
    echo "Unsupported verify target: $TARGET" >&2
    exit 1
    ;;
esac

exit "$STATUS"