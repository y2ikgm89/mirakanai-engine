#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [Parameter(Position = 0, ValueFromRemainingArguments = $true)]
    [string[]]$CommandLine = @(),

    [ValidateSet("amd64", "x86", "arm64", "arm")]
    [string]$Arch = "amd64",

    [ValidateSet("amd64", "x86", "arm64", "arm")]
    [string]$HostArch = "amd64"
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

if (-not [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
    Write-Error "MSVC Developer PowerShell is available only on Windows hosts."
}

$devShell = Get-VisualStudioDevShellScript
if (-not $devShell) {
    Write-Error "Visual Studio Developer PowerShell launcher was not found. Install official Visual Studio Build Tools with the C++ build tools workload."
}

& $devShell -Arch $Arch -HostArch $HostArch -SkipAutomaticLocation | Out-Null

if ($CommandLine.Count -eq 0) {
    Write-Information "msvc-dev-shell: initialized from $devShell" -InformationAction Continue
    foreach ($name in @("cl", "nmake", "MSBuild", "cmake")) {
        $resolved = Get-Command $name -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($resolved) {
            Write-Host "msvc-dev-shell: $name=$($resolved.Source)"
        } else {
            Write-Host "msvc-dev-shell: $name=missing"
        }
    }
    Write-Host "msvc-dev-shell: ok"
    exit 0
}

$filePath = $CommandLine[0]
$arguments = @($CommandLine | Select-Object -Skip 1)
Invoke-CheckedCommand $filePath @arguments
