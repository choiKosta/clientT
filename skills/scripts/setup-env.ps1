param(
    [ValidateSet('server', 'extensions', 'verify', 'all')]
    [string]$Target = 'all',
    [switch]$InstallOpenCvNative
)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

switch ($Target) {
    'server' {
        & "$ScriptDir/install-server-dev-tools.ps1" -InstallOpenCvNative:$InstallOpenCvNative
    }
    'extensions' {
        & "$ScriptDir/install-vscode-extensions.ps1"
    }
    'verify' {
        & "$ScriptDir/verify-env.ps1" -Target server
    }
    'all' {
        & "$ScriptDir/install-server-dev-tools.ps1" -InstallOpenCvNative:$InstallOpenCvNative
        & "$ScriptDir/install-vscode-extensions.ps1"
        & "$ScriptDir/verify-env.ps1" -Target server
    }
}