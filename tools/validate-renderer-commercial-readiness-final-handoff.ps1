#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$RepoFullName = "",

    [string]$RunnersJsonPath = "",

    [string]$IntakeManifestRelative = "",

    [string]$SourceRunId = "",

    [string]$MetalMemoryProfilingRunId = ""
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

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

function Read-JsonFile {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $fullPath = Resolve-RepoRelativePath -RelativePath $RelativePath -Label $Label
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        Write-Error "$Label does not exist: $RelativePath"
    }
    return Get-Content -LiteralPath $fullPath -Raw | ConvertFrom-Json
}

function ConvertTo-KeyValueMap {
    param([Parameter(Mandatory = $true)][string[]]$Lines)

    $map = @{}
    foreach ($line in @($Lines)) {
        $text = [string]$line
        $separatorIndex = $text.IndexOf("=")
        if ($separatorIndex -le 0) {
            continue
        }
        $key = $text.Substring(0, $separatorIndex)
        $value = $text.Substring($separatorIndex + 1)
        $map[$key] = $value
    }
    return $map
}

function Invoke-RunnerPreflight {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryFullName,
        [Parameter(Mandatory = $true)][string]$RunnerJsonPath
    )

    $preflightScript = Join-Path $PSScriptRoot "validate-renderer-metal-memory-profiling-capable-host-runner.ps1"
    if (-not (Test-Path -LiteralPath $preflightScript -PathType Leaf)) {
        Write-Error "required_tool_missing: tools/validate-renderer-metal-memory-profiling-capable-host-runner.ps1"
    }

    $lines = if (-not [string]::IsNullOrWhiteSpace($RepositoryFullName) -and
        -not [string]::IsNullOrWhiteSpace($RunnerJsonPath)) {
        @(& $preflightScript -RepoFullName $RepositoryFullName -RunnersJsonPath $RunnerJsonPath)
    } elseif (-not [string]::IsNullOrWhiteSpace($RunnerJsonPath)) {
        @(& $preflightScript -RunnersJsonPath $RunnerJsonPath)
    } else {
        @(& $preflightScript -RepoFullName $RepositoryFullName)
    }
    return ConvertTo-KeyValueMap -Lines ([string[]]$lines)
}

function Select-StringArray {
    param([AllowNull()]$Value)

    if ($null -eq $Value) {
        return @()
    }

    $items = [System.Collections.Generic.List[string]]::new()
    foreach ($item in @($Value)) {
        if ($null -eq $item) {
            continue
        }
        $text = [string]$item
        if (-not [string]::IsNullOrWhiteSpace($text)) {
            $items.Add($text) | Out-Null
        }
    }
    return @($items)
}

function Test-JsonReady {
    param([AllowNull()]$Value)

    if ($null -eq $Value) {
        return $false
    }
    return [bool]$Value
}

function Get-HandoffReadyProperty {
    param(
        [AllowNull()]$Manifest,
        [Parameter(Mandatory = $true)][string]$PropertyName
    )

    $handoff = Get-JsonPropertyValue -JsonObject $Manifest -Name $PropertyName
    return Test-JsonReady -Value (Get-JsonPropertyValue -JsonObject $handoff -Name "ready")
}

function Get-QualityVfxDependencyBlockers {
    param([AllowNull()]$Manifest)

    $blockers = Get-JsonPropertyValue -JsonObject $Manifest -Name "assembler_input_blockers"
    $qualityVfxBlocker = Get-JsonPropertyValue -JsonObject $blockers -Name "quality_vfx_host_evidence"
    return Select-StringArray -Value (Get-JsonPropertyValue -JsonObject $qualityVfxBlocker -Name "dependent_missing_inputs")
}

function Get-FinalPreflightArtifactRoot {
    param([AllowNull()]$Manifest)

    $handoff = Get-JsonPropertyValue -JsonObject $Manifest -Name "final_preflight_handoff"
    return [string](Get-JsonPropertyValue -JsonObject $handoff -Name "artifact_root")
}

function Get-AssemblerArguments {
    param(
        [AllowNull()]$Manifest,
        [string]$ArgumentProperty = "command_arguments"
    )

    $handoff = Get-JsonPropertyValue -JsonObject $Manifest -Name "assembler_handoff"
    return Select-StringArray -Value (Get-JsonPropertyValue -JsonObject $handoff -Name $ArgumentProperty)
}

function Join-CommandArguments {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)

    return [string]::Join(" ", @($Arguments))
}

if (-not [string]::IsNullOrWhiteSpace($RepoFullName) -and
    $RepoFullName -notmatch '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$') {
    Write-Error "RepoFullName must be in owner/repo form."
}
foreach ($runIdSpec in @(
        @{ Name = "SourceRunId"; Value = $SourceRunId },
        @{ Name = "MetalMemoryProfilingRunId"; Value = $MetalMemoryProfilingRunId }
    )) {
    if (-not [string]::IsNullOrWhiteSpace([string]$runIdSpec.Value) -and
        [string]$runIdSpec.Value -notmatch '^\d+$') {
        Write-Error "$($runIdSpec.Name) must contain only digits when provided."
    }
}

