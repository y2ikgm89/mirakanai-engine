#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [switch]$RequireReady
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$gameManifestPath = "games/sample_2d_desktop_runtime_package/game.agent.json"
$specPath = "docs/specs/2026-06-19-2d-production-workload-matrix-v1.md"
$requiredWorkloads = @(
    @{
        Id = "2d-dense-arena"
        Status = "ready"
        Recipes = @("installed-2d-sprite-throughput-smoke")
        Budgets = @("dense-arena-visible-sprites", "projectile-storm-logical-sprites", "sprite-throughput-upload-bytes")
        Evidence = @("sprite-throughput-workload-smoke")
    },
    @{
        Id = "2d-sandbox-tilemap"
        Status = "ready"
        Recipes = @("installed-2d-sandbox-package-budget-smoke", "installed-2d-tilemap-runtime-ux-smoke")
        Budgets = @("sandbox-tile-visible-cells", "sandbox-tile-draw-rows", "sandbox-streaming-load-rows")
        Evidence = @("sandbox-tilemap-workload-smoke")
    },
    @{
        Id = "2d-ui-overlay"
        Status = "ready"
        Recipes = @("installed-2d-runtime-ui-renderer-atlas-handoff-smoke")
        Budgets = @("ui-overlay-renderer-sprites", "ui-overlay-atlas-budget-rows")
        Evidence = @("ui-overlay-workload-smoke")
    },
    @{
        Id = "2d-hot-reload-package"
        Status = "host-gated"
        Recipes = @("installed-2d-package-smoke")
        Budgets = @("hot-reload-reviewed-replacement-rows", "hot-reload-unsafe-execution-claims")
        Evidence = @("hot-reload-package-playtest-evidence")
    },
    @{
        Id = "2d-long-run-selected"
        Status = "ready"
        Recipes = @("installed-2d-long-run-readiness-smoke", "host-2d-long-run-readiness-soak")
        Budgets = @("long-run-sample-frames", "long-run-over-budget-frames", "long-run-memory-growth-bytes")
        Evidence = @("long-run-selected-smoke", "long-run-host-soak-evidence")
    }
)
$requiredUnsupportedClaims = @(
    "broad-optimized-game",
    "cross-vendor-performance-parity",
    "cross-backend-performance-parity",
    "native-handles"
)
$readySmokeExpectations = @{
    "2d_sprite_throughput_status" = "ready"
    "2d_sprite_throughput_ready" = "1"
    "2d_sprite_throughput_workload_rows" = "3"
    "2d_sprite_throughput_dense_arena_4096_visible_sprites" = "4096"
    "2d_sprite_throughput_projectile_storm_logical_sprites" = "12000"
    "2d_sprite_throughput_upload_bytes" = "319488"
    "2d_sprite_throughput_over_budget_rows" = "0"
    "2d_sprite_throughput_diagnostics" = "0"
    "2d_sprite_throughput_claimed_broad_optimization" = "0"
    "2d_sprite_throughput_claimed_cross_backend_parity" = "0"
    "2d_sprite_throughput_claimed_metal_readiness" = "0"
    "sandbox_package_budget_status" = "ready"
    "sandbox_package_budget_diagnostics" = "0"
    "sandbox_package_budget_streaming_load_rows" = "1"
    "sandbox_package_budget_broad_renderer_quality_ready" = "0"
    "tilemap_diagnostics" = "0"
    "tile_chunk_renderer_status" = "ready"
    "tile_chunk_renderer_ready" = "1"
    "tile_chunk_renderer_visible_cells" = "3"
    "tile_chunk_renderer_draw_rows" = "2"
    "tile_chunk_renderer_backend_submission_invoked" = "0"
    "tile_chunk_renderer_native_texture_ownership_invoked" = "0"
    "runtime_ui_renderer_atlas_handoff_status" = "ready"
    "runtime_ui_renderer_atlas_handoff_ready" = "1"
    "runtime_ui_renderer_atlas_handoff_renderer_sprites_submitted" = "2"
    "runtime_ui_renderer_atlas_handoff_atlas_budget_rows" = "2"
    "runtime_ui_renderer_atlas_handoff_requested_renderer_texture_upload_api" = "0"
    "runtime_ui_renderer_atlas_handoff_requested_public_native_handle" = "0"
    "runtime_ui_renderer_atlas_handoff_invoked_source_image_decode" = "0"
    "runtime_ui_renderer_atlas_handoff_invoked_renderer_upload" = "0"
    "runtime_ui_renderer_atlas_handoff_diagnostics" = "0"
    "ui_renderer_layers" = "1"
    "ui_renderer_batches" = "1"
    "ui_renderer_clip_rects" = "1"
    "ui_renderer_unresolved_resources" = "0"
    "ui_renderer_native_handles_exposed" = "0"
    "performance_baseline_status" = "ready"
    "performance_baseline_frame_p95_us" = "16000"
    "performance_baseline_frame_p99_us" = "16000"
    "performance_baseline_over_budget" = "0"
    "long_run_readiness_status" = "ready"
    "long_run_readiness_ready" = "1"
    "long_run_readiness_frames" = "3"
    "long_run_readiness_over_budget_frames" = "0"
    "long_run_readiness_memory_growth_bytes" = "0"
    "long_run_readiness_gpu_async_overlap_applied" = "0"
    "long_run_readiness_cuda_path_used" = "0"
    "long_run_readiness_hip_path_used" = "0"
    "long_run_readiness_sycl_path_used" = "0"
    "long_run_readiness_native_handles_exposed" = "0"
    "2d_input_device_production_ux_status" = "ready"
    "2d_input_device_production_ux_diagnostics" = "0"
    "2d_input_device_production_ux_ui_rendering_rows" = "0"
    "2d_input_device_production_ux_native_handle_access_rows" = "0"
}

