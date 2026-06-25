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

function New-PackageHostEvidence {
    param([Parameter(Mandatory = $true)][bool]$FixtureOnly)

    return [ordered]@{
        schema_version = "GameEngine.RendererPackageCommercialQualityHostEvidence.v1"
        claim_id = "renderer-package-commercial-quality-artifacts-v1"
        validation_recipe = "renderer-package-commercial-quality-artifacts"
        fixture_only = $FixtureOnly
        source_id = "GameEngine-Renderer-Package-Commercial-Quality-2026-06-25"
        package_rows = [ordered]@{
            visible_3d = [ordered]@{
                validation_recipe = "desktop-3d-package"
                proof_rows = [ordered]@{
                    material_render = [ordered]@{
                        ready = $true
                        pbr_material_row = $true
                        texture_binding_row = $true
                        material_variant_rows = 1
                    }
                    lighting_row = [ordered]@{
                        ready = $true
                        direct_light_row = $true
                        ambient_light_row = $true
                        lighting_readback_nonzero = $true
                    }
                    shadow_postprocess = [ordered]@{
                        ready = $true
                        shadow_or_depth_row = $true
                        postprocess_row = $true
                        tone_mapping_row = $true
                    }
                    package_visible_readback = [ordered]@{
                        ready = $true
                        deterministic_hash_sha256 = "e55076775a63e556bece9ce3bb2dfe4f74641c4bbb48e5606f3323e9813a25b7"
                        readback_counter_rows = 1
                    }
                    manifest_binding = [ordered]@{
                        ready = $true
                        game_agent_manifest_row = $true
                        validation_recipe_id = "desktop-3d-package"
                        package_manifest_row = "sample_generated_desktop_runtime_3d_package"
                    }
                }
                validation_counters = [ordered]@{
                    renderer_visible_3d_material_ready = $true
                    renderer_visible_3d_lighting_ready = $true
                    renderer_visible_3d_shadow_postprocess_ready = $true
                    renderer_visible_3d_readback_hash_ready = $true
                    renderer_package_arbitrary_script_execution = $false
                    renderer_package_script_execution = $false
                }
                non_claims = [ordered]@{
                    arbitrary_script_execution = $false
                    package_script_execution = $false
                    native_handles_exposed = $false
                    external_engine_parity = $false
                    environment_ready = $false
                }
            }
            runtime_ui = [ordered]@{
                validation_recipe = "desktop-runtime-ui-package"
                proof_rows = [ordered]@{
                    ui_atlas_upload = [ordered]@{
                        ready = $true
                        atlas_texture_upload_row = $true
                        atlas_texture_usage_sampled = $true
                        upload_counter_rows = 1
                    }
                    ui_atlas_readback = [ordered]@{
                        ready = $true
                        readback_counter_rows = 1
                        deterministic_hash_sha256 = "8a6cf68c576f8f89bd1f2d7df9d35f7c80dbd6f5866222737345e222b7fb5c8d"
                    }
                    renderer_handoff = [ordered]@{
                        ready = $true
                        retained_upload_handoff_row = $true
                        renderer_consumed_ui_atlas_row = $true
                    }
                    manifest_binding = [ordered]@{
                        ready = $true
                        game_agent_manifest_row = $true
                        validation_recipe_id = "desktop-runtime-ui-package"
                        package_manifest_row = "sample_2d_desktop_runtime_package"
                    }
                }
                validation_counters = [ordered]@{
                    renderer_runtime_ui_atlas_upload_ready = $true
                    renderer_runtime_ui_atlas_readback_ready = $true
                    renderer_runtime_ui_handoff_ready = $true
                    renderer_package_arbitrary_script_execution = $false
                    renderer_package_script_execution = $false
                }
                non_claims = [ordered]@{
                    arbitrary_script_execution = $false
                    package_script_execution = $false
                    native_handles_exposed = $false
                    external_engine_parity = $false
                    environment_ready = $false
                }
            }
            environment = [ordered]@{
                validation_recipe = "environment-package"
                proof_rows = [ordered]@{
                    environment_renderer_package_consumption = [ordered]@{
                        ready = $true
                        environment_package_row_consumed = $true
                        renderer_environment_rows_consumed_count = 4
                        environment_ready_promoted = $false
                    }
                    manifest_binding = [ordered]@{
                        ready = $true
                        game_agent_manifest_row = $true
                        validation_recipe_id = "environment-package"
                        package_manifest_row = "environment_renderer_package"
                    }
                }
                validation_counters = [ordered]@{
                    renderer_environment_package_consumption_ready = $true
                    renderer_environment_ready_promoted = $false
                    renderer_package_arbitrary_script_execution = $false
                    renderer_package_script_execution = $false
                }
                non_claims = [ordered]@{
                    arbitrary_script_execution = $false
                    package_script_execution = $false
                    native_handles_exposed = $false
                    external_engine_parity = $false
                    environment_ready = $false
                }
            }
            generated_game = [ordered]@{
                validation_recipe = "generated-game-package"
                proof_rows = [ordered]@{
                    generated_game_output = [ordered]@{
                        ready = $true
                        generated_game_package_written = $true
                        output_manifest_rows = 1
                        deterministic_hash_sha256 = "b8ec9f6e45f0c36bbf4c5b9bfd31e2c8ad4754e6f2e53f4419bd116080e24f3a"
                    }
                    manifest_binding = [ordered]@{
                        ready = $true
                        game_agent_manifest_row = $true
                        validation_recipe_id = "generated-game-package"
                        generated_game_manifest_id = "generated_game_studio_v1_package"
                        package_manifest_row = "generated_game_package_output"
                    }
                }
                validation_counters = [ordered]@{
                    renderer_generated_game_package_output_ready = $true
                    renderer_generated_game_manifest_ready = $true
                    renderer_package_arbitrary_script_execution = $false
                    renderer_package_script_execution = $false
                }
                non_claims = [ordered]@{
                    arbitrary_script_execution = $false
                    package_script_execution = $false
                    native_handles_exposed = $false
                    external_engine_parity = $false
                    environment_ready = $false
                }
            }
        }
        non_claims = [ordered]@{
            arbitrary_script_execution = $false
            package_script_execution = $false
            native_handles_exposed = $false
            external_engine_parity = $false
            environment_ready = $false
            renderer_commercial_readiness = $false
        }
    }
}

