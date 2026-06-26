#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Join-Path $root ($RelativePath -replace "/", [System.IO.Path]::DirectorySeparatorChar)
}

function Write-TextFile {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Content
    )

    $path = ConvertTo-LocalPath $RelativePath
    $parent = Split-Path -Parent $path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        $null = New-Item -ItemType Directory -Path $parent -Force
    }
    Set-Content -LiteralPath $path -Value $Content -Encoding utf8NoBOM
}

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

$hostProducerScript = Join-Path $PSScriptRoot "collect-renderer-package-commercial-quality-host-evidence.ps1"
$packageArtifactProducerScript = Join-Path $PSScriptRoot "collect-renderer-package-commercial-quality-artifacts.ps1"
if (-not (Test-Path -LiteralPath $hostProducerScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-package-commercial-quality-host-evidence.ps1 must exist for retained package host evidence intake."
}
if (-not (Test-Path -LiteralPath $packageArtifactProducerScript -PathType Leaf)) {
    Write-Error "tools/collect-renderer-package-commercial-quality-artifacts.ps1 must exist for retained package artifact assembly."
}

$evidenceRootRelative = "artifacts/renderer/package-commercial-quality-host-evidence/contract-$PID"
$inputRootRelative = "$evidenceRootRelative/input"
$hostOutputRootRelative = "$evidenceRootRelative/output"
$artifactOutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/package-host-evidence-contract-$PID/output"
$visibleLogRelative = "$inputRootRelative/visible-3d.log"
$runtimeUiLogRelative = "$inputRootRelative/runtime-ui.log"
$environmentLogRelative = "$inputRootRelative/environment.log"
$generatedGameLogRelative = "$inputRootRelative/generated-game.log"
$hostEvidenceRelative = "$hostOutputRootRelative/package-host-evidence.json"
$evidenceRootPath = ConvertTo-LocalPath $evidenceRootRelative
$artifactOutputRootPath = ConvertTo-LocalPath "artifacts/renderer/commercial-readiness-evidence/package-host-evidence-contract-$PID"

try {
    foreach ($path in @($evidenceRootPath, $artifactOutputRootPath)) {
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -Recurse -Force
        }
    }

    Write-TextFile -RelativePath $visibleLogRelative -Content @"
sample_generated_desktop_runtime_3d_package status=completed visible_3d_status=not_requested visible_3d_ready=0
sample_generated_desktop_runtime_3d_package status=completed visible_3d_status=ready visible_3d_ready=1 visible_3d_diagnostics=0 visible_3d_expected_frames=2 visible_3d_presented_frames=2 visible_3d_d3d12_selected=1 visible_3d_vulkan_selected=0 visible_3d_null_fallback_used=0 visible_3d_scene_gpu_ready=1 visible_3d_postprocess_ready=1 visible_3d_renderer_quality_ready=1 visible_3d_playable_ready=1 visible_3d_ui_overlay_ready=1 renderer_quality_matrix_replay_hash=12953007953522728565 rendering_vfx_profiling_replay_hash=5030402595252288520 scene_gpu_material_resolved=2 ui_texture_overlay_sprites_submitted=2 ui_texture_overlay_texture_binds=2 ui_texture_overlay_draws=2 renderer_package_script_execution=0 renderer_package_arbitrary_script_execution=0 renderer_commercial_readiness=0 environment_ready=0
"@

    Write-TextFile -RelativePath $runtimeUiLogRelative -Content @"
sample_2d_desktop_runtime_package status=completed runtime_ui_renderer_atlas_handoff_status=not_requested runtime_ui_renderer_atlas_handoff_ready=0
sample_2d_desktop_runtime_package status=completed runtime_ui_renderer_atlas_handoff_status=ready runtime_ui_renderer_atlas_handoff_ready=1 runtime_ui_renderer_atlas_handoff_selected_package_evidence_ready=1 runtime_ui_renderer_atlas_handoff_reviewed=1 runtime_ui_renderer_atlas_handoff_image_atlas_pages=1 runtime_ui_renderer_atlas_handoff_image_atlas_bindings=1 runtime_ui_renderer_atlas_handoff_glyph_atlas_pages=1 runtime_ui_renderer_atlas_handoff_glyph_atlas_bindings=1 runtime_ui_renderer_atlas_handoff_atlas_placement_rows=2 runtime_ui_renderer_atlas_handoff_atlas_budget_rows=2 runtime_ui_renderer_atlas_handoff_atlas_eviction_diagnostic_rows=1 runtime_ui_renderer_atlas_handoff_texture_upload_handoff_rows=1 runtime_ui_renderer_atlas_handoff_renderer_submission_counter_rows=1 runtime_ui_renderer_atlas_handoff_text_glyphs_available=1 runtime_ui_renderer_atlas_handoff_text_glyphs_resolved=1 runtime_ui_renderer_atlas_handoff_text_glyphs_missing=0 runtime_ui_renderer_atlas_handoff_text_glyph_sprites_submitted=1 runtime_ui_renderer_atlas_handoff_image_placeholders_available=1 runtime_ui_renderer_atlas_handoff_image_resources_resolved=1 runtime_ui_renderer_atlas_handoff_image_resources_missing=0 runtime_ui_renderer_atlas_handoff_image_sprites_submitted=1 runtime_ui_renderer_atlas_handoff_renderer_sprites_submitted=2 runtime_ui_renderer_atlas_handoff_unsupported_claim_rows=0 runtime_ui_renderer_atlas_handoff_side_effect_rows=0 runtime_ui_renderer_atlas_handoff_requested_renderer_texture_upload_api=0 runtime_ui_renderer_atlas_handoff_requested_public_native_handle=0 runtime_ui_renderer_atlas_handoff_invoked_source_image_decode=0 runtime_ui_renderer_atlas_handoff_invoked_live_glyph_atlas_generation=0 runtime_ui_renderer_atlas_handoff_invoked_renderer_upload=0 runtime_ui_renderer_atlas_handoff_diagnostics=0 runtime_ui_renderer_atlas_handoff_replay_hash=48291 ui_renderer_layers=1 ui_renderer_batches=1 ui_renderer_clip_rects=1 ui_renderer_unresolved_resources=0 ui_renderer_native_handles_exposed=0 renderer_commercial_readiness=0 environment_ready=0
"@

    Write-TextFile -RelativePath $environmentLogRelative -Content @"
sample_desktop_runtime_game status=completed environment_profile_status=not_requested environment_profile_ready=0
sample_desktop_runtime_game status=completed environment_profile_status=ready environment_profile_ready=1 environment_profile_requested=1 environment_profile_package_file=1 environment_profile_package_index_entry=1 environment_profile_scene_reference=1 environment_profile_scene_required=1 environment_profile_scene_dependency=1 environment_profile_dependency_edge=1 environment_profile_source_parsing=0 environment_profile_diagnostics=0 environment_profile_v2_status=ready environment_profile_v2_ready=1 environment_profile_v2_quality_preset=high environment_profile_v2_diagnostics=0 environment_profile_v2_legacy_v1_accepted=0 environment_profile_v2_volume_rows=2 environment_profile_v2_weather_keyframes=2 renderer_package_script_execution=0 renderer_package_arbitrary_script_execution=0 renderer_commercial_readiness=0 environment_ready=0
"@

    Write-TextFile -RelativePath $generatedGameLogRelative -Content @"
sample_generated_desktop_runtime_3d_package status=completed package_upload_staging_status=not_requested package_upload_staging_ready=0
sample_generated_desktop_runtime_3d_package status=completed package_upload_staging_status=ready package_upload_staging_ready=1 package_upload_staging_diagnostics=0 package_upload_staging_package_transactions=4 package_upload_staging_texture_uploads=1 package_upload_staging_mesh_uploads=1 package_upload_staging_skinned_mesh_uploads=1 package_upload_staging_morph_mesh_uploads=1 package_upload_staging_resource_updates_ready=1 package_upload_staging_resource_updates=4 package_upload_staging_uploaded_bytes=16384 gameplay_systems_ready=1 gameplay_runtime_scheduler_replay_hash=77123 renderer_package_script_execution=0 renderer_package_arbitrary_script_execution=0 renderer_commercial_readiness=0 environment_ready=0
"@

    $planLines = @(& $hostProducerScript -Mode Plan -OutputRootRelative $hostOutputRootRelative)
    foreach ($expectedLine in @(
            "renderer_package_commercial_quality_host_evidence_mode=Plan",
            "renderer_package_commercial_quality_host_evidence_written=0",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        )) {
        Assert-LinePresent $planLines $expectedLine "package host evidence producer Plan mode"
    }

    $unsafeRejected = $false
    try {
        $null = & $hostProducerScript `
            -Mode Collect `
            -OutputRootRelative $hostOutputRootRelative `
            -Visible3dStatusLogRelative "../unsafe.log" `
            -RuntimeUiStatusLogRelative $runtimeUiLogRelative `
            -EnvironmentStatusLogRelative $environmentLogRelative `
            -GeneratedGameStatusLogRelative $generatedGameLogRelative 2>&1
    }
    catch {
        $unsafeRejected = [string]$_.Exception.Message -like "*unsafe_relative_path*"
    }
    if (-not $unsafeRejected) {
        Write-Error "package host evidence producer must reject unsafe status-log paths."
    }

    $environmentReadyRejected = $false
    $unsafeEnvironmentLogRelative = "$inputRootRelative/environment-ready-claimed.log"
    Write-TextFile -RelativePath $unsafeEnvironmentLogRelative -Content @"
sample_desktop_runtime_game status=completed environment_profile_status=ready environment_profile_ready=1 environment_profile_requested=1 environment_profile_package_file=1 environment_profile_package_index_entry=1 environment_profile_scene_reference=1 environment_profile_scene_required=1 environment_profile_scene_dependency=1 environment_profile_dependency_edge=1 environment_profile_source_parsing=0 environment_profile_diagnostics=0 environment_profile_v2_status=ready environment_profile_v2_ready=1 environment_profile_v2_quality_preset=high environment_profile_v2_diagnostics=0 environment_profile_v2_legacy_v1_accepted=0 environment_profile_v2_volume_rows=2 environment_profile_v2_weather_keyframes=2 environment_ready=1
"@
    try {
        $null = & $hostProducerScript `
            -Mode Collect `
            -OutputRootRelative $hostOutputRootRelative `
            -Visible3dStatusLogRelative $visibleLogRelative `
            -RuntimeUiStatusLogRelative $runtimeUiLogRelative `
            -EnvironmentStatusLogRelative $unsafeEnvironmentLogRelative `
            -GeneratedGameStatusLogRelative $generatedGameLogRelative 2>&1
    }
    catch {
        $environmentReadyRejected = [string]$_.Exception.Message -like "*unsupported_broad_environment_ready_claim*"
    }
    if (-not $environmentReadyRejected) {
        Write-Error "package host evidence producer must reject broad environment_ready claims."
    }

    $collectLines = @(& $hostProducerScript `
            -Mode Collect `
            -OutputRootRelative $hostOutputRootRelative `
            -Visible3dStatusLogRelative $visibleLogRelative `
            -RuntimeUiStatusLogRelative $runtimeUiLogRelative `
            -EnvironmentStatusLogRelative $environmentLogRelative `
            -GeneratedGameStatusLogRelative $generatedGameLogRelative)
    foreach ($expectedLine in @(
            "renderer_package_commercial_quality_host_evidence_mode=Collect",
            "renderer_package_commercial_quality_host_evidence_written=1",
            "renderer_package_commercial_quality_host_evidence_path=$hostEvidenceRelative",
            "renderer_visible_3d_package_ready=1",
            "renderer_runtime_ui_package_ready=1",
            "renderer_environment_package_ready=1",
            "renderer_generated_game_package_ready=1",
            "renderer_package_script_execution=0",
            "renderer_package_arbitrary_script_execution=0",
            "renderer_backend_parity_ready=0",
            "renderer_metal_broad_readiness=0",
            "renderer_broad_quality_ready=0",
            "renderer_commercial_readiness=0",
            "renderer_environment_ready=0"
        )) {
        Assert-LinePresent $collectLines $expectedLine "package host evidence producer Collect mode"
    }

    $hostEvidencePath = ConvertTo-LocalPath $hostEvidenceRelative
    if (-not (Test-Path -LiteralPath $hostEvidencePath -PathType Leaf)) {
        Write-Error "package host evidence producer did not write package-host-evidence.json."
    }
    $hostEvidence = Get-Content -LiteralPath $hostEvidencePath -Raw | ConvertFrom-Json
    if ([string]$hostEvidence.schema_version -ne "GameEngine.RendererPackageCommercialQualityHostEvidence.v1") {
        Write-Error "package host evidence schema_version mismatch."
    }
    if ([bool]$hostEvidence.fixture_only) {
        Write-Error "package host evidence must not be fixture_only."
    }

    $artifactLines = @(& $packageArtifactProducerScript `
            -Mode Assemble `
            -OutputRootRelative $artifactOutputRootRelative `
            -PackageHostEvidenceRelative $hostEvidenceRelative)
    Assert-LinePresent $artifactLines `
        "renderer_package_commercial_quality_artifacts_written=4" `
        "package artifact producer with generated host evidence"
    Assert-LinePresent $artifactLines `
        "renderer_commercial_readiness=0" `
        "package artifact producer with generated host evidence"
}
finally {
    foreach ($path in @($evidenceRootPath, $artifactOutputRootPath)) {
        if (Test-Path -LiteralPath $path) {
            Remove-Item -LiteralPath $path -Recurse -Force
        }
    }
}

Write-Information "renderer-package-commercial-quality-host-evidence-check: ok" -InformationAction Continue
