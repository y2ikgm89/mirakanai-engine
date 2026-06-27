#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$RepoFullName = "",

    [string]$RunnersJsonPath = "",

    [string[]]$RequiredLabels = @("self-hosted", "macOS", "ARM64", "metal-residency-set"),

    [switch]$RequireAvailable
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root
$apiEndpointTemplate = "/repos/{owner}/{repo}/actions/runners"

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function ConvertTo-CounterValue {
    param([Parameter(Mandatory = $true)][string]$Value)

    return ($Value -replace '[^A-Za-z0-9_.-]', '_')
}

function Get-JsonPropertyValue {
    param(
        [AllowNull()]$JsonObject,
        [Parameter(Mandatory = $true)][string]$Name
    )

    if ($null -eq $JsonObject) {
        return $null
    }
    $property = $JsonObject.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }
    return $property.Value
}

function Assert-PathUnderDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $pathFull = [System.IO.Path]::GetFullPath($Path)
    $directoryFull = [System.IO.Path]::GetFullPath($Directory).TrimEnd(
        [char[]]@([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar))
    $directoryPrefix = $directoryFull + [System.IO.Path]::DirectorySeparatorChar
    if ($pathFull -ne $directoryFull -and
        -not $pathFull.StartsWith($directoryPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "$Description escaped repository root '$directoryFull': $pathFull"
    }
}

function Resolve-RepoPath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $resolvedPath = if ([System.IO.Path]::IsPathRooted($Path)) {
        [System.IO.Path]::GetFullPath($Path)
    } else {
        [System.IO.Path]::GetFullPath((Join-Path $root $Path))
    }
    Assert-PathUnderDirectory -Path $resolvedPath -Directory $root -Description $Description
    return $resolvedPath
}

function Get-RunnerLabelNames {
    param([AllowNull()]$Runner)

    $labels = Get-JsonPropertyValue -JsonObject $Runner -Name "labels"
    if ($null -eq $labels) {
        return @()
    }

    $names = [System.Collections.Generic.List[string]]::new()
    foreach ($label in @($labels)) {
        if ($null -eq $label) {
            continue
        }
        if ($label -is [string]) {
            if (-not [string]::IsNullOrWhiteSpace($label)) {
                $names.Add([string]$label)
            }
            continue
        }

        $name = Get-JsonPropertyValue -JsonObject $label -Name "name"
        if ($null -ne $name -and -not [string]::IsNullOrWhiteSpace([string]$name)) {
            $names.Add([string]$name)
        }
    }
    return @($names)
}

function Test-HasRequiredLabels {
    param(
        [Parameter(Mandatory = $true)][string[]]$LabelNames,
        [Parameter(Mandatory = $true)][string[]]$RequiredLabelNames
    )

    foreach ($requiredLabel in $RequiredLabelNames) {
        if (-not $LabelNames.Contains($requiredLabel)) {
            return $false
        }
    }
    return $true
}

