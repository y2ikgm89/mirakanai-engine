#requires -Version 7.0
#requires -PSEdition Core
# Plan ID: renderer-commercial-readiness-evidence-promotion-v1

[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$ArtifactRootRelative = "artifacts/renderer/commercial-readiness-evidence/final-retained",
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

function ConvertFrom-KeyValueLines {
    param([string[]]$Lines = @())

    $values = @{}
    foreach ($line in @($Lines)) {
        $text = [string]$line
        $separator = $text.IndexOf("=")
        if ($separator -le 0) {
            continue
        }
        $values[$text.Substring(0, $separator)] = $text.Substring($separator + 1)
    }
    return $values
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

function Test-AllowedArtifactRoot {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $normalizedPath = $RelativePath.Replace("\", "/")
    return $normalizedPath -eq "artifacts/renderer/commercial-readiness-evidence" -or
        $normalizedPath.StartsWith(
            "artifacts/renderer/commercial-readiness-evidence/",
            [System.StringComparison]::Ordinal)
}

function Resolve-PreflightPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $root $RelativePath))
    $rootWithSeparator = $root.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "artifact_root_escape: $RelativePath"
    }
    return $fullPath
}

function Get-JsonPropertyValue {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
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

function Read-JsonOrNull {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    try {
        return Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    } catch {
        $Diagnostics.Add("invalid_${Name}_json")
        return $null
    }
}

function Add-Diagnostic {
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics,
        [Parameter(Mandatory = $true)][string]$Name
    )

    if (-not $Diagnostics.Contains($Name)) {
        $Diagnostics.Add($Name)
    }
}

function Test-ExpectedJsonString {
    param(
        [Parameter(Mandatory = $true)]$JsonObject,
        [Parameter(Mandatory = $true)][string]$PropertyName,
        [Parameter(Mandatory = $true)][string]$Expected,
        [Parameter(Mandatory = $true)][string]$DiagnosticName,
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Diagnostics
    )

    $value = Get-JsonPropertyValue -JsonObject $JsonObject -Name $PropertyName
    if ([string]$value -cne $Expected) {
        Add-Diagnostic -Diagnostics $Diagnostics -Name $DiagnosticName
        return $false
    }
    return $true
}

$diagnostics = [System.Collections.Generic.List[string]]::new()
$artifactRootFull = $null

if (-not (Test-SafeRepoRelativePath -RelativePath $ArtifactRootRelative)) {
    Write-Error "unsafe_artifact_root: ArtifactRootRelative must be a safe repo-relative path."
}
if (-not (Test-AllowedArtifactRoot -RelativePath $ArtifactRootRelative)) {
    Write-Error "unapproved_artifact_root: ArtifactRootRelative must be under artifacts/renderer/commercial-readiness-evidence."
}

$artifactRootFull = Resolve-PreflightPath -RelativePath $ArtifactRootRelative
$artifactRootExists = Test-Path -LiteralPath $artifactRootFull -PathType Container
if (-not $artifactRootExists) {
    Add-Diagnostic -Diagnostics $diagnostics -Name "artifact_root_missing"
}

