#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET="${1:-all}"

usage() {
  cat <<'EOF'
Usage: bash skills/scripts/setup-env.sh [client|vcpkg|extensions|verify|all]
EOF
}

run_client() {
  bash "$SCRIPT_DIR/install-client-dev-tools.sh"
}

run_vcpkg() {
  bash "$SCRIPT_DIR/install-vcpkg.sh"
}

run_extensions() {
  bash "$SCRIPT_DIR/install-vscode-extensions.sh"
}

run_verify() {
  bash "$SCRIPT_DIR/verify-env.sh" client
}

case "$TARGET" in
  client)
    run_client
    ;;
  vcpkg)
    run_vcpkg
    ;;
  extensions)
    run_extensions
    ;;
  verify)
    run_verify
    ;;
  all)
    run_client
    run_vcpkg
    run_extensions
    run_verify
    ;;
  *)
    usage
    exit 1
    ;;
esac