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

function Write-JsonObject {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][object]$Value
    )

    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        $null = New-Item -ItemType Directory -Path $parent -Force
    }
    $Value | ConvertTo-Json -Depth 16 |
        Set-Content -LiteralPath $Path -Encoding utf8NoBOM
}

function Assert-JsonStringProperty {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Expected,
        [Parameter(Mandatory = $true)][string]$Context
    )

    $actual = [string]$JsonObject.PSObject.Properties[$Name].Value
    if ($actual -cne $Expected) {
        Write-Error "$Context expected $Name=$Expected but found '$actual'."
    }
}

function Assert-JsonFalseProperty {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Context
    )

    $value = $JsonObject.PSObject.Properties[$Name].Value
    if ($false -ne [bool]$value -or $value -isnot [bool]) {
        Write-Error "$Context expected $Name=false."
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

$producerScript = Join-Path $root "tools/generate-renderer-apple-metal-commercial-quality-host-evidence.ps1"
if (-not (Test-Path -LiteralPath $producerScript -PathType Leaf)) {
    Write-Error "tools/generate-renderer-apple-metal-commercial-quality-host-evidence.ps1 must exist for retained Apple Metal host evidence generation."
}

$artifactCollectorScript = Join-Path $root "tools/collect-renderer-apple-metal-commercial-quality-artifact.ps1"
if (-not (Test-Path -LiteralPath $artifactCollectorScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-apple-metal-commercial-quality-artifact.ps1 must exist for retained Apple Metal artifact production."
}

$readinessCollectorScript = Join-Path $root "tools/collect-renderer-commercial-readiness-evidence.ps1"
if (-not (Test-Path -LiteralPath $readinessCollectorScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-commercial-readiness-evidence.ps1 must exist for retained artifact assembly."
}

$fixtureRoot = "tests/fixtures/renderer/commercial-readiness-evidence/ready"
$evidenceRootRelative = "artifacts/renderer/apple-metal-commercial-quality-host-evidence/contract-$PID"
$sourceRootRelative = "$evidenceRootRelative/source"
$producerOutputRootRelative = "$evidenceRootRelative/host-output"
$memoryRootRelative = "artifacts/renderer/metal-memory-profiling-host-evidence/apple-metal-host-evidence-contract-$PID"
$artifactOutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/apple-metal-host-evidence-contract-$PID/apple-artifact"
$collectorOutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/apple-metal-host-evidence-contract-$PID/readiness"
$minimalStatusRelative = "$sourceRootRelative/minimal-apple-metal-status.log"
$readyStatusRelative = "$sourceRootRelative/ready-apple-metal-status.log"
$memoryInputRelative = "$memoryRootRelative/metal-memory-profiling-evidence.json"
$evidenceRootPath = ConvertTo-LocalPath $evidenceRootRelative
$memoryRootPath = ConvertTo-LocalPath $memoryRootRelative
$commercialRootPath = ConvertTo-LocalPath "artifacts/renderer/commercial-readiness-evidence/apple-metal-host-evidence-contract-$PID"

$minimalStatusText = @"
renderer_apple_metal_commercial_quality_host_source_status=ready renderer_apple_metal_commercial_quality_host_source_schema=GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1 renderer_apple_metal_commercial_quality_host_source_recipe=renderer-metal-apple-host-evidence renderer_apple_metal_commercial_quality_memory_source_schema=GameEngine.RendererMetalMemoryProfilingHostEvidence.v1 renderer_apple_metal_commercial_quality_memory_source_recipe=renderer-metal-memory-profiling-host-evidence renderer_apple_metal_commercial_quality_artifact_ready=0 renderer_apple_metal_commercial_quality_native_handles_exposed=0 renderer_apple_metal_commercial_quality_cross_backend_inference=0 renderer_backend_parity_ready=0 renderer_metal_broad_readiness=0 renderer_broad_quality_ready=0 renderer_commercial_readiness=0 renderer_environment_ready=0
"@

$readyStatusText = @"
renderer_apple_metal_commercial_quality_host_source_status=ready renderer_apple_metal_commercial_quality_host_source_schema=GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1 renderer_apple_metal_commercial_quality_host_source_recipe=renderer-metal-apple-host-evidence renderer_apple_metal_commercial_quality_host_source_id=Apple-Metal-Commercial-Host-Bridge-2026-06-25 renderer_apple_metal_xcode_tools_ready=1 renderer_apple_metal_full_xcode_selected=1 renderer_apple_metal_metal_tool_ready=1 renderer_apple_metal_metallib_tool_ready=1 renderer_apple_metal_command_line_metal_tools=1 renderer_apple_metal_toolchain_source_id=Apple-Building-Shader-Library-Precompiling-Source-Files-2026-06-25 renderer_apple_metal_msl_shader_ready=1 renderer_apple_metal_msl_source_id=Apple-Metal-Shading-Language-Specification-2026-06-25 renderer_apple_metal_msl_address_spaces=device,constant,threadgroup renderer_apple_metal_msl_function_constant_attribute=[[function_constant]] renderer_apple_metal_msl_resource_binding_attributes=[[buffer]],[[texture]],[[sampler]] renderer_apple_metal_msl_stage_attributes=[[vertex]],[[fragment]],[[kernel]] renderer_apple_metal_environment_aggregate_recipe=renderer-metal-environment-aggregate-apple-host-evidence renderer_apple_metal_visible_package_evidence_status=ready renderer_apple_metal_visible_package_evidence_ready=1 renderer_apple_metal_visible_package_broad_claims=0 renderer_apple_metal_visible_package_selected_3d=1 renderer_apple_metal_visible_package_runtime_ui=1 renderer_apple_metal_visible_package_environment=1 renderer_apple_metal_visible_package_generated_game=1 renderer_apple_metal_visible_package_rows=4 renderer_apple_metal_visible_package_hash_sha256=0d4d4e443582ab659a52bfb21fcab6f9aed7f1fcd29882a595c9f5a4f6b8b77a renderer_apple_metal_commercial_quality_native_handles_exposed=0 renderer_apple_metal_commercial_quality_cross_backend_inference=0 renderer_backend_parity_ready=0 renderer_metal_broad_readiness=0 renderer_broad_quality_ready=0 renderer_commercial_readiness=0 renderer_environment_ready=0
"@

try {
    foreach ($path in @($evidenceRootPath, $memoryRootPath, $commercialRootPath)) {
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -Recurse -Force
        }
    }

    Write-TextFile -Path (ConvertTo-LocalPath $minimalStatusRelative) -Value $minimalStatusText
    Write-TextFile -Path (ConvertTo-LocalPath $readyStatusRelative) -Value $readyStatusText
    Write-JsonObject -Path (ConvertTo-LocalPath $memoryInputRelative) -Value (New-MetalMemoryProfilingEvidence)

    $planLines = @(& $producerScript -Mode Plan -OutputRootRelative $producerOutputRootRelative)
    Assert-LinePresent $planLines `
        "renderer_apple_metal_commercial_quality_host_evidence_generator_mode=Plan" `
        "Apple Metal host evidence generator Plan mode"
    Assert-LinePresent $planLines `
        "renderer_apple_metal_commercial_quality_host_evidence_written=0" `
        "Apple Metal host evidence generator Plan mode"
    Assert-LinePresent $planLines `
        "renderer_commercial_readiness=0" `
        "Apple Metal host evidence generator Plan mode"
    Assert-LinePresent $planLines `
        "renderer_environment_ready=0" `
        "Apple Metal host evidence generator Plan mode"

    $unsafeRejected = $false
    try {
        $null = & $producerScript `
            -Mode Generate `
            -OutputRootRelative $producerOutputRootRelative `
            -AppleMetalStatusLogRelative "../unsafe.txt" 2>&1
    }
    catch {
        $unsafeRejected = [string]$_.Exception.Message -like "*unsafe_relative_path*"
    }
    if (-not $unsafeRejected) {
        Write-Error "Apple Metal host evidence generator must reject unsafe relative paths."
    }

    $minimalRejected = $false
    try {
        $null = & $producerScript `
            -Mode Generate `
            -OutputRootRelative $producerOutputRootRelative `
            -AppleMetalStatusLogRelative $minimalStatusRelative `
            -RequireReady 2>&1
    }
    catch {
        $minimalRejected = [string]$_.Exception.Message -like "*apple_metal_commercial_quality_host_evidence_not_ready*"
    }
    if (-not $minimalRejected) {
        Write-Error "Apple Metal host evidence generator must fail closed for the existing minimal status rows."
    }
    $hostGatePath = ConvertTo-LocalPath "$producerOutputRootRelative/host-gate-summary.json"
    if (-not (Test-Path -LiteralPath $hostGatePath -PathType Leaf)) {
        Write-Error "Apple Metal host evidence generator must write host-gate-summary.json for incomplete rows."
    }
    $hostGate = Get-Content -LiteralPath $hostGatePath -Raw | ConvertFrom-Json
    Assert-JsonStringProperty -JsonObject $hostGate -Name "schema_version" `
        -Expected "GameEngine.RendererAppleMetalCommercialQualityHostGate.v1" `
        -Context "Apple Metal host gate summary"
    Assert-JsonStringProperty -JsonObject $hostGate -Name "status" `
        -Expected "host_evidence_required" `
        -Context "Apple Metal host gate summary"
    $minimalHostEvidencePath = ConvertTo-LocalPath "$producerOutputRootRelative/apple-metal-host-evidence.json"
    if (Test-Path -LiteralPath $minimalHostEvidencePath -PathType Leaf) {
        Write-Error "Incomplete Apple Metal status rows must not write host evidence JSON."
    }

    $generateLines = @(& $producerScript `
            -Mode Generate `
            -OutputRootRelative $producerOutputRootRelative `
            -AppleMetalStatusLogRelative $readyStatusRelative `
            -RequireReady)
    foreach ($expectedLine in @(
            "renderer_apple_metal_commercial_quality_host_evidence_generator_mode=Generate",
            "renderer_apple_metal_commercial_quality_host_evidence_status=ready",
            "renderer_apple_metal_commercial_quality_host_evidence_ready=1",
            "renderer_apple_metal_commercial_quality_host_evidence_written=1",
            "renderer_apple_metal_xcode_tools_ready=1",
            "renderer_apple_metal_msl_shader_ready=1",
            "renderer_apple_metal_visible_package_ready=1",
            "renderer_apple_metal_native_handles_exposed=0",
            "renderer_apple_metal_cross_backend_inference=0",
            "renderer_backend_parity_ready=0",
            "renderer_metal_broad_readiness=0",
            "renderer_broad_quality_ready=0",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        )) {
        Assert-LinePresent $generateLines $expectedLine "Apple Metal host evidence generator Generate mode"
    }

    $hostEvidenceRelative = "$producerOutputRootRelative/apple-metal-host-evidence.json"
    $hostEvidencePath = ConvertTo-LocalPath $hostEvidenceRelative
    if (-not (Test-Path -LiteralPath $hostEvidencePath -PathType Leaf)) {
        Write-Error "Apple Metal host evidence generator did not write apple-metal-host-evidence.json."
    }
    $hostEvidence = Get-Content -LiteralPath $hostEvidencePath -Raw | ConvertFrom-Json
    Assert-JsonStringProperty -JsonObject $hostEvidence -Name "schema_version" `
        -Expected "GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1" `
        -Context "Apple Metal host evidence"
    Assert-JsonStringProperty -JsonObject $hostEvidence -Name "claim_id" `
        -Expected "renderer-apple-metal-commercial-quality-artifact-v1" `
        -Context "Apple Metal host evidence"
    Assert-JsonStringProperty -JsonObject $hostEvidence -Name "validation_recipe" `
        -Expected "renderer-metal-apple-host-evidence" `
        -Context "Apple Metal host evidence"
    Assert-JsonStringProperty -JsonObject $hostEvidence -Name "source_id" `
        -Expected "Apple-Metal-Commercial-Host-Bridge-2026-06-25" `
        -Context "Apple Metal host evidence"
    if ([bool]$hostEvidence.fixture_only) {
        Write-Error "Apple Metal host evidence generator output must not be fixture_only."
    }
    Assert-JsonStringProperty -JsonObject $hostEvidence.proof_rows.host_toolchain -Name "toolchain_source_id" `
        -Expected "Apple-Building-Shader-Library-Precompiling-Source-Files-2026-06-25" `
        -Context "Apple Metal host evidence host_toolchain"
    Assert-JsonStringProperty -JsonObject $hostEvidence.proof_rows.msl_shader -Name "msl_source_id" `
        -Expected "Apple-Metal-Shading-Language-Specification-2026-06-25" `
        -Context "Apple Metal host evidence msl_shader"
    if ([string]$hostEvidence.proof_rows.visible_package.deterministic_hash_sha256 -cnotmatch "^[0-9a-f]{64}$") {
        Write-Error "Apple Metal host evidence visible package hash must be lower-case SHA-256."
    }
    Assert-JsonFalseProperty -JsonObject $hostEvidence.proof_rows.native_handles `
        -Name "native_handles_exposed" `
        -Context "Apple Metal host evidence native_handles"
    Assert-JsonFalseProperty -JsonObject $hostEvidence.non_claims `
        -Name "external_engine_parity" `
        -Context "Apple Metal host evidence non_claims"
    Assert-JsonFalseProperty -JsonObject $hostEvidence.non_claims `
        -Name "metal_objects_public" `
        -Context "Apple Metal host evidence non_claims"

    $artifactLines = @(& $artifactCollectorScript `
            -Mode Assemble `
            -OutputRootRelative $artifactOutputRootRelative `
            -AppleMetalHostEvidenceRelative $hostEvidenceRelative `
            -MetalMemoryProfilingHostEvidenceRelative $memoryInputRelative)
    Assert-LinePresent $artifactLines `
        "renderer_apple_metal_commercial_quality_artifact_written=1" `
        "Apple Metal artifact collector with generated host evidence"
    Assert-LinePresent $artifactLines `
        "renderer_apple_metal_commercial_quality_fixture_artifact=0" `
        "Apple Metal artifact collector with generated host evidence"

    $readinessArguments = @{
        OutputRootRelative = $collectorOutputRootRelative
        D3d12ArtifactRelative = "$fixtureRoot/d3d12-quality.json"
        VulkanStrictArtifactRelative = "$fixtureRoot/vulkan-strict-quality.json"
        AppleMetalArtifactRelative = "$artifactOutputRootRelative/apple-metal-host.json"
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
        "commercial readiness collector with generated Apple Metal host evidence"
    Assert-LinePresent $collectorLines `
        "renderer_commercial_readiness_evidence_collector_real_promotion_candidate=0" `
        "commercial readiness collector with generated Apple Metal host evidence"

    $validationLines = @(& (Join-Path $PSScriptRoot "validate-renderer-commercial-readiness-evidence.ps1") `
            -ArtifactRootRelative $collectorOutputRootRelative)
    $validationValues = ConvertFrom-KeyValueLines -Lines $validationLines
    if ([string]$validationValues["renderer_apple_metal_renderer_quality_ready"] -ne "1") {
        Write-Error "generated Apple Metal host evidence must validate as renderer_apple_metal_renderer_quality_ready."
    }
    if ([string]$validationValues["renderer_commercial_readiness"] -ne "0") {
        Write-Error "Apple Metal-only host evidence generation must not promote renderer_commercial_readiness."
    }
}
finally {
    foreach ($path in @($evidenceRootPath, $memoryRootPath, $commercialRootPath)) {
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -Recurse -Force
        }
    }
}

Write-Information "renderer-apple-metal-commercial-quality-host-evidence-check: ok" -InformationAction Continue
