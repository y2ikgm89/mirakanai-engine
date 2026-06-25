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

function New-AppleMetalHostEvidence {
    param([Parameter(Mandatory = $true)][bool]$FixtureOnly)

    return [ordered]@{
        schema_version = "GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1"
        claim_id = "renderer-apple-metal-commercial-quality-artifact-v1"
        validation_recipe = "renderer-metal-apple-host-evidence"
        fixture_only = $FixtureOnly
        source_id = "Apple-Metal-Commercial-Host-Bridge-2026-06-25"
        source_rows = [ordered]@{
            renderer_host_validation_recipe_id = "renderer-metal-apple-host-evidence"
            environment_aggregate_validation_recipe_id = "renderer-metal-environment-aggregate-apple-host-evidence"
            visible_package_evidence_status = "ready"
            visible_package_evidence_ready = $true
            visible_package_broad_claims = $false
        }
        proof_rows = [ordered]@{
            host_toolchain = [ordered]@{
                ready = $true
                xcode_host_ready = $true
                metal_tool_ready = $true
                metallib_tool_ready = $true
                command_line_metal_tools = $true
                toolchain_source_id = "Apple-Building-Shader-Library-Precompiling-Source-Files-2026-06-25"
            }
            msl_shader = [ordered]@{
                ready = $true
                address_spaces = @("device", "constant", "threadgroup")
                function_constant_attribute = "[[function_constant]]"
                resource_binding_attributes = @("[[buffer]]", "[[texture]]", "[[sampler]]")
                stage_attributes = @("[[vertex]]", "[[fragment]]", "[[kernel]]")
                msl_source_id = "Apple-Metal-Shading-Language-Specification-2026-06-25"
            }
            visible_package = [ordered]@{
                ready = $true
                selected_3d_package = $true
                runtime_ui_package = $true
                environment_package = $true
                generated_game_package = $true
                visible_package_rows = 4
                deterministic_hash_sha256 = "0d4d4e443582ab659a52bfb21fcab6f9aed7f1fcd29882a595c9f5a4f6b8b77a"
            }
            native_handles = [ordered]@{
                ready = $true
                native_handles_exposed = $false
                objective_cxx_boundary_private = $true
            }
            cross_backend_inference = [ordered]@{
                ready = $true
                d3d12_inferred = $false
                vulkan_inferred = $false
            }
        }
        validation_counters = [ordered]@{
            renderer_apple_metal_xcode_tools_ready = $true
            renderer_apple_metal_msl_shader_ready = $true
            renderer_apple_metal_visible_package_ready = $true
        }
        non_claims = [ordered]@{
            d3d12_inferred = $false
            vulkan_inferred = $false
            environment_ready = $false
            external_engine_parity = $false
            native_handles_exposed = $false
            metal_objects_public = $false
        }
    }
}

