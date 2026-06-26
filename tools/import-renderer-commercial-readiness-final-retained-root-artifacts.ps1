#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10I
# Imports retained renderer commercial readiness GitHub Actions artifacts without promoting readiness.
# Known hosted Metal gate examples include mtlresidencyset_unavailable.

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("Plan", "Inspect", "Import")]
    [string]$Mode = "Plan",

    [ValidatePattern('^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$')]
    [string]$RepoFullName = "y2ikgm89/mirakanai-engine",

    [ValidatePattern('^\d+$')]
    [string]$RunId = "",

    [string]$OutputRootRelative = "artifacts/renderer/commercial-readiness-evidence/final-retained-root-artifact-import",

    [string[]]$ArtifactNames = @(
        "renderer-clean-room-legal-review-artifacts",
        "renderer-commercial-readiness-final-retained-root",
        "renderer-d3d12-commercial-quality-host-evidence",
        "renderer-metal-memory-profiling-host-artifacts",
        "renderer-package-commercial-quality-host-evidence",
        "metal-host-optimization-artifacts",
        "linux-vulkan-host-evidence",
        "windows-packages"
    ),

    [string]$ArtifactListJsonRelative = "",

    [switch]$NoWrite,
    [switch]$RequireReady
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

function Test-AllowedOutputRoot {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $normalizedPath = $RelativePath.Replace("\", "/")
    return $normalizedPath.StartsWith(
        "artifacts/renderer/commercial-readiness-evidence/",
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

function ConvertTo-RepoRelativePath {
    param([Parameter(Mandatory = $true)][string]$FullPath)

    $rootFull = [System.IO.Path]::GetFullPath($root).TrimEnd([System.IO.Path]::DirectorySeparatorChar)
    $normalizedFullPath = [System.IO.Path]::GetFullPath($FullPath)
    return ([System.IO.Path]::GetRelativePath($rootFull, $normalizedFullPath) -replace "\\", "/")
}

function Read-JsonFileOrNull {
    param([Parameter(Mandatory = $true)][string]$Path)

    try {
        return Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    } catch {
        return $null
    }
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

function Find-JsonEvidencePath {
    param(
        [Parameter(Mandatory = $true)][string]$SearchRootFull,
        [Parameter(Mandatory = $true)][string]$SchemaVersion,
        [string]$ClaimId = "",
        [string]$ArtifactId = ""
    )

    if (-not (Test-Path -LiteralPath $SearchRootFull -PathType Container)) {
        return ""
    }

    $jsonFiles = @(Get-ChildItem -LiteralPath $SearchRootFull -Recurse -File -Filter "*.json" |
            Sort-Object FullName)
    foreach ($jsonFile in $jsonFiles) {
        $json = Read-JsonFileOrNull -Path $jsonFile.FullName
        if ($null -eq $json) {
            continue
        }
        if ([string](Get-JsonPropertyValue -JsonObject $json -Name "schema_version") -cne $SchemaVersion) {
            continue
        }
        if (-not [string]::IsNullOrWhiteSpace($ClaimId) -and
            [string](Get-JsonPropertyValue -JsonObject $json -Name "claim_id") -cne $ClaimId) {
            continue
        }
        if (-not [string]::IsNullOrWhiteSpace($ArtifactId) -and
            [string](Get-JsonPropertyValue -JsonObject $json -Name "artifact_id") -cne $ArtifactId) {
            continue
        }
        return ConvertTo-RepoRelativePath -FullPath $jsonFile.FullName
    }
    return ""
}

function Find-HostGateSummary {
    param([Parameter(Mandatory = $true)][string]$SearchRootFull)

    if (-not (Test-Path -LiteralPath $SearchRootFull -PathType Container)) {
        return $null
    }

    $summary = Get-ChildItem -LiteralPath $SearchRootFull -Recurse -File -Filter "host-gate-summary.json" |
        Sort-Object FullName |
        Select-Object -First 1
    if ($null -eq $summary) {
        return $null
    }
    $json = Read-JsonFileOrNull -Path $summary.FullName
    if ($null -eq $json) {
        return $null
    }
    return [pscustomobject]@{
        Path = ConvertTo-RepoRelativePath -FullPath $summary.FullName
        Status = [string](Get-JsonPropertyValue -JsonObject $json -Name "status")
        Reason = [string](Get-JsonPropertyValue -JsonObject $json -Name "reason")
    }
}

function Get-WorkflowArtifactListSummary {
    param(
        [string]$ArtifactListRelativePath,
        [Parameter(Mandatory = $true)][string[]]$RequiredArtifactNames
    )

    $summary = [ordered]@{
        present = $false
        path = ""
        available_artifacts = 0
        available_artifact_names = @()
        missing_artifacts = @()
        expired_artifacts = @()
    }

    if ([string]::IsNullOrWhiteSpace($ArtifactListRelativePath)) {
        return $summary
    }

    $artifactListFull = Resolve-RepoRelativePath `
        -RelativePath $ArtifactListRelativePath `
        -Label "ArtifactListJsonRelative"
    if (-not (Test-Path -LiteralPath $artifactListFull -PathType Leaf)) {
        Write-Error "artifact_list_json_missing: ArtifactListJsonRelative must point to a GitHub Actions artifacts API JSON file."
    }

    $artifactListJson = Read-JsonFileOrNull -Path $artifactListFull
    if ($null -eq $artifactListJson) {
        Write-Error "artifact_list_json_invalid: ArtifactListJsonRelative must be valid JSON."
    }

    $artifactRows = Get-JsonPropertyValue -JsonObject $artifactListJson -Name "artifacts"
    if ($null -eq $artifactRows) {
        Write-Error "artifact_list_json_invalid: ArtifactListJsonRelative must contain an artifacts array."
    }

    $availableNames = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    $expiredNames = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($artifactRow in @($artifactRows)) {
        $name = [string](Get-JsonPropertyValue -JsonObject $artifactRow -Name "name")
        if ([string]::IsNullOrWhiteSpace($name)) {
            continue
        }
        $expiredValue = Get-JsonPropertyValue -JsonObject $artifactRow -Name "expired"
        $expired = $false
        if ($null -ne $expiredValue) {
            $expired = [bool]$expiredValue
        }
        if ($expired) {
            $null = $expiredNames.Add($name)
            continue
        }
        $null = $availableNames.Add($name)
    }

    $missingArtifacts = [System.Collections.Generic.List[string]]::new()
    foreach ($artifactName in @($RequiredArtifactNames)) {
        if (-not $availableNames.Contains($artifactName)) {
            $missingArtifacts.Add($artifactName) | Out-Null
        }
    }

    $summary.present = $true
    $summary.path = $ArtifactListRelativePath
    $summary.available_artifacts = $availableNames.Count
    $summary.available_artifact_names = @($availableNames | Sort-Object)
    $summary.missing_artifacts = @($missingArtifacts)
    $summary.expired_artifacts = @($expiredNames | Sort-Object)
    return $summary
}

function Write-Utf8NoBomJson {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)]$Value
    )

    $parent = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        $null = New-Item -ItemType Directory -Force -Path $parent
    }
    $json = $Value | ConvertTo-Json -Depth 16
    Set-Content -LiteralPath $Path -Encoding utf8NoBOM -Value $json
}

