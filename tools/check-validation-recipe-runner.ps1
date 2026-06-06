#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$runner = Join-Path $PSScriptRoot "run-validation-recipe.ps1"
$recipeCore = Join-Path $PSScriptRoot "validation-recipe-core.ps1"

if (-not (Test-Path -LiteralPath $runner -PathType Leaf)) {
    Write-Error "Missing validation recipe runner: tools/run-validation-recipe.ps1"
}
if (-not (Test-Path -LiteralPath $recipeCore -PathType Leaf)) {
    Write-Error "Missing validation recipe core helper: tools/validation-recipe-core.ps1"
}

. $recipeCore
. (Join-Path $PSScriptRoot "run-validation-recipe-plans.ps1")

function ConvertTo-RunnerResultObject {
    param(
        [Parameter(Mandatory = $true)]
        $Result
    )

    return $Result | ConvertTo-Json -Depth 12 | ConvertFrom-Json
}

function Invoke-RunnerJson {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,

        [int]$ExpectedExitCode = 0
    )

    $mode = "DryRun"
    $recipe = ""
    $gameTarget = ""
    $strictBackend = ""
    $hostGateAcknowledgements = @()
    $timeoutSeconds = 0
    $remainingArguments = @()

    for ($index = 0; $index -lt $Arguments.Count; $index++) {
        $argument = $Arguments[$index]
        switch ($argument) {
            "-Mode" {
                $index++
                $mode = $Arguments[$index]
            }
            "-Recipe" {
                $index++
                $recipe = $Arguments[$index]
            }
            "-GameTarget" {
                $index++
                $gameTarget = $Arguments[$index]
            }
            "-StrictBackend" {
                $index++
                $strictBackend = $Arguments[$index]
            }
            "-HostGateAcknowledgements" {
                $index++
                $hostGateAcknowledgements += $Arguments[$index]
            }
            "-TimeoutSeconds" {
                $index++
                $timeoutSeconds = [int]$Arguments[$index]
            }
            "-RemainingArguments" {
                $index++
                $remainingArguments += $Arguments[$index]
            }
            default {
                $remainingArguments += $argument
            }
        }
    }

    $exitCode = 0
    if ([string]::IsNullOrWhiteSpace($recipe)) {
        $result = New-ValidationRecipeRejectedResult -Mode $mode -RecipeName "" -Diagnostic (New-RunnerDiagnostic -Severity "error" -Code "missing-recipe" -Message "Recipe is required.")
        $exitCode = 2
    } else {
        $plan = Get-ValidationRecipeCommandPlan -RecipeName $recipe -SelectedGameTarget $gameTarget -SelectedStrictBackend $strictBackend
        if ($null -eq $plan) {
            $result = New-ValidationRecipeRejectedResult -Mode $mode -RecipeName $recipe -Diagnostic (New-RunnerDiagnostic -Severity "error" -Code "unknown-recipe" -Message "Validation recipe '$recipe' is not in the reviewed run-validation-recipe allowlist." -ValidationRecipe $recipe)
            $exitCode = 2
        } else {
            $diagnostic = Test-ValidationRecipeRequest `
                -Plan $plan `
                -Mode $mode `
                -GameTarget $gameTarget `
                -StrictBackend $strictBackend `
                -HostGateAcknowledgements $hostGateAcknowledgements `
                -RemainingArguments $remainingArguments `
                -TimeoutSeconds $timeoutSeconds
            if ($null -ne $diagnostic) {
                $result = New-ValidationRecipeRejectedResult -Mode $mode -RecipeName $recipe -Diagnostic $diagnostic
                $result.hostGates = @($plan.hostGates)
                $result.validationRecipes = @($recipe)
                $exitCode = 2
            } else {
                $result = New-ValidationRecipeDryRunResult -Mode $mode -Plan $plan
            }
        }
    }

    if ($exitCode -ne $ExpectedExitCode) {
        Write-Error "run-validation-recipe.ps1 $($Arguments -join ' ') exited $exitCode, expected $ExpectedExitCode."
    }

    return ConvertTo-RunnerResultObject -Result $result
}

function Assert-RunnerCliSmoke {
    $output = @(& pwsh -NoProfile -ExecutionPolicy Bypass -File $runner -Mode DryRun -Recipe "agent-contract" 2>&1)
    $exitCode = if ($null -eq $global:LASTEXITCODE) { 0 } else { $global:LASTEXITCODE }
    if ($exitCode -ne 0) {
        Write-Error "run-validation-recipe.ps1 CLI smoke exited $exitCode. Output: $($output -join "`n")"
    }

    try {
        $result = (($output | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) }) -join "`n") | ConvertFrom-Json
    } catch {
        Write-Error "run-validation-recipe.ps1 CLI smoke did not return JSON. Output: $($output -join "`n")"
    }

    if ($result.status -ne "dry-run" -or $result.recipe -ne "agent-contract") {
        Write-Error "run-validation-recipe.ps1 CLI smoke returned unexpected result."
    }
    Assert-ArgvHasScriptSuffix -Result $result -ScriptSuffix "tools/check-ai-integration.ps1" -Label "CLI smoke argv for agent-contract"
}

function Assert-HasProperty($object, [string]$Property, [string]$Label) {
    if (-not $object.PSObject.Properties.Name.Contains($Property)) {
        Write-Error "$Label missing property: $Property"
    }
}

function Assert-ArrayContains($array, [string]$Expected, [string]$Label) {
    if (@($array) -notcontains $Expected) {
        Write-Error "$Label missing expected value: $Expected"
    }
}

function Assert-ArgvHasScriptSuffix {
    param(
        [Parameter(Mandatory = $true)][object]$Result,
        [Parameter(Mandatory = $true)][string]$ScriptSuffix,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $normalizedSuffix = $ScriptSuffix.Replace('\', '/').TrimEnd('/')
    $hit = $false
    foreach ($a in @($Result.argv)) {
        $s = ([string]$a).Replace('\', '/')
        if ($s.EndsWith($normalizedSuffix, [System.StringComparison]::OrdinalIgnoreCase)) {
            $hit = $true
            break
        }
        if ($s -eq $ScriptSuffix -or $s -eq $normalizedSuffix) {
            $hit = $true
            break
        }
    }
    if (-not $hit) {
        Write-Error "$Label missing argv entry ending with: $ScriptSuffix"
    }
}

function Assert-ArgvContainsText {
    param(
        [Parameter(Mandatory = $true)][object]$Result,
        [Parameter(Mandatory = $true)][string]$Expected,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $joined = [string]::Join("`n", @($Result.argv | ForEach-Object { [string]$_ }))
    if (-not $joined.Contains($Expected)) {
        Write-Error "$Label missing expected text: $Expected"
    }
}

