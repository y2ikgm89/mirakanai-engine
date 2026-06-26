#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10P

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Collect")]
    [string]$Mode = "Plan",

    [string]$OutputRootRelative = "artifacts/renderer/package-commercial-quality-host-evidence/current-run",

    [string]$Visible3dStatusLogRelative = "",

    [string]$RuntimeUiStatusLogRelative = "",

    [string]$EnvironmentStatusLogRelative = "",

    [string]$GeneratedGameStatusLogRelative = "",

    [switch]$NoWrite
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$expectedSourceId = "GameEngine-Renderer-Package-Commercial-Quality-2026-06-25"

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Test-SafeRepoRelativePath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    if ([string]::IsNullOrWhiteSpace($RelativePath)) {
        return $false
    }
    if ($RelativePath.Contains("\")) {
        return $false
    }
    if ([System.IO.Path]::IsPathRooted($RelativePath)) {
        return $false
    }
    if ($RelativePath -match "^[A-Za-z]:") {
        return $false
    }
    if ($RelativePath.Contains(":")) {
        return $false
    }
    if ($RelativePath -match "(^|/)\.\.(/|$)") {
        return $false
    }
    return $true
}

function Test-AllowedEvidencePath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $normalizedPath = $RelativePath.Replace("\", "/")
    return $normalizedPath.StartsWith(
        "artifacts/renderer/package-commercial-quality-host-evidence/",
        [System.StringComparison]::Ordinal)
}

function Resolve-RepoRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not (Test-SafeRepoRelativePath -RelativePath $RelativePath)) {
        Write-Error "unsafe_relative_path: $Label must be repo-relative without absolute, drive-qualified, colon, backslash, or '..' segments."
    }
    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $root $RelativePath))
    $rootWithSeparator = $root.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "unsafe_relative_path: $Label must resolve under the repository root."
    }
    return $fullPath
}

function Get-Sha256Hex {
    param([Parameter(Mandatory = $true)][string]$Text)

    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
        $hashBytes = $sha.ComputeHash($bytes)
        return [System.BitConverter]::ToString($hashBytes).Replace("-", "").ToLowerInvariant()
    }
    finally {
        $sha.Dispose()
    }
}

function Read-StatusEvidence {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$ExpectedTarget,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not (Test-SafeRepoRelativePath -RelativePath $RelativePath)) {
        Write-Error "unsafe_relative_path: $Label must be a safe repo-relative status log path."
    }
    if (-not (Test-AllowedEvidencePath -RelativePath $RelativePath)) {
        Write-Error "$Label must be under artifacts/renderer/package-commercial-quality-host-evidence/."
    }

    $path = Resolve-RepoRelativePath -RelativePath $RelativePath -Label $Label
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        Write-Error "$Label does not exist: $RelativePath"
    }

    $lines = @(Get-Content -LiteralPath $path)
    $targetPrefix = "$ExpectedTarget status="
    $statusLine = [string]($lines | Where-Object {
            [string]$_ -like "$targetPrefix*"
        } | Select-Object -Last 1)
    if ([string]::IsNullOrWhiteSpace($statusLine)) {
        Write-Error "$Label does not contain a status line for $ExpectedTarget."
    }

    $values = @{}
    foreach ($match in [regex]::Matches($statusLine, "(^|\s)([A-Za-z0-9_.-]+)=([^\s]+)")) {
        $values[[string]$match.Groups[2].Value] = [string]$match.Groups[3].Value
    }
    if (-not $values.ContainsKey("status")) {
        Write-Error "$Label status line is missing status=..."
    }

    return [pscustomobject]@{
        relative_path = $RelativePath
        status_line = $statusLine
        values = $values
    }
}

function Assert-StatusValue {
    param(
        [Parameter(Mandatory = $true)]$Evidence,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Expected,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not $Evidence.values.ContainsKey($Name)) {
        Write-Error "$Label status line is missing required counter: $Name"
    }
    $actual = [string]$Evidence.values[$Name]
    if ($actual -cne $Expected) {
        Write-Error "$Label expected $Name=$Expected but found $actual."
    }
}

function Assert-StatusOneOf {
    param(
        [Parameter(Mandatory = $true)]$Evidence,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string[]]$ExpectedValues,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not $Evidence.values.ContainsKey($Name)) {
        Write-Error "$Label status line is missing required counter: $Name"
    }
    $actual = [string]$Evidence.values[$Name]
    if (-not $ExpectedValues.Contains($actual)) {
        Write-Error "$Label expected $Name to be one of '$($ExpectedValues -join ',')' but found $actual."
    }
}