if (-not (Test-SafeRepoRelativePath -RelativePath $OutputRootRelative)) {
    Write-Error "unsafe_relative_path: OutputRootRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedOutputRoot -RelativePath $OutputRootRelative)) {
    Write-Error "OutputRootRelative must be under artifacts/renderer/commercial-readiness-evidence/."
}

$workflowArtifactListSummary = Get-WorkflowArtifactListSummary `
    -ArtifactListRelativePath $ArtifactListJsonRelative `
    -RequiredArtifactNames ([string[]]$ArtifactNames)

$finalRetainedRootArtifactName = "renderer-commercial-readiness-final-retained-root"
$assemblerSourceArtifactNames = @($ArtifactNames | Where-Object {
        [string]$_ -cne $finalRetainedRootArtifactName
    })
$availableWorkflowArtifactNames = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
foreach ($artifactName in @($workflowArtifactListSummary.available_artifact_names)) {
    if (-not [string]::IsNullOrWhiteSpace([string]$artifactName)) {
        $null = $availableWorkflowArtifactNames.Add([string]$artifactName)
    }
}
$finalRootWorkflowArtifactAvailable = [bool]$workflowArtifactListSummary.present -and
    $availableWorkflowArtifactNames.Contains($finalRetainedRootArtifactName)
$availableAssemblerSourceArtifacts = [System.Collections.Generic.List[string]]::new()
$missingAssemblerSourceArtifacts = [System.Collections.Generic.List[string]]::new()
if ([bool]$workflowArtifactListSummary.present) {
    foreach ($artifactName in @($assemblerSourceArtifactNames)) {
        if ($availableWorkflowArtifactNames.Contains([string]$artifactName)) {
            $availableAssemblerSourceArtifacts.Add([string]$artifactName) | Out-Null
        } else {
            $missingAssemblerSourceArtifacts.Add([string]$artifactName) | Out-Null
        }
    }
}

