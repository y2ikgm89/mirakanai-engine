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
    elseif ($RecipeName -eq 'renderer-metal-apple-host-evidence') {
        $cmdPlanEntry = Get-RepositoryToolCommandPlan -ToolScriptName 'validate-renderer-metal-apple.ps1'
        $diagMetal = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Renderer Metal Apple host evidence requires macOS with full Xcode/Metal tools, then runs the ci-macos-appleclang configure/build/CTest path for MK_backend_scaffold_tests and MK_renderer_quality_matrix_tests without marking Metal or broad renderer quality ready.' -ValidationRecipe $RecipeName -HostGate 'metal-apple'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($cmdPlanEntry) -HostGates @('metal-apple') -RequiredAcknowledgements @('metal-apple') -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @($diagMetal)
    }
    elseif ($RecipeName -eq 'desktop-game-runtime') {
        $cmdPlanEntry = Get-RepositoryToolCommandPlan -ToolScriptName 'validate-desktop-game-runtime.ps1'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($cmdPlanEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @()
    }
    elseif ($RecipeName -eq 'desktop-editor') {
        $cmdPlanEntry = Get-RepositoryToolCommandPlan -ToolScriptName 'build-editor.ps1'
        $diagEditor = New-RunnerDiagnostic -Severity 'info' -Code 'diagnostic-host-gate' -Message 'Optional native editor validation requires Windows with the desktop-editor preset and reports host readiness explicitly on non-ready hosts.' -ValidationRecipe $RecipeName -HostGate 'windows-msvc-desktop-editor'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($cmdPlanEntry) -HostGates @('windows-msvc-desktop-editor') -RequiredAcknowledgements @() -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @($diagEditor)
    }
    elseif ($RecipeName -eq 'installed-2d-sandbox-package-budget-smoke') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_2d_desktop_runtime_package' } else { $SelectedGameTarget }
        $smokeTail = @(
            '--smoke',
            '--require-config',
            'runtime/sample_2d_desktop_runtime_package.config',
            '--require-scene-package',
            'runtime/sample_2d_desktop_runtime_package.geindex',
            '--require-sandbox-package-budgets'
        )
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -SmokeArgs $smokeTail
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @('sample_2d_desktop_runtime_package') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @()
    }
    elseif ($RecipeName -eq 'installed-2d-performance-baseline-smoke') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_2d_desktop_runtime_package' } else { $SelectedGameTarget }
        $smokeTail = @(
            '--smoke',
            '--require-config',
            'runtime/sample_2d_desktop_runtime_package.config',
            '--require-scene-package',
            'runtime/sample_2d_desktop_runtime_package.geindex',
            '--require-win32-runtime-host',
            '--require-win32-d3d12-presentation',
            '--require-d3d12-shaders',
            '--require-d3d12-renderer',
            '--require-sandbox-package-budgets',
            '--require-performance-baseline'
        )
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -SmokeArgs $smokeTail
        $diagPerformance = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Selected 2D performance baseline smoke is restricted to the reviewed sample_2d_desktop_runtime_package D3D12 package lane.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_2d_desktop_runtime_package') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagPerformance)
    }
    elseif ($RecipeName -eq 'installed-2d-long-run-readiness-smoke') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_2d_desktop_runtime_package' } else { $SelectedGameTarget }
        $smokeTail = @(
            '--smoke',
            '--require-config',
            'runtime/sample_2d_desktop_runtime_package.config',
            '--require-scene-package',
            'runtime/sample_2d_desktop_runtime_package.geindex',
            '--require-win32-runtime-host',
            '--require-win32-d3d12-presentation',
            '--require-d3d12-shaders',
            '--require-d3d12-renderer',
            '--require-sandbox-package-budgets',
            '--require-performance-baseline',
            '--require-long-run-performance-readiness'
        )
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -SmokeArgs $smokeTail
        $diagLongRun = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Selected 2D long-run readiness smoke is restricted to the reviewed sample_2d_desktop_runtime_package D3D12 short-soak package lane and does not claim broad optimization.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_2d_desktop_runtime_package') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagLongRun)
    }
    elseif ($RecipeName -eq 'host-2d-long-run-readiness-soak') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_2d_desktop_runtime_package' } else { $SelectedGameTarget }
        $smokeTail = @(
            '--smoke',
            '--max-frames',
            '108000',
            '--require-config',
            'runtime/sample_2d_desktop_runtime_package.config',
            '--require-scene-package',
            'runtime/sample_2d_desktop_runtime_package.geindex',
            '--require-win32-runtime-host',
            '--require-win32-d3d12-presentation',
            '--require-d3d12-shaders',
            '--require-d3d12-renderer',
            '--require-sandbox-package-budgets',
            '--require-performance-baseline',
            '--require-long-run-performance-readiness'
        )
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -SmokeArgs $smokeTail
        $diagLongRunSoak = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Host-owned 2D long-run readiness soak is opt-in, uses 108000 frames on the reviewed sample_2d_desktop_runtime_package D3D12 package lane, and is excluded from default validation.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary', 'long-run-host-soak') -RequiredAcknowledgements @('d3d12-windows-primary', 'long-run-host-soak') -AllowedGameTargets @('sample_2d_desktop_runtime_package') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagLongRunSoak)
    }
    elseif ($RecipeName -eq 'network-enet') {
        $cmdPlanEntry = Get-RepositoryToolCommandPlan -ToolScriptName 'validate-network-enet.ps1'
        $diagNetwork = New-RunnerDiagnostic -Severity 'info' -Code 'diagnostic-host-gate' -Message 'Optional ENet validation requires bootstrapped vcpkg network-enet packages and reports missing dependency state explicitly on non-ready hosts.' -ValidationRecipe $RecipeName -HostGate 'network-enet-vcpkg'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($cmdPlanEntry) -HostGates @('network-enet-vcpkg') -RequiredAcknowledgements @() -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @($diagNetwork)
    }
    elseif ($RecipeName -eq 'network-production-security') {
        $ctestArgs = @('--preset', 'dev', '--output-on-failure', '-R', 'runtime_network|production_network_replication')
        $pwEntry = Get-PwshScriptCommandPlan -ScriptPath 'tools/ctest.ps1' -ScriptArguments $ctestArgs
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @()
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-scene-gpu-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $pkgArgs = @('-GameTarget', $target)
        $pwEntry = Get-PwshScriptCommandPlan -ScriptPath $packageScript -ScriptArguments $pkgArgs
        $diagD3dSample = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 selected package validation is restricted to the reviewed sample_desktop_runtime_game package builder.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dSample)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-fog-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentFogSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dFog = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 environment fog package validation is restricted to the reviewed sample_desktop_runtime_game D3D12 height-fog package lane and does not claim Vulkan, Metal, or broad environment readiness.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dFog)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-vulkan-environment-fog-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameVulkanEnvironmentFogPackageSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireVulkanShaders -SmokeArgs $smokeTail
        $diagVulkanFog = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Strict Vulkan environment fog package validation is restricted to the reviewed sample_desktop_runtime_game height-fog SPIR-V package lane with DXC SPIR-V CodeGen, spirv-val, Vulkan runtime readiness, and no Metal, volumetric fog, or broad environment_ready claim.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('vulkan-strict') -RequiredAcknowledgements @('vulkan-strict') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'Vulkan') -Diagnostics @($diagVulkanFog)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-volumetric-fog-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentVolumetricFogSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dVolumetricFog = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 volumetric fog package validation is restricted to the reviewed sample_desktop_runtime_game D3D12 froxel compute package lane and does not claim Vulkan, Metal, volumetric clouds, backend parity, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dVolumetricFog)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-physical-sky-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGamePhysicalSkySmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dPhysicalSky = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 physical sky package validation is restricted to the reviewed sample_desktop_runtime_game D3D12 physical-sky package lane and proves package-visible shader-contract, constants, LUT-intent, and zero native-handle counters without claiming Vulkan, Metal, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dPhysicalSky)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-cloud-layer-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameCloudLayerSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dCloudLayer = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 cloud layer package validation is restricted to the reviewed sample_desktop_runtime_game D3D12 low-cost cloud-layer package lane and does not claim texture upload, backend invocation, Vulkan, Metal, volumetric clouds, or broad environment readiness.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dCloudLayer)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-cloud-layer-renderer-execution') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameCloudLayerRendererExecutionSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dCloudLayerExecution = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 cloud layer renderer execution validation is restricted to the reviewed sample_desktop_runtime_game D3D12 low-cost cloud-layer package lane and proves texture upload, backend invocation, and renderer draw counters without claiming Vulkan, Metal, volumetric clouds, backend parity, broad optimization, or broad environment readiness.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dCloudLayerExecution)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-precipitation-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentPrecipitationSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dPrecipitation = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 environment precipitation package validation is restricted to the reviewed sample_desktop_runtime_game D3D12 rain package lane and does not claim particle-buffer upload, renderer/RHI backend invocation, material mutation, audio playback, Vulkan, Metal, or broad environment readiness.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dPrecipitation)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-precipitation-renderer-execution') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentPrecipitationRendererExecutionSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dPrecipitationExecution = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 rain precipitation renderer execution validation is restricted to the reviewed sample_desktop_runtime_game D3D12 package lane and proves particle-buffer upload, backend invocation, renderer draw, and scene-depth occlusion readback counters without claiming Vulkan, Metal, material wetness mutation, audio playback, backend parity, broad optimization, or broad environment readiness.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dPrecipitationExecution)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-snow-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentSnowSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dSnow = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 environment snow package validation is restricted to the reviewed sample_desktop_runtime_game D3D12 snow package lane and does not claim particle-buffer upload, renderer/RHI backend invocation, material mutation, audio playback, Vulkan, Metal, rain readiness, or broad environment readiness.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dSnow)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-snow-renderer-execution') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentSnowRendererExecutionSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dSnowExecution = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 snow precipitation renderer execution validation is restricted to the reviewed sample_desktop_runtime_game D3D12 package lane and proves snow particle-buffer upload, backend invocation, renderer draw, and scene-depth occlusion readback counters while keeping zero wetness mutation and no audio playback without claiming Vulkan, Metal, rain readiness, backend parity, broad optimization, or broad environment readiness.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dSnowExecution)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-profile-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentProfileSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -SmokeArgs $smokeTail
        $diagEnvironmentProfile = New-RunnerDiagnostic -Severity 'info' -Code 'package-evidence-reviewed' -Message 'Environment profile package validation is restricted to the reviewed sample_desktop_runtime_game package lane and proves cooked profile/index/scene dependency evidence without claiming renderer backend execution, Vulkan, Metal, rain readiness, or broad environment_ready.' -ValidationRecipe $RecipeName
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12', 'Vulkan') -Diagnostics @($diagEnvironmentProfile)
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
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireVulkanShaders -SmokeArgs $smokeTail
        $diagVulkan = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Strict Vulkan generated material/shader scaffold package validation requires local Vulkan runtime, DXC SPIR-V CodeGen, and spirv-val readiness.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('vulkan-strict') -RequiredAcknowledgements @('vulkan-strict') -AllowedGameTargets @('sample_generated_desktop_runtime_material_shader_package') -AllowedStrictBackend @('', 'Vulkan') -Diagnostics @($diagVulkan)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameVulkanSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireVulkanShaders -SmokeArgs $smokeTail
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
