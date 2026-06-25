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

function New-QualityVfxHostEvidence {
    param([Parameter(Mandatory = $true)][bool]$FixtureOnly)

    return [ordered]@{
        schema_version = "GameEngine.RendererQualityVfxCommercialHostEvidence.v1"
        claim_id = "renderer-quality-vfx-commercial-artifacts-v1"
        validation_recipe = "renderer-quality-vfx-commercial-artifacts"
        fixture_only = $FixtureOnly
        source_id = "GameEngine-Renderer-Quality-Vfx-Profiling-2026-06-25"
        quality_rows = [ordered]@{
            renderer_quality_matrix = [ordered]@{
                validation_recipe = "renderer-quality-matrix"
                proof_rows = [ordered]@{
                    matrix_status = [ordered]@{
                        ready = $true
                        renderer_quality_matrix_status = "host_evidence_required"
                        d3d12_renderer_quality_ready = $true
                        vulkan_strict_renderer_quality_ready = $true
                        apple_metal_host_evidence_supplied = $true
                        general_renderer_quality_claim = $false
                    }
                    side_effect_policy = [ordered]@{
                        ready = $true
                        gpu_command_side_effects = 0
                        native_capture_side_effects = 0
                        crash_upload_side_effects = 0
                    }
                    replay = [ordered]@{
                        ready = $true
                        deterministic_replay_hash_sha256 = "11efb7abda26952d0c620bd171949269bdf4d0fc14e4d35a7e2a2ec1ba6bc99f"
                    }
                    diagnostics = [ordered]@{
                        ready = $true
                        diagnostic_error_count = 0
                    }
                }
                validation_counters = [ordered]@{
                    renderer_quality_matrix_ready = $true
                    renderer_quality_matrix_general_renderer_quality_claim = $false
                    renderer_quality_matrix_gpu_command_side_effects = 0
                    renderer_quality_matrix_native_capture_side_effects = 0
                    renderer_quality_matrix_crash_upload_side_effects = 0
                }
                non_claims = [ordered]@{
                    broad_renderer_quality = $false
                    renderer_commercial_readiness = $false
                    external_engine_parity = $false
                    native_handles_exposed = $false
                    gpu_command_side_effects = $false
                    native_capture_side_effects = $false
                    crash_upload_side_effects = $false
                }
            }
            production_vfx_profiling = [ordered]@{
                validation_recipe = "renderer-production-vfx-profiling"
                proof_rows = [ordered]@{
                    vfx_profiling_review = [ordered]@{
                        ready = $true
                        rendering_vfx_profiling_reviewed = $true
                        d3d12_renderer_quality_ready = $true
                        vulkan_strict_renderer_quality_ready = $true
                        apple_metal_host_evidence_supplied = $true
                    }
                    debug_policy = [ordered]@{
                        ready = $true
                        debug_capture_policy_recorded = $true
                        debug_upload_policy_recorded = $true
                        crash_upload_policy_recorded = $true
                    }
                    memory_policy = [ordered]@{
                        ready = $true
                        memory_residency_policy_recorded = $true
                        metal_memory_profiling_evidence_supplied = $true
                    }
                    package_counters = [ordered]@{
                        ready = $true
                        visible_3d_package_ready = $true
                        runtime_ui_package_ready = $true
                        environment_package_ready = $true
                        generated_game_package_ready = $true
                    }
                    replay = [ordered]@{
                        ready = $true
                        deterministic_replay_hash_sha256 = "2282db5ec41d758f4b3ee0a43c05cc3e0a353ae7f7d15d508f8c98032dcc27b2"
                    }
                    side_effect_policy = [ordered]@{
                        ready = $true
                        native_capture_side_effects = 0
                        crash_upload_side_effects = 0
                        retained_official_profiler_artifact_selected = $false
                    }
                }
                validation_counters = [ordered]@{
                    renderer_production_vfx_profiling_ready = $true
                    rendering_vfx_profiling_reviewed = $true
                    renderer_production_vfx_native_capture_side_effects = 0
                    renderer_production_vfx_crash_upload_side_effects = 0
                }
                non_claims = [ordered]@{
                    broad_renderer_quality = $false
                    renderer_commercial_readiness = $false
                    external_engine_parity = $false
                    native_handles_exposed = $false
                    native_capture_side_effects = $false
                    crash_upload_side_effects = $false
                    retained_official_profiler_artifact_selected = $false
                }
            }
        }
        non_claims = [ordered]@{
            broad_renderer_quality = $false
            renderer_commercial_readiness = $false
            external_engine_parity = $false
            native_handles_exposed = $false
            gpu_command_side_effects = $false
            native_capture_side_effects = $false
            crash_upload_side_effects = $false
        }
    }
}

