#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [switch]$RequireReady,
    [string]$ReadinessEvidenceArtifactRootRelative = "",
    [switch]$BackendParityReady,
    [switch]$D3d12Ready,
    [switch]$VulkanStrictReady,
    [switch]$AppleMetalReady,
    [switch]$RendererQualityMatrixReady,
    [switch]$ProductionVfxProfilingReady,
    [switch]$MetalMemoryProfilingReady,
    [switch]$Visible3dPackageReady,
    [switch]$RuntimeUiPackageReady,
    [switch]$EnvironmentPackageReady,
    [switch]$GeneratedGamePackageReady,
    [switch]$StaticGuardsReady,
    [switch]$IosMetalPackageEvidenceRequired,
    [switch]$IosMetalPackageEvidenceReady,
    [switch]$CleanRoomSourceReviewMissing,
    [switch]$ThirdPartyNoticesIncomplete,
    [switch]$NativeHandleAccess,
    [switch]$CrossBackendInference,
    [switch]$ExternalEngineParity,
    [switch]$ExternalEngineApiUsed,
    [switch]$ExternalEngineCodeUsed,
    [switch]$ExternalEngineSampleUsed,
    [switch]$ExternalEngineAssetUsed,
    [switch]$ExternalEngineTrademarkUsed,
    [switch]$ExternalEngineUiExpressionUsed,
    [switch]$ExternalEngineCompatibilityClaims,
    [switch]$ExternalEngineEquivalenceClaims,
    [switch]$ExternalEngineApproval,
    [switch]$BroadBackendParityClaim,
    [switch]$BroadMetalReadinessClaim,
    [switch]$BroadRendererQualityClaim,
    [switch]$CommercialRendererReadinessClaim
)

$ErrorActionPreference = "Stop"

function ConvertTo-CounterBit {
    param([bool]$Value)
    if ($Value) { return "1" }
    return "0"
}

function Add-RendererCloseoutBlocker {
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Blockers,

        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    if (-not $Blockers.Contains($Name)) {
        $Blockers.Add($Name) | Out-Null
    }
}

function Join-RendererCloseoutBlockerValue {
    param([string[]]$Value = @())

    $usableValue = @($Value | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)
    if ($usableValue.Count -eq 0) {
        return "none"
    }

    return ($usableValue -join "+")
}

function ConvertFrom-RendererCloseoutKeyValueLines {
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

function Test-RendererCloseoutEvidenceCounter {
    param(
        [Parameter(Mandatory = $true)]$Values,
        [Parameter(Mandatory = $true)][string]$Name
    )

    return ([string]$Values[$Name]) -eq "1"
}

$readinessEvidenceValues = @{}
$readinessEvidenceStatus = "not_selected"
$readinessEvidenceReady = $false
$readinessEvidenceBlocker = ""

if (-not [string]::IsNullOrWhiteSpace($ReadinessEvidenceArtifactRootRelative)) {
    $readinessEvidenceScript = Join-Path $PSScriptRoot "validate-renderer-commercial-readiness-evidence.ps1"
    $readinessEvidenceArguments = @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        $readinessEvidenceScript,
        "-ArtifactRootRelative",
        $ReadinessEvidenceArtifactRootRelative
    )
    if ($RequireReady.IsPresent) {
        $readinessEvidenceArguments += "-RequireReady"
    }

    $readinessEvidenceOutput = @(& pwsh @readinessEvidenceArguments 2>&1)
    $readinessEvidenceValues = ConvertFrom-RendererCloseoutKeyValueLines -Lines $readinessEvidenceOutput
    $readinessEvidenceStatus = [string]$readinessEvidenceValues["renderer_commercial_readiness_evidence_status"]
    $readinessEvidenceReady = Test-RendererCloseoutEvidenceCounter `
        -Values $readinessEvidenceValues `
        -Name "renderer_commercial_readiness_evidence_ready"

    if ($LASTEXITCODE -ne 0 -or -not $readinessEvidenceReady) {
        $readinessEvidenceBlocker = "readiness_evidence_required"
    } else {
        $BackendParityReady = Test-RendererCloseoutEvidenceCounter `
            -Values $readinessEvidenceValues `
            -Name "renderer_backend_parity_ready"
        $D3d12Ready = Test-RendererCloseoutEvidenceCounter `
            -Values $readinessEvidenceValues `
            -Name "renderer_d3d12_renderer_quality_ready"
        $VulkanStrictReady = Test-RendererCloseoutEvidenceCounter `
            -Values $readinessEvidenceValues `
            -Name "renderer_vulkan_strict_renderer_quality_ready"
        $AppleMetalReady = Test-RendererCloseoutEvidenceCounter `
            -Values $readinessEvidenceValues `
            -Name "renderer_apple_metal_renderer_quality_ready"
        $RendererQualityMatrixReady = Test-RendererCloseoutEvidenceCounter `
            -Values $readinessEvidenceValues `
            -Name "renderer_quality_matrix_ready"
        $ProductionVfxProfilingReady = Test-RendererCloseoutEvidenceCounter `
            -Values $readinessEvidenceValues `
            -Name "renderer_production_vfx_profiling_ready"
        $MetalMemoryProfilingReady = Test-RendererCloseoutEvidenceCounter `
            -Values $readinessEvidenceValues `
            -Name "renderer_metal_memory_profiling_ready"
        $Visible3dPackageReady = Test-RendererCloseoutEvidenceCounter `
            -Values $readinessEvidenceValues `
            -Name "renderer_visible_3d_package_ready"
        $RuntimeUiPackageReady = Test-RendererCloseoutEvidenceCounter `
            -Values $readinessEvidenceValues `
            -Name "renderer_runtime_ui_package_ready"
        $EnvironmentPackageReady = Test-RendererCloseoutEvidenceCounter `
            -Values $readinessEvidenceValues `
            -Name "renderer_environment_package_ready"
        $GeneratedGamePackageReady = Test-RendererCloseoutEvidenceCounter `
            -Values $readinessEvidenceValues `
            -Name "renderer_generated_game_package_ready"
        $StaticGuardsReady = Test-RendererCloseoutEvidenceCounter `
            -Values $readinessEvidenceValues `
            -Name "renderer_static_guards_ready"
    }
}

