#requires -Version 7.0
#requires -PSEdition Core

# Chapter 6.9 for check-json-contracts.ps1 2D originality review static contracts.

$engine = Read-Json "engine/agent/manifest.json"
$geToolsModule = @($engine.modules | Where-Object { $_.name -eq "MK_tools" })
if ($geToolsModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_tools module for 2D originality review"
}

if (@($geToolsModule[0].publicHeaders) -notcontains "engine/tools/include/mirakana/tools/2d_originality_review.hpp") {
    Write-Error "engine manifest MK_tools publicHeaders must include 2d_originality_review.hpp"
}

$geToolsPurpose = [string]$geToolsModule[0].purpose
foreach ($needle in @(
        "review_2d_originality_sources",
        "review_2d_commercial_production_sources",
        "TwoDOriginalitySourceRow",
        "TwoDOriginalityReviewResult",
        "clean-room counters",
        "official_source_ledger_ready",
        "commercial_production_source_gate_ready",
        "counsel-review-required",
        "copied code/assets/documentation",
        "external engine schemas",
        "trademark surfaces",
        "external engine compatibility/equivalence/parity claims",
        "legal clearance automation"
    )) {
    if (-not $geToolsPurpose.Contains($needle)) {
        Write-Error "engine manifest MK_tools purpose must describe the 2D originality review gate and legal-clean-room non-claim: $needle"
    }
}
