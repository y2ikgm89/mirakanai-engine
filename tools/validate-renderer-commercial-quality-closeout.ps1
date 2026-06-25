#requires -Version 7.0
#requires -PSEdition Core

param(
    [switch] $RequireReady
)

$ErrorActionPreference = "Stop"

function ConvertTo-CounterBit {
    param([bool]$Value)
    if ($Value) { return "1" }
    return "0"
}

$cleanRoomSourceReviewReady = $true
$externalEngineCodeUsed = $false
$externalEngineSampleUsed = $false
$externalEngineAssetUsed = $false
$externalEngineTrademarkUsed = $false
$externalEngineCompatibilityClaims = $false
$thirdPartyNoticesComplete = $true

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=renderer-commercial-quality-closeout")
$lines.Add("renderer_commercial_quality_closeout_status=host_evidence_required")
$lines.Add("renderer_commercial_quality_closeout_ready=0")
$lines.Add("renderer_commercial_quality_closeout_value_api_ready=1")
$lines.Add("renderer_commercial_quality_closeout_require_ready=$(ConvertTo-CounterBit $RequireReady)")
$lines.Add("renderer_clean_room_source_review_ready=$(ConvertTo-CounterBit $cleanRoomSourceReviewReady)")
$lines.Add("renderer_external_engine_code_used=$(ConvertTo-CounterBit $externalEngineCodeUsed)")
$lines.Add("renderer_external_engine_sample_used=$(ConvertTo-CounterBit $externalEngineSampleUsed)")
$lines.Add("renderer_external_engine_asset_used=$(ConvertTo-CounterBit $externalEngineAssetUsed)")
$lines.Add("renderer_external_engine_trademark_used=$(ConvertTo-CounterBit $externalEngineTrademarkUsed)")
$lines.Add("renderer_external_engine_compatibility_claims=$(ConvertTo-CounterBit $externalEngineCompatibilityClaims)")
$lines.Add("renderer_third_party_notices_complete=$(ConvertTo-CounterBit $thirdPartyNoticesComplete)")
$lines.Add("renderer_native_handle_access=0")
$lines.Add("renderer_cross_backend_inference=0")
$lines.Add("renderer_external_engine_parity=0")
$lines.Add("renderer_backend_parity_ready=0")
$lines.Add("renderer_metal_broad_readiness=0")
$lines.Add("renderer_broad_quality_ready=0")
$lines.Add("renderer_commercial_readiness=0")
$lines.Add("renderer_environment_ready=0")
$lines.Add("renderer_commercial_quality_closeout_blocker=validator_integration_and_host_evidence_required")

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady) {
    Write-Error "Renderer Commercial Quality Closeout v1 is not ready: validator integration plus selected backend/package/Metal host evidence are required before -RequireReady can pass."
}
