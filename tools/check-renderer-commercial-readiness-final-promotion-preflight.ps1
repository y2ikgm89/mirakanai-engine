#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function Assert-LinePresent {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$ExpectedLine,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Lines.Contains($ExpectedLine)) {
        Write-Error "$Context missing expected line: $ExpectedLine"
    }
}

function Assert-TextPresent {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Needle,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Text.Contains($Needle)) {
        Write-Error "$Context missing expected text: $Needle"
    }
}

function Assert-PathUnderDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $pathFull = [System.IO.Path]::GetFullPath($Path)
    $directoryFull = [System.IO.Path]::GetFullPath($Directory).TrimEnd(
        [char[]]@([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar))
    $directoryPrefix = $directoryFull + [System.IO.Path]::DirectorySeparatorChar
    if ($pathFull -ne $directoryFull -and
        -not $pathFull.StartsWith($directoryPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "$Description escaped expected directory '$directoryFull': $pathFull"
    }
}

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Join-Path $root ($RelativePath -replace "/", [System.IO.Path]::DirectorySeparatorChar)
}

function ConvertFrom-KeyValueLines {
    param([string[]]$Lines = @())

    $values = @{}
    foreach ($line in @($Lines)) {
        $text = [string]$line
        $separator = $text.IndexOf("=")
        if ($separator -le 0) {
            continue
        }
        $values[$text.Substring(0, $separator)] = $text.Substring($separator + 1)
    }
    return $values
}

$preflightScript = Join-Path $root "tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1"
if (-not (Test-Path -LiteralPath $preflightScript -PathType Leaf)) {
    Write-Error "tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1 must exist for Task 10F final promotion preflight."
}

$approvedArtifactRoot = ConvertTo-LocalPath "artifacts/renderer/commercial-readiness-evidence"
$missingRootRelative = "artifacts/renderer/commercial-readiness-evidence/final-preflight-missing-$PID"
$missingRoot = ConvertTo-LocalPath $missingRootRelative
Assert-PathUnderDirectory -Path $missingRoot -Directory $approvedArtifactRoot `
    -Description "Renderer commercial readiness final preflight missing root"

if (Test-Path -LiteralPath $missingRoot) {
    Assert-PathUnderDirectory -Path $missingRoot -Directory $approvedArtifactRoot `
        -Description "Renderer commercial readiness stale final preflight missing root"
    Remove-Item -LiteralPath $missingRoot -Recurse -Force
}

$missingLines = @(& $preflightScript -ArtifactRootRelative $missingRootRelative)
$missingValues = ConvertFrom-KeyValueLines -Lines $missingLines
$missingText = [string]::Join("`n", $missingLines)
Assert-LinePresent -Lines $missingLines `
    -ExpectedLine "validation_recipe=renderer-commercial-readiness-final-promotion-preflight" `
    -Context "missing root preflight"
Assert-LinePresent -Lines $missingLines `
    -ExpectedLine "renderer_commercial_readiness_final_preflight_status=blocked" `
    -Context "missing root preflight"
Assert-LinePresent -Lines $missingLines `
    -ExpectedLine "renderer_commercial_readiness_final_preflight_required_files=12" `
    -Context "missing root preflight"
Assert-LinePresent -Lines $missingLines `
    -ExpectedLine "renderer_commercial_readiness_final_preflight_present_files=0" `
    -Context "missing root preflight"
Assert-LinePresent -Lines $missingLines `
    -ExpectedLine "renderer_commercial_readiness_final_preflight_missing_files=12" `
    -Context "missing root preflight"
Assert-LinePresent -Lines $missingLines `
    -ExpectedLine "renderer_commercial_readiness=0" `
    -Context "missing root preflight"
Assert-TextPresent -Text $missingText -Needle "artifact_root_missing" -Context "missing root preflight"
Assert-TextPresent -Text $missingText -Needle "metal_memory_profiling_host_evidence_required" `
    -Context "missing root preflight"
Assert-TextPresent -Text $missingText -Needle "clean_room_legal_artifact_required" `
    -Context "missing root preflight"