$coreEvidenceRows = @(
    @{ Counter = "renderer_backend_parity_evidence_ready"; Ready = [bool]$BackendParityReady; Blocker = "backend_parity_evidence_required" },
    @{ Counter = "renderer_d3d12_renderer_quality_ready"; Ready = [bool]$D3d12Ready; Blocker = "d3d12_renderer_quality_evidence_required" },
    @{ Counter = "renderer_vulkan_strict_renderer_quality_ready"; Ready = [bool]$VulkanStrictReady; Blocker = "vulkan_strict_renderer_quality_evidence_required" },
    @{ Counter = "renderer_apple_metal_renderer_quality_ready"; Ready = [bool]$AppleMetalReady; Blocker = "apple_metal_renderer_quality_evidence_required" },
    @{ Counter = "renderer_quality_matrix_ready"; Ready = [bool]$RendererQualityMatrixReady; Blocker = "renderer_quality_matrix_evidence_required" },
    @{ Counter = "renderer_production_vfx_profiling_ready"; Ready = [bool]$ProductionVfxProfilingReady; Blocker = "production_vfx_profiling_evidence_required" },
    @{ Counter = "renderer_metal_memory_profiling_ready"; Ready = [bool]$MetalMemoryProfilingReady; Blocker = "metal_memory_profiling_evidence_required" },
    @{ Counter = "renderer_visible_3d_package_ready"; Ready = [bool]$Visible3dPackageReady; Blocker = "visible_3d_package_evidence_required" },
    @{ Counter = "renderer_runtime_ui_package_ready"; Ready = [bool]$RuntimeUiPackageReady; Blocker = "runtime_ui_package_evidence_required" },
    @{ Counter = "renderer_environment_package_ready"; Ready = [bool]$EnvironmentPackageReady; Blocker = "environment_package_evidence_required" },
    @{ Counter = "renderer_generated_game_package_ready"; Ready = [bool]$GeneratedGamePackageReady; Blocker = "generated_game_package_evidence_required" },
    @{ Counter = "renderer_static_guards_ready"; Ready = [bool]$StaticGuardsReady; Blocker = "static_guard_evidence_required" }
)

if ($IosMetalPackageEvidenceRequired) {
    $coreEvidenceRows += @{
        Counter = "renderer_ios_metal_package_evidence_ready"
        Ready = [bool]$IosMetalPackageEvidenceReady
        Blocker = "ios_metal_package_evidence_required"
    }
}