$producerScript = Join-Path $root "tools/collect-renderer-quality-vfx-commercial-artifacts.ps1"
if (-not (Test-Path -LiteralPath $producerScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-quality-vfx-commercial-artifacts.ps1 must exist for Task 10D quality/VFX artifact production."
}

$collectorScript = Join-Path $root "tools/collect-renderer-commercial-readiness-evidence.ps1"
if (-not (Test-Path -LiteralPath $collectorScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-commercial-readiness-evidence.ps1 must exist for Task 10 retained artifact assembly."
}

$fixtureRoot = "tests/fixtures/renderer/commercial-readiness-evidence/ready"
$evidenceRootRelative = "artifacts/renderer/commercial-readiness-evidence/quality-vfx-commercial-contract-$PID"
$inputRootRelative = "$evidenceRootRelative/input"
$producerOutputRootRelative = "$evidenceRootRelative/producer-output"
$collectorOutputRootRelative = "$evidenceRootRelative/collector-output"
$realInputRelative = "$inputRootRelative/quality-vfx-host-evidence.json"
$fixtureInputRelative = "$inputRootRelative/quality-vfx-fixture-evidence.json"
$evidenceRootPath = ConvertTo-LocalPath $evidenceRootRelative

try {
    if (Test-Path -LiteralPath $evidenceRootPath) {
        Remove-Item -LiteralPath $evidenceRootPath -Recurse -Force
    }

    Write-JsonObject -Path (ConvertTo-LocalPath $realInputRelative) `
        -Value (New-QualityVfxHostEvidence -FixtureOnly $false)
    Write-JsonObject -Path (ConvertTo-LocalPath $fixtureInputRelative) `
        -Value (New-QualityVfxHostEvidence -FixtureOnly $true)

    $planLines = @(& $producerScript -Mode Plan -OutputRootRelative $producerOutputRootRelative)
    foreach ($expectedLine in @(
            "renderer_quality_vfx_commercial_artifacts_collector_mode=Plan",
            "renderer_quality_vfx_commercial_artifacts_written=0",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        )) {
        Assert-LinePresent $planLines $expectedLine "quality/VFX artifact producer Plan mode"
    }

    $unsafeRejected = $false
    try {
        $null = & $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -QualityVfxHostEvidenceRelative "../unsafe.json" 2>&1
    }
    catch {
        $unsafeRejected = [string]$_.Exception.Message -like "*unsafe_relative_path*"
    }
    if (-not $unsafeRejected) {
        Write-Error "quality/VFX artifact producer must reject unsafe relative paths."
    }

    $fixtureRejected = $false
    try {
        $null = & $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -QualityVfxHostEvidenceRelative $fixtureInputRelative 2>&1
    }
    catch {
        $fixtureRejected = [string]$_.Exception.Message -like "*fixture_artifact_input_rejected*"
    }
    if (-not $fixtureRejected) {
        Write-Error "quality/VFX artifact producer must reject fixture-only host evidence."
    }

    $assembleLines = @(& $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -QualityVfxHostEvidenceRelative $realInputRelative)
    foreach ($expectedLine in @(
            "renderer_quality_vfx_commercial_artifacts_collector_mode=Assemble",
            "renderer_quality_vfx_commercial_artifacts_written=2",
            "renderer_quality_vfx_commercial_fixture_artifact=0",
            "renderer_quality_matrix_ready=1",
            "renderer_quality_matrix_status=host_evidence_required",
            "rendering_vfx_profiling_reviewed=1",
            "renderer_production_vfx_profiling_ready=1",
            "renderer_quality_matrix_gpu_command_side_effects=0",
            "renderer_quality_matrix_native_capture_side_effects=0",
            "renderer_quality_matrix_crash_upload_side_effects=0",
            "renderer_production_vfx_native_capture_side_effects=0",
            "renderer_production_vfx_crash_upload_side_effects=0",
            "renderer_backend_parity_ready=0",
            "renderer_metal_broad_readiness=0",
            "renderer_broad_quality_ready=0",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        )) {
        Assert-LinePresent $assembleLines $expectedLine "quality/VFX artifact producer Assemble mode"
    }

    foreach ($artifactSpec in @(
            @{ Path = "renderer-quality-matrix.json"; ArtifactId = "renderer-quality-matrix"; Recipe = "renderer-quality-matrix" },
            @{ Path = "production-vfx-profiling.json"; ArtifactId = "production-vfx-profiling"; Recipe = "renderer-production-vfx-profiling" }
        )) {
        $artifactPath = ConvertTo-LocalPath "$producerOutputRootRelative/$($artifactSpec.Path)"
        if (-not (Test-Path -LiteralPath $artifactPath -PathType Leaf)) {
            Write-Error "quality/VFX artifact producer did not write $($artifactSpec.Path)."
        }
        $artifact = Get-Content -LiteralPath $artifactPath -Raw | ConvertFrom-Json
        if ([string]$artifact.schema_version -ne "GameEngine.RendererCommercialQualityCloseout.v1") {
            Write-Error "$($artifactSpec.Path) schema_version mismatch."
        }
        if ([string]$artifact.artifact_id -ne [string]$artifactSpec.ArtifactId) {
            Write-Error "$($artifactSpec.Path) artifact_id mismatch."
        }
        if ([string]$artifact.validation_recipe -ne [string]$artifactSpec.Recipe) {
            Write-Error "$($artifactSpec.Path) validation_recipe mismatch."
        }
        if ([bool]$artifact.fixture_only) {
            Write-Error "$($artifactSpec.Path) must not be fixture_only when quality/VFX evidence is retained real evidence."
        }
        if ([bool]$artifact.non_claims.external_engine_parity) {
            Write-Error "$($artifactSpec.Path) must not claim external engine parity."
        }
        if ([bool]$artifact.non_claims.native_handles_exposed) {
            Write-Error "$($artifactSpec.Path) must keep native handles unexposed."
        }
    }

    $collectorArguments = @{
        OutputRootRelative = $collectorOutputRootRelative
        D3d12ArtifactRelative = "$fixtureRoot/d3d12-quality.json"
        VulkanStrictArtifactRelative = "$fixtureRoot/vulkan-strict-quality.json"
        AppleMetalArtifactRelative = "$fixtureRoot/apple-metal-host.json"
        Visible3dPackageArtifactRelative = "$fixtureRoot/visible-3d-package.json"
        RuntimeUiPackageArtifactRelative = "$fixtureRoot/runtime-ui-package.json"
        EnvironmentPackageArtifactRelative = "$fixtureRoot/environment-package.json"
        GeneratedGamePackageArtifactRelative = "$fixtureRoot/generated-game-package.json"
        RendererQualityMatrixArtifactRelative = "$producerOutputRootRelative/renderer-quality-matrix.json"
        ProductionVfxProfilingArtifactRelative = "$producerOutputRootRelative/production-vfx-profiling.json"
        MetalMemoryResidencyArtifactRelative = "$fixtureRoot/metal-memory-residency.json"
        MetalProfilingCaptureArtifactRelative = "$fixtureRoot/metal-profiling-capture.json"
        OfficialDocsOnlyReviewReady = $true
        LegalReviewReady = $true
        ExternalEngineZeroMaterialReviewReady = $true
        ThirdPartyNoticesComplete = $true
    }
    $collectorLines = @(& $collectorScript -Mode Assemble @collectorArguments -AllowFixtureArtifactsForSelfTest)
    Assert-LinePresent $collectorLines `
        "renderer_commercial_readiness_evidence_collector_fixture_artifacts=9" `
        "commercial readiness collector with retained quality/VFX artifacts"
    Assert-LinePresent $collectorLines `
        "renderer_commercial_readiness_evidence_collector_real_promotion_candidate=0" `
        "commercial readiness collector with retained quality/VFX artifacts"

    $validationLines = @(& (Join-Path $PSScriptRoot "validate-renderer-commercial-readiness-evidence.ps1") `
            -ArtifactRootRelative $collectorOutputRootRelative)
    $validationValues = ConvertFrom-KeyValueLines -Lines $validationLines
    foreach ($counter in @(
            "renderer_quality_matrix_ready",
            "renderer_production_vfx_profiling_ready"
        )) {
        if ([string]$validationValues[$counter] -ne "1") {
            Write-Error "retained quality/VFX artifacts must validate $counter=1."
        }
    }
    if ([string]$validationValues["renderer_commercial_readiness_fixture_artifacts_rejected"] -ne "9") {
        Write-Error "collector output must reject only the remaining fixture artifacts."
    }
    if ([string]$validationValues["renderer_broad_quality_ready"] -ne "0") {
        Write-Error "quality/VFX artifact producer must not promote broad renderer quality."
    }
    if ([string]$validationValues["renderer_commercial_readiness"] -ne "0") {
        Write-Error "quality/VFX-only producer must not promote renderer_commercial_readiness."
    }
}
finally {
    if (Test-Path -LiteralPath $evidenceRootPath) {
        Remove-Item -LiteralPath $evidenceRootPath -Recurse -Force
    }
}

Write-Information "renderer-quality-vfx-commercial-artifacts-check: ok" -InformationAction Continue
