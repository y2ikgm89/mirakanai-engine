#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

function Assert-InstalledSdkNonEmptyFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Write-Error "$Description was not found: $Path"
    }

    $item = Get-Item -LiteralPath $Path
    if (-not ($item -is [System.IO.FileInfo])) {
        Write-Error "$Description path is not a file: $Path"
    }
    if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        Write-Error "$Description must not be a reparse point: $Path"
    }

    $text = Get-Content -LiteralPath $Path -Raw
    if ([string]::IsNullOrWhiteSpace($text)) {
        Write-Error "$Description is empty: $Path"
    }

    return $text
}

function Assert-InstalledSdkJsonFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $text = Assert-InstalledSdkNonEmptyFile -Path $Path -Description $Description
    try {
        $text | ConvertFrom-Json | Out-Null
    } catch {
        Write-Error "$Description is not valid JSON: $Path"
    }
}

function Assert-InstalledSdkSampleManifest {
    param([Parameter(Mandatory = $true)][string]$SamplesRoot)

    if (-not (Test-Path -LiteralPath $SamplesRoot -PathType Container)) {
        Write-Error "Installed Mirakanai samples directory was not found: $SamplesRoot"
    }

    $sampleManifests = @(Get-ChildItem -LiteralPath $SamplesRoot -Recurse -Filter "game.agent.json" -File)
    if ($sampleManifests.Count -eq 0) {
        Write-Error "Installed Mirakanai samples did not include any game.agent.json files: $SamplesRoot"
    }

    foreach ($manifest in $sampleManifests) {
        Assert-InstalledSdkJsonFile -Path $manifest.FullName -Description "Installed Mirakanai sample manifest"
    }
}

function Assert-InstalledSdkMetadata {
    param([Parameter(Mandatory = $true)][string]$InstallPrefix)

    $installPrefixPath = [System.IO.Path]::GetFullPath($InstallPrefix)
    $cmakePackageDir = Join-Path $installPrefixPath "lib/cmake/Mirakanai"
    $shareDir = Join-Path $installPrefixPath "share/Mirakanai"
    $docDir = Join-Path $installPrefixPath "share/doc/Mirakanai"

    Assert-InstalledSdkNonEmptyFile `
        -Path (Join-Path $cmakePackageDir "MirakanaiConfig.cmake") `
        -Description "Installed Mirakanai CMake package config" | Out-Null
    Assert-InstalledSdkNonEmptyFile `
        -Path (Join-Path $cmakePackageDir "MirakanaiConfigVersion.cmake") `
        -Description "Installed Mirakanai CMake package version config" | Out-Null

    Assert-InstalledSdkJsonFile `
        -Path (Join-Path $shareDir "manifest.json") `
        -Description "Installed Mirakanai AI manifest"
    Assert-InstalledSdkJsonFile `
        -Path (Join-Path $shareDir "schemas/engine-agent.schema.json") `
        -Description "Installed Mirakanai engine agent schema"
    Assert-InstalledSdkJsonFile `
        -Path (Join-Path $shareDir "schemas/engine-agent/ai-operable-production-loop.schema.json") `
        -Description "Installed Mirakanai engine agent production-loop schema fragment"
    Assert-InstalledSdkJsonFile `
        -Path (Join-Path $shareDir "schemas/game-agent.schema.json") `
        -Description "Installed Mirakanai game agent schema"

    Assert-InstalledSdkNonEmptyFile `
        -Path (Join-Path $shareDir "tools/validate.ps1") `
        -Description "Installed Mirakanai validation tool" | Out-Null
    Assert-InstalledSdkNonEmptyFile `
        -Path (Join-Path $shareDir "tools/agent-context.ps1") `
        -Description "Installed Mirakanai agent context tool" | Out-Null

    Assert-InstalledSdkNonEmptyFile `
        -Path (Join-Path $shareDir "examples/installed_consumer/CMakeLists.txt") `
        -Description "Installed Mirakanai installed consumer CMakeLists" | Out-Null
    Assert-InstalledSdkNonEmptyFile `
        -Path (Join-Path $shareDir "examples/installed_consumer/main.cpp") `
        -Description "Installed Mirakanai installed consumer source" | Out-Null
    Assert-InstalledSdkSampleManifest -SamplesRoot (Join-Path $shareDir "samples")

    Assert-InstalledSdkNonEmptyFile `
        -Path (Join-Path $docDir "README.md") `
        -Description "Installed Mirakanai documentation README" | Out-Null
    Assert-InstalledSdkNonEmptyFile `
        -Path (Join-Path $shareDir "THIRD_PARTY_NOTICES.md") `
        -Description "Installed Mirakanai third-party notices" | Out-Null
    Assert-InstalledSdkNonEmptyFile `
        -Path (Join-Path $shareDir "LICENSES/LicenseRef-Proprietary.txt") `
        -Description "Installed Mirakanai proprietary license" | Out-Null
}