$producerScript = Join-Path $root "tools/collect-renderer-package-commercial-quality-artifacts.ps1"
if (-not (Test-Path -LiteralPath $producerScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-package-commercial-quality-artifacts.ps1 must exist for Task 10D package artifact production."
}

$collectorScript = Join-Path $root "tools/collect-renderer-commercial-readiness-evidence.ps1"
if (-not (Test-Path -LiteralPath $collectorScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-commercial-readiness-evidence.ps1 must exist for Task 10 retained artifact assembly."
}

$fixtureRoot = "tests/fixtures/renderer/commercial-readiness-evidence/ready"
$evidenceRootRelative = "artifacts/renderer/commercial-readiness-evidence/package-commercial-quality-contract-$PID"
$inputRootRelative = "$evidenceRootRelative/input"
$producerOutputRootRelative = "$evidenceRootRelative/producer-output"
$collectorOutputRootRelative = "$evidenceRootRelative/collector-output"
$realInputRelative = "$inputRootRelative/package-host-evidence.json"
$fixtureInputRelative = "$inputRootRelative/package-fixture-evidence.json"
$evidenceRootPath = ConvertTo-LocalPath $evidenceRootRelative

try {
    if (Test-Path -LiteralPath $evidenceRootPath) {
        Remove-Item -LiteralPath $evidenceRootPath -Recurse -Force
    }

    Write-JsonObject -Path (ConvertTo-LocalPath $realInputRelative) `
        -Value (New-PackageHostEvidence -FixtureOnly $false)
    Write-JsonObject -Path (ConvertTo-LocalPath $fixtureInputRelative) `
        -Value (New-PackageHostEvidence -FixtureOnly $true)

    $planLines = @(& $producerScript -Mode Plan -OutputRootRelative $producerOutputRootRelative)
    foreach ($expectedLine in @(
            "renderer_package_commercial_quality_artifacts_collector_mode=Plan",
            "renderer_package_commercial_quality_artifacts_written=0",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        )) {
        Assert-LinePresent $planLines $expectedLine "package artifact producer Plan mode"
    }

    $unsafeRejected = $false
    try {
        $null = & $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -PackageHostEvidenceRelative "../unsafe.json" 2>&1
    }
    catch {
        $unsafeRejected = [string]$_.Exception.Message -like "*unsafe_relative_path*"
    }
    if (-not $unsafeRejected) {
        Write-Error "package artifact producer must reject unsafe relative paths."
    }

    $fixtureRejected = $false
    try {
        $null = & $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -PackageHostEvidenceRelative $fixtureInputRelative 2>&1
    }
    catch {
        $fixtureRejected = [string]$_.Exception.Message -like "*fixture_artifact_input_rejected*"
    }
    if (-not $fixtureRejected) {
        Write-Error "package artifact producer must reject fixture-only host evidence."
    }

    $assembleLines = @(& $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -PackageHostEvidenceRelative $realInputRelative)
    foreach ($expectedLine in @(
            "renderer_package_commercial_quality_artifacts_collector_mode=Assemble",
            "renderer_package_commercial_quality_artifacts_written=4",
            "renderer_package_commercial_quality_fixture_artifact=0",
            "renderer_visible_3d_package_ready=1",
            "renderer_runtime_ui_package_ready=1",
            "renderer_environment_package_ready=1",
            "renderer_generated_game_package_ready=1",
            "renderer_package_arbitrary_script_execution=0",
            "renderer_package_script_execution=0",
            "renderer_backend_parity_ready=0",
            "renderer_metal_broad_readiness=0",
            "renderer_broad_quality_ready=0",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        )) {
        Assert-LinePresent $assembleLines $expectedLine "package artifact producer Assemble mode"
    }

    foreach ($artifactSpec in @(
            @{ Path = "visible-3d-package.json"; ArtifactId = "visible-3d-package"; Schema = "GameEngine.DesktopRuntimePackageEvidence.v1" },
            @{ Path = "runtime-ui-package.json"; ArtifactId = "runtime-ui-package"; Schema = "GameEngine.DesktopRuntimePackageEvidence.v1" },
            @{ Path = "environment-package.json"; ArtifactId = "environment-package"; Schema = "GameEngine.EnvironmentPackageEvidence.v1" },
            @{ Path = "generated-game-package.json"; ArtifactId = "generated-game-package"; Schema = "GameEngine.GeneratedGamePackageEvidence.v1" }
        )) {
        $artifactPath = ConvertTo-LocalPath "$producerOutputRootRelative/$($artifactSpec.Path)"
        if (-not (Test-Path -LiteralPath $artifactPath -PathType Leaf)) {
            Write-Error "package artifact producer did not write $($artifactSpec.Path)."
        }
        $artifact = Get-Content -LiteralPath $artifactPath -Raw | ConvertFrom-Json
        if ([string]$artifact.schema_version -ne [string]$artifactSpec.Schema) {
            Write-Error "$($artifactSpec.Path) schema_version mismatch."
        }
        if ([string]$artifact.artifact_id -ne [string]$artifactSpec.ArtifactId) {
            Write-Error "$($artifactSpec.Path) artifact_id mismatch."
        }
        if ([bool]$artifact.fixture_only) {
            Write-Error "$($artifactSpec.Path) must not be fixture_only when package evidence is retained real evidence."
        }
        if ([bool]$artifact.non_claims.external_engine_parity) {
            Write-Error "$($artifactSpec.Path) must not claim external engine parity."
        }
        if ([bool]$artifact.non_claims.native_handles_exposed) {
            Write-Error "$($artifactSpec.Path) must keep native handles unexposed."
        }
        if ([bool]$artifact.non_claims.environment_ready) {
            Write-Error "$($artifactSpec.Path) must not promote broad environment readiness."
        }
    }

    $collectorArguments = @{
        OutputRootRelative = $collectorOutputRootRelative
        D3d12ArtifactRelative = "$fixtureRoot/d3d12-quality.json"
        VulkanStrictArtifactRelative = "$fixtureRoot/vulkan-strict-quality.json"
        AppleMetalArtifactRelative = "$fixtureRoot/apple-metal-host.json"
        Visible3dPackageArtifactRelative = "$producerOutputRootRelative/visible-3d-package.json"
        RuntimeUiPackageArtifactRelative = "$producerOutputRootRelative/runtime-ui-package.json"
        EnvironmentPackageArtifactRelative = "$producerOutputRootRelative/environment-package.json"
        GeneratedGamePackageArtifactRelative = "$producerOutputRootRelative/generated-game-package.json"
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
        "renderer_commercial_readiness_evidence_collector_fixture_artifacts=7" `
        "commercial readiness collector with retained package artifacts"
    Assert-LinePresent $collectorLines `
        "renderer_commercial_readiness_evidence_collector_real_promotion_candidate=0" `
        "commercial readiness collector with retained package artifacts"

    $validationLines = @(& (Join-Path $PSScriptRoot "validate-renderer-commercial-readiness-evidence.ps1") `
            -ArtifactRootRelative $collectorOutputRootRelative)
    $validationValues = ConvertFrom-KeyValueLines -Lines $validationLines
    foreach ($counter in @(
            "renderer_visible_3d_package_ready",
            "renderer_runtime_ui_package_ready",
            "renderer_environment_package_ready",
            "renderer_generated_game_package_ready"
        )) {
        if ([string]$validationValues[$counter] -ne "1") {
            Write-Error "retained package artifacts must validate $counter=1."
        }
    }
    if ([string]$validationValues["renderer_commercial_readiness_fixture_artifacts_rejected"] -ne "7") {
        Write-Error "collector output must reject only the remaining fixture artifacts."
    }
    if ([string]$validationValues["renderer_environment_ready"] -ne "0") {
        Write-Error "package artifact producer must not promote renderer_environment_ready."
    }
    if ([string]$validationValues["renderer_commercial_readiness"] -ne "0") {
        Write-Error "package-only producer must not promote renderer_commercial_readiness."
    }
}
finally {
    if (Test-Path -LiteralPath $evidenceRootPath) {
        Remove-Item -LiteralPath $evidenceRootPath -Recurse -Force
    }
}

Write-Information "renderer-package-commercial-quality-artifacts-check: ok" -InformationAction Continue
