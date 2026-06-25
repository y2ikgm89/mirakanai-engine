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

function New-VulkanHostEvidence {
    param([Parameter(Mandatory = $true)][bool]$FixtureOnly)

    return [ordered]@{
        schema_version = "GameEngine.RendererVulkanStrictCommercialQualityHostEvidence.v1"
        claim_id = "renderer-vulkan-strict-commercial-quality-artifact-v1"
        validation_recipe = "renderer-vulkan-strict-quality-evidence"
        fixture_only = $FixtureOnly
        source_id = "Khronos-Vulkan-Synchronization2-Memory-QueueOwnership-2026-06-25"
        proof_rows = [ordered]@{
            synchronization2 = [ordered]@{
                ready = $true
                vk_cmd_pipeline_barrier2_recorded = $true
                vk_dependency_info_recorded = $true
                api_name = "vkCmdPipelineBarrier2"
                structure_name = "VkDependencyInfo"
            }
            validation_layer = [ordered]@{
                ready = $true
                layer_name = "VK_LAYER_KHRONOS_validation"
                validation_log_clean = $true
                validation_error_count = 0
            }
            synchronization_validation = [ordered]@{
                ready = $true
                sync_validation_enabled = $true
                sync_validation_error_count = 0
            }
            memory_binding = [ordered]@{
                ready = $true
                buffer_memory_bound = $true
                image_memory_bound = $true
                vuid_constraints_checked = $true
                vuid_reference = "VUID-vkBindBufferMemory-memory-01030"
            }
            timestamp_query = [ordered]@{
                ready = $true
                query_pool_timestamp = $true
                timestamps_resolved = $true
                timestamp_period_ns = 0.5
            }
            spirv_shader_validation = [ordered]@{
                ready = $true
                spirv_val_ready = $true
                shader_modules_validated = $true
                validation_error_count = 0
            }
            package_visible_readback = [ordered]@{
                ready = $true
                deterministic_hash_sha256 = "7f86d7be87b48d9a93f56d4a2f072c435f96a8364eb88c1a91fa6bda38f8d94b"
                readback_counter_rows = 1
                package_counter_id = "renderer_vulkan_package_visible_readback"
            }
            native_handles = [ordered]@{
                ready = $true
                native_handles_exposed = $false
            }
        }
        validation_counters = [ordered]@{
            renderer_vulkan_synchronization2_ready = $true
            renderer_vulkan_validation_layer_ready = $true
            renderer_vulkan_sync_validation_ready = $true
            renderer_vulkan_memory_binding_ready = $true
            renderer_vulkan_timestamp_ready = $true
            renderer_vulkan_shader_validation_ready = $true
            renderer_vulkan_package_readback_ready = $true
        }
        non_claims = [ordered]@{
            d3d12_inferred = $false
            metal_inferred = $false
            debugging_only_full_pipeline_barrier = $false
            environment_ready = $false
            external_engine_parity = $false
            native_handles_exposed = $false
        }
    }
}