$manifest = $null
$manifestPresent = $false
if (-not [string]::IsNullOrWhiteSpace($IntakeManifestRelative)) {
    $manifest = Read-JsonFile -RelativePath $IntakeManifestRelative -Label "IntakeManifestRelative"
    $manifestPresent = $true
}

$runnerPreflightKnown = -not [string]::IsNullOrWhiteSpace($RepoFullName) -or
    -not [string]::IsNullOrWhiteSpace($RunnersJsonPath)
$runnerAvailable = $false
$runnerStatus = "unknown"
if ($runnerPreflightKnown) {
    $runnerMap = Invoke-RunnerPreflight -RepositoryFullName $RepoFullName -RunnerJsonPath $RunnersJsonPath
    $runnerStatus = [string]$runnerMap["renderer_metal_memory_profiling_capable_host_runner_status"]
    $runnerAvailable = [string]$runnerMap["renderer_metal_memory_profiling_capable_host_runner_available"] -ceq "1"
}

$manifestRunId = if ($manifestPresent) {
    [string](Get-JsonPropertyValue -JsonObject $manifest -Name "run_id")
} else {
    ""
}
$resolvedSourceRunId = if (-not [string]::IsNullOrWhiteSpace($SourceRunId)) {
    $SourceRunId
} else {
    $manifestRunId
}
$sourceRunReady = -not [string]::IsNullOrWhiteSpace($resolvedSourceRunId)
$metalRunReady = -not [string]::IsNullOrWhiteSpace($MetalMemoryProfilingRunId)

$missingAssemblerInputNames = if ($manifestPresent) {
    Select-StringArray -Value (Get-JsonPropertyValue -JsonObject $manifest -Name "missing_assembler_inputs")
} else {
    @()
}
$missingAssemblerInputSet = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
foreach ($missingAssemblerInputName in @($missingAssemblerInputNames)) {
    $null = $missingAssemblerInputSet.Add($missingAssemblerInputName)
}
$qualityVfxDependencyBlockers = @(Get-QualityVfxDependencyBlockers -Manifest $manifest)
$finalPreflightReady = Get-HandoffReadyProperty -Manifest $manifest -PropertyName "final_preflight_handoff"
$assemblerHandoffReady = Get-HandoffReadyProperty -Manifest $manifest -PropertyName "assembler_handoff"
$metalMemoryInputMissing = $missingAssemblerInputSet.Contains("metal_memory_profiling_host_evidence") -or
    $qualityVfxDependencyBlockers.Contains("metal_memory_profiling_host_evidence")

$status = "artifact_intake_required"
$nextAction = "inspect_current_run_artifact_intake"
if ($finalPreflightReady) {
    $status = "ready_for_final_preflight"
    $nextAction = "run_final_preflight"
} elseif ($assemblerHandoffReady) {
    $status = "ready_for_final_assembler"
    $nextAction = "run_final_assembler"
} elseif (-not $manifestPresent) {
    $status = "artifact_intake_required"
    $nextAction = "inspect_current_run_artifact_intake"
} elseif (-not $sourceRunReady) {
    $status = "source_run_required"
    $nextAction = "identify_source_artifact_run"
} elseif ($metalMemoryInputMissing -and -not $metalRunReady) {
    if (-not $runnerPreflightKnown) {
        $status = "capable_host_runner_preflight_required"
        $nextAction = "run_capable_host_runner_preflight"
    } elseif ($runnerAvailable) {
        $status = "metal_memory_profiling_run_required"
        $nextAction = "run_metal_memory_profiling_capable_host_workflow"
    } else {
        $status = "capable_host_runner_required"
        $nextAction = "provision_capable_host_runner"
    }
} elseif ($metalMemoryInputMissing -and $metalRunReady) {
    $status = "ready_for_final_from_runs_workflow"
    $nextAction = "run_final_from_runs_workflow"
} elseif (@($missingAssemblerInputNames).Count -gt 0) {
    $status = "assembler_inputs_required"
    $nextAction = "collect_missing_assembler_inputs"
} else {
    $status = "artifact_intake_ready"
    $nextAction = "inspect_final_handoff"
}

