#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10AM

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan")]
    [string]$Mode = "Plan",

    [string]$RepoFullName = "",

    [string]$Branch = "main",

    [string]$SourceWorkflowName = "Validate",

    [string]$MetalWorkflowName = "Renderer Metal Memory Profiling Capable Host",

    [string]$SourceArtifactName = "renderer-commercial-readiness-current-run-artifact-intake",

    [string]$MetalMemoryProfilingArtifactName = "renderer-metal-memory-profiling-host-artifacts",

    [string]$SourceRunsJsonPath = "",

    [string]$SourceArtifactsJsonPath = "",

    [string]$MetalRunsJsonPath = "",

    [string]$MetalArtifactsJsonPath = "",

    [ValidateRange(1, 100)]
    [int]$PerPage = 100
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$runsEndpointTemplate = "/repos/{owner}/{repo}/actions/runs"
$artifactsEndpointTemplate = "/repos/{owner}/{repo}/actions/runs/{run_id}/artifacts"
$finalFromRunsWorkflowName = "renderer-commercial-readiness-final-from-runs.yml"

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

function Read-JsonFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $jsonPath = Resolve-RepoPath -Path $Path -Description $Description
    if (-not (Test-Path -LiteralPath $jsonPath -PathType Leaf)) {
        Write-Error "$Description does not exist: $jsonPath"
    }
    return Get-Content -LiteralPath $jsonPath -Raw | ConvertFrom-Json
}

function Assert-SafeArtifactName {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Description
    )

    if ([string]::IsNullOrWhiteSpace($Name)) {
        Write-Error "$Description must not be blank."
    }
    if ($Name.Contains("/") -or $Name.Contains("\") -or $Name.Contains(":") -or
        $Name -match "(^|/)\.\.(/|$)") {
        Write-Error "$Description must be an artifact name, not a path."
    }
}

function Invoke-GitHubApiJson {
    param([Parameter(Mandatory = $true)][string]$Endpoint)

    $gh = Get-Command gh -ErrorAction SilentlyContinue
    if ($null -eq $gh) {
        Write-Error "gh CLI is required when fixture JSON paths are not provided."
    }

    $output = @(& $gh.Source "api" "-H" "Accept: application/vnd.github+json" $Endpoint 2>&1)
    if ($LASTEXITCODE -ne 0) {
        Write-Error "gh api $Endpoint failed: $([string]::Join("`n", $output))"
    }
    return ([string]::Join("`n", $output) | ConvertFrom-Json)
}

function New-ActionsRunsEndpoint {
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryFullName,
        [Parameter(Mandatory = $true)][string]$HeadBranch,
        [Parameter(Mandatory = $true)][int]$PageSize
    )

    $query = [System.Collections.Generic.List[string]]::new()
    $query.Add("status=success")
    $query.Add("per_page=$PageSize")
    if (-not [string]::IsNullOrWhiteSpace($HeadBranch)) {
        $query.Add("branch=$([uri]::EscapeDataString($HeadBranch))")
    }
    return "/repos/$RepositoryFullName/actions/runs?$([string]::Join('&', @($query)))"
}

function Get-WorkflowRuns {
    param([AllowNull()]$RunsJson)

    if ($null -eq $RunsJson) {
        return @()
    }
    if ($RunsJson -is [array]) {
        return @($RunsJson)
    }
    $runs = Get-JsonPropertyValue -JsonObject $RunsJson -Name "workflow_runs"
    if ($null -eq $runs) {
        return @()
    }
    return @($runs)
}

function Get-RunSortKey {
    param([AllowNull()]$Run)

    foreach ($name in @("created_at", "run_started_at", "updated_at")) {
        $value = Get-JsonPropertyValue -JsonObject $Run -Name $name
        if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
            return [string]$value
        }
    }
    return ""
}

