#requires -Version 7.0
#requires -PSEdition Core
# Contract script: check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1

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

function Remove-TestRoot {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $fullPath = [System.IO.Path]::GetFullPath((ConvertTo-LocalPath $RelativePath))
    $commercialRoot = [System.IO.Path]::GetFullPath(
        (ConvertTo-LocalPath "artifacts/renderer/commercial-readiness-evidence"))
    $qualityVfxRoot = [System.IO.Path]::GetFullPath(
        (ConvertTo-LocalPath "artifacts/renderer/quality-vfx-commercial-host-evidence"))
    $commercialRootWithSeparator = $commercialRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    $qualityVfxRootWithSeparator = $qualityVfxRoot.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    $allowed =
        $fullPath.StartsWith($commercialRootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase) -or
        $fullPath.StartsWith($qualityVfxRootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)
    if (-not $allowed) {
        Write-Error "Refusing to remove test root outside approved renderer artifact roots: $fullPath"
    }
    if (Test-Path -LiteralPath $fullPath) {
        Remove-Item -LiteralPath $fullPath -Recurse -Force
    }
}

function New-EvidenceStub {
    param(
        [Parameter(Mandatory = $true)][string]$SchemaVersion,
        [Parameter(Mandatory = $true)][string]$ClaimId
    )

    return [ordered]@{
        schema_version = $SchemaVersion
        claim_id = $ClaimId
        fixture_only = $false
    }
}

function Write-Generated3dQualityVfxLog {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $statusLine = [string]::Join(" ", @(
            "sample_generated_desktop_runtime_3d_package status=ready",
            "renderer_quality_matrix_status=host_evidence_required",
            "renderer_quality_matrix_reviewed=1",
            "renderer_quality_matrix_d3d12_ready=1",
            "renderer_quality_matrix_vulkan_strict_ready=1",
            "renderer_quality_matrix_general_renderer_quality_ready=0",
            "renderer_quality_matrix_diagnostics=0",
            "renderer_quality_matrix_replay_hash=42",
            "renderer_quality_matrix_gpu_command_side_effects=0",
            "renderer_quality_matrix_native_capture_side_effects=0",
            "renderer_quality_matrix_crash_upload_side_effects=0",
            "rendering_vfx_profiling_reviewed=1",
            "rendering_vfx_profiling_d3d12_host_evidence_ready=1",
            "rendering_vfx_profiling_vulkan_strict_host_evidence_ready=1",
            "rendering_vfx_profiling_debug_policy_ready=1",
            "rendering_vfx_profiling_debug_cpu_profile_zone_evidence_ready=1",
            "rendering_vfx_profiling_debug_trace_capture_handoff_evidence_ready=1",
            "rendering_vfx_profiling_debug_package_counter_evidence_ready=1",
            "rendering_vfx_profiling_memory_policy_ready=1",
            "rendering_vfx_profiling_memory_budget_evidence_ready=1",
            "rendering_vfx_profiling_memory_residency_pressure_evidence_ready=1",
            "rendering_vfx_profiling_memory_package_counter_evidence_ready=1",
            "rendering_vfx_profiling_diagnostics=0",
            "rendering_vfx_profiling_replay_hash=43",
            "renderer_production_vfx_native_capture_side_effects=0",
            "renderer_production_vfx_crash_upload_side_effects=0",
            "renderer_backend_parity_ready=0",
            "renderer_metal_broad_readiness=0",
            "renderer_broad_quality_ready=0",
            "renderer_commercial_readiness=0",
            "environment_ready=0",
            "native_handles_exposed=0",
            "external_engine_parity=0"
        ))
    Set-Content -LiteralPath (ConvertTo-LocalPath $RelativePath) -Encoding utf8NoBOM -Value $statusLine
}

$importerScript = Join-Path $root "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1"
if (-not (Test-Path -LiteralPath $importerScript -PathType Leaf)) {
    Write-Error "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1 must exist."
}
$importerText = Get-Content -LiteralPath $importerScript -Raw
if (-not $importerText.Contains('$global:LASTEXITCODE = 0')) {
    Write-Error "GitHub artifact intake must clear handled native gh download failures before returning from non-RequireReady import."
}

