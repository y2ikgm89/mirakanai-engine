#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [ValidatePattern('^[A-Za-z0-9._-]+$')]
    [string]$DistroName = "Ubuntu-24.04",

    [switch]$CheckOnly,

    [switch]$NonInteractiveSudo
)

$ErrorActionPreference = "Stop"

$packages = @("zlib1g-dev", "clang-format", "pkg-config")

function ConvertTo-BashSingleQuoted {
    param([Parameter(Mandatory = $true)][string]$Value)

    return "'" + $Value.Replace("'", "'`"`"'") + "'"
}

function Invoke-WslBash {
    param([Parameter(Mandatory = $true)][string]$Command)

    $wslCommand = Get-Command wsl.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $wslCommand) {
        Write-Error "wsl.exe was not found. Install or enable Windows Subsystem for Linux before installing WSL packages."
    }

    $arguments = @("--distribution", $DistroName, "--exec", "bash", "-lc", $Command)
    & $wslCommand.Source @arguments
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        Write-Error "WSL command failed with exit code $exitCode for distro '$DistroName'."
    }
}

$quotedPackages = ($packages | ForEach-Object { ConvertTo-BashSingleQuoted $_ }) -join " "

$checkCommand = @'
set -euo pipefail

for package_name in __PACKAGES__; do
  status="$(dpkg-query -W -f='${Status}' "${package_name}" 2>/dev/null || true)"
  if [ "${status}" = "install ok installed" ]; then
    version="$(dpkg-query -W -f='${Version}' "${package_name}")"
    echo "codebase-memory-wsl-deps: ${package_name}=installed:${version}"
  else
    candidate="$(LC_ALL=C apt-cache policy "${package_name}" | awk '/Candidate:/ {print $2}')"
    echo "codebase-memory-wsl-deps: ${package_name}=missing:candidate=${candidate}"
  fi
done
'@.Replace("__PACKAGES__", $quotedPackages)

if ($CheckOnly.IsPresent) {
    Invoke-WslBash -Command $checkCommand
    return
}

$nonInteractiveFlag = if ($NonInteractiveSudo.IsPresent) { "1" } else { "0" }
$installCommand = @'
set -euo pipefail

packages=(__PACKAGES__)

if [ "__NON_INTERACTIVE_SUDO__" = "1" ]; then
  sudo -n true || {
    echo "codebase-memory-wsl-deps: sudo requires a password. Re-run this script from an interactive terminal without -NonInteractiveSudo." >&2
    exit 10
  }
  sudo -n apt-get update
  sudo -n env DEBIAN_FRONTEND=noninteractive apt-get install -y "${packages[@]}"
else
  sudo apt-get update
  sudo env DEBIAN_FRONTEND=noninteractive apt-get install -y "${packages[@]}"
fi

for package_name in "${packages[@]}"; do
  version="$(dpkg-query -W -f='${Version}' "${package_name}")"
  echo "codebase-memory-wsl-deps: ${package_name}=installed:${version}"
done
'@.Replace("__PACKAGES__", $quotedPackages).Replace("__NON_INTERACTIVE_SUDO__", $nonInteractiveFlag)

$packageList = $packages -join ", "
if (-not $PSCmdlet.ShouldProcess("WSL distro '$DistroName'", "Install packages: $packageList")) {
    Write-Information "codebase-memory-wsl-deps: skipped package installation." -InformationAction Continue
    return
}

Invoke-WslBash -Command $installCommand
