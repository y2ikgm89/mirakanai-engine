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

function Write-JsonObject {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][object]$Value
    )

    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        $null = New-Item -ItemType Directory -Path $parent -Force
    }
    $json = $Value | ConvertTo-Json -Depth 16
    Set-Content -LiteralPath $Path -Value $json -Encoding utf8NoBOM
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

function New-D3d12HostSupplement {
    param([Parameter(Mandatory = $true)][bool]$FixtureOnly)

    return [ordered]@{
        schema_version = "GameEngine.RendererD3d12CommercialQualityHostSupplement.v1"
        validation_recipe = "renderer-d3d12-commercial-quality-host-evidence"
        fixture_only = $FixtureOnly
        source_id = "Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25"
        proof_rows = [ordered]@{
            clock_calibration = [ordered]@{
                ready = $true
                api_name = "ID3D12CommandQueue::GetClockCalibration"
                cpu_qpc_sample = $true
            }
            debug_validation = [ordered]@{
                ready = $true
                debug_layer_or_gpu_based_validation_clean = $true
                debug_message_count = 0
                gpu_based_validation_message_count = 0
            }
            residency = [ordered]@{
                ready = $true
                query_video_memory_info_ready = $true
                enqueue_make_resident_fence_signaled = $true
                residency_api_name = "ID3D12Device3::EnqueueMakeResident"
                budget_api_name = "IDXGIAdapter3::QueryVideoMemoryInfo"
            }
            unordered_access_barrier = [ordered]@{
                ready = $true
                resource_barrier_api_name = "D3D12_RESOURCE_BARRIER"
            }
            native_handles = [ordered]@{
                ready = $true
                native_handles_exposed = $false
            }
        }
        non_claims = [ordered]@{
            vulkan_inferred = $false
            metal_inferred = $false
            broad_ui_parity = $false
            environment_ready = $false
            external_engine_parity = $false
            native_handles_exposed = $false
        }
    }
}

$producerScript = Join-Path $root "tools/generate-renderer-d3d12-commercial-quality-host-evidence.ps1"
if (-not (Test-Path -LiteralPath $producerScript -PathType Leaf)) {
    Write-Error "tools/generate-renderer-d3d12-commercial-quality-host-evidence.ps1 must exist for Task 10F.1 retained D3D12 host evidence generation."
}