$requiredAssemblerInputs = @(
    @{
        Name = "d3d12_host_evidence"
        Schema = "GameEngine.RendererD3d12CommercialQualityHostEvidence.v1"
        Claim = "renderer-d3d12-commercial-quality-artifact-v1"
    },
    @{
        Name = "vulkan_strict_host_evidence"
        Schema = "GameEngine.RendererVulkanStrictCommercialQualityHostEvidence.v1"
        Claim = "renderer-vulkan-strict-commercial-quality-artifact-v1"
    },
    @{
        Name = "apple_metal_host_evidence"
        Schema = "GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1"
        Claim = "renderer-apple-metal-commercial-quality-artifact-v1"
    },
    @{
        Name = "metal_memory_profiling_host_evidence"
        Schema = "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1"
        Claim = "renderer-metal-memory-profiling-host-evidence-v1"
    },
    @{
        Name = "package_host_evidence"
        Schema = "GameEngine.RendererPackageCommercialQualityHostEvidence.v1"
        Claim = "renderer-package-commercial-quality-artifacts-v1"
    },
    @{
        Name = "quality_vfx_host_evidence"
        Schema = "GameEngine.RendererQualityVfxCommercialHostEvidence.v1"
        Claim = "renderer-quality-vfx-commercial-artifacts-v1"
    },
    @{
        Name = "clean_room_legal_review"
        Schema = "GameEngine.RendererCleanRoomLegalReviewInput.v1"
        Claim = "renderer-clean-room-legal-artifact-v1"
    }
)

Write-Output "validation_recipe=renderer-commercial-readiness-final-retained-root-artifact-import"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_mode=$Mode"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_repo=$RepoFullName"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_output_root=$OutputRootRelative"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_required_workflow_artifacts=$($ArtifactNames.Count)"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_required_assembler_inputs=$($requiredAssemblerInputs.Count)"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_workflow_artifact_list_present=$(ConvertTo-CounterBit ([bool]$workflowArtifactListSummary.present))"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_available_workflow_artifacts=$($workflowArtifactListSummary.available_artifacts)"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_missing_workflow_artifacts=$(@($workflowArtifactListSummary.missing_artifacts).Count)"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_expired_workflow_artifacts=$(@($workflowArtifactListSummary.expired_artifacts).Count)"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_final_root_workflow_artifact_available=$(ConvertTo-CounterBit $finalRootWorkflowArtifactAvailable)"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_source_workflow_artifacts=$(@($availableAssemblerSourceArtifacts).Count)"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_source_workflow_artifacts=$(@($missingAssemblerSourceArtifacts).Count)"
if (@($workflowArtifactListSummary.missing_artifacts).Count -gt 0) {
    Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_missing_workflow_artifact_names=$((@($workflowArtifactListSummary.missing_artifacts) | ForEach-Object { ConvertTo-CounterValue -Value ([string]$_) }) -join ',')"
}
if (@($missingAssemblerSourceArtifacts).Count -gt 0) {
    Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_source_workflow_artifact_names=$((@($missingAssemblerSourceArtifacts) | ForEach-Object { ConvertTo-CounterValue -Value ([string]$_) }) -join ',')"
}
if (@($workflowArtifactListSummary.expired_artifacts).Count -gt 0) {
    Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_expired_workflow_artifact_names=$((@($workflowArtifactListSummary.expired_artifacts) | ForEach-Object { ConvertTo-CounterValue -Value ([string]$_) }) -join ',')"
}
Write-Output "renderer_backend_parity_ready=0"
Write-Output "renderer_metal_broad_readiness=0"
Write-Output "renderer_broad_quality_ready=0"
Write-Output "renderer_commercial_readiness=0"
Write-Output "renderer_environment_ready=0"

if ($Mode -eq "Plan") {
    Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_writes_evidence=0"
    Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_downloads_artifacts=0"
    Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_ready=0"
    return
}

$outputRootFull = Resolve-RepoRelativePath -RelativePath $OutputRootRelative -Label "OutputRootRelative"
$downloadedArtifacts = 0
$downloadFailures = [System.Collections.Generic.List[string]]::new()

