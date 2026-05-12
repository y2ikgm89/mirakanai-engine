#requires -Version 7.0
#requires -PSEdition Core
# Dot-sourced only from tools/run-validation-recipe.ps1 (do not run this file directly).

function Get-ValidationRecipeCommandPlan {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true, Position = 0)]
        [string]$RecipeName,
        [Parameter(Position = 1)]
        [string]$SelectedGameTarget = '',
        [Parameter(Position = 2)]
        [string]$SelectedStrictBackend = ''
    )

    $packageScript = 'tools/package-desktop-runtime.ps1'

    if ($RecipeName -eq 'agent-contract') {
        # validate.ps1 already runs check-json-contracts.ps1 before this recipe; keep
        # agent-contract execution focused on check-ai-integration.ps1 to avoid redundant multi-minute schema work.
        $cmdPlanEntry = Get-RepositoryToolCommandPlan -ToolScriptName 'check-ai-integration.ps1'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($cmdPlanEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @()
    }
    elseif ($RecipeName -eq 'default') {
        $cmdPlanEntry = Get-RepositoryToolCommandPlan -ToolScriptName 'validate.ps1'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($cmdPlanEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @()
    }
    elseif ($RecipeName -eq 'public-api-boundary') {
        $cmdPlanEntry = Get-RepositoryToolCommandPlan -ToolScriptName 'check-public-api-boundaries.ps1'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($cmdPlanEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @()
    }
    elseif ($RecipeName -eq 'shader-toolchain') {
        $cmdPlanEntry = Get-RepositoryToolCommandPlan -ToolScriptName 'check-shader-toolchain.ps1'
        $diagShader = New-RunnerDiagnostic -Severity 'info' -Code 'diagnostic-host-gate' -Message 'Shader toolchain validation reports Vulkan and Metal tool availability without marking missing host tools ready.' -ValidationRecipe $RecipeName
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($cmdPlanEntry) -HostGates @('vulkan-strict', 'metal-apple') -RequiredAcknowledgements @() -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @($diagShader)
    }
    elseif ($RecipeName -eq 'desktop-game-runtime') {
        $cmdPlanEntry = Get-RepositoryToolCommandPlan -ToolScriptName 'validate-desktop-game-runtime.ps1'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($cmdPlanEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @()
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-scene-gpu-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $pkgArgs = @('-GameTarget', $target)
        $pwEntry = Get-PwshScriptCommandPlan -ScriptPath $packageScript -ScriptArguments $pkgArgs
        $diagD3dSample = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 selected package validation is restricted to the reviewed sample_desktop_runtime_game package builder.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dSample)
    }
    elseif ($RecipeName -eq 'desktop-runtime-generated-material-shader-scaffold-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_generated_desktop_runtime_material_shader_package' } else { $SelectedGameTarget }
        $pkgArgs = @('-GameTarget', $target)
        $pwEntry = Get-PwshScriptCommandPlan -ScriptPath $packageScript -ScriptArguments $pkgArgs
        $diagD3dGen = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 generated material/shader package validation is restricted to the reviewed sample_generated_desktop_runtime_material_shader_package builder.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_generated_desktop_runtime_material_shader_package') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dGen)
    }
    elseif ($RecipeName -eq 'desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_generated_desktop_runtime_material_shader_package' } else { $SelectedGameTarget }
        $smokeTail = @(Get-GeneratedMaterialShaderScaffoldPackageVulkanSmokeArgs)
        $vulkanPkgArgs = @('-GameTarget', $target, '-RequireVulkanShaders', '-SmokeArgs') + $smokeTail
        $pwEntry = Get-PwshScriptCommandPlan -ScriptPath $packageScript -ScriptArguments $vulkanPkgArgs
        $diagVulkan = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Strict Vulkan generated material/shader scaffold package validation requires local Vulkan runtime, DXC SPIR-V CodeGen, and spirv-val readiness.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('vulkan-strict') -RequiredAcknowledgements @('vulkan-strict') -AllowedGameTargets @('sample_generated_desktop_runtime_material_shader_package') -AllowedStrictBackend @('', 'Vulkan') -Diagnostics @($diagVulkan)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameVulkanSmokeArgs)
        $vulkanPkgArgs = @('-GameTarget', $target, '-RequireVulkanShaders', '-SmokeArgs') + $smokeTail
        $pwEntry = Get-PwshScriptCommandPlan -ScriptPath $packageScript -ScriptArguments $vulkanPkgArgs
        $diagVulkan = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Strict Vulkan selected package validation requires local Vulkan runtime, DXC SPIR-V CodeGen, and spirv-val readiness.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('vulkan-strict') -RequiredAcknowledgements @('vulkan-strict') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'Vulkan') -Diagnostics @($diagVulkan)
    }
    elseif ($RecipeName -eq 'dev-windows-editor-game-module-driver-load-tests') {
        $driverScript = 'tools/run-editor-game-module-driver-load-tests.ps1'
        $pwEntry = Get-PwshScriptCommandPlan -ScriptPath $driverScript
        $diagWindows = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'MK_editor_game_module_driver_load_tests is a WIN32-only CMake target; run on Windows with MSVC dev preset outputs for the probe DLL and load-test binary.' -ValidationRecipe $RecipeName -HostGate 'windows-msvc-dev-editor-game-module-driver-ctest'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('windows-msvc-dev-editor-game-module-driver-ctest') -RequiredAcknowledgements @('windows-msvc-dev-editor-game-module-driver-ctest') -AllowedGameTargets @() -AllowedStrictBackend @('', 'D3D12', 'Vulkan') -Diagnostics @($diagWindows)
    }

    return $null
}