function Test-WorkflowRunMatches {
    param(
        [AllowNull()]$Run,
        [Parameter(Mandatory = $true)][string]$WorkflowName,
        [Parameter(Mandatory = $true)][string]$HeadBranch
    )

    $runName = [string](Get-JsonPropertyValue -JsonObject $Run -Name "name")
    if (-not [string]::IsNullOrWhiteSpace($WorkflowName) -and
        -not $runName.Equals($WorkflowName, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $false
    }

    $runBranch = [string](Get-JsonPropertyValue -JsonObject $Run -Name "head_branch")
    if (-not [string]::IsNullOrWhiteSpace($HeadBranch) -and
        -not $runBranch.Equals($HeadBranch, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $false
    }

    $conclusion = [string](Get-JsonPropertyValue -JsonObject $Run -Name "conclusion")
    $status = [string](Get-JsonPropertyValue -JsonObject $Run -Name "status")
    if (-not [string]::IsNullOrWhiteSpace($conclusion)) {
        return $conclusion.Equals("success", [System.StringComparison]::OrdinalIgnoreCase)
    }
    return $status.Equals("success", [System.StringComparison]::OrdinalIgnoreCase) -or
        $status.Equals("completed", [System.StringComparison]::OrdinalIgnoreCase)
}

function Select-LatestWorkflowRun {
    param(
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][object[]]$Runs,
        [Parameter(Mandatory = $true)][string]$WorkflowName,
        [Parameter(Mandatory = $true)][string]$HeadBranch
    )

    $candidateRuns = [System.Collections.Generic.List[object]]::new()
    foreach ($run in @($Runs)) {
        if (Test-WorkflowRunMatches -Run $run -WorkflowName $WorkflowName -HeadBranch $HeadBranch) {
            $candidateRuns.Add($run)
        }
    }

    $sortedRuns = @($candidateRuns | Sort-Object -Property @{ Expression = { Get-RunSortKey -Run $_ }; Descending = $true })
    if ($sortedRuns.Count -eq 0) {
        return [ordered]@{
            run = $null
            candidate_count = 0
        }
    }
    return [ordered]@{
        run = $sortedRuns[0]
        candidate_count = $sortedRuns.Count
    }
}

function Get-Artifacts {
    param([AllowNull()]$ArtifactsJson)

    if ($null -eq $ArtifactsJson) {
        return @()
    }
    if ($ArtifactsJson -is [array]) {
        return @($ArtifactsJson)
    }
    $artifacts = Get-JsonPropertyValue -JsonObject $ArtifactsJson -Name "artifacts"
    if ($null -eq $artifacts) {
        return @()
    }
    return @($artifacts)
}

function Test-ArtifactPresent {
    param(
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][object[]]$Artifacts,
        [Parameter(Mandatory = $true)][string]$ArtifactName
    )

    $matching = 0
    $expired = 0
    foreach ($artifact in @($Artifacts)) {
        $name = [string](Get-JsonPropertyValue -JsonObject $artifact -Name "name")
        if (-not $name.Equals($ArtifactName, [System.StringComparison]::Ordinal)) {
            continue
        }
        $matching += 1
        $expiredValue = Get-JsonPropertyValue -JsonObject $artifact -Name "expired"
        $isExpired = if ($null -eq $expiredValue) { $false } else { [bool]$expiredValue }
        if ($isExpired) {
            $expired += 1
        }
    }

    return [ordered]@{
        present = ($matching -gt 0 -and $expired -eq 0)
        matching_count = $matching
        expired_count = $expired
    }
}

function Read-RunsJson {
    param(
        [AllowEmptyString()][string]$RunsJsonPath,
        [AllowEmptyString()][string]$RepositoryFullName,
        [AllowEmptyString()][string]$HeadBranch,
        [Parameter(Mandatory = $true)][int]$PageSize,
        [Parameter(Mandatory = $true)][string]$Description
    )

    if (-not [string]::IsNullOrWhiteSpace($RunsJsonPath)) {
        return Read-JsonFile -Path $RunsJsonPath -Description $Description
    }
    if ([string]::IsNullOrWhiteSpace($RepositoryFullName)) {
        return $null
    }
    $endpoint = New-ActionsRunsEndpoint -RepositoryFullName $RepositoryFullName -HeadBranch $HeadBranch -PageSize $PageSize
    return Invoke-GitHubApiJson -Endpoint $endpoint
}

function Read-ArtifactsJson {
    param(
        [AllowEmptyString()][string]$ArtifactsJsonPath,
        [AllowEmptyString()][string]$RepositoryFullName,
        [AllowEmptyString()][string]$RunId,
        [Parameter(Mandatory = $true)][int]$PageSize,
        [Parameter(Mandatory = $true)][string]$Description
    )

    if (-not [string]::IsNullOrWhiteSpace($ArtifactsJsonPath)) {
        return Read-JsonFile -Path $ArtifactsJsonPath -Description $Description
    }
    if ([string]::IsNullOrWhiteSpace($RepositoryFullName) -or [string]::IsNullOrWhiteSpace($RunId)) {
        return $null
    }
    $endpoint = "/repos/$RepositoryFullName/actions/runs/$RunId/artifacts?per_page=$PageSize"
    return Invoke-GitHubApiJson -Endpoint $endpoint
}

