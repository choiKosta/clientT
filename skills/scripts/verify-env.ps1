param(
    [ValidateSet('server')]
    [string]$Target = 'server'
)

$ErrorActionPreference = 'Stop'
$status = 0

function Test-Tool {
    param([string]$Name)

    if (Get-Command $Name -ErrorAction SilentlyContinue) {
        Write-Host "OK   command: $Name"
    }
    else {
        Write-Host "MISS command: $Name"
        $script:status = 1
    }
}

switch ($Target) {
    'server' {
        Test-Tool java
        Test-Tool gradle
        Test-Tool git
        if ($env:VCPKG_ROOT) {
            if (Test-Path (Join-Path $env:VCPKG_ROOT 'vcpkg.exe')) {
                Write-Host "OK   vcpkg: $(Join-Path $env:VCPKG_ROOT 'vcpkg.exe')"
            }
            else {
                Write-Host "MISS vcpkg: $(Join-Path $env:VCPKG_ROOT 'vcpkg.exe')"
                $script:status = 1
            }
        }
    }
}

exit $status