function Get-ObjectPropertyValue {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }
    return $property.Value
}

function Assert-ObjectProperties {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string[]]$Names,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($name in $Names) {
        if ($null -eq $Object.PSObject.Properties[$name]) {
            Write-Error "$Label missing required property: $name"
        }
    }
}

function New-Set {
    param([Parameter(Mandatory = $true)][object[]]$Values)

    $set = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($value in @($Values)) {
        $null = $set.Add([string]$value)
    }
    return $set
}

function Assert-ContainsAll {
    param(
        [Parameter(Mandatory = $true)]$Set,
        [Parameter(Mandatory = $true)][string[]]$Expected,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($item in $Expected) {
        if (-not $Set.Contains($item)) {
            Write-Error "$Label missing required row: $item"
        }
    }
}

function Assert-SafeArtifactPath {
    param(
        [Parameter(Mandatory = $true)][string]$ArtifactPath,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($ArtifactPath) -or
        $ArtifactPath.Contains("\") -or
        $ArtifactPath.StartsWith("/", [System.StringComparison]::Ordinal) -or
        $ArtifactPath -match "^[A-Za-z]:" -or
        $ArtifactPath.Contains(":") -or
        $ArtifactPath -match "(^|/)\.\.(/|$)") {
        Write-Error "$Label artifactPath must be a safe repository-relative path: $ArtifactPath"
    }
}

function ConvertFrom-StatusLine {
    param([Parameter(Mandatory = $true)][string[]]$Output)

    $statusLine = @($Output | Where-Object { $_ -match "^sample_2d_desktop_runtime_package status=" } | Select-Object -Last 1)
    if ($statusLine.Count -eq 0) {
        Write-Error "2d-production-workloads: package smoke did not emit sample_2d_desktop_runtime_package status line"
    }

    $fields = @{}
    foreach ($match in [regex]::Matches($statusLine[0], '([A-Za-z0-9_]+)=([^ ]+)')) {
        $fields[$match.Groups[1].Value] = $match.Groups[2].Value
    }
    return $fields
}

function Assert-SmokeField {
    param(
        [Parameter(Mandatory = $true)]$Fields,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Expected
    )

    if (-not $Fields.ContainsKey($Name)) {
        Write-Error "2d-production-workloads: package smoke missing counter $Name"
    }
    if ([string]$Fields[$Name] -ne $Expected) {
        Write-Error "2d-production-workloads: package smoke counter $Name expected $Expected but was $($Fields[$Name])"
    }
}

if (-not (Test-Path -LiteralPath (Join-Path $root $specPath) -PathType Leaf)) {
    Write-Error "2d-production-workloads: missing spec $specPath"
}

$game = Read-Json -RelativePath $gameManifestPath -Root $root
$budget = Get-ObjectPropertyValue -Object $game -Name "performanceBudgets"
if ($null -eq $budget) {
    Write-Error "$gameManifestPath must declare performanceBudgets"
}

Assert-ObjectProperties $budget @(
    "budgetRows",
    "evidenceRows",
    "workloadRows",
    "validationRecipeIds",
    "unsupportedClaims"
) "$gameManifestPath performanceBudgets"

$validationRecipeSet = New-Set (@($game.validationRecipes) | ForEach-Object { [string]$_.name })
$budgetValidationRecipeSet = New-Set @($budget.validationRecipeIds)
$budgetRowSet = New-Set (@($budget.budgetRows) | ForEach-Object { [string]$_.id })
$evidenceRowSet = New-Set (@($budget.evidenceRows) | ForEach-Object { [string]$_.id })
$workloadRows = @($budget.workloadRows)
if ($workloadRows.Count -ne $requiredWorkloads.Count) {
    Write-Error "$gameManifestPath performanceBudgets workloadRows expected $($requiredWorkloads.Count) rows but found $($workloadRows.Count)"
}

Assert-ContainsAll $budgetValidationRecipeSet @(
    "installed-2d-sprite-throughput-smoke",
    "installed-2d-sandbox-package-budget-smoke",
    "installed-2d-tilemap-runtime-ux-smoke",
    "installed-2d-runtime-ui-renderer-atlas-handoff-smoke",
    "installed-2d-input-device-production-ux-smoke",
    "installed-2d-long-run-readiness-smoke",
    "host-2d-long-run-readiness-soak"
) "$gameManifestPath performanceBudgets validationRecipeIds"
Assert-ContainsAll (New-Set @($budget.unsupportedClaims)) $requiredUnsupportedClaims "$gameManifestPath performanceBudgets unsupportedClaims"

foreach ($row in @($budget.evidenceRows)) {
    $rowId = [string]$row.id
    Assert-ObjectProperties $row @("id", "budgetRowIds", "evidenceKind", "validationRecipeId", "status") "$gameManifestPath performanceBudgets evidenceRows $rowId"
    if (-not $validationRecipeSet.Contains([string]$row.validationRecipeId)) {
        Write-Error "$gameManifestPath performanceBudgets evidenceRows '$rowId' references missing validation recipe: $($row.validationRecipeId)"
    }
    if ([string]$row.status -eq "ready" -and
        (@("trace-artifact", "profiler-artifact") -contains [string]$row.evidenceKind)) {
        if ($null -eq $row.PSObject.Properties["artifactPath"]) {
            Write-Error "$gameManifestPath performanceBudgets ready artifact evidence '$rowId' must carry artifactPath"
        }
        Assert-SafeArtifactPath ([string]$row.artifactPath) "$gameManifestPath performanceBudgets evidenceRows '$rowId'"
        if ($RequireReady.IsPresent -and -not (Test-Path -LiteralPath (Join-Path $root ([string]$row.artifactPath)) -PathType Leaf)) {
            Write-Error "$gameManifestPath performanceBudgets ready artifact evidence '$rowId' file is missing: $($row.artifactPath)"
        }
    }
}

$workloadRowById = @{}
foreach ($row in $workloadRows) {
    Assert-ObjectProperties $row @(
        "id",
        "status",
        "validationRecipeIds",
        "budgetRowIds",
        "evidenceRowIds",
        "artifactPolicy",
        "unsupportedClaims"
    ) "$gameManifestPath performanceBudgets workloadRows"
    $rowId = [string]$row.id
    if ($rowId -notmatch "^[a-z0-9][a-z0-9-]*$") {
        Write-Error "$gameManifestPath performanceBudgets workloadRows id must be kebab-case or selected 2d-prefixed kebab-case: $rowId"
    }
    if ($workloadRowById.ContainsKey($rowId)) {
        Write-Error "$gameManifestPath performanceBudgets workloadRows id is duplicated: $rowId"
    }
    $workloadRowById[$rowId] = $row
    if (@("ready", "host-gated") -notcontains [string]$row.status) {
        Write-Error "$gameManifestPath performanceBudgets workloadRows '$rowId' has unsupported status: $($row.status)"
    }
    foreach ($recipeId in @($row.validationRecipeIds)) {
        if (-not $validationRecipeSet.Contains([string]$recipeId)) {
            Write-Error "$gameManifestPath performanceBudgets workloadRows '$rowId' references missing validation recipe: $recipeId"
        }
    }
    foreach ($budgetRowId in @($row.budgetRowIds)) {
        if (-not $budgetRowSet.Contains([string]$budgetRowId)) {
            Write-Error "$gameManifestPath performanceBudgets workloadRows '$rowId' references missing budget row: $budgetRowId"
        }
    }
    foreach ($evidenceRowId in @($row.evidenceRowIds)) {
        if (-not $evidenceRowSet.Contains([string]$evidenceRowId)) {
            Write-Error "$gameManifestPath performanceBudgets workloadRows '$rowId' references missing evidence row: $evidenceRowId"
        }
    }
    Assert-ContainsAll (New-Set @($row.unsupportedClaims)) $requiredUnsupportedClaims "$gameManifestPath performanceBudgets workloadRows '$rowId' unsupportedClaims"
}

foreach ($required in $requiredWorkloads) {
    $rowId = [string]$required.Id
    if (-not $workloadRowById.ContainsKey($rowId)) {
        Write-Error "$gameManifestPath performanceBudgets workloadRows missing required workload: $rowId"
    }
    $row = $workloadRowById[$rowId]
    if ([string]$row.status -ne [string]$required.Status) {
        Write-Error "$gameManifestPath performanceBudgets workloadRows '$rowId' expected status $($required.Status) but was $($row.status)"
    }
    Assert-ContainsAll (New-Set @($row.validationRecipeIds)) $required.Recipes "$gameManifestPath performanceBudgets workloadRows '$rowId' validationRecipeIds"
    Assert-ContainsAll (New-Set @($row.budgetRowIds)) $required.Budgets "$gameManifestPath performanceBudgets workloadRows '$rowId' budgetRowIds"
    Assert-ContainsAll (New-Set @($row.evidenceRowIds)) $required.Evidence "$gameManifestPath performanceBudgets workloadRows '$rowId' evidenceRowIds"
}

if ($RequireReady.IsPresent) {
    $smokeArgs = @(
        "--smoke",
        "--max-frames",
        "3",
        "--require-config",
        "runtime/sample_2d_desktop_runtime_package.config",
        "--require-scene-package",
        "runtime/sample_2d_desktop_runtime_package.geindex",
        "--require-win32-runtime-host",
        "--require-d3d12-shaders",
        "--require-d3d12-renderer",
        "--require-2d-sprite-throughput",
        "--require-gameplay-systems",
        "--require-procedural-generation",
        "--require-tilemap-runtime-ux",
        "--require-production-tile-renderer",
        "--require-runtime-ui-renderer-atlas-handoff",
        "--require-sandbox-package-budgets",
        "--require-performance-baseline",
        "--require-long-run-performance-readiness",
        "--require-2d-input-device-production-ux"
    )

    $packageOutput = & (Join-Path $PSScriptRoot "package-desktop-runtime.ps1") `
        -GameTarget "sample_2d_desktop_runtime_package" `
        -RequireD3d12Shaders `
        -SmokeArgs $smokeArgs 2>&1
    $packageExitCode = $LASTEXITCODE
    if ($packageExitCode -ne 0) {
        Write-Error "2d-production-workloads: package smoke failed with exit code $packageExitCode`n$(@($packageOutput) -join "`n")"
    }

    $installedExe = Join-Path $root "out/install/desktop-runtime-release/bin/sample_2d_desktop_runtime_package.exe"
    if (-not (Test-Path -LiteralPath $installedExe -PathType Leaf)) {
        Write-Error "2d-production-workloads: installed sample executable missing after package smoke: $installedExe"
    }
    $smokeOutput = & $installedExe @smokeArgs 2>&1
    $smokeExitCode = $LASTEXITCODE
    if ($smokeExitCode -ne 0) {
        Write-Error "2d-production-workloads: installed package smoke failed with exit code $smokeExitCode`n$(@($smokeOutput) -join "`n")"
    }

    $fields = ConvertFrom-StatusLine -Output @($smokeOutput)
    foreach ($entry in $readySmokeExpectations.GetEnumerator()) {
        Assert-SmokeField $fields $entry.Key $entry.Value
    }
    foreach ($positiveCounter in @(
            "2d_sprite_throughput_retained_timing_artifact_hash",
            "sandbox_package_budget_replay_hash",
            "runtime_ui_renderer_atlas_handoff_replay_hash",
            "long_run_readiness_memory_high_water_bytes"
        )) {
        if (-not $fields.ContainsKey($positiveCounter) -or [decimal]$fields[$positiveCounter] -le 0) {
            Write-Error "2d-production-workloads: package smoke counter $positiveCounter must be positive"
        }
    }
}

Write-Information "2d-production-workloads: ok" -InformationAction Continue
