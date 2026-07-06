$ErrorActionPreference = 'Stop'

function Get-CodeCli {
    foreach ($candidate in @('code', 'code-insiders', 'codium')) {
        if (Get-Command $candidate -ErrorAction SilentlyContinue) {
            return $candidate
        }
    }

    throw 'VS Code CLI not found. Install VS Code and add the code command to PATH.'
}

$codeCli = Get-CodeCli
$extensions = @(
    'github.copilot',
    'github.copilot-chat',
    'ms-vscode.cpptools',
    'ms-vscode.cmake-tools',
    'ms-azuretools.vscode-docker',
    'ms-vscode-remote.remote-wsl',
    'vscjava.vscode-java-pack',
    'vscjava.vscode-gradle',
    'vmware.vscode-spring-boot',
    'editorconfig.editorconfig',
    'eamodio.gitlens'
)

foreach ($extension in $extensions) {
    Write-Host "Installing VS Code extension: $extension"
    & $codeCli --install-extension $extension --force
}

Write-Host 'VS Code extensions installation completed'