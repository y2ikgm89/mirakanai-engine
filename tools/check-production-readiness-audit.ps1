#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$manifestPath = Join-Path $root "engine\agent\manifest.json"
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    Write-Error "Production readiness audit requires engine/agent/manifest.json"
}

try {
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
}
catch {
    Write-Error "engine/agent/manifest.json is not parseable JSON: $($_.Exception.Message)"
}

$productionLoop = $manifest.aiOperableProductionLoop
if ($null -eq $productionLoop) {
    Write-Error "engine/agent/manifest.json is missing aiOperableProductionLoop"
}

$gaps = @($productionLoop.unsupportedProductionGaps)
$zeroGapCloseoutArchive = Join-Path $root "docs\superpowers\master-plans\production-completion-v1\99-historical-verdict-archive.md"
if ($gaps.Count -eq 0) {
    if (-not (Test-Path -LiteralPath $zeroGapCloseoutArchive -PathType Leaf)) {
        Write-Error "Production readiness audit accepts a zero-gap manifest only after the full repository quality gate closeout evidence exists."
    }
    $zeroGapCloseoutArchiveText = Get-Content -LiteralPath $zeroGapCloseoutArchive -Raw
    foreach ($needle in @("2026-05-18-full-repository-quality-gate-1-0-closeout-v1.md", "Full Repository Quality Gate 1.0 Closeout")) {
        if (-not $zeroGapCloseoutArchiveText.Contains($needle)) {
            Write-Error "Production readiness audit accepts a zero-gap manifest only after the full repository quality gate closeout evidence is archived: $needle"
        }
    }
}

$allowedStatuses = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
foreach ($status in @(
    "implemented-contract-only",
    "implemented-foundation-only",
    "implemented-generated-desktop-3d-package-proof",
    "implemented-source-tree-foundation-plus-generated-package-runtime-ux-proof",
    "planned-plus-scene-mesh-package-telemetry",
    "planned",
    "partly-ready"
)) {
    $allowedStatuses.Add($status) | Out-Null
}

$allowedTiers = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
foreach ($tier in @(
        "foundation-follow-up",
        "package-evidence",
        "closeout-wedge"
    )) {
    $allowedTiers.Add($tier) | Out-Null
}

$seenIds = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
$statusCounts = @{}

foreach ($gap in $gaps) {
    $id = [string]$gap.id
    $status = [string]$gap.status
    $required = @($gap.requiredBeforeReadyClaim)
    $notes = [string]$gap.notes

    if ([string]::IsNullOrWhiteSpace($id)) {
        Write-Error "unsupportedProductionGaps row has an empty id"
    }
    if (-not $seenIds.Add($id)) {
        Write-Error "unsupportedProductionGaps contains a duplicate id: $id"
    }
    if ([string]::IsNullOrWhiteSpace($status)) {
        Write-Error "unsupportedProductionGaps[$id] has an empty status"
    }
    if (-not $allowedStatuses.Contains($status)) {
        Write-Error "unsupportedProductionGaps[$id] has unexpected status '$status'"
    }
    if ($required.Count -eq 0) {
        Write-Error "unsupportedProductionGaps[$id] must list requiredBeforeReadyClaim entries"
    }
    foreach ($claim in $required) {
        if ([string]::IsNullOrWhiteSpace([string]$claim)) {
            Write-Error "unsupportedProductionGaps[$id] contains an empty requiredBeforeReadyClaim entry"
        }
    }
    if ([string]::IsNullOrWhiteSpace($notes)) {
        Write-Error "unsupportedProductionGaps[$id] must describe the unsupported boundary in notes"
    }

    $tier = [string]$gap.oneDotZeroCloseoutTier
    if ([string]::IsNullOrWhiteSpace($tier)) {
        Write-Error "unsupportedProductionGaps[$id] must set oneDotZeroCloseoutTier"
    }
    if (-not $allowedTiers.Contains($tier)) {
        Write-Error "unsupportedProductionGaps[$id] has unexpected oneDotZeroCloseoutTier '$tier'"
    }

    if (-not $statusCounts.ContainsKey($status)) {
        $statusCounts[$status] = 0
    }
    $statusCounts[$status] += 1
}

Write-Host "production-readiness-audit: unsupported_gaps=$($gaps.Count)"
foreach ($status in ($statusCounts.Keys | Sort-Object)) {
    Write-Host "production-readiness-audit: status[$status]=$($statusCounts[$status])"
}
foreach ($gap in ($gaps | Sort-Object id)) {
    $requiredCount = @($gap.requiredBeforeReadyClaim).Count
    Write-Host "production-readiness-audit: gap=$($gap.id) tier=$($gap.oneDotZeroCloseoutTier) status=$($gap.status) required=$requiredCount"
}
Write-Host "production-readiness-audit-check: ok"