function Get-RunDiscovery {
    param(
        [Parameter(Mandatory = $true)][string]$WorkflowName,
        [Parameter(Mandatory = $true)][string]$ArtifactName,
        [AllowEmptyString()][string]$RunsJsonPath,
        [AllowEmptyString()][string]$ArtifactsJsonPath,
        [AllowEmptyString()][string]$RepositoryFullName,
        [AllowEmptyString()][string]$HeadBranch,
        [Parameter(Mandatory = $true)][int]$PageSize,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $runsJson = Read-RunsJson `
        -RunsJsonPath $RunsJsonPath `
        -RepositoryFullName $RepositoryFullName `
        -HeadBranch $HeadBranch `
        -PageSize $PageSize `
        -Description "$Description runs JSON"
    $runs = @(Get-WorkflowRuns -RunsJson $runsJson)
    $selection = Select-LatestWorkflowRun -Runs $runs -WorkflowName $WorkflowName -HeadBranch $HeadBranch
    $run = $selection.run
    $runIdValue = if ($null -eq $run) { "" } else { [string](Get-JsonPropertyValue -JsonObject $run -Name "id") }
    $artifactsJson = Read-ArtifactsJson `
        -ArtifactsJsonPath $ArtifactsJsonPath `
        -RepositoryFullName $RepositoryFullName `
        -RunId $runIdValue `
        -PageSize $PageSize `
        -Description "$Description artifacts JSON"
    $artifacts = @(Get-Artifacts -ArtifactsJson $artifactsJson)
    $artifactSummary = Test-ArtifactPresent -Artifacts $artifacts -ArtifactName $ArtifactName

    return [ordered]@{
        run = $run
        run_id = $runIdValue
        run_present = -not [string]::IsNullOrWhiteSpace($runIdValue)
        candidate_runs = [int]$selection.candidate_count
        artifact_present = [bool]$artifactSummary.present
        artifact_matches = [int]$artifactSummary.matching_count
        artifact_expired = [int]$artifactSummary.expired_count
        artifact_count = $artifacts.Count
        ready = (-not [string]::IsNullOrWhiteSpace($runIdValue) -and [bool]$artifactSummary.present)
    }
}

if (-not [string]::IsNullOrWhiteSpace($RepoFullName) -and
    $RepoFullName -notmatch '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$') {
    Write-Error "RepoFullName must be in owner/repo form."
}
foreach ($artifactSpec in @(
        @{ Name = $SourceArtifactName; Description = "SourceArtifactName" },
        @{ Name = $MetalMemoryProfilingArtifactName; Description = "MetalMemoryProfilingArtifactName" }
    )) {
    Assert-SafeArtifactName -Name ([string]$artifactSpec.Name) -Description ([string]$artifactSpec.Description)
}

$sourceDiscovery = Get-RunDiscovery `
    -WorkflowName $SourceWorkflowName `
    -ArtifactName $SourceArtifactName `
    -RunsJsonPath $SourceRunsJsonPath `
    -ArtifactsJsonPath $SourceArtifactsJsonPath `
    -RepositoryFullName $RepoFullName `
    -HeadBranch $Branch `
    -PageSize $PerPage `
    -Description "source workflow"

$metalDiscovery = Get-RunDiscovery `
    -WorkflowName $MetalWorkflowName `
    -ArtifactName $MetalMemoryProfilingArtifactName `
    -RunsJsonPath $MetalRunsJsonPath `
    -ArtifactsJsonPath $MetalArtifactsJsonPath `
    -RepositoryFullName $RepoFullName `
    -HeadBranch $Branch `
    -PageSize $PerPage `
    -Description "Metal memory/profiling workflow"

$status = "ready_for_final_from_runs_workflow"
$nextAction = "run_final_from_runs_workflow"
if (-not [bool]$sourceDiscovery.ready) {
    $status = "source_run_required"
    $nextAction = "wait_for_current_run_artifact_intake"
} elseif (-not [bool]$metalDiscovery.ready) {
    $status = "metal_memory_profiling_run_required"
    $nextAction = "run_metal_memory_profiling_capable_host_workflow"
}

$sourceRunId = [string]$sourceDiscovery.run_id
$metalRunId = [string]$metalDiscovery.run_id

Write-Output "validation_recipe=renderer-commercial-readiness-final-run-discovery"
Write-Output "renderer_commercial_readiness_final_run_discovery_mode=$Mode"
if (-not [string]::IsNullOrWhiteSpace($RepoFullName)) {
    Write-Output "renderer_commercial_readiness_final_run_discovery_repo=$RepoFullName"
}
Write-Output "renderer_commercial_readiness_final_run_discovery_branch=$(ConvertTo-CounterValue -Value $Branch)"
Write-Output "renderer_commercial_readiness_final_run_discovery_status=$status"
Write-Output "renderer_commercial_readiness_final_run_discovery_next_action=$nextAction"
Write-Output "renderer_commercial_readiness_final_run_discovery_official_context7_rest=/websites/github_en_rest"
Write-Output "renderer_commercial_readiness_final_run_discovery_official_context7_actions=/websites/github_en_actions"
Write-Output "renderer_commercial_readiness_final_run_discovery_actions_runs_endpoint=$runsEndpointTemplate"
Write-Output "renderer_commercial_readiness_final_run_discovery_actions_run_artifacts_endpoint=$artifactsEndpointTemplate"
Write-Output "renderer_commercial_readiness_final_run_discovery_source_workflow=$(ConvertTo-CounterValue -Value $SourceWorkflowName)"
Write-Output "renderer_commercial_readiness_final_run_discovery_source_artifact_name=$SourceArtifactName"
Write-Output "renderer_commercial_readiness_final_run_discovery_source_runs_discovered=$($sourceDiscovery.candidate_runs)"
Write-Output "renderer_commercial_readiness_final_run_discovery_source_run_present=$(ConvertTo-CounterBit ([bool]$sourceDiscovery.run_present))"
Write-Output "renderer_commercial_readiness_final_run_discovery_source_run_ready=$(ConvertTo-CounterBit ([bool]$sourceDiscovery.ready))"
if (-not [string]::IsNullOrWhiteSpace($sourceRunId)) {
    Write-Output "renderer_commercial_readiness_final_run_discovery_source_run_id=$sourceRunId"
}
Write-Output "renderer_commercial_readiness_final_run_discovery_source_artifacts_discovered=$($sourceDiscovery.artifact_count)"
Write-Output "renderer_commercial_readiness_final_run_discovery_source_artifact_matches=$($sourceDiscovery.artifact_matches)"
Write-Output "renderer_commercial_readiness_final_run_discovery_source_artifact_expired=$($sourceDiscovery.artifact_expired)"
Write-Output "renderer_commercial_readiness_final_run_discovery_source_artifact_present=$(ConvertTo-CounterBit ([bool]$sourceDiscovery.artifact_present))"
Write-Output "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_workflow=$(ConvertTo-CounterValue -Value $MetalWorkflowName)"
Write-Output "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_artifact_name=$MetalMemoryProfilingArtifactName"
Write-Output "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_runs_discovered=$($metalDiscovery.candidate_runs)"
Write-Output "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_run_present=$(ConvertTo-CounterBit ([bool]$metalDiscovery.run_present))"
Write-Output "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_run_ready=$(ConvertTo-CounterBit ([bool]$metalDiscovery.ready))"
if (-not [string]::IsNullOrWhiteSpace($metalRunId)) {
    Write-Output "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_run_id=$metalRunId"
}
Write-Output "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_artifacts_discovered=$($metalDiscovery.artifact_count)"
Write-Output "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_artifact_matches=$($metalDiscovery.artifact_matches)"
Write-Output "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_artifact_expired=$($metalDiscovery.artifact_expired)"
Write-Output "renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_artifact_present=$(ConvertTo-CounterBit ([bool]$metalDiscovery.artifact_present))"

if ($status -ceq "ready_for_final_from_runs_workflow") {
    $finalCommand = "gh workflow run $finalFromRunsWorkflowName -f source_artifact_run_id=$sourceRunId -f metal_memory_profiling_run_id=$metalRunId -f confirm_final_retained_root_handoff=renderer-commercial-final-retained-root"
    Write-Output "renderer_commercial_readiness_final_run_discovery_final_from_runs_workflow_command=$finalCommand"
}

Write-Output "renderer_commercial_readiness_final_run_discovery_workflow_dispatched=0"
Write-Output "renderer_commercial_readiness_final_run_discovery_artifacts_downloaded=0"
Write-Output "renderer_commercial_readiness_final_run_discovery_gpu_workload_executed=0"
Write-Output "renderer_commercial_readiness_final_run_discovery_evidence_assembled=0"
Write-Output "renderer_backend_parity_ready=0"
Write-Output "renderer_metal_broad_readiness=0"
Write-Output "renderer_broad_quality_ready=0"
Write-Output "renderer_commercial_readiness=0"
Write-Output "renderer_environment_ready=0"