$artifactSpecs = @(
    @{
        Name = "evidence_json"
        RelativePath = "evidence.json"
        Schema = "GameEngine.RendererCommercialReadinessEvidence.v1"
        ClaimId = "renderer-commercial-readiness-evidence-promotion-v1"
        Missing = "evidence_json_required"
        FixtureFlagRequired = $false
    },
    @{
        Name = "d3d12_quality"
        RelativePath = "d3d12-quality.json"
        Schema = "GameEngine.RendererCommercialQualityCloseout.v1"
        ArtifactId = "d3d12-quality"
        Recipe = "renderer-d3d12-quality-evidence"
        Missing = "d3d12_quality_artifact_required"
        FixtureFlagRequired = $true
    },
    @{
        Name = "vulkan_strict_quality"
        RelativePath = "vulkan-strict-quality.json"
        Schema = "GameEngine.RendererCommercialQualityCloseout.v1"
        ArtifactId = "vulkan-strict-quality"
        Recipe = "renderer-vulkan-strict-quality-evidence"
        Missing = "vulkan_strict_quality_artifact_required"
        FixtureFlagRequired = $true
    },
    @{
        Name = "apple_metal_host"
        RelativePath = "apple-metal-host.json"
        Schema = "GameEngine.RendererCommercialQualityCloseout.v1"
        ArtifactId = "apple-metal-host"
        Recipe = "renderer-metal-apple-host-evidence"
        Missing = "apple_metal_host_artifact_required"
        FixtureFlagRequired = $true
    },
    @{
        Name = "visible_3d_package"
        RelativePath = "visible-3d-package.json"
        Schema = "GameEngine.DesktopRuntimePackageEvidence.v1"
        ArtifactId = "visible-3d-package"
        Recipe = "desktop-3d-package"
        Missing = "visible_3d_package_artifact_required"
        FixtureFlagRequired = $true
    },
    @{
        Name = "runtime_ui_package"
        RelativePath = "runtime-ui-package.json"
        Schema = "GameEngine.DesktopRuntimePackageEvidence.v1"
        ArtifactId = "runtime-ui-package"
        Recipe = "desktop-runtime-ui-package"
        Missing = "runtime_ui_package_artifact_required"
        FixtureFlagRequired = $true
    },
    @{
        Name = "environment_package"
        RelativePath = "environment-package.json"
        Schema = "GameEngine.EnvironmentPackageEvidence.v1"
        ArtifactId = "environment-package"
        Recipe = "environment-package"
        Missing = "environment_package_artifact_required"
        FixtureFlagRequired = $true
    },
    @{
        Name = "generated_game_package"
        RelativePath = "generated-game-package.json"
        Schema = "GameEngine.GeneratedGamePackageEvidence.v1"
        ArtifactId = "generated-game-package"
        Recipe = "generated-game-package"
        Missing = "generated_game_package_artifact_required"
        FixtureFlagRequired = $true
    },
    @{
        Name = "renderer_quality_matrix"
        RelativePath = "renderer-quality-matrix.json"
        Schema = "GameEngine.RendererCommercialQualityCloseout.v1"
        ArtifactId = "renderer-quality-matrix"
        Recipe = "renderer-quality-matrix"
        Missing = "renderer_quality_matrix_artifact_required"
        FixtureFlagRequired = $true
    },
    @{
        Name = "production_vfx_profiling"
        RelativePath = "production-vfx-profiling.json"
        Schema = "GameEngine.RendererCommercialQualityCloseout.v1"
        ArtifactId = "production-vfx-profiling"
        Recipe = "renderer-production-vfx-profiling"
        Missing = "production_vfx_profiling_artifact_required"
        FixtureFlagRequired = $true
    },
    @{
        Name = "metal_memory_profiling_host_evidence"
        RelativePath = "metal-memory-profiling-host-evidence/evidence.json"
        Schema = "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1"
        ClaimId = "renderer-metal-memory-profiling-host-evidence-v1"
        Missing = "metal_memory_profiling_host_evidence_required"
        FixtureFlagRequired = $false
    },
    @{
        Name = "clean_room_legal"
        RelativePath = "clean-room-legal.json"
        Schema = "GameEngine.RendererCleanRoomLegalArtifact.v1"
        ArtifactId = "clean-room-legal"
        Recipe = "renderer-clean-room-legal-artifact"
        Missing = "clean_room_legal_artifact_required"
        FixtureFlagRequired = $true
    }
)

$presentFiles = 0
$fixtureArtifactsRejected = 0
$schemaMismatches = 0
$hashCandidateFiles = 0

foreach ($artifactSpec in $artifactSpecs) {
    $artifactPath = Join-Path $artifactRootFull ([string]$artifactSpec.RelativePath)
    if (-not (Test-Path -LiteralPath $artifactPath -PathType Leaf)) {
        Add-Diagnostic -Diagnostics $diagnostics -Name ([string]$artifactSpec.Missing)
        continue
    }

    $presentFiles += 1
    $artifactJson = Read-JsonOrNull `
        -Path $artifactPath `
        -Name ([string]$artifactSpec.Name) `
        -Diagnostics $diagnostics
    if ($null -eq $artifactJson) {
        continue
    }

    $schemaOk = Test-ExpectedJsonString `
        -JsonObject $artifactJson `
        -PropertyName "schema_version" `
        -Expected ([string]$artifactSpec.Schema) `
        -DiagnosticName "invalid_$($artifactSpec.Name)_schema" `
        -Diagnostics $diagnostics
    $idOk = $true
    if ($artifactSpec.ContainsKey("ArtifactId")) {
        $idOk = Test-ExpectedJsonString `
            -JsonObject $artifactJson `
            -PropertyName "artifact_id" `
            -Expected ([string]$artifactSpec.ArtifactId) `
            -DiagnosticName "invalid_$($artifactSpec.Name)_artifact_id" `
            -Diagnostics $diagnostics
    }
    if ($artifactSpec.ContainsKey("ClaimId")) {
        $idOk = Test-ExpectedJsonString `
            -JsonObject $artifactJson `
            -PropertyName "claim_id" `
            -Expected ([string]$artifactSpec.ClaimId) `
            -DiagnosticName "invalid_$($artifactSpec.Name)_claim_id" `
            -Diagnostics $diagnostics
    }
    $recipeOk = $true
    if ($artifactSpec.ContainsKey("Recipe")) {
        $recipeOk = Test-ExpectedJsonString `
            -JsonObject $artifactJson `
            -PropertyName "validation_recipe" `
            -Expected ([string]$artifactSpec.Recipe) `
            -DiagnosticName "invalid_$($artifactSpec.Name)_recipe" `
            -Diagnostics $diagnostics
    }

    if (-not ($schemaOk -and $idOk -and $recipeOk)) {
        $schemaMismatches += 1
    }

    if ([bool]$artifactSpec.FixtureFlagRequired) {
        $fixtureOnly = Get-JsonPropertyValue -JsonObject $artifactJson -Name "fixture_only"
        if ($fixtureOnly -isnot [bool]) {
            Add-Diagnostic -Diagnostics $diagnostics -Name "invalid_$($artifactSpec.Name)_fixture_flag"
            $fixtureArtifactsRejected += 1
        } elseif ([bool]$fixtureOnly) {
            Add-Diagnostic -Diagnostics $diagnostics -Name "fixture_artifact_rejected_$($artifactSpec.Name)"
            $fixtureArtifactsRejected += 1
        }
    }

    $hashCandidateFiles += 1
}

$requiredFiles = $artifactSpecs.Count
$missingFiles = $requiredFiles - $presentFiles
$validatorValues = @{}
$validatorExecuted = $artifactRootExists -and (Test-Path -LiteralPath (Join-Path $artifactRootFull "evidence.json") -PathType Leaf)
$validatorExitCode = 0

if ($validatorExecuted) {
    $validatorScript = Join-Path $PSScriptRoot "validate-renderer-commercial-readiness-evidence.ps1"
    $validatorOutput = @(& pwsh -NoProfile -ExecutionPolicy Bypass -File $validatorScript `
            -ArtifactRootRelative $ArtifactRootRelative 2>&1)
    $validatorExitCode = $LASTEXITCODE
    $validatorValues = ConvertFrom-KeyValueLines -Lines @($validatorOutput | ForEach-Object { [string]$_ })
    if ($validatorExitCode -ne 0) {
        Add-Diagnostic -Diagnostics $diagnostics -Name "readiness_validator_failed"
    }
} else {
    Add-Diagnostic -Diagnostics $diagnostics -Name "readiness_validator_not_run"
}