$contractRootRelative = "artifacts/renderer/commercial-readiness-evidence/final-retained-root-artifact-import-contract-$PID"
$finalRootOnlyContractRootRelative = "artifacts/renderer/commercial-readiness-evidence/final-retained-root-artifact-import-final-root-only-contract-$PID"
$multiRunContractRootRelative = "artifacts/renderer/commercial-readiness-evidence/final-retained-root-artifact-import-multi-run-contract-$PID"
$multiRunQualityRootRelative = "artifacts/renderer/quality-vfx-commercial-host-evidence/final-retained-root-artifact-import-multi-run-contract-$PID"

try {
    Remove-TestRoot -RelativePath $contractRootRelative
    Remove-TestRoot -RelativePath $finalRootOnlyContractRootRelative
    Remove-TestRoot -RelativePath $multiRunContractRootRelative
    Remove-TestRoot -RelativePath $multiRunQualityRootRelative

    $planLines = @(& $importerScript -Mode Plan -OutputRootRelative $contractRootRelative)
    foreach ($expectedLine in @(
            "validation_recipe=renderer-commercial-readiness-final-retained-root-artifact-import",
            "renderer_commercial_readiness_final_retained_root_artifact_import_mode=Plan",
            "renderer_commercial_readiness_final_retained_root_artifact_import_required_workflow_artifacts=11",
            "renderer_commercial_readiness_final_retained_root_artifact_import_required_assembler_inputs=7",
            "renderer_commercial_readiness_final_retained_root_artifact_import_workflow_artifact_list_present=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_missing_workflow_artifacts=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_downloads_artifacts=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_ready=0",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $planLines $expectedLine "GitHub artifact intake Plan mode"
    }

    $unsafeRejected = $false
    try {
        $null = & $importerScript -Mode Plan -OutputRootRelative "../unsafe" 2>&1
    }
    catch {
        $unsafeRejected = [string]$_.Exception.Message -like "*unsafe_relative_path*"
    }
    if (-not $unsafeRejected) {
        Write-Error "GitHub artifact intake must reject unsafe output roots."
    }

    $artifactListUnsafeRejected = $false
    try {
        $null = & $importerScript -Mode Inspect -OutputRootRelative $contractRootRelative `
            -ArtifactListJsonRelative "../unsafe.json" 2>&1
    }
    catch {
        $artifactListUnsafeRejected = [string]$_.Exception.Message -like "*unsafe_relative_path*"
    }
    if (-not $artifactListUnsafeRejected) {
        Write-Error "GitHub artifact intake must reject unsafe artifact list paths."
    }

    $missingLines = @(& $importerScript -Mode Inspect -OutputRootRelative $contractRootRelative -NoWrite)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_retained_root_artifact_import_present_assembler_inputs=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_inputs=7",
            "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_input_names=d3d12_host_evidence,vulkan_strict_host_evidence,apple_metal_host_evidence,metal_memory_profiling_host_evidence,package_host_evidence,quality_vfx_host_evidence,clean_room_legal_review",
            "renderer_commercial_readiness_final_retained_root_artifact_import_final_retained_root_present=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_metal_host_gate_summary_present=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_writes_evidence=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_ready=0"
        )) {
        Assert-LinePresent $missingLines $expectedLine "GitHub artifact intake missing Inspect mode"
    }

    $artifactListPath = "$contractRootRelative/github-artifacts.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $artifactListPath) -Value ([ordered]@{
            total_count = 9
            artifacts = @(
                [ordered]@{ name = "windows-packages"; expired = $false },
                [ordered]@{ name = "renderer-d3d12-commercial-quality-host-evidence"; expired = $false },
                [ordered]@{ name = "renderer-vulkan-strict-commercial-quality-host-evidence"; expired = $false },
                [ordered]@{ name = "renderer-apple-metal-commercial-quality-host-evidence"; expired = $false },
                [ordered]@{ name = "linux-vulkan-host-evidence"; expired = $false },
                [ordered]@{ name = "renderer-metal-memory-profiling-host-artifacts"; expired = $false },
                [ordered]@{ name = "renderer-package-commercial-quality-host-evidence"; expired = $false },
                [ordered]@{ name = "renderer-quality-vfx-commercial-artifacts"; expired = $false },
                [ordered]@{ name = "metal-host-optimization-artifacts"; expired = $false }
            )
        })
    $artifactListLines = @(& $importerScript -Mode Inspect `
            -OutputRootRelative $contractRootRelative `
            -ArtifactListJsonRelative $artifactListPath `
            -NoWrite)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_retained_root_artifact_import_workflow_artifact_list_present=1",
            "renderer_commercial_readiness_final_retained_root_artifact_import_available_workflow_artifacts=9",
            "renderer_commercial_readiness_final_retained_root_artifact_import_missing_workflow_artifacts=2",
            "renderer_commercial_readiness_final_retained_root_artifact_import_missing_workflow_artifact_names=renderer-clean-room-legal-review-artifacts,renderer-commercial-readiness-final-retained-root",
            "renderer_commercial_readiness_final_retained_root_artifact_import_final_root_workflow_artifact_available=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_source_workflow_artifacts=9",
            "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_source_workflow_artifacts=1",
            "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_source_workflow_artifact_names=renderer-clean-room-legal-review-artifacts",
            "renderer_commercial_readiness_final_retained_root_artifact_import_expired_workflow_artifacts=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_ready=0",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $artifactListLines $expectedLine "GitHub artifact intake artifact-list Inspect mode"
    }

    $hostGatePath = ConvertTo-LocalPath "$contractRootRelative/renderer-metal-memory-profiling-host-artifacts/host-gate-summary.json"
    Write-JsonObject -Path $hostGatePath -Value ([ordered]@{
            schema = "GameEngine.RendererMetalMemoryProfilingHostGate.v1"
            validation_recipe = "renderer-metal-memory-profiling-host-evidence"
            status = "host_gated"
            reason = "mtlresidencyset_unavailable"
        })
    $qualityHostGatePath = ConvertTo-LocalPath "$contractRootRelative/renderer-quality-vfx-commercial-artifacts/host-gate-summary.json"
    Write-JsonObject -Path $qualityHostGatePath -Value ([ordered]@{
            schema_version = "GameEngine.RendererQualityVfxCommercialHostGate.v1"
            validation_recipe = "renderer-quality-vfx-commercial-artifacts"
            status = "host_gated"
            reason = "metal_memory_profiling_host_evidence_required"
        })
    $nestedMetalHostGatePath = ConvertTo-LocalPath "$contractRootRelative/renderer-quality-vfx-commercial-artifacts/source-artifacts/renderer-metal-memory-profiling-host-artifacts/host-gate-summary.json"
    Write-JsonObject -Path $nestedMetalHostGatePath -Value ([ordered]@{
            schema = "GameEngine.RendererMetalMemoryProfilingHostGate.v1"
            validation_recipe = "renderer-metal-memory-profiling-host-evidence"
            status = "host_gated"
            reason = "mtlresidencyset_unavailable"
        })
    $hostGateLines = @(& $importerScript -Mode Inspect -OutputRootRelative $contractRootRelative -NoWrite)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_retained_root_artifact_import_host_gate_summaries=3",
            "renderer_commercial_readiness_final_retained_root_artifact_import_host_gate_summary_statuses=host_gated,host_gated,host_gated",
            "renderer_commercial_readiness_final_retained_root_artifact_import_host_gate_summary_reasons=mtlresidencyset_unavailable,metal_memory_profiling_host_evidence_required",
            "renderer_commercial_readiness_final_retained_root_artifact_import_host_gate_blocked_assembler_inputs=2",
            "renderer_commercial_readiness_final_retained_root_artifact_import_host_gate_blocked_assembler_input_names=metal_memory_profiling_host_evidence,quality_vfx_host_evidence",
            "renderer_commercial_readiness_final_retained_root_artifact_import_quality_vfx_dependency_blockers=metal_memory_profiling_host_evidence",
            "renderer_commercial_readiness_final_retained_root_artifact_import_metal_host_gate_summary_present=1",
            "renderer_commercial_readiness_final_retained_root_artifact_import_metal_host_gate_status=host_gated",
            "renderer_commercial_readiness_final_retained_root_artifact_import_metal_host_gate_reason=mtlresidencyset_unavailable",
            "renderer_commercial_readiness_final_retained_root_artifact_import_ready=0",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $hostGateLines $expectedLine "GitHub artifact intake host-gated Inspect mode"
    }
    $hostGateManifestLines = @(& $importerScript -Mode Inspect -OutputRootRelative $contractRootRelative)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_retained_root_artifact_import_host_gate_blocked_assembler_inputs=2",
            "renderer_commercial_readiness_final_retained_root_artifact_import_host_gate_blocked_assembler_input_names=metal_memory_profiling_host_evidence,quality_vfx_host_evidence",
            "renderer_commercial_readiness_final_retained_root_artifact_import_quality_vfx_dependency_blockers=metal_memory_profiling_host_evidence"
        )) {
        Assert-LinePresent $hostGateManifestLines $expectedLine "GitHub artifact intake host-gated manifest mode"
    }
    $hostGateManifest = Get-Content -LiteralPath (ConvertTo-LocalPath "$contractRootRelative/intake-manifest.json") -Raw |
        ConvertFrom-Json
    if (@($hostGateManifest.assembler_input_blockers.PSObject.Properties).Count -ne 2) {
        Write-Error "GitHub artifact intake manifest must expose two host-gate blocked assembler input rows."
    }
    $qualityVfxBlocker = $hostGateManifest.assembler_input_blockers.quality_vfx_host_evidence
    if ($null -eq $qualityVfxBlocker) {
        Write-Error "GitHub artifact intake manifest must expose quality_vfx_host_evidence blocker details."
    }
    if (-not @($qualityVfxBlocker.dependent_missing_inputs).Contains("metal_memory_profiling_host_evidence")) {
        Write-Error "GitHub artifact intake quality/VFX blocker must name metal_memory_profiling_host_evidence as the missing dependency."
    }
    if (-not @($qualityVfxBlocker.host_gate_reasons).Contains("metal_memory_profiling_host_evidence_required")) {
        Write-Error "GitHub artifact intake quality/VFX blocker must retain its own Metal memory/profiling dependency reason."
    }
    if (@($qualityVfxBlocker.host_gate_reasons).Contains("mtlresidencyset_unavailable")) {
        Write-Error "GitHub artifact intake quality/VFX blocker must not absorb nested Metal memory/profiling host-gate reasons."
    }

    Write-JsonObject `
        -Path (ConvertTo-LocalPath "$finalRootOnlyContractRootRelative/renderer-commercial-readiness-final-retained-root/evidence.json") `
        -Value ([ordered]@{
            schema_version = "GameEngine.RendererCommercialReadinessEvidenceBundle.v1"
            fixture_only = $false
        })
    $finalRootOnlyLines = @(& $importerScript -Mode Inspect -OutputRootRelative $finalRootOnlyContractRootRelative)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_retained_root_artifact_import_present_assembler_inputs=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_inputs=7",
            "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_input_names=d3d12_host_evidence,vulkan_strict_host_evidence,apple_metal_host_evidence,metal_memory_profiling_host_evidence,package_host_evidence,quality_vfx_host_evidence,clean_room_legal_review",
            "renderer_commercial_readiness_final_retained_root_artifact_import_final_retained_root_present=1",
            "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_handoff_ready=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_final_preflight_handoff_ready=1",
            "renderer_commercial_readiness_final_retained_root_artifact_import_ready=1",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $finalRootOnlyLines $expectedLine "GitHub artifact intake final-root-only Inspect mode"
    }
    $finalRootOnlyManifest = Get-Content -LiteralPath (ConvertTo-LocalPath "$finalRootOnlyContractRootRelative/intake-manifest.json") -Raw |
        ConvertFrom-Json
    if (-not [bool]$finalRootOnlyManifest.final_preflight_handoff.ready) {
        Write-Error "GitHub artifact intake final preflight handoff must be ready when a final retained root exists."
    }
    if ([string]$finalRootOnlyManifest.final_preflight_handoff.script -cne "tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1") {
        Write-Error "GitHub artifact intake final preflight handoff must point at the final promotion preflight script."
    }
    if ([string]$finalRootOnlyManifest.final_preflight_handoff.artifact_root -cne "$finalRootOnlyContractRootRelative/renderer-commercial-readiness-final-retained-root") {
        Write-Error "GitHub artifact intake final preflight handoff must record the exact final retained-root artifact path."
    }
    if ([bool]$finalRootOnlyManifest.assembler_handoff.ready) {
        Write-Error "GitHub artifact intake assembler handoff must stay blocked when only a final retained root exists."
    }

    $multiRunInputSpecs = @(
        @{
            Path = "$multiRunContractRootRelative/renderer-d3d12-commercial-quality-host-evidence/d3d12-host-evidence.json"
            Schema = "GameEngine.RendererD3d12CommercialQualityHostEvidence.v1"
            Claim = "renderer-d3d12-commercial-quality-artifact-v1"
        },
        @{
            Path = "$multiRunContractRootRelative/renderer-vulkan-strict-commercial-quality-host-evidence/vulkan-host-evidence.json"
            Schema = "GameEngine.RendererVulkanStrictCommercialQualityHostEvidence.v1"
            Claim = "renderer-vulkan-strict-commercial-quality-artifact-v1"
        },
        @{
            Path = "$multiRunContractRootRelative/renderer-apple-metal-commercial-quality-host-evidence/apple-host-evidence.json"
            Schema = "GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1"
            Claim = "renderer-apple-metal-commercial-quality-artifact-v1"
        },
        @{
            Path = "$multiRunContractRootRelative/renderer-metal-memory-profiling-host-artifacts/evidence.json"
            Schema = "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1"
            Claim = "renderer-metal-memory-profiling-host-evidence-v1"
        },
        @{
            Path = "$multiRunContractRootRelative/renderer-package-commercial-quality-host-evidence/package-host-evidence.json"
            Schema = "GameEngine.RendererPackageCommercialQualityHostEvidence.v1"
            Claim = "renderer-package-commercial-quality-artifacts-v1"
        },
        @{
            Path = "$multiRunContractRootRelative/renderer-clean-room-legal-review-artifacts/clean-room-legal-review.json"
            Schema = "GameEngine.RendererCleanRoomLegalReviewInput.v1"
            Claim = "renderer-clean-room-legal-artifact-v1"
        }
    )
    foreach ($inputSpec in $multiRunInputSpecs) {
        Write-JsonObject `
            -Path (ConvertTo-LocalPath ([string]$inputSpec.Path)) `
            -Value (New-EvidenceStub -SchemaVersion ([string]$inputSpec.Schema) -ClaimId ([string]$inputSpec.Claim))
    }
    Write-Generated3dQualityVfxLog `
        -RelativePath "$multiRunContractRootRelative/renderer-package-commercial-quality-host-evidence/generated-3d-package.log"

    $multiRunLines = @(& $importerScript -Mode Inspect `
            -OutputRootRelative $multiRunContractRootRelative `
            -SupplementalRunIds "123456789" `
            -SupplementalArtifactNames "renderer-metal-memory-profiling-host-artifacts" `
            -RegenerateQualityVfx)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_retained_root_artifact_import_supplemental_run_count=1",
            "renderer_commercial_readiness_final_retained_root_artifact_import_supplemental_artifact_names=renderer-metal-memory-profiling-host-artifacts",
            "renderer_commercial_readiness_final_retained_root_artifact_import_quality_vfx_regenerate_requested=1",
            "renderer_commercial_readiness_final_retained_root_artifact_import_quality_vfx_regenerate_ran=1",
            "renderer_commercial_readiness_final_retained_root_artifact_import_quality_vfx_regenerate_ready=1",
            "renderer_commercial_readiness_final_retained_root_artifact_import_present_assembler_inputs=7",
            "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_inputs=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_ready=1",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $multiRunLines $expectedLine "GitHub artifact intake supplemental multi-run quality/VFX regeneration"
    }
    $multiRunManifest = Get-Content -LiteralPath (ConvertTo-LocalPath "$multiRunContractRootRelative/intake-manifest.json") -Raw |
        ConvertFrom-Json
    if (@($multiRunManifest.supplemental_import.run_ids).Count -ne 1) {
        Write-Error "GitHub artifact intake manifest must record the supplemental run id list."
    }
    if (-not [bool]$multiRunManifest.quality_vfx_regenerate.ready) {
        Write-Error "GitHub artifact intake quality/VFX regeneration must become ready when all source evidence is available."
    }
    if ([string]$multiRunManifest.assembler_inputs.quality_vfx_host_evidence.path -cne "$multiRunQualityRootRelative/quality-vfx-host-evidence.json") {
        Write-Error "GitHub artifact intake assembler handoff must use the regenerated quality/VFX host evidence path."
    }

    $inputSpecs = @(
        @{
            Path = "$contractRootRelative/input/d3d12/d3d12-host-evidence.json"
            Schema = "GameEngine.RendererD3d12CommercialQualityHostEvidence.v1"
            Claim = "renderer-d3d12-commercial-quality-artifact-v1"
        },
        @{
            Path = "$contractRootRelative/input/vulkan/vulkan-host-evidence.json"
            Schema = "GameEngine.RendererVulkanStrictCommercialQualityHostEvidence.v1"
            Claim = "renderer-vulkan-strict-commercial-quality-artifact-v1"
        },
        @{
            Path = "$contractRootRelative/input/apple/apple-host-evidence.json"
            Schema = "GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1"
            Claim = "renderer-apple-metal-commercial-quality-artifact-v1"
        },
        @{
            Path = "$contractRootRelative/input/metal-memory/evidence.json"
            Schema = "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1"
            Claim = "renderer-metal-memory-profiling-host-evidence-v1"
        },
        @{
            Path = "$contractRootRelative/input/package/package-host-evidence.json"
            Schema = "GameEngine.RendererPackageCommercialQualityHostEvidence.v1"
            Claim = "renderer-package-commercial-quality-artifacts-v1"
        },
        @{
            Path = "$contractRootRelative/input/quality-vfx/quality-vfx-host-evidence.json"
            Schema = "GameEngine.RendererQualityVfxCommercialHostEvidence.v1"
            Claim = "renderer-quality-vfx-commercial-artifacts-v1"
        },
        @{
            Path = "$contractRootRelative/input/legal/clean-room-legal-review.json"
            Schema = "GameEngine.RendererCleanRoomLegalReviewInput.v1"
            Claim = "renderer-clean-room-legal-artifact-v1"
        }
    )
    foreach ($inputSpec in $inputSpecs) {
        Write-JsonObject `
            -Path (ConvertTo-LocalPath ([string]$inputSpec.Path)) `
            -Value (New-EvidenceStub -SchemaVersion ([string]$inputSpec.Schema) -ClaimId ([string]$inputSpec.Claim))
    }

    $readyLines = @(& $importerScript -Mode Inspect -OutputRootRelative $contractRootRelative)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_retained_root_artifact_import_present_assembler_inputs=7",
            "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_inputs=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_handoff_ready=1",
            "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_handoff_required_input_paths=7",
            "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_handoff_output_root=$contractRootRelative/assembled-final-retained-root",
            "renderer_commercial_readiness_final_retained_root_artifact_import_final_preflight_handoff_ready=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_writes_evidence=1",
            "renderer_commercial_readiness_final_retained_root_artifact_import_ready=1",
            "renderer_backend_parity_ready=0",
            "renderer_metal_broad_readiness=0",
            "renderer_broad_quality_ready=0",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $readyLines $expectedLine "GitHub artifact intake complete Inspect mode"
    }

    if (-not (Test-Path -LiteralPath (ConvertTo-LocalPath "$contractRootRelative/intake-manifest.json") -PathType Leaf)) {
        Write-Error "GitHub artifact intake did not write intake-manifest.json."
    }
    $manifest = Get-Content -LiteralPath (ConvertTo-LocalPath "$contractRootRelative/intake-manifest.json") -Raw |
        ConvertFrom-Json
    if (-not [bool]$manifest.assembler_handoff.ready) {
        Write-Error "GitHub artifact intake manifest must include a ready assembler handoff when all seven inputs are present."
    }
    if (@($manifest.artifact_handoff_strategy.assembler_source_artifacts.missing_artifacts).Count -ne 0) {
        Write-Error "GitHub artifact intake manifest must expose zero missing assembler source artifacts when the complete source artifact set is available."
    }
    if (@($manifest.missing_assembler_inputs).Count -ne 0) {
        Write-Error "GitHub artifact intake manifest must expose zero missing assembler inputs when all seven inputs are present."
    }
    if (@($manifest.host_gate_summaries).Count -ne 3) {
        Write-Error "GitHub artifact intake manifest must expose every retained host gate summary."
    }
    $hostGateReasons = @($manifest.host_gate_summaries | ForEach-Object { [string]$_.reason })
    foreach ($expectedReason in @("mtlresidencyset_unavailable", "metal_memory_profiling_host_evidence_required")) {
        if (-not $hostGateReasons.Contains($expectedReason)) {
            Write-Error "GitHub artifact intake manifest missing host gate reason: $expectedReason"
        }
    }
    if ([string]$manifest.assembler_handoff.script -cne "tools/assemble-renderer-commercial-readiness-final-retained-root.ps1") {
        Write-Error "GitHub artifact intake manifest must point at the final retained-root assembler script."
    }
    if ([string]$manifest.assembler_handoff.output_root -cne "$contractRootRelative/assembled-final-retained-root") {
        Write-Error "GitHub artifact intake manifest must record the exact assembler output root."
    }
    if (@($manifest.assembler_handoff.input_paths.PSObject.Properties).Count -ne 7) {
        Write-Error "GitHub artifact intake manifest must record exactly seven assembler input paths."
    }
    $assemblerArguments = @($manifest.assembler_handoff.command_arguments)
    foreach ($expectedArgument in @(
            "-Mode",
            "Assemble",
            "-OutputRootRelative",
            "$contractRootRelative/assembled-final-retained-root",
            "-D3d12HostEvidenceRelative",
            "$contractRootRelative/input/d3d12/d3d12-host-evidence.json",
            "-VulkanStrictHostEvidenceRelative",
            "$contractRootRelative/input/vulkan/vulkan-host-evidence.json",
            "-AppleMetalHostEvidenceRelative",
            "$contractRootRelative/input/apple/apple-host-evidence.json",
            "-MetalMemoryProfilingHostEvidenceRelative",
            "$contractRootRelative/input/metal-memory/evidence.json",
            "-PackageHostEvidenceRelative",
            "$contractRootRelative/input/package/package-host-evidence.json",
            "-QualityVfxHostEvidenceRelative",
            "$contractRootRelative/input/quality-vfx/quality-vfx-host-evidence.json",
            "-CleanRoomLegalReviewRelative",
            "$contractRootRelative/input/legal/clean-room-legal-review.json"
        )) {
        if (-not $assemblerArguments.Contains($expectedArgument)) {
            Write-Error "GitHub artifact intake assembler handoff missing command argument: $expectedArgument"
        }
    }
    $requireReadyArguments = @($manifest.assembler_handoff.require_ready_command_arguments)
    if (-not $requireReadyArguments.Contains("-RequireReady")) {
        Write-Error "GitHub artifact intake assembler handoff must include a separate RequireReady command argument list."
    }
    if ([bool]$manifest.final_preflight_handoff.ready) {
        Write-Error "GitHub artifact intake final preflight handoff must stay blocked when no final retained root exists."
    }

    $autoAssembleLines = @(& $importerScript -Mode Inspect -OutputRootRelative $contractRootRelative -AutoAssemble)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_retained_root_artifact_import_auto_assemble_requested=1",
            "renderer_commercial_readiness_final_retained_root_artifact_import_auto_assemble_ran=1",
            "renderer_commercial_readiness_final_retained_root_artifact_import_auto_assemble_ready=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_auto_assemble_output_root=$contractRootRelative/assembled-final-retained-root",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $autoAssembleLines $expectedLine "GitHub artifact intake AutoAssemble Inspect mode"
    }
    $autoManifest = Get-Content -LiteralPath (ConvertTo-LocalPath "$contractRootRelative/intake-manifest.json") -Raw |
        ConvertFrom-Json
    if (-not [bool]$autoManifest.auto_assemble.requested) {
        Write-Error "GitHub artifact intake auto assemble manifest must record requested=true."
    }
    if (-not [bool]$autoManifest.auto_assemble.ran) {
        Write-Error "GitHub artifact intake auto assemble manifest must record ran=true when all seven inputs are present."
    }
    if ([bool]$autoManifest.auto_assemble.ready) {
        Write-Error "GitHub artifact intake auto assemble must not mark stub assembler inputs ready."
    }
    if ([string]$autoManifest.auto_assemble.output_root -cne "$contractRootRelative/assembled-final-retained-root") {
        Write-Error "GitHub artifact intake auto assemble manifest must record the exact assembler output root."
    }
    if ([string]::IsNullOrWhiteSpace([string]$autoManifest.auto_assemble.output_log)) {
        Write-Error "GitHub artifact intake auto assemble manifest must record the assembler output log path."
    }
}
finally {
    Remove-TestRoot -RelativePath $contractRootRelative
    Remove-TestRoot -RelativePath $finalRootOnlyContractRootRelative
    Remove-TestRoot -RelativePath $multiRunContractRootRelative
    Remove-TestRoot -RelativePath $multiRunQualityRootRelative
}

Write-Information "renderer-commercial-readiness-final-retained-root-artifact-import-check: ok" -InformationAction Continue
