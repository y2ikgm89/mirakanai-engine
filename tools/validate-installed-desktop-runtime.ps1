#requires -Version 7.0
#requires -PSEdition Core

param(
    [string]$InstallPrefix = "",
    [string]$BuildConfig = "Release",
    [string]$GameTarget = "",
    [string[]]$SmokeArgs = @(),
    [switch]$RequireD3d12Shaders,
    [switch]$RequireVulkanShaders
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$tools = Assert-CppBuildTools

if ([string]::IsNullOrWhiteSpace($InstallPrefix)) {
    $InstallPrefix = Join-Path $root "out/install/desktop-runtime-release"
}
$installPrefixPath = [System.IO.Path]::GetFullPath($InstallPrefix)
$installedBin = Join-Path $installPrefixPath "bin"
$installedMetadataPath = Join-Path $installPrefixPath "share/Mirakanai/desktop-runtime-games.json"
$gameMetadata = $null
if (Test-Path -LiteralPath $installedMetadataPath -PathType Leaf) {
    $metadata = Read-DesktopRuntimeGameMetadata -Path $installedMetadataPath
    if ([string]::IsNullOrWhiteSpace($metadata.selectedPackageTarget)) {
        Write-Error "Installed desktop runtime metadata must declare selectedPackageTarget: $installedMetadataPath"
    }
    if ([string]::IsNullOrWhiteSpace($GameTarget)) {
        $GameTarget = $metadata.selectedPackageTarget
    } elseif ($metadata.selectedPackageTarget -ne $GameTarget) {
        Write-Error "Installed desktop runtime metadata selected '$($metadata.selectedPackageTarget)' but validation requested '$GameTarget'."
    }
    $gameMetadata = Get-DesktopRuntimeGameMetadata -Metadata $metadata -GameTarget $GameTarget
    Assert-DesktopRuntimeGameManifestContract `
        -GameMetadata $gameMetadata `
        -Root $installPrefixPath `
        -RequiredPackagingTargets @("desktop-game-runtime", "desktop-runtime-release") `
        -Installed | Out-Null
    Assert-DesktopRuntimeGamePackageFiles `
        -GameMetadata $gameMetadata `
        -Root $installPrefixPath `
        -Installed | Out-Null
}
if ([string]::IsNullOrWhiteSpace($GameTarget)) {
    Write-Error "GameTarget is required when installed desktop runtime metadata is unavailable."
}
if ($SmokeArgs.Count -eq 0 -and $null -ne $gameMetadata) {
    $SmokeArgs = @($gameMetadata.packageSmokeArgs)
}
if ($SmokeArgs.Count -eq 0) {
    Write-Error "SmokeArgs are required when installed desktop runtime metadata is unavailable or incomplete."
}
$requireD3d12ShaderArtifacts = $RequireD3d12Shaders.IsPresent -or ($null -ne $gameMetadata -and [bool]$gameMetadata.requiresD3d12Shaders)
$requireVulkanShaderArtifacts = $RequireVulkanShaders.IsPresent -or ($null -ne $gameMetadata -and $gameMetadata.PSObject.Properties.Name.Contains("requiresVulkanShaders") -and [bool]$gameMetadata.requiresVulkanShaders)
$runtimeExecutableName = $GameTarget
if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
    $runtimeExecutableName = "$GameTarget.exe"
}
$runtimeExecutable = Join-Path $installedBin $runtimeExecutableName
$sdlDll = Join-Path $installedBin "SDL3.dll"

function Invoke-InstalledRuntimeSmoke {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [string[]]$Arguments = @()
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.WorkingDirectory = (Get-Location).Path
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in $Arguments) {
        $startInfo.ArgumentList.Add($argument) | Out-Null
    }
    $startInfo.Environment.Clear()
    foreach ($entry in Get-NormalizedProcessEnvironment) {
        $startInfo.Environment[$entry.Key] = $entry.Value
    }

    $process = [System.Diagnostics.Process]::Start($startInfo)
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()
    if (-not [string]::IsNullOrWhiteSpace($stdout)) {
        Write-Host $stdout.TrimEnd()
    }
    if (-not [string]::IsNullOrWhiteSpace($stderr)) {
        Write-Host $stderr.TrimEnd()
    }
    if ($process.ExitCode -ne 0) {
        Write-Error "Command failed with exit code $($process.ExitCode): $FilePath $($Arguments -join ' ')"
    }
    return $stdout
}

if (-not (Test-Path -LiteralPath $runtimeExecutable -PathType Leaf)) {
    Write-Error "Installed desktop runtime game was not found: $runtimeExecutable"
}

if (Test-Path -LiteralPath $sdlDll -PathType Leaf) {
    Write-Error "Installed desktop runtime package must not ship SDL3 runtime DLL: $sdlDll"
}

if ($requireD3d12ShaderArtifacts) {
    $shaderArtifacts = @()
    if ($null -ne $gameMetadata -and $gameMetadata.PSObject.Properties.Name.Contains("d3d12ShaderArtifacts")) {
        $shaderArtifacts = @($gameMetadata.d3d12ShaderArtifacts | ForEach-Object { [string]$_ })
    }
    if ($shaderArtifacts.Count -eq 0) {
        $shaderArtifacts = @("shaders/runtime_shell.vs.dxil", "shaders/runtime_shell.ps.dxil")
    }

    foreach ($shaderArtifact in $shaderArtifacts) {
        $normalizedShaderArtifact = ConvertTo-DesktopRuntimeMetadataRelativePath `
            -Path $shaderArtifact `
            -Description "Installed desktop runtime D3D12 shader artifact"
        if (-not $normalizedShaderArtifact.StartsWith("shaders/", [System.StringComparison]::Ordinal)) {
            Write-Error "Installed desktop runtime D3D12 shader artifact must stay under shaders/: $normalizedShaderArtifact"
        }
        $shaderPath = Join-Path $installedBin $normalizedShaderArtifact.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
        if (-not (Test-Path -LiteralPath $shaderPath -PathType Leaf)) {
            Write-Error "Installed desktop runtime D3D12 shader artifact was not found: $shaderPath"
        }
        if ((Get-Item -LiteralPath $shaderPath).Length -le 0) {
            Write-Error "Installed desktop runtime D3D12 shader artifact is empty: $shaderPath"
        }
    }
}

if ($requireVulkanShaderArtifacts) {
    $shaderArtifacts = @()
    if ($null -ne $gameMetadata -and $gameMetadata.PSObject.Properties.Name.Contains("vulkanShaderArtifacts")) {
        $shaderArtifacts = @($gameMetadata.vulkanShaderArtifacts | ForEach-Object { [string]$_ })
    }
    if ($shaderArtifacts.Count -eq 0) {
        $shaderArtifacts = @("shaders/runtime_shell.vs.spv", "shaders/runtime_shell.ps.spv")
    }

    foreach ($shaderArtifact in $shaderArtifacts) {
        $normalizedShaderArtifact = ConvertTo-DesktopRuntimeMetadataRelativePath `
            -Path $shaderArtifact `
            -Description "Installed desktop runtime Vulkan shader artifact"
        if (-not $normalizedShaderArtifact.StartsWith("shaders/", [System.StringComparison]::Ordinal)) {
            Write-Error "Installed desktop runtime Vulkan shader artifact must stay under shaders/: $normalizedShaderArtifact"
        }
        $shaderPath = Join-Path $installedBin $normalizedShaderArtifact.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
        if (-not (Test-Path -LiteralPath $shaderPath -PathType Leaf)) {
            Write-Error "Installed desktop runtime Vulkan shader artifact was not found: $shaderPath"
        }
        if ((Get-Item -LiteralPath $shaderPath).Length -le 0) {
            Write-Error "Installed desktop runtime Vulkan shader artifact is empty: $shaderPath"
        }
    }
}

$env:PATH = (($installedBin, $env:PATH) -join [System.IO.Path]::PathSeparator)
$smokeOutput = Invoke-InstalledRuntimeSmoke -FilePath $runtimeExecutable -Arguments @($SmokeArgs)
$requiresPostprocess = @($SmokeArgs) -contains "--require-postprocess"
$requiresPostprocessDepthInput = @($SmokeArgs) -contains "--require-postprocess-depth-input"
$requiresDirectionalShadow = @($SmokeArgs) -contains "--require-directional-shadow"
$requiresDirectionalShadowFiltering = @($SmokeArgs) -contains "--require-directional-shadow-filtering"
$requiresD3d12ShadowCascadePolicy = @($SmokeArgs) -contains "--require-d3d12-shadow-cascade-policy"
$requiresVulkanShadowCascadePolicy = @($SmokeArgs) -contains "--require-vulkan-shadow-cascade-policy"
$requiresLightingShadowPolicy = @($SmokeArgs) -contains "--require-lighting-shadow-policy"
$requiresSceneScalePolicy = @($SmokeArgs) -contains "--require-scene-scale-policy"
$requiresD3d12InstancedDrawEvidence = @($SmokeArgs) -contains "--require-d3d12-instanced-draw-evidence"
$requiresVulkanInstancedDrawEvidence = @($SmokeArgs) -contains "--require-vulkan-instanced-draw-evidence"
$requiresD3d12PostprocessEvidence = @($SmokeArgs) -contains "--require-d3d12-postprocess-evidence"
$requiresVulkanPostprocessEvidence = @($SmokeArgs) -contains "--require-vulkan-postprocess-evidence"
$requiresGpuMemoryPolicy = @($SmokeArgs) -contains "--require-gpu-memory-policy"
$requiresMemoryDiagnostics = @($SmokeArgs) -contains "--require-memory-diagnostics"
$requiresD3d12GpuMemoryEvidence = @($SmokeArgs) -contains "--require-d3d12-gpu-memory-evidence"
$requiresVulkanGpuMemoryEvidence = @($SmokeArgs) -contains "--require-vulkan-gpu-memory-evidence"
$requiresDebugProfilingPolicy = @($SmokeArgs) -contains "--require-debug-profiling-policy"
$requiresD3d12DebugProfilingEvidence = @($SmokeArgs) -contains "--require-d3d12-debug-profiling-evidence"
$requiresVulkanDebugProfilingEvidence = @($SmokeArgs) -contains "--require-vulkan-debug-profiling-evidence"
$requiresJobSchedulingEvidence = @($SmokeArgs) -contains "--require-job-scheduling-evidence"
$requiresJobExecutionFoundation = @($SmokeArgs) -contains "--require-job-execution-foundation"
$requiresJobExecutionTopologyPolicy = @($SmokeArgs) -contains "--require-job-execution-topology-policy"
if ($requiresD3d12InstancedDrawEvidence -or $requiresVulkanInstancedDrawEvidence) {
    $requiresSceneScalePolicy = $true
}
if ($requiresD3d12GpuMemoryEvidence -or $requiresVulkanGpuMemoryEvidence) {
    $requiresGpuMemoryPolicy = $true
}
if ($requiresMemoryDiagnostics) {
    $requiresGpuMemoryPolicy = $true
}
if ($requiresD3d12DebugProfilingEvidence -or $requiresVulkanDebugProfilingEvidence) {
    $requiresDebugProfilingPolicy = $true
}
$requiresShadowMorphComposition = @($SmokeArgs) -contains "--require-shadow-morph-composition"
$requiresRendererQualityGates = @($SmokeArgs) -contains "--require-renderer-quality-gates"
$requiresFramegraphMultiqueueEvidence = @($SmokeArgs) -contains "--require-framegraph-multiqueue-evidence"
$requiresVulkanFramegraphMultiqueueEvidence = @($SmokeArgs) -contains "--require-vulkan-framegraph-multiqueue-evidence"
$requiresAnyFramegraphMultiqueueEvidence = $requiresFramegraphMultiqueueEvidence -or $requiresVulkanFramegraphMultiqueueEvidence
$requiresRendererQualityMatrix = @($SmokeArgs) -contains "--require-renderer-quality-matrix"
$requiresRenderingVfxProfiling = @($SmokeArgs) -contains "--require-rendering-vfx-profiling"
$requiresPlayable3dSlice = @($SmokeArgs) -contains "--require-playable-3d-slice"
$requiresVisible3dProductionProof = @($SmokeArgs) -contains "--require-visible-3d-production-proof"
$requiresVulkanVisible3dProductionProof = @($SmokeArgs) -contains "--require-vulkan-visible-3d-production-proof"
$requiresComputeMorphNormalTangent = @($SmokeArgs) -contains "--require-compute-morph-normal-tangent"
$requiresComputeMorphSkin = @($SmokeArgs) -contains "--require-compute-morph-skin"
$requiresComputeMorphAsyncTelemetry = @($SmokeArgs) -contains "--require-compute-morph-async-telemetry"
$requiresComputeMorph = (@($SmokeArgs) -contains "--require-compute-morph") -or $requiresComputeMorphNormalTangent -or $requiresComputeMorphSkin -or $requiresComputeMorphAsyncTelemetry
$requiresNativeUiOverlay = @($SmokeArgs) -contains "--require-native-ui-overlay"
$requiresNativeUiTexturedSpriteAtlas = @($SmokeArgs) -contains "--require-native-ui-textured-sprite-atlas"
$requiresNativeUiTextGlyphAtlas = @($SmokeArgs) -contains "--require-native-ui-text-glyph-atlas"
$requiresGpuSkinning = @($SmokeArgs) -contains "--require-gpu-skinning"
$requiresD3d12GpuSkinningEvidence = @($SmokeArgs) -contains "--require-d3d12-gpu-skinning-evidence"
$requiresVulkanGpuSkinningEvidence = @($SmokeArgs) -contains "--require-vulkan-gpu-skinning-evidence"
if ($requiresD3d12GpuSkinningEvidence) {
    $requiresGpuSkinning = $true
}
if ($requiresVulkanGpuSkinningEvidence) {
    $requiresGpuSkinning = $true
}
if ($requiresRenderingVfxProfiling) {
    $requiresRendererQualityGates = $true
    $requiresRendererQualityMatrix = $true
}
$requiresFrameGraphMultiQueueEvidence = @($SmokeArgs) -contains "--require-framegraph-multiqueue-evidence"
$requiresVulkanFrameGraphMultiQueueEvidence = @($SmokeArgs) -contains "--require-vulkan-framegraph-multiqueue-evidence"
$requiresPackageUploadStaging = @($SmokeArgs) -contains "--require-package-upload-staging"
$requiresKtx2BasisTextureReview = @($SmokeArgs) -contains "--require-ktx2-basis-texture-review"
$requiresGltfSceneImportReview = @($SmokeArgs) -contains "--require-gltf-scene-import-review"
$requiresNative2dSprites = @($SmokeArgs) -contains "--require-native-2d-sprites"
$requiresSpriteAnimation = @($SmokeArgs) -contains "--require-sprite-animation"
$requiresTilemapRuntimeUx = @($SmokeArgs) -contains "--require-tilemap-runtime-ux"
$requiresProductionTileRenderer = @($SmokeArgs) -contains "--require-production-tile-renderer"
$requiresWin32RuntimeHost = @($SmokeArgs) -contains "--require-win32-runtime-host"
$requiresWin32D3d12Presentation = @($SmokeArgs) -contains "--require-win32-d3d12-presentation"
$requiresD3d12Renderer = @($SmokeArgs) -contains "--require-d3d12-renderer"
$requiresGameplaySystems = @($SmokeArgs) -contains "--require-gameplay-systems"
$requiresProceduralGeneration = @($SmokeArgs) -contains "--require-procedural-generation"
$requiresWorldRegionStreaming = @($SmokeArgs) -contains "--require-world-region-streaming"
$requiresSandboxWorldRuntime = @($SmokeArgs) -contains "--require-sandbox-world-runtime"
$requiresSandboxWorldPersistence = @($SmokeArgs) -contains "--require-sandbox-world-persistence"
$requiresSandboxWorldStreaming = @($SmokeArgs) -contains "--require-sandbox-world-streaming"
$requiresEntityScaleCulling = @($SmokeArgs) -contains "--require-entity-scale-culling"
$requiresScriptingSandboxPolicy = @($SmokeArgs) -contains "--require-scripting-sandbox-policy"
$requiresNetworkingFoundationPolicy = @($SmokeArgs) -contains "--require-networking-foundation-policy"
$requiresSimulationOrchestration = @($SmokeArgs) -contains "--require-simulation-orchestration"
$requiresGameplayAuthoringReview = @($SmokeArgs) -contains "--require-gameplay-authoring-review"
$requiresSandboxAuthoringReview = @($SmokeArgs) -contains "--require-sandbox-authoring-review"
$requiresProductionAuthoringWorkflows = @($SmokeArgs) -contains "--require-production-authoring-workflows"
$requiresRuntimeUiWorkbench = @($SmokeArgs) -contains "--require-runtime-ui-workbench"
$requiresRuntimeUiProductionStack = @($SmokeArgs) -contains "--require-runtime-ui-production-stack"
$requiresRuntimeUiRendererAtlasHandoff = @($SmokeArgs) -contains "--require-runtime-ui-renderer-atlas-handoff"
$requiresPackageStreamingSafePoint = @($SmokeArgs) -contains "--require-package-streaming-safe-point"
$requiresSceneCollisionPackage = @($SmokeArgs) -contains "--require-scene-collision-package"
$requiresAudioProduction = @($SmokeArgs) -contains "--require-audio-production"
$requiresWasapiAudio = @($SmokeArgs) -contains "--require-wasapi-audio"
$requiresSandboxPackageBudgets = @($SmokeArgs) -contains "--require-sandbox-package-budgets"
$requiresPerformanceBaseline = @($SmokeArgs) -contains "--require-performance-baseline"
$expectedSmokeFrames = if ($GameTarget -eq "sample_2d_desktop_runtime_package") { 3 } else { 2 }
for ($index = 0; $index -lt ($SmokeArgs.Count - 1); ++$index) {
    if ($SmokeArgs[$index] -eq "--max-frames") {
        $parsedMaxFrames = 0
        if (-not [int]::TryParse($SmokeArgs[$index + 1], [ref]$parsedMaxFrames) -or $parsedMaxFrames -le 0) {
            Write-Error "Installed desktop runtime smoke includes invalid --max-frames value: $($SmokeArgs[$index + 1])"
        }
        $expectedSmokeFrames = $parsedMaxFrames
    }
}
function Get-ExpectedInstalledFramegraphBarrierExecutions {
    param(
        [Parameter(Mandatory = $true)]
        [int]$ExpectedFrames,

        [Parameter(Mandatory = $true)]
        [bool]$RequiresDirectionalShadow,

        [Parameter(Mandatory = $true)]
        [bool]$RequiresPostprocessDepthInput
    )

    if ($ExpectedFrames -le 0) {
        return [int64]0
    }
    if ($RequiresDirectionalShadow) {
        return [int64](9 + (($ExpectedFrames - 1) * 6))
    }
    if ($RequiresPostprocessDepthInput) {
        return [int64](1 + ($ExpectedFrames * 4))
    }
    return [int64]($ExpectedFrames * 2)
}
function Assert-InstalledAudioProductionEvidence {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SmokeOutput,

        [Parameter(Mandatory = $true)]
        [string]$EscapedGameTarget,

        [Parameter(Mandatory = $true)]
        [string]$Context
    )

    foreach ($expected in @{
            "audio_production_status" = "host_evidence_required"
            "audio_production_reviewed" = "1"
            "audio_production_ready" = "0"
            "audio_production_selected_package_ready" = "1"
            "audio_production_package_evidence_ready" = "1"
            "audio_production_decoded_source_rows" = "1"
            "audio_production_streaming_chunk_rows" = "1"
            "audio_production_format_conversion_policy_rows" = "1"
            "audio_production_bus_budget_rows" = "1"
            "audio_production_voice_budget_rows" = "1"
            "audio_production_dsp_graph_rows" = "1"
            "audio_production_listener_rows" = "1"
            "audio_production_spatial_source_rows" = "1"
            "audio_production_hrtf_host_gate_rows" = "1"
            "audio_production_device_lifecycle_rows" = "1"
            "audio_production_device_host_evidence" = "0"
            "audio_production_hrtf_host_evidence" = "0"
            "audio_production_unsupported_claim_rows" = "0"
            "audio_production_invoked_codec_decode" = "0"
            "audio_production_invoked_background_streaming" = "0"
            "audio_production_invoked_middleware" = "0"
            "audio_production_invoked_hrtf" = "0"
            "audio_production_invoked_device_callback" = "0"
            "audio_production_invoked_device_io" = "0"
            "audio_production_diagnostics" = "2"
        }.GetEnumerator()) {
        if ($SmokeOutput -notmatch "(?m)^$EscapedGameTarget status=.*\b$([regex]::Escape($expected.Key))=$([regex]::Escape($expected.Value))\b") {
            Write-Error "Installed $Context smoke status line did not prove audio production field: $($expected.Key)=$($expected.Value)."
        }
    }
    if ($SmokeOutput -notmatch "(?m)^$EscapedGameTarget status=.*\baudio_production_replay_hash=[1-9]\d*\b") {
        Write-Error "Installed $Context smoke status line did not prove positive audio production replay hash."
    }
}
function Assert-InstalledWin32RuntimeHostEvidence {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SmokeOutput,

        [Parameter(Mandatory = $true)]
        [string]$EscapedGameTarget,

        [Parameter(Mandatory = $true)]
        [int]$ExpectedFrames,

        [Parameter(Mandatory = $true)]
        [string]$Context
    )

    foreach ($expected in @{
            "win32_runtime_host_ready" = "1"
            "win32_runtime_host_frames" = [string]$ExpectedFrames
            "win32_runtime_host_game_frames" = [string]$ExpectedFrames
            "win32_runtime_host_diagnostics" = "0"
        }.GetEnumerator()) {
        if ($SmokeOutput -notmatch "(?m)^$EscapedGameTarget status=.*\b$([regex]::Escape($expected.Key))=$([regex]::Escape($expected.Value))\b") {
            Write-Error "Installed $Context smoke status line did not prove Win32 runtime host field: $($expected.Key)=$($expected.Value)."
        }
    }
}
function Assert-InstalledWin32D3d12PresentationEvidence {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SmokeOutput,

        [Parameter(Mandatory = $true)]
        [string]$EscapedGameTarget,

        [Parameter(Mandatory = $true)]
        [string]$Context
    )

    foreach ($expected in @{
            "win32_d3d12_presentation_ready" = "1"
            "win32_d3d12_presentation_selected" = "1"
            "win32_d3d12_presentation_backend" = "d3d12"
            "win32_d3d12_presentation_diagnostics" = "0"
        }.GetEnumerator()) {
        if ($SmokeOutput -notmatch "(?m)^$EscapedGameTarget status=.*\b$([regex]::Escape($expected.Key))=$([regex]::Escape($expected.Value))\b") {
            Write-Error "Installed $Context smoke status line did not prove Win32 D3D12 presentation field: $($expected.Key)=$($expected.Value)."
        }
    }
    if ($SmokeOutput -notmatch "(?m)^$EscapedGameTarget status=.*\bwin32_d3d12_presentation_frames=[1-9]\d*\b") {
        Write-Error "Installed $Context smoke status line did not prove positive Win32 D3D12 presentation frames."
    }
}
function Assert-InstalledWasapiAudioEvidence {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SmokeOutput,

        [Parameter(Mandatory = $true)]
        [string]$EscapedGameTarget,

        [Parameter(Mandatory = $true)]
        [string]$Context
    )

    foreach ($expected in @{
            "wasapi_audio_evidence_ready" = "1"
            "wasapi_audio_device_io_invoked" = "0"
        }.GetEnumerator()) {
        if ($SmokeOutput -notmatch "(?m)^$EscapedGameTarget status=.*\b$([regex]::Escape($expected.Key))=$([regex]::Escape($expected.Value))\b") {
            Write-Error "Installed $Context smoke status line did not prove WASAPI audio field: $($expected.Key)=$($expected.Value)."
        }
    }
}
function Assert-InstalledSandboxPackageBudgetEvidence {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SmokeOutput,

        [Parameter(Mandatory = $true)]
        [string]$EscapedGameTarget,

        [Parameter(Mandatory = $true)]
        [string]$Context
    )

    foreach ($expected in @{
            "sandbox_package_budget_status" = "ready"
            "sandbox_package_budget_rows" = "89"
            "sandbox_package_budget_diagnostics" = "0"
            "sandbox_package_budget_row_limit" = "256"
            "sandbox_package_budget_byte_limit" = "7409"
            "sandbox_package_budget_chunk_bytes" = "7408"
            "sandbox_package_budget_dirty_chunks" = "2"
            "sandbox_package_budget_renderer_draw_rows" = "5"
            "sandbox_package_budget_tile_draw_calls" = "2"
            "sandbox_package_budget_light_rows" = "3"
            "sandbox_package_budget_persisted_chunks" = "2"
            "sandbox_package_budget_streaming_load_rows" = "1"
            "sandbox_package_budget_streaming_unload_rows" = "1"
            "sandbox_package_budget_package_io_invoked" = "0"
            "sandbox_package_budget_native_handles_exposed" = "0"
            "sandbox_package_budget_arbitrary_shell_invoked" = "0"
            "sandbox_package_budget_external_download_invoked" = "0"
            "sandbox_package_budget_source_image_decode_invoked" = "0"
            "sandbox_package_budget_broad_multiplayer_ready" = "0"
            "sandbox_package_budget_backend_submission_invoked" = "0"
            "sandbox_package_budget_native_texture_ownership_invoked" = "0"
            "sandbox_package_budget_renderer_rhi_residency_invoked" = "0"
            "sandbox_package_budget_broad_renderer_quality_ready" = "0"
        }.GetEnumerator()) {
        if ($SmokeOutput -notmatch "(?m)^$EscapedGameTarget status=.*\b$([regex]::Escape($expected.Key))=$([regex]::Escape($expected.Value))\b") {
            Write-Error "Installed $Context smoke status line did not prove sandbox package budget field: $($expected.Key)=$($expected.Value)."
        }
    }
    if ($SmokeOutput -notmatch "(?m)^$EscapedGameTarget status=.*\bsandbox_package_budget_replay_hash=[1-9]\d*\b") {
        Write-Error "Installed $Context smoke status line did not prove positive sandbox package budget replay hash."
    }
}
function Get-ExpectedInstalledPerformanceBaselinePercentileUs {
    param(
        [Parameter(Mandatory = $true)]
        [int]$ExpectedFrames
    )

    if ($ExpectedFrames -le 1) {
        return 15400
    }
    if ($ExpectedFrames -eq 2) {
        return 15800
    }
    return 16000
}
function Assert-InstalledPerformanceBaselineEvidence {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SmokeOutput,

        [Parameter(Mandatory = $true)]
        [string]$EscapedGameTarget,

        [Parameter(Mandatory = $true)]
        [int]$ExpectedFrames,

        [Parameter(Mandatory = $true)]
        [string]$Context
    )

    $expectedP95Us = Get-ExpectedInstalledPerformanceBaselinePercentileUs -ExpectedFrames $ExpectedFrames
    $expectedP99Us = $expectedP95Us
    foreach ($expected in @{
            "performance_baseline_status" = "ready"
            "performance_baseline_backend_scope" = "d3d12"
            "performance_baseline_warmup_frames" = "0"
            "performance_baseline_sample_frames" = [string]$ExpectedFrames
            "performance_baseline_required_sample_frames" = [string]$ExpectedFrames
            "performance_baseline_counter_name" = "sample_2d.frame_time_us"
            "performance_baseline_profile_name" = "sample_2d.frame"
            "performance_baseline_counter_status" = "ready"
            "performance_baseline_profile_status" = "ready"
            "performance_baseline_counter_samples" = [string]$ExpectedFrames
            "performance_baseline_profile_samples" = [string]$ExpectedFrames
            "performance_baseline_frame_p95_us" = [string]$expectedP95Us
            "performance_baseline_frame_p99_us" = [string]$expectedP99Us
            "performance_baseline_profile_p95_us" = [string]$expectedP95Us
            "performance_baseline_profile_p99_us" = [string]$expectedP99Us
            "performance_baseline_frame_budget_p95_us" = "16670"
            "performance_baseline_frame_budget_p99_us" = "16670"
            "performance_baseline_non_finite_samples" = "0"
            "performance_baseline_diagnostics" = "0"
            "performance_baseline_over_budget" = "0"
        }.GetEnumerator()) {
        if ($SmokeOutput -notmatch "(?m)^$EscapedGameTarget status=.*\b$([regex]::Escape($expected.Key))=$([regex]::Escape($expected.Value))\b") {
            Write-Error "Installed $Context smoke status line did not prove performance baseline field: $($expected.Key)=$($expected.Value)."
        }
    }
}
if ($requiresWin32D3d12Presentation) {
    $requiresWin32RuntimeHost = $true
    $requiresD3d12Renderer = $true
}
if ($requiresPerformanceBaseline) {
    $requiresSandboxPackageBudgets = $true
    $requiresWin32RuntimeHost = $true
    $requiresWin32D3d12Presentation = $true
    $requiresD3d12Renderer = $true
}
if ($requiresSandboxWorldPersistence) {
    $requiresSandboxWorldRuntime = $true
    $requiresGameplaySystems = $true
}
if ($requiresSandboxWorldStreaming) {
    $requiresSandboxWorldRuntime = $true
    $requiresGameplaySystems = $true
    $requiresWorldRegionStreaming = $true
}
if ($requiresSandboxWorldRuntime) {
    $requiresGameplaySystems = $true
}
if ($requiresSandboxPackageBudgets) {
    $requiresWin32RuntimeHost = $true
    $requiresWasapiAudio = $true
    $requiresSandboxWorldRuntime = $true
    $requiresSandboxWorldPersistence = $true
    $requiresSandboxWorldStreaming = $true
    $requiresGameplaySystems = $true
    $requiresWorldRegionStreaming = $true
    $requiresProductionTileRenderer = $true
    $requiresSandboxAuthoringReview = $true
    $requiresProductionAuthoringWorkflows = $true
    $requiresRuntimeUiRendererAtlasHandoff = $true
}
if ($requiresPlayable3dSlice) {
    $requiresRendererQualityGates = $true
    $requiresPackageStreamingSafePoint = $true
}
if ($requiresVisible3dProductionProof) {
    $requiresPostprocess = $true
    $requiresPostprocessDepthInput = $true
    $requiresRendererQualityGates = $true
    $requiresPlayable3dSlice = $true
    $requiresPackageStreamingSafePoint = $true
    $requiresNativeUiOverlay = $true
}
if ($requiresSceneCollisionPackage) {
    $requiresPackageStreamingSafePoint = $true
}
if ($requiresShadowMorphComposition) {
    $requiresPostprocess = $true
    $requiresDirectionalShadow = $true
    $requiresDirectionalShadowFiltering = $true
    $requiresPostprocessDepthInput = $true
    $requiresRendererQualityGates = $true
}
if ($requiresPostprocessDepthInput) {
    $requiresPostprocess = $true
}
if ($requiresD3d12PostprocessEvidence) {
    $requiresPostprocess = $true
    $requiresD3d12Renderer = $true
}
if ($requiresVulkanPostprocessEvidence) {
    $requiresPostprocess = $true
}
if ($requiresVulkanShadowCascadePolicy) {
    $requiresDirectionalShadow = $true
    $requiresDirectionalShadowFiltering = $true
}
if ($requiresDirectionalShadowFiltering) {
    $requiresDirectionalShadow = $true
}
if ($requiresDirectionalShadow) {
    $requiresPostprocess = $true
    $requiresPostprocessDepthInput = $true
}
if ($requiresRendererQualityGates) {
    $requiresPostprocess = $true
}
if ($requiresNativeUiTexturedSpriteAtlas) {
    $requiresNativeUiOverlay = $true
}
if ($requiresNativeUiTextGlyphAtlas) {
    $requiresNativeUiOverlay = $true
}
$escapedGameTarget = [regex]::Escape($GameTarget)
$statusLinePattern = "(?m)^$escapedGameTarget status="
if ($smokeOutput -notmatch $statusLinePattern) {
    Write-Error "Installed desktop runtime smoke did not emit the required '$GameTarget status=' report line."
}
if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpresentation_selected=") {
    Write-Error "Installed desktop runtime smoke status line did not include presentation_selected report field."
}
if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpresentation_backend_reports=") {
    Write-Error "Installed desktop runtime smoke status line did not include presentation_backend_reports report field."
}
if ($requiresWin32RuntimeHost) {
    Assert-InstalledWin32RuntimeHostEvidence -SmokeOutput $smokeOutput -EscapedGameTarget $escapedGameTarget -ExpectedFrames $expectedSmokeFrames -Context $GameTarget
}
if ($requiresWin32D3d12Presentation) {
    Assert-InstalledWin32D3d12PresentationEvidence -SmokeOutput $smokeOutput -EscapedGameTarget $escapedGameTarget -Context $GameTarget
}
if ($requiresWasapiAudio) {
    Assert-InstalledWasapiAudioEvidence -SmokeOutput $smokeOutput -EscapedGameTarget $escapedGameTarget -Context $GameTarget
}
if ($requiresAudioProduction -and $GameTarget -ne "sample_2d_desktop_runtime_package") {
    Assert-InstalledAudioProductionEvidence -SmokeOutput $smokeOutput -EscapedGameTarget $escapedGameTarget -Context $GameTarget
}
if ($GameTarget -eq "sample_generated_desktop_runtime_material_shader_package") {
    $expectedVulkanMaterialShaderEvidence = if ($requireVulkanShaderArtifacts) { 1 } else { 0 }
    $expectedShaderGenerationReadyRows = if ($requireVulkanShaderArtifacts) { 4 } else { 2 }
    $expectedShaderGenerationHostGatedRows = if ($requireVulkanShaderArtifacts) { 0 } else { 2 }
    $expectedShaderGenerationSpirvValidationRows = if ($requireVulkanShaderArtifacts) { 2 } else { 0 }
    foreach ($field in @(
            "modern_material_variants",
            "modern_material_ready",
            "modern_material_host_gated",
            "modern_material_unsupported",
            "modern_material_invalid",
            "modern_material_diagnostics",
            "modern_material_texture_dependencies",
            "modern_material_shader_evidence_ready",
            "modern_material_d3d12_shader_evidence_ready",
            "modern_material_vulkan_shader_evidence_ready",
            "modern_material_selected_shader_evidence_ready",
            "material_graph_authoring_targets",
            "material_graph_lowered_materials",
            "material_graph_shader_exports",
            "material_graph_compile_targets",
            "material_graph_compile_requests",
            "material_graph_d3d12_compile_requests",
            "material_graph_vulkan_compile_requests",
            "material_graph_runtime_sources_shipped",
            "material_graph_unsupported_boundaries",
            "material_graph_authoring_diagnostics",
            "shader_generation_cache_reviewed",
            "shader_generation_cache_selected_ready",
            "shader_generation_cache_d3d12_ready",
            "shader_generation_cache_vulkan_ready",
            "shader_generation_cache_rows",
            "shader_generation_cache_ready_rows",
            "shader_generation_cache_host_gated_rows",
            "shader_generation_cache_d3d12_rows",
            "shader_generation_cache_vulkan_rows",
            "shader_generation_cache_spirv_validation_rows",
            "shader_generation_cache_key_rows",
            "shader_generation_cache_provenance_rows",
            "shader_generation_cache_unsupported_claim_rows",
            "shader_generation_cache_diagnostics",
            "shader_generation_cache_live_generation_ready",
            "shader_generation_cache_runtime_compiler_ready",
            "shader_generation_cache_native_cache_handle_ready",
            "shader_generation_cache_renderer_residency_ready",
            "shader_generation_cache_metal_library_ready"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed material/shader package smoke status line did not include modern material field: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bmodern_material_variants=1\b") {
        Write-Error "Installed material/shader package smoke status line did not prove one modern material variant."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bmodern_material_ready=1\b") {
        Write-Error "Installed material/shader package smoke status line did not prove one ready modern material variant."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bmodern_material_diagnostics=0\b") {
        Write-Error "Installed material/shader package smoke status line did not prove clean modern material diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bmodern_material_texture_dependencies=1\b") {
        Write-Error "Installed material/shader package smoke status line did not prove the material texture dependency count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bmodern_material_shader_evidence_ready=1\b") {
        Write-Error "Installed material/shader package smoke status line did not prove shader evidence readiness."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bmodern_material_d3d12_shader_evidence_ready=1\b") {
        Write-Error "Installed material/shader package smoke status line did not prove D3D12 shader evidence readiness."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bmodern_material_vulkan_shader_evidence_ready=$expectedVulkanMaterialShaderEvidence\b") {
        Write-Error "Installed material/shader package smoke status line did not match expected Vulkan shader evidence readiness: $expectedVulkanMaterialShaderEvidence."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bmodern_material_selected_shader_evidence_ready=1\b") {
        Write-Error "Installed material/shader package smoke status line did not prove selected backend shader evidence readiness."
    }
    foreach ($expected in @{
            "material_graph_authoring_targets" = "1"
            "material_graph_lowered_materials" = "1"
            "material_graph_shader_exports" = "1"
            "material_graph_compile_targets" = "2"
            "material_graph_compile_requests" = "4"
            "material_graph_d3d12_compile_requests" = "2"
            "material_graph_vulkan_compile_requests" = "2"
            "material_graph_runtime_sources_shipped" = "0"
            "material_graph_unsupported_boundaries" = "4"
            "material_graph_authoring_diagnostics" = "0"
            "shader_generation_cache_reviewed" = "1"
            "shader_generation_cache_selected_ready" = "1"
            "shader_generation_cache_d3d12_ready" = "1"
            "shader_generation_cache_vulkan_ready" = [string]$expectedVulkanMaterialShaderEvidence
            "shader_generation_cache_rows" = "4"
            "shader_generation_cache_ready_rows" = [string]$expectedShaderGenerationReadyRows
            "shader_generation_cache_host_gated_rows" = [string]$expectedShaderGenerationHostGatedRows
            "shader_generation_cache_d3d12_rows" = "2"
            "shader_generation_cache_vulkan_rows" = "2"
            "shader_generation_cache_spirv_validation_rows" = [string]$expectedShaderGenerationSpirvValidationRows
            "shader_generation_cache_key_rows" = [string]$expectedShaderGenerationReadyRows
            "shader_generation_cache_provenance_rows" = [string]$expectedShaderGenerationReadyRows
            "shader_generation_cache_unsupported_claim_rows" = "0"
            "shader_generation_cache_diagnostics" = "0"
            "shader_generation_cache_live_generation_ready" = "0"
            "shader_generation_cache_runtime_compiler_ready" = "0"
            "shader_generation_cache_native_cache_handle_ready" = "0"
            "shader_generation_cache_renderer_residency_ready" = "0"
            "shader_generation_cache_metal_library_ready" = "0"
        }.GetEnumerator()) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$([regex]::Escape($expected.Key))=$([regex]::Escape($expected.Value))\b") {
            Write-Error "Installed material/shader package smoke status line did not prove $($expected.Key)=$($expected.Value)."
        }
    }
}
if ($GameTarget -eq "sample_desktop_runtime_game") {
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcamera_primary=1\b") {
        Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove a primary 3D camera."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcamera_controller_ticks=\d+\b") {
        Write-Error "Installed sample_desktop_runtime_game smoke status line did not include camera_controller_ticks."
    }
    foreach ($field in @(
            "scene_mesh_plan_meshes",
            "scene_mesh_plan_draws",
            "scene_mesh_plan_unique_meshes",
            "scene_mesh_plan_unique_materials",
            "scene_mesh_plan_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed sample_desktop_runtime_game smoke status line did not include 3D scene mesh plan field: $field"
        }
    }
    foreach ($field in @(
            "scene_mesh_plan_meshes",
            "scene_mesh_plan_draws",
            "scene_mesh_plan_unique_meshes",
            "scene_mesh_plan_unique_materials"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
            Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove 3D scene mesh plan count: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bscene_mesh_plan_diagnostics=0\b") {
        Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove clean 3D scene mesh planning diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bhud_boxes=\d+\b") {
        Write-Error "Installed sample_desktop_runtime_game smoke status line did not include HUD diagnostics."
    }
    if ($requiresNativeUiOverlay) {
        foreach ($field in @("ui_overlay_requested", "ui_overlay_status", "ui_overlay_ready", "ui_overlay_sprites_submitted", "ui_overlay_draws")) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not include native UI overlay field: $field"
            }
        }
    }
    if ($requiresNativeUiTexturedSpriteAtlas) {
        foreach ($field in @("ui_texture_overlay_requested", "ui_texture_overlay_status", "ui_texture_overlay_atlas_ready", "ui_texture_overlay_sprites_submitted", "ui_texture_overlay_texture_binds", "ui_texture_overlay_draws", "ui_atlas_metadata_status", "ui_atlas_metadata_pages", "ui_atlas_metadata_bindings")) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not include textured UI overlay field: $field"
            }
        }
    }
    if ($requiresLightingShadowPolicy) {
        $expectedLightingShadowPolicyFields = @{
            "lighting_shadow_policy_status" = "ready"
            "lighting_shadow_policy_ready" = "1"
            "lighting_shadow_policy_diagnostics" = "0"
            "lighting_shadow_policy_lights" = "1"
            "lighting_shadow_policy_directional_lights" = "1"
            "lighting_shadow_policy_point_lights" = "0"
            "lighting_shadow_policy_spot_lights" = "0"
            "lighting_shadow_policy_shadowed_lights" = "1"
            "lighting_shadow_policy_directional_cascades" = "1"
            "lighting_shadow_policy_shadow_atlas_width" = "1024"
            "lighting_shadow_policy_shadow_atlas_height" = "1024"
            "lighting_shadow_policy_light_rows" = "1"
            "lighting_shadow_policy_ready_frames" = [string]$expectedSmokeFrames
        }
        foreach ($field in $expectedLightingShadowPolicyFields.Keys) {
            $expectedValue = [regex]::Escape($expectedLightingShadowPolicyFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove lighting shadow policy field: $field=$($expectedLightingShadowPolicyFields[$field])"
            }
        }
    }
    if ($requiresSceneScalePolicy) {
        $expectedSceneScaleBackendInstancingEvidence =
            if ($requiresD3d12InstancedDrawEvidence -or $requiresVulkanInstancedDrawEvidence) { "1" } else { "0" }
        $expectedSceneScalePolicyFields = @{
            "scene_scale_policy_status" = "ready"
            "scene_scale_policy_ready" = "1"
            "scene_scale_policy_diagnostics" = "0"
            "scene_scale_policy_scene_resources_ready" = "1"
            "scene_scale_policy_expected_frames" = [string]$expectedSmokeFrames
            "scene_scale_policy_frames_finished" = [string]$expectedSmokeFrames
            "scene_scale_policy_frames_current" = "1"
            "scene_scale_policy_culled_instances" = "0"
            "scene_scale_policy_instanced_draw_calls" = "0"
            "scene_scale_policy_instanced_visible_instances" = "0"
            "scene_scale_policy_lod_groups" = "0"
            "scene_scale_policy_cpu_culling_groups" = "0"
            "scene_scale_policy_backend_instancing_evidence_required" = $expectedSceneScaleBackendInstancingEvidence
            "scene_scale_policy_backend_instancing_evidence_ready" = $expectedSceneScaleBackendInstancingEvidence
            "scene_scale_policy_performance_measurement_required" = "0"
            "scene_scale_policy_performance_measurement_ready" = "0"
        }
        foreach ($field in $expectedSceneScalePolicyFields.Keys) {
            $expectedValue = [regex]::Escape($expectedSceneScalePolicyFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove scene-scale policy field: $field=$($expectedSceneScalePolicyFields[$field])"
            }
        }
        foreach ($field in @(
                "scene_scale_policy_draw_groups",
                "scene_scale_policy_requested_instances",
                "scene_scale_policy_visible_instances",
                "scene_scale_policy_draw_calls"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove positive scene-scale policy field: $field"
            }
        }
        if ($requiresGpuSkinning -and
            $smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bscene_scale_policy_skinned_mesh_groups=1\b") {
            Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove scene-scale skinned mesh grouping."
        }
    }
    if ($requiresD3d12InstancedDrawEvidence) {
        $expectedD3d12InstancedInstances = [string]($expectedSmokeFrames * 3)
        $expectedD3d12InstancedFields = @{
            "d3d12_instanced_draw_execution_status" = "ready"
            "d3d12_instanced_draw_execution_ready" = "1"
            "d3d12_instanced_draw_execution_selected" = "1"
            "d3d12_instanced_draw_execution_expected_instances" = $expectedD3d12InstancedInstances
            "d3d12_instanced_draws_ok" = "1"
            "d3d12_instanced_instances_ok" = "1"
        }
        foreach ($field in $expectedD3d12InstancedFields.Keys) {
            $expectedValue = [regex]::Escape($expectedD3d12InstancedFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove D3D12 instanced draw evidence field: $field=$($expectedD3d12InstancedFields[$field])"
            }
        }
        foreach ($field in @(
                "d3d12_instanced_draw_calls",
                "d3d12_instanced_indexed_draw_calls",
                "d3d12_instanced_instances_submitted",
                "rhi_instanced_draw_calls",
                "rhi_instanced_indexed_draw_calls",
                "rhi_instanced_instances_submitted"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove positive D3D12 instanced draw evidence field: $field"
            }
        }
    }
    if ($requiresVulkanInstancedDrawEvidence) {
        $expectedVulkanInstancedInstances = [string]($expectedSmokeFrames * 3)
        $expectedVulkanInstancedFields = @{
            "vulkan_instanced_draw_execution_status" = "ready"
            "vulkan_instanced_draw_execution_ready" = "1"
            "vulkan_instanced_draw_execution_selected" = "1"
            "vulkan_instanced_draw_execution_expected_instances" = $expectedVulkanInstancedInstances
            "vulkan_instanced_draws_ok" = "1"
            "vulkan_instanced_instances_ok" = "1"
        }
        foreach ($field in $expectedVulkanInstancedFields.Keys) {
            $expectedValue = [regex]::Escape($expectedVulkanInstancedFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove Vulkan instanced draw evidence field: $field=$($expectedVulkanInstancedFields[$field])"
            }
        }
        foreach ($field in @(
                "vulkan_instanced_draw_calls",
                "vulkan_instanced_indexed_draw_calls",
                "vulkan_instanced_instances_submitted",
                "rhi_instanced_draw_calls",
                "rhi_instanced_indexed_draw_calls",
                "rhi_instanced_instances_submitted"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove positive Vulkan instanced draw evidence field: $field"
            }
        }
    }
    if ($requiresGpuMemoryPolicy) {
        $expectedGpuMemoryBackendEvidence =
            if ($requiresD3d12GpuMemoryEvidence -or $requiresVulkanGpuMemoryEvidence) { "1" } else { "0" }
        $expectedGpuMemoryOsVideoBudgetRequired = if ($requiresD3d12GpuMemoryEvidence) { "1" } else { "0" }
        $expectedGpuMemoryPolicyFields = @{
            "gpu_memory_policy_status" = "ready"
            "gpu_memory_policy_ready" = "1"
            "gpu_memory_policy_diagnostics" = "0"
            "gpu_memory_policy_scene_resources_ready" = "1"
            "gpu_memory_policy_expected_frames" = [string]$expectedSmokeFrames
            "gpu_memory_policy_frames_finished" = [string]$expectedSmokeFrames
            "gpu_memory_policy_frames_current" = "1"
            "gpu_memory_policy_backend_memory_evidence_required" = $expectedGpuMemoryBackendEvidence
            "gpu_memory_policy_backend_memory_evidence_ready" = $expectedGpuMemoryBackendEvidence
            "gpu_memory_policy_os_video_memory_budget_required" = $expectedGpuMemoryOsVideoBudgetRequired
            "gpu_memory_policy_memory_budget_evidence_ready" = "1"
            "gpu_memory_policy_residency_pressure_evidence_ready" = "1"
            "gpu_memory_policy_package_counter_evidence_ready" = "1"
        }
        foreach ($field in $expectedGpuMemoryPolicyFields.Keys) {
            $expectedValue = [regex]::Escape($expectedGpuMemoryPolicyFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove GPU memory policy field: $field=$($expectedGpuMemoryPolicyFields[$field])"
            }
        }
        foreach ($field in @(
                "gpu_memory_policy_requests",
                "gpu_memory_policy_requested_bytes",
                "gpu_memory_policy_counted_bytes",
                "gpu_memory_policy_committed_byte_estimate",
                "gpu_memory_policy_upload_bytes_written",
                "gpu_memory_policy_declared_budget_requests",
                "gpu_memory_policy_residency_pressure_requests",
                "gpu_memory_policy_package_counter_requests",
                "gpu_memory_policy_residency_pressure_events"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove positive GPU memory policy field: $field"
            }
        }
    }
    if ($requiresMemoryDiagnostics) {
        $expectedMemoryDiagnosticsFields = @{
            "memory_diagnostics_status" = "ready"
            "memory_diagnostics_ready" = "1"
            "memory_diagnostics_diagnostics" = "0"
            "memory_diagnostics_budget_pressure_classes" = "0"
            "memory_diagnostics_budget_exceeded_classes" = "0"
            "memory_diagnostics_invalid_counter" = "0"
            "memory_diagnostics_stale_generation" = "0"
            "memory_diagnostics_use_after_safe_point" = "0"
            "memory_diagnostics_resident_gpu_pressure" = "nominal"
            "memory_diagnostics_transient_gpu_allocations" = "0"
            "memory_diagnostics_transient_gpu_framegraph_aliasing_ready" = if ($requiresAnyFramegraphMultiqueueEvidence) { "1" } else { "0" }
        }
        foreach ($field in $expectedMemoryDiagnosticsFields.Keys) {
            $expectedValue = [regex]::Escape($expectedMemoryDiagnosticsFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove memory diagnostics field: $field=$($expectedMemoryDiagnosticsFields[$field])"
            }
        }
        foreach ($field in @(
                "memory_diagnostics_rows",
                "memory_diagnostics_classes",
                "memory_diagnostics_total_bytes",
                "memory_diagnostics_high_water_bytes",
                "memory_diagnostics_total_allocation_count",
                "memory_diagnostics_budgeted_classes",
                "memory_diagnostics_package_resident_cpu_bytes",
                "memory_diagnostics_package_resident_cpu_allocations",
                "memory_diagnostics_resident_gpu_bytes",
                "memory_diagnostics_resident_gpu_allocations",
                "memory_diagnostics_resident_gpu_budget_bytes",
                "memory_diagnostics_upload_staging_bytes",
                "memory_diagnostics_upload_staging_allocations"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove positive memory diagnostics field: $field"
            }
        }
        if ($requiresAnyFramegraphMultiqueueEvidence -and
            $smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bmemory_diagnostics_transient_gpu_aliasing_barriers=[1-9]\d*\b") {
            Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove positive memory diagnostics transient GPU aliasing evidence."
        }
    }
    if ($requiresJobSchedulingEvidence) {
        $expectedJobSchedulingFields = @{
            "job_scheduling_evidence_status" = "ready"
            "job_scheduling_evidence_ready" = "1"
            "job_scheduling_evidence_diagnostics" = "0"
            "job_scheduling_evidence_scratch_diagnostics" = "0"
            "job_scheduling_evidence_topology_rows" = "1"
            "job_scheduling_evidence_worker_count" = "2"
            "job_scheduling_evidence_queue_count" = "2"
            "job_scheduling_evidence_queue_rows" = "2"
            "job_scheduling_evidence_submitted_jobs" = "3"
            "job_scheduling_evidence_completed_jobs" = "3"
            "job_scheduling_evidence_execution_rows" = "3"
            "job_scheduling_evidence_deterministic_merges" = "3"
            "job_scheduling_evidence_nondeterministic_merges" = "0"
            "job_scheduling_evidence_blocked_dependencies" = "0"
            "job_scheduling_evidence_dependency_cycles" = "0"
            "job_scheduling_evidence_queue_overflows" = "0"
            "job_scheduling_evidence_scratch_misuse" = "0"
            "job_scheduling_evidence_scratch_rows" = "2"
            "job_scheduling_evidence_native_threads_started" = "0"
            "job_scheduling_evidence_thread_pool_started" = "0"
            "job_scheduling_evidence_affinity_policy_applied" = "0"
            "job_scheduling_evidence_numa_policy_applied" = "0"
            "job_scheduling_evidence_simd_dispatch_applied" = "0"
            "job_scheduling_evidence_gpu_async_overlap_applied" = "0"
        }
        foreach ($field in $expectedJobSchedulingFields.Keys) {
            $expectedValue = [regex]::Escape($expectedJobSchedulingFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove job scheduling evidence field: $field=$($expectedJobSchedulingFields[$field])"
            }
        }
        foreach ($field in @(
                "job_scheduling_evidence_scratch_bytes",
                "job_scheduling_evidence_scratch_high_water_bytes"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove positive job scheduling evidence field: $field"
            }
        }
    }
    if ($requiresJobExecutionTopologyPolicy) {
        $expectedJobExecutionTopologyPolicyFields = @{
            "job_execution_topology_policy_status" = "ready"
            "job_execution_topology_policy_ready" = "1"
            "job_execution_topology_policy_diagnostics" = "0"
            "job_execution_topology_policy_observed_logical_processors" = "8"
            "job_execution_topology_policy_effective_logical_processors" = "8"
            "job_execution_topology_policy_selected_worker_count" = "2"
            "job_execution_topology_policy_worker_count_limit" = "2"
            "job_execution_topology_policy_reserved_logical_processors" = "1"
            "job_execution_topology_policy_fallback_used" = "0"
            "job_execution_topology_policy_worker_count_limited_by_cap" = "1"
            "job_execution_topology_policy_requested_worker_count_used" = "0"
            "job_execution_topology_policy_clamped_to_logical_processors" = "0"
            "job_execution_topology_policy_processor_group_count" = "1"
            "job_execution_topology_policy_numa_node_count" = "1"
            "job_execution_topology_policy_processor_group_policy_applied" = "0"
            "job_execution_topology_policy_numa_policy_applied" = "0"
            "job_execution_topology_policy_affinity_policy_applied" = "0"
            "job_execution_topology_policy_simd_dispatch_applied" = "0"
            "job_execution_topology_policy_gpu_async_overlap_applied" = "0"
            "job_execution_topology_policy_cuda_path_used" = "0"
            "job_execution_topology_policy_hip_path_used" = "0"
            "job_execution_topology_policy_sycl_path_used" = "0"
        }
        foreach ($field in $expectedJobExecutionTopologyPolicyFields.Keys) {
            $expectedValue = [regex]::Escape($expectedJobExecutionTopologyPolicyFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove job execution topology policy field: $field=$($expectedJobExecutionTopologyPolicyFields[$field])"
            }
        }
    }
    if ($requiresJobExecutionFoundation) {
        $expectedJobExecutionFoundationFields = @{
            "job_execution_foundation_status" = "ready"
            "job_execution_foundation_ready" = "1"
            "job_execution_foundation_pool_status" = "ready"
            "job_execution_foundation_run_status" = "ready"
            "job_execution_foundation_diagnostics" = "0"
            "job_execution_foundation_worker_threads_started" = "2"
            "job_execution_foundation_tasks_submitted" = "3"
            "job_execution_foundation_tasks_executed" = "3"
            "job_execution_foundation_tasks_failed" = "0"
            "job_execution_foundation_task_side_effects" = "3"
            "job_execution_foundation_execution_rows" = "3"
            "job_execution_foundation_queue_rows" = "2"
            "job_execution_foundation_deterministic_merges" = "3"
            "job_execution_foundation_work_stealing_applied" = "0"
            "job_execution_foundation_affinity_policy_applied" = "0"
            "job_execution_foundation_numa_policy_applied" = "0"
            "job_execution_foundation_simd_dispatch_applied" = "0"
            "job_execution_foundation_gpu_async_overlap_applied" = "0"
            "job_execution_foundation_cuda_path_used" = "0"
            "job_execution_foundation_hip_path_used" = "0"
            "job_execution_foundation_sycl_path_used" = "0"
        }
        foreach ($field in $expectedJobExecutionFoundationFields.Keys) {
            $expectedValue = [regex]::Escape($expectedJobExecutionFoundationFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove job execution foundation field: $field=$($expectedJobExecutionFoundationFields[$field])"
            }
        }
        foreach ($field in @(
                "job_execution_foundation_scratch_bytes",
                "job_execution_foundation_scratch_high_water_bytes"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove positive job execution foundation field: $field"
            }
        }
    }
    if ($requiresD3d12GpuMemoryEvidence) {
        $expectedD3d12GpuMemoryFields = @{
            "d3d12_gpu_memory_execution_status" = "ready"
            "d3d12_gpu_memory_execution_ready" = "1"
            "d3d12_gpu_memory_execution_selected" = "1"
            "d3d12_gpu_memory_execution_committed_byte_estimate_available" = "1"
            "d3d12_gpu_memory_execution_budget_ok" = "1"
            "d3d12_gpu_memory_execution_transient_heap_ok" = "1"
        }
        foreach ($field in $expectedD3d12GpuMemoryFields.Keys) {
            $expectedValue = [regex]::Escape($expectedD3d12GpuMemoryFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove D3D12 GPU memory evidence field: $field=$($expectedD3d12GpuMemoryFields[$field])"
            }
        }
        foreach ($field in @(
                "d3d12_gpu_memory_execution_committed_resources_byte_estimate",
                "d3d12_gpu_memory_execution_upload_bytes_written"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove positive D3D12 GPU memory evidence field: $field"
            }
        }
    }
    if ($requiresVulkanGpuMemoryEvidence) {
        $expectedVulkanGpuMemoryFields = @{
            "vulkan_gpu_memory_execution_status" = "ready"
            "vulkan_gpu_memory_execution_ready" = "1"
            "vulkan_gpu_memory_execution_selected" = "1"
            "vulkan_gpu_memory_execution_committed_byte_estimate_available" = "1"
            "vulkan_gpu_memory_execution_budget_ok" = "1"
            "vulkan_gpu_memory_execution_transient_heap_ok" = "1"
        }
        foreach ($field in $expectedVulkanGpuMemoryFields.Keys) {
            $expectedValue = [regex]::Escape($expectedVulkanGpuMemoryFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove Vulkan GPU memory evidence field: $field=$($expectedVulkanGpuMemoryFields[$field])"
            }
        }
        foreach ($field in @(
                "vulkan_gpu_memory_execution_committed_resources_byte_estimate",
                "vulkan_gpu_memory_execution_upload_bytes_written",
                "vulkan_gpu_memory_execution_framegraph_barrier_steps_executed"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove positive Vulkan GPU memory evidence field: $field"
            }
        }
    }
    if ($requiresDebugProfilingPolicy) {
        $expectedDebugProfilingBackendEvidence =
            if ($requiresD3d12DebugProfilingEvidence -or $requiresVulkanDebugProfilingEvidence) { "1" } else { "0" }
        $expectedDebugProfilingPolicyFields = @{
            "debug_profiling_policy_status" = "ready"
            "debug_profiling_policy_ready" = "1"
            "debug_profiling_policy_diagnostics" = "0"
            "debug_profiling_policy_scene_resources_ready" = "1"
            "debug_profiling_policy_expected_frames" = [string]$expectedSmokeFrames
            "debug_profiling_policy_frames_finished" = [string]$expectedSmokeFrames
            "debug_profiling_policy_frames_current" = "1"
            "debug_profiling_policy_backend_profiling_evidence_required" = $expectedDebugProfilingBackendEvidence
            "debug_profiling_policy_backend_profiling_evidence_ready" = $expectedDebugProfilingBackendEvidence
            "debug_profiling_policy_cpu_profile_zone_evidence_ready" = "1"
            "debug_profiling_policy_trace_capture_handoff_evidence_ready" = "1"
            "debug_profiling_policy_package_counter_evidence_ready" = "1"
        }
        foreach ($field in $expectedDebugProfilingPolicyFields.Keys) {
            $expectedValue = [regex]::Escape($expectedDebugProfilingPolicyFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove debug profiling policy field: $field=$($expectedDebugProfilingPolicyFields[$field])"
            }
        }
        foreach ($field in @(
                "debug_profiling_policy_requests",
                "debug_profiling_policy_gpu_debug_markers_inserted",
                "debug_profiling_policy_framegraph_barrier_steps_executed",
                "debug_profiling_policy_framegraph_render_passes_recorded",
                "debug_profiling_policy_cpu_profile_zones",
                "debug_profiling_policy_trace_capture_handoff_rows",
                "debug_profiling_policy_cpu_profile_zone_requests",
                "debug_profiling_policy_trace_capture_handoff_requests",
                "debug_profiling_policy_package_counter_requests"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove positive debug profiling policy field: $field"
            }
        }
        if ($requiresD3d12DebugProfilingEvidence) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bdebug_profiling_policy_gpu_timestamp_ticks_per_second=[1-9]\d*\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove positive debug profiling policy field: debug_profiling_policy_gpu_timestamp_ticks_per_second"
            }
        }
    }
    if ($requiresD3d12DebugProfilingEvidence) {
        $expectedD3d12DebugProfilingFields = @{
            "d3d12_debug_profiling_execution_status" = "ready"
            "d3d12_debug_profiling_execution_ready" = "1"
            "d3d12_debug_profiling_execution_selected" = "1"
            "d3d12_debug_profiling_execution_gpu_timestamps_ok" = "1"
            "d3d12_debug_profiling_execution_gpu_debug_markers_ok" = "1"
            "d3d12_debug_profiling_execution_frame_diagnostics_ok" = "1"
        }
        foreach ($field in $expectedD3d12DebugProfilingFields.Keys) {
            $expectedValue = [regex]::Escape($expectedD3d12DebugProfilingFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove D3D12 debug profiling evidence field: $field=$($expectedD3d12DebugProfilingFields[$field])"
            }
        }
        foreach ($field in @(
                "d3d12_debug_profiling_execution_gpu_timestamp_ticks_per_second",
                "d3d12_debug_profiling_execution_gpu_debug_markers_inserted",
                "d3d12_debug_profiling_execution_framegraph_barrier_steps_executed",
                "d3d12_debug_profiling_execution_framegraph_render_passes_recorded"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove positive D3D12 debug profiling evidence field: $field"
            }
        }
    }
    if ($requiresVulkanDebugProfilingEvidence) {
        $expectedVulkanDebugProfilingFields = @{
            "vulkan_debug_profiling_execution_status" = "ready"
            "vulkan_debug_profiling_execution_ready" = "1"
            "vulkan_debug_profiling_execution_selected" = "1"
            "vulkan_debug_profiling_execution_gpu_debug_markers_ok" = "1"
            "vulkan_debug_profiling_execution_frame_diagnostics_ok" = "1"
        }
        foreach ($field in $expectedVulkanDebugProfilingFields.Keys) {
            $expectedValue = [regex]::Escape($expectedVulkanDebugProfilingFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove Vulkan debug profiling evidence field: $field=$($expectedVulkanDebugProfilingFields[$field])"
            }
        }
        foreach ($field in @(
                "vulkan_debug_profiling_execution_gpu_debug_markers_inserted",
                "vulkan_debug_profiling_execution_framegraph_barrier_steps_executed",
                "vulkan_debug_profiling_execution_framegraph_render_passes_recorded"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed sample_desktop_runtime_game smoke status line did not prove positive Vulkan debug profiling evidence field: $field"
            }
        }
    }
}
if ($GameTarget -eq "sample_2d_desktop_runtime_package") {
    foreach ($field in @(
            "sprite_batch_plan_sprites",
            "sprite_batch_plan_textured_sprites",
            "sprite_batch_plan_draws",
            "sprite_batch_plan_texture_binds",
            "sprite_batch_plan_atlas_backed_batches",
            "sprite_batch_plan_repeated_atlas_batches",
            "sprite_batch_plan_repeated_atlas_sprites",
            "sprite_batch_plan_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not include sprite batch plan field: $field"
        }
    }
    foreach ($field in @(
            "sprite_batch_plan_sprites",
            "sprite_batch_plan_textured_sprites",
            "sprite_batch_plan_draws",
            "sprite_batch_plan_texture_binds",
            "sprite_batch_plan_atlas_backed_batches",
            "sprite_batch_plan_repeated_atlas_batches",
            "sprite_batch_plan_repeated_atlas_sprites"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove sprite batch plan count: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsprite_batch_plan_diagnostics=0\b") {
        Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove clean sprite batch planning diagnostics."
    }
    Assert-InstalledAudioProductionEvidence -SmokeOutput $smokeOutput -EscapedGameTarget $escapedGameTarget -Context "sample_2d_desktop_runtime_package"
    if ($requiresSandboxPackageBudgets) {
        Assert-InstalledSandboxPackageBudgetEvidence -SmokeOutput $smokeOutput -EscapedGameTarget $escapedGameTarget -Context "sample_2d_desktop_runtime_package"
    }
    if ($requiresPerformanceBaseline) {
        Assert-InstalledPerformanceBaselineEvidence -SmokeOutput $smokeOutput -EscapedGameTarget $escapedGameTarget -ExpectedFrames $expectedSmokeFrames -Context "sample_2d_desktop_runtime_package"
    }
    foreach ($field in @(
            "sprite_batch_budget_status",
            "sprite_batch_budget_profiles_ready",
            "sprite_batch_budget_rows",
            "sprite_batch_budget_diagnostics",
            "sprite_batch_budget_total_sprites",
            "sprite_batch_budget_total_draws",
            "sprite_batch_budget_total_texture_binds",
            "sprite_batch_budget_world_ready",
            "sprite_batch_budget_world_max_sprites",
            "sprite_batch_budget_world_max_draws",
            "sprite_batch_budget_world_max_texture_binds",
            "sprite_batch_budget_ui_ready",
            "sprite_batch_budget_ui_max_sprites",
            "sprite_batch_budget_ui_max_draws",
            "sprite_batch_budget_ui_max_texture_binds",
            "sprite_batch_budget_effects_ready",
            "sprite_batch_budget_effects_max_sprites",
            "sprite_batch_budget_effects_max_draws",
            "sprite_batch_budget_effects_max_texture_binds"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not include sprite batch budget field: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsprite_batch_budget_status=ready\b") {
        Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove ready sprite batch budget status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsprite_batch_budget_diagnostics=0\b") {
        Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove clean sprite batch budget diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsprite_batch_budget_ui_max_texture_binds=0\b") {
        Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove the UI lane zero-texture-bind budget."
    }
    foreach ($field in @(
            "sprite_batch_budget_profiles_ready",
            "sprite_batch_budget_rows",
            "sprite_batch_budget_total_sprites",
            "sprite_batch_budget_total_draws",
            "sprite_batch_budget_total_texture_binds",
            "sprite_batch_budget_world_ready",
            "sprite_batch_budget_world_max_sprites",
            "sprite_batch_budget_world_max_draws",
            "sprite_batch_budget_world_max_texture_binds",
            "sprite_batch_budget_ui_ready",
            "sprite_batch_budget_ui_max_sprites",
            "sprite_batch_budget_ui_max_draws",
            "sprite_batch_budget_effects_ready",
            "sprite_batch_budget_effects_max_sprites",
            "sprite_batch_budget_effects_max_draws",
            "sprite_batch_budget_effects_max_texture_binds"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove positive sprite batch budget field: $field"
        }
    }
    if ($requiresGameplaySystems) {
        foreach ($field in @(
                "gameplay_systems_status",
                "gameplay_systems_ready",
                "gameplay_systems_ticks",
                "gameplay_systems_physics_ticks",
                "gameplay_systems_physics_bodies",
                "gameplay_systems_physics_contacts",
                "gameplay_systems_physics_trigger_overlaps",
                "gameplay_systems_navigation_path_points",
                "gameplay_systems_navigation_reached",
                "gameplay_systems_navigation_plan_status",
                "gameplay_systems_navigation_plan_diagnostic",
                "gameplay_systems_navigation_agent_status",
                "gameplay_systems_perception_status",
                "gameplay_systems_perception_targets",
                "gameplay_systems_perception_has_primary_target",
                "gameplay_systems_perception_visible_count",
                "gameplay_systems_perception_audible_count",
                "gameplay_systems_perception_readiness_status",
                "gameplay_systems_perception_readiness_diagnostic",
                "gameplay_systems_perception_readiness_diagnostics",
                "gameplay_systems_perception_stable_primary_target_ready",
                "gameplay_systems_perception_blackboard_projection_ready",
                "gameplay_systems_blackboard_status",
                "gameplay_systems_blackboard_has_target",
                "gameplay_systems_blackboard_needs_move",
                "gameplay_systems_behavior_status",
                "gameplay_systems_behavior_nodes",
                "gameplay_systems_behavior_authoring_ready",
                "gameplay_systems_behavior_authoring_readiness_status",
                "gameplay_systems_behavior_authoring_readiness_diagnostic",
                "gameplay_systems_behavior_authoring_diagnostics",
                "gameplay_systems_behavior_authoring_readiness_diagnostics",
                "gameplay_systems_behavior_authoring_trace_nodes",
                "gameplay_systems_behavior_authoring_deterministic_trace_ready",
                "gameplay_systems_behavior_authoring_behaviors",
                "gameplay_systems_behavior_authoring_action_bindings",
                "gameplay_systems_behavior_authoring_blackboard_conditions",
                "gameplay_systems_quest_dialogue_ready",
                "gameplay_systems_quest_dialogue_diagnostics",
                "gameplay_systems_quest_dialogue_transition_rows",
                "gameplay_systems_quest_dialogue_completed_objectives",
                "gameplay_systems_quest_dialogue_flags",
                "gameplay_systems_quest_dialogue_dialogue_nodes",
                "gameplay_systems_quest_dialogue_action_ids",
                "gameplay_systems_quest_dialogue_reward_ids",
                "gameplay_systems_quest_dialogue_state_rows",
                "gameplay_systems_inventory_items_ready",
                "gameplay_systems_inventory_items_diagnostics",
                "gameplay_systems_inventory_items_catalog_rows",
                "gameplay_systems_inventory_items_state_rows",
                "gameplay_systems_inventory_items_transition_rows",
                "gameplay_systems_inventory_items_accepted_rows",
                "gameplay_systems_inventory_items_completed_rows",
                "gameplay_systems_inventory_items_final_stacks",
                "gameplay_systems_inventory_items_final_workbench_quantity",
                "gameplay_systems_scene_binding_ready",
                "gameplay_systems_scene_binding_source_rows",
                "gameplay_systems_scene_binding_rows",
                "gameplay_systems_scene_binding_systems",
                "gameplay_systems_scene_binding_component_rows",
                "gameplay_systems_scene_binding_diagnostics",
                "gameplay_systems_scene_interaction_rows",
                "gameplay_systems_scene_interaction_diagnostics",
                "gameplay_systems_scene_interaction_final_session_state",
                "input_context_rebinding_ready",
                "input_context_rebinding_layers",
                "input_context_rebinding_active_contexts",
                "input_context_rebinding_capture_active",
                "input_context_rebinding_gameplay_consumed",
                "input_rebinding_profile_overlays_applied",
                "input_rebinding_action_capture_status",
                "input_rebinding_axis_capture_status",
                "input_rebinding_focus_consumed",
                "input_rebinding_focus_retained",
                "input_rebinding_presentation_rows",
                "input_rebinding_glyph_lookup_keys",
                "input_rebinding_diagnostics",
                "gameplay_systems_construction_placement_ready",
                "gameplay_systems_construction_placement_diagnostics",
                "gameplay_systems_construction_placement_validation_rows",
                "gameplay_systems_construction_placement_intent_rows",
                "gameplay_systems_construction_placement_intent_accepted_rows",
                "gameplay_systems_construction_placement_intent_occupied_cells",
                "gameplay_systems_procedural_generation_ready",
                "gameplay_systems_procedural_generation_diagnostics",
                "gameplay_systems_procedural_generation_rows",
                "gameplay_systems_procedural_generation_object_rows",
                "gameplay_systems_procedural_generation_encounter_rows",
                "gameplay_systems_procedural_generation_loot_rows",
                "gameplay_systems_procedural_generation_replay_hash",
                "gameplay_systems_procedural_generation_package_visible_rows",
                "gameplay_systems_procedural_generation_placement_intent_rows",
                "gameplay_systems_procedural_generation_placement_intent_accepted_rows",
                "rpg_systems_status",
                "rpg_systems_ready",
                "rpg_systems_party_members",
                "rpg_systems_enemy_members",
                "rpg_systems_stat_rows",
                "rpg_systems_progression_rows",
                "rpg_systems_skill_rows",
                "rpg_systems_skill_blocked_rows",
                "rpg_systems_equipment_rows",
                "rpg_systems_equipment_blocked_rows",
                "rpg_systems_combat_turn_rows",
                "rpg_systems_combat_rounds",
                "rpg_systems_reward_rows",
                "rpg_systems_save_validation_rows",
                "rpg_systems_save_validation_repairable_rows",
                "rpg_systems_replay_hash",
                "rpg_systems_invoked_combat_execution",
                "rpg_systems_invoked_reward_application",
                "rpg_systems_invoked_save_io",
                "rpg_systems_diagnostics",
                "sandbox_world_status",
                "sandbox_world_ready",
                "sandbox_world_chunk_rows",
                "sandbox_world_resident_chunk_rows",
                "sandbox_world_existing_cell_rows",
                "sandbox_world_placement_intent_rows",
                "sandbox_world_placement_accepted_rows",
                "sandbox_world_placement_rejected_rows",
                "sandbox_world_destruction_intent_rows",
                "sandbox_world_destruction_accepted_rows",
                "sandbox_world_destruction_rejected_rows",
                "sandbox_world_construction_cost_rows",
                "sandbox_world_cost_consumption_rows",
                "sandbox_world_tile_drop_rows",
                "sandbox_world_tool_effectiveness_rows",
                "sandbox_world_spawn_region_rows",
                "sandbox_world_day_night_event_rows",
                "sandbox_world_trigger_rows",
                "sandbox_world_mutation_rows",
                "sandbox_world_persistence_rows",
                "sandbox_world_persistence_repairable_rows",
                "sandbox_world_rejected_unsafe_mutation_rows",
                "sandbox_world_replay_hash",
                "sandbox_world_invoked_world_mutation",
                "sandbox_world_invoked_persistence_io",
                "sandbox_world_invoked_package_io",
                "sandbox_world_diagnostics",
                "simulation_management_status",
                "simulation_management_ready",
                "simulation_management_tick_count",
                "simulation_management_resource_balance_rows",
                "simulation_management_job_rows",
                "simulation_management_job_assignment_rows",
                "simulation_management_logistics_links",
                "simulation_management_logistics_transfer_rows",
                "simulation_management_logistics_scheduled_transfer_rows",
                "simulation_management_economy_summary_rows",
                "simulation_management_population_need_rows",
                "simulation_management_need_deficit_rows",
                "simulation_management_schedule_rows",
                "simulation_management_save_review_rows",
                "simulation_management_save_review_repairable_rows",
                "simulation_management_dashboard_rows",
                "simulation_management_replay_hash",
                "simulation_management_invoked_economy_execution",
                "simulation_management_invoked_save_io",
                "simulation_management_invoked_runtime_ui",
                "simulation_management_invoked_package_io",
                "simulation_management_diagnostics",
                "network_replication_status",
                "network_replication_reviewed",
                "network_replication_ready",
                "network_replication_object_rows",
                "network_replication_input_rows",
                "network_replication_snapshot_rows",
                "network_replication_rollback_rows",
                "network_replication_rejected_unsafe_rows",
                "network_replication_replay_hash",
                "network_replication_requires_transport_host_evidence",
                "network_replication_transport_host_evidence",
                "network_replication_invoked_network_io",
                "network_replication_invoked_rollback_execution",
                "network_replication_invoked_world_mutation",
                "network_replication_diagnostics",
                "network_production_security_status",
                "network_production_security_reviewed",
                "network_production_security_ready",
                "network_production_security_threat_model_reviewed",
                "network_production_security_loopback_host_evidence",
                "network_production_security_replication_evidence_ready",
                "network_production_security_general_online_ready",
                "network_production_security_session_lifecycle_rows",
                "network_production_security_connection_state_rows",
                "network_production_security_channel_policy_rows",
                "network_production_security_reliable_delivery_rows",
                "network_production_security_unreliable_delivery_rows",
                "network_production_security_sequence_replay_rejection_rows",
                "network_production_security_input_command_validation_rows",
                "network_production_security_snapshot_validation_rows",
                "network_production_security_rollback_window_diagnostic_rows",
                "network_production_security_unsupported_online_claim_rows",
                "network_production_security_replay_hash",
                "network_production_security_invoked_external_network_io",
                "network_production_security_invoked_threads",
                "network_production_security_invoked_save_io",
                "network_production_security_invoked_world_mutation",
                "network_production_security_diagnostics"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
                Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not include gameplay systems field: $field"
            }
        }
        foreach ($field in @("gameplay_systems_ready", "gameplay_systems_navigation_reached", "gameplay_systems_perception_has_primary_target", "gameplay_systems_blackboard_has_target", "gameplay_systems_blackboard_needs_move", "gameplay_systems_behavior_authoring_ready", "gameplay_systems_behavior_authoring_deterministic_trace_ready", "gameplay_systems_quest_dialogue_ready", "gameplay_systems_inventory_items_ready", "gameplay_systems_construction_placement_ready", "gameplay_systems_procedural_generation_ready", "rpg_systems_ready", "sandbox_world_ready", "simulation_management_ready", "network_replication_reviewed", "network_production_security_reviewed", "network_production_security_threat_model_reviewed")) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=1\b") {
                Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove ready gameplay systems field: $field"
            }
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_binding_ready=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove ready scene gameplay bindings."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_binding_source_rows=3\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact scene gameplay binding source rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_binding_rows=3\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact resolved scene gameplay binding rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_binding_systems=3\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact scene gameplay binding system count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_binding_component_rows=3\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact component-backed scene gameplay binding rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_binding_diagnostics=0\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove clean scene gameplay binding diagnostics."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_interaction_rows=3\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact scene gameplay interaction rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_interaction_diagnostics=0\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove clean scene gameplay interaction diagnostics."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_interaction_final_session_state=won\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove scene gameplay interaction win state."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_context_rebinding_ready=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove ready input context rebinding."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_context_rebinding_layers=3\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact input context layer count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_context_rebinding_active_contexts=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact active input context count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_context_rebinding_capture_active=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove active capture context."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_context_rebinding_gameplay_consumed=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove gameplay input consumption."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_profile_overlays_applied=2\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact input rebinding overlay count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_action_capture_status=captured\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove captured action rebinding input."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_axis_capture_status=captured\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove captured axis rebinding input."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_focus_consumed=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove focused capture consumption."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_focus_retained=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove retained focused capture."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_presentation_rows=2\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact input rebinding presentation rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_glyph_lookup_keys=5\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact symbolic glyph lookup key count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_diagnostics=0\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove clean input rebinding diagnostics."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_status=ready\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove ready gameplay systems status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_ticks=$expectedSmokeFrames\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove frame-exact gameplay tick count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_ticks=$expectedSmokeFrames\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove frame-exact gameplay physics tick count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_bodies=3\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact gameplay physics body count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_contacts=[1-9]\d*\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove non-zero gameplay physics contacts."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_trigger_overlaps=[1-9]\d*\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove non-zero gameplay physics trigger overlaps."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_path_points=2\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove 2D gameplay navigation path points."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_plan_status=ready\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove ready gameplay navigation plan status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_plan_diagnostic=none\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove clean gameplay navigation diagnostics."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_agent_status=reached_destination\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove gameplay navigation destination reached."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_perception_status=ready\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove ready gameplay perception status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_perception_targets=2\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact gameplay perception target count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_perception_visible_count=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact gameplay visible target count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_perception_audible_count=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact gameplay audible target count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_perception_readiness_status=ready\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove ready gameplay perception readiness status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_perception_readiness_diagnostic=none\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove clean gameplay perception readiness diagnostic."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_perception_readiness_diagnostics=0\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove zero gameplay perception readiness diagnostics."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_perception_stable_primary_target_ready=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove stable gameplay perception primary target."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_perception_blackboard_projection_ready=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove ready gameplay perception blackboard projection."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_blackboard_status=ready\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove ready gameplay blackboard status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_behavior_status=success\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove successful gameplay behavior tree."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_behavior_nodes=4\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact gameplay behavior node visit count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_behavior_authoring_readiness_status=ready\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove ready behavior authoring readiness status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_behavior_authoring_readiness_diagnostic=none\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove clean behavior authoring readiness diagnostic."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_behavior_authoring_diagnostics=0\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove clean gameplay behavior authoring diagnostics."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_behavior_authoring_readiness_diagnostics=0\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove clean behavior authoring readiness diagnostics."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_behavior_authoring_trace_nodes=4\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact gameplay behavior authoring trace nodes."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_behavior_authoring_behaviors=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact behavior authoring behavior count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_behavior_authoring_action_bindings=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact behavior authoring action binding count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_behavior_authoring_blackboard_conditions=2\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact behavior authoring blackboard condition count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_quest_dialogue_diagnostics=0\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove clean quest dialogue diagnostics."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_quest_dialogue_transition_rows=3\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact quest dialogue transition rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_quest_dialogue_completed_objectives=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact quest dialogue objective progress."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_quest_dialogue_flags=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact quest dialogue flag progress."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_quest_dialogue_dialogue_nodes=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact quest dialogue node progress."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_quest_dialogue_action_ids=2\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact quest dialogue action ids."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_quest_dialogue_reward_ids=2\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact quest dialogue reward ids."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_quest_dialogue_state_rows=3\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact quest dialogue save-state rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_inventory_items_diagnostics=0\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove clean inventory item diagnostics."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_inventory_items_catalog_rows=2\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact inventory item catalog rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_inventory_items_state_rows=2\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact inventory state rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_inventory_items_transition_rows=2\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact inventory item transition rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_inventory_items_accepted_rows=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact accepted inventory rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_inventory_items_completed_rows=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact completed inventory rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_inventory_items_final_stacks=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact final inventory stack count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_inventory_items_final_workbench_quantity=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact final workbench quantity."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_construction_placement_diagnostics=0\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove clean construction placement diagnostics."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_construction_placement_validation_rows=3\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact construction placement validation rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_construction_placement_intent_rows=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact construction placement intent rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_construction_placement_intent_accepted_rows=1\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact accepted construction placement intent rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_construction_placement_intent_occupied_cells=2\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact construction placement occupied cells."
        }
        foreach ($expected in @{
                "rpg_systems_status" = "ready"
                "rpg_systems_party_members" = "2"
                "rpg_systems_enemy_members" = "1"
                "rpg_systems_stat_rows" = "8"
                "rpg_systems_progression_rows" = "2"
                "rpg_systems_skill_rows" = "2"
                "rpg_systems_skill_blocked_rows" = "1"
                "rpg_systems_equipment_rows" = "2"
                "rpg_systems_equipment_blocked_rows" = "1"
                "rpg_systems_combat_turn_rows" = "6"
                "rpg_systems_combat_rounds" = "2"
                "rpg_systems_reward_rows" = "2"
                "rpg_systems_save_validation_rows" = "2"
                "rpg_systems_save_validation_repairable_rows" = "1"
                "rpg_systems_invoked_combat_execution" = "0"
                "rpg_systems_invoked_reward_application" = "0"
                "rpg_systems_invoked_save_io" = "0"
                "rpg_systems_diagnostics" = "0"
            }.GetEnumerator()) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$([regex]::Escape($expected.Key))=$([regex]::Escape($expected.Value))\b") {
                Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove RPG systems field: $($expected.Key)=$($expected.Value)."
            }
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\brpg_systems_replay_hash=[1-9]\d*\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove positive RPG systems replay hash."
        }
        foreach ($expected in @{
                "sandbox_world_status" = "ready"
                "sandbox_world_ready" = "1"
                "sandbox_world_chunk_rows" = "2"
                "sandbox_world_resident_chunk_rows" = "2"
                "sandbox_world_existing_cell_rows" = "2"
                "sandbox_world_placement_intent_rows" = "3"
                "sandbox_world_placement_accepted_rows" = "1"
                "sandbox_world_placement_rejected_rows" = "2"
                "sandbox_world_destruction_intent_rows" = "2"
                "sandbox_world_destruction_accepted_rows" = "1"
                "sandbox_world_destruction_rejected_rows" = "1"
                "sandbox_world_construction_cost_rows" = "2"
                "sandbox_world_cost_consumption_rows" = "1"
                "sandbox_world_tile_drop_rows" = "1"
                "sandbox_world_tool_effectiveness_rows" = "1"
                "sandbox_world_spawn_region_rows" = "1"
                "sandbox_world_day_night_event_rows" = "1"
                "sandbox_world_trigger_rows" = "1"
                "sandbox_world_mutation_rows" = "5"
                "sandbox_world_persistence_rows" = "2"
                "sandbox_world_persistence_repairable_rows" = "1"
                "sandbox_world_rejected_unsafe_mutation_rows" = "3"
                "sandbox_world_invoked_world_mutation" = "0"
                "sandbox_world_invoked_persistence_io" = "0"
                "sandbox_world_invoked_package_io" = "0"
                "sandbox_world_diagnostics" = "0"
            }.GetEnumerator()) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$([regex]::Escape($expected.Key))=$([regex]::Escape($expected.Value))\b") {
                Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove sandbox world field: $($expected.Key)=$($expected.Value)."
            }
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsandbox_world_replay_hash=[1-9]\d*\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove positive sandbox world replay hash."
        }
        foreach ($expected in @{
                "simulation_management_status" = "ready"
                "simulation_management_ready" = "1"
                "simulation_management_tick_count" = "240"
                "simulation_management_resource_balance_rows" = "4"
                "simulation_management_job_rows" = "2"
                "simulation_management_job_assignment_rows" = "1"
                "simulation_management_logistics_links" = "2"
                "simulation_management_logistics_transfer_rows" = "2"
                "simulation_management_logistics_scheduled_transfer_rows" = "1"
                "simulation_management_economy_summary_rows" = "1"
                "simulation_management_population_need_rows" = "2"
                "simulation_management_need_deficit_rows" = "1"
                "simulation_management_schedule_rows" = "2"
                "simulation_management_save_review_rows" = "2"
                "simulation_management_save_review_repairable_rows" = "1"
                "simulation_management_dashboard_rows" = "7"
                "simulation_management_invoked_economy_execution" = "0"
                "simulation_management_invoked_save_io" = "0"
                "simulation_management_invoked_runtime_ui" = "0"
                "simulation_management_invoked_package_io" = "0"
                "simulation_management_diagnostics" = "0"
            }.GetEnumerator()) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$([regex]::Escape($expected.Key))=$([regex]::Escape($expected.Value))\b") {
                Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove simulation management field: $($expected.Key)=$($expected.Value)."
            }
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsimulation_management_replay_hash=[1-9]\d*\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove positive simulation management replay hash."
        }
        foreach ($expected in @{
                "network_replication_status" = "host_evidence_required"
                "network_replication_reviewed" = "1"
                "network_replication_ready" = "0"
                "network_replication_object_rows" = "2"
                "network_replication_input_rows" = "2"
                "network_replication_snapshot_rows" = "2"
                "network_replication_rollback_rows" = "1"
                "network_replication_rejected_unsafe_rows" = "0"
                "network_replication_requires_transport_host_evidence" = "1"
                "network_replication_transport_host_evidence" = "0"
                "network_replication_invoked_network_io" = "0"
                "network_replication_invoked_rollback_execution" = "0"
                "network_replication_invoked_world_mutation" = "0"
                "network_replication_diagnostics" = "0"
            }.GetEnumerator()) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$([regex]::Escape($expected.Key))=$([regex]::Escape($expected.Value))\b") {
                Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove network replication field: $($expected.Key)=$($expected.Value)."
            }
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnetwork_replication_replay_hash=[1-9]\d*\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove positive network replication replay hash."
        }
        foreach ($expected in @{
                "network_production_security_status" = "host_evidence_required"
                "network_production_security_reviewed" = "1"
                "network_production_security_ready" = "0"
                "network_production_security_threat_model_reviewed" = "1"
                "network_production_security_loopback_host_evidence" = "0"
                "network_production_security_replication_evidence_ready" = "0"
                "network_production_security_general_online_ready" = "0"
                "network_production_security_session_lifecycle_rows" = "1"
                "network_production_security_connection_state_rows" = "1"
                "network_production_security_channel_policy_rows" = "2"
                "network_production_security_reliable_delivery_rows" = "0"
                "network_production_security_unreliable_delivery_rows" = "0"
                "network_production_security_sequence_replay_rejection_rows" = "1"
                "network_production_security_input_command_validation_rows" = "1"
                "network_production_security_snapshot_validation_rows" = "1"
                "network_production_security_rollback_window_diagnostic_rows" = "1"
                "network_production_security_unsupported_online_claim_rows" = "0"
                "network_production_security_invoked_external_network_io" = "0"
                "network_production_security_invoked_threads" = "0"
                "network_production_security_invoked_save_io" = "0"
                "network_production_security_invoked_world_mutation" = "0"
                "network_production_security_diagnostics" = "2"
            }.GetEnumerator()) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$([regex]::Escape($expected.Key))=$([regex]::Escape($expected.Value))\b") {
                Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove network production security field: $($expected.Key)=$($expected.Value)."
            }
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnetwork_production_security_replay_hash=[1-9]\d*\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove positive network production security replay hash."
        }
    }
    if ($requiresProceduralGeneration) {
        foreach ($expected in @{
                "gameplay_systems_procedural_generation_ready" = "1"
                "gameplay_systems_procedural_generation_diagnostics" = "0"
                "gameplay_systems_procedural_generation_rows" = "3"
                "gameplay_systems_procedural_generation_object_rows" = "1"
                "gameplay_systems_procedural_generation_encounter_rows" = "1"
                "gameplay_systems_procedural_generation_loot_rows" = "1"
                "gameplay_systems_procedural_generation_package_visible_rows" = "1"
                "gameplay_systems_procedural_generation_placement_intent_rows" = "1"
                "gameplay_systems_procedural_generation_placement_intent_accepted_rows" = "1"
            }.GetEnumerator()) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$([regex]::Escape($expected.Key))=$([regex]::Escape($expected.Value))\b") {
                Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact procedural generation field: $($expected.Key)=$($expected.Value)."
            }
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_procedural_generation_replay_hash=[1-9]\d*\b") {
            Write-Error "Installed sample_2d_desktop_runtime_package smoke status line did not prove exact procedural generation replay hash evidence."
        }
    }
}
if ($GameTarget -eq "sample_generated_desktop_runtime_3d_package" -and $requiresGameplaySystems) {
    foreach ($field in @(
            "gameplay_systems_status",
            "gameplay_systems_ready",
            "gameplay_systems_diagnostics",
            "gameplay_runtime_scheduler_status",
            "gameplay_runtime_scheduler_ready",
            "gameplay_runtime_scheduler_available_steps",
            "gameplay_runtime_scheduler_steps",
            "gameplay_runtime_scheduler_system_rows",
            "gameplay_runtime_scheduler_command_rows",
            "gameplay_runtime_scheduler_consumed_time_us",
            "gameplay_runtime_scheduler_remaining_time_us",
            "gameplay_runtime_scheduler_budget_limited",
            "gameplay_runtime_scheduler_replay_hash",
            "gameplay_runtime_scheduler_diagnostics",
            "world_entity_model_status",
            "world_entity_model_ready",
            "world_entity_model_entities",
            "world_entity_model_components",
            "world_entity_model_region_ownership_rows",
            "world_entity_model_lifecycle_rows",
            "world_entity_model_persistence_rows",
            "world_entity_model_streaming_region_rows",
            "world_entity_model_spawn_rows",
            "world_entity_model_move_rows",
            "world_entity_model_despawn_rows",
            "world_entity_model_duplicate_entity_diagnostics",
            "world_entity_model_bridge_rejection_status",
            "world_entity_model_bridge_rejection_diagnostics",
            "world_entity_model_bridge_rejection_persistence_rows",
            "world_entity_model_bridge_rejection_streaming_region_rows",
            "world_entity_model_bridge_rejection_streaming_diagnostics_present",
            "world_entity_model_bridge_rejection_fail_closed",
            "world_entity_model_diagnostics",
            "addressable_content_status",
            "addressable_content_ready",
            "addressable_content_address_rows",
            "addressable_content_dependency_rows",
            "addressable_content_load_rows",
            "addressable_content_release_rows",
            "addressable_content_refcount_rows",
            "addressable_content_resident_bytes",
            "addressable_content_resident_budget_bytes",
            "addressable_content_budget_rejection_status",
            "addressable_content_budget_rejection_diagnostics",
            "addressable_content_package_io",
            "addressable_content_async_execution",
            "addressable_content_committed",
            "addressable_content_diagnostics",
            "rpg_systems_status",
            "rpg_systems_ready",
            "rpg_systems_party_members",
            "rpg_systems_enemy_members",
            "rpg_systems_stat_rows",
            "rpg_systems_progression_rows",
            "rpg_systems_skill_rows",
            "rpg_systems_skill_blocked_rows",
            "rpg_systems_equipment_rows",
            "rpg_systems_equipment_blocked_rows",
            "rpg_systems_combat_turn_rows",
            "rpg_systems_combat_rounds",
            "rpg_systems_reward_rows",
            "rpg_systems_save_validation_rows",
            "rpg_systems_save_validation_repairable_rows",
            "rpg_systems_replay_hash",
            "rpg_systems_invoked_combat_execution",
            "rpg_systems_invoked_reward_application",
            "rpg_systems_invoked_save_io",
            "rpg_systems_diagnostics",
            "sandbox_world_status",
            "sandbox_world_ready",
            "sandbox_world_chunk_rows",
            "sandbox_world_resident_chunk_rows",
            "sandbox_world_existing_cell_rows",
            "sandbox_world_placement_intent_rows",
            "sandbox_world_placement_accepted_rows",
            "sandbox_world_placement_rejected_rows",
            "sandbox_world_destruction_intent_rows",
            "sandbox_world_destruction_accepted_rows",
            "sandbox_world_destruction_rejected_rows",
            "sandbox_world_construction_cost_rows",
            "sandbox_world_mutation_rows",
            "sandbox_world_persistence_rows",
            "sandbox_world_persistence_repairable_rows",
            "sandbox_world_rejected_unsafe_mutation_rows",
            "sandbox_world_replay_hash",
            "sandbox_world_invoked_world_mutation",
            "sandbox_world_invoked_persistence_io",
            "sandbox_world_invoked_package_io",
            "sandbox_world_diagnostics",
            "simulation_management_status",
            "simulation_management_ready",
            "simulation_management_tick_count",
            "simulation_management_resource_balance_rows",
            "simulation_management_job_rows",
            "simulation_management_job_assignment_rows",
            "simulation_management_logistics_links",
            "simulation_management_logistics_transfer_rows",
            "simulation_management_logistics_scheduled_transfer_rows",
            "simulation_management_economy_summary_rows",
            "simulation_management_population_need_rows",
            "simulation_management_need_deficit_rows",
            "simulation_management_schedule_rows",
            "simulation_management_save_review_rows",
            "simulation_management_save_review_repairable_rows",
            "simulation_management_dashboard_rows",
            "simulation_management_replay_hash",
            "simulation_management_invoked_economy_execution",
            "simulation_management_invoked_save_io",
            "simulation_management_invoked_runtime_ui",
            "simulation_management_invoked_package_io",
            "simulation_management_diagnostics",
            "network_replication_status",
            "network_replication_reviewed",
            "network_replication_ready",
            "network_replication_object_rows",
            "network_replication_input_rows",
            "network_replication_snapshot_rows",
            "network_replication_rollback_rows",
            "network_replication_rejected_unsafe_rows",
            "network_replication_replay_hash",
            "network_replication_requires_transport_host_evidence",
            "network_replication_transport_host_evidence",
            "network_replication_invoked_network_io",
            "network_replication_invoked_rollback_execution",
            "network_replication_invoked_world_mutation",
            "network_replication_diagnostics",
            "gameplay_systems_scene_binding_ready",
            "gameplay_systems_scene_binding_source_rows",
            "gameplay_systems_scene_binding_rows",
            "gameplay_systems_scene_binding_systems",
            "gameplay_systems_scene_binding_component_rows",
            "gameplay_systems_scene_binding_diagnostics",
            "gameplay_systems_scene_interaction_rows",
            "gameplay_systems_scene_interaction_diagnostics",
            "gameplay_systems_scene_interaction_final_session_state",
            "input_context_rebinding_ready",
            "input_context_rebinding_layers",
            "input_context_rebinding_active_contexts",
            "input_context_rebinding_capture_active",
            "input_context_rebinding_gameplay_consumed",
            "input_rebinding_profile_overlays_applied",
            "input_rebinding_action_capture_status",
            "input_rebinding_axis_capture_status",
            "input_rebinding_focus_consumed",
            "input_rebinding_focus_retained",
            "input_rebinding_presentation_rows",
            "input_rebinding_glyph_lookup_keys",
            "input_rebinding_diagnostics",
            "gameplay_systems_navigation_navmesh_status",
            "gameplay_systems_navigation_navmesh_diagnostic",
            "gameplay_systems_navigation_navmesh_polygons",
            "gameplay_systems_navigation_navmesh_dynamic_obstacles",
            "gameplay_systems_navigation_navmesh_total_cost",
            "gameplay_systems_navigation_navmesh_readiness_status",
            "gameplay_systems_navigation_navmesh_readiness_diagnostic",
            "gameplay_systems_navigation_navmesh_readiness_diagnostics",
            "gameplay_systems_navigation_navmesh_scene_refs",
            "gameplay_systems_navigation_navmesh_points",
            "gameplay_systems_navigation_navmesh_visited_polygons",
            "gameplay_systems_navigation_crowd_status",
            "gameplay_systems_navigation_crowd_diagnostic",
            "gameplay_systems_navigation_crowd_rows",
            "gameplay_systems_navigation_crowd_source_order_ready",
            "gameplay_systems_navigation_crowd_agents",
            "gameplay_systems_navigation_crowd_route_successes",
            "gameplay_systems_navigation_crowd_avoidance_successes",
            "gameplay_systems_navigation_crowd_applied_neighbors",
            "gameplay_systems_navigation_crowd_dynamic_obstacles",
            "gameplay_systems_navigation_crowd_readiness_status",
            "gameplay_systems_navigation_crowd_readiness_diagnostic",
            "gameplay_systems_navigation_crowd_readiness_diagnostics",
            "gameplay_systems_navigation_crowd_readiness_rows",
            "gameplay_systems_navigation_crowd_readiness_source_order_ready",
            "gameplay_systems_navigation_crowd_readiness_applied_neighbors",
            "gameplay_systems_navigation_crowd_readiness_dynamic_obstacles",
            "gameplay_systems_local_avoidance_status",
            "gameplay_systems_local_avoidance_diagnostic",
            "gameplay_systems_local_avoidance_steps",
            "gameplay_systems_local_avoidance_applied_neighbors",
            "gameplay_systems_physics_policy_status",
            "gameplay_systems_physics_policy_diagnostic",
            "gameplay_systems_physics_policy_rows",
            "gameplay_systems_physics_policy_dynamic_pushes",
            "gameplay_systems_physics_policy_solid_contacts",
            "gameplay_systems_physics_policy_trigger_overlaps",
            "gameplay_systems_advanced_controller_status",
            "gameplay_systems_advanced_controller_diagnostic",
            "gameplay_systems_advanced_controller_movement_rows",
            "gameplay_systems_advanced_controller_platform_rows",
            "gameplay_systems_advanced_controller_platform_applied",
            "gameplay_systems_advanced_controller_constraint_rows",
            "gameplay_systems_advanced_controller_replay_changed",
            "gameplay_systems_advanced_controller_replay_before_bodies",
            "gameplay_systems_advanced_controller_replay_after_bodies",
            "gameplay_systems_character_dynamics_status",
            "gameplay_systems_character_dynamics_diagnostic",
            "gameplay_systems_character_dynamics_movement_rows",
            "gameplay_systems_character_dynamics_dynamic_pushes",
            "gameplay_systems_character_dynamics_step_ups",
            "gameplay_systems_character_dynamics_walkable_slope_rows",
            "gameplay_systems_character_dynamics_ground_probes",
            "gameplay_systems_character_dynamics_moving_platform_rows",
            "gameplay_systems_character_dynamics_applied_platform_rows",
            "gameplay_systems_character_dynamics_constraint_rows",
            "gameplay_systems_character_dynamics_replay_changed",
            "gameplay_systems_character_dynamics_diagnostics",
            "gameplay_systems_physics_constraints_status",
            "gameplay_systems_physics_constraints_diagnostic",
            "gameplay_systems_physics_constraints_rows",
            "gameplay_systems_physics_constraints_fixed_rows",
            "gameplay_systems_physics_constraints_linear_axis_rows",
            "gameplay_systems_physics_constraints_axis_limit_clamped",
            "gameplay_systems_kinematic_motion_status",
            "gameplay_systems_kinematic_motion_diagnostic",
            "gameplay_systems_kinematic_motion_rows",
            "gameplay_systems_kinematic_motion_grounded",
            "gameplay_systems_vehicle_status",
            "gameplay_systems_vehicle_diagnostic",
            "gameplay_systems_vehicle_wheel_rows",
            "gameplay_systems_vehicle_grounded_wheels",
            "gameplay_systems_vehicle_wheel_probe_hits",
            "gameplay_systems_vehicle_final_x"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not include gameplay systems field: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_status=ready\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready gameplay systems status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_ready=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready gameplay systems."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_diagnostics=0\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean gameplay systems diagnostics."
    }
    $expectedGameplayRuntimeSchedulerFields = @{
        "gameplay_runtime_scheduler_status" = "budget_limited"
        "gameplay_runtime_scheduler_ready" = "1"
        "gameplay_runtime_scheduler_available_steps" = "3"
        "gameplay_runtime_scheduler_steps" = "2"
        "gameplay_runtime_scheduler_system_rows" = "6"
        "gameplay_runtime_scheduler_command_rows" = "2"
        "gameplay_runtime_scheduler_consumed_time_us" = "33332"
        "gameplay_runtime_scheduler_remaining_time_us" = "16666"
        "gameplay_runtime_scheduler_budget_limited" = "1"
        "gameplay_runtime_scheduler_diagnostics" = "0"
    }
    foreach ($field in $expectedGameplayRuntimeSchedulerFields.Keys) {
        $expectedValue = $expectedGameplayRuntimeSchedulerFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove gameplay runtime scheduler field $field=$expectedValue."
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_runtime_scheduler_replay_hash=[1-9]\d*\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove positive gameplay runtime scheduler replay hash."
    }
    $expectedWorldEntityModelFields = @{
        "world_entity_model_status" = "ready"
        "world_entity_model_ready" = "1"
        "world_entity_model_entities" = "3"
        "world_entity_model_components" = "4"
        "world_entity_model_region_ownership_rows" = "3"
        "world_entity_model_lifecycle_rows" = "3"
        "world_entity_model_persistence_rows" = "1"
        "world_entity_model_streaming_region_rows" = "1"
        "world_entity_model_spawn_rows" = "1"
        "world_entity_model_move_rows" = "1"
        "world_entity_model_despawn_rows" = "1"
        "world_entity_model_duplicate_entity_diagnostics" = "1"
        "world_entity_model_bridge_rejection_status" = "invalid_request"
        "world_entity_model_bridge_rejection_diagnostics" = "5"
        "world_entity_model_bridge_rejection_persistence_rows" = "0"
        "world_entity_model_bridge_rejection_streaming_region_rows" = "0"
        "world_entity_model_bridge_rejection_streaming_diagnostics_present" = "1"
        "world_entity_model_bridge_rejection_fail_closed" = "1"
        "world_entity_model_diagnostics" = "0"
    }
    foreach ($field in $expectedWorldEntityModelFields.Keys) {
        $expectedValue = $expectedWorldEntityModelFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove world entity model field $field=$expectedValue."
        }
    }
    $expectedAddressableContentFields = @{
        "addressable_content_status" = "ready"
        "addressable_content_ready" = "1"
        "addressable_content_address_rows" = "6"
        "addressable_content_dependency_rows" = "5"
        "addressable_content_load_rows" = "4"
        "addressable_content_release_rows" = "1"
        "addressable_content_refcount_rows" = "5"
        "addressable_content_resident_bytes" = "2222"
        "addressable_content_resident_budget_bytes" = "67108864"
        "addressable_content_budget_rejection_status" = "budget_limited"
        "addressable_content_budget_rejection_diagnostics" = "1"
        "addressable_content_package_io" = "0"
        "addressable_content_async_execution" = "0"
        "addressable_content_committed" = "0"
        "addressable_content_diagnostics" = "0"
    }
    foreach ($field in $expectedAddressableContentFields.Keys) {
        $expectedValue = $expectedAddressableContentFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove addressable content field $field=$expectedValue."
        }
    }
    $expectedRpgSystemsFields = @{
        "rpg_systems_status" = "ready"
        "rpg_systems_ready" = "1"
        "rpg_systems_party_members" = "2"
        "rpg_systems_enemy_members" = "1"
        "rpg_systems_stat_rows" = "8"
        "rpg_systems_progression_rows" = "2"
        "rpg_systems_skill_rows" = "2"
        "rpg_systems_skill_blocked_rows" = "1"
        "rpg_systems_equipment_rows" = "2"
        "rpg_systems_equipment_blocked_rows" = "1"
        "rpg_systems_combat_turn_rows" = "6"
        "rpg_systems_combat_rounds" = "2"
        "rpg_systems_reward_rows" = "2"
        "rpg_systems_save_validation_rows" = "2"
        "rpg_systems_save_validation_repairable_rows" = "1"
        "rpg_systems_invoked_combat_execution" = "0"
        "rpg_systems_invoked_reward_application" = "0"
        "rpg_systems_invoked_save_io" = "0"
        "rpg_systems_diagnostics" = "0"
    }
    foreach ($field in $expectedRpgSystemsFields.Keys) {
        $expectedValue = $expectedRpgSystemsFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove RPG systems field $field=$expectedValue."
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\brpg_systems_replay_hash=[1-9]\d*\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove positive RPG systems replay hash."
    }
    $expectedSandboxWorldFields = @{
        "sandbox_world_status" = "ready"
        "sandbox_world_ready" = "1"
        "sandbox_world_chunk_rows" = "2"
        "sandbox_world_resident_chunk_rows" = "2"
        "sandbox_world_existing_cell_rows" = "2"
        "sandbox_world_placement_intent_rows" = "3"
        "sandbox_world_placement_accepted_rows" = "1"
        "sandbox_world_placement_rejected_rows" = "2"
        "sandbox_world_destruction_intent_rows" = "2"
        "sandbox_world_destruction_accepted_rows" = "1"
        "sandbox_world_destruction_rejected_rows" = "1"
        "sandbox_world_construction_cost_rows" = "2"
        "sandbox_world_mutation_rows" = "5"
        "sandbox_world_persistence_rows" = "2"
        "sandbox_world_persistence_repairable_rows" = "1"
        "sandbox_world_rejected_unsafe_mutation_rows" = "3"
        "sandbox_world_invoked_world_mutation" = "0"
        "sandbox_world_invoked_persistence_io" = "0"
        "sandbox_world_invoked_package_io" = "0"
        "sandbox_world_diagnostics" = "0"
    }
    foreach ($field in $expectedSandboxWorldFields.Keys) {
        $expectedValue = $expectedSandboxWorldFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove sandbox world field $field=$expectedValue."
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsandbox_world_replay_hash=[1-9]\d*\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove positive sandbox world replay hash."
    }
    $expectedSimulationManagementFields = @{
        "simulation_management_status" = "ready"
        "simulation_management_ready" = "1"
        "simulation_management_tick_count" = "240"
        "simulation_management_resource_balance_rows" = "4"
        "simulation_management_job_rows" = "2"
        "simulation_management_job_assignment_rows" = "1"
        "simulation_management_logistics_links" = "2"
        "simulation_management_logistics_transfer_rows" = "2"
        "simulation_management_logistics_scheduled_transfer_rows" = "1"
        "simulation_management_economy_summary_rows" = "1"
        "simulation_management_population_need_rows" = "2"
        "simulation_management_need_deficit_rows" = "1"
        "simulation_management_schedule_rows" = "2"
        "simulation_management_save_review_rows" = "2"
        "simulation_management_save_review_repairable_rows" = "1"
        "simulation_management_dashboard_rows" = "7"
        "simulation_management_invoked_economy_execution" = "0"
        "simulation_management_invoked_save_io" = "0"
        "simulation_management_invoked_runtime_ui" = "0"
        "simulation_management_invoked_package_io" = "0"
        "simulation_management_diagnostics" = "0"
    }
    foreach ($field in $expectedSimulationManagementFields.Keys) {
        $expectedValue = $expectedSimulationManagementFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove simulation management field $field=$expectedValue."
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsimulation_management_replay_hash=[1-9]\d*\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove positive simulation management replay hash."
    }
    $expectedNetworkReplicationFields = @{
        "network_replication_status" = "host_evidence_required"
        "network_replication_reviewed" = "1"
        "network_replication_ready" = "0"
        "network_replication_object_rows" = "2"
        "network_replication_input_rows" = "2"
        "network_replication_snapshot_rows" = "2"
        "network_replication_rollback_rows" = "1"
        "network_replication_rejected_unsafe_rows" = "0"
        "network_replication_requires_transport_host_evidence" = "1"
        "network_replication_transport_host_evidence" = "0"
        "network_replication_invoked_network_io" = "0"
        "network_replication_invoked_rollback_execution" = "0"
        "network_replication_invoked_world_mutation" = "0"
        "network_replication_diagnostics" = "0"
    }
    foreach ($field in $expectedNetworkReplicationFields.Keys) {
        $expectedValue = $expectedNetworkReplicationFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove network replication field $field=$expectedValue."
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnetwork_replication_replay_hash=[1-9]\d*\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove positive network replication replay hash."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_binding_ready=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready scene gameplay bindings."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_binding_source_rows=3\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact scene gameplay binding source rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_binding_rows=3\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact resolved scene gameplay binding rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_binding_systems=3\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact scene gameplay binding system count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_binding_component_rows=3\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact component-backed scene gameplay binding rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_binding_diagnostics=0\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean scene gameplay binding diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_interaction_rows=3\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact scene gameplay interaction rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_interaction_diagnostics=0\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean scene gameplay interaction diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_scene_interaction_final_session_state=won\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove scene gameplay interaction win state."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_context_rebinding_ready=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready input context rebinding."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_context_rebinding_layers=3\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact input context layer count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_context_rebinding_active_contexts=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact active input context count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_context_rebinding_capture_active=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove active capture context."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_context_rebinding_gameplay_consumed=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove gameplay input consumption."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_profile_overlays_applied=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact input rebinding overlay count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_action_capture_status=captured\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove captured action rebinding input."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_axis_capture_status=captured\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove captured axis rebinding input."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_focus_consumed=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove focused capture consumption."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_focus_retained=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove retained focused capture."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_presentation_rows=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact input rebinding presentation rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_glyph_lookup_keys=5\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact symbolic glyph lookup key count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\binput_rebinding_diagnostics=0\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean input rebinding diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_navmesh_status=success\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove successful gameplay navmesh path."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_navmesh_diagnostic=none\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean gameplay navmesh diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_navmesh_polygons=3\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove 3D gameplay navmesh polygon route count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_navmesh_dynamic_obstacles=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove gameplay navmesh dynamic obstacle participation."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_navmesh_total_cost=[1-9]\d*\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove gameplay navmesh non-zero cost."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_navmesh_readiness_status=ready\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready gameplay navmesh readiness."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_navmesh_readiness_diagnostic=none\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean gameplay navmesh readiness diagnostic."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_navmesh_readiness_diagnostics=0\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean gameplay navmesh readiness diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_navmesh_scene_refs=3\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact gameplay navmesh scene-ref count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_navmesh_points=3\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact gameplay navmesh point path count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_navmesh_visited_polygons=3\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact gameplay navmesh visited polygon count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_status=success\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove successful gameplay navmesh crowd plan."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_diagnostic=none\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean gameplay navmesh crowd diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_rows=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact gameplay navmesh crowd row count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_source_order_ready=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove deterministic gameplay navmesh crowd source ordering."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_agents=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact gameplay navmesh crowd agent count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_route_successes=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove gameplay navmesh crowd route successes."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_avoidance_successes=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove gameplay navmesh crowd avoidance successes."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_applied_neighbors=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove gameplay navmesh crowd applied neighbors."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_dynamic_obstacles=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove gameplay navmesh crowd dynamic obstacle propagation."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_readiness_status=ready\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready gameplay navmesh crowd readiness."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_readiness_diagnostic=none\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean gameplay navmesh crowd readiness diagnostic."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_readiness_diagnostics=0\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean gameplay navmesh crowd readiness diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_readiness_rows=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact gameplay navmesh crowd readiness row count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_readiness_source_order_ready=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove deterministic gameplay navmesh crowd readiness ordering."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_readiness_applied_neighbors=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove gameplay navmesh crowd readiness applied neighbors."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_navigation_crowd_readiness_dynamic_obstacles=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove gameplay navmesh crowd readiness dynamic obstacle propagation."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_local_avoidance_status=success\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove successful gameplay local avoidance."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_local_avoidance_diagnostic=none\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean gameplay local avoidance diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_local_avoidance_steps=$expectedSmokeFrames\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove frame-exact gameplay local avoidance steps."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_local_avoidance_applied_neighbors=$expectedSmokeFrames\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove frame-exact gameplay local avoidance applied neighbors."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_policy_status=constrained\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove constrained gameplay physics policy status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_policy_diagnostic=none\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean gameplay physics policy diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_policy_rows=3\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact gameplay physics policy row count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_policy_dynamic_pushes=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove gameplay physics dynamic push row."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_policy_solid_contacts=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove gameplay physics solid contact row."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_policy_trigger_overlaps=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove gameplay physics trigger overlap row."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_advanced_controller_status=moved\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove moved advanced controller status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_advanced_controller_diagnostic=none\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean advanced controller diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_advanced_controller_movement_rows=5\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact advanced controller movement rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_advanced_controller_platform_rows=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact advanced controller platform rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_advanced_controller_platform_applied=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove advanced controller moving-platform application."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_advanced_controller_constraint_rows=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact advanced controller constraint rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_advanced_controller_replay_changed=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove changed advanced controller replay signature."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_advanced_controller_replay_before_bodies=6\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact advanced controller replay-before body count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_advanced_controller_replay_after_bodies=6\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact advanced controller replay-after body count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_character_dynamics_status=ready\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready character dynamics status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_character_dynamics_diagnostic=none\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean character dynamics diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_character_dynamics_movement_rows=5\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact character dynamics movement rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_character_dynamics_dynamic_pushes=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove character dynamics dynamic push evidence."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_character_dynamics_step_ups=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove character dynamics step-up evidence."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_character_dynamics_walkable_slope_rows=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove character dynamics walkable-slope evidence."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_character_dynamics_ground_probes=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove character dynamics ground-probe evidence."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_character_dynamics_moving_platform_rows=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove character dynamics moving-platform rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_character_dynamics_applied_platform_rows=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove character dynamics applied platform evidence."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_character_dynamics_constraint_rows=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove character dynamics constraint rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_character_dynamics_replay_changed=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove character dynamics replay change."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_character_dynamics_diagnostics=0\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove zero character dynamics diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_constraints_status=solved\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove solved physics constraints status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_constraints_diagnostic=none\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean physics constraints diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_constraints_rows=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact physics constraints row count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_constraints_fixed_rows=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact fixed physics constraint row count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_constraints_linear_axis_rows=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact linear-axis physics constraint row count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_physics_constraints_axis_limit_clamped=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clamped linear-axis physics constraint evidence."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_kinematic_motion_status=constrained\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove constrained kinematic motion status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_kinematic_motion_diagnostic=none\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean kinematic motion diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_kinematic_motion_rows=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact kinematic motion row count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_kinematic_motion_grounded=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove kinematic motion grounding."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_vehicle_status=grounded\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove grounded simple vehicle policy status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_vehicle_diagnostic=none\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean simple vehicle diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_vehicle_wheel_rows=4\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact simple vehicle wheel rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_vehicle_grounded_wheels=4\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact grounded simple vehicle wheels."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_vehicle_wheel_probe_hits=4\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact simple vehicle wheel probe hits."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_systems_vehicle_final_x=1(\.0+)?\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove simple vehicle final x."
    }
}
if ($GameTarget -eq "sample_generated_desktop_runtime_3d_package" -and $requiresNativeUiOverlay) {
    foreach ($field in @("hud_boxes", "ui_overlay_requested", "ui_overlay_status", "ui_overlay_ready", "ui_overlay_sprites_submitted", "ui_overlay_draws")) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not include native UI overlay HUD field: $field"
        }
    }
    $expectedGenerated3dUiOverlaySprites = $expectedSmokeFrames
    $expectedGenerated3dTextureOverlaySprites = 0
    if ($requiresNativeUiTexturedSpriteAtlas) {
        $expectedGenerated3dUiOverlaySprites += $expectedSmokeFrames
        $expectedGenerated3dTextureOverlaySprites += $expectedSmokeFrames
        foreach ($field in @("hud_images", "ui_atlas_metadata_requested", "ui_atlas_metadata_status", "ui_atlas_metadata_pages", "ui_atlas_metadata_bindings", "ui_texture_overlay_requested", "ui_texture_overlay_status", "ui_texture_overlay_atlas_ready", "ui_texture_overlay_sprites_submitted", "ui_texture_overlay_texture_binds", "ui_texture_overlay_draws")) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
                Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not include textured UI atlas field: $field"
            }
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bhud_images=$expectedSmokeFrames\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove frame-exact HUD image submission."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_requested=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove UI atlas metadata was requested."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_status=ready\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready UI atlas metadata."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_pages=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact UI atlas page count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_bindings=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact UI atlas image binding count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_requested=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove texture overlay was requested."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_status=ready\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready texture overlay status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_atlas_ready=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove texture atlas page readiness."
        }
    }
    if ($requiresNativeUiTextGlyphAtlas) {
        $expectedGenerated3dUiOverlaySprites += $expectedSmokeFrames
        $expectedGenerated3dTextureOverlaySprites += $expectedSmokeFrames
        foreach ($field in @("hud_text_glyphs", "text_glyphs_available", "text_glyphs_resolved", "text_glyphs_missing", "text_glyph_sprites_submitted", "ui_atlas_metadata_requested", "ui_atlas_metadata_status", "ui_atlas_metadata_pages", "ui_atlas_metadata_bindings", "ui_atlas_metadata_glyphs", "ui_texture_overlay_requested", "ui_texture_overlay_status", "ui_texture_overlay_atlas_ready", "ui_texture_overlay_sprites_submitted", "ui_texture_overlay_texture_binds", "ui_texture_overlay_draws")) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
                Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not include text glyph UI atlas field: $field"
            }
        }
        foreach ($field in @("hud_text_glyphs", "text_glyphs_available", "text_glyphs_resolved", "text_glyph_sprites_submitted")) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedSmokeFrames\b") {
                Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove frame-exact text glyph field: $field"
            }
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\btext_glyphs_missing=0\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove zero missing text glyphs."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_requested=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove UI glyph atlas metadata was requested."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_status=ready\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready UI glyph atlas metadata."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_pages=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact UI glyph atlas page count."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_bindings=0\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove zero image bindings for the glyph atlas package."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_glyphs=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact UI glyph binding count."
        }
    }
    if ($expectedGenerated3dTextureOverlaySprites -gt 0) {
        foreach ($field in @("ui_texture_overlay_sprites_submitted", "ui_texture_overlay_texture_binds", "ui_texture_overlay_draws")) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedGenerated3dTextureOverlaySprites\b") {
                Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove frame-exact texture overlay field: $field"
            }
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bhud_boxes=$expectedSmokeFrames\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove frame-exact HUD box submission."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_requested=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove native UI overlay was requested."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_status=ready\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready native UI overlay status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_ready=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready native UI overlay."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_sprites_submitted=$expectedGenerated3dUiOverlaySprites\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove frame-exact native UI overlay sprite submission."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_draws=$expectedSmokeFrames\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove frame-exact native UI overlay draw output."
    }
}
if ($GameTarget -eq "sample_generated_desktop_runtime_3d_package" -and
    ($requiresVisible3dProductionProof -or $requiresVulkanVisible3dProductionProof)) {
    foreach ($field in @(
            "visible_3d_status",
            "visible_3d_ready",
            "visible_3d_diagnostics",
            "visible_3d_expected_frames",
            "visible_3d_presented_frames",
            "visible_3d_d3d12_selected",
            "visible_3d_vulkan_selected",
            "visible_3d_null_fallback_used",
            "visible_3d_scene_gpu_ready",
            "visible_3d_postprocess_ready",
            "visible_3d_renderer_quality_ready",
            "visible_3d_playable_ready",
            "visible_3d_ui_overlay_ready"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not include visible 3D production proof field: $field"
        }
    }
    $visible3dReadyFields = @(
        "visible_3d_ready",
        "visible_3d_scene_gpu_ready",
        "visible_3d_postprocess_ready",
        "visible_3d_renderer_quality_ready",
        "visible_3d_playable_ready",
        "visible_3d_ui_overlay_ready"
    )
    if ($requiresVisible3dProductionProof) {
        $visible3dReadyFields += "visible_3d_d3d12_selected"
    }
    if ($requiresVulkanVisible3dProductionProof) {
        $visible3dReadyFields += "visible_3d_vulkan_selected"
    }
    foreach ($field in $visible3dReadyFields) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready visible 3D field: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bvisible_3d_status=ready\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready visible 3D status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bvisible_3d_diagnostics=0\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean visible 3D diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bvisible_3d_expected_frames=$expectedSmokeFrames\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact visible 3D expected frame count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bvisible_3d_presented_frames=$expectedSmokeFrames\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact visible 3D presented frame count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bvisible_3d_null_fallback_used=0\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove visible 3D avoided null fallback."
    }
}
if ($GameTarget -eq "sample_generated_desktop_runtime_3d_package" -and $requiresSceneCollisionPackage) {
    foreach ($field in @(
            "collision_package_status",
            "collision_package_ready",
            "collision_package_diagnostics",
            "collision_package_bodies",
            "collision_package_triggers",
            "collision_package_contacts",
            "collision_package_trigger_overlaps",
            "collision_package_world_ready",
            "collision_query_batch_ready",
            "collision_query_batch_source_order_ready",
            "collision_query_readiness_status",
            "collision_query_readiness_diagnostic",
            "collision_query_readiness_diagnostics",
            "collision_query_batch_rows",
            "collision_query_batch_hits",
            "collision_query_batch_no_hits",
            "collision_query_batch_invalid_requests",
            "collision_query_batch_budget_rejections",
            "gameplay_systems_collision_package_ready"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not include scene collision package field: $field"
        }
    }
    foreach ($field in @(
            "collision_package_ready",
            "collision_package_world_ready",
            "collision_query_batch_ready",
            "collision_query_batch_source_order_ready",
            "gameplay_systems_collision_package_ready"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=1\b") {
            Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready scene collision package field: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_package_status=ready\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready scene collision package status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_package_diagnostics=0\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean scene collision package diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_query_readiness_status=ready\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove ready scene collision query readiness."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_query_readiness_diagnostic=none\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean scene collision query readiness diagnostic."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_query_readiness_diagnostics=0\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove clean scene collision query readiness diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_package_bodies=3\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact scene collision body count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_package_triggers=1\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact scene collision trigger count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_package_contacts=[1-9]\d*\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove non-zero scene collision contacts."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_package_trigger_overlaps=[1-9]\d*\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove non-zero scene collision trigger overlaps."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_query_batch_rows=6\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact scene collision query batch row count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_query_batch_hits=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact scene collision query batch hit count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_query_batch_no_hits=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact scene collision query batch no-hit count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_query_batch_invalid_requests=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact scene collision query batch invalid-request count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bcollision_query_batch_budget_rejections=2\b") {
        Write-Error "Installed sample_generated_desktop_runtime_3d_package smoke status line did not prove exact scene collision query batch budget rejection count."
    }
}
if ($requiresNative2dSprites) {
    foreach ($field in @(
            "native_2d_sprites_requested",
            "native_2d_sprites_status",
            "native_2d_sprites_ready",
            "native_2d_sprites_submitted",
            "native_2d_textured_sprites_submitted",
            "native_2d_texture_atlas_ready",
            "native_2d_texture_binds",
            "native_2d_draws",
            "native_2d_textured_draws",
            "native_2d_sprite_batches_executed",
            "native_2d_sprite_batch_sprites_executed",
            "native_2d_sprite_batch_textured_sprites_executed",
            "native_2d_sprite_batch_texture_binds"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include native 2D sprite field: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprites_requested=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D sprite overlay was requested."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprites_status=ready\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove ready native 2D sprite overlay status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprites_ready=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove ready native 2D sprite overlay."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprites_submitted=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D sprite submission."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_textured_sprites_submitted=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D textured sprite submission."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_texture_atlas_ready=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove ready native 2D sprite atlas texture."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_texture_binds=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D sprite texture binding."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_draws=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D sprite draw output."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_textured_draws=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D textured sprite draw output."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprite_batches_executed=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D sprite batch execution."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprite_batch_sprites_executed=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D sprite batch sprite execution."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprite_batch_textured_sprites_executed=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D textured sprite batch execution."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bnative_2d_sprite_batch_texture_binds=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove native 2D sprite batch texture binding."
    }
}
if ($requiresSpriteAnimation) {
    foreach ($field in @(
            "sprite_animation_frames_sampled",
            "sprite_animation_frames_applied",
            "sprite_animation_selected_frame_sum",
            "sprite_animation_diagnostics",
            "sprite_flipbook_ticks",
            "sprite_flipbook_frames_sampled",
            "sprite_flipbook_frames_applied",
            "sprite_flipbook_selected_frame_sum",
            "sprite_flipbook_diagnostics",
            "sprite_flipbook_direction_sets",
            "sprite_flipbook_event_rows",
            "sprite_flipbook_playback_modes",
            "sprite_flipbook_gameplay_state_rows",
            "sprite_flipbook_events_sampled",
            "sprite_flipbook_playback_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include sprite animation field: $field"
        }
    }
    foreach ($field in @(
            "sprite_animation_frames_sampled",
            "sprite_animation_frames_applied",
            "sprite_animation_selected_frame_sum",
            "sprite_flipbook_frames_sampled",
            "sprite_flipbook_frames_applied",
            "sprite_flipbook_selected_frame_sum",
            "sprite_flipbook_direction_sets",
            "sprite_flipbook_event_rows",
            "sprite_flipbook_playback_modes",
            "sprite_flipbook_gameplay_state_rows",
            "sprite_flipbook_events_sampled"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove sprite animation count: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsprite_flipbook_ticks=$expectedSmokeFrames\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove frame-exact sprite flipbook ticks."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsprite_animation_diagnostics=0\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove clean sprite animation diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsprite_flipbook_diagnostics=0\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove clean sprite flipbook diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsprite_flipbook_playback_diagnostics=0\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove clean sprite flipbook playback diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsprite_flipbook_direction_sets=2\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove sprite flipbook direction-set rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsprite_flipbook_event_rows=2\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove sprite flipbook event rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsprite_flipbook_playback_modes=2\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove sprite flipbook playback mode rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsprite_flipbook_gameplay_state_rows=2\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove sprite flipbook gameplay-state rows."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsprite_flipbook_events_sampled=$expectedSmokeFrames\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove frame-exact sprite flipbook events."
    }
}
if ($requiresTilemapRuntimeUx) {
    foreach ($field in @(
            "tilemap_layers",
            "tilemap_visible_layers",
            "tilemap_tiles",
            "tilemap_non_empty_cells",
            "tilemap_cells_sampled",
            "tilemap_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include tilemap runtime UX field: $field"
        }
    }
    foreach ($field in @(
            "tilemap_layers",
            "tilemap_visible_layers",
            "tilemap_tiles",
            "tilemap_non_empty_cells",
            "tilemap_cells_sampled"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove tilemap runtime UX count: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\btilemap_diagnostics=0\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove clean tilemap runtime UX diagnostics."
    }
}
if ($requiresProductionTileRenderer) {
    foreach ($field in @(
            "tile_chunk_renderer_status",
            "tile_chunk_renderer_ready",
            "tile_chunk_renderer_visible_cells",
            "tile_chunk_renderer_opaque_cells",
            "tile_chunk_renderer_transparent_cells",
            "tile_chunk_renderer_sprite_rows",
            "tile_chunk_renderer_draw_rows",
            "tile_chunk_renderer_texture_binds",
            "tile_chunk_renderer_dirty_rebuild_rows",
            "tile_chunk_renderer_dirty_rebuild_cells",
            "tile_chunk_renderer_light_rows",
            "tile_chunk_renderer_changed_light_cells",
            "tile_chunk_renderer_diagnostics",
            "tile_chunk_renderer_backend_submission_invoked",
            "tile_chunk_renderer_native_texture_ownership_invoked"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include production tile renderer field: $field"
        }
    }
    $expectedTileRendererFields = [ordered]@{
        "tile_chunk_renderer_status" = "ready"
        "tile_chunk_renderer_ready" = "1"
        "tile_chunk_renderer_visible_cells" = "3"
        "tile_chunk_renderer_opaque_cells" = "2"
        "tile_chunk_renderer_transparent_cells" = "1"
        "tile_chunk_renderer_sprite_rows" = "3"
        "tile_chunk_renderer_draw_rows" = "2"
        "tile_chunk_renderer_texture_binds" = "1"
        "tile_chunk_renderer_dirty_rebuild_rows" = "2"
        "tile_chunk_renderer_dirty_rebuild_cells" = "5"
        "tile_chunk_renderer_light_rows" = "3"
        "tile_chunk_renderer_changed_light_cells" = "1"
        "tile_chunk_renderer_diagnostics" = "0"
        "tile_chunk_renderer_backend_submission_invoked" = "0"
        "tile_chunk_renderer_native_texture_ownership_invoked" = "0"
    }
    foreach ($expectedTileRendererField in $expectedTileRendererFields.GetEnumerator()) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$($expectedTileRendererField.Key)=$($expectedTileRendererField.Value)\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove production tile renderer field: $($expectedTileRendererField.Key)=$($expectedTileRendererField.Value)"
        }
    }
}
if ($requiresPackageStreamingSafePoint) {
    foreach ($field in @(
            "package_streaming_status",
            "package_streaming_ready",
            "package_streaming_resident_bytes",
            "package_streaming_resident_budget_bytes",
            "package_streaming_committed_records",
            "package_streaming_stale_handles",
            "package_streaming_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include package streaming field: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpackage_streaming_status=committed\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove committed package streaming status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpackage_streaming_ready=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove ready package streaming safe-point execution."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpackage_streaming_resident_bytes=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove positive package streaming resident bytes."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpackage_streaming_committed_records=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove committed package streaming records."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpackage_streaming_diagnostics=0\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove clean package streaming diagnostics."
    }
}
if ($requiresWorldRegionStreaming) {
    foreach ($field in @(
            "world_region_streaming_status",
            "world_region_streaming_ready",
            "world_region_streaming_plan_rows",
            "world_region_streaming_load_rows",
            "world_region_streaming_keep_rows",
            "world_region_streaming_unload_rows",
            "world_region_streaming_safe_point_rows",
            "world_region_streaming_committed",
            "world_region_streaming_committed_rows",
            "world_region_streaming_reviewed_package_adoptions",
            "world_region_streaming_projected_regions",
            "world_region_streaming_projected_bytes",
            "world_region_streaming_budget_bytes",
            "world_region_streaming_missing_region_diagnostics",
            "world_region_streaming_safe_point_diagnostics",
            "world_region_streaming_large_scene_readiness_status",
            "world_region_streaming_large_scene_readiness_diagnostic",
            "world_region_streaming_large_scene_readiness_diagnostics",
            "world_region_streaming_navigation_resident_regions",
            "world_region_streaming_navigation_missing_resident_regions",
            "world_region_streaming_navigation_path_cache_ready"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include world-region streaming field: $field"
        }
    }
    foreach ($field in @(
            "world_region_streaming_ready",
            "world_region_streaming_committed",
            "world_region_streaming_reviewed_package_adoptions",
            "world_region_streaming_plan_rows",
            "world_region_streaming_safe_point_rows",
            "world_region_streaming_projected_regions",
            "world_region_streaming_projected_bytes",
            "world_region_streaming_budget_bytes",
            "world_region_streaming_missing_region_diagnostics",
            "world_region_streaming_navigation_resident_regions"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove positive world-region streaming field: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_status=completed\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove completed world-region streaming status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_load_rows=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove exact world-region load row count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_keep_rows=2\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove exact world-region keep row count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_unload_rows=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove exact world-region unload row count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_safe_point_rows=4\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove exact world-region safe-point row count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_committed=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove committed world-region streaming evidence."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_committed_rows=2\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove exact world-region committed row count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_reviewed_package_adoptions=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove exact world-region package adoption count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_projected_regions=2\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove exact world-region projected region count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_missing_region_diagnostics=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove exact world-region missing-region diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_safe_point_diagnostics=0\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove clean world-region streaming safe-point diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_large_scene_readiness_status=ready\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove ready world-region large-scene readiness."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_large_scene_readiness_diagnostic=none\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove clean world-region large-scene readiness diagnostic."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_large_scene_readiness_diagnostics=0\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove zero world-region large-scene readiness diagnostics."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_navigation_resident_regions=2\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove exact world-region navigation residency count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_navigation_missing_resident_regions=0\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove clean world-region navigation missing-residency count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bworld_region_streaming_navigation_path_cache_ready=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove ready world-region navigation path cache evidence."
    }
}
if ($requiresEntityScaleCulling) {
    foreach ($field in @(
            "entity_scale_culling_status",
            "entity_scale_culling_ready",
            "entity_scale_culling_rows",
            "entity_scale_culling_visible_rows",
            "entity_scale_culling_culled_rows",
            "entity_scale_culling_lod_rows",
            "entity_scale_culling_priority_update_rows",
            "entity_scale_culling_normal_update_rows",
            "entity_scale_culling_background_update_rows",
            "entity_scale_culling_projected_draw_cost",
            "entity_scale_culling_projected_update_cost",
            "entity_scale_culling_budget_protected_rows",
            "entity_scale_culling_diagnostics",
            "entity_scale_culling_budget_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include entity scale/culling field: $field"
        }
    }
    foreach ($field in @(
            "entity_scale_culling_ready",
            "entity_scale_culling_rows",
            "entity_scale_culling_visible_rows",
            "entity_scale_culling_culled_rows",
            "entity_scale_culling_lod_rows",
            "entity_scale_culling_priority_update_rows",
            "entity_scale_culling_normal_update_rows",
            "entity_scale_culling_background_update_rows",
            "entity_scale_culling_projected_draw_cost",
            "entity_scale_culling_projected_update_cost",
            "entity_scale_culling_budget_protected_rows",
            "entity_scale_culling_budget_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove positive entity scale/culling field: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bentity_scale_culling_status=planned\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove planned entity scale/culling status."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bentity_scale_culling_diagnostics=0\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove clean entity scale/culling diagnostics."
    }
}
if ($requiresScriptingSandboxPolicy) {
    foreach ($field in @(
            "scripting_sandbox_status",
            "scripting_sandbox_ready",
            "scripting_sandbox_entrypoint_rows",
            "scripting_sandbox_permission_rows",
            "scripting_sandbox_allowed_permission_rows",
            "scripting_sandbox_denied_permission_rows",
            "scripting_sandbox_rejected_unsafe_capability_rows",
            "scripting_sandbox_unsupported_host_api_diagnostics",
            "scripting_sandbox_budget_rows",
            "scripting_sandbox_projected_instruction_budget",
            "scripting_sandbox_projected_memory_budget_bytes",
            "scripting_sandbox_budget_diagnostics",
            "scripting_sandbox_replay_seed_rows",
            "scripting_sandbox_replay_seed_sum",
            "scripting_sandbox_diagnostics",
            "scripting_sandbox_execution_status",
            "scripting_sandbox_execution_ready",
            "scripting_sandbox_execution_dispatches",
            "scripting_sandbox_execution_host_api_calls",
            "scripting_sandbox_execution_replay_signature",
            "scripting_sandbox_execution_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include scripting sandbox field: $field"
        }
    }
    $expectedScriptingSandboxFields = @{
        "scripting_sandbox_status" = "planned"
        "scripting_sandbox_ready" = "1"
        "scripting_sandbox_entrypoint_rows" = "2"
        "scripting_sandbox_permission_rows" = "7"
        "scripting_sandbox_allowed_permission_rows" = "7"
        "scripting_sandbox_denied_permission_rows" = "5"
        "scripting_sandbox_rejected_unsafe_capability_rows" = "6"
        "scripting_sandbox_unsupported_host_api_diagnostics" = "1"
        "scripting_sandbox_budget_rows" = "2"
        "scripting_sandbox_projected_instruction_budget" = "1800"
        "scripting_sandbox_projected_memory_budget_bytes" = "6144"
        "scripting_sandbox_budget_diagnostics" = "2"
        "scripting_sandbox_replay_seed_rows" = "2"
        "scripting_sandbox_replay_seed_sum" = "3003"
        "scripting_sandbox_diagnostics" = "0"
        "scripting_sandbox_execution_status" = "completed"
        "scripting_sandbox_execution_ready" = "1"
        "scripting_sandbox_execution_dispatches" = "1"
        "scripting_sandbox_execution_host_api_calls" = "1"
        "scripting_sandbox_execution_diagnostics" = "0"
    }
    foreach ($field in $expectedScriptingSandboxFields.Keys) {
        $expectedValue = $expectedScriptingSandboxFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove scripting sandbox field $field=$expectedValue."
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bscripting_sandbox_execution_replay_signature=[1-9][0-9]*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove a non-zero scripting sandbox execution replay signature."
    }
}
if ($requiresNetworkingFoundationPolicy) {
    foreach ($field in @(
            "networking_foundation_status",
            "networking_foundation_ready",
            "networking_foundation_session_rows",
            "networking_foundation_transport_rows",
            "networking_foundation_channel_rows",
            "networking_foundation_rejected_unsafe_transport_rows",
            "networking_foundation_replay_prerequisite_rows",
            "networking_foundation_replay_seed_sum",
            "networking_foundation_remote_session_rows",
            "networking_foundation_secure_remote_session_rows",
            "networking_foundation_security_diagnostics",
            "networking_foundation_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include networking foundation field: $field"
        }
    }
    $expectedNetworkingFoundationFields = @{
        "networking_foundation_status" = "planned"
        "networking_foundation_ready" = "1"
        "networking_foundation_session_rows" = "2"
        "networking_foundation_transport_rows" = "4"
        "networking_foundation_channel_rows" = "3"
        "networking_foundation_rejected_unsafe_transport_rows" = "3"
        "networking_foundation_replay_prerequisite_rows" = "2"
        "networking_foundation_replay_seed_sum" = "49"
        "networking_foundation_remote_session_rows" = "1"
        "networking_foundation_secure_remote_session_rows" = "1"
        "networking_foundation_security_diagnostics" = "2"
        "networking_foundation_diagnostics" = "0"
    }
    foreach ($field in $expectedNetworkingFoundationFields.Keys) {
        $expectedValue = $expectedNetworkingFoundationFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove networking foundation field $field=$expectedValue."
        }
    }
}
if ($requiresSimulationOrchestration) {
    foreach ($field in @(
            "simulation_orchestration_status",
            "simulation_orchestration_ready",
            "simulation_orchestration_available_steps",
            "simulation_orchestration_planned_steps",
            "simulation_orchestration_step_rows",
            "simulation_orchestration_command_rows",
            "simulation_orchestration_command_playback_rows",
            "simulation_orchestration_consumed_time_us",
            "simulation_orchestration_remaining_time_us",
            "simulation_orchestration_budget_limited_status",
            "simulation_orchestration_budget_limited_available_steps",
            "simulation_orchestration_budget_limited_planned_steps",
            "simulation_orchestration_budget_limited_remaining_time_us",
            "simulation_orchestration_invalid_command_diagnostics",
            "simulation_orchestration_diagnostics",
            "simulation_management_status",
            "simulation_management_ready",
            "simulation_management_tick_count",
            "simulation_management_resource_balance_rows",
            "simulation_management_job_rows",
            "simulation_management_job_assignment_rows",
            "simulation_management_logistics_links",
            "simulation_management_logistics_transfer_rows",
            "simulation_management_logistics_scheduled_transfer_rows",
            "simulation_management_economy_summary_rows",
            "simulation_management_population_need_rows",
            "simulation_management_need_deficit_rows",
            "simulation_management_schedule_rows",
            "simulation_management_save_review_rows",
            "simulation_management_save_review_repairable_rows",
            "simulation_management_dashboard_rows",
            "simulation_management_replay_hash",
            "simulation_management_invoked_economy_execution",
            "simulation_management_invoked_save_io",
            "simulation_management_invoked_runtime_ui",
            "simulation_management_invoked_package_io",
            "simulation_management_diagnostics",
            "gameplay_runtime_scheduler_status",
            "gameplay_runtime_scheduler_ready",
            "gameplay_runtime_scheduler_available_steps",
            "gameplay_runtime_scheduler_steps",
            "gameplay_runtime_scheduler_system_rows",
            "gameplay_runtime_scheduler_command_rows",
            "gameplay_runtime_scheduler_consumed_time_us",
            "gameplay_runtime_scheduler_remaining_time_us",
            "gameplay_runtime_scheduler_budget_limited",
            "gameplay_runtime_scheduler_replay_hash",
            "gameplay_runtime_scheduler_diagnostics",
            "world_entity_model_status",
            "world_entity_model_ready",
            "world_entity_model_entities",
            "world_entity_model_components",
            "world_entity_model_region_ownership_rows",
            "world_entity_model_lifecycle_rows",
            "world_entity_model_persistence_rows",
            "world_entity_model_streaming_region_rows",
            "world_entity_model_spawn_rows",
            "world_entity_model_move_rows",
            "world_entity_model_despawn_rows",
            "world_entity_model_duplicate_entity_diagnostics",
            "world_entity_model_bridge_rejection_status",
            "world_entity_model_bridge_rejection_diagnostics",
            "world_entity_model_bridge_rejection_persistence_rows",
            "world_entity_model_bridge_rejection_streaming_region_rows",
            "world_entity_model_bridge_rejection_streaming_diagnostics_present",
            "world_entity_model_bridge_rejection_fail_closed",
            "world_entity_model_diagnostics",
            "addressable_content_status",
            "addressable_content_ready",
            "addressable_content_address_rows",
            "addressable_content_dependency_rows",
            "addressable_content_load_rows",
            "addressable_content_release_rows",
            "addressable_content_refcount_rows",
            "addressable_content_resident_bytes",
            "addressable_content_resident_budget_bytes",
            "addressable_content_budget_rejection_status",
            "addressable_content_budget_rejection_diagnostics",
            "addressable_content_package_io",
            "addressable_content_async_execution",
            "addressable_content_committed",
            "addressable_content_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include simulation orchestration field: $field"
        }
    }
    $expectedSimulationOrchestrationFields = @{
        "simulation_orchestration_status" = "planned"
        "simulation_orchestration_ready" = "1"
        "simulation_orchestration_available_steps" = "3"
        "simulation_orchestration_planned_steps" = "3"
        "simulation_orchestration_step_rows" = "3"
        "simulation_orchestration_command_rows" = "3"
        "simulation_orchestration_command_playback_rows" = "3"
        "simulation_orchestration_consumed_time_us" = "49998"
        "simulation_orchestration_remaining_time_us" = "2"
        "simulation_orchestration_budget_limited_status" = "budget_limited"
        "simulation_orchestration_budget_limited_available_steps" = "6"
        "simulation_orchestration_budget_limited_planned_steps" = "2"
        "simulation_orchestration_budget_limited_remaining_time_us" = "133334"
        "simulation_orchestration_invalid_command_diagnostics" = "4"
        "simulation_orchestration_diagnostics" = "0"
    }
    foreach ($field in $expectedSimulationOrchestrationFields.Keys) {
        $expectedValue = $expectedSimulationOrchestrationFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove simulation orchestration field $field=$expectedValue."
        }
    }
    $expectedSimulationManagementFields = @{
        "simulation_management_status" = "ready"
        "simulation_management_ready" = "1"
        "simulation_management_tick_count" = "240"
        "simulation_management_resource_balance_rows" = "4"
        "simulation_management_job_rows" = "2"
        "simulation_management_job_assignment_rows" = "1"
        "simulation_management_logistics_links" = "2"
        "simulation_management_logistics_transfer_rows" = "2"
        "simulation_management_logistics_scheduled_transfer_rows" = "1"
        "simulation_management_economy_summary_rows" = "1"
        "simulation_management_population_need_rows" = "2"
        "simulation_management_need_deficit_rows" = "1"
        "simulation_management_schedule_rows" = "2"
        "simulation_management_save_review_rows" = "2"
        "simulation_management_save_review_repairable_rows" = "1"
        "simulation_management_dashboard_rows" = "7"
        "simulation_management_invoked_economy_execution" = "0"
        "simulation_management_invoked_save_io" = "0"
        "simulation_management_invoked_runtime_ui" = "0"
        "simulation_management_invoked_package_io" = "0"
        "simulation_management_diagnostics" = "0"
    }
    foreach ($field in $expectedSimulationManagementFields.Keys) {
        $expectedValue = $expectedSimulationManagementFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove simulation management field $field=$expectedValue."
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsimulation_management_replay_hash=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove positive simulation management replay hash."
    }
    $expectedGameplayRuntimeSchedulerFields = @{
        "gameplay_runtime_scheduler_status" = "budget_limited"
        "gameplay_runtime_scheduler_ready" = "1"
        "gameplay_runtime_scheduler_available_steps" = "3"
        "gameplay_runtime_scheduler_steps" = "2"
        "gameplay_runtime_scheduler_system_rows" = "6"
        "gameplay_runtime_scheduler_command_rows" = "2"
        "gameplay_runtime_scheduler_consumed_time_us" = "33332"
        "gameplay_runtime_scheduler_remaining_time_us" = "16666"
        "gameplay_runtime_scheduler_budget_limited" = "1"
        "gameplay_runtime_scheduler_diagnostics" = "0"
    }
    foreach ($field in $expectedGameplayRuntimeSchedulerFields.Keys) {
        $expectedValue = $expectedGameplayRuntimeSchedulerFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove gameplay runtime scheduler field $field=$expectedValue."
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgameplay_runtime_scheduler_replay_hash=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove positive gameplay runtime scheduler replay hash."
    }
    $expectedWorldEntityModelFields = @{
        "world_entity_model_status" = "ready"
        "world_entity_model_ready" = "1"
        "world_entity_model_entities" = "3"
        "world_entity_model_components" = "4"
        "world_entity_model_region_ownership_rows" = "3"
        "world_entity_model_lifecycle_rows" = "3"
        "world_entity_model_persistence_rows" = "1"
        "world_entity_model_streaming_region_rows" = "1"
        "world_entity_model_spawn_rows" = "1"
        "world_entity_model_move_rows" = "1"
        "world_entity_model_despawn_rows" = "1"
        "world_entity_model_duplicate_entity_diagnostics" = "1"
        "world_entity_model_bridge_rejection_status" = "invalid_request"
        "world_entity_model_bridge_rejection_diagnostics" = "5"
        "world_entity_model_bridge_rejection_persistence_rows" = "0"
        "world_entity_model_bridge_rejection_streaming_region_rows" = "0"
        "world_entity_model_bridge_rejection_streaming_diagnostics_present" = "1"
        "world_entity_model_bridge_rejection_fail_closed" = "1"
        "world_entity_model_diagnostics" = "0"
    }
    foreach ($field in $expectedWorldEntityModelFields.Keys) {
        $expectedValue = $expectedWorldEntityModelFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove world entity model field $field=$expectedValue."
        }
    }
    $expectedAddressableContentFields = @{
        "addressable_content_status" = "ready"
        "addressable_content_ready" = "1"
        "addressable_content_address_rows" = "6"
        "addressable_content_dependency_rows" = "7"
        "addressable_content_load_rows" = "3"
        "addressable_content_release_rows" = "1"
        "addressable_content_refcount_rows" = "4"
        "addressable_content_resident_bytes" = "1626"
        "addressable_content_resident_budget_bytes" = "16777216"
        "addressable_content_budget_rejection_status" = "budget_limited"
        "addressable_content_budget_rejection_diagnostics" = "1"
        "addressable_content_package_io" = "0"
        "addressable_content_async_execution" = "0"
        "addressable_content_committed" = "0"
        "addressable_content_diagnostics" = "0"
    }
    foreach ($field in $expectedAddressableContentFields.Keys) {
        $expectedValue = $expectedAddressableContentFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove addressable content field $field=$expectedValue."
        }
    }
}
if ($requiresGameplayAuthoringReview) {
    foreach ($field in @(
            "gameplay_authoring_review_status",
            "gameplay_authoring_review_ready",
            "gameplay_authoring_review_feature_rows",
            "gameplay_authoring_review_accepted_rows",
            "gameplay_authoring_review_mutation_ledger_rows",
            "gameplay_authoring_review_remediation_rows",
            "gameplay_authoring_review_missing_required_capability_diagnostics",
            "gameplay_authoring_review_missing_validation_recipe_diagnostics",
            "gameplay_authoring_review_missing_package_evidence_diagnostics",
            "gameplay_authoring_review_unsupported_claim_diagnostics",
            "gameplay_authoring_review_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include gameplay authoring review field: $field"
        }
    }
    $expectedGameplayAuthoringReviewFields = @{
        "gameplay_authoring_review_status" = "ready"
        "gameplay_authoring_review_ready" = "1"
        "gameplay_authoring_review_feature_rows" = "4"
        "gameplay_authoring_review_accepted_rows" = "4"
        "gameplay_authoring_review_mutation_ledger_rows" = "4"
        "gameplay_authoring_review_remediation_rows" = "4"
        "gameplay_authoring_review_missing_required_capability_diagnostics" = "1"
        "gameplay_authoring_review_missing_validation_recipe_diagnostics" = "1"
        "gameplay_authoring_review_missing_package_evidence_diagnostics" = "1"
        "gameplay_authoring_review_unsupported_claim_diagnostics" = "1"
        "gameplay_authoring_review_diagnostics" = "0"
    }
    foreach ($field in $expectedGameplayAuthoringReviewFields.Keys) {
        $expectedValue = $expectedGameplayAuthoringReviewFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove gameplay authoring review field $field=$expectedValue."
        }
    }
}
if ($requiresSandboxAuthoringReview) {
    foreach ($field in @(
            "sandbox_authoring_review_status",
            "sandbox_authoring_review_ready",
            "sandbox_authoring_review_tile_definition_rows",
            "sandbox_authoring_review_palette_brush_rows",
            "sandbox_authoring_review_chunk_template_rows",
            "sandbox_authoring_review_procedural_seed_rows",
            "sandbox_authoring_review_changed_files",
            "sandbox_authoring_review_tilemap_package_changed_files",
            "sandbox_authoring_review_package_dependency_edges",
            "sandbox_authoring_review_preview_hash",
            "sandbox_authoring_review_external_image_decoding_invoked",
            "sandbox_authoring_review_external_download_invoked",
            "sandbox_authoring_review_importer_plugin_invoked",
            "sandbox_authoring_review_package_apply_invoked",
            "sandbox_authoring_review_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include sandbox authoring review field: $field"
        }
    }
    $expectedSandboxAuthoringReviewFields = @{
        "sandbox_authoring_review_status" = "ready"
        "sandbox_authoring_review_ready" = "1"
        "sandbox_authoring_review_tile_definition_rows" = "2"
        "sandbox_authoring_review_palette_brush_rows" = "1"
        "sandbox_authoring_review_chunk_template_rows" = "1"
        "sandbox_authoring_review_procedural_seed_rows" = "1"
        "sandbox_authoring_review_changed_files" = "3"
        "sandbox_authoring_review_tilemap_package_changed_files" = "2"
        "sandbox_authoring_review_package_dependency_edges" = "1"
        "sandbox_authoring_review_external_image_decoding_invoked" = "0"
        "sandbox_authoring_review_external_download_invoked" = "0"
        "sandbox_authoring_review_importer_plugin_invoked" = "0"
        "sandbox_authoring_review_package_apply_invoked" = "0"
        "sandbox_authoring_review_diagnostics" = "0"
    }
    foreach ($field in $expectedSandboxAuthoringReviewFields.Keys) {
        $expectedValue = [regex]::Escape($expectedSandboxAuthoringReviewFields[$field])
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove sandbox authoring review field $field=$($expectedSandboxAuthoringReviewFields[$field])."
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bsandbox_authoring_review_preview_hash=[1-9][0-9]*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove sandbox authoring review nonzero preview hash."
    }
}
if ($requiresProductionAuthoringWorkflows) {
    foreach ($field in @(
            "production_authoring_workflow_status",
            "production_authoring_workflow_ready",
            "production_authoring_workflow_rows",
            "production_authoring_workflow_accepted_rows",
            "production_authoring_workflow_mutation_ledger_rows",
            "production_authoring_workflow_validation_repair_rows",
            "production_authoring_workflow_shared_surface_mutation_diagnostics",
            "production_authoring_workflow_arbitrary_shell_diagnostics",
            "production_authoring_workflow_cooked_package_mutation_diagnostics",
            "production_authoring_workflow_native_backend_term_diagnostics",
            "production_authoring_workflow_invalid_target_path_diagnostics",
            "production_authoring_workflow_invoked_file_mutation",
            "production_authoring_workflow_invoked_package_io",
            "production_authoring_workflow_invoked_command_execution",
            "production_authoring_workflow_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include production authoring workflow field: $field"
        }
    }
    $expectedProductionAuthoringWorkflowFields = @{
        "production_authoring_workflow_status" = "ready"
        "production_authoring_workflow_ready" = "1"
        "production_authoring_workflow_rows" = "6"
        "production_authoring_workflow_accepted_rows" = "6"
        "production_authoring_workflow_mutation_ledger_rows" = "6"
        "production_authoring_workflow_validation_repair_rows" = "1"
        "production_authoring_workflow_shared_surface_mutation_diagnostics" = "1"
        "production_authoring_workflow_arbitrary_shell_diagnostics" = "1"
        "production_authoring_workflow_cooked_package_mutation_diagnostics" = "1"
        "production_authoring_workflow_native_backend_term_diagnostics" = "1"
        "production_authoring_workflow_invalid_target_path_diagnostics" = "1"
        "production_authoring_workflow_invoked_file_mutation" = "0"
        "production_authoring_workflow_invoked_package_io" = "0"
        "production_authoring_workflow_invoked_command_execution" = "0"
        "production_authoring_workflow_diagnostics" = "0"
    }
    foreach ($field in $expectedProductionAuthoringWorkflowFields.Keys) {
        $expectedValue = $expectedProductionAuthoringWorkflowFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove production authoring workflow field $field=$expectedValue."
        }
    }
}
if ($requiresRuntimeUiWorkbench) {
    foreach ($field in @(
            "runtime_ui_workbench_status",
            "runtime_ui_workbench_ready",
            "runtime_ui_workbench_panels",
            "runtime_ui_workbench_table_columns",
            "runtime_ui_workbench_table_rows",
            "runtime_ui_workbench_graph_series",
            "runtime_ui_workbench_item_rows",
            "runtime_ui_workbench_inventory_rows",
            "runtime_ui_workbench_equipment_rows",
            "runtime_ui_workbench_shop_rows",
            "runtime_ui_workbench_text_inputs",
            "runtime_ui_workbench_platform_text_input_requests",
            "runtime_ui_workbench_focus_edges",
            "runtime_ui_workbench_localization_refs",
            "runtime_ui_workbench_localization_identity_ready",
            "runtime_ui_workbench_accessibility_refs",
            "runtime_ui_workbench_accessibility_identity_ready",
            "runtime_ui_workbench_renderer_submission",
            "runtime_ui_workbench_text_shaping",
            "runtime_ui_workbench_font_rasterization",
            "runtime_ui_workbench_ime_sessions",
            "runtime_ui_workbench_accessibility_bridge",
            "runtime_ui_workbench_image_decoding",
            "runtime_ui_workbench_native_platform",
            "runtime_ui_workbench_diagnostics"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include runtime UI workbench field: $field"
        }
    }
    $expectedRuntimeUiWorkbenchFields = @{
        "runtime_ui_workbench_status" = "ready"
        "runtime_ui_workbench_ready" = "1"
        "runtime_ui_workbench_panels" = "5"
        "runtime_ui_workbench_table_columns" = "3"
        "runtime_ui_workbench_table_rows" = "2"
        "runtime_ui_workbench_graph_series" = "2"
        "runtime_ui_workbench_item_rows" = "3"
        "runtime_ui_workbench_inventory_rows" = "1"
        "runtime_ui_workbench_equipment_rows" = "1"
        "runtime_ui_workbench_shop_rows" = "1"
        "runtime_ui_workbench_text_inputs" = "1"
        "runtime_ui_workbench_platform_text_input_requests" = "1"
        "runtime_ui_workbench_focus_edges" = "4"
        "runtime_ui_workbench_localization_refs" = "17"
        "runtime_ui_workbench_localization_identity_ready" = "1"
        "runtime_ui_workbench_accessibility_refs" = "11"
        "runtime_ui_workbench_accessibility_identity_ready" = "1"
        "runtime_ui_workbench_renderer_submission" = "0"
        "runtime_ui_workbench_text_shaping" = "0"
        "runtime_ui_workbench_font_rasterization" = "0"
        "runtime_ui_workbench_ime_sessions" = "0"
        "runtime_ui_workbench_accessibility_bridge" = "0"
        "runtime_ui_workbench_image_decoding" = "0"
        "runtime_ui_workbench_native_platform" = "0"
        "runtime_ui_workbench_diagnostics" = "0"
    }
    foreach ($field in $expectedRuntimeUiWorkbenchFields.Keys) {
        $expectedValue = $expectedRuntimeUiWorkbenchFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove runtime UI workbench field $field=$expectedValue."
        }
    }
}
if ($requiresRuntimeUiProductionStack) {
    foreach ($field in @(
            "runtime_ui_production_stack_status",
            "runtime_ui_production_stack_reviewed",
            "runtime_ui_production_stack_ready",
            "runtime_ui_production_stack_rows",
            "runtime_ui_production_stack_ready_rows",
            "runtime_ui_production_stack_host_gated_rows",
            "runtime_ui_production_stack_text_contract_ready",
            "runtime_ui_production_stack_selected_package_evidence_ready",
            "runtime_ui_production_stack_production_ready",
            "runtime_ui_production_stack_requires_ime_host_evidence",
            "runtime_ui_production_stack_ime_host_evidence",
            "runtime_ui_production_stack_ime_session_rows",
            "runtime_ui_production_stack_ime_composition_rows",
            "runtime_ui_production_stack_ime_candidate_rows",
            "runtime_ui_production_stack_ime_text_area_cursor_rows",
            "runtime_ui_production_stack_ime_committed_text_rows",
            "runtime_ui_production_stack_ime_clipboard_rows",
            "runtime_ui_production_stack_ime_platform_adapter_proof_rows",
            "runtime_ui_production_stack_ime_platform_host_gate_rows",
            "runtime_ui_production_stack_ime_platform_parity_ready",
            "runtime_ui_production_stack_requires_accessibility_host_evidence",
            "runtime_ui_production_stack_accessibility_host_evidence",
            "runtime_ui_production_stack_accessibility_role_rows",
            "runtime_ui_production_stack_accessibility_name_rows",
            "runtime_ui_production_stack_accessibility_description_rows",
            "runtime_ui_production_stack_accessibility_state_rows",
            "runtime_ui_production_stack_accessibility_focus_rows",
            "runtime_ui_production_stack_accessibility_action_rows",
            "runtime_ui_production_stack_accessibility_relationship_rows",
            "runtime_ui_production_stack_accessibility_live_region_rows",
            "runtime_ui_production_stack_accessibility_keyboard_pattern_rows",
            "runtime_ui_production_stack_accessibility_publication_status_rows",
            "runtime_ui_production_stack_accessibility_uia_host_gate_rows",
            "runtime_ui_production_stack_accessibility_platform_host_gate_rows",
            "runtime_ui_production_stack_accessibility_platform_parity_ready",
            "runtime_ui_production_stack_invoked_text_shaping",
            "runtime_ui_production_stack_invoked_font_rasterization",
            "runtime_ui_production_stack_invoked_ime",
            "runtime_ui_production_stack_invoked_accessibility_bridge",
            "runtime_ui_production_stack_invoked_native_platform",
            "runtime_ui_production_stack_invoked_renderer_upload",
            "runtime_ui_production_stack_diagnostics",
            "runtime_ui_production_stack_replay_hash"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include runtime UI production stack field: $field"
        }
    }
    $expectedRuntimeUiProductionStackFields = @{
        "runtime_ui_production_stack_status" = "host_evidence_required"
        "runtime_ui_production_stack_reviewed" = "1"
        "runtime_ui_production_stack_ready" = "1"
        "runtime_ui_production_stack_rows" = "6"
        "runtime_ui_production_stack_ready_rows" = "4"
        "runtime_ui_production_stack_host_gated_rows" = "2"
        "runtime_ui_production_stack_text_contract_ready" = "1"
        "runtime_ui_production_stack_selected_package_evidence_ready" = "1"
        "runtime_ui_production_stack_production_ready" = "0"
        "runtime_ui_production_stack_requires_ime_host_evidence" = "1"
        "runtime_ui_production_stack_ime_host_evidence" = "0"
        "runtime_ui_production_stack_ime_session_rows" = "1"
        "runtime_ui_production_stack_ime_composition_rows" = "1"
        "runtime_ui_production_stack_ime_candidate_rows" = "1"
        "runtime_ui_production_stack_ime_text_area_cursor_rows" = "1"
        "runtime_ui_production_stack_ime_committed_text_rows" = "1"
        "runtime_ui_production_stack_ime_clipboard_rows" = "1"
        "runtime_ui_production_stack_ime_platform_adapter_proof_rows" = "1"
        "runtime_ui_production_stack_ime_platform_host_gate_rows" = "1"
        "runtime_ui_production_stack_ime_platform_parity_ready" = "0"
        "runtime_ui_production_stack_requires_accessibility_host_evidence" = "1"
        "runtime_ui_production_stack_accessibility_host_evidence" = "0"
        "runtime_ui_production_stack_accessibility_role_rows" = "1"
        "runtime_ui_production_stack_accessibility_name_rows" = "1"
        "runtime_ui_production_stack_accessibility_description_rows" = "1"
        "runtime_ui_production_stack_accessibility_state_rows" = "1"
        "runtime_ui_production_stack_accessibility_focus_rows" = "1"
        "runtime_ui_production_stack_accessibility_action_rows" = "1"
        "runtime_ui_production_stack_accessibility_relationship_rows" = "1"
        "runtime_ui_production_stack_accessibility_live_region_rows" = "1"
        "runtime_ui_production_stack_accessibility_keyboard_pattern_rows" = "1"
        "runtime_ui_production_stack_accessibility_publication_status_rows" = "1"
        "runtime_ui_production_stack_accessibility_uia_host_gate_rows" = "1"
        "runtime_ui_production_stack_accessibility_platform_host_gate_rows" = "1"
        "runtime_ui_production_stack_accessibility_platform_parity_ready" = "0"
        "runtime_ui_production_stack_invoked_text_shaping" = "0"
        "runtime_ui_production_stack_invoked_font_rasterization" = "0"
        "runtime_ui_production_stack_invoked_ime" = "0"
        "runtime_ui_production_stack_invoked_accessibility_bridge" = "0"
        "runtime_ui_production_stack_invoked_native_platform" = "0"
        "runtime_ui_production_stack_invoked_renderer_upload" = "0"
        "runtime_ui_production_stack_diagnostics" = "0"
    }
    foreach ($field in $expectedRuntimeUiProductionStackFields.Keys) {
        $expectedValue = $expectedRuntimeUiProductionStackFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove runtime UI production stack field $field=$expectedValue."
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bruntime_ui_production_stack_replay_hash=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove positive runtime UI production stack replay hash."
    }
}
if ($requiresRuntimeUiRendererAtlasHandoff) {
    foreach ($field in @(
            "runtime_ui_renderer_atlas_handoff_status",
            "runtime_ui_renderer_atlas_handoff_ready",
            "runtime_ui_renderer_atlas_handoff_selected_package_evidence_ready",
            "runtime_ui_renderer_atlas_handoff_reviewed",
            "runtime_ui_renderer_atlas_handoff_image_atlas_pages",
            "runtime_ui_renderer_atlas_handoff_image_atlas_bindings",
            "runtime_ui_renderer_atlas_handoff_glyph_atlas_pages",
            "runtime_ui_renderer_atlas_handoff_glyph_atlas_bindings",
            "runtime_ui_renderer_atlas_handoff_atlas_placement_rows",
            "runtime_ui_renderer_atlas_handoff_atlas_budget_rows",
            "runtime_ui_renderer_atlas_handoff_atlas_eviction_diagnostic_rows",
            "runtime_ui_renderer_atlas_handoff_texture_upload_handoff_rows",
            "runtime_ui_renderer_atlas_handoff_renderer_submission_counter_rows",
            "runtime_ui_renderer_atlas_handoff_text_glyphs_available",
            "runtime_ui_renderer_atlas_handoff_text_glyphs_resolved",
            "runtime_ui_renderer_atlas_handoff_text_glyphs_missing",
            "runtime_ui_renderer_atlas_handoff_text_glyph_sprites_submitted",
            "runtime_ui_renderer_atlas_handoff_image_placeholders_available",
            "runtime_ui_renderer_atlas_handoff_image_resources_resolved",
            "runtime_ui_renderer_atlas_handoff_image_resources_missing",
            "runtime_ui_renderer_atlas_handoff_image_sprites_submitted",
            "runtime_ui_renderer_atlas_handoff_renderer_sprites_submitted",
            "runtime_ui_renderer_atlas_handoff_unsupported_claim_rows",
            "runtime_ui_renderer_atlas_handoff_side_effect_rows",
            "runtime_ui_renderer_atlas_handoff_requested_renderer_texture_upload_api",
            "runtime_ui_renderer_atlas_handoff_requested_public_native_handle",
            "runtime_ui_renderer_atlas_handoff_invoked_source_image_decode",
            "runtime_ui_renderer_atlas_handoff_invoked_live_glyph_atlas_generation",
            "runtime_ui_renderer_atlas_handoff_invoked_renderer_upload",
            "runtime_ui_renderer_atlas_handoff_diagnostics",
            "runtime_ui_renderer_atlas_handoff_replay_hash"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=") {
            Write-Error "Installed desktop runtime smoke status line did not include runtime UI renderer atlas handoff field: $field"
        }
    }
    $expectedRuntimeUiRendererAtlasHandoffFields = @{
        "runtime_ui_renderer_atlas_handoff_status" = "ready"
        "runtime_ui_renderer_atlas_handoff_ready" = "1"
        "runtime_ui_renderer_atlas_handoff_selected_package_evidence_ready" = "1"
        "runtime_ui_renderer_atlas_handoff_reviewed" = "1"
        "runtime_ui_renderer_atlas_handoff_image_atlas_pages" = "1"
        "runtime_ui_renderer_atlas_handoff_image_atlas_bindings" = "1"
        "runtime_ui_renderer_atlas_handoff_glyph_atlas_pages" = "1"
        "runtime_ui_renderer_atlas_handoff_glyph_atlas_bindings" = "1"
        "runtime_ui_renderer_atlas_handoff_atlas_placement_rows" = "2"
        "runtime_ui_renderer_atlas_handoff_atlas_budget_rows" = "2"
        "runtime_ui_renderer_atlas_handoff_atlas_eviction_diagnostic_rows" = "1"
        "runtime_ui_renderer_atlas_handoff_texture_upload_handoff_rows" = "1"
        "runtime_ui_renderer_atlas_handoff_renderer_submission_counter_rows" = "1"
        "runtime_ui_renderer_atlas_handoff_text_glyphs_available" = "1"
        "runtime_ui_renderer_atlas_handoff_text_glyphs_resolved" = "1"
        "runtime_ui_renderer_atlas_handoff_text_glyphs_missing" = "0"
        "runtime_ui_renderer_atlas_handoff_text_glyph_sprites_submitted" = "1"
        "runtime_ui_renderer_atlas_handoff_image_placeholders_available" = "1"
        "runtime_ui_renderer_atlas_handoff_image_resources_resolved" = "1"
        "runtime_ui_renderer_atlas_handoff_image_resources_missing" = "0"
        "runtime_ui_renderer_atlas_handoff_image_sprites_submitted" = "1"
        "runtime_ui_renderer_atlas_handoff_renderer_sprites_submitted" = "2"
        "runtime_ui_renderer_atlas_handoff_unsupported_claim_rows" = "0"
        "runtime_ui_renderer_atlas_handoff_side_effect_rows" = "0"
        "runtime_ui_renderer_atlas_handoff_requested_renderer_texture_upload_api" = "0"
        "runtime_ui_renderer_atlas_handoff_requested_public_native_handle" = "0"
        "runtime_ui_renderer_atlas_handoff_invoked_source_image_decode" = "0"
        "runtime_ui_renderer_atlas_handoff_invoked_live_glyph_atlas_generation" = "0"
        "runtime_ui_renderer_atlas_handoff_invoked_renderer_upload" = "0"
        "runtime_ui_renderer_atlas_handoff_diagnostics" = "0"
    }
    foreach ($field in $expectedRuntimeUiRendererAtlasHandoffFields.Keys) {
        $expectedValue = $expectedRuntimeUiRendererAtlasHandoffFields[$field]
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove runtime UI renderer atlas handoff field $field=$expectedValue."
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bruntime_ui_renderer_atlas_handoff_replay_hash=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove positive runtime UI renderer atlas handoff replay hash."
    }
}
if ($requiresPackageUploadStaging) {
    $expectedPackageUploadStagingFields = @{
        "package_upload_staging_status" = "ready"
        "package_upload_staging_ready" = "1"
        "package_upload_staging_diagnostics" = "0"
        "package_upload_staging_package_transactions" = "4"
        "package_upload_staging_texture_uploads" = "1"
        "package_upload_staging_mesh_uploads" = "1"
        "package_upload_staging_skinned_mesh_uploads" = "1"
        "package_upload_staging_morph_mesh_uploads" = "1"
        "package_upload_staging_texture_bindings" = "1"
        "package_upload_staging_mesh_bindings" = "1"
        "package_upload_staging_skinned_mesh_bindings" = "1"
        "package_upload_staging_morph_mesh_bindings" = "1"
        "package_upload_staging_staging_pool_leases" = "4"
        "package_upload_staging_ring_backed_uploads" = "4"
        "package_upload_staging_resource_updates_ready" = "1"
        "package_upload_staging_resource_updates" = "4"
        "package_upload_staging_resource_update_submitted_fences" = "4"
        "package_upload_staging_resource_update_graphics_ready_updates" = "4"
        "package_upload_staging_resource_update_graphics_queue_waits_recorded" = "3"
        "package_upload_staging_resource_update_same_queue_graphics_updates" = "1"
        "package_upload_staging_submitted_fences" = "4"
        "package_upload_staging_upload_queue_waits_recorded" = "3"
        "package_upload_staging_copy_queue_submits" = "3"
        "package_upload_staging_graphics_queue_submits" = "1"
        "package_upload_staging_queue_waits" = "3"
        "package_upload_staging_fence_waits" = "0"
        "package_upload_staging_graphics_waited_for_copy" = "1"
    }
    foreach ($field in $expectedPackageUploadStagingFields.Keys) {
        $expectedValue = [regex]::Escape($expectedPackageUploadStagingFields[$field])
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove package upload staging field: $field=$($expectedPackageUploadStagingFields[$field])"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpackage_upload_staging_uploaded_bytes=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove positive package upload staging bytes."
    }
}
if ($requiresKtx2BasisTextureReview) {
    $expectedKtx2BasisTextureReviewFields = @{
        "ktx_basis_texture_review_status" = "host_evidence_required"
        "ktx_basis_texture_review_reviewed" = "1"
        "ktx_basis_texture_review_ready" = "1"
        "ktx_basis_texture_review_rows" = "7"
        "ktx_basis_texture_review_ready_rows" = "6"
        "ktx_basis_texture_review_host_gated_rows" = "1"
        "ktx_basis_texture_review_dependency_gated_rows" = "0"
        "ktx_basis_texture_review_unsupported_claim_rows" = "0"
        "ktx_basis_texture_review_container_validation_rows" = "1"
        "ktx_basis_texture_review_supercompression_policy_rows" = "1"
        "ktx_basis_texture_review_transcode_target_policy_rows" = "1"
        "ktx_basis_texture_review_gpu_target_compatibility_rows" = "1"
        "ktx_basis_texture_review_source_provenance_rows" = "1"
        "ktx_basis_texture_review_package_output_rows" = "1"
        "ktx_basis_texture_review_host_tool_gate_rows" = "1"
        "ktx_basis_texture_review_dependency_legal_records_ready" = "1"
        "ktx_basis_texture_review_selected_package_evidence_ready" = "1"
        "ktx_basis_texture_review_ktx_basis_review_ready" = "1"
        "ktx_basis_texture_review_broad_texture_codec_ready" = "0"
        "ktx_basis_texture_review_invoked_runtime_transcoding" = "0"
        "ktx_basis_texture_review_invoked_gpu_upload" = "0"
        "ktx_basis_texture_review_invoked_compression_tool" = "0"
        "ktx_basis_texture_review_diagnostics" = "0"
    }
    foreach ($field in $expectedKtx2BasisTextureReviewFields.Keys) {
        $expectedValue = [regex]::Escape($expectedKtx2BasisTextureReviewFields[$field])
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove KTX2/Basis texture review field: $field=$($expectedKtx2BasisTextureReviewFields[$field])"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bktx_basis_texture_review_replay_hash=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove positive KTX2/Basis texture review replay hash."
    }
}
if ($requiresGltfSceneImportReview) {
    $expectedGltfSceneImportReviewFields = @{
        "gltf_scene_import_review_status" = "ready"
        "gltf_scene_import_review_reviewed" = "1"
        "gltf_scene_import_review_ready" = "1"
        "gltf_scene_import_review_rows" = "8"
        "gltf_scene_import_review_ready_rows" = "8"
        "gltf_scene_import_review_dependency_gated_rows" = "0"
        "gltf_scene_import_review_unsupported_claim_rows" = "0"
        "gltf_scene_import_review_source_root_rows" = "1"
        "gltf_scene_import_review_parser_validation_rows" = "1"
        "gltf_scene_import_review_geometry_payload_rows" = "1"
        "gltf_scene_import_review_material_payload_rows" = "1"
        "gltf_scene_import_review_animation_payload_rows" = "1"
        "gltf_scene_import_review_external_resource_policy_rows" = "1"
        "gltf_scene_import_review_source_provenance_rows" = "1"
        "gltf_scene_import_review_package_output_rows" = "1"
        "gltf_scene_import_review_dependency_legal_records_ready" = "1"
        "gltf_scene_import_review_selected_package_evidence_ready" = "1"
        "gltf_scene_import_review_gltf_scene_import_ready" = "1"
        "gltf_scene_import_review_broad_scene_import_ready" = "0"
        "gltf_scene_import_review_invoked_external_network_fetch" = "0"
        "gltf_scene_import_review_invoked_runtime_source_parsing" = "0"
        "gltf_scene_import_review_leaked_parser_type" = "0"
        "gltf_scene_import_review_exposed_native_handle" = "0"
        "gltf_scene_import_review_mutated_packages" = "0"
        "gltf_scene_import_review_diagnostics" = "0"
    }
    foreach ($field in $expectedGltfSceneImportReviewFields.Keys) {
        $expectedValue = [regex]::Escape($expectedGltfSceneImportReviewFields[$field])
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove glTF scene import review field: $field=$($expectedGltfSceneImportReviewFields[$field])"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bgltf_scene_import_review_replay_hash=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove positive glTF scene import review replay hash."
    }
}
if ($smokeOutput -match "(?m)^$escapedGameTarget status=.*\bscene_gpu_status=ready") {
    foreach ($field in @(
            "scene_gpu_texture_uploads",
            "scene_gpu_material_uploads",
            "scene_gpu_material_pipeline_layouts",
            "scene_gpu_uploaded_texture_bytes",
            "scene_gpu_uploaded_mesh_bytes",
            "scene_gpu_uploaded_material_factor_bytes"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove scene GPU upload execution field: $field"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bscene_gpu_mesh_uploads=[1-9]\d*\b" -and
        $smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bscene_gpu_skinned_mesh_uploads=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove scene GPU static or skinned mesh upload execution."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bscene_gpu_mesh_resolved=[1-9]\d*\b" -and
        $smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bscene_gpu_skinned_mesh_resolved=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove scene GPU static or skinned mesh resolution."
    }
    if ($requiresShadowMorphComposition) {
        foreach ($field in @(
                "scene_gpu_morph_mesh_bindings",
                "scene_gpu_morph_mesh_uploads",
                "scene_gpu_morph_mesh_resolved",
                "scene_gpu_uploaded_morph_bytes",
                "renderer_gpu_morph_draws",
                "renderer_morph_descriptor_binds"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed desktop runtime smoke status line did not prove shadow morph composition field: $field"
            }
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpostprocess_status=ready") {
        Write-Error "Installed desktop runtime smoke status line did not prove ready postprocess_status for a ready scene GPU path."
    }
    if ($requiresPostprocess) {
        $expectedPostprocessPolicySceneDepthRequired = if ($requiresPostprocessDepthInput) { "1" } else { "0" }
        $expectedPostprocessPolicyFields = @{
            "postprocess_policy_status" = "ready"
            "postprocess_policy_ready" = "1"
            "postprocess_policy_diagnostics" = "0"
            "postprocess_policy_effects" = "1"
            "postprocess_policy_postprocess_passes" = "1"
            "postprocess_policy_framegraph_passes" = "2"
            "postprocess_policy_framegraph_barrier_step_budget" = "2"
            "postprocess_policy_scene_color_required" = "1"
            "postprocess_policy_scene_depth_required" = $expectedPostprocessPolicySceneDepthRequired
            "postprocess_policy_color_grading_effect" = "1"
            "postprocess_policy_backend_shader_evidence_ready" = "1"
        }
        foreach ($field in $expectedPostprocessPolicyFields.Keys) {
            $expectedValue = [regex]::Escape($expectedPostprocessPolicyFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed desktop runtime smoke status line did not prove postprocess policy field: $field=$($expectedPostprocessPolicyFields[$field])"
            }
        }
        if ($requiresD3d12PostprocessEvidence) {
            $expectedD3d12PostprocessExecutionFields = @{
                "postprocess_d3d12_execution_status" = "ready"
                "postprocess_d3d12_execution_ready" = "1"
                "postprocess_d3d12_execution_selected" = "1"
                "postprocess_d3d12_execution_shader_evidence_ready" = "1"
                "postprocess_d3d12_execution_expected_passes" = [string]$expectedSmokeFrames
                "postprocess_d3d12_execution_passes" = [string]$expectedSmokeFrames
                "postprocess_d3d12_execution_passes_ok" = "1"
            }
            foreach ($field in $expectedD3d12PostprocessExecutionFields.Keys) {
                $expectedValue = [regex]::Escape($expectedD3d12PostprocessExecutionFields[$field])
                if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                    Write-Error "Installed desktop runtime smoke status line did not prove D3D12 postprocess execution field: $field=$($expectedD3d12PostprocessExecutionFields[$field])"
                }
            }
        }
        if ($requiresVulkanPostprocessEvidence) {
            $expectedVulkanPostprocessExecutionFields = @{
                "vulkan_postprocess_execution_status" = "ready"
                "vulkan_postprocess_execution_ready" = "1"
                "vulkan_postprocess_execution_selected" = "1"
                "vulkan_postprocess_execution_shader_evidence_ready" = "1"
                "vulkan_postprocess_execution_expected_passes" = [string]$expectedSmokeFrames
                "vulkan_postprocess_execution_passes" = [string]$expectedSmokeFrames
                "vulkan_postprocess_execution_passes_ok" = "1"
            }
            foreach ($field in $expectedVulkanPostprocessExecutionFields.Keys) {
                $expectedValue = [regex]::Escape($expectedVulkanPostprocessExecutionFields[$field])
                if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                    Write-Error "Installed desktop runtime smoke status line did not prove Vulkan postprocess execution field: $field=$($expectedVulkanPostprocessExecutionFields[$field])"
                }
            }
        }
    }
    $expectedFramegraphPasses = if ($requiresDirectionalShadow) { 3 } else { 2 }
    $expectedFramegraphPassExecutions = $expectedSmokeFrames * $expectedFramegraphPasses
    $expectedFramegraphBarrierExecutions = Get-ExpectedInstalledFramegraphBarrierExecutions `
        -ExpectedFrames $expectedSmokeFrames `
        -RequiresDirectionalShadow $requiresDirectionalShadow `
        -RequiresPostprocessDepthInput $requiresPostprocessDepthInput
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bframegraph_passes=$expectedFramegraphPasses\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove the frame graph pass count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bframegraph_passes_executed=$expectedFramegraphPassExecutions\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove frame graph pass execution count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bframegraph_render_passes_recorded=$expectedFramegraphPassExecutions\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove frame graph render-pass envelope count."
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bframegraph_barrier_steps_executed=$expectedFramegraphBarrierExecutions\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove frame graph barrier-step execution count."
    }
    if ($requiresPostprocessDepthInput -and
        $smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bpostprocess_depth_input_ready=1\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove ready postprocess depth input."
    }
    if ($requiresDirectionalShadow) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bdirectional_shadow_status=ready\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready directional shadow status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bdirectional_shadow_ready=1\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready directional shadow smoke."
        }
        if ($requiresDirectionalShadowFiltering) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bdirectional_shadow_filter_mode=fixed_pcf_3x3\b") {
                Write-Error "Installed desktop runtime smoke status line did not prove fixed PCF directional shadow filtering."
            }
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bdirectional_shadow_filter_taps=9\b") {
                Write-Error "Installed desktop runtime smoke status line did not prove 3x3 directional shadow filter tap count."
            }
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bdirectional_shadow_filter_radius_texels=1\b") {
                Write-Error "Installed desktop runtime smoke status line did not prove directional shadow filter radius."
            }
        }
        $expectedShadowCascadeFields = @{
            "directional_shadow_cascade_count" = "4"
            "directional_shadow_cascade_tile_width" = "225"
            "directional_shadow_atlas_width" = "900"
            "directional_shadow_atlas_height" = "225"
            "directional_shadow_light_space_cascades" = "4"
            "directional_shadow_cascade_splits" = "5"
        }
        if ($requiresD3d12ShadowCascadePolicy) {
            $expectedD3d12ShadowCascadePolicyFields = @{
                "d3d12_shadow_cascade_policy_ready" = "1"
                "d3d12_shadow_cascade_policy_selected" = "1"
            }
            foreach ($field in $expectedD3d12ShadowCascadePolicyFields.Keys) {
                $expectedValue = [regex]::Escape($expectedD3d12ShadowCascadePolicyFields[$field])
                if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                    Write-Error "Installed desktop runtime smoke status line did not prove D3D12 shadow cascade policy field: $field=$($expectedD3d12ShadowCascadePolicyFields[$field])"
                }
            }
            foreach ($field in $expectedShadowCascadeFields.Keys) {
                $expectedValue = [regex]::Escape($expectedShadowCascadeFields[$field])
                if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                    Write-Error "Installed desktop runtime smoke status line did not prove D3D12 shadow cascade field: $field=$($expectedShadowCascadeFields[$field])"
                }
            }
        }
        if ($requiresVulkanShadowCascadePolicy) {
            $expectedVulkanShadowCascadePolicyFields = @{
                "vulkan_shadow_cascade_policy_ready" = "1"
                "vulkan_shadow_cascade_policy_selected" = "1"
            }
            foreach ($field in $expectedVulkanShadowCascadePolicyFields.Keys) {
                $expectedValue = [regex]::Escape($expectedVulkanShadowCascadePolicyFields[$field])
                if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                    Write-Error "Installed desktop runtime smoke status line did not prove Vulkan shadow cascade policy field: $field=$($expectedVulkanShadowCascadePolicyFields[$field])"
                }
            }
            foreach ($field in $expectedShadowCascadeFields.Keys) {
                $expectedValue = [regex]::Escape($expectedShadowCascadeFields[$field])
                if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                    Write-Error "Installed desktop runtime smoke status line did not prove Vulkan shadow cascade field: $field=$($expectedShadowCascadeFields[$field])"
                }
            }
        }
    }
    if ($requiresNativeUiOverlay) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_requested=1\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove native UI overlay was requested."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_status=ready\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready native UI overlay status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_ready=1\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready native UI overlay."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_sprites_submitted=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove native UI overlay sprite submission."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_overlay_draws=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove native UI overlay draw output."
        }
    }
    if ($requiresNativeUiTexturedSpriteAtlas) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_status=ready\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready UI atlas package metadata."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_pages=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove UI atlas metadata page rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_bindings=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove UI atlas metadata image bindings."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_requested=1\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove textured UI overlay was requested."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_status=ready\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready textured UI overlay status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_atlas_ready=1\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready textured UI atlas page."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_sprites_submitted=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove textured UI sprite submission."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_texture_binds=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove textured UI texture binding."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_draws=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove textured UI overlay draw output."
        }
    }
    if ($requiresNativeUiTextGlyphAtlas) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_status=ready\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready UI glyph atlas package metadata."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_pages=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove UI glyph atlas metadata page rows."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_atlas_metadata_glyphs=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove UI glyph atlas metadata glyph bindings."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_requested=1\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove glyph UI overlay was requested."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_status=ready\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready glyph UI overlay status."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_atlas_ready=1\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove ready glyph UI atlas page."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_sprites_submitted=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove glyph UI sprite submission."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_texture_binds=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove glyph UI texture binding."
        }
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\bui_texture_overlay_draws=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove glyph UI overlay draw output."
        }
    }
    if ($requiresGpuSkinning) {
        foreach ($field in @(
                "scene_gpu_skinned_mesh_bindings",
                "scene_gpu_skinned_mesh_uploads",
                "scene_gpu_skinned_mesh_resolved",
                "renderer_gpu_skinning_draws",
                "renderer_skinned_palette_descriptor_binds"
            )) {
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
                Write-Error "Installed desktop runtime smoke status line did not prove GPU skinning field: $field"
            }
        }
    }
    if ($requiresD3d12GpuSkinningEvidence) {
        $expectedD3d12GpuSkinningFields = @{
            "d3d12_gpu_skinning_evidence_ready" = "1"
            "d3d12_gpu_skinning_evidence_selected" = "1"
        }
        foreach ($field in $expectedD3d12GpuSkinningFields.Keys) {
            $expectedValue = [regex]::Escape($expectedD3d12GpuSkinningFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed desktop runtime smoke status line did not prove D3D12 GPU skinning field: $field=$($expectedD3d12GpuSkinningFields[$field])"
            }
        }
    }
    if ($requiresVulkanGpuSkinningEvidence) {
        $expectedVulkanGpuSkinningFields = @{
            "vulkan_gpu_skinning_evidence_ready" = "1"
            "vulkan_gpu_skinning_evidence_selected" = "1"
        }
        foreach ($field in $expectedVulkanGpuSkinningFields.Keys) {
            $expectedValue = [regex]::Escape($expectedVulkanGpuSkinningFields[$field])
            if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
                Write-Error "Installed desktop runtime smoke status line did not prove Vulkan GPU skinning field: $field=$($expectedVulkanGpuSkinningFields[$field])"
            }
        }
    }
}
if ($requiresRendererQualityGates) {
    $expectedRendererQualityFramegraphPasses = if ($requiresDirectionalShadow) { "3" } else { "2" }
    $expectedRendererQualityFramegraphBarrierSteps = [string](Get-ExpectedInstalledFramegraphBarrierExecutions `
            -ExpectedFrames $expectedSmokeFrames `
            -RequiresDirectionalShadow $requiresDirectionalShadow `
            -RequiresPostprocessDepthInput $requiresPostprocessDepthInput)
    $expectedRendererQualityFields = @{
        "renderer_quality_status" = "ready"
        "renderer_quality_ready" = "1"
        "renderer_quality_diagnostics" = "0"
        "renderer_quality_expected_framegraph_passes" = $expectedRendererQualityFramegraphPasses
        "renderer_quality_expected_framegraph_render_passes" = [string]$expectedFramegraphPassExecutions
        "renderer_quality_expected_framegraph_barrier_steps" = $expectedRendererQualityFramegraphBarrierSteps
        "renderer_quality_framegraph_passes_ok" = "1"
        "renderer_quality_framegraph_render_passes_ok" = "1"
        "renderer_quality_framegraph_barrier_steps_ok" = "1"
        "renderer_quality_framegraph_execution_budget_ok" = "1"
        "renderer_quality_scene_gpu_ready" = "1"
        "renderer_quality_postprocess_ready" = "1"
    }
    if ($requiresPostprocessDepthInput) {
        $expectedRendererQualityFields["renderer_quality_postprocess_depth_input_ready"] = "1"
    }
    if ($requiresDirectionalShadow) {
        $expectedRendererQualityFields["renderer_quality_directional_shadow_ready"] = "1"
    }
    if ($requiresDirectionalShadowFiltering) {
        $expectedRendererQualityFields["renderer_quality_directional_shadow_filter_ready"] = "1"
    }
    foreach ($field in $expectedRendererQualityFields.Keys) {
        $expectedValue = [regex]::Escape($expectedRendererQualityFields[$field])
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove renderer quality gate field: $field=$($expectedRendererQualityFields[$field])"
        }
    }
}
if ($requiresRendererQualityMatrix) {
    $expectedRendererQualityMatrixFields = @{
        "renderer_quality_matrix_status" = "host_evidence_required"
        "renderer_quality_matrix_reviewed" = "1"
        "renderer_quality_matrix_ready" = "0"
        "renderer_quality_matrix_rows" = "21"
        "renderer_quality_matrix_ready_rows" = "14"
        "renderer_quality_matrix_host_gated_rows" = "7"
        "renderer_quality_matrix_dependency_gated_rows" = "0"
        "renderer_quality_matrix_unsupported_rows" = "0"
        "renderer_quality_matrix_host_validated_backends" = "2"
        "renderer_quality_matrix_d3d12_ready" = "1"
        "renderer_quality_matrix_vulkan_strict_ready" = "1"
        "renderer_quality_matrix_metal_ready" = "0"
        "renderer_quality_matrix_requires_metal_host_evidence" = "1"
        "renderer_quality_matrix_metal_host_evidence" = "0"
        "renderer_quality_matrix_selected_package_evidence_ready" = "1"
        "renderer_quality_matrix_general_renderer_quality_ready" = "0"
        "renderer_quality_matrix_invoked_gpu_commands" = "0"
        "renderer_quality_matrix_invoked_native_capture" = "0"
        "renderer_quality_matrix_invoked_crash_upload" = "0"
        "renderer_quality_matrix_diagnostics" = "0"
    }
    foreach ($field in $expectedRendererQualityMatrixFields.Keys) {
        $expectedValue = [regex]::Escape($expectedRendererQualityMatrixFields[$field])
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove renderer quality matrix field: $field=$($expectedRendererQualityMatrixFields[$field])"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\brenderer_quality_matrix_replay_hash=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove positive renderer quality matrix replay hash."
    }
}
if ($requiresRenderingVfxProfiling) {
    $expectedRenderingVfxProfilingFields = @{
        "rendering_vfx_profiling_status" = "host_evidence_required"
        "rendering_vfx_profiling_reviewed" = "1"
        "rendering_vfx_profiling_ready" = "0"
        "rendering_vfx_profiling_feature_rows" = "3"
        "rendering_vfx_profiling_gpu_particle_budget_rows" = "3"
        "rendering_vfx_profiling_postprocess_rows" = "3"
        "rendering_vfx_profiling_backend_timing_rows" = "3"
        "rendering_vfx_profiling_backend_evidence_rows" = "3"
        "rendering_vfx_profiling_backend_evidence_ready" = "2"
        "rendering_vfx_profiling_backend_evidence_host_gated" = "1"
        "rendering_vfx_profiling_cpu_profile_rows" = "3"
        "rendering_vfx_profiling_package_counter_rows" = "3"
        "rendering_vfx_profiling_package_counter_ready" = "2"
        "rendering_vfx_profiling_package_counter_host_gated" = "1"
        "rendering_vfx_profiling_crash_telemetry_handoff_rows" = "3"
        "rendering_vfx_profiling_host_validated_backends" = "2"
        "rendering_vfx_profiling_rejected_unsafe_rows" = "0"
        "rendering_vfx_profiling_d3d12_host_evidence_ready" = "1"
        "rendering_vfx_profiling_vulkan_strict_host_evidence_ready" = "1"
        "rendering_vfx_profiling_metal_host_evidence_ready" = "0"
        "rendering_vfx_profiling_requires_metal_host_evidence" = "1"
        "rendering_vfx_profiling_metal_host_evidence" = "0"
        "rendering_vfx_profiling_invoked_gpu_commands" = "0"
        "rendering_vfx_profiling_invoked_native_capture" = "0"
        "rendering_vfx_profiling_invoked_crash_upload" = "0"
        "rendering_vfx_profiling_debug_policy_ready" = "1"
        "rendering_vfx_profiling_debug_cpu_profile_zone_evidence_ready" = "1"
        "rendering_vfx_profiling_debug_trace_capture_handoff_evidence_ready" = "1"
        "rendering_vfx_profiling_debug_package_counter_evidence_ready" = "1"
        "rendering_vfx_profiling_memory_policy_ready" = "1"
        "rendering_vfx_profiling_memory_budget_evidence_ready" = "1"
        "rendering_vfx_profiling_memory_residency_pressure_evidence_ready" = "1"
        "rendering_vfx_profiling_memory_package_counter_evidence_ready" = "1"
        "rendering_vfx_profiling_diagnostics" = "0"
    }
    foreach ($field in $expectedRenderingVfxProfilingFields.Keys) {
        $expectedValue = [regex]::Escape($expectedRenderingVfxProfilingFields[$field])
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove rendering VFX/profiling field: $field=$($expectedRenderingVfxProfilingFields[$field])"
        }
    }
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\brendering_vfx_profiling_replay_hash=[1-9]\d*\b") {
        Write-Error "Installed desktop runtime smoke status line did not prove positive rendering VFX/profiling replay hash."
    }
    foreach ($field in @(
            "rendering_vfx_profiling_debug_gpu_timestamp_ticks_per_second",
            "rendering_vfx_profiling_debug_cpu_profile_zones",
            "rendering_vfx_profiling_debug_trace_capture_handoff_rows",
            "rendering_vfx_profiling_debug_cpu_profile_zone_requests",
            "rendering_vfx_profiling_debug_trace_capture_handoff_requests",
            "rendering_vfx_profiling_debug_package_counter_requests",
            "rendering_vfx_profiling_memory_requested_bytes",
            "rendering_vfx_profiling_memory_residency_pressure_events",
            "rendering_vfx_profiling_memory_declared_budget_requests",
            "rendering_vfx_profiling_memory_residency_pressure_requests",
            "rendering_vfx_profiling_memory_package_counter_requests"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove positive rendering VFX/profiling field: $field"
        }
    }
}
if ($requiresFrameGraphMultiQueueEvidence -or $requiresVulkanFrameGraphMultiQueueEvidence) {
    $expectedFrameGraphMultiQueueFields = @{
        "framegraph_multiqueue_status" = "ready"
        "framegraph_multiqueue_ready" = "1"
        "framegraph_multiqueue_diagnostics" = "0"
        "framegraph_multiqueue_command_lists_submitted" = "4"
        "framegraph_multiqueue_queue_waits_recorded" = "3"
        "framegraph_multiqueue_barriers_recorded" = "4"
        "framegraph_multiqueue_aliasing_barriers_recorded" = "1"
        "framegraph_multiqueue_pass_callbacks_invoked" = "4"
        "framegraph_multiqueue_submitted_pass_fences" = "4"
        "framegraph_multiqueue_graphics_waited_for_copy" = "1"
    }
    foreach ($field in $expectedFrameGraphMultiQueueFields.Keys) {
        $expectedValue = [regex]::Escape($expectedFrameGraphMultiQueueFields[$field])
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove Frame Graph multi-queue field: $field=$($expectedFrameGraphMultiQueueFields[$field])"
        }
    }
    foreach ($field in @(
            "framegraph_multiqueue_copy_queue_submits",
            "framegraph_multiqueue_graphics_queue_submits",
            "framegraph_multiqueue_queue_waits"
        )) {
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=[1-9]\d*\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove positive Frame Graph multi-queue field: $field"
        }
    }
}
if ($requiresFrameGraphMultiQueueEvidence) {
    $expectedD3d12FrameGraphMultiQueueFields = @{
        "d3d12_framegraph_multiqueue_evidence_ready" = "1"
        "d3d12_framegraph_multiqueue_evidence_selected" = "1"
    }
    foreach ($field in $expectedD3d12FrameGraphMultiQueueFields.Keys) {
        $expectedValue = [regex]::Escape($expectedD3d12FrameGraphMultiQueueFields[$field])
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove D3D12 Frame Graph multi-queue field: $field=$($expectedD3d12FrameGraphMultiQueueFields[$field])"
        }
    }
}
if ($requiresVulkanFrameGraphMultiQueueEvidence) {
    $expectedVulkanFrameGraphMultiQueueFields = @{
        "vulkan_framegraph_multiqueue_evidence_ready" = "1"
        "vulkan_framegraph_multiqueue_evidence_selected" = "1"
    }
    foreach ($field in $expectedVulkanFrameGraphMultiQueueFields.Keys) {
        $expectedValue = [regex]::Escape($expectedVulkanFrameGraphMultiQueueFields[$field])
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove Vulkan Frame Graph multi-queue field: $field=$($expectedVulkanFrameGraphMultiQueueFields[$field])"
        }
    }
}
if ($requiresPlayable3dSlice) {
    $expectedPlayable3dFields = @{
        "playable_3d_status" = "ready"
        "playable_3d_ready" = "1"
        "playable_3d_diagnostics" = "0"
        "playable_3d_expected_frames" = "2"
        "playable_3d_frames_ok" = "1"
        "playable_3d_game_frames_ok" = "1"
        "playable_3d_scene_mesh_plan_ready" = "1"
        "playable_3d_camera_controller_ready" = "1"
        "playable_3d_animation_ready" = "1"
        "playable_3d_morph_ready" = "1"
        "playable_3d_quaternion_ready" = "1"
        "playable_3d_package_streaming_ready" = "1"
        "playable_3d_scene_gpu_ready" = "1"
        "playable_3d_postprocess_ready" = "1"
        "playable_3d_renderer_quality_ready" = "1"
    }
    if ($requiresComputeMorph) {
        $expectedPlayable3dFields["playable_3d_compute_morph_selected"] = "1"
        $expectedPlayable3dFields["playable_3d_compute_morph_ready"] = "1"
    }
    if ($requiresComputeMorphNormalTangent -and -not $requiresComputeMorphSkin) {
        $expectedPlayable3dFields["playable_3d_compute_morph_normal_tangent_selected"] = "1"
        $expectedPlayable3dFields["playable_3d_compute_morph_normal_tangent_ready"] = "1"
    }
    if ($requiresComputeMorphSkin) {
        $expectedPlayable3dFields["playable_3d_compute_morph_skin_selected"] = "1"
        $expectedPlayable3dFields["playable_3d_compute_morph_skin_ready"] = "1"
    }
    if ($requiresComputeMorphAsyncTelemetry) {
        $expectedPlayable3dFields["playable_3d_compute_morph_async_telemetry_selected"] = "1"
        $expectedPlayable3dFields["playable_3d_compute_morph_async_telemetry_ready"] = "1"
    }
    foreach ($field in $expectedPlayable3dFields.Keys) {
        $expectedValue = [regex]::Escape($expectedPlayable3dFields[$field])
        if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$field=$expectedValue\b") {
            Write-Error "Installed desktop runtime smoke status line did not prove generated 3D playable package field: $field=$($expectedPlayable3dFields[$field])"
        }
    }
}

Write-Host "installed-desktop-runtime-validation: ok ($GameTarget)"
