#!/usr/bin/env bash
set -euo pipefail

VCPKG_ROOT="${VCPKG_ROOT:-$HOME/.local/share/vcpkg}"

if [[ ! -d "$VCPKG_ROOT/.git" ]]; then
  echo "Cloning vcpkg into $VCPKG_ROOT"
  mkdir -p "$(dirname "$VCPKG_ROOT")"
  git clone https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT"
fi

echo "Bootstrapping vcpkg"
"$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics

echo "vcpkg installation completed"