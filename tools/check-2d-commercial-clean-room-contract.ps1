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

function Write-TextFile {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Text
    )

    $path = ConvertTo-LocalPath $RelativePath
    $parent = Split-Path -Parent $path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        $null = New-Item -ItemType Directory -Path $parent -Force
    }
    Set-Content -LiteralPath $path -Value $Text -Encoding utf8NoBOM
}

$cleanRoomCheckScript = Join-Path $root "tools/check-2d-commercial-clean-room.ps1"
if (-not (Test-Path -LiteralPath $cleanRoomCheckScript -PathType Leaf)) {
    Write-Error "tools/check-2d-commercial-clean-room.ps1 must exist for the 2D commercial clean-room static guard."
}

$generatorScript = Join-Path $root "tools/generate-2d-commercial-clean-room-review-input.ps1"
if (-not (Test-Path -LiteralPath $generatorScript -PathType Leaf)) {
    Write-Error "tools/generate-2d-commercial-clean-room-review-input.ps1 must exist for counsel-ready 2D clean-room input records."
}

$reviewInputSchema = ConvertTo-LocalPath "schemas/2d-commercial-clean-room-review-input.schema.json"
if (-not (Test-Path -LiteralPath $reviewInputSchema -PathType Leaf)) {
    Write-Error "schemas/2d-commercial-clean-room-review-input.schema.json must exist for retained 2D clean-room review input validation."
}

$sourceSummarySchema = ConvertTo-LocalPath "schemas/2d-commercial-official-source-summary.schema.json"
if (-not (Test-Path -LiteralPath $sourceSummarySchema -PathType Leaf)) {
    Write-Error "schemas/2d-commercial-official-source-summary.schema.json must exist for retained 2D official source summary validation."
}

$contractRootRelative = "out/2d-commercial-clean-room-contract-$PID"
$cleanRootRelative = "$contractRootRelative/clean"
$badRootRelative = "$contractRootRelative/bad"
$outputRootRelative = "artifacts/2d-commercial/clean-room-review/contract-$PID"
$contractRootPath = ConvertTo-LocalPath $contractRootRelative
$outputRootPath = ConvertTo-LocalPath $outputRootRelative