$validatorCommercialReady = [string]$validatorValues["renderer_commercial_readiness"] -eq "1"
$validatorBackendParityReady = [string]$validatorValues["renderer_backend_parity_ready"] -eq "1"
$validatorMetalBroadReady = [string]$validatorValues["renderer_metal_broad_readiness"] -eq "1"
$validatorBroadQualityReady = [string]$validatorValues["renderer_broad_quality_ready"] -eq "1"
$preflightReady = $missingFiles -eq 0 -and
    $fixtureArtifactsRejected -eq 0 -and
    $schemaMismatches -eq 0 -and
    $validatorExecuted -and
    $validatorExitCode -eq 0 -and
    $validatorCommercialReady

if ($RequireReady.IsPresent -and -not $preflightReady) {
    Add-Diagnostic -Diagnostics $diagnostics -Name "require_ready_without_complete_retained_artifacts"
}

$status = if ($preflightReady) {
    "ready"
} else {
    "blocked"
}

Write-Output "validation_recipe=renderer-commercial-readiness-final-promotion-preflight"
Write-Output "artifact_root=$ArtifactRootRelative"
Write-Output "renderer_commercial_readiness_final_preflight_status=$status"
Write-Output "renderer_commercial_readiness_final_preflight_ready=$(ConvertTo-CounterBit $preflightReady)"
Write-Output "renderer_commercial_readiness_final_preflight_require_ready=$(ConvertTo-CounterBit $RequireReady.IsPresent)"
Write-Output "renderer_commercial_readiness_final_preflight_required_files=$requiredFiles"
Write-Output "renderer_commercial_readiness_final_preflight_present_files=$presentFiles"
Write-Output "renderer_commercial_readiness_final_preflight_missing_files=$missingFiles"
Write-Output "renderer_commercial_readiness_final_preflight_fixture_artifacts_rejected=$fixtureArtifactsRejected"
Write-Output "renderer_commercial_readiness_final_preflight_schema_mismatches=$schemaMismatches"
Write-Output "renderer_commercial_readiness_final_preflight_hash_candidate_files=$hashCandidateFiles"
Write-Output "renderer_commercial_readiness_final_preflight_validator_executed=$(ConvertTo-CounterBit $validatorExecuted)"
Write-Output "renderer_commercial_readiness_final_preflight_validator_exit_code=$validatorExitCode"
Write-Output "renderer_backend_parity_ready=$(ConvertTo-CounterBit $validatorBackendParityReady)"
Write-Output "renderer_metal_broad_readiness=$(ConvertTo-CounterBit $validatorMetalBroadReady)"
Write-Output "renderer_broad_quality_ready=$(ConvertTo-CounterBit $validatorBroadQualityReady)"
Write-Output "renderer_commercial_readiness=$(ConvertTo-CounterBit $validatorCommercialReady)"
Write-Output "renderer_environment_ready=0"
Write-Output "renderer_commercial_readiness_final_preflight_blocker=$($diagnostics -join '+')"

if ($RequireReady.IsPresent -and -not $preflightReady) {
    Write-Error "Renderer commercial readiness final promotion preflight is not ready: $($diagnostics -join ', ')"
}
