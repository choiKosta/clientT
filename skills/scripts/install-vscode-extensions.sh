#!/usr/bin/env bash
set -euo pipefail

find_code_cli() {
  local candidates=(code code-insiders codium)
  local candidate

  for candidate in "${candidates[@]}"; do
    if command -v "$candidate" >/dev/null 2>&1; then
      echo "$candidate"
      return 0
    fi
  done

  return 1
}

if ! CODE_CLI="$(find_code_cli)"; then
  echo "VS Code CLI not found. Install VS Code and ensure 'code' is on PATH." >&2
  exit 1
fi

is_wsl_remote=false
if [[ -n "${WSL_DISTRO_NAME:-}" ]]; then
  is_wsl_remote=true
fi

EXTENSIONS=(
  github.copilot
  github.copilot-chat
  ms-vscode.cpptools
  ms-vscode.cmake-tools
  ms-azuretools.vscode-docker
  ms-vscode-remote.remote-wsl
  vscjava.vscode-java-pack
  vscjava.vscode-gradle
  vmware.vscode-spring-boot
  editorconfig.editorconfig
  eamodio.gitlens
)

for extension in "${EXTENSIONS[@]}"; do
  if [[ "$extension" == "ms-vscode-remote.remote-wsl" && "$is_wsl_remote" == true ]]; then
    echo "Skipping VS Code extension in WSL remote session: $extension"
    continue
  fi

  echo "Installing VS Code extension: $extension"
  "$CODE_CLI" --install-extension "$extension" --force
done

echo "VS Code extensions installation completed"