function Assert-DryRunRecipe {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Recipe,

        [string[]]$ExpectedArgv = @(),

        [string[]]$HostGateAcknowledgements = @()
    )

    $runnerArguments = @("-Mode", "DryRun", "-Recipe", $Recipe)
    foreach ($acknowledgement in $HostGateAcknowledgements) {
        $runnerArguments += @("-HostGateAcknowledgements", $acknowledgement)
    }

    $result = Invoke-RunnerJson -Arguments $runnerArguments
    foreach ($property in @("recipe", "status", "command", "argv", "hostGates", "diagnostics", "blockedBy")) {
        Assert-HasProperty $result $property "dry-run result for $Recipe"
    }
    if ($result.recipe -ne $Recipe) {
        Write-Error "dry-run result used recipe '$($result.recipe)', expected '$Recipe'"
    }
    if ($result.status -ne "dry-run") {
        Write-Error "dry-run result for $Recipe must use status dry-run"
    }
    foreach ($expected in $ExpectedArgv) {
        if ($expected -like "*.ps1") {
            Assert-ArgvHasScriptSuffix -Result $result -ScriptSuffix $expected -Label "dry-run argv for $Recipe"
        } else {
            Assert-ArrayContains $result.argv $expected "dry-run argv for $Recipe"
        }
    }

    return $result
}

