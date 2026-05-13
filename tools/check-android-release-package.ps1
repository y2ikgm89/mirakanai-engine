#requires -Version 7.0
#requires -PSEdition Core

param(
    [string]$Game = "sample_headless",
    [switch]$UseLocalValidationKey
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

function New-SecretText {
    $bytes = [byte[]]::new(36)
    [System.Security.Cryptography.RandomNumberGenerator]::Fill($bytes)
    return [Convert]::ToBase64String($bytes).TrimEnd("=")
}

function Invoke-SecretCommand {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.WorkingDirectory = (Get-Location).Path
    $startInfo.UseShellExecute = $false
    foreach ($argument in $Arguments) {
        $startInfo.ArgumentList.Add($argument) | Out-Null
    }
    $startInfo.Environment.Clear()
    foreach ($entry in Get-NormalizedProcessEnvironment) {
        $startInfo.Environment[$entry.Key] = $entry.Value
    }

    $process = [System.Diagnostics.Process]::Start($startInfo)
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) {
        Write-Error "$Label failed with exit code $($process.ExitCode)."
    }
}

function Set-AndroidSigningEnvironmentFromAnyScope {
    foreach ($name in @(
        "MK_ANDROID_KEYSTORE",
        "MK_ANDROID_KEYSTORE_PASSWORD",
        "MK_ANDROID_KEY_ALIAS",
        "MK_ANDROID_KEY_PASSWORD"
    )) {
        Set-ProcessEnvironmentFromAnyScope $name
    }
}

function Test-AndroidSigningEnvironment {
    foreach ($name in @(
        "MK_ANDROID_KEYSTORE",
        "MK_ANDROID_KEYSTORE_PASSWORD",
        "MK_ANDROID_KEY_ALIAS",
        "MK_ANDROID_KEY_PASSWORD"
    )) {
        if ([string]::IsNullOrWhiteSpace([Environment]::GetEnvironmentVariable($name, "Process"))) {
            return $false
        }
    }

    return (Test-Path -LiteralPath $env:MK_ANDROID_KEYSTORE -PathType Leaf)
}

function New-LocalValidationUploadKey {
    $keytool = Find-JavaToolCommand "keytool"
    if (-not $keytool) {
        Write-Error "keytool is required to create a local Android upload key. Install JDK 17 and set JAVA_HOME."
    }

    $localAppData = Get-LocalApplicationDataRoot
    if ([string]::IsNullOrWhiteSpace($localAppData)) {
        Write-Error "Unable to resolve a local application data directory for Android upload key generation."
    }

    $keyRoot = Join-Path $localAppData "Mirakanai\android\keys"
    New-Item -ItemType Directory -Force -Path $keyRoot | Out-Null

    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $keystore = Join-Path $keyRoot "mirakanai-local-upload-$stamp.p12"
    $storePassword = New-SecretText
    $keyPassword = $storePassword
    $alias = "mirakanai-upload"

    Invoke-SecretCommand $keytool @(
        "-genkeypair",
        "-keystore", $keystore,
        "-storetype", "PKCS12",
        "-alias", $alias,
        "-keyalg", "RSA",
        "-keysize", "4096",
        "-validity", "10000",
        "-dname", "CN=Mirakanai Local Upload,O=Mirakanai,C=US",
        "-storepass", $storePassword,
        "-keypass", $keyPassword,
        "-noprompt"
    ) "Android local upload key generation"

    [Environment]::SetEnvironmentVariable("MK_ANDROID_KEYSTORE", $keystore, "Process")
    [Environment]::SetEnvironmentVariable("MK_ANDROID_KEYSTORE_PASSWORD", $storePassword, "Process")
    [Environment]::SetEnvironmentVariable("MK_ANDROID_KEY_ALIAS", $alias, "Process")
    [Environment]::SetEnvironmentVariable("MK_ANDROID_KEY_PASSWORD", $keyPassword, "Process")

    return $keystore
}

Set-ProcessEnvironmentFromAnyScope "ANDROID_HOME"
Set-ProcessEnvironmentFromAnyScope "ANDROID_SDK_ROOT"
Set-ProcessEnvironmentFromAnyScope "JAVA_HOME"
Set-AndroidSigningEnvironmentFromAnyScope

if (-not (Test-AndroidSigningEnvironment)) {
    if (-not $UseLocalValidationKey) {
        Write-Error "Android Release signing requires MK_ANDROID_KEYSTORE, MK_ANDROID_KEYSTORE_PASSWORD, MK_ANDROID_KEY_ALIAS, and MK_ANDROID_KEY_PASSWORD. Use -UseLocalValidationKey for a local non-repository validation key."
    }

    $createdKey = New-LocalValidationUploadKey
    Write-Host "android-release-package-check: local-upload-key=$createdKey"
}

$keystorePath = [System.IO.Path]::GetFullPath($env:MK_ANDROID_KEYSTORE)
$repoRootPath = [System.IO.Path]::GetFullPath($root).TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if ($keystorePath.StartsWith($repoRootPath, [System.StringComparison]::OrdinalIgnoreCase)) {
    Write-Error "Android Release signing key must not be stored inside the repository: $keystorePath"
}

& (Join-Path $PSScriptRoot "build-mobile-android.ps1") -Game $Game -Configuration Release

$apk = Join-Path $root "platform\android\app\build\outputs\apk\release\app-release.apk"
if (-not (Test-Path -LiteralPath $apk -PathType Leaf)) {
    Write-Error "Android Release APK was not produced: $apk"
}

$apksigner = Find-AndroidBuildToolCommand "apksigner"
if (-not $apksigner) {
    Write-Error "apksigner is required to verify Android Release signing. Install Android SDK Build Tools."
}

Invoke-CheckedCommand $apksigner "verify" "--verbose" "--print-certs" "--min-sdk-version" "26" $apk

$keytool = Find-JavaToolCommand "keytool"
if ($keytool) {
    $certificateDir = Join-Path $root "out\mobile\android\release"
    New-Item -ItemType Directory -Force -Path $certificateDir | Out-Null
    $certificate = Join-Path $certificateDir "upload_certificate.pem"
    Invoke-SecretCommand $keytool @(
        "-exportcert",
        "-rfc",
        "-keystore", $env:MK_ANDROID_KEYSTORE,
        "-storepass", $env:MK_ANDROID_KEYSTORE_PASSWORD,
        "-alias", $env:MK_ANDROID_KEY_ALIAS,
        "-file", $certificate
    ) "Android upload certificate export"
    Write-Host "android-release-package-check: upload-certificate=$certificate"
}

Write-Host "android-release-package-check: apk=$apk"
Write-Host "android-release-package-check: ok"