try {
    foreach ($path in @($contractRootPath, $outputRootPath)) {
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -Recurse -Force
        }
    }

    Write-TextFile `
        -RelativePath "$cleanRootRelative/games/first_party_game/game.agent.json" `
        -Text @'
{
  "id": "first-party-clean-2d-game",
  "validationRecipes": [
    { "id": "installed-2d-first-party-package-smoke" }
  ]
}
'@

    $cleanLines = @(& $cleanRoomCheckScript -ScanRootRelative $cleanRootRelative 6>&1)
    Assert-LinePresent $cleanLines "2d-commercial-clean-room: ok" "clean 2D commercial clean-room scan"

    Write-TextFile `
        -RelativePath "$badRootRelative/games/bad_game/game.agent.json" `
        -Text @'
{
  "id": "unity-compatible-2d-game",
  "publicLabel": "Unity compatible importer",
  "runtimePackageFiles": ["levels/main.tscn"]
}
'@

    $badRejected = $false
    try {
        $null = & $cleanRoomCheckScript -ScanRootRelative $badRootRelative 2>&1
    } catch {
        $message = [string]$_.Exception.Message
        $badRejected = $message.Contains("forbidden_2d_commercial")
    }
    if (-not $badRejected) {
        Write-Error "2D commercial clean-room scan must reject external-engine compatibility claims and schema tokens."
    }

    $defaultLines = @(& $cleanRoomCheckScript 6>&1)
    Assert-LinePresent $defaultLines "2d-commercial-clean-room: ok" "repository 2D commercial clean-room scan"

    $planLines = @(& $generatorScript -Mode Plan -OutputRootRelative $outputRootRelative)
    foreach ($expectedLine in @(
            "2d_commercial_clean_room_review_input_generator_mode=Plan",
            "2d_commercial_clean_room_review_input_written=0",
            "2d_commercial_clean_room_review_input_ready=0",
            "requires_legal_counsel_review=1",
            "2d_commercial_clean_room_commercial_ready=0",
            "2d_commercial_clean_room_legal_approval=0"
        )) {
        Assert-LinePresent $planLines $expectedLine "2D commercial clean-room review generator Plan mode"
    }

    $unsafeRejected = $false
    try {
        $null = & $generatorScript -Mode Generate -OutputRootRelative "../unsafe" 2>&1
    } catch {
        $unsafeRejected = [string]$_.Exception.Message -like "*unsafe_relative_path*"
    }
    if (-not $unsafeRejected) {
        Write-Error "2D commercial clean-room review generator must reject unsafe output paths."
    }

    $generateLines = @(& $generatorScript -Mode Generate -OutputRootRelative $outputRootRelative)
    foreach ($expectedLine in @(
            "2d_commercial_clean_room_review_input_generator_mode=Generate",
            "2d_commercial_clean_room_review_input_written=1",
            "2d_commercial_clean_room_review_input_ready=1",
            "2d_commercial_clean_room_public_docs_only=1",
            "2d_commercial_clean_room_external_engine_zero_material_ready=1",
            "2d_commercial_clean_room_external_engine_forbidden_material_detected_rows=0",
            "2d_commercial_clean_room_external_engine_compatibility_claim=0",
            "2d_commercial_clean_room_external_engine_equivalence_claim=0",
            "2d_commercial_clean_room_external_engine_parity_claim=0",
            "requires_legal_counsel_review=1",
            "2d_commercial_clean_room_commercial_ready=0",
            "2d_commercial_clean_room_legal_approval=0"
        )) {
        Assert-LinePresent $generateLines $expectedLine "2D commercial clean-room review generator Generate mode"
    }

    $values = ConvertFrom-KeyValueLines $generateLines
    $reviewRelative = [string]$values["2d_commercial_clean_room_review_input_path"]
    $summaryRelative = [string]$values["2d_commercial_clean_room_source_summary_path"]
    foreach ($relativePath in @($reviewRelative, $summaryRelative)) {
        if (-not (Test-Path -LiteralPath (ConvertTo-LocalPath $relativePath) -PathType Leaf)) {
            Write-Error "2D commercial clean-room generator did not write expected file: $relativePath"
        }
    }

    $review = Get-Content -LiteralPath (ConvertTo-LocalPath $reviewRelative) -Raw | ConvertFrom-Json
    if (-not (Test-Json -Path (ConvertTo-LocalPath $reviewRelative) -SchemaFile $reviewInputSchema -ErrorAction Stop)) {
        Write-Error "2D commercial clean-room review input must validate against its JSON schema."
    }
    if (-not (Test-Json -Path (ConvertTo-LocalPath $summaryRelative) -SchemaFile $sourceSummarySchema -ErrorAction Stop)) {
        Write-Error "2D commercial official source summary must validate against its JSON schema."
    }
    if ([string]$review.schema_version -ne "GameEngine.TwoDCommercialCleanRoomReviewInput.v1") {
        Write-Error "2D commercial clean-room review schema_version mismatch."
    }
    if ([bool]$review.fixture_only) {
        Write-Error "2D commercial clean-room review input must not be fixture_only."
    }
    if ([bool]$review.non_claims.legal_approval) {
        Write-Error "2D commercial clean-room review input must not claim legal approval."
    }
    if (@($review.official_sources).Count -lt 10) {
        Write-Error "2D commercial clean-room review input must retain first-party, official SDK, legal, copyright, and trademark sources."
    }
    if (@($review.reference_sources).Count -lt 3) {
        Write-Error "2D commercial clean-room review input must retain Context7 reference rows separately from official source rows."
    }
    $mslReferenceRows = @($review.reference_sources | Where-Object { [string]$_.source_id -eq "Context7-Metal-Shading-Language-2026-06-30" })
    if ($mslReferenceRows.Count -ne 1 -or [string]$mslReferenceRows[0].source_class -ne "context7_documentation_mirror") {
        Write-Error "2D commercial clean-room review input must classify the Context7 MSL mirror as non-official reference evidence."
    }
}
finally {
    foreach ($path in @($contractRootPath, $outputRootPath)) {
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -Recurse -Force
        }
    }
}

Write-Information "2d-commercial-clean-room-contract-check: ok" -InformationAction Continue