Write-Host "validation-recipe-runner-check: dry-run recipe contracts..."
Assert-RunnerCliSmoke
Assert-DryRunRecipe -Recipe "agent-contract" -ExpectedArgv @("-File", "check-ai-integration.ps1") | Out-Null
Assert-DryRunRecipe -Recipe "default" -ExpectedArgv @("-File", "validate.ps1") | Out-Null
Assert-DryRunRecipe -Recipe "shader-toolchain" -ExpectedArgv @("-File", "check-shader-toolchain.ps1") | Out-Null
Assert-DryRunRecipe -Recipe "renderer-metal-apple-host-evidence" -ExpectedArgv @("-File", "validate-renderer-metal-apple.ps1") | Out-Null
Assert-DryRunRecipe -Recipe "network-enet" -ExpectedArgv @("-File", "validate-network-enet.ps1") | Out-Null
Assert-DryRunRecipe -Recipe "desktop-editor" -ExpectedArgv @("-File", "build-editor.ps1") | Out-Null
Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-scene-gpu-package" -ExpectedArgv @("-File", "tools/package-desktop-runtime.ps1", "-GameTarget", "sample_desktop_runtime_game") | Out-Null
$sampleEnvironmentFogDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-fog-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-fog-evidence", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentFogDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-fog-package"
}
$sampleVulkanEnvironmentFogDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-environment-fog-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-environment-fog-vulkan-package-evidence", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleVulkanEnvironmentFogDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-environment-fog-package"
}
$sampleVulkanPhysicalSkyDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-physical-sky-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-physical-sky-vulkan-package-evidence", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleVulkanPhysicalSkyDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-physical-sky-package"
}
$sampleEnvironmentVolumetricFogDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-volumetric-fog-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-volumetric-fog-package-evidence", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentVolumetricFogDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-volumetric-fog-package"
}
$sampleVulkanEnvironmentVolumetricFogRendererExecutionDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-volumetric-fog-renderer-execution" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-environment-volumetric-fog-vulkan-renderer-execution", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleVulkanEnvironmentVolumetricFogRendererExecutionDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-volumetric-fog-renderer-execution"
}
$sampleVulkanEnvironmentVolumetricCloudRendererExecutionDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-volumetric-cloud-renderer-execution" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-environment-volumetric-cloud-vulkan-renderer-execution", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleVulkanEnvironmentVolumetricCloudRendererExecutionDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-volumetric-cloud-renderer-execution"
}
$sampleVulkanEnvironmentIblRendererExecutionDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-environment-ibl-renderer-execution" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-environment-lighting-vulkan-renderer-execution", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleVulkanEnvironmentIblRendererExecutionDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-environment-ibl-renderer-execution"
}
$sampleCloudLayerDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-cloud-layer-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-cloud-layer-package-evidence", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleCloudLayerDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-cloud-layer-package"
}
$sampleCloudLayerRendererExecutionDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-cloud-layer-renderer-execution" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-cloud-layer-renderer-execution", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleCloudLayerRendererExecutionDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-cloud-layer-renderer-execution"
}
$sampleEnvironmentPrecipitationDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-precipitation-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-precipitation-package-evidence", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentPrecipitationDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-precipitation-package"
}
$sampleEnvironmentPrecipitationRendererExecutionDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-precipitation-renderer-execution" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-precipitation-renderer-execution", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentPrecipitationRendererExecutionDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-precipitation-renderer-execution"
}
$sampleVulkanEnvironmentPrecipitationRendererExecutionDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-environment-precipitation-renderer-execution" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-environment-precipitation-vulkan-renderer-execution", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleVulkanEnvironmentPrecipitationRendererExecutionDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-environment-precipitation-renderer-execution"
}
$sampleEnvironmentSnowDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-snow-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-snow-package-evidence", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentSnowDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-snow-package"
}
$sampleEnvironmentSnowRendererExecutionDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-snow-renderer-execution" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-snow-renderer-execution", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentSnowRendererExecutionDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-snow-renderer-execution"
}
$sampleEnvironmentMaterialWeatheringDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-material-weathering" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-material-weathering", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentMaterialWeatheringDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-material-weathering"
}
$sampleEnvironmentAudioPlaybackDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-audio-playback" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-audio-playback", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentAudioPlaybackDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-audio-playback"
}
$sampleEnvironmentProfileDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-profile-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-SmokeArgs @(", "--require-environment-profile", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentProfileDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-profile-package"
}
Assert-DryRunRecipe -Recipe "desktop-runtime-generated-material-shader-scaffold-package" -ExpectedArgv @("-File", "tools/package-desktop-runtime.ps1", "-GameTarget", "sample_generated_desktop_runtime_material_shader_package") | Out-Null
$materialVulkanDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-vulkan-scene-shaders", "--require-material-graph-authoring")) {
    Assert-ArgvContainsText -Result $materialVulkanDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict"
}
$sampleVulkanDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-vulkan-renderer", "--require-native-ui-textured-sprite-atlas")) {
    Assert-ArgvContainsText -Result $sampleVulkanDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package"
}
Assert-DryRunRecipe -Recipe "dev-windows-editor-game-module-driver-load-tests" -ExpectedArgv @("-File", "tools/run-editor-game-module-driver-load-tests.ps1") | Out-Null

$unknown = Invoke-RunnerJson -Arguments @("-Mode", "DryRun", "-Recipe", "not-a-recipe") -ExpectedExitCode 2
if ($unknown.status -ne "rejected" -or @($unknown.diagnostics | Where-Object { $_.code -eq "unknown-recipe" }).Count -ne 1) {
    Write-Error "unknown recipe id must be rejected with diagnostic code unknown-recipe"
}

