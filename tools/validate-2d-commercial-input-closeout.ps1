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
$recipeId = "installed-2d-commercial-input-closeout-smoke"
$readySmokeExpectations = @{
    "2d_commercial_input_closeout_status" = "ready"
    "2d_commercial_input_closeout_ready" = "1"
    "2d_commercial_input_action_map_ready" = "1"
    "2d_commercial_input_rebinding_ready" = "1"
    "2d_commercial_input_device_ux_ready" = "1"
    "2d_commercial_input_accessibility_navigation_ready" = "1"
    "2d_commercial_input_official_source_ready" = "1"
    "2d_commercial_input_action_binding_rows" = "3"
    "2d_commercial_input_axis_binding_rows" = "1"
    "2d_commercial_input_profile_overlay_rows" = "2"
    "2d_commercial_input_presentation_rows" = "4"
    "2d_commercial_input_glyph_lookup_keys" = "9"
    "2d_commercial_input_keyboard_mouse_device_rows" = "1"
    "2d_commercial_input_gamepad_device_rows" = "1"
    "2d_commercial_input_touch_gesture_rows" = "2"
    "2d_commercial_input_multiplayer_device_assignment_rows" = "2"
    "2d_commercial_input_per_device_profile_rows" = "1"
    "2d_commercial_input_glyph_asset_lookup_rows" = "3"
    "2d_commercial_input_keyboard_layout_label_rows" = "2"
    "2d_commercial_input_accessibility_navigation_rows" = "2"
    "2d_commercial_input_keyboard_accessible_rows" = "2"
    "2d_commercial_input_controller_accessible_rows" = "2"
    "2d_commercial_input_official_source_rows" = "5"
    "2d_commercial_input_diagnostics" = "0"
    "2d_commercial_input_native_handle_access_rows" = "0"
    "2d_commercial_input_input_middleware_rows" = "0"
    "2d_commercial_input_ui_rendering_rows" = "0"
    "2d_commercial_input_glyph_rendering_rows" = "0"
    "2d_commercial_input_ui_widget_rows" = "0"
    "2d_commercial_input_external_engine_claim_rows" = "0"
    "2d_commercial_input_cross_platform_parity_claim_rows" = "0"
    "2d_commercial_input_legal_approval_claim_rows" = "0"
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
        Write-Error "2d-commercial-input-closeout: package smoke did not emit sample_2d_desktop_runtime_package status line"
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
        Write-Error "2d-commercial-input-closeout: package smoke missing counter $Name"
    }
    if ([string]$Fields[$Name] -ne $Expected) {
        Write-Error "2d-commercial-input-closeout: package smoke counter $Name expected $Expected but was $($Fields[$Name])"
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

$qualityGate = @($gameDesignSpec.qualityGates | Where-Object { [string]$_.id -eq "input-commercial-closeout-2d" })
if ($qualityGate.Count -ne 1) {
    Write-Error "$gameManifestPath gameDesignSpec qualityGates must declare exactly one input-commercial-closeout-2d row"
}
Assert-ObjectProperties $qualityGate[0] @("id", "evidence", "recipeIds") "$gameManifestPath input-commercial-closeout-2d quality gate"
Assert-ContainsAll (New-Set @($qualityGate[0].recipeIds)) @($recipeId) "$gameManifestPath input-commercial-closeout-2d recipeIds"
foreach ($needle in @(
        "zero native-handle",
        "zero input middleware",
        "zero external-engine",
        "zero legal-approval"
    )) {
    if (-not ([string]$qualityGate[0].evidence).Contains($needle)) {
        Write-Error "$gameManifestPath input-commercial-closeout-2d evidence must mention $needle"
    }
}

$gameplayContract = Get-ObjectPropertyValue -Object $game -Name "gameplayContract"
if ($null -eq $gameplayContract) {
    Write-Error "$gameManifestPath must declare gameplayContract"
}
Assert-ObjectProperties $gameplayContract @("inputDeviceProductionUx2D", "inputCommercialCloseout2D") "$gameManifestPath gameplayContract"
$commercialContract = [string](Get-ObjectPropertyValue -Object $gameplayContract -Name "inputCommercialCloseout2D")
foreach ($needle in @(
        "evaluate_runtime_2d_commercial_input_closeout",
        "--require-2d-commercial-input-closeout",
        "2d_commercial_input_closeout_ready=1",
        "2d_commercial_input_official_source_rows=5",
        "2d_commercial_input_external_engine_claim_rows=0",
        "2d_commercial_input_legal_approval_claim_rows=0",
        "external engine compatibility",
        "legal approval"
    )) {
    if (-not $commercialContract.Contains($needle)) {
        Write-Error "$gameManifestPath gameplayContract.inputCommercialCloseout2D missing expected text: $needle"
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
        "--require-2d-commercial-input-closeout"
    )

    $packageOutput = & (Join-Path $PSScriptRoot "package-desktop-runtime.ps1") `
        -GameTarget "sample_2d_desktop_runtime_package" `
        -SmokeArgs $smokeArgs 2>&1
    $packageExitCode = $LASTEXITCODE
    if ($packageExitCode -ne 0) {
        Write-Error "2d-commercial-input-closeout: package smoke failed with exit code $packageExitCode`n$(@($packageOutput) -join "`n")"
    }

    $installedExe = Join-Path $root "out/install/desktop-runtime-release/bin/sample_2d_desktop_runtime_package.exe"
    if (-not (Test-Path -LiteralPath $installedExe -PathType Leaf)) {
        Write-Error "2d-commercial-input-closeout: installed sample executable missing after package smoke: $installedExe"
    }
    $smokeOutput = & $installedExe @smokeArgs 2>&1
    $smokeExitCode = $LASTEXITCODE
    if ($smokeExitCode -ne 0) {
        Write-Error "2d-commercial-input-closeout: installed package smoke failed with exit code $smokeExitCode`n$(@($smokeOutput) -join "`n")"
    }

    $fields = ConvertFrom-StatusLine -Output @($smokeOutput)
    foreach ($entry in $readySmokeExpectations.GetEnumerator()) {
        Assert-SmokeField $fields $entry.Key $entry.Value
    }
}

Write-Information "2d-commercial-input-closeout: ok" -InformationAction Continue