$artifactCollectorScript = Join-Path $root "tools/collect-renderer-d3d12-commercial-quality-artifact.ps1"
if (-not (Test-Path -LiteralPath $artifactCollectorScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-d3d12-commercial-quality-artifact.ps1 must exist for retained D3D12 artifact production."
}

$readinessCollectorScript = Join-Path $root "tools/collect-renderer-commercial-readiness-evidence.ps1"
if (-not (Test-Path -LiteralPath $readinessCollectorScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-commercial-readiness-evidence.ps1 must exist for retained artifact assembly."
}

$fixtureRoot = "tests/fixtures/renderer/commercial-readiness-evidence/ready"
$evidenceRootRelative = "artifacts/renderer/d3d12-commercial-quality-host-evidence/contract-$PID"
$sourceRootRelative = "$evidenceRootRelative/source"
$producerOutputRootRelative = "$evidenceRootRelative/host-output"
$artifactOutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/d3d12-host-evidence-contract-$PID/d3d12-artifact"
$collectorOutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/d3d12-host-evidence-contract-$PID/readiness"
$packageSmokeRelative = "$sourceRootRelative/package-smoke.txt"
$supplementRelative = "$sourceRootRelative/d3d12-host-supplement.json"
$fixtureSupplementRelative = "$sourceRootRelative/d3d12-host-supplement-fixture.json"
$evidenceRootPath = ConvertTo-LocalPath $evidenceRootRelative
$commercialRootPath = ConvertTo-LocalPath "artifacts/renderer/commercial-readiness-evidence/d3d12-host-evidence-contract-$PID"

$packageSmokeText = @"
sample_desktop_runtime_game status=ready d3d12_framegraph_multiqueue_evidence_ready=1 framegraph_multiqueue_command_lists_submitted=4 framegraph_multiqueue_queue_waits_recorded=3 framegraph_multiqueue_barriers_recorded=4 framegraph_multiqueue_aliasing_barriers_recorded=1 framegraph_multiqueue_submitted_pass_fences=4 framegraph_multiqueue_graphics_waited_for_copy=1 framegraph_barrier_steps_executed=15 framegraph_render_passes_recorded=6 renderer_quality_status=ready renderer_quality_ready=1 renderer_quality_diagnostics=0 renderer_quality_framegraph_barrier_steps_ok=1 renderer_quality_framegraph_execution_budget_ok=1 postprocess_d3d12_execution_status=ready postprocess_d3d12_execution_ready=1 postprocess_d3d12_execution_passes_ok=1 debug_profiling_policy_status=ready debug_profiling_policy_ready=1 debug_profiling_policy_diagnostics=0 debug_profiling_policy_backend_profiling_evidence_required=1 debug_profiling_policy_backend_profiling_evidence_ready=1 debug_profiling_policy_gpu_timestamp_ticks_per_second=1000000000 d3d12_debug_profiling_execution_status=ready d3d12_debug_profiling_execution_ready=1 d3d12_debug_profiling_execution_selected=1 d3d12_debug_profiling_execution_gpu_timestamp_ticks_per_second=1000000000 d3d12_debug_profiling_execution_gpu_timestamps_ok=1 d3d12_debug_profiling_execution_gpu_debug_markers_ok=1 d3d12_debug_profiling_execution_frame_diagnostics_ok=1 d3d12_debug_profiling_execution_framegraph_barrier_steps_executed=15 d3d12_debug_profiling_execution_framegraph_render_passes_recorded=6 gpu_memory_policy_status=ready gpu_memory_policy_ready=1 gpu_memory_policy_diagnostics=0 gpu_memory_policy_backend_memory_evidence_required=1 gpu_memory_policy_backend_memory_evidence_ready=1 memory_diagnostics_status=ready memory_diagnostics_ready=1 memory_diagnostics_diagnostics=0 memory_diagnostics_total_bytes=4096 memory_diagnostics_package_resident_cpu_bytes=2048 memory_diagnostics_resident_gpu_bytes=1024 memory_diagnostics_upload_staging_bytes=512 memory_diagnostics_transient_gpu_aliasing_barriers=1 memory_diagnostics_transient_gpu_framegraph_aliasing_ready=1 d3d12_gpu_memory_execution_status=ready d3d12_gpu_memory_execution_ready=1 d3d12_gpu_memory_execution_selected=1 d3d12_gpu_memory_execution_committed_byte_estimate_available=1 d3d12_gpu_memory_execution_committed_resources_byte_estimate=1024 d3d12_gpu_memory_execution_upload_bytes_written=512 d3d12_gpu_memory_execution_budget_ok=1 d3d12_gpu_memory_execution_transient_heap_ok=1
"@

try {
    foreach ($path in @($evidenceRootPath, $commercialRootPath)) {
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -Recurse -Force
        }
    }

    Write-TextFile -Path (ConvertTo-LocalPath $packageSmokeRelative) -Value $packageSmokeText
    Write-JsonObject -Path (ConvertTo-LocalPath $supplementRelative) `
        -Value (New-D3d12HostSupplement -FixtureOnly $false)
    Write-JsonObject -Path (ConvertTo-LocalPath $fixtureSupplementRelative) `
        -Value (New-D3d12HostSupplement -FixtureOnly $true)

    $planLines = @(& $producerScript -Mode Plan -OutputRootRelative $producerOutputRootRelative)
    Assert-LinePresent $planLines `
        "renderer_d3d12_commercial_quality_host_evidence_generator_mode=Plan" `
        "D3D12 host evidence generator Plan mode"
    Assert-LinePresent $planLines `
        "renderer_d3d12_commercial_quality_host_evidence_written=0" `
        "D3D12 host evidence generator Plan mode"
    Assert-LinePresent $planLines `
        "renderer_commercial_readiness=0" `
        "D3D12 host evidence generator Plan mode"
    Assert-LinePresent $planLines `
        "renderer_environment_ready=0" `
        "D3D12 host evidence generator Plan mode"

    $unsafeRejected = $false
    try {
        $null = & $producerScript `
            -Mode Generate `
            -OutputRootRelative $producerOutputRootRelative `
            -PackageSmokeOutputRelative "../unsafe.txt" `
            -SupplementalHostEvidenceRelative $supplementRelative 2>&1
    }
    catch {
        $unsafeRejected = [string]$_.Exception.Message -like "*unsafe_relative_path*"
    }
    if (-not $unsafeRejected) {
        Write-Error "D3D12 host evidence generator must reject unsafe relative paths."
    }

    $fixtureSupplementRejected = $false
    try {
        $null = & $producerScript `
            -Mode Generate `
            -OutputRootRelative $producerOutputRootRelative `
            -PackageSmokeOutputRelative $packageSmokeRelative `
            -SupplementalHostEvidenceRelative $fixtureSupplementRelative `
            -RequireReady 2>&1
    }
    catch {
        $fixtureSupplementRejected = [string]$_.Exception.Message -like "*fixture_host_supplement_rejected*"
    }
    if (-not $fixtureSupplementRejected) {
        Write-Error "D3D12 host evidence generator must reject fixture-only supplemental host evidence."
    }

    $packageOnlyRejected = $false
    try {
        $null = & $producerScript `
            -Mode Generate `
            -OutputRootRelative $producerOutputRootRelative `
            -PackageSmokeOutputRelative $packageSmokeRelative `
            -RequireReady 2>&1
    }
    catch {
        $packageOnlyRejected = [string]$_.Exception.Message -like "*d3d12_host_supplement_required*"
    }
    if (-not $packageOnlyRejected) {
        Write-Error "D3D12 host evidence generator must fail closed without supplemental host proof."
    }

    $generateLines = @(& $producerScript `
            -Mode Generate `
            -OutputRootRelative $producerOutputRootRelative `
            -PackageSmokeOutputRelative $packageSmokeRelative `
            -SupplementalHostEvidenceRelative $supplementRelative `
            -RequireReady)
    foreach ($expectedLine in @(
            "renderer_d3d12_commercial_quality_host_evidence_generator_mode=Generate",
            "renderer_d3d12_commercial_quality_host_evidence_status=ready",
            "renderer_d3d12_commercial_quality_host_evidence_ready=1",
            "renderer_d3d12_commercial_quality_host_evidence_written=1",
            "renderer_d3d12_command_allocator_fence_ready=1",
            "renderer_d3d12_resource_barrier_ready=1",
            "renderer_d3d12_timestamp_ready=1",
            "renderer_d3d12_debug_validation_ready=1",
            "renderer_d3d12_residency_ready=1",
            "renderer_d3d12_package_readback_ready=1",
            "renderer_d3d12_native_handles_exposed=0",
            "renderer_backend_parity_ready=0",
            "renderer_metal_broad_readiness=0",
            "renderer_broad_quality_ready=0",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        )) {
        Assert-LinePresent $generateLines $expectedLine "D3D12 host evidence generator Generate mode"
    }

    $hostEvidenceRelative = "$producerOutputRootRelative/d3d12-host-evidence.json"
    $hostEvidencePath = ConvertTo-LocalPath $hostEvidenceRelative
    if (-not (Test-Path -LiteralPath $hostEvidencePath -PathType Leaf)) {
        Write-Error "D3D12 host evidence generator did not write d3d12-host-evidence.json."
    }
    $hostEvidence = Get-Content -LiteralPath $hostEvidencePath -Raw | ConvertFrom-Json
    if ([string]$hostEvidence.schema_version -ne "GameEngine.RendererD3d12CommercialQualityHostEvidence.v1") {
        Write-Error "D3D12 host evidence schema_version mismatch."
    }
    if ([string]$hostEvidence.claim_id -ne "renderer-d3d12-commercial-quality-artifact-v1") {
        Write-Error "D3D12 host evidence claim_id mismatch."
    }
    if ([bool]$hostEvidence.fixture_only) {
        Write-Error "D3D12 host evidence must not be fixture_only."
    }
    if (-not [bool]$hostEvidence.proof_rows.timestamp.clock_calibration) {
        Write-Error "D3D12 host evidence must prove clock calibration."
    }
    if ([int]$hostEvidence.proof_rows.debug_validation.debug_message_count -ne 0) {
        Write-Error "D3D12 host evidence debug_message_count must be zero."
    }
    if ([string]$hostEvidence.proof_rows.residency.residency_api_name -ne "ID3D12Device3::EnqueueMakeResident") {
        Write-Error "D3D12 host evidence residency API mismatch."
    }
    if ([string]$hostEvidence.proof_rows.package_visible_readback.deterministic_hash_sha256 -cnotmatch "^[0-9a-f]{64}$") {
        Write-Error "D3D12 host evidence readback hash must be lower-case SHA-256."
    }
    if ([bool]$hostEvidence.proof_rows.native_handles.native_handles_exposed) {
        Write-Error "D3D12 host evidence must keep native handles unexposed."
    }
    if ([bool]$hostEvidence.non_claims.external_engine_parity) {
        Write-Error "D3D12 host evidence must not claim external-engine parity."
    }

    $artifactLines = @(& $artifactCollectorScript `
            -Mode Assemble `
            -OutputRootRelative $artifactOutputRootRelative `
            -D3d12HostEvidenceRelative $hostEvidenceRelative)
    Assert-LinePresent $artifactLines `
        "renderer_d3d12_commercial_quality_artifact_written=1" `
        "D3D12 artifact collector with generated host evidence"
    Assert-LinePresent $artifactLines `
        "renderer_d3d12_commercial_quality_fixture_artifact=0" `
        "D3D12 artifact collector with generated host evidence"

    $readinessArguments = @{
        OutputRootRelative = $collectorOutputRootRelative
        D3d12ArtifactRelative = "$artifactOutputRootRelative/d3d12-quality.json"
        VulkanStrictArtifactRelative = "$fixtureRoot/vulkan-strict-quality.json"
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
        "commercial readiness collector with generated D3D12 host evidence"
    Assert-LinePresent $collectorLines `
        "renderer_commercial_readiness_evidence_collector_real_promotion_candidate=0" `
        "commercial readiness collector with generated D3D12 host evidence"

    $validationLines = @(& (Join-Path $PSScriptRoot "validate-renderer-commercial-readiness-evidence.ps1") `
            -ArtifactRootRelative $collectorOutputRootRelative)
    $validationValues = ConvertFrom-KeyValueLines -Lines $validationLines
    if ([string]$validationValues["renderer_d3d12_renderer_quality_ready"] -ne "1") {
        Write-Error "generated D3D12 host evidence must validate as renderer_d3d12_renderer_quality_ready."
    }
    if ([string]$validationValues["renderer_commercial_readiness"] -ne "0") {
        Write-Error "D3D12-only host evidence generation must not promote renderer_commercial_readiness."
    }
}
finally {
    foreach ($path in @($evidenceRootPath, $commercialRootPath)) {
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -Recurse -Force
        }
    }
}

Write-Information "renderer-d3d12-commercial-quality-host-evidence-check: ok" -InformationAction Continue
