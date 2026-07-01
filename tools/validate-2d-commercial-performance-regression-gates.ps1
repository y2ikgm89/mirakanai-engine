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
$recipeId = "installed-2d-commercial-performance-regression-smoke"
$qualityGateId = "performance-regression-gate-2d"
$readySmokeExpectations = @{
    "2d_commercial_performance_regression_status" = "ready"
    "2d_commercial_performance_regression_ready" = "1"
    "2d_commercial_performance_workload_gate_ready" = "1"
    "2d_commercial_performance_metric_gate_ready" = "1"
    "2d_commercial_performance_official_source_ready" = "1"
    "2d_commercial_performance_host_threshold_gate_ready" = "1"
    "2d_commercial_performance_workload_rows" = "8"
    "2d_commercial_performance_metric_rows" = "11"
    "2d_commercial_performance_host_class_threshold_rows" = "11"
    "2d_commercial_performance_package_visible_metric_rows" = "6"
    "2d_commercial_performance_host_gated_profiler_artifact_rows" = "5"
    "2d_commercial_performance_official_source_rows" = "7"
    "2d_commercial_performance_cpu_frame_p50_us" = "15800"
    "2d_commercial_performance_cpu_frame_p95_us" = "16000"
    "2d_commercial_performance_cpu_frame_p99_us" = "16000"
    "2d_commercial_performance_over_budget_rows" = "0"
    "2d_commercial_performance_broad_optimization_claim_rows" = "0"
    "2d_commercial_performance_cross_vendor_parity_claim_rows" = "0"
    "2d_commercial_performance_cross_backend_parity_claim_rows" = "0"
    "2d_commercial_performance_native_handle_access_rows" = "0"
    "2d_commercial_performance_renderer_rhi_residency_claim_rows" = "0"
    "2d_commercial_performance_allocator_gpu_budget_enforcement_claim_rows" = "0"
    "2d_commercial_performance_pgo_lto_default_lane_claim_rows" = "0"
    "2d_commercial_performance_profiler_artifact_ready_without_artifact_rows" = "0"
    "2d_commercial_performance_external_engine_claim_rows" = "0"
    "2d_commercial_performance_legal_approval_claim_rows" = "0"
    "2d_commercial_performance_diagnostics" = "0"
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

function New-Set {
    param([Parameter(Mandatory = $true)][object[]]$Values)

    $set = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($value in @($Values)) {
        $null = $set.Add([string]$value)
    }
    return $set
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

function ConvertFrom-StatusLine {
    param([Parameter(Mandatory = $true)][string[]]$Output)

    $statusLine = @($Output | Where-Object { $_ -match "^sample_2d_desktop_runtime_package status=" } | Select-Object -Last 1)
    if ($statusLine.Count -eq 0) {
        Write-Error "2d-commercial-performance-regression: package smoke did not emit sample_2d_desktop_runtime_package status line"
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
        Write-Error "2d-commercial-performance-regression: package smoke missing counter $Name"
    }
    if ([string]$Fields[$Name] -ne $Expected) {
        Write-Error "2d-commercial-performance-regression: package smoke counter $Name expected $Expected but was $($Fields[$Name])"
    }
}

$game = Read-Json -RelativePath $gameManifestPath -Root $root
$validationRecipeSet = New-Set (@($game.validationRecipes) | ForEach-Object { [string]$_.name })
if (-not $validationRecipeSet.Contains($recipeId)) {
    Write-Error "$gameManifestPath validationRecipes missing $recipeId"
}

$aiWorkflow = Get-ObjectPropertyValue -Object $game -Name "aiWorkflow"
if ($null -eq $aiWorkflow) {
    Write-Error "$gameManifestPath must declare aiWorkflow"
}
$gameDesignSpec = Get-ObjectPropertyValue -Object $aiWorkflow -Name "gameDesignSpec"
if ($null -eq $gameDesignSpec) {
    Write-Error "$gameManifestPath aiWorkflow must declare gameDesignSpec"
}
Assert-ObjectProperties $gameDesignSpec @("validationRecipeIds", "qualityGates") "$gameManifestPath gameDesignSpec"
Assert-ContainsAll (New-Set @($gameDesignSpec.validationRecipeIds)) @($recipeId) "$gameManifestPath gameDesignSpec validationRecipeIds"

$qualityGate = @($gameDesignSpec.qualityGates | Where-Object { [string]$_.id -eq $qualityGateId })
if ($qualityGate.Count -ne 1) {
    Write-Error "$gameManifestPath gameDesignSpec qualityGates must declare exactly one $qualityGateId row"
}
Assert-ObjectProperties $qualityGate[0] @("id", "evidence", "recipeIds") "$gameManifestPath $qualityGateId quality gate"
Assert-ContainsAll (New-Set @($qualityGate[0].recipeIds)) @($recipeId) "$gameManifestPath $qualityGateId recipeIds"
foreach ($needle in @(
        "zero broad optimization",
        "zero cross-vendor",
        "zero cross-backend",
        "zero native-handle",
        "zero legal-approval"
    )) {
    if (-not ([string]$qualityGate[0].evidence).Contains($needle)) {
        Write-Error "$gameManifestPath $qualityGateId evidence must mention $needle"
    }
}

$gameplayContract = Get-ObjectPropertyValue -Object $game -Name "gameplayContract"
if ($null -eq $gameplayContract) {
    Write-Error "$gameManifestPath must declare gameplayContract"
}
Assert-ObjectProperties $gameplayContract @("commercialPerformanceRegression2D") "$gameManifestPath gameplayContract"
$commercialContract = [string](Get-ObjectPropertyValue -Object $gameplayContract -Name "commercialPerformanceRegression2D")
foreach ($needle in @(
        "evaluate_runtime_2d_commercial_performance_regression_gate",
        "--require-2d-commercial-performance-regression-gate",
        "2d_commercial_performance_regression_ready=1",
        "2d_commercial_performance_workload_rows=8",
        "2d_commercial_performance_metric_rows=11",
        "2d_commercial_performance_official_source_rows=7",
        "2d_commercial_performance_cpu_frame_p50_us=15800",
        "2d_commercial_performance_cpu_frame_p95_us=16000",
        "2d_commercial_performance_cpu_frame_p99_us=16000",
        "2d_commercial_performance_over_budget_rows=0",
        "external commercial engine compatibility",
        "legal approval"
    )) {
    if (-not $commercialContract.Contains($needle)) {
        Write-Error "$gameManifestPath gameplayContract.commercialPerformanceRegression2D missing expected text: $needle"
    }
}

$budget = Get-ObjectPropertyValue -Object $game -Name "performanceBudgets"
if ($null -ne $budget) {
    Assert-ContainsAll (New-Set @($budget.validationRecipeIds)) @($recipeId) "$gameManifestPath performanceBudgets validationRecipeIds"
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
        "--require-2d-commercial-performance-regression-gate"
    )

    $packageOutput = & (Join-Path $PSScriptRoot "package-desktop-runtime.ps1") `
        -GameTarget "sample_2d_desktop_runtime_package" `
        -SmokeArgs $smokeArgs 2>&1
    $packageExitCode = $LASTEXITCODE
    if ($packageExitCode -ne 0) {
        Write-Error "2d-commercial-performance-regression: package smoke failed with exit code $packageExitCode`n$(@($packageOutput) -join "`n")"
    }

    $installedExe = Join-Path $root "out/install/desktop-runtime-release/bin/sample_2d_desktop_runtime_package.exe"
    if (-not (Test-Path -LiteralPath $installedExe -PathType Leaf)) {
        Write-Error "2d-commercial-performance-regression: installed sample executable missing after package smoke: $installedExe"
    }
    $smokeOutput = & $installedExe @smokeArgs 2>&1
    $smokeExitCode = $LASTEXITCODE
    if ($smokeExitCode -ne 0) {
        Write-Error "2d-commercial-performance-regression: installed package smoke failed with exit code $smokeExitCode`n$(@($smokeOutput) -join "`n")"
    }

    $fields = ConvertFrom-StatusLine -Output @($smokeOutput)
    foreach ($entry in $readySmokeExpectations.GetEnumerator()) {
        Assert-SmokeField $fields $entry.Key $entry.Value
    }
}

Write-Information "2d-commercial-performance-regression: ok" -InformationAction Continue