$producerScript = Join-Path $root "tools/collect-renderer-vulkan-strict-commercial-quality-artifact.ps1"
if (-not (Test-Path -LiteralPath $producerScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-vulkan-strict-commercial-quality-artifact.ps1 must exist for Task 10B retained strict Vulkan artifact production."
}

$collectorScript = Join-Path $root "tools/collect-renderer-commercial-readiness-evidence.ps1"
if (-not (Test-Path -LiteralPath $collectorScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-commercial-readiness-evidence.ps1 must exist for Task 10 retained artifact assembly."
}

$fixtureRoot = "tests/fixtures/renderer/commercial-readiness-evidence/ready"
$evidenceRootRelative = "artifacts/renderer/commercial-readiness-evidence/vulkan-strict-commercial-quality-contract-$PID"
$inputRootRelative = "$evidenceRootRelative/input"
$producerOutputRootRelative = "$evidenceRootRelative/producer-output"
$collectorOutputRootRelative = "$evidenceRootRelative/collector-output"
$realInputRelative = "$inputRootRelative/vulkan-strict-host-evidence.json"
$fixtureInputRelative = "$inputRootRelative/vulkan-strict-fixture-evidence.json"
$evidenceRootPath = ConvertTo-LocalPath $evidenceRootRelative

try {
    if (Test-Path -LiteralPath $evidenceRootPath) {
        Remove-Item -LiteralPath $evidenceRootPath -Recurse -Force
    }

    Write-JsonObject `
        -Path (ConvertTo-LocalPath $realInputRelative) `
        -Value (New-VulkanHostEvidence -FixtureOnly $false)
    Write-JsonObject `
        -Path (ConvertTo-LocalPath $fixtureInputRelative) `
        -Value (New-VulkanHostEvidence -FixtureOnly $true)

    $planLines = @(& $producerScript -Mode Plan -OutputRootRelative $producerOutputRootRelative)
    Assert-LinePresent $planLines `
        "renderer_vulkan_strict_commercial_quality_artifact_collector_mode=Plan" `
        "strict Vulkan artifact producer Plan mode"
    Assert-LinePresent $planLines `
        "renderer_vulkan_strict_commercial_quality_artifact_written=0" `
        "strict Vulkan artifact producer Plan mode"
    Assert-LinePresent $planLines `
        "renderer_commercial_readiness=0" `
        "strict Vulkan artifact producer Plan mode"
    Assert-LinePresent $planLines `
        "renderer_environment_ready=0" `
        "strict Vulkan artifact producer Plan mode"

    $unsafeRejected = $false
    try {
        $null = & $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -VulkanStrictHostEvidenceRelative "../unsafe.json" 2>&1
    }
    catch {
        $unsafeRejected = [string]$_.Exception.Message -like "*unsafe_relative_path*"
    }
    if (-not $unsafeRejected) {
        Write-Error "strict Vulkan artifact producer must reject unsafe relative paths."
    }

    $fixtureRejected = $false
    try {
        $null = & $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -VulkanStrictHostEvidenceRelative $fixtureInputRelative 2>&1
    }
    catch {
        $fixtureRejected = [string]$_.Exception.Message -like "*fixture_artifact_input_rejected*"
    }
    if (-not $fixtureRejected) {
        Write-Error "strict Vulkan artifact producer must reject fixture-only host evidence."
    }

    $assembleLines = @(& $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -VulkanStrictHostEvidenceRelative $realInputRelative)
    Assert-LinePresent $assembleLines `
        "renderer_vulkan_strict_commercial_quality_artifact_collector_mode=Assemble" `
        "strict Vulkan artifact producer Assemble mode"
    Assert-LinePresent $assembleLines `
        "renderer_vulkan_strict_commercial_quality_artifact_written=1" `
        "strict Vulkan artifact producer Assemble mode"
    Assert-LinePresent $assembleLines `
        "renderer_vulkan_strict_commercial_quality_fixture_artifact=0" `
        "strict Vulkan artifact producer Assemble mode"
    foreach ($expectedLine in @(
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
        Assert-LinePresent $assembleLines $expectedLine "strict Vulkan artifact producer Assemble mode"
    }

    $artifactPath = ConvertTo-LocalPath "$producerOutputRootRelative/vulkan-strict-quality.json"
    if (-not (Test-Path -LiteralPath $artifactPath -PathType Leaf)) {
        Write-Error "strict Vulkan artifact producer did not write vulkan-strict-quality.json."
    }
    $artifact = Get-Content -LiteralPath $artifactPath -Raw | ConvertFrom-Json
    if ([string]$artifact.schema_version -ne "GameEngine.RendererCommercialQualityCloseout.v1") {
        Write-Error "strict Vulkan artifact schema_version mismatch."
    }
    if ([string]$artifact.artifact_id -ne "vulkan-strict-quality") {
        Write-Error "strict Vulkan artifact_id mismatch."
    }
    if ([bool]$artifact.fixture_only) {
        Write-Error "strict Vulkan producer output must not be fixture_only when host evidence is retained real evidence."
    }
    if ([bool]$artifact.non_claims.external_engine_parity) {
        Write-Error "strict Vulkan producer output must not claim external engine parity."
    }
    if ([bool]$artifact.non_claims.debugging_only_full_pipeline_barrier) {
        Write-Error "strict Vulkan producer output must reject debugging-only full pipeline barriers."
    }
    if ([bool]$artifact.proof_rows.native_handles.native_handles_exposed) {
        Write-Error "strict Vulkan producer output must keep native handles unexposed."
    }

    $collectorArguments = @{
        OutputRootRelative = $collectorOutputRootRelative
        D3d12ArtifactRelative = "$fixtureRoot/d3d12-quality.json"
        VulkanStrictArtifactRelative = "$producerOutputRootRelative/vulkan-strict-quality.json"
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
        "commercial readiness collector with retained strict Vulkan artifact"
    Assert-LinePresent $collectorLines `
        "renderer_commercial_readiness_evidence_collector_real_promotion_candidate=0" `
        "commercial readiness collector with retained strict Vulkan artifact"

    $validationLines = @(& (Join-Path $PSScriptRoot "validate-renderer-commercial-readiness-evidence.ps1") `
            -ArtifactRootRelative $collectorOutputRootRelative)
    $validationValues = ConvertFrom-KeyValueLines -Lines $validationLines
    if ([string]$validationValues["renderer_vulkan_strict_renderer_quality_ready"] -ne "1") {
        Write-Error "retained strict Vulkan artifact must validate as renderer_vulkan_strict_renderer_quality_ready."
    }
    if ([string]$validationValues["renderer_commercial_readiness_fixture_artifacts_rejected"] -ne "10") {
        Write-Error "collector output must reject only the remaining fixture artifacts."
    }
    if ([string]$validationValues["renderer_commercial_readiness"] -ne "0") {
        Write-Error "retained strict Vulkan-only producer must not promote renderer_commercial_readiness."
    }
}
finally {
    if (Test-Path -LiteralPath $evidenceRootPath) {
        Remove-Item -LiteralPath $evidenceRootPath -Recurse -Force
    }
}

Write-Information "renderer-vulkan-strict-commercial-quality-artifact-check: ok" -InformationAction Continue
