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
$recipeId = "installed-2d-commercial-release-legal-gate-smoke"
$qualityGateId = "release-legal-gate-2d"
$readySmokeExpectations = @{
    "2d_commercial_release_legal_gate_status" = "ready"
    "2d_commercial_release_legal_gate_ready" = "1"
    "2d_commercial_release_evidence_gate_ready" = "1"
    "2d_commercial_release_official_source_ready" = "1"
    "2d_commercial_release_platform_gate_ready" = "1"
    "2d_commercial_release_engineering_review_input_ready" = "1"
    "2d_commercial_release_counsel_review_required" = "1"
    "2d_commercial_release_evidence_rows" = "9"
    "2d_commercial_release_official_source_rows" = "5"
    "2d_commercial_release_platform_gate_rows" = "3"
    "2d_commercial_release_host_gated_platform_rows" = "3"
    "2d_commercial_release_blocker_rows" = "0"
    "2d_commercial_release_missing_notice_rows" = "0"
    "2d_commercial_release_unknown_license_rows" = "0"
    "2d_commercial_release_unapproved_dependency_rows" = "0"
    "2d_commercial_release_external_engine_mark_rows" = "0"
    "2d_commercial_release_copied_asset_rows" = "0"
    "2d_commercial_release_unreviewed_generated_asset_rows" = "0"
    "2d_commercial_release_external_engine_claim_rows" = "0"
    "2d_commercial_release_legal_approval_claim_rows" = "0"
    "2d_commercial_release_diagnostics" = "0"
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
        Write-Error "2d-commercial-release-legal-gate: package smoke did not emit sample_2d_desktop_runtime_package status line"
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
        Write-Error "2d-commercial-release-legal-gate: package smoke missing counter $Name"
    }
    if ([string]$Fields[$Name] -ne $Expected) {
        Write-Error "2d-commercial-release-legal-gate: package smoke counter $Name expected $Expected but was $($Fields[$Name])"
    }
}

& (Join-Path $PSScriptRoot "check-2d-commercial-release-legal-review-input.ps1")

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
        "counsel-ready engineering review input",
        "Microsoft MSIX signing",
        "Apple notarization",
        "Android app signing",
        "missing notices",
        "unknown licenses",
        "unapproved dependencies",
        "external commercial engine marks",
        "legal-approval claims"
    )) {
    if (-not ([string]$qualityGate[0].evidence).Contains($needle)) {
        Write-Error "$gameManifestPath $qualityGateId evidence must mention $needle"
    }
}

$gameplayContract = Get-ObjectPropertyValue -Object $game -Name "gameplayContract"
if ($null -eq $gameplayContract) {
    Write-Error "$gameManifestPath must declare gameplayContract"
}
Assert-ObjectProperties $gameplayContract @("commercialReleaseLegalGate2D") "$gameManifestPath gameplayContract"
$releaseContract = [string](Get-ObjectPropertyValue -Object $gameplayContract -Name "commercialReleaseLegalGate2D")
foreach ($needle in @(
        "evaluate_runtime_2d_commercial_release_legal_gate",
        "--require-2d-commercial-release-legal-gate",
        "2d_commercial_release_legal_gate_ready=1",
        "2d_commercial_release_evidence_rows=9",
        "2d_commercial_release_official_source_rows=5",
        "2d_commercial_release_platform_gate_rows=3",
        "tools/generate-2d-commercial-release-legal-review-input.ps1",
        "Microsoft MSIX signing",
        "Apple notarization",
        "Android app signing",
        "not legal advice",
        "does not claim legal approval"
    )) {
    if (-not $releaseContract.Contains($needle)) {
        Write-Error "$gameManifestPath gameplayContract.commercialReleaseLegalGate2D missing expected text: $needle"
    }
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
        "--require-2d-commercial-release-legal-gate"
    )

    $packageOutput = & (Join-Path $PSScriptRoot "package-desktop-runtime.ps1") `
        -GameTarget "sample_2d_desktop_runtime_package" `
        -SmokeArgs $smokeArgs 2>&1
    $packageExitCode = $LASTEXITCODE
    if ($packageExitCode -ne 0) {
        Write-Error "2d-commercial-release-legal-gate: package smoke failed with exit code $packageExitCode`n$(@($packageOutput) -join "`n")"
    }

    $installedExe = Join-Path $root "out/install/desktop-runtime-release/bin/sample_2d_desktop_runtime_package.exe"
    if (-not (Test-Path -LiteralPath $installedExe -PathType Leaf)) {
        Write-Error "2d-commercial-release-legal-gate: installed sample executable missing after package smoke: $installedExe"
    }
    $smokeOutput = & $installedExe @smokeArgs 2>&1
    $smokeExitCode = $LASTEXITCODE
    if ($smokeExitCode -ne 0) {
        Write-Error "2d-commercial-release-legal-gate: installed package smoke failed with exit code $smokeExitCode`n$(@($smokeOutput) -join "`n")"
    }

    $fields = ConvertFrom-StatusLine -Output @($smokeOutput)
    foreach ($entry in $readySmokeExpectations.GetEnumerator()) {
        Assert-SmokeField $fields $entry.Key $entry.Value
    }
}

Write-Information "2d-commercial-release-legal-gate: ok" -InformationAction Continue