$capableHostWorkflowCommand = "gh workflow run renderer-metal-memory-profiling-capable-host.yml -f confirm_capable_apple_host=MTLGPUFamilyApple6"
$finalFromRunsWorkflowCommand = ""
if ($sourceRunReady -and $metalRunReady) {
    $finalFromRunsWorkflowCommand = "gh workflow run renderer-commercial-readiness-final-from-runs.yml -f source_artifact_run_id=$resolvedSourceRunId -f metal_memory_profiling_run_id=$MetalMemoryProfilingRunId -f confirm_final_retained_root_handoff=renderer-commercial-final-retained-root"
}
$finalPreflightCommand = ""
if ($finalPreflightReady) {
    $artifactRoot = Get-FinalPreflightArtifactRoot -Manifest $manifest
    if (-not [string]::IsNullOrWhiteSpace($artifactRoot)) {
        $finalPreflightCommand = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1 -ArtifactRootRelative $artifactRoot"
    }
}
$assemblerCommand = ""
if ($assemblerHandoffReady) {
    $assemblerArguments = @(Get-AssemblerArguments -Manifest $manifest)
    if (@($assemblerArguments).Count -gt 0) {
        $assemblerCommand = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/assemble-renderer-commercial-readiness-final-retained-root.ps1 $(Join-CommandArguments -Arguments ([string[]]$assemblerArguments))"
    }
}

Write-Output "validation_recipe=renderer-commercial-readiness-final-handoff"
if (-not [string]::IsNullOrWhiteSpace($RepoFullName)) {
    Write-Output "renderer_commercial_readiness_final_handoff_repo=$RepoFullName"
}
Write-Output "renderer_commercial_readiness_final_handoff_status=$status"
Write-Output "renderer_commercial_readiness_final_handoff_next_action=$nextAction"
Write-Output "renderer_commercial_readiness_final_handoff_intake_manifest_present=$(ConvertTo-CounterBit $manifestPresent)"
if ($manifestPresent) {
    Write-Output "renderer_commercial_readiness_final_handoff_intake_manifest=$IntakeManifestRelative"
}
Write-Output "renderer_commercial_readiness_final_handoff_source_run_ready=$(ConvertTo-CounterBit $sourceRunReady)"
if ($sourceRunReady) {
    Write-Output "renderer_commercial_readiness_final_handoff_source_run_id=$resolvedSourceRunId"
}
Write-Output "renderer_commercial_readiness_final_handoff_metal_memory_profiling_run_ready=$(ConvertTo-CounterBit $metalRunReady)"
if ($metalRunReady) {
    Write-Output "renderer_commercial_readiness_final_handoff_metal_memory_profiling_run_id=$MetalMemoryProfilingRunId"
}
Write-Output "renderer_commercial_readiness_final_handoff_runner_preflight_known=$(ConvertTo-CounterBit $runnerPreflightKnown)"
Write-Output "renderer_commercial_readiness_final_handoff_runner_status=$(ConvertTo-CounterValue -Value $runnerStatus)"
Write-Output "renderer_commercial_readiness_final_handoff_runner_available=$(ConvertTo-CounterBit $runnerAvailable)"
Write-Output "renderer_commercial_readiness_final_handoff_missing_assembler_inputs=$(@($missingAssemblerInputNames).Count)"
if (@($missingAssemblerInputNames).Count -gt 0) {
    Write-Output "renderer_commercial_readiness_final_handoff_missing_assembler_input_names=$((@($missingAssemblerInputNames) | ForEach-Object { ConvertTo-CounterValue -Value ([string]$_) }) -join ',')"
}
Write-Output "renderer_commercial_readiness_final_handoff_quality_vfx_dependency_blocker_count=$(@($qualityVfxDependencyBlockers).Count)"
if (@($qualityVfxDependencyBlockers).Count -gt 0) {
    Write-Output "renderer_commercial_readiness_final_handoff_quality_vfx_dependency_blockers=$((@($qualityVfxDependencyBlockers) | ForEach-Object { ConvertTo-CounterValue -Value ([string]$_) }) -join ',')"
}
Write-Output "renderer_commercial_readiness_final_handoff_assembler_handoff_ready=$(ConvertTo-CounterBit $assemblerHandoffReady)"
Write-Output "renderer_commercial_readiness_final_handoff_final_preflight_ready=$(ConvertTo-CounterBit $finalPreflightReady)"
if ($nextAction -ceq "run_metal_memory_profiling_capable_host_workflow" -or $runnerAvailable) {
    Write-Output "renderer_commercial_readiness_final_handoff_capable_host_workflow_command=$capableHostWorkflowCommand"
}
if (-not [string]::IsNullOrWhiteSpace($finalFromRunsWorkflowCommand)) {
    Write-Output "renderer_commercial_readiness_final_handoff_final_from_runs_workflow_command=$finalFromRunsWorkflowCommand"
}
if (-not [string]::IsNullOrWhiteSpace($finalPreflightCommand)) {
    Write-Output "renderer_commercial_readiness_final_handoff_final_preflight_command=$finalPreflightCommand"
}
if (-not [string]::IsNullOrWhiteSpace($assemblerCommand)) {
    Write-Output "renderer_commercial_readiness_final_handoff_final_assembler_command=$assemblerCommand"
}
Write-Output "renderer_backend_parity_ready=0"
Write-Output "renderer_metal_broad_readiness=0"
Write-Output "renderer_broad_quality_ready=0"
Write-Output "renderer_commercial_readiness=0"
Write-Output "renderer_environment_ready=0"

Write-Information "renderer-commercial-readiness-final-handoff: ok" -InformationAction Continue