$cleanRoomSourceReviewReady = -not $CleanRoomSourceReviewMissing
$thirdPartyNoticesComplete = -not $ThirdPartyNoticesIncomplete
$externalEngineMaterialPresent = (
    $ExternalEngineApiUsed -or
    $ExternalEngineCodeUsed -or
    $ExternalEngineSampleUsed -or
    $ExternalEngineAssetUsed -or
    $ExternalEngineTrademarkUsed -or
    $ExternalEngineUiExpressionUsed -or
    $ExternalEngineCompatibilityClaims -or
    $ExternalEngineEquivalenceClaims
)
$externalEngineApprovalReady = (-not $externalEngineMaterialPresent) -or [bool]$ExternalEngineApproval

$readyRows = 0
$missingEvidenceRows = 0
$blockers = [System.Collections.Generic.List[string]]::new()

foreach ($row in $coreEvidenceRows) {
    if ([bool]$row.Ready) {
        $readyRows += 1
    }
    else {
        $missingEvidenceRows += 1
        if ($RequireReady) {
            Add-RendererCloseoutBlocker -Blockers $blockers -Name $row.Blocker
        }
    }
}

if (-not $cleanRoomSourceReviewReady) {
    Add-RendererCloseoutBlocker -Blockers $blockers -Name "clean_room_source_review_required"
}
if (-not $thirdPartyNoticesComplete) {
    Add-RendererCloseoutBlocker -Blockers $blockers -Name "third_party_notices_required"
}
if (-not $externalEngineApprovalReady) {
    Add-RendererCloseoutBlocker -Blockers $blockers -Name "external_engine_material_without_approval"
}
if ($NativeHandleAccess) {
    Add-RendererCloseoutBlocker -Blockers $blockers -Name "native_handle_access_forbidden"
}
if ($CrossBackendInference) {
    Add-RendererCloseoutBlocker -Blockers $blockers -Name "cross_backend_inference_forbidden"
}
if ($ExternalEngineParity) {
    Add-RendererCloseoutBlocker -Blockers $blockers -Name "external_engine_parity_forbidden"
}
if (-not [string]::IsNullOrWhiteSpace($readinessEvidenceBlocker)) {
    Add-RendererCloseoutBlocker -Blockers $blockers -Name $readinessEvidenceBlocker
}

$allCoreEvidenceReady = ($missingEvidenceRows -eq 0)
$safetyReady = (
    $cleanRoomSourceReviewReady -and
    $thirdPartyNoticesComplete -and
    $externalEngineApprovalReady -and
    (-not $NativeHandleAccess) -and
    (-not $CrossBackendInference) -and
    (-not $ExternalEngineParity)
)
$readyEvidenceAvailable = ($allCoreEvidenceReady -and $safetyReady)

if ($BroadBackendParityClaim -and -not $readyEvidenceAvailable) {
    Add-RendererCloseoutBlocker -Blockers $blockers -Name "broad_backend_parity_claim_without_exact_evidence"
}
if ($BroadMetalReadinessClaim -and -not $readyEvidenceAvailable) {
    Add-RendererCloseoutBlocker -Blockers $blockers -Name "broad_metal_readiness_claim_without_exact_evidence"
}
if ($BroadRendererQualityClaim -and -not $readyEvidenceAvailable) {
    Add-RendererCloseoutBlocker -Blockers $blockers -Name "broad_renderer_quality_claim_without_exact_evidence"
}
if ($CommercialRendererReadinessClaim -and -not $readyEvidenceAvailable) {
    Add-RendererCloseoutBlocker -Blockers $blockers -Name "commercial_renderer_readiness_claim_without_exact_evidence"
}

$ready = ($RequireReady -and $readyEvidenceAvailable -and $blockers.Count -eq 0)

if (-not $RequireReady) {
    Add-RendererCloseoutBlocker -Blockers $blockers -Name "require_ready_not_selected"
}