function New-MetalMemoryProfilingEvidence {
    return [ordered]@{
        schema_version = "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1"
        claim_id = "renderer-metal-memory-profiling-host-evidence-v1"
        host = [ordered]@{
            platform = "macos"
            macos_version = "15.5"
            xcode_version = "16.4"
            full_xcode_selected = $true
            metal_tool_ready = $true
            metallib_tool_ready = $true
        }
        source_rows = [ordered]@{
            heap_documentation_source_id = "Apple-Metal-MTLHeap-2026-06-24"
            residency_set_documentation_source_id = "Apple-Metal-MTLResidencySet-2026-06-24"
            residency_request_documentation_source_id = "Apple-Metal-MTLResidencySet-requestResidency-2026-06-24"
            command_queue_residency_documentation_source_id = "Apple-Metal-MTLCommandQueue-addResidencySet-2026-06-24"
            capture_manager_documentation_source_id = "Apple-Metal-MTLCaptureManager-2026-06-24"
            programmatic_capture_documentation_source_id = "Apple-Metal-ProgrammaticCapture-2026-06-24"
        }
        memory_residency_row = [ordered]@{
            proof_row_id = "memory_residency"
            host_validation_recipe_id = "renderer-metal-memory-profiling-host-evidence"
            first_party_workload_id = "renderer-commercial-readiness-visible-package"
            runtime_ready = $true
            command_queue_ready = $true
            heap_api_name = "MTLHeap"
            heap_allocation_ready = $true
            heap_resource_allocation_ready = $true
            heap_resource_rows = 2
            heap_allocated_bytes = 4096
            resident_bytes = 2048
            budget_bytes = 4096
            residency_api_name = "MTLResidencySet"
            residency_set_ready = $true
            residency_set_allocation_rows = 2
            residency_request_ready = $true
            residency_commit_ready = $true
            command_queue_residency_set_committed = $true
            residency_pressure_evidence_ready = $true
            memory_pressure_sample_rows = 1
            memory_pressure_budget_status = "within_budget"
        }
        profiling_capture_row = [ordered]@{
            proof_row_id = "profiling_capture"
            host_validation_recipe_id = "renderer-metal-memory-profiling-host-evidence"
            first_party_workload_id = "renderer-commercial-readiness-visible-package"
            runtime_ready = $true
            command_queue_ready = $true
            capture_api_name = "MTLCaptureManager"
            capture_manager_ready = $true
            capture_descriptor_ready = $true
            capture_object_ready = $true
            capture_scope_ready = $true
            capture_scope_label = "renderer-commercial-readiness-visible-package"
            capture_boundary_ready = $true
            capture_started = $true
            capture_stopped = $true
            command_buffer_captured = $true
            capture_artifact_path = "capture.gputrace"
            capture_artifact_hash_sha256 = "60b725f10c9c85c70d97880dfe8191b3a9b5813d033ed4a55c5228512c6501de"
            deterministic_capture_hash_sha256 = "60b725f10c9c85c70d97880dfe8191b3a9b5813d033ed4a55c5228512c6501de"
            capture_artifact_rows = 1
        }
        non_claims = [ordered]@{
            simulator_only_evidence = $false
            cross_backend_inference = $false
            native_handles_exposed = $false
            broad_backend_parity_ready = $false
            broad_metal_readiness = $false
            commercial_renderer_readiness = $false
            broad_renderer_quality = $false
            environment_ready = $false
            external_engine_api_parity = $false
        }
    }
}

