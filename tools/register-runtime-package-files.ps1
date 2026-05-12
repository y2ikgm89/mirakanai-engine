#requires -Version 7.0
#requires -PSEdition Core

param(
    [Parameter(Mandatory = $true)][string]$GameManifest,
    [Parameter(Mandatory = $true)][string[]]$RuntimePackageFile,
    [string]$RepositoryRoot = "",
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

function Write-RegistrationSummary {
    param(
        [Parameter(Mandatory = $true)][string]$Status,
        [Parameter(Mandatory = $true)][string]$Path
    )

    Write-Output "runtime-package-file $Status $Path"
}

function Get-NormalizedGameManifestPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $normalized = ConvertTo-DesktopRuntimeMetadataRelativePath `
        -Path $Path `
        -Description "GameManifest"
    if ($normalized -notmatch "^games/[a-z][a-z0-9_]*/game\.agent\.json$") {
        Write-Error "GameManifest must point at games/<game_name>/game.agent.json: $Path"
    }

    return $normalized
}

function Add-RuntimePackageFilesProperty {
    param([Parameter(Mandatory = $true)]$Manifest)

    if (-not $Manifest.PSObject.Properties.Name.Contains("runtimePackageFiles")) {
        $Manifest | Add-Member -MemberType NoteProperty -Name "runtimePackageFiles" -Value @()
    }
    if ($null -eq $Manifest.runtimePackageFiles -or $Manifest.runtimePackageFiles -isnot [System.Array]) {
        Write-Error "game manifest runtimePackageFiles must be an array."
    }
}

if ([string]::IsNullOrWhiteSpace($RepositoryRoot)) {
    $RepositoryRoot = Get-RepoRoot
}

$root = [System.IO.Path]::GetFullPath($RepositoryRoot)
$manifestRelativePath = Get-NormalizedGameManifestPath -Path $GameManifest
$gameDirectory = $manifestRelativePath.Substring(0, $manifestRelativePath.LastIndexOf("/"))
$manifestPath = Join-Path $root $manifestRelativePath.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
Assert-DesktopRuntimePathUnderRoot -Path $manifestPath -Root $root -Description "GameManifest"

if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    Write-Error "GameManifest does not exist: $manifestRelativePath"
}

$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
Add-RuntimePackageFilesProperty -Manifest $manifest

$seenPackageFiles = @{}
$existingPackageFiles = @()
foreach ($entry in @($manifest.runtimePackageFiles)) {
    if ($entry -isnot [string]) {
        Write-Error "$manifestRelativePath runtimePackageFiles entries must be strings."
    }
    $packageFile = ConvertTo-DesktopRuntimeMetadataRelativePath `
        -Path $entry `
        -Description "$manifestRelativePath runtimePackageFiles entry"
    if ($packageFile.StartsWith("games/", [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "$manifestRelativePath runtimePackageFiles entries must be game-relative, not repository-relative: $packageFile"
    }
    if ($packageFile -match ";") {
        Write-Error "$manifestRelativePath runtimePackageFiles entries must not contain CMake list separators: $packageFile"
    }
    Assert-DesktopRuntimeSnakeCaseRelativePath `
        -Path $packageFile `
        -Description "$manifestRelativePath runtimePackageFiles entry"

    $key = ConvertTo-DesktopRuntimePathKey -Path $packageFile
    if ($seenPackageFiles.ContainsKey($key)) {
        Write-Error "$manifestRelativePath runtimePackageFiles entry is duplicated after normalization: $packageFile"
    }
    $seenPackageFiles[$key] = $true
    $existingPackageFiles += $packageFile
}

$updatedPackageFiles = @($existingPackageFiles)
$requestedPackageFiles = @{}
foreach ($entry in $RuntimePackageFile) {
    $packageFile = ConvertTo-DesktopRuntimeMetadataRelativePath `
        -Path $entry `
        -Description "RuntimePackageFile"
    if ($packageFile.StartsWith("games/", [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "RuntimePackageFile entries must be game-relative, not repository-relative: $packageFile"
    }
    if ($packageFile -match ";") {
        Write-Error "RuntimePackageFile entries must not contain CMake list separators: $packageFile"
    }
    Assert-DesktopRuntimeSnakeCaseRelativePath `
        -Path $packageFile `
        -Description "RuntimePackageFile"

    $sourceRelativePath = "$gameDirectory/$packageFile"
    $sourcePath = Join-Path $root $sourceRelativePath.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
    Assert-DesktopRuntimePathUnderRoot -Path $sourcePath -Root (Join-Path $root $gameDirectory) -Description "RuntimePackageFile"
    if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
        Write-Error "RuntimePackageFile entry does not exist: $sourceRelativePath"
    }

    $key = ConvertTo-DesktopRuntimePathKey -Path $packageFile
    if ($requestedPackageFiles.ContainsKey($key)) {
        Write-Error "RuntimePackageFile entry is duplicated after normalization: $packageFile"
    }
    $requestedPackageFiles[$key] = $true

    if ($seenPackageFiles.ContainsKey($key)) {
        Write-RegistrationSummary -Status "already-present" -Path $packageFile
        continue
    }

    $seenPackageFiles[$key] = $true
    $updatedPackageFiles += $packageFile
    Write-RegistrationSummary -Status "add" -Path $packageFile
}

if ($DryRun.IsPresent) {
    Write-Output "runtime-package-file dry-run no-write"
    return
}

$manifest.runtimePackageFiles = @($updatedPackageFiles)
$json = $manifest | ConvertTo-Json -Depth 20
Set-Content -LiteralPath $manifestPath -Value ($json + "`n") -NoNewline
