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
    $allowedRoot = [System.IO.Path]::GetFullPath(
        (ConvertTo-LocalPath "artifacts/renderer/commercial-readiness-evidence")).TrimEnd(
        [System.IO.Path]::DirectorySeparatorChar) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($allowedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "Refusing to remove test root outside artifacts/renderer/commercial-readiness-evidence: $fullPath"
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

$importerScript = Join-Path $root "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1"
if (-not (Test-Path -LiteralPath $importerScript -PathType Leaf)) {
    Write-Error "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1 must exist."
}

$contractRootRelative = "artifacts/renderer/commercial-readiness-evidence/final-retained-root-artifact-import-contract-$PID"

try {
    Remove-TestRoot -RelativePath $contractRootRelative

    $planLines = @(& $importerScript -Mode Plan -OutputRootRelative $contractRootRelative)
    foreach ($expectedLine in @(
            "validation_recipe=renderer-commercial-readiness-final-retained-root-artifact-import",
            "renderer_commercial_readiness_final_retained_root_artifact_import_mode=Plan",
            "renderer_commercial_readiness_final_retained_root_artifact_import_required_workflow_artifacts=6",
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
            "renderer_commercial_readiness_final_retained_root_artifact_import_final_retained_root_present=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_metal_host_gate_summary_present=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_writes_evidence=0",
            "renderer_commercial_readiness_final_retained_root_artifact_import_ready=0"
        )) {
        Assert-LinePresent $missingLines $expectedLine "GitHub artifact intake missing Inspect mode"
    }

    $artifactListPath = "$contractRootRelative/github-artifacts.json"
    Write-JsonObject -Path (ConvertTo-LocalPath $artifactListPath) -Value ([ordered]@{
            total_count = 4
            artifacts = @(
                [ordered]@{ name = "windows-packages"; expired = $false },
                [ordered]@{ name = "linux-vulkan-host-evidence"; expired = $false },
                [ordered]@{ name = "renderer-metal-memory-profiling-host-artifacts"; expired = $false },
                [ordered]@{ name = "metal-host-optimization-artifacts"; expired = $false }
            )
        })
    $artifactListLines = @(& $importerScript -Mode Inspect `
            -OutputRootRelative $contractRootRelative `
            -ArtifactListJsonRelative $artifactListPath `
            -NoWrite)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_retained_root_artifact_import_workflow_artifact_list_present=1",
            "renderer_commercial_readiness_final_retained_root_artifact_import_available_workflow_artifacts=4",
            "renderer_commercial_readiness_final_retained_root_artifact_import_missing_workflow_artifacts=2",
            "renderer_commercial_readiness_final_retained_root_artifact_import_missing_workflow_artifact_names=renderer-clean-room-legal-review-artifacts,renderer-commercial-readiness-final-retained-root",
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
    $hostGateLines = @(& $importerScript -Mode Inspect -OutputRootRelative $contractRootRelative -NoWrite)
    foreach ($expectedLine in @(
            "renderer_commercial_readiness_final_retained_root_artifact_import_metal_host_gate_summary_present=1",
            "renderer_commercial_readiness_final_retained_root_artifact_import_metal_host_gate_status=host_gated",
            "renderer_commercial_readiness_final_retained_root_artifact_import_metal_host_gate_reason=mtlresidencyset_unavailable",
            "renderer_commercial_readiness_final_retained_root_artifact_import_ready=0",
            "renderer_commercial_readiness=0"
        )) {
        Assert-LinePresent $hostGateLines $expectedLine "GitHub artifact intake host-gated Inspect mode"
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
}
finally {
    Remove-TestRoot -RelativePath $contractRootRelative
}

Write-Information "renderer-commercial-readiness-final-retained-root-artifact-import-check: ok" -InformationAction Continue
