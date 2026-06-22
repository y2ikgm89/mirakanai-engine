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

    function Get-EnvironmentHighestCommercialSkeletonPlan {
        param(
            [Parameter(Mandatory = $true)]
            [string]$Recipe,

            [Parameter(Mandatory = $true)]
            [string]$HostGate,

            [Parameter(Mandatory = $true)]
            [string]$ReadyCounter,

            [string[]]$AdditionalCounters = @()
        )

        $counterRows = @(
            "validation_recipe=$Recipe",
            "validation_recipe_skeleton=1",
            "host_gate=$HostGate",
            "ready_claim=0",
            "$ReadyCounter=0",
            "package_command_executed=0",
            "gpu_command_executed=0",
            "host_command_executed=0"
        ) + @($AdditionalCounters)
        $scriptText = "Write-Output " + (ConvertTo-PwshSingleQuotedLiteral ([string]::Join(' ', $counterRows)))
        $entry = New-CommandPlanEntry -Command 'pwsh' -Argv @(
            '-NoProfile',
            '-ExecutionPolicy',
            'Bypass',
            '-Command',
            $scriptText
        )
        $diagnostic = New-RunnerDiagnostic -Severity 'info' -Code 'skeleton-ready-claim-zero' -Message "Highest commercial readiness skeleton recipe '$Recipe' is registered for reviewed dry-run routing only; all ready claims remain 0 until its owning implementation task replaces the skeleton with package-visible evidence." -ValidationRecipe $Recipe -HostGate $HostGate
        return New-RecipePlanRow -Recipe $Recipe -CommandPlan @($entry) -HostGates @($HostGate) -RequiredAcknowledgements @($HostGate) -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @($diagnostic)
    }

    function Get-EnvironmentPlatformVulkanHostPlan {
        param(
            [Parameter(Mandatory = $true)]
            [string]$Recipe,

            [Parameter(Mandatory = $true)]
            [string]$ScriptPath,

            [Parameter(Mandatory = $true)]
            [string]$HostGate,

            [Parameter(Mandatory = $true)]
            [string[]]$ExpectedEvidenceCounters,

            [string[]]$AdditionalScriptArguments = @(),

            [Parameter(Mandatory = $true)]
            [string]$Message
        )

        $scriptArguments = @('-RequireReady') + @($AdditionalScriptArguments) + @('-ExpectedEvidenceCounters') + @($ExpectedEvidenceCounters)
        $entry = Get-PwshScriptCommandPlan -ScriptPath $ScriptPath -ScriptArguments $scriptArguments
        $diagnostic = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message $Message -ValidationRecipe $Recipe -HostGate $HostGate
        return New-RecipePlanRow -Recipe $Recipe -CommandPlan @($entry) -HostGates @($HostGate) -RequiredAcknowledgements @($HostGate) -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @($diagnostic)
    }

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
        $diagMetal = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Renderer Metal Apple host evidence requires macOS with full Xcode/Metal tools, then runs the ci-macos-appleclang configure/build/CTest path for MK_backend_scaffold_tests and MK_renderer_quality_matrix_tests. The recipe builds generated environment_feature_evidence.metallib, requires feature-local render/compute pipeline, cube/HDR texture, synchronization/readback proof, and emits metal_environment_* counters. Those counters may feed make_backend_renderer_parity_apple_metal_environment_proofs only for backend-local synchronization, shader_validation, and package_evidence rows; memory_residency and profiling_capture stay separate, and the recipe does not mark backend parity, broad Metal readiness, broad renderer quality, or broad environment_ready ready by inference.' -ValidationRecipe $RecipeName -HostGate 'metal-apple'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($cmdPlanEntry) -HostGates @('metal-apple') -RequiredAcknowledgements @('metal-apple') -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @($diagMetal)
    }
    elseif ($RecipeName -eq 'renderer-metal-environment-aggregate-apple-host-evidence') {
        $cmdPlanEntry = Get-RepositoryToolCommandPlan -ToolScriptName 'validate-environment-metal-host-aggregate.ps1'
        $diagMetalEnvironmentAggregate = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Environment Metal host aggregate evidence requires macOS with full Xcode/Metal tools, delegates to renderer-metal-apple-host-evidence, and only then emits environment_metal_host_aggregate_* counters plus a Metal-only backend parity bridge with environment_backend_parity_metal_evidence_ready=1 while environment_backend_parity_ready=0, a Metal-only commercial blocker view with environment_commercial_metal_evidence_requested=1, environment_commercial_metal_host_aggregate_ready=1, environment_commercial_macos_metal_ready=1, and environment_commercial_ready=0, and a macOS Metal platform bridge with environment_platform_macos_metal_evidence_requested=1, environment_platform_macos_metal_ready=1, and environment_platform_readiness_ready=0. It does not mark backend parity, all-platform readiness, broad optimization, commercial readiness, Linux Vulkan, iOS Metal, Android Vulkan, D3D12 readiness, or broad environment_ready ready by inference.' -ValidationRecipe $RecipeName -HostGate 'metal-apple'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($cmdPlanEntry) -HostGates @('metal-apple') -RequiredAcknowledgements @('metal-apple') -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @($diagMetalEnvironmentAggregate)
    }
    elseif ($RecipeName -eq 'environment-weather-metal-solver-host-gate') {
        $cmdPlanEntry = Get-RepositoryToolCommandPlan -ToolScriptName 'validate-environment-weather-metal-solver-host-gate.ps1'
        $diagMetalWeatherSolver = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Environment weather Metal solver evidence requires macOS with full Xcode/Metal tools. It validates only the selected Metal compute weather solver metallib, command queue/buffer, compute pipeline, buffer bindings 0/1/2, one dispatch, readback hash, and elapsed/budget counters. It does not run the Metal aggregate recipe and must not mark backend parity, Vulkan/Metal solver parity, production solver readiness, complete physical weather simulation, all-platform readiness, commercial readiness, or broad environment_ready ready by inference.' -ValidationRecipe $RecipeName -HostGate 'metal-apple'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($cmdPlanEntry) -HostGates @('metal-apple') -RequiredAcknowledgements @('metal-apple') -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @($diagMetalWeatherSolver)
    }
    elseif ($RecipeName -eq 'environment-platform-linux-vulkan-host-gate') {
        $cmdPlanEntry = Get-PwshScriptCommandPlan -ScriptPath 'tools/validate-linux-vulkan-runtime-host.ps1' -ScriptArguments @('-RequireReady')
        $diagLinuxVulkanHost = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Linux Vulkan platform evidence requires a Linux host with Vulkan SDK tools, Linux Vulkan ICD/runtime/driver evidence, VK_LAYER_KHRONOS_validation, DXC SPIR-V CodeGen, spirv-val, a first-party Linux desktop/runtime host, a Linux package validation script, installed Linux package smoke, and readback counters. It must keep environment_platform_linux_vulkan_ready=0 and environment_all_platform_unconditional_ready=0 until those exact Linux package counters exist, and it must not accept Windows Vulkan, Win32 package evidence, Android Vulkan, compile-only evidence, backend parity, commercial readiness, or broad environment_ready by inference.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict-linux'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($cmdPlanEntry) -HostGates @('vulkan-strict-linux') -RequiredAcknowledgements @('vulkan-strict-linux') -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @($diagLinuxVulkanHost)
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
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-backend-parity-ready') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentBackendParityReadySmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -RequireVulkanShaders -SmokeArgs $smokeTail
        $diagEnvironmentBackendParityReady = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Environment backend parity ready closeout validates the reviewed D3D12 package aggregate, strict Vulkan aggregate, and separately hosted Apple Metal aggregate evidence through the same package-visible parity matrix. It expects environment_backend_parity_status=ready, environment_backend_parity_ready=1, ready_rows=21, host_gated_rows=0, host_validated_backends=3, metal_evidence_consumed=1, cross_host_aggregate_ready=1, zero diagnostics/native-handle/GPU-command side effects, zero D3D12/Vulkan/Metal inference counters, and no all-platform, commercial, broad optimization, or broad environment_ready promotion.' -ValidationRecipe $RecipeName -HostGate 'metal-apple'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary', 'vulkan-strict', 'metal-apple') -RequiredAcknowledgements @('d3d12-windows-primary', 'vulkan-strict', 'metal-apple') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12', 'Vulkan') -Diagnostics @($diagEnvironmentBackendParityReady)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-platform-readiness') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentPlatformReadinessSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagEnvironmentPlatformReadiness = New-RunnerDiagnostic -Severity 'info' -Code 'host-evidence-required' -Message 'Environment platform readiness package validation is the D3D12 row-map lane over exact platform/backend evidence. It proves Windows D3D12 from the selected D3D12 aggregate, leaves Windows Vulkan to the dedicated Windows Vulkan evidence bridge, keeps Linux Vulkan, macOS Metal, iOS Metal, and Android Vulkan host-gated, expects environment_platform_readiness_status=host_evidence_required, environment_platform_readiness_ready=0, environment_all_platform_unconditional_ready=0, zero diagnostics/native-handle/GPU-command side effects, and does not promote backend parity, commercial readiness, broad optimization, or broad environment_ready claims.' -ValidationRecipe $RecipeName -HostGate 'all-platform-host-matrix'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary', 'vulkan-strict', 'metal-apple', 'android-gameactivity') -RequiredAcknowledgements @('d3d12-windows-primary') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagEnvironmentPlatformReadiness)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-platform-windows-vulkan-evidence') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentPlatformWindowsVulkanEvidenceSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireVulkanShaders -SmokeArgs $smokeTail
        $diagEnvironmentPlatformWindowsVulkan = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Windows Vulkan platform evidence validation is the Vulkan-only row bridge for the reviewed sample_desktop_runtime_game strict Vulkan package lane. It proves environment_platform_windows_vulkan_evidence_requested=1, environment_platform_windows_vulkan_strict_aggregate_ready=1, environment_platform_windows_vulkan_ready=1, environment_platform_requires_windows_vulkan_host_evidence=0, environment_platform_readiness_ready=0, environment_all_platform_unconditional_ready=0, zero diagnostics/native-handle/GPU-command side effects, and no Linux Vulkan, Android Vulkan, Metal, backend parity, commercial, broad optimization, or broad environment_ready promotion.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('vulkan-strict') -RequiredAcknowledgements @('vulkan-strict') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'Vulkan') -Diagnostics @($diagEnvironmentPlatformWindowsVulkan)
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
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-artist-workflow-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentArtistWorkflowPackageSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -SmokeArgs $smokeTail
        $diagEnvironmentArtistWorkflow = New-RunnerDiagnostic -Severity 'info' -Code 'package-evidence-reviewed' -Message 'Environment artist workflow package validation is restricted to the reviewed sample_desktop_runtime_game D3D12 package lane. It proves environment_artist_workflow_ready=1 through eight package-visible reviewed rows covering visible editor shell, OpenEXR/KTX/Basis asset pipeline, selected preset library, validation remediation, revision safety, production walkthrough package, editor-core no-execution boundary, and operator review. It requires zero backend/package-script/validation-recipe/native-handle execution from editor core and keeps environment_artist_workflow_commercial_ready=0.' -ValidationRecipe $RecipeName
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12') -Diagnostics @($diagEnvironmentArtistWorkflow)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-commercial-readiness') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentCommercialReadinessSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireD3d12Shaders -RequireVulkanShaders -SmokeArgs $smokeTail
        $diagEnvironmentCommercial = New-RunnerDiagnostic -Severity 'info' -Code 'retained-blocker-lane' -Message 'This retained Phase 12 commercial readiness package lane is a non-promoting blocker view for sample_desktop_runtime_game. The highest commercial closeout is the separate environment-highest-commercial-readiness-closeout validator, which may report environment_highest_commercial_ready=1 and environment_commercial_ready=1 only after every selected dependency row is ready, package-visible, legally current, validation-guarded, and adjacent broad non-claims remain explicit.' -ValidationRecipe $RecipeName -HostGate 'commercial-environment-closeout'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('d3d12-windows-primary', 'vulkan-strict', 'metal-apple', 'android-gameactivity', 'commercial-environment-closeout') -RequiredAcknowledgements @('d3d12-windows-primary', 'vulkan-strict') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12', 'Vulkan') -Diagnostics @($diagEnvironmentCommercial)
    }
    elseif ($RecipeName -eq 'environment-highest-commercial-readiness-closeout') {
        $expectedCounters = @(
            'validation_recipe=environment-highest-commercial-readiness-closeout',
            'environment_highest_commercial_status=ready',
            'environment_highest_commercial_ready=1',
            'environment_commercial_ready=1',
            'environment_commercial_required_rows=16',
            'environment_commercial_ready_rows=16',
            'environment_host_gated_rows=0',
            'environment_dependency_gated_rows=0',
            'environment_blocked_rows=0',
            'environment_unsupported_rows=0',
            'environment_missing_rows=0',
            'environment_native_handle_access=0',
            'environment_commercial_diagnostics=0',
            'environment_strict_vulkan_aggregate_ready=1',
            'environment_metal_aggregate_ready=1',
            'environment_backend_parity_ready=1',
            'environment_platform_windows_d3d12_ready=1',
            'environment_platform_windows_vulkan_ready=1',
            'environment_platform_linux_vulkan_ready=1',
            'environment_platform_macos_metal_ready=1',
            'environment_platform_ios_metal_ready=1',
            'environment_platform_android_vulkan_ready=1',
            'environment_platform_readiness_ready=1',
            'environment_all_platform_unconditional_ready=1',
            'environment_broad_optimization_ready=1',
            'environment_optimization_measurement_workload_rows=21',
            'environment_optimization_measurement_backend_rows=3',
            'environment_optimization_measurement_before_after_pairs=21',
            'environment_optimization_measurement_profiler_artifacts=21',
            'environment_optimization_measurement_trace_event_json=21',
            'environment_optimization_measurement_missing_artifacts=0',
            'environment_optimization_measurement_over_budget=0',
            'environment_asset_pipeline_openexr_ktx_basis_full_ready=1',
            'runtime_source_parsing=0',
            'environment_aaa_preset_asset_library_ready=1',
            'environment_physical_weather_simulation_ready=1',
            'environment_weather_simulation_backend_parity_ready=1',
            'environment_artist_workflow_production_ready=1',
            'workflow_visible_shell_execution_ready=1',
            'workflow_operator_review_ready=1',
            'environment_ready=0',
            'environment_ready_unchanged=1'
        )
        $scriptArguments = @('-RequireReady', '-ExpectedEvidenceCounters') + $expectedCounters
        $pwEntry = Get-PwshScriptCommandPlan `
            -ScriptPath 'tools/validate-environment-highest-commercial-readiness.ps1' `
            -ScriptArguments $scriptArguments
        $diagEnvironmentHighestCommercial = New-RunnerDiagnostic -Severity 'info' -Code 'final-aggregate-fail-closed' -Message 'Environment highest commercial readiness closeout validates the clean-break commercial v2 aggregate. It builds and runs MK_environment_commercial_readiness_v2_tests, reads the manifest commercial claim matrix for the 16 exact v2 dependency rows, requires every row to be ready, requires zero host-gated, dependency-gated, blocked, unsupported, missing, native-handle, and diagnostic rows, emits environment_highest_commercial_ready=1 and environment_commercial_ready=1 only when those conditions are met, and keeps broad environment_ready unchanged at 0.' -ValidationRecipe $RecipeName -HostGate 'commercial-environment-highest-closeout'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('commercial-environment-highest-closeout') -RequiredAcknowledgements @('commercial-environment-highest-closeout') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12', 'Vulkan') -Diagnostics @($diagEnvironmentHighestCommercial)
    }
    elseif ($RecipeName -eq 'environment-platform-linux-vulkan-package') {
        return Get-EnvironmentPlatformVulkanHostPlan `
            -Recipe $RecipeName `
            -ScriptPath 'tools/validate-linux-vulkan-runtime-host.ps1' `
            -HostGate 'linux-vulkan-runtime-host' `
            -ExpectedEvidenceCounters @(
                'validation_recipe=environment-platform-linux-vulkan-package',
                'host=linux',
                'vulkaninfo_ready=1',
                'VK_LAYER_KHRONOS_validation_ready=1',
                'dxc_spirv_codegen_ready=1',
                'spirv_val_ready=1',
                'linux_icd_runtime_ready=1',
                'first_party_linux_runtime_host_ready=1',
                'linux_package_script_ready=1',
                'linux_installed_validator_ready=1',
                'linux_package_smoke_ready=1',
                'linux_vulkan_readback_ready=1',
                'linux_vulkan_validation_log_clean=1',
                'environment_platform_linux_vulkan_ready=1',
                'environment_platform_requires_linux_vulkan_host_evidence=0',
                'environment_all_platform_unconditional_ready=0'
            ) `
            -Message 'Linux Vulkan platform validation requires a Linux host with Vulkan SDK tools, vulkaninfo summary evidence, VK_LAYER_KHRONOS_validation, DXC SPIR-V CodeGen, spirv-val, Linux ICD/runtime evidence, first-party Linux runtime host/package script/installed validator rows, package smoke, Vulkan readback, clean validation log evidence, and no Windows Vulkan, Android Vulkan, or compile-only inference.'
    }
    elseif ($RecipeName -eq 'environment-platform-android-vulkan-package') {
        return Get-EnvironmentPlatformVulkanHostPlan `
            -Recipe $RecipeName `
            -ScriptPath 'tools/validate-android-vulkan-runtime-host.ps1' `
            -HostGate 'android-vulkan-runtime-host' `
            -AdditionalScriptArguments @(
                '-ValidationLayerJniLibs',
                'artifacts/environment/android/validation-layers/jniLibs'
            ) `
            -ExpectedEvidenceCounters @(
                'validation_recipe=environment-platform-android-vulkan-package',
                'host_has_android_sdk=1',
                'host_has_android_ndk=1',
                'adb_device_or_emulator_ready=1',
                'android_vulkan_profile_ready=1',
                'android_gpu_debuggable_ready=1',
                'android_validation_layer_jni_libs_ready=1',
                'android_validation_layer_apk_packaged=1',
                'VK_LAYER_KHRONOS_validation_ready=1',
                'android_package_smoke_ready=1',
                'android_vulkan_readback_ready=1',
                'android_vulkan_validation_layer_enumerated=1',
                'android_vulkan_validation_log_clean=1',
                'environment_platform_android_vulkan_ready=1',
                'environment_platform_requires_android_vulkan_host_evidence=0',
                'environment_all_platform_unconditional_ready=0'
            ) `
            -Message 'Android Vulkan platform validation requires Android SDK, NDK, adb device or emulator evidence, manifest Vulkan version/level feature declarations, Android debug-build instrumentation, host-supplied validation layer jniLibs, APK-packaged VK_LAYER_KHRONOS_validation evidence, same-launch VK_LAYER_KHRONOS_validation enumeration, clean validation logcat output, Android package smoke, Android Vulkan readback evidence, and no desktop Vulkan or Linux Vulkan inference.'
    }
    elseif ($RecipeName -eq 'environment-platform-ios-metal-package') {
        return Get-EnvironmentPlatformVulkanHostPlan `
            -Recipe $RecipeName `
            -ScriptPath 'tools/validate-apple-metal-platform-host.ps1' `
            -HostGate 'ios-metal-host' `
            -AdditionalScriptArguments @('-Platform', 'ios') `
            -ExpectedEvidenceCounters @(
                'validation_recipe=environment-platform-ios-metal-package',
                'host=macos',
                'xcode_ios_sdk_ready=1',
                'ios_simulator_or_device_ready=1',
                'ios_metal_feature_set_checked=1',
                'ios_package_smoke_ready=1',
                'ios_metal_command_queue_ready=1',
                'ios_metal_pipeline_ready=1',
                'ios_metal_command_buffer_ready=1',
                'ios_metal_readback_ready=1',
                'environment_platform_ios_metal_ready=1',
                'environment_platform_requires_ios_metal_host_evidence=0',
                'environment_all_platform_unconditional_ready=0'
            ) `
            -Message 'iOS Metal platform validation requires a macOS host with full Xcode, iOS Simulator SDK/runtime or device evidence, package smoke that reads an app-written Metal evidence file, Metal feature-set check, command queue, compute pipeline, command buffer, readback evidence, and no macOS-to-iOS or desktop Metal inference.'
    }
    elseif ($RecipeName -eq 'environment-backend-parity-v2-closeout') {
        $pwEntry = Get-PwshScriptCommandPlan `
            -ScriptPath 'tools/validate-environment-backend-parity-v2.ps1' `
            -ScriptArguments @('-RequireReady')
        $diagEnvironmentBackendParityV2 = New-RunnerDiagnostic -Severity 'info' -Code 'local-value-validation' -Message 'Environment backend parity v2 closeout builds and runs the MK_renderer value-only v2 parity tests. It expects a 15-feature by 3-backend matrix, 45 backend-local ready rows, zero host-gated/blocked/unsupported rows, zero native-handle/GPU-command side effects, zero D3D12/Vulkan/Metal inference counters, environment_backend_parity_ready=1, and no all-platform, commercial, broad optimization, or broad environment_ready promotion.' -ValidationRecipe $RecipeName
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @($diagEnvironmentBackendParityV2)
    }
    elseif ($RecipeName -eq 'environment-broad-optimization-cross-backend-measurement') {
        $expectedCounters = @(
            'validation_recipe=environment-broad-optimization-cross-backend-measurement',
            'environment_optimization_measurement_status=ready',
            'environment_optimization_measurement_ready=1',
            'environment_optimization_measurement_workload_rows=21',
            'environment_optimization_measurement_backend_rows=3',
            'environment_optimization_measurement_before_after_pairs=21',
            'environment_optimization_measurement_profiler_artifacts=21',
            'environment_optimization_measurement_trace_event_json=21',
            'environment_optimization_measurement_missing_artifacts=0',
            'environment_optimization_measurement_invalid_hashes=0',
            'environment_optimization_measurement_path_escapes=0',
            'environment_optimization_measurement_over_budget=0',
            'environment_broad_optimization_ready=1',
            'environment_ready=0',
            'environment_commercial_ready=0'
        )
        $scriptArguments = @('-RequireReady', '-ExpectedEvidenceCounters') + $expectedCounters
        $pwEntry = Get-PwshScriptCommandPlan `
            -ScriptPath 'tools/validate-environment-optimization-artifacts.ps1' `
            -ScriptArguments $scriptArguments
        $diagEnvironmentOptimizationArtifacts = New-RunnerDiagnostic -Severity 'info' -Code 'host-evidence-required' -Message 'Environment broad optimization cross-backend measurement validates retained official profiler and trace artifacts under artifacts/environment/optimization/<task-id>/<backend>/<workload>/ for seven workloads across d3d12, vulkan_strict, and metal_apple_host. It requires 21 before/after CPU/GPU/memory/upload/barrier/shader-cache/stutter rows, GPU timestamp frequency rows, SHA-256 verified profiler artifacts, trace-event JSON files, zero escaped paths, zero invalid hashes, zero missing artifacts, zero over-budget rows, and keeps broad environment_ready and commercial readiness at 0.' -ValidationRecipe $RecipeName -HostGate 'environment-optimization-artifact-host'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('environment-optimization-artifact-host') -RequiredAcknowledgements @('environment-optimization-artifact-host') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12', 'Vulkan') -Diagnostics @($diagEnvironmentOptimizationArtifacts)
    }
    elseif ($RecipeName -eq 'environment-metal-host-optimization-artifact-producer') {
        $expectedCounters = @(
            'validation_recipe=environment-metal-host-optimization-artifact-producer',
            'host=macos',
            'host_gate=metal-apple',
            'environment_metal_host_optimization_artifact_status=ready',
            'environment_metal_host_optimization_artifact_ready=1',
            'xcrun_xctrace_ready=1',
            'xctrace_template=Metal_System_Trace',
            'environment_metal_host_optimization_artifacts_written=7',
            'environment_metal_host_optimization_profiler_artifacts=7',
            'environment_metal_host_optimization_trace_event_json=7',
            'environment_optimization_measurement_missing_artifacts=0',
            'environment_broad_optimization_ready=1',
            'environment_ready=0',
            'environment_commercial_ready=0'
        )
        $scriptArguments = @('-RequireReady', '-ExpectedEvidenceCounters') + $expectedCounters
        $pwEntry = Get-PwshScriptCommandPlan `
            -ScriptPath 'tools/generate-environment-metal-optimization-artifacts.ps1' `
            -ScriptArguments $scriptArguments
        $diagEnvironmentMetalOptimizationProducer = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Environment Metal host optimization artifact producer requires macOS with full Xcode, xcrun, and xctrace Metal System Trace. It profiles validate-environment-metal-host-aggregate.ps1, writes seven metal_apple_host evidence.json / trace-events.json / xctrace TOC profiler artifacts under artifacts/environment/optimization/2026-06-19-metal-host-xctrace-smoke/metal_apple_host/<workload>/, then runs the fail-closed cross-backend optimization artifact validator before broad optimization can become ready. It does not infer Linux Vulkan, Android Vulkan, all-platform readiness, commercial readiness, or broad environment_ready.' -ValidationRecipe $RecipeName -HostGate 'metal-apple'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('metal-apple') -RequiredAcknowledgements @('metal-apple') -AllowedGameTargets @() -AllowedStrictBackend @() -Diagnostics @($diagEnvironmentMetalOptimizationProducer)
    }
    elseif ($RecipeName -eq 'environment-asset-pipeline-openexr-ktx-basis-full') {
        $expectedCounters = @(
            'validation_recipe=environment-asset-pipeline-openexr-ktx-basis-full',
            'environment_asset_pipeline_openexr_ktx_basis_full_status=ready',
            'environment_asset_pipeline_openexr_ktx_basis_full_ready=1',
            'environment_asset_pipeline_required_rows=14',
            'environment_asset_pipeline_ready_rows=14',
            'environment_asset_pipeline_openexr_rows=5',
            'environment_asset_pipeline_ktx2_basis_rows=4',
            'environment_asset_pipeline_backend_target_rows=4',
            'environment_asset_pipeline_runtime_rows=1',
            'environment_asset_pipeline_dependency_gated_rows=0',
            'environment_asset_pipeline_package_visible_rows=14',
            'environment_asset_pipeline_host_validated_rows=14',
            'environment_asset_pipeline_source_artifact_rows=14',
            'environment_asset_pipeline_cooked_artifact_rows=14',
            'environment_asset_pipeline_package_counter_rows=14',
            'environment_asset_pipeline_replay_hash_rows=14',
            'environment_asset_pipeline_rejection_diagnostic_rows=1',
            'openexr_scanline_rgba16f_ready=1',
            'openexr_tiled_rgba16f_ready=1',
            'openexr_multipart_ready=1',
            'openexr_metadata_preservation_ready=1',
            'openexr_deep_image_rejected_with_diagnostic=1',
            'ktx2_basis_etc1s_transcode_ready=1',
            'ktx2_basis_uastc_transcode_ready=1',
            'ktx2_mip_level_validation_ready=1',
            'ktx2_color_space_metadata_ready=1',
            'd3d12_bc7_target_ready=1',
            'vulkan_bc7_target_ready=1',
            'metal_astc_target_ready=1',
            'android_vulkan_astc_target_ready=1',
            'runtime_cooked_only_ingest_ready=1',
            'runtime_source_parsing=0',
            'environment_asset_pipeline_runtime_source_parsing=0',
            'environment_asset_pipeline_runtime_optional_codec_execution=0',
            'environment_asset_pipeline_native_handle_access=0',
            'environment_asset_pipeline_gpu_command_executed=0',
            'environment_asset_pipeline_package_command_executed=0',
            'environment_asset_pipeline_cmake_configure_dependency_install=0',
            'environment_asset_pipeline_optional_dependency_feature=asset-importers',
            'environment_asset_pipeline_openexr_dependency_recorded=1',
            'environment_asset_pipeline_ktx_dependency_recorded=1',
            'environment_ready=0',
            'environment_commercial_ready=0'
        )
        $scriptArguments = @('-RequireReady', '-ExpectedEvidenceCounters') + $expectedCounters
        $pwEntry = Get-PwshScriptCommandPlan `
            -ScriptPath 'tools/validate-environment-asset-pipeline-full.ps1' `
            -ScriptArguments $scriptArguments
        $diagEnvironmentAssetPipelineFull = New-RunnerDiagnostic -Severity 'info' -Code 'local-value-validation' -Message 'Environment full OpenEXR/KTX2/Basis asset-pipeline validation builds and runs MK_environment_texture_pipeline_v2_tests, validates the optional asset-importers build lane, and requires 14 package-visible host-validated rows for OpenEXR scanline/tiled/multipart/metadata/deep rejection, KTX2/Basis ETC1S and UASTC transcode, mip and color metadata, D3D12/Vulkan BC7, Metal/Android Vulkan ASTC, and runtime cooked-only ingest. It keeps runtime source parsing, runtime optional codec execution, native-handle access, GPU command execution, package command execution, broad environment_ready, and commercial readiness at 0.' -ValidationRecipe $RecipeName
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12', 'Vulkan') -Diagnostics @($diagEnvironmentAssetPipelineFull)
    }
    elseif ($RecipeName -eq 'environment-aaa-preset-asset-library-production') {
        $expectedCounters = @(
            'validation_recipe=environment-aaa-preset-asset-library-production',
            'environment_aaa_preset_asset_library_status=ready',
            'environment_aaa_preset_asset_library_ready=1',
            'environment_aaa_preset_asset_library_asset_rows=156',
            'environment_aaa_preset_asset_library_sky_atmosphere_presets=24',
            'environment_aaa_preset_asset_library_volumetric_cloud_presets=24',
            'environment_aaa_preset_asset_library_fog_volume_presets=16',
            'environment_aaa_preset_asset_library_rain_presets=12',
            'environment_aaa_preset_asset_library_snow_presets=12',
            'environment_aaa_preset_asset_library_wind_presets=12',
            'environment_aaa_preset_asset_library_material_weathering_presets=24',
            'environment_aaa_preset_asset_library_lighting_ibl_presets=12',
            'environment_aaa_preset_asset_library_weather_timeline_presets=12',
            'environment_aaa_preset_asset_library_biome_environment_presets=8',
            'environment_aaa_preset_asset_library_preview_screenshot_rows=144',
            'environment_aaa_preset_asset_library_sample_scene_consumption_rows=8',
            'environment_preset_asset_license_provenance_rows=156',
            'environment_preset_asset_package_budget_rows=156',
            'environment_preset_asset_license_missing_rows=0',
            'environment_preset_asset_package_budget_overages=0',
            'environment_preset_asset_external_asset_rows=0',
            'environment_aaa_preset_asset_library_missing_objective_rows=0',
            'environment_aaa_preset_asset_library_backend_execution=0',
            'environment_aaa_preset_asset_library_package_script_execution=0',
            'environment_aaa_preset_asset_library_native_handle_access=0',
            'environment_ready=0',
            'environment_commercial_ready=0'
        )
        $scriptArguments = @('-RequireReady', '-ExpectedEvidenceCounters') + $expectedCounters
        $pwEntry = Get-PwshScriptCommandPlan `
            -ScriptPath 'tools/validate-environment-aaa-preset-asset-library.ps1' `
            -ScriptArguments $scriptArguments
        $diagEnvironmentPresetAssetLibrary = New-RunnerDiagnostic -Severity 'info' -Code 'local-value-validation' -Message 'Environment AAA preset asset library production validation uses deterministic first-party objective rows: 156 preset assets across ten categories, 144 preview screenshot rows, 8 sample-scene consumption rows, complete license/provenance and package-budget rows, zero external assets, zero missing rows, zero native-handle access, and no broad environment_ready or commercial aggregate promotion.' -ValidationRecipe $RecipeName
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12', 'Vulkan') -Diagnostics @($diagEnvironmentPresetAssetLibrary)
    }
    elseif ($RecipeName -eq 'environment-physical-weather-simulation-closeout') {
        $expectedCounters = @(
            'validation_recipe=environment-physical-weather-simulation-closeout',
            'environment_physical_weather_simulation_status=ready',
            'environment_physical_weather_simulation_ready=1',
            'environment_weather_simulation_cpu_reference_solver_ready=1',
            'environment_weather_simulation_production_solver_ready=1',
            'environment_weather_simulation_backend_parity_ready=1',
            'environment_weather_simulation_physical_weather_ready=1',
            'environment_weather_simulation_d3d12_gpu_solver_ready=1',
            'environment_weather_simulation_vulkan_gpu_solver_ready=1',
            'environment_weather_simulation_metal_gpu_solver_ready=1',
            'environment_weather_simulation_coupled_field_rows=13',
            'environment_weather_simulation_required_field_rows=13',
            'environment_weather_simulation_canonical_dataset_rows=12',
            'environment_weather_simulation_dataset_provenance_rows=12',
            'environment_weather_simulation_cf_netcdf_or_grib_or_synthetic_rows=12',
            'environment_weather_simulation_canonical_image_rows=12',
            'environment_weather_simulation_backend_solver_rows=3',
            'environment_weather_simulation_host_validated_backend_rows=3',
            'environment_weather_simulation_compute_dispatch_rows=3',
            'environment_weather_simulation_synchronization_rows=3',
            'environment_weather_simulation_readback_rows=3',
            'environment_weather_simulation_mass_conservation_relative_error_max=0.001',
            'environment_weather_simulation_energy_or_stability_error_max=0.002',
            'environment_weather_simulation_negative_density_cells=0',
            'environment_weather_simulation_nan_or_inf_cells=0',
            'environment_weather_simulation_solver_budget_overages=0',
            'environment_weather_simulation_visual_regression_failures=0',
            'environment_weather_simulation_validation_failures=0',
            'environment_weather_simulation_backend_inference=0',
            'environment_weather_simulation_native_handle_access=0',
            'environment_weather_simulation_package_visible_rows=41',
            'environment_ready=0',
            'environment_commercial_ready=0'
        )
        $scriptArguments = @('-RequireReady', '-ExpectedEvidenceCounters') + $expectedCounters
        $pwEntry = Get-PwshScriptCommandPlan `
            -ScriptPath 'tools/validate-environment-weather-physics.ps1' `
            -ScriptArguments $scriptArguments
        $diagEnvironmentWeatherPhysics = New-RunnerDiagnostic -Severity 'info' -Code 'local-value-validation' -Message 'Environment physical weather simulation closeout builds and runs the CPU closeout plus D3D12, strict Vulkan, and Metal weather solver tests. It requires 13 coupled physical fields, 12 canonical validation datasets with synthetic/CF-netCDF/GRIB provenance, 12 canonical validation images, D3D12/Vulkan/Metal host-validated compute dispatch/synchronization/readback rows, backend parity, mass conservation <=0.005, energy/stability <=0.010, zero negative-density cells, zero NaN/Inf cells, zero budget overages, zero visual regression failures, zero backend inference, zero native-handle access, and keeps broad environment_ready and commercial readiness at 0.' -ValidationRecipe $RecipeName
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12', 'Vulkan') -Diagnostics @($diagEnvironmentWeatherPhysics)
    }
    elseif ($RecipeName -eq 'environment-artist-workflow-production-closeout') {
        $expectedCounters = @(
            'validation_recipe=environment-artist-workflow-production-closeout',
            'environment_artist_workflow_production_status=ready',
            'environment_artist_workflow_production_ready=1',
            'workflow_import_openexr_ready=1',
            'workflow_import_ktx2_basis_ready=1',
            'workflow_import_gltf_material_ready=1',
            'workflow_review_usd_materialx_ocio_ready=1',
            'workflow_cook_package_ready=1',
            'workflow_live_preview_d3d12_ready=1',
            'workflow_live_preview_vulkan_ready=1',
            'workflow_live_preview_metal_host_ready=1',
            'workflow_weather_timeline_edit_ready=1',
            'workflow_preset_batch_apply_ready=1',
            'workflow_validation_report_ready=1',
            'workflow_profiler_artifact_review_ready=1',
            'workflow_undo_redo_revision_safety_ready=1',
            'workflow_operator_review_ready=1',
            'environment_artist_workflow_production_requirement_rows=14',
            'environment_artist_workflow_production_ready_rows=14',
            'environment_artist_workflow_editor_core_backend_execution=0',
            'environment_artist_workflow_editor_core_package_script_execution=0',
            'environment_artist_workflow_editor_core_validation_recipe_execution=0',
            'environment_artist_workflow_native_handle_access=0',
            'environment_ready=0',
            'environment_commercial_ready=0'
        )
        $scriptArguments = @('-RequireReady', '-ExpectedEvidenceCounters') + $expectedCounters
        $pwEntry = Get-PwshScriptCommandPlan `
            -ScriptPath 'tools/validate-environment-artist-workflow-production.ps1' `
            -ScriptArguments $scriptArguments
        $diagEnvironmentArtistWorkflowProduction = New-RunnerDiagnostic -Severity 'info' -Code 'local-value-validation' -Message 'Environment production artist workflow closeout builds and runs MK_editor_environment_tests, then packages sample_desktop_runtime_game with the reviewed artist workflow package smoke. It requires 14 package-visible workflow rows covering OpenEXR import, KTX2/Basis import, glTF material import, USD/MaterialX/OCIO review, package cooking, D3D12/Vulkan/Metal-host live preview evidence, weather timeline editing, preset batch apply, validation report, profiler artifact review, undo/redo revision safety, and operator review. It keeps editor-core backend, package-script, validation-recipe, native-handle side effects, broad environment_ready, and commercial readiness at 0.' -ValidationRecipe $RecipeName
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @() -RequiredAcknowledgements @() -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'D3D12', 'Vulkan') -Diagnostics @($diagEnvironmentArtistWorkflowProduction)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-commercial-vulkan-evidence') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentCommercialVulkanEvidenceSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireVulkanShaders -SmokeArgs $smokeTail
        $diagEnvironmentCommercialVulkan = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Environment commercial Vulkan evidence validation runs the selected strict Vulkan aggregate and feeds only the strict_vulkan_aggregate and platform_windows_vulkan commercial dependency rows as ready evidence. It must emit environment_commercial_ready=0, required_rows=14, ready_rows=3, host_gated_rows=5, blocked_rows=6, strict Vulkan commercial evidence counters all 1, zero native-handle/broad-environment claims, and no Metal, Linux, Android, D3D12 commercial rows, backend-parity, broad-optimization, complete physical-weather, or broad environment_ready promotion.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('vulkan-strict', 'commercial-environment-closeout') -RequiredAcknowledgements @('vulkan-strict') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'Vulkan') -Diagnostics @($diagEnvironmentCommercialVulkan)
    }
    elseif ($RecipeName -eq 'desktop-runtime-sample-game-environment-weather-simulation-vulkan-solver-package') {
        $target = if ([string]::IsNullOrWhiteSpace($SelectedGameTarget)) { 'sample_desktop_runtime_game' } else { $SelectedGameTarget }
        $smokeTail = @(Get-SampleDesktopRuntimeGameEnvironmentWeatherSimulationVulkanSolverPackageSmokeArgs)
        $pwEntry = Get-DesktopRuntimePackageCommandPlan -ScriptPath $packageScript -GameTarget $target -RequireVulkanShaders -SmokeArgs $smokeTail
        $diagEnvironmentWeatherSimulationVulkan = New-RunnerDiagnostic -Severity 'info' -Code 'host-gate-acknowledged' -Message 'Strict Vulkan environment weather simulation solver package validation requires local Vulkan runtime, DXC SPIR-V CodeGen, and spirv-val readiness. It proves explicit cs_environment_weather SPIR-V execution through descriptor-set binding count, compute dispatch count, sync2 barrier count, readback hash, elapsed/budget counters, and zero native-handle/backend-parity/D3D12-inference/Metal-inference claims while keeping production solver ready, physical weather ready, all-platform readiness, and broad environment_ready false.' -ValidationRecipe $RecipeName -HostGate 'vulkan-strict'
        return New-RecipePlanRow -Recipe $RecipeName -CommandPlan @($pwEntry) -HostGates @('vulkan-strict') -RequiredAcknowledgements @('vulkan-strict') -AllowedGameTargets @('sample_desktop_runtime_game') -AllowedStrictBackend @('', 'Vulkan') -Diagnostics @($diagEnvironmentWeatherSimulationVulkan)
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
