#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$RepoFullName = "",

    [string]$RunnersJsonPath = "",

    [string]$RepositoryJsonPath = "",

    [string]$ConfirmPublicRepoSelfHostedRunnerSecurityReview = "",

    [string]$PublicRepoRunnerSecurityReviewRelative = "",

    [string]$IntakeManifestRelative = "",

    [string]$SourceRunId = "",

    [string]$MetalMemoryProfilingRunId = "",

    [switch]$UseRunDiscovery,

    [string]$RunDiscoveryBranch = "main",

    [string]$RunDiscoverySourceRunsJsonPath = "",

    [string]$RunDiscoverySourceArtifactsJsonPath = "",

    [string]$RunDiscoveryMetalRunsJsonPath = "",

    [string]$RunDiscoveryMetalArtifactsJsonPath = ""
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
        [string]$RepositoryFullName = "",
        [string]$RunnerJsonPath = ""
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

function Invoke-FinalRunDiscovery {
    param(
        [string]$RepositoryFullName = "",
        [string]$HeadBranch = "main",
        [string]$SourceRunsJsonPath = "",
        [string]$SourceArtifactsJsonPath = "",
        [string]$MetalRunsJsonPath = "",
        [string]$MetalArtifactsJsonPath = ""
    )

    $discoveryScript = Join-Path $PSScriptRoot "plan-renderer-commercial-readiness-final-run-discovery.ps1"
    if (-not (Test-Path -LiteralPath $discoveryScript -PathType Leaf)) {
        Write-Error "required_tool_missing: tools/plan-renderer-commercial-readiness-final-run-discovery.ps1"
    }

    $lines = @(& $discoveryScript `
            -Mode Plan `
            -RepoFullName $RepositoryFullName `
            -Branch $HeadBranch `
            -SourceRunsJsonPath $SourceRunsJsonPath `
            -SourceArtifactsJsonPath $SourceArtifactsJsonPath `
            -MetalRunsJsonPath $MetalRunsJsonPath `
            -MetalArtifactsJsonPath $MetalArtifactsJsonPath)
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

function Test-JsonBooleanTrue {
    param([AllowNull()]$Value)

    if ($null -eq $Value -or -not ($Value -is [bool])) {
        return $false
    }
    return [bool]$Value
}

function Test-StringSetEquals {
    param(
        [AllowNull()]$Value,
        [Parameter(Mandatory = $true)][string[]]$ExpectedValues
    )

    $actualSet = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($item in @(Select-StringArray -Value $Value)) {
        $null = $actualSet.Add($item)
    }
    if ($actualSet.Count -ne @($ExpectedValues).Count) {
        return $false
    }
    foreach ($expectedValue in @($ExpectedValues)) {
        if (-not $actualSet.Contains($expectedValue)) {
            return $false
        }
    }
    return $true
}

function Test-StringArrayContains {
    param(
        [AllowNull()]$Value,
        [Parameter(Mandatory = $true)][string]$ExpectedValue
    )

    foreach ($item in @(Select-StringArray -Value $Value)) {
        if ($item -ceq $ExpectedValue) {
            return $true
        }
    }
    return $false
}

function Test-PublicRepoRunnerSecurityReviewWorkflowAudit {
    param(
        [AllowNull()]$Review,
        [Parameter(Mandatory = $true)][string[]]$RequiredLabels
    )

    $workflowFile = [string](Get-JsonPropertyValue -JsonObject $Review -Name "reviewed_workflow_file")
    if ($workflowFile -cne ".github/workflows/renderer-metal-memory-profiling-capable-host.yml") {
        return $false
    }

    $audit = Get-JsonPropertyValue -JsonObject $Review -Name "workflow_audit"
    if ($null -eq $audit) {
        return $false
    }
    if (-not (Test-JsonBooleanTrue -Value (Get-JsonPropertyValue -JsonObject $audit -Name "workflow_dispatch_only"))) {
        return $false
    }
    if (Test-JsonBooleanTrue -Value (Get-JsonPropertyValue -JsonObject $audit -Name "untrusted_pr_triggers_present")) {
        return $false
    }
    if (-not (Test-JsonBooleanTrue -Value (Get-JsonPropertyValue -JsonObject $audit -Name "contents_permission_read_only"))) {
        return $false
    }
    if (-not (Test-JsonBooleanTrue -Value (Get-JsonPropertyValue -JsonObject $audit -Name "checkout_action_pinned"))) {
        return $false
    }
    if (-not (Test-JsonBooleanTrue -Value (Get-JsonPropertyValue -JsonObject $audit -Name "checkout_persist_credentials_disabled"))) {
        return $false
    }
    if (-not (Test-JsonBooleanTrue -Value (Get-JsonPropertyValue -JsonObject $audit -Name "confirm_input_required"))) {
        return $false
    }
    if (-not (Test-StringSetEquals `
                -Value (Get-JsonPropertyValue -JsonObject $audit -Name "required_labels") `
                -ExpectedValues $RequiredLabels)) {
        return $false
    }

    return $true
}

function Test-PublicRepoRunnerSecurityReviewArtifact {
    param(
        [AllowNull()]$Review,
        [string]$RepositoryFullName = "",
        [Parameter(Mandatory = $true)][string[]]$RequiredLabels
    )

    if ($null -eq $Review) {
        return $false
    }

    if ([string](Get-JsonPropertyValue -JsonObject $Review -Name "schema_version") -cne
        "GameEngine.RendererPublicSelfHostedRunnerSecurityReview.v1") {
        return $false
    }
    if (-not [string]::IsNullOrWhiteSpace($RepositoryFullName) -and
        [string](Get-JsonPropertyValue -JsonObject $Review -Name "repo_full_name") -cne $RepositoryFullName) {
        return $false
    }
    if ([string](Get-JsonPropertyValue -JsonObject $Review -Name "repository_visibility") -cne "public") {
        return $false
    }
    if ([string](Get-JsonPropertyValue -JsonObject $Review -Name "review_status") -cne "approved") {
        return $false
    }
    foreach ($requiredBoolean in @(
            "reviewed_public_fork_pr_risk",
            "reviewed_runner_isolation",
            "reviewed_secret_exposure",
            "reviewed_metal_probe_truth"
        )) {
        if (-not (Test-JsonBooleanTrue -Value (Get-JsonPropertyValue -JsonObject $Review -Name $requiredBoolean))) {
            return $false
        }
    }
    if (-not (Test-StringArrayContains `
                -Value (Get-JsonPropertyValue -JsonObject $Review -Name "reviewed_allowed_workflows") `
                -ExpectedValue "renderer-metal-memory-profiling-capable-host.yml")) {
        return $false
    }
    if (-not (Test-StringSetEquals `
                -Value (Get-JsonPropertyValue -JsonObject $Review -Name "reviewed_required_labels") `
                -ExpectedValues $RequiredLabels)) {
        return $false
    }
    if (-not (Test-PublicRepoRunnerSecurityReviewWorkflowAudit -Review $Review -RequiredLabels $RequiredLabels)) {
        return $false
    }

    return $true
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

function Get-GitHubRepositoryUrl {
    param([string]$RepositoryFullName = "")

    if ([string]::IsNullOrWhiteSpace($RepositoryFullName)) {
        return "https://github.com/<owner>/<repo>"
    }
    return "https://github.com/$RepositoryFullName"
}

function Get-GitHubRunnerRegistrationTokenEndpoint {
    param([string]$RepositoryFullName = "")

    if ([string]::IsNullOrWhiteSpace($RepositoryFullName)) {
        return "/repos/<owner>/<repo>/actions/runners/registration-token"
    }
    return "/repos/$RepositoryFullName/actions/runners/registration-token"
}

function Invoke-GitHubRepositoryMetadata {
    param([string]$RepositoryFullName = "")

    if ([string]::IsNullOrWhiteSpace($RepositoryFullName)) {
        return $null
    }

    $endpoint = "/repos/$RepositoryFullName"
    $output = & gh api $endpoint -H "Accept: application/vnd.github+json" 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "gh api $endpoint failed: $(@($output) -join "`n")"
    }
    return (@($output) -join "`n") | ConvertFrom-Json
}