$producerScript = Join-Path $root "tools/collect-renderer-apple-metal-commercial-quality-artifact.ps1"
if (-not (Test-Path -LiteralPath $producerScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-apple-metal-commercial-quality-artifact.ps1 must exist for Task 10C retained Apple Metal artifact production."
}

$collectorScript = Join-Path $root "tools/collect-renderer-commercial-readiness-evidence.ps1"
if (-not (Test-Path -LiteralPath $collectorScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-commercial-readiness-evidence.ps1 must exist for Task 10 retained artifact assembly."
}

$fixtureRoot = "tests/fixtures/renderer/commercial-readiness-evidence/ready"
$evidenceRootRelative = "artifacts/renderer/commercial-readiness-evidence/apple-metal-commercial-quality-contract-$PID"
$inputRootRelative = "$evidenceRootRelative/input"
$producerOutputRootRelative = "$evidenceRootRelative/producer-output"
$collectorOutputRootRelative = "$evidenceRootRelative/collector-output"
$realAppleInputRelative = "$inputRootRelative/apple-metal-host-evidence.json"
$fixtureAppleInputRelative = "$inputRootRelative/apple-metal-fixture-evidence.json"
$memoryInputRelative = "$inputRootRelative/metal-memory-profiling-evidence.json"
$evidenceRootPath = ConvertTo-LocalPath $evidenceRootRelative

try {
    if (Test-Path -LiteralPath $evidenceRootPath) {
        Remove-Item -LiteralPath $evidenceRootPath -Recurse -Force
    }

    Write-JsonObject `
        -Path (ConvertTo-LocalPath $realAppleInputRelative) `
        -Value (New-AppleMetalHostEvidence -FixtureOnly $false)
    Write-JsonObject `
        -Path (ConvertTo-LocalPath $fixtureAppleInputRelative) `
        -Value (New-AppleMetalHostEvidence -FixtureOnly $true)
    Write-JsonObject `
        -Path (ConvertTo-LocalPath $memoryInputRelative) `
        -Value (New-MetalMemoryProfilingEvidence)

    $planLines = @(& $producerScript -Mode Plan -OutputRootRelative $producerOutputRootRelative)
    Assert-LinePresent $planLines `
        "renderer_apple_metal_commercial_quality_artifact_collector_mode=Plan" `
        "Apple Metal artifact producer Plan mode"
    Assert-LinePresent $planLines `
        "renderer_apple_metal_commercial_quality_artifact_written=0" `
        "Apple Metal artifact producer Plan mode"
    Assert-LinePresent $planLines `
        "renderer_apple_metal_commercial_quality_host_gated=1" `
        "Apple Metal artifact producer Plan mode"
    Assert-LinePresent $planLines `
        "renderer_commercial_readiness=0" `
        "Apple Metal artifact producer Plan mode"
    Assert-LinePresent $planLines `
        "renderer_environment_ready=0" `
        "Apple Metal artifact producer Plan mode"

    $unsafeRejected = $false
    try {
        $null = & $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -AppleMetalHostEvidenceRelative "../unsafe.json" `
            -MetalMemoryProfilingHostEvidenceRelative $memoryInputRelative 2>&1
    }
    catch {
        $unsafeRejected = [string]$_.Exception.Message -like "*unsafe_relative_path*"
    }
    if (-not $unsafeRejected) {
        Write-Error "Apple Metal artifact producer must reject unsafe relative paths."
    }

    $fixtureRejected = $false
    try {
        $null = & $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -AppleMetalHostEvidenceRelative $fixtureAppleInputRelative `
            -MetalMemoryProfilingHostEvidenceRelative $memoryInputRelative 2>&1
    }
    catch {
        $fixtureRejected = [string]$_.Exception.Message -like "*fixture_artifact_input_rejected*"
    }
    if (-not $fixtureRejected) {
        Write-Error "Apple Metal artifact producer must reject fixture-only host evidence."
    }

    $assembleLines = @(& $producerScript `
            -Mode Assemble `
            -OutputRootRelative $producerOutputRootRelative `
            -AppleMetalHostEvidenceRelative $realAppleInputRelative `
            -MetalMemoryProfilingHostEvidenceRelative $memoryInputRelative)
    Assert-LinePresent $assembleLines `
        "renderer_apple_metal_commercial_quality_artifact_collector_mode=Assemble" `
        "Apple Metal artifact producer Assemble mode"
    Assert-LinePresent $assembleLines `
        "renderer_apple_metal_commercial_quality_artifact_written=1" `
        "Apple Metal artifact producer Assemble mode"
    Assert-LinePresent $assembleLines `
        "renderer_apple_metal_commercial_quality_fixture_artifact=0" `
        "Apple Metal artifact producer Assemble mode"
    foreach ($expectedLine in @(
            "renderer_apple_metal_xcode_tools_ready=1",
            "renderer_apple_metal_msl_shader_ready=1",
            "renderer_apple_metal_heap_ready=1",
            "renderer_apple_metal_residency_set_ready=1",
            "renderer_apple_metal_capture_ready=1",
            "renderer_apple_metal_visible_package_ready=1",
            "renderer_apple_metal_native_handles_exposed=0",
            "renderer_apple_metal_cross_backend_inference=0",
            "renderer_backend_parity_ready=0",
            "renderer_metal_broad_readiness=0",
            "renderer_broad_quality_ready=0",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        )) {
        Assert-LinePresent $assembleLines $expectedLine "Apple Metal artifact producer Assemble mode"
    }

    $artifactPath = ConvertTo-LocalPath "$producerOutputRootRelative/apple-metal-host.json"
    if (-not (Test-Path -LiteralPath $artifactPath -PathType Leaf)) {
        Write-Error "Apple Metal artifact producer did not write apple-metal-host.json."
    }
    $artifact = Get-Content -LiteralPath $artifactPath -Raw | ConvertFrom-Json
    if ([string]$artifact.schema_version -ne "GameEngine.RendererCommercialQualityCloseout.v1") {
        Write-Error "Apple Metal artifact schema_version mismatch."
    }
    if ([string]$artifact.artifact_id -ne "apple-metal-host") {
        Write-Error "Apple Metal artifact_id mismatch."
    }
    if ([bool]$artifact.fixture_only) {
        Write-Error "Apple Metal producer output must not be fixture_only when host evidence is retained real evidence."
    }
    if ([bool]$artifact.non_claims.external_engine_parity) {
        Write-Error "Apple Metal producer output must not claim external engine parity."
    }
    if ([bool]$artifact.non_claims.metal_objects_public) {
        Write-Error "Apple Metal producer output must keep Metal objects private."
    }
    if ([bool]$artifact.proof_rows.native_handles.native_handles_exposed) {
        Write-Error "Apple Metal producer output must keep native handles unexposed."
    }
    if ([string]$artifact.proof_rows.heap_resources.api_name -ne "MTLHeap") {
        Write-Error "Apple Metal producer output must preserve MTLHeap proof."
    }
    if ([string]$artifact.proof_rows.residency_set.api_name -ne "MTLResidencySet") {
        Write-Error "Apple Metal producer output must preserve MTLResidencySet proof."
    }
    if ([string]$artifact.proof_rows.capture_manager.api_name -ne "MTLCaptureManager") {
        Write-Error "Apple Metal producer output must preserve MTLCaptureManager proof."
    }

    $collectorArguments = @{
        OutputRootRelative = $collectorOutputRootRelative
        D3d12ArtifactRelative = "$fixtureRoot/d3d12-quality.json"
        VulkanStrictArtifactRelative = "$fixtureRoot/vulkan-strict-quality.json"
        AppleMetalArtifactRelative = "$producerOutputRootRelative/apple-metal-host.json"
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
        "commercial readiness collector with retained Apple Metal artifact"
    Assert-LinePresent $collectorLines `
        "renderer_commercial_readiness_evidence_collector_real_promotion_candidate=0" `
        "commercial readiness collector with retained Apple Metal artifact"

    $validationLines = @(& (Join-Path $PSScriptRoot "validate-renderer-commercial-readiness-evidence.ps1") `
            -ArtifactRootRelative $collectorOutputRootRelative)
    $validationValues = ConvertFrom-KeyValueLines -Lines $validationLines
    if ([string]$validationValues["renderer_apple_metal_renderer_quality_ready"] -ne "1") {
        Write-Error "retained Apple Metal artifact must validate as renderer_apple_metal_renderer_quality_ready."
    }
    if ([string]$validationValues["renderer_commercial_readiness_fixture_artifacts_rejected"] -ne "10") {
        Write-Error "collector output must reject only the remaining fixture artifacts."
    }
    if ([string]$validationValues["renderer_metal_broad_readiness"] -ne "0") {
        Write-Error "retained Apple Metal-only producer must not promote renderer_metal_broad_readiness."
    }
    if ([string]$validationValues["renderer_commercial_readiness"] -ne "0") {
        Write-Error "retained Apple Metal-only producer must not promote renderer_commercial_readiness."
    }
}
finally {
    if (Test-Path -LiteralPath $evidenceRootPath) {
        Remove-Item -LiteralPath $evidenceRootPath -Recurse -Force
    }
}

Write-Information "renderer-apple-metal-commercial-quality-artifact-check: ok" -InformationAction Continue