$status = if ($ready) {
    "ready"
}
elseif ($RequireReady) {
    "blocked"
}
else {
    "host_evidence_required"
}

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("validation_recipe=renderer-commercial-quality-closeout")
$lines.Add("renderer_commercial_quality_closeout_status=$status")
$lines.Add("renderer_commercial_quality_closeout_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("renderer_commercial_quality_closeout_value_api_ready=1")
$lines.Add("renderer_commercial_quality_closeout_require_ready=$(ConvertTo-CounterBit $RequireReady)")
$lines.Add("renderer_commercial_readiness_evidence_status=$readinessEvidenceStatus")
$lines.Add("renderer_commercial_readiness_evidence_ready=$(ConvertTo-CounterBit $readinessEvidenceReady)")
$lines.Add("renderer_selected_evidence_rows=$($coreEvidenceRows.Count)")
$lines.Add("renderer_selected_evidence_ready_rows=$readyRows")
$lines.Add("renderer_selected_evidence_missing_rows=$missingEvidenceRows")
$lines.Add("renderer_ios_metal_package_evidence_required=$(ConvertTo-CounterBit $IosMetalPackageEvidenceRequired)")

foreach ($row in $coreEvidenceRows) {
    $lines.Add("$($row.Counter)=$(ConvertTo-CounterBit ([bool]$row.Ready))")
}

if (-not $IosMetalPackageEvidenceRequired) {
    $lines.Add("renderer_ios_metal_package_evidence_ready=0")
}

$lines.Add("renderer_clean_room_source_review_ready=$(ConvertTo-CounterBit $cleanRoomSourceReviewReady)")
$lines.Add("renderer_third_party_notices_complete=$(ConvertTo-CounterBit $thirdPartyNoticesComplete)")
$lines.Add("renderer_clean_room_legal_ready=$(ConvertTo-CounterBit ($cleanRoomSourceReviewReady -and $thirdPartyNoticesComplete -and $externalEngineApprovalReady))")
$lines.Add("renderer_external_engine_approval_ready=$(ConvertTo-CounterBit $externalEngineApprovalReady)")
$lines.Add("renderer_external_engine_approval=$(ConvertTo-CounterBit $ExternalEngineApproval)")
$lines.Add("renderer_external_engine_api_used=$(ConvertTo-CounterBit $ExternalEngineApiUsed)")
$lines.Add("renderer_external_engine_code_used=$(ConvertTo-CounterBit $ExternalEngineCodeUsed)")
$lines.Add("renderer_external_engine_sample_used=$(ConvertTo-CounterBit $ExternalEngineSampleUsed)")
$lines.Add("renderer_external_engine_asset_used=$(ConvertTo-CounterBit $ExternalEngineAssetUsed)")
$lines.Add("renderer_external_engine_trademark_used=$(ConvertTo-CounterBit $ExternalEngineTrademarkUsed)")
$lines.Add("renderer_external_engine_ui_expression_used=$(ConvertTo-CounterBit $ExternalEngineUiExpressionUsed)")
$lines.Add("renderer_external_engine_compatibility_claims=$(ConvertTo-CounterBit $ExternalEngineCompatibilityClaims)")
$lines.Add("renderer_external_engine_equivalence_claims=$(ConvertTo-CounterBit $ExternalEngineEquivalenceClaims)")
$lines.Add("renderer_native_handle_access=$(ConvertTo-CounterBit $NativeHandleAccess)")
$lines.Add("renderer_cross_backend_inference=$(ConvertTo-CounterBit $CrossBackendInference)")
$lines.Add("renderer_external_engine_parity=$(ConvertTo-CounterBit $ExternalEngineParity)")
$lines.Add("renderer_broad_backend_parity_claim=$(ConvertTo-CounterBit $BroadBackendParityClaim)")
$lines.Add("renderer_broad_metal_readiness_claim=$(ConvertTo-CounterBit $BroadMetalReadinessClaim)")
$lines.Add("renderer_broad_renderer_quality_claim=$(ConvertTo-CounterBit $BroadRendererQualityClaim)")
$lines.Add("renderer_commercial_renderer_readiness_claim=$(ConvertTo-CounterBit $CommercialRendererReadinessClaim)")
$lines.Add("renderer_backend_parity_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("renderer_metal_broad_readiness=$(ConvertTo-CounterBit $ready)")
$lines.Add("renderer_broad_quality_ready=$(ConvertTo-CounterBit $ready)")
$lines.Add("renderer_commercial_readiness=$(ConvertTo-CounterBit $ready)")
$lines.Add("renderer_environment_ready=0")
$lines.Add("renderer_commercial_quality_closeout_blocker=$(Join-RendererCloseoutBlockerValue $blockers.ToArray())")

foreach ($line in $lines) {
    Write-Output $line
}

if ($RequireReady -and -not $ready) {
    Write-Error "Renderer Commercial Quality Closeout v1 is not ready: $($blockers -join ', ')"
}