$requireReadyOutput = @()
$requireReadyFailed = $false
try {
    $requireReadyOutput = @(& $preflightScript -ArtifactRootRelative $missingRootRelative -RequireReady 2>&1)
} catch {
    $requireReadyFailed = $true
    $requireReadyOutput = @([string]$_.Exception.Message)
}
if (-not $requireReadyFailed) {
    Write-Error "final promotion preflight must fail when -RequireReady is used without retained artifacts."
}
Assert-TextPresent -Text ([string]::Join("`n", $requireReadyOutput)) `
    -Needle "require_ready_without_complete_retained_artifacts" `
    -Context "RequireReady missing root preflight"

$unsafeOutput = @()
$unsafeFailed = $false
try {
    $unsafeOutput = @(& $preflightScript -ArtifactRootRelative "../unsafe" 2>&1)
} catch {
    $unsafeFailed = $true
    $unsafeOutput = @([string]$_.Exception.Message)
}
if (-not $unsafeFailed) {
    Write-Error "final promotion preflight must reject unsafe artifact roots."
}
Assert-TextPresent -Text ([string]::Join("`n", $unsafeOutput)) `
    -Needle "unsafe_artifact_root" `
    -Context "unsafe root preflight"

$fixtureRootRelative = "artifacts/renderer/commercial-readiness-evidence/final-preflight-fixtures-$PID"
$fixtureRoot = ConvertTo-LocalPath $fixtureRootRelative
$sourceFixtureRoot = ConvertTo-LocalPath "tests/fixtures/renderer/commercial-readiness-evidence/ready"
Assert-PathUnderDirectory -Path $fixtureRoot -Directory $approvedArtifactRoot `
    -Description "Renderer commercial readiness final preflight fixture root"

try {
    if (Test-Path -LiteralPath $fixtureRoot) {
        Assert-PathUnderDirectory -Path $fixtureRoot -Directory $approvedArtifactRoot `
            -Description "Renderer commercial readiness stale final preflight fixture root"
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    }
    $null = New-Item -ItemType Directory -Path $fixtureRoot -Force
    Copy-Item -Path (Join-Path $sourceFixtureRoot "*") -Destination $fixtureRoot -Recurse -Force

    $fixtureLines = @(& $preflightScript -ArtifactRootRelative $fixtureRootRelative)
    $fixtureValues = ConvertFrom-KeyValueLines -Lines $fixtureLines
    $fixtureText = [string]::Join("`n", $fixtureLines)

    if ([string]$fixtureValues["renderer_commercial_readiness_final_preflight_ready"] -ne "0") {
        Write-Error "fixture final preflight unexpectedly became ready."
    }
    if ([string]$fixtureValues["renderer_commercial_readiness_final_preflight_present_files"] -ne "10") {
        Write-Error "fixture final preflight should see 10 final-path fixture files."
    }
    if ([string]$fixtureValues["renderer_commercial_readiness_final_preflight_missing_files"] -ne "2") {
        Write-Error "fixture final preflight should miss full Metal host and clean-room legal artifacts."
    }
    if ([string]$fixtureValues["renderer_commercial_readiness_final_preflight_fixture_artifacts_rejected"] -ne "9") {
        Write-Error "fixture final preflight should reject 9 fixture-only commercial/package/quality artifacts."
    }
    Assert-TextPresent -Text $fixtureText -Needle "metal_memory_profiling_host_evidence_required" `
        -Context "fixture root preflight"
    Assert-TextPresent -Text $fixtureText -Needle "clean_room_legal_artifact_required" `
        -Context "fixture root preflight"
    Assert-TextPresent -Text $fixtureText -Needle "fixture_artifact_rejected_d3d12_quality" `
        -Context "fixture root preflight"
    Assert-LinePresent -Lines $fixtureLines `
        -ExpectedLine "renderer_commercial_readiness=0" `
        -Context "fixture root preflight"
} finally {
    if (Test-Path -LiteralPath $fixtureRoot) {
        Assert-PathUnderDirectory -Path $fixtureRoot -Directory $approvedArtifactRoot `
            -Description "Renderer commercial readiness final preflight fixture cleanup root"
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    }
}

Write-Information "renderer-commercial-readiness-final-promotion-preflight-check: ok" -InformationAction Continue
