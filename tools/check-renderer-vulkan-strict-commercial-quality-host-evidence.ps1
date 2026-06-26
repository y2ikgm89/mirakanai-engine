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
        foreach ($token in ([string]$line -split "\s+")) {
            $separator = $token.IndexOf("=")
            if ($separator -le 0) {
                continue
            }
            $values[$token.Substring(0, $separator)] = $token.Substring($separator + 1)
        }
    }
    return $values
}

function Write-TextFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Value
    )

    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        $null = New-Item -ItemType Directory -Path $parent -Force
    }
    Set-Content -LiteralPath $Path -Value $Value -Encoding utf8NoBOM
}

$producerScript = Join-Path $root "tools/generate-renderer-vulkan-strict-commercial-quality-host-evidence.ps1"
if (-not (Test-Path -LiteralPath $producerScript -PathType Leaf)) {
    Write-Error "tools/generate-renderer-vulkan-strict-commercial-quality-host-evidence.ps1 must exist for retained strict Vulkan host evidence generation."
}

$artifactCollectorScript = Join-Path $root "tools/collect-renderer-vulkan-strict-commercial-quality-artifact.ps1"
if (-not (Test-Path -LiteralPath $artifactCollectorScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-vulkan-strict-commercial-quality-artifact.ps1 must exist for retained strict Vulkan artifact production."
}

$readinessCollectorScript = Join-Path $root "tools/collect-renderer-commercial-readiness-evidence.ps1"
if (-not (Test-Path -LiteralPath $readinessCollectorScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-commercial-readiness-evidence.ps1 must exist for retained artifact assembly."
}

$fixtureRoot = "tests/fixtures/renderer/commercial-readiness-evidence/ready"
$evidenceRootRelative = "artifacts/renderer/vulkan-strict-commercial-quality-host-evidence/contract-$PID"
$sourceRootRelative = "$evidenceRootRelative/source"
$producerOutputRootRelative = "$evidenceRootRelative/host-output"
$artifactOutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/vulkan-strict-host-evidence-contract-$PID/vulkan-artifact"
$collectorOutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/vulkan-strict-host-evidence-contract-$PID/readiness"
$platformOnlySmokeRelative = "$sourceRootRelative/linux-platform-only.log"
$packageSmokeRelative = "$sourceRootRelative/vulkan-strict-package.log"
$linuxHostStrictSmokeRelative = "artifacts/environment/platform/linux-vulkan-host/strict-contract-$PID/validate-linux-vulkan-runtime-host.txt"
$evidenceRootPath = ConvertTo-LocalPath $evidenceRootRelative
$commercialRootPath = ConvertTo-LocalPath "artifacts/renderer/commercial-readiness-evidence/vulkan-strict-host-evidence-contract-$PID"
$linuxHostStrictSmokePath = ConvertTo-LocalPath $linuxHostStrictSmokeRelative

$platformOnlySmokeText = @"
sample_desktop_runtime_game status=ready validation_recipe=environment-platform-linux-vulkan-package host=linux vulkaninfo_ready=1 VK_LAYER_KHRONOS_validation_ready=1 dxc_spirv_codegen_ready=1 spirv_val_ready=1 linux_package_smoke_ready=1 linux_vulkan_readback_ready=1 linux_vulkan_validation_log_clean=1 environment_platform_linux_vulkan_ready=1 environment_all_platform_unconditional_ready=0 native_handle_access=0
"@

$packageSmokeText = @"
sample_desktop_runtime_game status=ready environment_vulkan_strict_aggregate_status=ready environment_vulkan_strict_aggregate_ready=1 environment_vulkan_strict_aggregate_selected_backend=vulkan environment_vulkan_strict_aggregate_toolchain_ready=1 environment_vulkan_strict_aggregate_vulkan_sdk_tools_ready=1 environment_vulkan_strict_aggregate_dxc_spirv_codegen_ready=1 environment_vulkan_strict_aggregate_spirv_validation_ready=1 environment_vulkan_strict_aggregate_validation_layers_ready=1 environment_vulkan_strict_aggregate_device_features_ready=1 environment_vulkan_strict_aggregate_toolchain_rows=6 environment_vulkan_strict_aggregate_missing_toolchain_rows=0 environment_vulkan_strict_aggregate_missing_validation_layer_rows=0 environment_vulkan_strict_aggregate_missing_spirv_validation_rows=0 environment_vulkan_strict_aggregate_unsupported_feature_device_rows=0 environment_vulkan_strict_aggregate_synchronization2_barriers=7 environment_vulkan_strict_aggregate_resource_usage_layout_ready=1 environment_vulkan_strict_aggregate_resource_usage_layout_rows=20 environment_vulkan_strict_aggregate_sampled_texture_usage_layout_rows=6 environment_vulkan_strict_aggregate_storage_buffer_usage_layout_rows=2 environment_vulkan_strict_aggregate_cube_map_usage_layout_rows=1 environment_vulkan_strict_aggregate_readback_resource_usage_layout_rows=5 environment_vulkan_strict_aggregate_renderer_draws=2 environment_vulkan_strict_aggregate_compute_dispatches=1 environment_vulkan_strict_aggregate_texture_uploads=3 environment_vulkan_strict_aggregate_readback_rows=5 environment_vulkan_strict_aggregate_native_handle_access=0 environment_vulkan_strict_aggregate_d3d12_fallback=0 environment_vulkan_strict_aggregate_metal_fallback=0 environment_vulkan_strict_aggregate_backend_parity=0 environment_vulkan_strict_aggregate_broad_optimization_claimed=0 environment_vulkan_strict_aggregate_diagnostics=0 vulkan_gpu_memory_execution_status=ready vulkan_gpu_memory_execution_ready=1 vulkan_gpu_memory_execution_selected=1 vulkan_gpu_memory_execution_committed_byte_estimate_available=1 vulkan_gpu_memory_execution_committed_resources_byte_estimate=4096 vulkan_gpu_memory_execution_upload_bytes_written=2048 vulkan_gpu_memory_execution_framegraph_barrier_steps_executed=7 vulkan_gpu_memory_execution_budget_ok=1 vulkan_gpu_memory_execution_transient_heap_ok=1 debug_profiling_policy_status=ready debug_profiling_policy_ready=1 debug_profiling_policy_diagnostics=0 debug_profiling_policy_backend_profiling_evidence_required=1 debug_profiling_policy_backend_profiling_evidence_ready=1 debug_profiling_policy_gpu_timestamp_ticks_per_second=1000000000 debug_profiling_policy_gpu_timestamp_requests=1 vulkan_debug_profiling_execution_status=ready vulkan_debug_profiling_execution_ready=1 vulkan_debug_profiling_execution_selected=1 vulkan_debug_profiling_execution_gpu_timestamp_ticks_per_second=1000000000 vulkan_debug_profiling_execution_gpu_timestamps_ok=1 vulkan_debug_profiling_execution_gpu_debug_markers_ok=1 vulkan_debug_profiling_execution_frame_diagnostics_ok=1 vulkan_debug_profiling_execution_framegraph_barrier_steps_executed=7 vulkan_debug_profiling_execution_framegraph_render_passes_recorded=3
"@

try {
    foreach ($path in @($evidenceRootPath, $commercialRootPath)) {
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -Recurse -Force
        }
    }

    Write-TextFile -Path (ConvertTo-LocalPath $platformOnlySmokeRelative) -Value $platformOnlySmokeText
    Write-TextFile -Path (ConvertTo-LocalPath $packageSmokeRelative) -Value $packageSmokeText
    Write-TextFile -Path $linuxHostStrictSmokePath -Value $packageSmokeText

    $linuxHostGateScriptText = Get-Content -LiteralPath (Join-Path $root "tools/validate-linux-vulkan-runtime-host.ps1") -Raw
    foreach ($needle in @("[string[]]`$SmokeArgs = @()", "-SmokeArgs", "`$SmokeArgs")) {
        if (-not $linuxHostGateScriptText.Contains($needle)) {
            Write-Error "Linux Vulkan runtime host gate must accept and forward strict package SmokeArgs for commercial Vulkan evidence: $needle"
        }
    }
    foreach ($needle in @("ConvertTo-PowerShellSingleQuotedLiteral", "`"-EncodedCommand`"", "[System.Text.Encoding]::Unicode")) {
        if (-not $linuxHostGateScriptText.Contains($needle)) {
            Write-Error "Linux Vulkan runtime host gate must encode strict package SmokeArgs so dash-leading values bind as string[]: $needle"
        }
    }
    if ($linuxHostGateScriptText.Contains("`$packageArguments += `"`-SmokeArgs`"")) {
        Write-Error "Linux Vulkan runtime host gate must not forward strict package SmokeArgs directly through pwsh -File argument parsing."
    }

    $workflowText = Get-Content -LiteralPath (Join-Path $root ".github/workflows/validate.yml") -Raw
    foreach ($needle in @(
            "Validate Linux Vulkan host evidence gate",
            "--require-environment-vulkan-strict-aggregate",
            "Collect renderer strict Vulkan commercial quality host evidence",
            "artifacts/environment/platform/linux-vulkan-host/validate-linux-vulkan-runtime-host.txt",
            "Upload renderer strict Vulkan commercial quality host evidence",
            "name: renderer-vulkan-strict-commercial-quality-host-evidence"
        )) {
        if (-not $workflowText.Contains($needle)) {
            Write-Error "Validate workflow must collect strict Vulkan commercial evidence from the Linux Vulkan host lane: $needle"
        }
    }

    $planLines = @(& $producerScript -Mode Plan -OutputRootRelative $producerOutputRootRelative)
    Assert-LinePresent $planLines `
        "renderer_vulkan_strict_commercial_quality_host_evidence_generator_mode=Plan" `
        "strict Vulkan host evidence generator Plan mode"
    Assert-LinePresent $planLines `
        "renderer_vulkan_strict_commercial_quality_host_evidence_written=0" `
        "strict Vulkan host evidence generator Plan mode"
    Assert-LinePresent $planLines `
        "renderer_commercial_readiness=0" `
        "strict Vulkan host evidence generator Plan mode"
    Assert-LinePresent $planLines `
        "renderer_environment_ready=0" `
        "strict Vulkan host evidence generator Plan mode"

    $unsafeRejected = $false
    try {
        $null = & $producerScript `
            -Mode Generate `
            -OutputRootRelative $producerOutputRootRelative `
            -PackageSmokeOutputRelative "../unsafe.txt" 2>&1
    }
    catch {
        $unsafeRejected = [string]$_.Exception.Message -like "*unsafe_relative_path*"
    }
    if (-not $unsafeRejected) {
        Write-Error "strict Vulkan host evidence generator must reject unsafe relative paths."
    }

    $platformOnlyRejected = $false
    try {
        $null = & $producerScript `
            -Mode Generate `
            -OutputRootRelative $producerOutputRootRelative `
            -PackageSmokeOutputRelative $platformOnlySmokeRelative `
            -RequireReady 2>&1
    }
    catch {
        $platformOnlyRejected = [string]$_.Exception.Message -like "*vulkan_strict_host_evidence_not_ready*"
    }
    if (-not $platformOnlyRejected) {
        Write-Error "strict Vulkan host evidence generator must fail closed for Linux/platform-only smoke logs."
    }
    $platformHostEvidencePath = ConvertTo-LocalPath "$producerOutputRootRelative/vulkan-strict-host-evidence.json"
    if (Test-Path -LiteralPath $platformHostEvidencePath -PathType Leaf) {
        Write-Error "platform-only Vulkan smoke must not write strict Vulkan host evidence JSON."
    }

    $linuxHostGenerateLines = @(& $producerScript `
            -Mode Generate `
            -OutputRootRelative $producerOutputRootRelative `
            -PackageSmokeOutputRelative $linuxHostStrictSmokeRelative `
            -RequireReady)
    foreach ($expectedLine in @(
            "renderer_vulkan_strict_commercial_quality_host_evidence_status=ready",
            "renderer_vulkan_strict_commercial_quality_host_evidence_ready=1",
            "renderer_vulkan_synchronization2_ready=1",
            "renderer_vulkan_validation_layer_ready=1",
            "renderer_vulkan_sync_validation_ready=1",
            "renderer_vulkan_memory_binding_ready=1",
            "renderer_vulkan_timestamp_ready=1",
            "renderer_vulkan_shader_validation_ready=1",
            "renderer_vulkan_package_readback_ready=1"
        )) {
        Assert-LinePresent $linuxHostGenerateLines $expectedLine "strict Vulkan host evidence generator Linux host gate log input"
    }
    if (Test-Path -LiteralPath $platformHostEvidencePath -PathType Leaf) {
        Remove-Item -LiteralPath $platformHostEvidencePath -Force
    }

    $generateLines = @(& $producerScript `
            -Mode Generate `
            -OutputRootRelative $producerOutputRootRelative `
            -PackageSmokeOutputRelative $packageSmokeRelative `
            -RequireReady)
    foreach ($expectedLine in @(
            "renderer_vulkan_strict_commercial_quality_host_evidence_generator_mode=Generate",
            "renderer_vulkan_strict_commercial_quality_host_evidence_status=ready",
            "renderer_vulkan_strict_commercial_quality_host_evidence_ready=1",
            "renderer_vulkan_strict_commercial_quality_host_evidence_written=1",
            "renderer_vulkan_synchronization2_ready=1",
            "renderer_vulkan_validation_layer_ready=1",
            "renderer_vulkan_sync_validation_ready=1",
            "renderer_vulkan_memory_binding_ready=1",
            "renderer_vulkan_timestamp_ready=1",
            "renderer_vulkan_shader_validation_ready=1",
            "renderer_vulkan_package_readback_ready=1",
            "renderer_vulkan_native_handles_exposed=0",
            "renderer_backend_parity_ready=0",
            "renderer_metal_broad_readiness=0",
            "renderer_broad_quality_ready=0",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        )) {
        Assert-LinePresent $generateLines $expectedLine "strict Vulkan host evidence generator Generate mode"
    }

    $hostEvidenceRelative = "$producerOutputRootRelative/vulkan-strict-host-evidence.json"
    $hostEvidencePath = ConvertTo-LocalPath $hostEvidenceRelative
    if (-not (Test-Path -LiteralPath $hostEvidencePath -PathType Leaf)) {
        Write-Error "strict Vulkan host evidence generator did not write vulkan-strict-host-evidence.json."
    }
    $hostEvidence = Get-Content -LiteralPath $hostEvidencePath -Raw | ConvertFrom-Json
    if ([string]$hostEvidence.schema_version -ne "GameEngine.RendererVulkanStrictCommercialQualityHostEvidence.v1") {
        Write-Error "strict Vulkan host evidence schema_version mismatch."
    }
    if ([string]$hostEvidence.claim_id -ne "renderer-vulkan-strict-commercial-quality-artifact-v1") {
        Write-Error "strict Vulkan host evidence claim_id mismatch."
    }
    if ([string]$hostEvidence.source_id -ne "Khronos-Vulkan-Synchronization2-Memory-QueueOwnership-2026-06-25") {
        Write-Error "strict Vulkan host evidence source_id mismatch."
    }
    if ([bool]$hostEvidence.fixture_only) {
        Write-Error "strict Vulkan host evidence must not be fixture_only."
    }
    if ([string]$hostEvidence.proof_rows.synchronization2.api_name -ne "vkCmdPipelineBarrier2") {
        Write-Error "strict Vulkan synchronization2 API mismatch."
    }
    if ([string]$hostEvidence.proof_rows.synchronization2.structure_name -ne "VkDependencyInfo") {
        Write-Error "strict Vulkan synchronization2 structure mismatch."
    }
    if ([string]$hostEvidence.proof_rows.validation_layer.layer_name -ne "VK_LAYER_KHRONOS_validation") {
        Write-Error "strict Vulkan validation layer mismatch."
    }
    if ([string]$hostEvidence.proof_rows.memory_binding.vuid_reference -notlike "*VUID*") {
        Write-Error "strict Vulkan memory binding row must retain a VUID reference."
    }
    if ([double]$hostEvidence.proof_rows.timestamp_query.timestamp_period_ns -le 0.0) {
        Write-Error "strict Vulkan timestamp row must retain a positive timestamp period."
    }
    if ([string]$hostEvidence.proof_rows.package_visible_readback.deterministic_hash_sha256 -cnotmatch "^[0-9a-f]{64}$") {
        Write-Error "strict Vulkan host evidence readback hash must be lower-case SHA-256."
    }
    if ([bool]$hostEvidence.proof_rows.native_handles.native_handles_exposed) {
        Write-Error "strict Vulkan host evidence must keep native handles unexposed."
    }
    if ([bool]$hostEvidence.non_claims.environment_ready) {
        Write-Error "strict Vulkan host evidence must not claim environment_ready."
    }

    $artifactLines = @(& $artifactCollectorScript `
            -Mode Assemble `
            -OutputRootRelative $artifactOutputRootRelative `
            -VulkanStrictHostEvidenceRelative $hostEvidenceRelative)
    Assert-LinePresent $artifactLines `
        "renderer_vulkan_strict_commercial_quality_artifact_written=1" `
        "strict Vulkan artifact collector with generated host evidence"
    Assert-LinePresent $artifactLines `
        "renderer_vulkan_strict_commercial_quality_fixture_artifact=0" `
        "strict Vulkan artifact collector with generated host evidence"

    $readinessArguments = @{
        OutputRootRelative = $collectorOutputRootRelative
        D3d12ArtifactRelative = "$fixtureRoot/d3d12-quality.json"
        VulkanStrictArtifactRelative = "$artifactOutputRootRelative/vulkan-strict-quality.json"
        AppleMetalArtifactRelative = "$fixtureRoot/apple-metal-host.json"
        Visible3dPackageArtifactRelative = "$fixtureRoot/visible-3d-package.json"
        RuntimeUiPackageArtifactRelative = "$fixtureRoot/runtime-ui-package.json"
        EnvironmentPackageArtifactRelative = "$fixtureRoot/environment-package.json"
        GeneratedGamePackageArtifactRelative = "$fixtureRoot/generated-game-package.json"
        RendererQualityMatrixArtifactRelative = "$fixtureRoot/renderer-quality-matrix.json"
        ProductionVfxProfilingArtifactRelative = "$fixtureRoot/production-vfx-profiling.json"
        MetalMemoryResidencyArtifactRelative = "$fixtureRoot/metal-memory-residency.json"
        MetalProfilingCaptureArtifactRelative = "$fixtureRoot/metal-profiling-capture.json"
        OfficialDocsOnlyReviewReady = $true
        LegalReviewReady = $true
        ExternalEngineZeroMaterialReviewReady = $true
        ThirdPartyNoticesComplete = $true
    }
    $collectorLines = @(& $readinessCollectorScript -Mode Assemble @readinessArguments -AllowFixtureArtifactsForSelfTest)
    Assert-LinePresent $collectorLines `
        "renderer_commercial_readiness_evidence_collector_fixture_artifacts=10" `
        "commercial readiness collector with generated strict Vulkan host evidence"
    Assert-LinePresent $collectorLines `
        "renderer_commercial_readiness_evidence_collector_real_promotion_candidate=0" `
        "commercial readiness collector with generated strict Vulkan host evidence"

    $validationLines = @(& (Join-Path $PSScriptRoot "validate-renderer-commercial-readiness-evidence.ps1") `
            -ArtifactRootRelative $collectorOutputRootRelative)
    $validationValues = ConvertFrom-KeyValueLines -Lines $validationLines
    if ([string]$validationValues["renderer_vulkan_strict_renderer_quality_ready"] -ne "1") {
        Write-Error "generated strict Vulkan host evidence must validate as renderer_vulkan_strict_renderer_quality_ready."
    }
    if ([string]$validationValues["renderer_commercial_readiness"] -ne "0") {
        Write-Error "strict Vulkan-only host evidence generation must not promote renderer_commercial_readiness."
    }
}
finally {
    foreach ($path in @($evidenceRootPath, $commercialRootPath)) {
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -Recurse -Force
        }
    }
    $linuxHostStrictSmokeDirectory = Split-Path -Parent $linuxHostStrictSmokePath
    if (Test-Path -LiteralPath $linuxHostStrictSmokeDirectory -PathType Container) {
        Remove-Item -LiteralPath $linuxHostStrictSmokeDirectory -Recurse -Force
    }
}

Write-Information "renderer-vulkan-strict-commercial-quality-host-evidence-check: ok" -InformationAction Continue
