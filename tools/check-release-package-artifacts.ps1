#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$repoRoot = Get-RepoRoot
$helperPath = Join-Path $PSScriptRoot "release-package-artifacts.ps1"

if (-not (Test-Path -LiteralPath $helperPath)) {
    Write-Error "Missing release package artifact validation helper: $helperPath"
}

. $helperPath

if (-not (Get-Command Assert-ReleasePackageArtifacts -ErrorAction SilentlyContinue)) {
    Write-Error "Missing Assert-ReleasePackageArtifacts in $helperPath"
}

function Add-TestFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root,
        [Parameter(Mandatory = $true)]
        [string]$RelativePath,
        [Parameter(Mandatory = $true)]
        [string]$Value
    )

    $path = Join-Path $Root $RelativePath
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $path) | Out-Null
    Set-Content -LiteralPath $path -Value $Value -Encoding utf8
}

function New-FakeReleaseBuild {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,
        [string[]]$OmitEntries = @()
    )

    $buildDir = Join-Path $repoRoot "out/tmp/release-package-artifacts-check/$Name"
    Reset-RepoTmpDirectory -Path $buildDir

    $packageBaseName = "Mirakanai-0.1.0-Windows-AMD64"
    $cpackConfig = @"
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_CHECKSUM "SHA256")
set(CPACK_PACKAGE_FILE_NAME "$packageBaseName")
"@
    Set-Content -LiteralPath (Join-Path $buildDir "CPackConfig.cmake") -Value $cpackConfig -Encoding utf8

    $stageDir = Join-Path $buildDir "stage"
    $packageRoot = Join-Path $stageDir $packageBaseName
    $requiredEntries = @(
        "lib/cmake/Mirakanai/MirakanaiConfig.cmake",
        "lib/cmake/Mirakanai/MirakanaiConfigVersion.cmake",
        "share/Mirakanai/manifest.json",
        "share/Mirakanai/schemas/engine-agent.schema.json",
        "share/Mirakanai/schemas/engine-agent/ai-operable-production-loop.schema.json",
        "share/Mirakanai/schemas/game-agent.schema.json",
        "share/Mirakanai/tools/validate.ps1",
        "share/Mirakanai/tools/agent-context.ps1",
        "share/Mirakanai/examples/installed_consumer/CMakeLists.txt",
        "share/doc/Mirakanai/README.md",
        "share/Mirakanai/THIRD_PARTY_NOTICES.md",
        "share/Mirakanai/LICENSES/LicenseRef-Proprietary.txt"
    )

    foreach ($entry in $requiredEntries) {
        if ($OmitEntries -contains $entry) {
            continue
        }
        Add-TestFile -Root $packageRoot -RelativePath $entry -Value "fake payload for $entry"
    }

    $zipPath = Join-Path $buildDir "$packageBaseName.zip"
    Compress-Archive -Path (Join-Path $stageDir $packageBaseName) -DestinationPath $zipPath -CompressionLevel Fastest

    $hash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
    Set-Content -LiteralPath "$zipPath.sha256" -Value "$hash  $packageBaseName.zip" -Encoding ascii

    return @{
        BuildDir = $buildDir
        ZipPath = $zipPath
        ShaPath = "$zipPath.sha256"
    }
}

function Assert-FailsWith {
    param(
        [Parameter(Mandatory = $true)]
        [scriptblock]$ScriptBlock,
        [Parameter(Mandatory = $true)]
        [string]$Pattern
    )

    try {
        & $ScriptBlock
    }
    catch {
        $message = $_.Exception.Message
        if ($message -notmatch $Pattern) {
            throw "Expected failure matching '$Pattern' but got: $message"
        }
        return
    }

    throw "Expected failure matching '$Pattern' but command succeeded."
}

$valid = New-FakeReleaseBuild -Name "valid"
Assert-ReleasePackageArtifacts -BuildDir $valid.BuildDir

$missingChecksum = New-FakeReleaseBuild -Name "missing-checksum"
Remove-Item -LiteralPath $missingChecksum.ShaPath -Force
Assert-FailsWith -Pattern "checksum" -ScriptBlock {
    Assert-ReleasePackageArtifacts -BuildDir $missingChecksum.BuildDir
}

$checksumMismatch = New-FakeReleaseBuild -Name "checksum-mismatch"
Set-Content -LiteralPath $checksumMismatch.ShaPath -Value ("0" * 64 + "  " + (Split-Path -Leaf $checksumMismatch.ZipPath)) -Encoding ascii
Assert-FailsWith -Pattern "SHA-256" -ScriptBlock {
    Assert-ReleasePackageArtifacts -BuildDir $checksumMismatch.BuildDir
}

$missingManifest = New-FakeReleaseBuild -Name "missing-manifest" -OmitEntries @("share/Mirakanai/manifest.json")
Assert-FailsWith -Pattern "manifest\.json" -ScriptBlock {
    Assert-ReleasePackageArtifacts -BuildDir $missingManifest.BuildDir
}

Write-Host "release-package-artifacts-check: ok"
