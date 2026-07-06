param(
    [switch]$InstallOpenCvNative
)

$ErrorActionPreference = 'Stop'

function Assert-Command {
    param([string]$Name)

    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "$Name is required but was not found on PATH."
    }
}

function Install-WingetPackage {
    param(
        [Parameter(Mandatory = $true)][string]$Id,
        [string]$Override
    )

    $arguments = @('install', '--id', $Id, '--exact', '--accept-package-agreements', '--accept-source-agreements')
    if ($Override) {
        $arguments += @('--override', $Override)
    }

    Write-Host "Installing package $Id"
    & winget @arguments
}

function Install-VcpkgOpenCv {
    $vcpkgRoot = if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { Join-Path $env:USERPROFILE 'vcpkg' }

    if (-not (Test-Path $vcpkgRoot)) {
        git clone https://github.com/microsoft/vcpkg.git $vcpkgRoot
    }

    & (Join-Path $vcpkgRoot 'bootstrap-vcpkg.bat') -disableMetrics
    & (Join-Path $vcpkgRoot 'vcpkg.exe') install opencv4:x64-windows
}

Assert-Command winget

Install-WingetPackage -Id 'Git.Git'
Install-WingetPackage -Id 'Microsoft.OpenJDK.21'
Install-WingetPackage -Id 'Gradle.Gradle'
Install-WingetPackage -Id 'Microsoft.VisualStudioCode'
Install-WingetPackage -Id 'JetBrains.IntelliJIDEA.Community'
Install-WingetPackage -Id 'Docker.DockerDesktop'

if ($InstallOpenCvNative) {
    Install-WingetPackage -Id 'Kitware.CMake'
    Install-WingetPackage -Id 'Microsoft.VisualStudio.2022.BuildTools'
    Install-VcpkgOpenCv
}

Write-Host 'Server development environment installation completed'