$unsafeTarget = Invoke-RunnerJson -Arguments @("-Mode", "DryRun", "-Recipe", "desktop-runtime-sample-game-scene-gpu-package", "-GameTarget", "..\escape", "-HostGateAcknowledgements", "d3d12-windows-primary") -ExpectedExitCode 2
if ($unsafeTarget.status -ne "rejected" -or @($unsafeTarget.diagnostics | Where-Object { $_.code -eq "unsafe-game-target" }).Count -ne 1) {
    Write-Error "unsafe gameTarget must be rejected with diagnostic code unsafe-game-target"
}

$unsupportedMaterialTarget = Invoke-RunnerJson -Arguments @("-Mode", "DryRun", "-Recipe", "desktop-runtime-generated-material-shader-scaffold-package", "-GameTarget", "sample_desktop_runtime_game", "-HostGateAcknowledgements", "d3d12-windows-primary") -ExpectedExitCode 2
if ($unsupportedMaterialTarget.status -ne "rejected" -or @($unsupportedMaterialTarget.diagnostics | Where-Object { $_.code -eq "unsupported-game-target" }).Count -ne 1) {
    Write-Error "material/shader package recipe must reject non-allowlisted game targets"
}

$unsupportedMaterialVulkanTarget = Invoke-RunnerJson -Arguments @("-Mode", "DryRun", "-Recipe", "desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict", "-GameTarget", "sample_desktop_runtime_game", "-HostGateAcknowledgements", "vulkan-strict") -ExpectedExitCode 2
if ($unsupportedMaterialVulkanTarget.status -ne "rejected" -or @($unsupportedMaterialVulkanTarget.diagnostics | Where-Object { $_.code -eq "unsupported-game-target" }).Count -ne 1) {
    Write-Error "strict Vulkan material/shader package recipe must reject non-allowlisted game targets"
}

$unsupportedArgs = Invoke-RunnerJson -Arguments @("-Mode", "DryRun", "-Recipe", "agent-contract", "-RemainingArguments", "unexpected-free-form-arg") -ExpectedExitCode 2
if ($unsupportedArgs.status -ne "rejected" -or @($unsupportedArgs.diagnostics | Where-Object { $_.code -eq "unsupported-arguments" }).Count -ne 1) {
    Write-Error "unsupported free-form arguments must be rejected with diagnostic code unsupported-arguments"
}

$missingGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package") -ExpectedExitCode 2
if ($missingGate.status -ne "rejected" -or @($missingGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "missing host-gate acknowledgement must be rejected with diagnostic code missing-host-gate-acknowledgement"
}

$missingVulkanPrecipitationGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-vulkan-environment-precipitation-renderer-execution") -ExpectedExitCode 2
if ($missingVulkanPrecipitationGate.status -ne "rejected" -or @($missingVulkanPrecipitationGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "strict Vulkan precipitation renderer execution recipe must require vulkan-strict acknowledgement before execute"
}

$missingVulkanVolumetricFogGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-vulkan-volumetric-fog-renderer-execution") -ExpectedExitCode 2
if ($missingVulkanVolumetricFogGate.status -ne "rejected" -or @($missingVulkanVolumetricFogGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "strict Vulkan volumetric fog renderer execution recipe must require vulkan-strict acknowledgement before execute"
}

$missingVulkanIblGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-vulkan-environment-ibl-renderer-execution") -ExpectedExitCode 2
if ($missingVulkanIblGate.status -ne "rejected" -or @($missingVulkanIblGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "strict Vulkan IBL renderer execution recipe must require vulkan-strict acknowledgement before execute"
}

$missingMaterialVulkanGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict") -ExpectedExitCode 2
if ($missingMaterialVulkanGate.status -ne "rejected" -or @($missingMaterialVulkanGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "strict Vulkan material/shader package recipe must require vulkan-strict acknowledgement before execute"
}

$missingEditorDriverGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "dev-windows-editor-game-module-driver-load-tests") -ExpectedExitCode 2
if ($missingEditorDriverGate.status -ne "rejected" -or @($missingEditorDriverGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "dev-windows-editor-game-module-driver-load-tests recipe must require windows-msvc-dev-editor-game-module-driver-ctest acknowledgement before execute"
}

$missingMetalAppleGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "renderer-metal-apple-host-evidence") -ExpectedExitCode 2
if ($missingMetalAppleGate.status -ne "rejected" -or @($missingMetalAppleGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "renderer-metal-apple-host-evidence recipe must require metal-apple acknowledgement before execute"
}

# Full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` / `tools/check-ai-integration.ps1` runs once from `tools/validate.ps1` after this script.
# Keeping a second Execute(agent-contract) here would duplicate that multi-minute integration pass.

Write-Host "validation-recipe-runner-check: ok"