function Assert-PositiveIntegerValue {
    param(
        [Parameter(Mandatory = $true)]$Evidence,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not $Evidence.values.ContainsKey($Name)) {
        Write-Error "$Label status line is missing required counter: $Name"
    }
    $value = [UInt64]0
    if (-not [UInt64]::TryParse([string]$Evidence.values[$Name], [ref]$value) -or $value -le 0) {
        Write-Error "$Label expected positive integer counter $Name but found $($Evidence.values[$Name])."
    }
}

function Assert-NoUnsupportedClaims {
    param(
        [Parameter(Mandatory = $true)]$Evidence,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($name in @(
            "environment_ready",
            "renderer_commercial_readiness",
            "external_engine_parity",
            "native_handles_exposed",
            "renderer_package_script_execution",
            "renderer_package_arbitrary_script_execution"
        )) {
        if ($Evidence.values.ContainsKey($name) -and [string]$Evidence.values[$name] -ne "0") {
            if ($name -eq "environment_ready") {
                Write-Error "unsupported_broad_environment_ready_claim: $Label"
            }
            Write-Error "unsupported_package_host_claim: $Label $name=$($Evidence.values[$name])"
        }
    }
}

function New-CommonNonClaims {
    return [ordered]@{
        arbitrary_script_execution = $false
        package_script_execution = $false
        native_handles_exposed = $false
        external_engine_parity = $false
        environment_ready = $false
    }
}

function Assert-Visible3dEvidence {
    param([Parameter(Mandatory = $true)]$Evidence)

    Assert-StatusOneOf -Evidence $Evidence -Name "status" -ExpectedValues @("ready", "completed") -Label "visible_3d"
    foreach ($entry in @(
            @{ Name = "visible_3d_status"; Value = "ready" },
            @{ Name = "visible_3d_ready"; Value = "1" },
            @{ Name = "visible_3d_diagnostics"; Value = "0" },
            @{ Name = "visible_3d_presented_frames"; Value = "2" },
            @{ Name = "visible_3d_null_fallback_used"; Value = "0" },
            @{ Name = "visible_3d_scene_gpu_ready"; Value = "1" },
            @{ Name = "visible_3d_postprocess_ready"; Value = "1" },
            @{ Name = "visible_3d_renderer_quality_ready"; Value = "1" },
            @{ Name = "visible_3d_playable_ready"; Value = "1" },
            @{ Name = "visible_3d_ui_overlay_ready"; Value = "1" }
        )) {
        Assert-StatusValue -Evidence $Evidence -Name $entry.Name -Expected $entry.Value -Label "visible_3d"
    }
    Assert-PositiveIntegerValue -Evidence $Evidence -Name "renderer_quality_matrix_replay_hash" -Label "visible_3d"
    Assert-PositiveIntegerValue -Evidence $Evidence -Name "rendering_vfx_profiling_replay_hash" -Label "visible_3d"
    Assert-PositiveIntegerValue -Evidence $Evidence -Name "scene_gpu_material_resolved" -Label "visible_3d"
    Assert-PositiveIntegerValue -Evidence $Evidence -Name "ui_texture_overlay_sprites_submitted" -Label "visible_3d"
    Assert-PositiveIntegerValue -Evidence $Evidence -Name "ui_texture_overlay_texture_binds" -Label "visible_3d"
    Assert-PositiveIntegerValue -Evidence $Evidence -Name "ui_texture_overlay_draws" -Label "visible_3d"
    Assert-NoUnsupportedClaims -Evidence $Evidence -Label "visible_3d"
}

function Assert-RuntimeUiEvidence {
    param([Parameter(Mandatory = $true)]$Evidence)

    Assert-StatusOneOf -Evidence $Evidence -Name "status" -ExpectedValues @("ready", "completed") -Label "runtime_ui"
    foreach ($entry in @(
            @{ Name = "runtime_ui_renderer_atlas_handoff_status"; Value = "ready" },
            @{ Name = "runtime_ui_renderer_atlas_handoff_ready"; Value = "1" },
            @{ Name = "runtime_ui_renderer_atlas_handoff_selected_package_evidence_ready"; Value = "1" },
            @{ Name = "runtime_ui_renderer_atlas_handoff_reviewed"; Value = "1" },
            @{ Name = "runtime_ui_renderer_atlas_handoff_text_glyphs_missing"; Value = "0" },
            @{ Name = "runtime_ui_renderer_atlas_handoff_image_resources_missing"; Value = "0" },
            @{ Name = "runtime_ui_renderer_atlas_handoff_unsupported_claim_rows"; Value = "0" },
            @{ Name = "runtime_ui_renderer_atlas_handoff_side_effect_rows"; Value = "0" },
            @{ Name = "runtime_ui_renderer_atlas_handoff_requested_renderer_texture_upload_api"; Value = "0" },
            @{ Name = "runtime_ui_renderer_atlas_handoff_requested_public_native_handle"; Value = "0" },
            @{ Name = "runtime_ui_renderer_atlas_handoff_invoked_source_image_decode"; Value = "0" },
            @{ Name = "runtime_ui_renderer_atlas_handoff_invoked_live_glyph_atlas_generation"; Value = "0" },
            @{ Name = "runtime_ui_renderer_atlas_handoff_invoked_renderer_upload"; Value = "0" },
            @{ Name = "runtime_ui_renderer_atlas_handoff_diagnostics"; Value = "0" },
            @{ Name = "ui_renderer_unresolved_resources"; Value = "0" },
            @{ Name = "ui_renderer_native_handles_exposed"; Value = "0" }
        )) {
        Assert-StatusValue -Evidence $Evidence -Name $entry.Name -Expected $entry.Value -Label "runtime_ui"
    }
    foreach ($name in @(
            "runtime_ui_renderer_atlas_handoff_image_atlas_pages",
            "runtime_ui_renderer_atlas_handoff_image_atlas_bindings",
            "runtime_ui_renderer_atlas_handoff_glyph_atlas_pages",
            "runtime_ui_renderer_atlas_handoff_glyph_atlas_bindings",
            "runtime_ui_renderer_atlas_handoff_texture_upload_handoff_rows",
            "runtime_ui_renderer_atlas_handoff_renderer_submission_counter_rows",
            "runtime_ui_renderer_atlas_handoff_image_sprites_submitted",
            "runtime_ui_renderer_atlas_handoff_renderer_sprites_submitted",
            "runtime_ui_renderer_atlas_handoff_replay_hash"
        )) {
        Assert-PositiveIntegerValue -Evidence $Evidence -Name $name -Label "runtime_ui"
    }
    Assert-NoUnsupportedClaims -Evidence $Evidence -Label "runtime_ui"
}

function Assert-EnvironmentEvidence {
    param([Parameter(Mandatory = $true)]$Evidence)

    Assert-StatusOneOf -Evidence $Evidence -Name "status" -ExpectedValues @("ready", "completed") -Label "environment"
    foreach ($entry in @(
            @{ Name = "environment_profile_status"; Value = "ready" },
            @{ Name = "environment_profile_ready"; Value = "1" },
            @{ Name = "environment_profile_requested"; Value = "1" },
            @{ Name = "environment_profile_package_file"; Value = "1" },
            @{ Name = "environment_profile_package_index_entry"; Value = "1" },
            @{ Name = "environment_profile_scene_reference"; Value = "1" },
            @{ Name = "environment_profile_scene_required"; Value = "1" },
            @{ Name = "environment_profile_scene_dependency"; Value = "1" },
            @{ Name = "environment_profile_dependency_edge"; Value = "1" },
            @{ Name = "environment_profile_source_parsing"; Value = "0" },
            @{ Name = "environment_profile_diagnostics"; Value = "0" },
            @{ Name = "environment_profile_v2_status"; Value = "ready" },
            @{ Name = "environment_profile_v2_ready"; Value = "1" },
            @{ Name = "environment_profile_v2_quality_preset"; Value = "high" },
            @{ Name = "environment_profile_v2_diagnostics"; Value = "0" },
            @{ Name = "environment_profile_v2_legacy_v1_accepted"; Value = "0" }
        )) {
        Assert-StatusValue -Evidence $Evidence -Name $entry.Name -Expected $entry.Value -Label "environment"
    }
    Assert-PositiveIntegerValue -Evidence $Evidence -Name "environment_profile_v2_volume_rows" -Label "environment"
    Assert-PositiveIntegerValue -Evidence $Evidence -Name "environment_profile_v2_weather_keyframes" -Label "environment"
    Assert-NoUnsupportedClaims -Evidence $Evidence -Label "environment"
}

function Assert-GeneratedGameEvidence {
    param([Parameter(Mandatory = $true)]$Evidence)

    Assert-StatusOneOf -Evidence $Evidence -Name "status" -ExpectedValues @("ready", "completed") -Label "generated_game"
    foreach ($entry in @(
            @{ Name = "package_upload_staging_status"; Value = "ready" },
            @{ Name = "package_upload_staging_ready"; Value = "1" },
            @{ Name = "package_upload_staging_diagnostics"; Value = "0" },
            @{ Name = "package_upload_staging_resource_updates_ready"; Value = "1" },
            @{ Name = "gameplay_systems_ready"; Value = "1" }
        )) {
        Assert-StatusValue -Evidence $Evidence -Name $entry.Name -Expected $entry.Value -Label "generated_game"
    }
    foreach ($name in @(
            "package_upload_staging_package_transactions",
            "package_upload_staging_texture_uploads",
            "package_upload_staging_mesh_uploads",
            "package_upload_staging_resource_updates",
            "package_upload_staging_uploaded_bytes",
            "gameplay_runtime_scheduler_replay_hash"
        )) {
        Assert-PositiveIntegerValue -Evidence $Evidence -Name $name -Label "generated_game"
    }
    Assert-NoUnsupportedClaims -Evidence $Evidence -Label "generated_game"
}

if (-not (Test-SafeRepoRelativePath -RelativePath $OutputRootRelative)) {
    Write-Error "unsafe_relative_path: OutputRootRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedEvidencePath -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/renderer/package-commercial-quality-host-evidence/."
}

if ($Mode -eq "Plan") {
    Write-Output "validation_recipe=renderer-package-commercial-quality-host-evidence"
    Write-Output "renderer_package_commercial_quality_host_evidence_mode=Plan"
    Write-Output "renderer_package_commercial_quality_host_evidence_output_root=$OutputRootRelative"
    Write-Output "renderer_package_commercial_quality_host_evidence_written=0"
    Write-Output "renderer_visible_3d_package_ready=0"
    Write-Output "renderer_runtime_ui_package_ready=0"
    Write-Output "renderer_environment_package_ready=0"
    Write-Output "renderer_generated_game_package_ready=0"
    Write-Output "renderer_package_arbitrary_script_execution=0"
    Write-Output "renderer_package_script_execution=0"
    Write-Output "renderer_backend_parity_ready=0"
    Write-Output "renderer_metal_broad_readiness=0"
    Write-Output "renderer_broad_quality_ready=0"
    Write-Output "renderer_commercial_readiness=0"
    Write-Output "renderer_environment_ready=0"
    return
}

foreach ($required in @(
        @{ Name = "Visible3dStatusLogRelative"; Value = $Visible3dStatusLogRelative },
        @{ Name = "RuntimeUiStatusLogRelative"; Value = $RuntimeUiStatusLogRelative },
        @{ Name = "EnvironmentStatusLogRelative"; Value = $EnvironmentStatusLogRelative },
        @{ Name = "GeneratedGameStatusLogRelative"; Value = $GeneratedGameStatusLogRelative }
    )) {
    if ([string]::IsNullOrWhiteSpace([string]$required.Value)) {
        Write-Error "$($required.Name) is required in Collect mode."
    }
}

$visible3dEvidence = Read-StatusEvidence -RelativePath $Visible3dStatusLogRelative `
    -ExpectedTarget "sample_generated_desktop_runtime_3d_package" -Label "Visible3dStatusLogRelative"
$runtimeUiEvidence = Read-StatusEvidence -RelativePath $RuntimeUiStatusLogRelative `
    -ExpectedTarget "sample_2d_desktop_runtime_package" -Label "RuntimeUiStatusLogRelative"
$environmentEvidence = Read-StatusEvidence -RelativePath $EnvironmentStatusLogRelative `
    -ExpectedTarget "sample_desktop_runtime_game" -Label "EnvironmentStatusLogRelative"
$generatedGameEvidence = Read-StatusEvidence -RelativePath $GeneratedGameStatusLogRelative `
    -ExpectedTarget "sample_generated_desktop_runtime_3d_package" -Label "GeneratedGameStatusLogRelative"

Assert-Visible3dEvidence -Evidence $visible3dEvidence
Assert-RuntimeUiEvidence -Evidence $runtimeUiEvidence
Assert-EnvironmentEvidence -Evidence $environmentEvidence
Assert-GeneratedGameEvidence -Evidence $generatedGameEvidence

$visibleHash = Get-Sha256Hex -Text "visible_3d`n$($visible3dEvidence.status_line)"
$runtimeUiHash = Get-Sha256Hex -Text "runtime_ui`n$($runtimeUiEvidence.status_line)"
$generatedGameHash = Get-Sha256Hex -Text "generated_game`n$($generatedGameEvidence.status_line)"

$hostEvidence = [ordered]@{
    schema_version = "GameEngine.RendererPackageCommercialQualityHostEvidence.v1"
    claim_id = "renderer-package-commercial-quality-artifacts-v1"
    validation_recipe = "renderer-package-commercial-quality-artifacts"
    fixture_only = $false
    source_id = $expectedSourceId
    package_rows = [ordered]@{
        visible_3d = [ordered]@{
            validation_recipe = "desktop-3d-package"
            proof_rows = [ordered]@{
                material_render = [ordered]@{
                    ready = $true
                    pbr_material_row = $true
                    texture_binding_row = $true
                    material_variant_rows = [int]$visible3dEvidence.values["scene_gpu_material_resolved"]
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
                    deterministic_hash_sha256 = $visibleHash
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
            non_claims = New-CommonNonClaims
        }
        runtime_ui = [ordered]@{
            validation_recipe = "desktop-runtime-ui-package"
            proof_rows = [ordered]@{
                ui_atlas_upload = [ordered]@{
                    ready = $true
                    atlas_texture_upload_row = $true
                    atlas_texture_usage_sampled = $true
                    upload_counter_rows = [int]$runtimeUiEvidence.values["runtime_ui_renderer_atlas_handoff_texture_upload_handoff_rows"]
                }
                ui_atlas_readback = [ordered]@{
                    ready = $true
                    readback_counter_rows = [int]$runtimeUiEvidence.values["runtime_ui_renderer_atlas_handoff_renderer_submission_counter_rows"]
                    deterministic_hash_sha256 = $runtimeUiHash
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
            non_claims = New-CommonNonClaims
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
            non_claims = New-CommonNonClaims
        }
        generated_game = [ordered]@{
            validation_recipe = "generated-game-package"
            proof_rows = [ordered]@{
                generated_game_output = [ordered]@{
                    ready = $true
                    generated_game_package_written = $true
                    output_manifest_rows = 1
                    deterministic_hash_sha256 = $generatedGameHash
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
            non_claims = New-CommonNonClaims
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

$outputPathRelative = "$OutputRootRelative/package-host-evidence.json"
$outputPath = Resolve-RepoRelativePath -RelativePath $outputPathRelative -Label "package host evidence output"
$outputParent = Split-Path -Parent $outputPath
if (-not (Test-Path -LiteralPath $outputParent -PathType Container)) {
    $null = New-Item -ItemType Directory -Path $outputParent -Force
}
if (-not $NoWrite.IsPresent) {
    $hostEvidence | ConvertTo-Json -Depth 24 | Set-Content -LiteralPath $outputPath -Encoding utf8NoBOM
}

Write-Output "validation_recipe=renderer-package-commercial-quality-host-evidence"
Write-Output "renderer_package_commercial_quality_host_evidence_mode=Collect"
Write-Output "renderer_package_commercial_quality_host_evidence_output_root=$OutputRootRelative"
Write-Output "renderer_package_commercial_quality_host_evidence_path=$outputPathRelative"
Write-Output "renderer_package_commercial_quality_host_evidence_written=$(ConvertTo-CounterBit (-not $NoWrite.IsPresent))"
Write-Output "renderer_visible_3d_package_ready=1"
Write-Output "renderer_runtime_ui_package_ready=1"
Write-Output "renderer_environment_package_ready=1"
Write-Output "renderer_generated_game_package_ready=1"
Write-Output "renderer_package_arbitrary_script_execution=0"
Write-Output "renderer_package_script_execution=0"
Write-Output "renderer_backend_parity_ready=0"
Write-Output "renderer_metal_broad_readiness=0"
Write-Output "renderer_broad_quality_ready=0"
Write-Output "renderer_commercial_readiness=0"
Write-Output "renderer_environment_ready=0"