function Read-RunnerJson {
    if (-not [string]::IsNullOrWhiteSpace($RunnersJsonPath)) {
        $jsonPath = Resolve-RepoPath -Path $RunnersJsonPath -Description "RunnersJsonPath"
        if (-not (Test-Path -LiteralPath $jsonPath -PathType Leaf)) {
            Write-Error "RunnersJsonPath does not exist: $jsonPath"
        }
        return Get-Content -LiteralPath $jsonPath -Raw | ConvertFrom-Json
    }

    if ([string]::IsNullOrWhiteSpace($RepoFullName)) {
        Write-Error "RepoFullName is required when RunnersJsonPath is not provided."
    }
    if ($RepoFullName -notmatch '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$') {
        Write-Error "RepoFullName must be in owner/repo form."
    }
    $gh = Get-Command gh -ErrorAction SilentlyContinue
    if ($null -eq $gh) {
        Write-Error "gh CLI is required when RunnersJsonPath is not provided."
    }

    $apiPath = "/repos/$RepoFullName/actions/runners?per_page=100"
    $apiOutput = @(& $gh.Source "api" "-H" "Accept: application/vnd.github+json" $apiPath 2>&1)
    if ($LASTEXITCODE -ne 0) {
        Write-Error "failed to list GitHub Actions self-hosted runners: $([string]::Join("`n", $apiOutput))"
    }
    return ([string]::Join("`n", $apiOutput) | ConvertFrom-Json)
}

if ($RequiredLabels.Count -eq 0) {
    Write-Error "RequiredLabels must contain at least one label."
}
foreach ($requiredLabel in @($RequiredLabels)) {
    if ([string]::IsNullOrWhiteSpace($requiredLabel)) {
        Write-Error "RequiredLabels must not contain blank labels."
    }
}

$runnerJson = Read-RunnerJson
$runners = @((Get-JsonPropertyValue -JsonObject $runnerJson -Name "runners"))
$totalCountValue = Get-JsonPropertyValue -JsonObject $runnerJson -Name "total_count"
$totalCount = if ($null -eq $totalCountValue) { $runners.Count } else { [int]$totalCountValue }

$matchingLabelRunners = 0
$onlineMatchingRunners = 0
$idleMatchingRunners = 0
$availableRunnerNames = [System.Collections.Generic.List[string]]::new()

foreach ($runner in @($runners)) {
    $labelNames = @(Get-RunnerLabelNames -Runner $runner)
    if (-not (Test-HasRequiredLabels -LabelNames $labelNames -RequiredLabelNames $RequiredLabels)) {
        continue
    }

    $matchingLabelRunners += 1
    $status = [string](Get-JsonPropertyValue -JsonObject $runner -Name "status")
    $busy = [bool](Get-JsonPropertyValue -JsonObject $runner -Name "busy")
    if ($status -ceq "online") {
        $onlineMatchingRunners += 1
        if (-not $busy) {
            $idleMatchingRunners += 1
            $name = Get-JsonPropertyValue -JsonObject $runner -Name "name"
            if ($null -ne $name -and -not [string]::IsNullOrWhiteSpace([string]$name)) {
                $availableRunnerNames.Add((ConvertTo-CounterValue -Value ([string]$name)))
            }
        }
    }
}

$available = $idleMatchingRunners -gt 0
$statusValue = if ($available) { "ready" } else { "host_evidence_required" }
$requiredLabelText = [string]::Join(",", @($RequiredLabels))

Write-Output "validation_recipe=renderer-metal-memory-profiling-capable-host-runner-preflight"
Write-Output "renderer_metal_memory_profiling_capable_host_runner_api_endpoint=$apiEndpointTemplate"
Write-Output "renderer_metal_memory_profiling_capable_host_runner_status=$statusValue"
Write-Output "renderer_metal_memory_profiling_capable_host_runner_available=$(ConvertTo-CounterBit $available)"
Write-Output "renderer_metal_memory_profiling_capable_host_runner_total=$totalCount"
Write-Output "renderer_metal_memory_profiling_capable_host_runner_matching_label_runners=$matchingLabelRunners"
Write-Output "renderer_metal_memory_profiling_capable_host_runner_online_matching_runners=$onlineMatchingRunners"
Write-Output "renderer_metal_memory_profiling_capable_host_runner_idle_matching_runners=$idleMatchingRunners"
Write-Output "renderer_metal_memory_profiling_capable_host_runner_required_labels=$requiredLabelText"
if ($availableRunnerNames.Count -gt 0) {
    Write-Output "renderer_metal_memory_profiling_capable_host_runner_available_names=$([string]::Join(',', @($availableRunnerNames)))"
}
Write-Output "renderer_backend_parity_ready=0"
Write-Output "renderer_metal_broad_readiness=0"
Write-Output "renderer_broad_quality_ready=0"
Write-Output "renderer_commercial_readiness=0"
Write-Output "renderer_environment_ready=0"

if ($RequireAvailable.IsPresent -and -not $available) {
    Write-Error "Renderer Metal memory profiling capable host runner is not available for labels: $requiredLabelText"
}

Write-Information "renderer-metal-memory-profiling-capable-host-runner-preflight: ok" -InformationAction Continue