if (-not [string]::IsNullOrWhiteSpace($RepoFullName) -and
    $RepoFullName -notmatch '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$') {
    Write-Error "RepoFullName must be in owner/repo form."
}
if (-not [string]::IsNullOrWhiteSpace($ConfirmPublicRepoSelfHostedRunnerSecurityReview) -and
    $ConfirmPublicRepoSelfHostedRunnerSecurityReview -cne "public-repo-self-hosted-runner-risk-reviewed") {
    Write-Error "ConfirmPublicRepoSelfHostedRunnerSecurityReview must equal public-repo-self-hosted-runner-risk-reviewed when provided."
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

$repositoryMetadata = $null
$repositoryMetadataKnown = $false
$repositoryVisibility = "unknown"
$repositoryPrivate = $false
$repositoryPrivateKnown = $false
$publicRepoSecurityReviewRequired = $true
$publicRepoSecurityReviewConfirmed = $false
$publicRepoRegistrationBlocked = $false
$publicRepoSecurityReviewConfirmationRequired = "public-repo-self-hosted-runner-risk-reviewed"
$publicRepoSecurityReviewArtifactPresent = -not [string]::IsNullOrWhiteSpace($PublicRepoRunnerSecurityReviewRelative)
$publicRepoSecurityReviewArtifactValid = $false
$publicRepoSecurityReviewArtifactWorkflowAuditValid = $false
$publicRepoSecurityReviewArtifactStatus = "not_supplied"

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

$runDiscoveryKnown = $UseRunDiscovery.IsPresent
$runDiscoveryStatus = "not_requested"
$runDiscoveryNextAction = "not_requested"
$runDiscoverySourceRunId = ""
$runDiscoverySourceRunReady = $false
$runDiscoveryMetalRunId = ""
$runDiscoveryMetalRunReady = $false
if ($runDiscoveryKnown) {
    $runDiscoveryMap = Invoke-FinalRunDiscovery `
        -RepositoryFullName $RepoFullName `
        -HeadBranch $RunDiscoveryBranch `
        -SourceRunsJsonPath $RunDiscoverySourceRunsJsonPath `
        -SourceArtifactsJsonPath $RunDiscoverySourceArtifactsJsonPath `
        -MetalRunsJsonPath $RunDiscoveryMetalRunsJsonPath `
        -MetalArtifactsJsonPath $RunDiscoveryMetalArtifactsJsonPath
    $runDiscoveryStatus = [string]$runDiscoveryMap["renderer_commercial_readiness_final_run_discovery_status"]
    $runDiscoveryNextAction = [string]$runDiscoveryMap["renderer_commercial_readiness_final_run_discovery_next_action"]
    $runDiscoverySourceRunId = [string]$runDiscoveryMap["renderer_commercial_readiness_final_run_discovery_source_run_id"]
    $runDiscoverySourceRunReady = [string]$runDiscoveryMap["renderer_commercial_readiness_final_run_discovery_source_run_ready"] -ceq "1"
    $runDiscoveryMetalRunId = [string]$runDiscoveryMap["renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_run_id"]
    $runDiscoveryMetalRunReady = [string]$runDiscoveryMap["renderer_commercial_readiness_final_run_discovery_metal_memory_profiling_run_ready"] -ceq "1"
}

$manifestRunId = if ($manifestPresent) {
    [string](Get-JsonPropertyValue -JsonObject $manifest -Name "run_id")
} else {
    ""
}
$resolvedSourceRunId = if (-not [string]::IsNullOrWhiteSpace($SourceRunId)) {
    $SourceRunId
} elseif (-not [string]::IsNullOrWhiteSpace($manifestRunId)) {
    $manifestRunId
} elseif ($runDiscoverySourceRunReady -and -not [string]::IsNullOrWhiteSpace($runDiscoverySourceRunId)) {
    $runDiscoverySourceRunId
} else {
    ""
}
$sourceRunReady = -not [string]::IsNullOrWhiteSpace($resolvedSourceRunId)
$resolvedMetalMemoryProfilingRunId = if (-not [string]::IsNullOrWhiteSpace($MetalMemoryProfilingRunId)) {
    $MetalMemoryProfilingRunId
} elseif ($runDiscoveryMetalRunReady -and -not [string]::IsNullOrWhiteSpace($runDiscoveryMetalRunId)) {
    $runDiscoveryMetalRunId
} else {
    ""
}
$metalRunReady = -not [string]::IsNullOrWhiteSpace($resolvedMetalMemoryProfilingRunId)

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
} elseif (-not $manifestPresent -and -not $runDiscoveryKnown) {
    $status = "artifact_intake_required"
    $nextAction = "inspect_current_run_artifact_intake"
} elseif (-not $sourceRunReady) {
    $status = "source_run_required"
    $nextAction = "identify_source_artifact_run"
} elseif (-not $manifestPresent -and $runDiscoveryKnown -and -not $metalRunReady) {
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
} elseif (-not $manifestPresent -and $runDiscoveryKnown -and $metalRunReady) {
    $status = "ready_for_final_from_runs_workflow"
    $nextAction = "run_final_from_runs_workflow"
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
    $finalFromRunsWorkflowCommand = "gh workflow run renderer-commercial-readiness-final-from-runs.yml -f source_artifact_run_id=$resolvedSourceRunId -f metal_memory_profiling_run_id=$resolvedMetalMemoryProfilingRunId -f confirm_final_retained_root_handoff=renderer-commercial-final-retained-root"
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
$runnerRequiredLabels = @("self-hosted", "macOS", "ARM64", "metal-residency-set")
$runnerRequiredLabelText = [string]::Join(",", $runnerRequiredLabels)
$runnerRegistrationTokenEndpoint = Get-GitHubRunnerRegistrationTokenEndpoint -RepositoryFullName $RepoFullName
$runnerRegistrationTokenCommand = "gh api -X POST -H `"Accept: application/vnd.github+json`" $runnerRegistrationTokenEndpoint"
$runnerConfigCommandTemplate = "./config.sh --url $(Get-GitHubRepositoryUrl -RepositoryFullName $RepoFullName) --token <registration-token> --labels metal-residency-set"

if ($nextAction -ceq "provision_capable_host_runner") {
    if (-not [string]::IsNullOrWhiteSpace($RepositoryJsonPath)) {
        $repositoryMetadata = Read-JsonFile -RelativePath $RepositoryJsonPath -Label "RepositoryJsonPath"
        $repositoryMetadataKnown = $true
    } elseif (-not [string]::IsNullOrWhiteSpace($RepoFullName) -and
        [string]::IsNullOrWhiteSpace($RunnersJsonPath)) {
        $repositoryMetadata = Invoke-GitHubRepositoryMetadata -RepositoryFullName $RepoFullName
        $repositoryMetadataKnown = $null -ne $repositoryMetadata
    }

    if ($repositoryMetadataKnown) {
        $visibilityValue = Get-JsonPropertyValue -JsonObject $repositoryMetadata -Name "visibility"
        if (-not [string]::IsNullOrWhiteSpace([string]$visibilityValue)) {
            $repositoryVisibility = [string]$visibilityValue
        }

        $privateValue = Get-JsonPropertyValue -JsonObject $repositoryMetadata -Name "private"
        if ($null -ne $privateValue) {
            $repositoryPrivateKnown = $true
            $repositoryPrivate = [bool]$privateValue
        }

        $isPublicRepository = $repositoryVisibility -ceq "public" -or
            ($repositoryPrivateKnown -and -not $repositoryPrivate)
        $publicRepoSecurityReviewRequired = $isPublicRepository
    }

    if ($publicRepoSecurityReviewArtifactPresent) {
        $publicRepoSecurityReviewArtifact = Read-JsonFile `
            -RelativePath $PublicRepoRunnerSecurityReviewRelative `
            -Label "PublicRepoRunnerSecurityReviewRelative"
        $reviewStatusValue = Get-JsonPropertyValue -JsonObject $publicRepoSecurityReviewArtifact -Name "review_status"
        if (-not [string]::IsNullOrWhiteSpace([string]$reviewStatusValue)) {
            $publicRepoSecurityReviewArtifactStatus = [string]$reviewStatusValue
        } else {
            $publicRepoSecurityReviewArtifactStatus = "missing_review_status"
        }
        $publicRepoSecurityReviewArtifactWorkflowAuditValid = Test-PublicRepoRunnerSecurityReviewWorkflowAudit `
            -Review $publicRepoSecurityReviewArtifact `
            -RequiredLabels $runnerRequiredLabels
        $publicRepoSecurityReviewArtifactValid = Test-PublicRepoRunnerSecurityReviewArtifact `
            -Review $publicRepoSecurityReviewArtifact `
            -RepositoryFullName $RepoFullName `
            -RequiredLabels $runnerRequiredLabels
    }

    $publicRepoSecurityReviewConfirmedByString = $ConfirmPublicRepoSelfHostedRunnerSecurityReview -ceq
        $publicRepoSecurityReviewConfirmationRequired
    $publicRepoSecurityReviewConfirmedByArtifact = $publicRepoSecurityReviewArtifactPresent -and
        $publicRepoSecurityReviewArtifactValid
    $publicRepoSecurityReviewConfirmed = if ($publicRepoSecurityReviewArtifactPresent -and
        -not $publicRepoSecurityReviewArtifactValid) {
        $false
    } else {
        $publicRepoSecurityReviewConfirmedByString -or $publicRepoSecurityReviewConfirmedByArtifact
    }
    $publicRepoSecurityReviewArtifactInvalid = $publicRepoSecurityReviewArtifactPresent -and
        -not $publicRepoSecurityReviewArtifactValid
    $publicRepoRegistrationBlocked = ($publicRepoSecurityReviewRequired -and -not $publicRepoSecurityReviewConfirmed) -or
        $publicRepoSecurityReviewArtifactInvalid
    if ($publicRepoRegistrationBlocked -and
        ($repositoryMetadataKnown -or $publicRepoSecurityReviewArtifactInvalid)) {
        $status = "public_runner_security_review_required"
        $nextAction = "complete_public_runner_security_review"
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
Write-Output "renderer_commercial_readiness_final_handoff_run_discovery_known=$(ConvertTo-CounterBit $runDiscoveryKnown)"
if ($runDiscoveryKnown) {
    Write-Output "renderer_commercial_readiness_final_handoff_run_discovery_branch=$(ConvertTo-CounterValue -Value $RunDiscoveryBranch)"
    Write-Output "renderer_commercial_readiness_final_handoff_run_discovery_status=$(ConvertTo-CounterValue -Value $runDiscoveryStatus)"
    Write-Output "renderer_commercial_readiness_final_handoff_run_discovery_next_action=$(ConvertTo-CounterValue -Value $runDiscoveryNextAction)"
    Write-Output "renderer_commercial_readiness_final_handoff_run_discovery_source_run_ready=$(ConvertTo-CounterBit $runDiscoverySourceRunReady)"
    Write-Output "renderer_commercial_readiness_final_handoff_run_discovery_metal_memory_profiling_run_ready=$(ConvertTo-CounterBit $runDiscoveryMetalRunReady)"
}
Write-Output "renderer_commercial_readiness_final_handoff_source_run_ready=$(ConvertTo-CounterBit $sourceRunReady)"
if ($sourceRunReady) {
    Write-Output "renderer_commercial_readiness_final_handoff_source_run_id=$resolvedSourceRunId"
}
Write-Output "renderer_commercial_readiness_final_handoff_metal_memory_profiling_run_ready=$(ConvertTo-CounterBit $metalRunReady)"
if ($metalRunReady) {
    Write-Output "renderer_commercial_readiness_final_handoff_metal_memory_profiling_run_id=$resolvedMetalMemoryProfilingRunId"
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
if ($nextAction -ceq "provision_capable_host_runner" -or
    $nextAction -ceq "complete_public_runner_security_review") {
    Write-Output "renderer_commercial_readiness_final_handoff_runner_required_labels=$runnerRequiredLabelText"
    Write-Output "renderer_commercial_readiness_final_handoff_runner_repository_metadata_known=$(ConvertTo-CounterBit $repositoryMetadataKnown)"
    if ($repositoryMetadataKnown) {
        Write-Output "renderer_commercial_readiness_final_handoff_runner_repository_visibility=$(ConvertTo-CounterValue -Value $repositoryVisibility)"
        if ($repositoryPrivateKnown) {
            Write-Output "renderer_commercial_readiness_final_handoff_runner_repository_private=$(ConvertTo-CounterBit $repositoryPrivate)"
        }
    }
    Write-Output "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_required=$(ConvertTo-CounterBit $publicRepoSecurityReviewRequired)"
    Write-Output "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmed=$(ConvertTo-CounterBit $publicRepoSecurityReviewConfirmed)"
    Write-Output "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_confirmation_required=$publicRepoSecurityReviewConfirmationRequired"
    Write-Output "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_present=$(ConvertTo-CounterBit $publicRepoSecurityReviewArtifactPresent)"
    if ($publicRepoSecurityReviewArtifactPresent) {
        Write-Output "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact=$PublicRepoRunnerSecurityReviewRelative"
        Write-Output "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_valid=$(ConvertTo-CounterBit $publicRepoSecurityReviewArtifactValid)"
        Write-Output "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_status=$(ConvertTo-CounterValue -Value $publicRepoSecurityReviewArtifactStatus)"
        Write-Output "renderer_commercial_readiness_final_handoff_runner_public_repo_security_review_artifact_workflow_audit_valid=$(ConvertTo-CounterBit $publicRepoSecurityReviewArtifactWorkflowAuditValid)"
    }
    Write-Output "renderer_commercial_readiness_final_handoff_runner_public_repo_registration_blocked=$(ConvertTo-CounterBit $publicRepoRegistrationBlocked)"
    Write-Output "renderer_commercial_readiness_final_handoff_runner_registration_token_endpoint=$runnerRegistrationTokenEndpoint"
    Write-Output "renderer_commercial_readiness_final_handoff_runner_registration_token_expires_minutes=60"
    Write-Output "renderer_commercial_readiness_final_handoff_runner_label_truth_requires_metal_probe=1"
    if ($nextAction -ceq "provision_capable_host_runner") {
        Write-Output "renderer_commercial_readiness_final_handoff_runner_registration_token_command=$runnerRegistrationTokenCommand"
        Write-Output "renderer_commercial_readiness_final_handoff_runner_config_command_template=$runnerConfigCommandTemplate"
    }
}
Write-Output "renderer_backend_parity_ready=0"
Write-Output "renderer_metal_broad_readiness=0"
Write-Output "renderer_broad_quality_ready=0"
Write-Output "renderer_commercial_readiness=0"
Write-Output "renderer_environment_ready=0"

Write-Information "renderer-commercial-readiness-final-handoff: ok" -InformationAction Continue