if ($Mode -eq "Import") {
    if ([string]::IsNullOrWhiteSpace($RunId)) {
        Write-Error "RunId is required in Import mode."
    }

    $gh = Find-CommandOnCombinedPath "gh"
    if (-not $gh) {
    Write-Error "GitHub CLI 'gh' is required in Import mode."
    }

    # Official references for `gh run download`: https://cli.github.com/manual/gh_run_download and
    # https://docs.github.com/en/rest/actions/artifacts
    foreach ($artifactName in @($ArtifactNames)) {
        $artifactDirectory = Join-Path $outputRootFull $artifactName
        $null = New-Item -ItemType Directory -Force -Path $artifactDirectory
        $output = @(& $gh "run" "download" $RunId "-R" $RepoFullName "-n" $artifactName "-D" $artifactDirectory 2>&1)
        if ($LASTEXITCODE -eq 0) {
            $downloadedArtifacts += 1
        } else {
            $downloadFailures.Add((ConvertTo-CounterValue -Value $artifactName)) | Out-Null
            $logPath = Join-Path $artifactDirectory "download-failed.log"
            Set-Content -LiteralPath $logPath -Encoding utf8NoBOM -Value ([string]::Join("`n", @($output)))
        }
    }
}

$presentInputs = 0
$inputRows = [ordered]@{}
$missingAssemblerInputNames = [System.Collections.Generic.List[string]]::new()
foreach ($inputSpec in $requiredAssemblerInputs) {
    $relativePath = Find-JsonEvidencePath `
        -SearchRootFull $outputRootFull `
        -SchemaVersion ([string]$inputSpec.Schema) `
        -ClaimId ([string]$inputSpec.Claim)
    $present = -not [string]::IsNullOrWhiteSpace($relativePath)
    if ($present) {
        $presentInputs += 1
    } else {
        $missingAssemblerInputNames.Add([string]$inputSpec.Name) | Out-Null
    }
    $inputRows[[string]$inputSpec.Name] = [ordered]@{
        present = $present
        path = $relativePath
    }
    Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_$($inputSpec.Name)=$(ConvertTo-CounterBit $present)"
    if ($present) {
        Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_$($inputSpec.Name)_path=$relativePath"
    }
}

$finalRetainedRootRelative = "$OutputRootRelative/renderer-commercial-readiness-final-retained-root"
$finalRetainedRootFull = Resolve-RepoRelativePath `
    -RelativePath $finalRetainedRootRelative `
    -Label "final retained root artifact"
$finalRootEvidencePath = Join-Path $finalRetainedRootFull "evidence.json"
$finalRootPresent = Test-Path -LiteralPath $finalRootEvidencePath -PathType Leaf
$metalHostGateSummary = Find-HostGateSummary -SearchRootFull $outputRootFull
$metalHostGateSummaryPresent = $null -ne $metalHostGateSummary
$intakeReady = $presentInputs -eq $requiredAssemblerInputs.Count -or $finalRootPresent
$assemblerOutputRootRelative = "$OutputRootRelative/assembled-final-retained-root"
$assemblerHandoffReady = $presentInputs -eq $requiredAssemblerInputs.Count
$assemblerInputPaths = [ordered]@{}
foreach ($inputSpec in $requiredAssemblerInputs) {
    $inputName = [string]$inputSpec.Name
    $assemblerInputPaths[$inputName] = [string]$inputRows[$inputName].path
}
$assemblerCommandArguments = @()
if ($assemblerHandoffReady) {
    $assemblerCommandArguments = @(
        "-Mode", "Assemble",
        "-OutputRootRelative", $assemblerOutputRootRelative,
        "-D3d12HostEvidenceRelative", [string]$assemblerInputPaths["d3d12_host_evidence"],
        "-VulkanStrictHostEvidenceRelative", [string]$assemblerInputPaths["vulkan_strict_host_evidence"],
        "-AppleMetalHostEvidenceRelative", [string]$assemblerInputPaths["apple_metal_host_evidence"],
        "-MetalMemoryProfilingHostEvidenceRelative", [string]$assemblerInputPaths["metal_memory_profiling_host_evidence"],
        "-PackageHostEvidenceRelative", [string]$assemblerInputPaths["package_host_evidence"],
        "-QualityVfxHostEvidenceRelative", [string]$assemblerInputPaths["quality_vfx_host_evidence"],
        "-CleanRoomLegalReviewRelative", [string]$assemblerInputPaths["clean_room_legal_review"]
    )
}
$assemblerRequireReadyArguments = @($assemblerCommandArguments)
if ($assemblerHandoffReady) {
    $assemblerRequireReadyArguments += "-RequireReady"
}
$finalPreflightArguments = @()
if ($finalRootPresent) {
    $finalPreflightArguments = @("-ArtifactRootRelative", $finalRetainedRootRelative)
}
$manifestRelative = "$OutputRootRelative/intake-manifest.json"
$manifestFull = Resolve-RepoRelativePath -RelativePath $manifestRelative -Label "intake manifest"
$willWriteManifest = -not $NoWrite.IsPresent

$manifest = [ordered]@{
    schema_version = "GameEngine.RendererCommercialReadinessFinalRetainedRootArtifactImport.v1"
    validation_recipe = "renderer-commercial-readiness-final-retained-root-artifact-import"
    repo_full_name = $RepoFullName
    run_id = $RunId
    output_root = $OutputRootRelative
    requested_artifacts = @($ArtifactNames)
    workflow_artifact_list = $workflowArtifactListSummary
    artifact_handoff_strategy = [ordered]@{
        final_retained_root_artifact = [ordered]@{
            name = $finalRetainedRootArtifactName
            available = $finalRootWorkflowArtifactAvailable
        }
        assembler_source_artifacts = [ordered]@{
            required_artifacts = @($assemblerSourceArtifactNames)
            available_artifacts = @($availableAssemblerSourceArtifacts)
            missing_artifacts = @($missingAssemblerSourceArtifacts)
        }
    }
    downloaded_artifacts = $downloadedArtifacts
    download_failures = @($downloadFailures)
    final_retained_root = [ordered]@{
        present = $finalRootPresent
        path = if ($finalRootPresent) { "$finalRetainedRootRelative/evidence.json" } else { "" }
    }
    assembler_inputs = $inputRows
    missing_assembler_inputs = @($missingAssemblerInputNames)
    assembler_handoff = [ordered]@{
        ready = $assemblerHandoffReady
        script = "tools/assemble-renderer-commercial-readiness-final-retained-root.ps1"
        output_root = $assemblerOutputRootRelative
        input_paths = $assemblerInputPaths
        command_arguments = @($assemblerCommandArguments)
        require_ready_command_arguments = @($assemblerRequireReadyArguments)
    }
    final_preflight_handoff = [ordered]@{
        ready = $finalRootPresent
        script = "tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1"
        artifact_root = if ($finalRootPresent) { $finalRetainedRootRelative } else { "" }
        command_arguments = @($finalPreflightArguments)
    }
    metal_host_gate_summary = [ordered]@{
        present = $metalHostGateSummaryPresent
        path = if ($metalHostGateSummaryPresent) { $metalHostGateSummary.Path } else { "" }
        status = if ($metalHostGateSummaryPresent) { $metalHostGateSummary.Status } else { "" }
        reason = if ($metalHostGateSummaryPresent) { $metalHostGateSummary.Reason } else { "" }
    }
    ready = $intakeReady
}

if ($willWriteManifest) {
    Write-Utf8NoBomJson -Path $manifestFull -Value $manifest
}

Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_downloads_artifacts=$(ConvertTo-CounterBit ($Mode -eq "Import"))"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_downloaded_workflow_artifacts=$downloadedArtifacts"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_download_failures=$($downloadFailures.Count)"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_present_assembler_inputs=$presentInputs"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_inputs=$($requiredAssemblerInputs.Count - $presentInputs)"
if (@($missingAssemblerInputNames).Count -gt 0) {
    Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_input_names=$((@($missingAssemblerInputNames) | ForEach-Object { ConvertTo-CounterValue -Value ([string]$_) }) -join ',')"
}
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_final_retained_root_present=$(ConvertTo-CounterBit $finalRootPresent)"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_handoff_ready=$(ConvertTo-CounterBit $assemblerHandoffReady)"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_handoff_required_input_paths=$($requiredAssemblerInputs.Count)"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_handoff_output_root=$assemblerOutputRootRelative"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_final_preflight_handoff_ready=$(ConvertTo-CounterBit $finalRootPresent)"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_metal_host_gate_summary_present=$(ConvertTo-CounterBit $metalHostGateSummaryPresent)"
if ($metalHostGateSummaryPresent) {
    Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_metal_host_gate_status=$(ConvertTo-CounterValue -Value $metalHostGateSummary.Status)"
    Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_metal_host_gate_reason=$(ConvertTo-CounterValue -Value $metalHostGateSummary.Reason)"
}
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_manifest=$manifestRelative"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_writes_evidence=$(ConvertTo-CounterBit $willWriteManifest)"
Write-Output "renderer_commercial_readiness_final_retained_root_artifact_import_ready=$(ConvertTo-CounterBit $intakeReady)"

# Missing workflow artifacts are represented in the manifest and counters above.
# Do not leak handled `gh run download` native exit codes into CI unless readiness is required.
$global:LASTEXITCODE = 0

if ($RequireReady.IsPresent -and -not $intakeReady) {
    Write-Error "Renderer commercial readiness GitHub artifact intake is not ready: missing assembler inputs or final retained root."
}
