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

function New-D3d12HostEvidence {
    param([Parameter(Mandatory = $true)][bool]$FixtureOnly)

    return [ordered]@{
        schema_version = "GameEngine.RendererD3d12CommercialQualityHostEvidence.v1"
        claim_id = "renderer-d3d12-commercial-quality-artifact-v1"
        validation_recipe = "renderer-d3d12-quality-evidence"
        fixture_only = $FixtureOnly
        source_id = "Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25"
        proof_rows = [ordered]@{
            command_allocator_list_fence = [ordered]@{
                ready = $true
                command_allocator_reuse_fenced = $true
                command_list_closed_before_execute = $true
                fence_signal_wait_recorded = $true
                fence_api_name = "ID3D12Fence"
            }
            resource_barriers = [ordered]@{
                ready = $true
                render_transition_explicit = $true
                copy_transition_explicit = $true
                unordered_access_barrier_explicit = $true
                readback_transition_explicit = $true
                resource_barrier_api_name = "D3D12_RESOURCE_BARRIER"
            }
            timestamp = [ordered]@{
                ready = $true
                query_type = "D3D12_QUERY_TYPE_TIMESTAMP"
                resolved_query_data = $true
                queue_frequency_hz = 1000000000
                clock_calibration = $true
            }
            debug_validation = [ordered]@{
                ready = $true
                debug_layer_or_gpu_based_validation_clean = $true
                debug_message_count = 0
                gpu_based_validation_message_count = 0
            }
            residency = [ordered]@{
                ready = $true
                video_memory_budget_queried = $true
                make_resident_or_budget_recorded = $true
                residency_api_name = "ID3D12Device3::EnqueueMakeResident"
                budget_api_name = "IDXGIAdapter3::QueryVideoMemoryInfo"
            }
            package_visible_readback = [ordered]@{
                ready = $true
                deterministic_hash_sha256 = "4c5d0a311d81e9fb91938f124890cb8456b744a6129da1b2ce2f6ff3954494f2"
                readback_counter_rows = 1
                package_counter_id = "renderer_d3d12_package_visible_readback"
            }
            native_handles = [ordered]@{
                ready = $true
                native_handles_exposed = $false
            }
        }
        validation_counters = [ordered]@{
            renderer_d3d12_command_allocator_fence_ready = $true
            renderer_d3d12_resource_barrier_ready = $true
            renderer_d3d12_timestamp_ready = $true
            renderer_d3d12_debug_validation_ready = $true
            renderer_d3d12_residency_ready = $true
            renderer_d3d12_package_readback_ready = $true
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

$producerScript = Join-Path $root "tools/collect-renderer-d3d12-commercial-quality-artifact.ps1"
if (-not (Test-Path -LiteralPath $producerScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-d3d12-commercial-quality-artifact.ps1 must exist for Task 10A retained D3D12 artifact production."
}

$collectorScript = Join-Path $root "tools/collect-renderer-commercial-readiness-evidence.ps1"
if (-not (Test-Path -LiteralPath $collectorScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-commercial-readiness-evidence.ps1 must exist for Task 10 retained artifact assembly."
}

$fixtureRoot = "tests/fixtures/renderer/commercial-readiness-evidence/ready"
$evidenceRootRelative = "artifacts/renderer/commercial-readiness-evidence/d3d12-commercial-quality-contract-$PID"
$inputRootRelative = "$evidenceRootRelative/input"
$producerOutputRootRelative = "$evidenceRootRelative/producer-output"
$collectorOutputRootRelative = "$evidenceRootRelative/collector-output"
$realInputRelative = "$inputRootRelative/d3d12-host-evidence.json"
$fixtureInputRelative = "$inputRootRelative/d3d12-fixture-evidence.json"
$evidenceRootPath = ConvertTo-LocalPath $evidenceRootRelative

try {
    if (Test-Path -LiteralPath $evidenceRootPath) {
        Remove-Item -LiteralPath $evidenceRootPath -Recurse -Force
    }

    Write-JsonObject `
        -Path (ConvertTo-LocalPath $realInputRelative) `
        -Value (New-D3d12HostEvidence -FixtureOnly $false)
    Write-JsonObject `
        -Path (ConvertTo-LocalPath $fixtureInputRelative) `
        -Value (New-D3d12HostEvidence -FixtureOnly $true)

    $planLines = @(& $producerScript -Mode Plan -OutputRootRelative $producerOutputRootRelative)
    Assert-LinePresent $planLines `
        "renderer_d3d12_commercial_quality_artifact_collector_mode=Plan" `
        "D3D12 artifact producer Plan mode"
    Assert-LinePresent $planLines `
        "renderer_d3d12_commercial_quality_artifact_written=0" `
        "D3D12 artifact producer Plan mode"
    Assert-LinePresent $planLines `
        "renderer_commercial_readiness=0" `
        "D3D12 artifact producer Plan mode"
    Assert-LinePresent $planLines `
        "renderer_environment_ready=0" `
        "D3D12 artifact producer Plan mode"

    $unsafeRejected = $false
    try {
        $null = & $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -D3d12HostEvidenceRelative "../unsafe.json" 2>&1
    }
    catch {
        $unsafeRejected = [string]$_.Exception.Message -like "*unsafe_relative_path*"
    }
    if (-not $unsafeRejected) {
        Write-Error "D3D12 artifact producer must reject unsafe relative paths."
    }

    $fixtureRejected = $false
    try {
        $null = & $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -D3d12HostEvidenceRelative $fixtureInputRelative 2>&1
    }
    catch {
        $fixtureRejected = [string]$_.Exception.Message -like "*fixture_artifact_input_rejected*"
    }
    if (-not $fixtureRejected) {
        Write-Error "D3D12 artifact producer must reject fixture-only host evidence."
    }

    $assembleLines = @(& $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -D3d12HostEvidenceRelative $realInputRelative)
    Assert-LinePresent $assembleLines `
        "renderer_d3d12_commercial_quality_artifact_collector_mode=Assemble" `
        "D3D12 artifact producer Assemble mode"
    Assert-LinePresent $assembleLines `
        "renderer_d3d12_commercial_quality_artifact_written=1" `
        "D3D12 artifact producer Assemble mode"
    Assert-LinePresent $assembleLines `
        "renderer_d3d12_commercial_quality_fixture_artifact=0" `
        "D3D12 artifact producer Assemble mode"
    Assert-LinePresent $assembleLines `
        "renderer_d3d12_command_allocator_fence_ready=1" `
        "D3D12 artifact producer Assemble mode"
    Assert-LinePresent $assembleLines `
        "renderer_d3d12_resource_barrier_ready=1" `
        "D3D12 artifact producer Assemble mode"
    Assert-LinePresent $assembleLines `
        "renderer_d3d12_timestamp_ready=1" `
        "D3D12 artifact producer Assemble mode"
    Assert-LinePresent $assembleLines `
        "renderer_d3d12_debug_validation_ready=1" `
        "D3D12 artifact producer Assemble mode"
    Assert-LinePresent $assembleLines `
        "renderer_d3d12_residency_ready=1" `
        "D3D12 artifact producer Assemble mode"
    Assert-LinePresent $assembleLines `
        "renderer_d3d12_package_readback_ready=1" `
        "D3D12 artifact producer Assemble mode"
    Assert-LinePresent $assembleLines `
        "renderer_d3d12_native_handles_exposed=0" `
        "D3D12 artifact producer Assemble mode"
    Assert-LinePresent $assembleLines `
        "renderer_commercial_readiness=0" `
        "D3D12 artifact producer Assemble mode"
    Assert-LinePresent $assembleLines `
        "renderer_environment_ready=0" `
        "D3D12 artifact producer Assemble mode"

    $artifactPath = ConvertTo-LocalPath "$producerOutputRootRelative/d3d12-quality.json"
    if (-not (Test-Path -LiteralPath $artifactPath -PathType Leaf)) {
        Write-Error "D3D12 artifact producer did not write d3d12-quality.json."
    }
    $artifact = Get-Content -LiteralPath $artifactPath -Raw | ConvertFrom-Json
    if ([string]$artifact.schema_version -ne "GameEngine.RendererCommercialQualityCloseout.v1") {
        Write-Error "D3D12 artifact schema_version mismatch."
    }
    if ([string]$artifact.artifact_id -ne "d3d12-quality") {
        Write-Error "D3D12 artifact_id mismatch."
    }
    if ([bool]$artifact.fixture_only) {
        Write-Error "D3D12 producer output must not be fixture_only when host evidence is retained real evidence."
    }
    if ([bool]$artifact.non_claims.external_engine_parity) {
        Write-Error "D3D12 producer output must not claim external engine parity."
    }
    if ([bool]$artifact.proof_rows.native_handles.native_handles_exposed) {
        Write-Error "D3D12 producer output must keep native handles unexposed."
    }

    $collectorArguments = @{
        OutputRootRelative = $collectorOutputRootRelative
        D3d12ArtifactRelative = "$producerOutputRootRelative/d3d12-quality.json"
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
    $collectorLines = @(& $collectorScript -Mode Assemble @collectorArguments -AllowFixtureArtifactsForSelfTest)
    Assert-LinePresent $collectorLines `
        "renderer_commercial_readiness_evidence_collector_fixture_artifacts=10" `
        "commercial readiness collector with retained D3D12 artifact"
    Assert-LinePresent $collectorLines `
        "renderer_commercial_readiness_evidence_collector_real_promotion_candidate=0" `
        "commercial readiness collector with retained D3D12 artifact"

    $validationLines = @(& (Join-Path $PSScriptRoot "validate-renderer-commercial-readiness-evidence.ps1") `
            -ArtifactRootRelative $collectorOutputRootRelative)
    $validationValues = ConvertFrom-KeyValueLines -Lines $validationLines
    if ([string]$validationValues["renderer_d3d12_renderer_quality_ready"] -ne "1") {
        Write-Error "retained D3D12 artifact must validate as renderer_d3d12_renderer_quality_ready."
    }
    if ([string]$validationValues["renderer_commercial_readiness_fixture_artifacts_rejected"] -ne "10") {
        Write-Error "collector output must reject only the remaining fixture artifacts."
    }
    if ([string]$validationValues["renderer_commercial_readiness"] -ne "0") {
        Write-Error "retained D3D12-only producer must not promote renderer_commercial_readiness."
    }
}
finally {
    if (Test-Path -LiteralPath $evidenceRootPath) {
        Remove-Item -LiteralPath $evidenceRootPath -Recurse -Force
    }
}

Write-Information "renderer-d3d12-commercial-quality-artifact-check: ok" -InformationAction Continue
