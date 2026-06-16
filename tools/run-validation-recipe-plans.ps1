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
        $diagMetal = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Renderer Metal Apple host evidence requires macOS with full Xcode/Metal tools, then runs the ci-macos-appleclang configure/build/CTest path for MK_backend_scaffold_tests and MK_renderer_quality_matrix_tests. The recipe builds generated environment_feature_evidence.metallib, requires feature-local render/compute pipeline, cube/HDR texture, synchronization/readback proof, and emits metal_environment_* counters without marking backend parity, broad Metal readiness, broad renderer quality, or broad environment_ready ready by inference.' -ValidationRecipe $RecipeName -HostGate 'metal-apple'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($cmdPlanEntry) -HostGates @('metal-apple') -RequiredAcknowledgements @('metal-apple') -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @($diagMetal)
    }
    elseif ($RecipeName -eq 'renderer-metal-environment-aggregate-apple-host-evidence') {
        $cmdPlanEntry = Get-RepositoryToolCommandPlan -ToolScriptName 'validate-environment-metal-host-aggregate.ps1'
        $diagMetalEnvironmentAggregate = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Environment Metal host aggregate evidence requires macOS with full Xcode/Metal tools, delegates to renderer-metal-apple-host-evidence, and only then emits environment_metal_host_aggregate_* counters for selected sky, fog, cloud, precipitation, IBL, Metal resource, pipeline, synchronization, draw/dispatch, and readback proof. It does not mark backend parity, all-platform readiness, broad optimization, commercial readiness, Vulkan readiness, D3D12 readiness, or broad environment_ready ready by inference.' -ValidationRecipe $RecipeName -HostGate 'metal-apple'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($cmdPlanEntry) -HostGates @('metal-apple') -RequiredAcknowledgements @('metal-apple') -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @($diagMetalEnvironmentAggregate)
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
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-vulkan-volumetric-fog-renderer-execution') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameVulkanEnvironmentVolumetricFogRendererExecutionSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireVulkanShaders -SmokeArgs $smokeTail
        $diagVulkanVolumetricFogExecution = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Strict Vulkan volumetric fog renderer execution validation is restricted to the reviewed sample_desktop_runtime_game Vulkan package lane with --require-environment-volumetric-fog-vulkan-renderer-execution and proves SPIR-V compute shader artifacts, storage-buffer froxel output binding, descriptor-set binding evidence, compute dispatch, synchronization2 memory barrier evidence, froxel readback, and zero native-handle counters without claiming D3D12 by inference, Metal, backend parity, broad optimization, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('vulkan-strict') -RequiredAcknowledgements @('vulkan-strict') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'Vulkan') -Diagnostics @($diagVulkanVolumetricFogExecution)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-volumetric-cloud-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameVolumetricCloudSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dVolumetricCloud = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 volumetric cloud package validation is restricted to the reviewed sample_desktop_runtime_game D3D12 package lane and proves package/shader/quality rows without claiming texture upload, renderer/RHI execution, Vulkan, Metal, backend parity, broad optimization, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dVolumetricCloud)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-volumetric-cloud-renderer-execution') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameVolumetricCloudRendererExecutionSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dVolumetricCloudExecution = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 volumetric cloud renderer execution validation is restricted to the reviewed sample_desktop_runtime_game D3D12 package lane and proves volume texture upload, backend invocation, renderer draw, raymarch, and readback counters without claiming Vulkan, Metal, backend parity, broad optimization, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dVolumetricCloudExecution)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-vulkan-volumetric-cloud-renderer-execution') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameVulkanVolumetricCloudRendererExecutionSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireVulkanShaders -SmokeArgs $smokeTail
        $diagVulkanVolumetricCloudExecution = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Strict Vulkan volumetric cloud renderer execution validation is restricted to the reviewed sample_desktop_runtime_game Vulkan package lane with --require-environment-volumetric-cloud-vulkan-renderer-execution and proves SPIR-V vertex/fragment shader artifacts, shifted descriptor bindings, descriptor-set binding evidence, synchronization2 image-barrier evidence, volume texture uploads, renderer draw, raymarch, readback, and zero native-handle/audio/precipitation counters without claiming D3D12 by inference, Metal, backend parity, broad optimization, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('vulkan-strict') -RequiredAcknowledgements @('vulkan-strict') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'Vulkan') -Diagnostics @($diagVulkanVolumetricCloudExecution)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-lighting-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentLightingSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dEnvironmentLighting = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 environment lighting package validation is restricted to the reviewed sample_desktop_runtime_game first-party IBL metadata package lane and proves reflection cubemap, irradiance, radiance mip, HDR metadata, and package-source counters without claiming renderer cubemap upload, runtime capture, Vulkan, Metal, backend parity, broad optimization, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dEnvironmentLighting)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-ibl-renderer-execution') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentLightingRendererExecutionSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dEnvironmentLightingExecution = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 environment IBL renderer execution validation is restricted to the reviewed sample_desktop_runtime_game D3D12 package lane and proves TextureCube upload, renderer backend invocation, runtime cubemap capture readback, and zero native-handle counters without claiming Vulkan, Metal, backend parity, broad optimization, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dEnvironmentLightingExecution)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-vulkan-environment-ibl-renderer-execution') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameVulkanEnvironmentIblRendererExecutionSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireVulkanShaders -SmokeArgs $smokeTail
        $diagVulkanEnvironmentIblExecution = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Strict Vulkan environment IBL renderer execution validation is restricted to the reviewed sample_desktop_runtime_game Vulkan package lane with --require-environment-lighting-vulkan-renderer-execution and proves target-specific SPIR-V sample shader artifacts, cube-compatible image allocation, cube image view, sampler, descriptor-set binding evidence, synchronization2 image barriers, TextureCube upload, runtime cubemap capture readback, dynamic-rendering sampled draw/readback proof, and zero native-handle counters without claiming D3D12 by inference, Metal, backend parity, broad optimization, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('vulkan-strict') -RequiredAcknowledgements @('vulkan-strict') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'Vulkan') -Diagnostics @($diagVulkanEnvironmentIblExecution)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-physical-sky-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGamePhysicalSkySmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dPhysicalSky = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 physical sky package validation is restricted to the reviewed sample_desktop_runtime_game D3D12 physical-sky package lane and proves package-visible shader-contract, constants, LUT-intent, and zero native-handle counters without claiming Vulkan, Metal, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dPhysicalSky)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-vulkan-physical-sky-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameVulkanPhysicalSkySmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireVulkanShaders -SmokeArgs $smokeTail
        $diagVulkanPhysicalSky = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Strict Vulkan physical sky package validation is restricted to the reviewed sample_desktop_runtime_game Vulkan package lane with --require-physical-sky-vulkan-package-evidence and proves Vulkan scene SPIR-V artifacts, package-visible shader-contract, constants, LUT-intent, selected Vulkan backend evidence, and zero native-handle counters without claiming D3D12 by inference, Metal, backend parity, broad optimization, broad sky readiness, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('vulkan-strict') -RequiredAcknowledgements @('vulkan-strict') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'Vulkan') -Diagnostics @($diagVulkanPhysicalSky)
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
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-vulkan-environment-precipitation-renderer-execution') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameVulkanEnvironmentPrecipitationRendererExecutionSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireVulkanShaders -SmokeArgs $smokeTail
        $diagVulkanPrecipitationExecution = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Strict Vulkan rain precipitation renderer execution validation is restricted to the reviewed sample_desktop_runtime_game Vulkan package lane and proves SPIR-V precipitation shader artifacts, descriptor-set binding evidence, particle buffer upload, backend invocation, renderer draw, scene-depth occlusion readback, synchronization2 barrier evidence, and zero native-handle counters without claiming D3D12 by inference, Metal, backend parity, broad optimization, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('vulkan-strict') -RequiredAcknowledgements @('vulkan-strict') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'Vulkan') -Diagnostics @($diagVulkanPrecipitationExecution)
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
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-material-weathering') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentMaterialWeatheringSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dMaterialWeathering = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 material weathering validation is restricted to the reviewed sample_desktop_runtime_game D3D12 material-parameter package lane and proves wet/snow material response rows, positive material binding/upload counters, zero source-material mutation, and zero native-handle counters without claiming Vulkan, Metal, backend parity, broad optimization, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dMaterialWeathering)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-audio-playback') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentAudioPlaybackSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagD3dWeatherAudio = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 environment weather audio playback validation uses --require-environment-audio-playback and is restricted to the reviewed sample_desktop_runtime_game package lane; it proves MK_environment trigger rows consumed by the MK_audio mixer/render lane with positive environment_audio_runtime_cues_started, positive environment_audio_runtime_render_commands, positive environment_audio_runtime_render_frames, and zero environment-owned device, device IO, and native-handle counters; it does not claim physical WASAPI endpoint playback, Vulkan, Metal, backend parity, broad optimization, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagD3dWeatherAudio)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-profile-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentProfileSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -SmokeArgs $smokeTail
        $diagEnvironmentProfile = New-RunnerDiagnostic -Severity 'info' -Code 'package-evidence-reviewed' -Message 'Environment profile package validation is restricted to the reviewed sample_desktop_runtime_game package lane and proves cooked profile/index/scene dependency evidence without claiming renderer backend execution, Vulkan, Metal, rain readiness, or broad environment_ready.' -ValidationRecipe $RecipeName
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12', 'Vulkan') -Diagnostics @($diagEnvironmentProfile)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-ready-aggregate') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentReadyAggregateSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagEnvironmentReady = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 environment ready aggregate validation is restricted to the reviewed sample_desktop_runtime_game D3D12 package lane and proves selected profile v2, physical sky, height fog, volumetric fog, cloud layer, rain precipitation, volumetric cloud, IBL renderer execution, material weathering, weather audio playback, and quality-budget counters with zero native-handle access; it does not claim Vulkan strict readiness, Metal host readiness, backend parity, broad optimization, or snow-by-inference.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagEnvironmentReady)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-vulkan-strict-aggregate') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentVulkanStrictAggregateSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireVulkanShaders -SmokeArgs $smokeTail
        $diagEnvironmentVulkanStrict = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Strict Vulkan environment aggregate validation is restricted to the reviewed sample_desktop_runtime_game Vulkan package lane and proves selected profile v2, Vulkan physical sky, height fog, IBL renderer execution, volumetric fog compute, volumetric cloud renderer execution, rain precipitation renderer execution, quality-budget counters, descriptor-set bindings, strict toolchain rows, Vulkan validation layer enablement, SPIR-V validation, dynamic rendering, synchronization2, resource usage/layout rows, synchronization2 barriers, draw/dispatch/upload/readback counters, and zero diagnostics/native-handle/fallback counters without claiming D3D12 by inference, Metal host readiness, backend parity, broad optimization, all-platform readiness, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('vulkan-strict') -RequiredAcknowledgements @('vulkan-strict') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'Vulkan') -Diagnostics @($diagEnvironmentVulkanStrict)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-texture-asset-pipeline-vulkan-upload') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(
            '--smoke',
            '--require-config',
            'runtime/sample_desktop_runtime_game.config',
            '--require-scene-package',
            'runtime/sample_desktop_runtime_game.geindex',
            '--require-environment-texture-asset-pipeline-package',
            '--require-environment-texture-asset-pipeline-vulkan-upload'
        )
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -SmokeArgs $smokeTail
        $diagEnvironmentTextureVulkanUpload = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Strict Vulkan environment texture asset-pipeline upload validation is restricted to the reviewed sample_desktop_runtime_game Vulkan runtime RHI upload/readback lane. It proves the selected package RGBA8 payload through IRhiDevice texture upload, descriptor write, Vulkan readback, compact checksum comparison, row-pitch evidence, positive resource transitions, and zero native-handle/Metal/backend-parity/broad-ready counters without requiring scene SPIR-V artifacts or claiming backend-target compressed payload execution.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('vulkan-strict') -RequiredAcknowledgements @('vulkan-strict') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'Vulkan') -Diagnostics @($diagEnvironmentTextureVulkanUpload)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-texture-asset-pipeline-d3d12-compressed-upload') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(
            '--smoke',
            '--require-config',
            'runtime/sample_desktop_runtime_game.config',
            '--require-scene-package',
            'runtime/sample_desktop_runtime_game.geindex',
            '--require-environment-texture-asset-pipeline-package',
            '--require-environment-texture-asset-pipeline-d3d12-compressed-upload'
        )
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -SmokeArgs $smokeTail
        $diagEnvironmentTextureD3d12CompressedUpload = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'D3D12 environment texture asset-pipeline compressed upload validation is restricted to the reviewed sample_desktop_runtime_game D3D12 WARP backend-target BC7 lane. It proves official D3D12 format-support evidence, BC7 block footprint rows, IRhiDevice texture upload, descriptor write, D3D12 readback, compact checksum comparison, row-pitch evidence, positive resource transitions, and zero native-handle/backend-parity/broad-ready counters without claiming Vulkan BC7, Metal/ASTC execution, backend parity, broad asset-pipeline coverage beyond the selected OpenEXR/KTX/Basis closeout lane, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagEnvironmentTextureD3d12CompressedUpload)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-texture-asset-pipeline-vulkan-compressed-upload') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(
            '--smoke',
            '--require-config',
            'runtime/sample_desktop_runtime_game.config',
            '--require-scene-package',
            'runtime/sample_desktop_runtime_game.geindex',
            '--require-environment-texture-asset-pipeline-package',
            '--require-environment-texture-asset-pipeline-vulkan-compressed-upload'
        )
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -SmokeArgs $smokeTail
        $diagEnvironmentTextureVulkanCompressedUpload = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Strict Vulkan environment texture asset-pipeline compressed upload validation is restricted to the reviewed sample_desktop_runtime_game Vulkan backend-target BC7 lane. It proves official vkGetPhysicalDeviceFormatProperties evidence for the selected BC7 sampled-image and transfer feature set, BC7 block footprint rows, IRhiDevice texture upload, descriptor write, Vulkan readback, compact checksum comparison, row-pitch evidence, positive resource transitions, and zero native-handle/Metal/backend-parity/broad-ready counters without claiming Metal/ASTC execution, backend parity, all-platform readiness, broad asset-pipeline coverage beyond the selected OpenEXR/KTX/Basis closeout lane, commercial readiness, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('vulkan-strict') -RequiredAcknowledgements @('vulkan-strict') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'Vulkan') -Diagnostics @($diagEnvironmentTextureVulkanCompressedUpload)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-texture-asset-pipeline-metal-compressed-upload') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(
            '--smoke',
            '--require-config',
            'runtime/sample_desktop_runtime_game.config',
            '--require-scene-package',
            'runtime/sample_desktop_runtime_game.geindex',
            '--require-environment-texture-asset-pipeline-package',
            '--require-environment-texture-asset-pipeline-metal-compressed-upload'
        )
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -SmokeArgs $smokeTail
        $diagEnvironmentTextureMetalCompressedUpload = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Apple-host Metal environment texture asset-pipeline compressed upload validation is restricted to the reviewed sample_desktop_runtime_game Metal ASTC lane. It proves official Apple Metal Feature Set Tables / MTLPixelFormat ASTC 4x4 evidence, ASTC block footprint rows, Metal private texture upload through MTLBlitCommandEncoder copyFromBuffer, Metal readback, compact checksum comparison, texture usage rows, and zero public native-handle/Vulkan/backend-parity/broad-ready counters without claiming Metal aggregate readiness, backend parity, all-platform readiness, broad asset-pipeline coverage beyond the selected OpenEXR/KTX/Basis closeout lane, commercial readiness, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'metal-apple'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('metal-apple') -RequiredAcknowledgements @('metal-apple') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'Metal') -Diagnostics @($diagEnvironmentTextureMetalCompressedUpload)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-asset-pipeline-openexr-ktx-basis-ready') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(
            '--smoke',
            '--require-config',
            'runtime/sample_desktop_runtime_game.config',
            '--require-scene-package',
            'runtime/sample_desktop_runtime_game.geindex',
            '--require-environment-texture-asset-pipeline-package',
            '--require-environment-asset-pipeline-openexr-ktx-basis-ready'
        )
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -SmokeArgs $smokeTail
        $diagEnvironmentAssetPipelineReady = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'OpenEXR/KTX/Basis asset-pipeline closeout validates the selected source-to-package cooker, package smoke, D3D12/Vulkan RGBA8 upload/readback rows, D3D12/Vulkan BC7 compressed upload/readback rows, and separately hosted Apple Metal ASTC recipe evidence. It emits environment_asset_pipeline_openexr_ktx_basis_ready=1 only for this selected package-visible lane and does not claim backend parity, all-platform readiness, runtime source parsing, runtime optional codec/transcode execution, commercial readiness, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'metal-apple'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary', 'vulkan-strict', 'metal-apple') -RequiredAcknowledgements @('d3d12-windows-primary', 'vulkan-strict', 'metal-apple') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12', 'Vulkan') -Diagnostics @($diagEnvironmentAssetPipelineReady)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-backend-parity') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentBackendParitySmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagEnvironmentBackendParity = New-RunnerDiagnostic -Severity 'info' -Code 'host-evidence-required' -Message 'Environment backend parity package validation is restricted to the reviewed sample_desktop_runtime_game D3D12 package lane plus existing strict Vulkan aggregate row evidence. It feeds the value-only parity matrix with D3D12 and strict Vulkan ready rows plus Apple-host Metal host-gated rows, expects environment_backend_parity_status=host_evidence_required, environment_backend_parity_ready=0, zero diagnostics/native-handle/GPU-command side effects, and does not promote all-platform, commercial, broad optimization, or broad environment_ready claims.' -ValidationRecipe $RecipeName -HostGate 'metal-apple'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary', 'vulkan-strict', 'metal-apple') -RequiredAcknowledgements @('d3d12-windows-primary', 'vulkan-strict') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagEnvironmentBackendParity)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-platform-readiness') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentPlatformReadinessSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagEnvironmentPlatformReadiness = New-RunnerDiagnostic -Severity 'info' -Code 'host-evidence-required' -Message 'Environment platform readiness package validation is a review-only row map over exact platform/backend evidence. It proves Windows D3D12 from the selected D3D12 aggregate, leaves Windows Vulkan, Linux Vulkan, macOS Metal, iOS Metal, and Android Vulkan host-gated, expects environment_platform_readiness_status=host_evidence_required, environment_platform_readiness_ready=0, environment_all_platform_unconditional_ready=0, zero diagnostics/native-handle/GPU-command side effects, and does not promote backend parity, commercial readiness, broad optimization, or broad environment_ready claims.' -ValidationRecipe $RecipeName -HostGate 'all-platform-host-matrix'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary', 'vulkan-strict', 'metal-apple', 'android-gameactivity') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagEnvironmentPlatformReadiness)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-optimization-measurement') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentOptimizationMeasurementSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagEnvironmentOptimization = New-RunnerDiagnostic -Severity 'info' -Code 'host-evidence-required' -Message 'Environment optimization measurement package validation is restricted to the reviewed sample_desktop_runtime_game D3D12 package lane. It emits preset_pack_flythrough, storm_precipitation, dense_volumetric_fog, volumetric_cloud_sunset, snowfield_material_weathering, weather_simulation_stress, and asset_library_cold_load before/after measurement rows, regression-budget counters, and zero diagnostics/native-handle/GPU-command side effects while keeping environment_broad_optimization_ready=0 until backend parity, retained host profiler artifacts, and regression guards are complete.' -ValidationRecipe $RecipeName -HostGate 'd3d12-windows-primary'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagEnvironmentOptimization)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-weather-simulation-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentWeatherSimulationPackageSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagEnvironmentWeatherSimulation = New-RunnerDiagnostic -Severity 'info' -Code 'package-evidence-reviewed' -Message 'Environment weather simulation package validation is restricted to the reviewed sample_desktop_runtime_game package lane. It emits deterministic CPU reference counters for one clamped timestep, four cells, water-conservation/error bounds, CPU fallback use, selected CPU reference solver elapsed/budget counters, selected D3D12 WARP GPU solver elapsed/budget/dispatch/barrier/hash counters, solver_budget_status=host_evidence_required, zero diagnostics/native-handle/backend-parity side effects, profiler artifacts=2, profiler tool rows=2, profiler backend rows=1, profiler budget ready=1, production solver package counter review ready=1, production solver package counter rows=1, selected production solver core review ready=1, production solver core rows=1, selected broad physical accuracy review ready=1, broad physical accuracy rows=1, selected visual quality review ready=1, visual quality rows=1, production solver ready=0, physical weather ready=0, and environment_physical_weather_simulation_ready=0; it does not promote complete physical weather simulation, Vulkan/Metal parity, backend parity, broad optimization, commercial readiness, or broad environment_ready claims.' -ValidationRecipe $RecipeName
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagEnvironmentWeatherSimulation)
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
