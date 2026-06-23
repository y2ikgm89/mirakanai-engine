#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string[]]$ChangedPath = @(),
    [switch]$RunAll,
    [string]$GitHubOutputPath
)

$ErrorActionPreference = "Stop"

function ConvertTo-CiPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $normalizedPath = $Path -replace "\\", "/"
    while ($normalizedPath.StartsWith("./", [System.StringComparison]::Ordinal)) {
        $normalizedPath = $normalizedPath.Substring(2)
    }

    return $normalizedPath
}

function Test-CiWorkflowPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        $Path -match "^\.github/workflows/"
    )
}

function Test-RuntimeOrBuildPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        $Path -eq "CMakeLists.txt" -or
        $Path -match "/CMakeLists\.txt$" -or
        $Path -eq "CMakePresets.json" -or
        $Path -match "^(cmake|engine|editor|games|platform|runtime|shaders|examples|tests)/" -or
        $Path -eq "vcpkg.json" -or
        $Path -match "^tools/(validate|build|test|bootstrap-deps|check-toolchain|check-tidy|check-format|check-text-format|check-text-format-contract|format|format-text|text-format-core|check-dependency-policy|package|package-desktop-runtime|check-public-api-boundaries|check-shader-toolchain|run-validation-recipe|common)\.ps1$"
    )
}

function Test-WindowsCpuProfilingHostPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        $Path -match "^tools/(validate-cpu-profiling-matrix-host-gate|collect-cpu-profiling-host-evidence|check-cpu-profiling-host-evidence|check-cpu-profiling-host-evidence-collector|check-cpu-profiling-matrix-host-gate-closeout)\.ps1$"
    )
}

function Test-WindowsAssetImportersPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        $Path -eq "CMakeLists.txt" -or
        $Path -eq "CMakePresets.json" -or
        $Path -eq "vcpkg.json" -or
        $Path -match "^cmake/" -or
        $Path -match "^engine/(assets|tools)/" -or
        $Path -match "^tests/(unit/(asset|assets|tools)_.*|unit/tools_tests\.cpp)" -or
        $Path -match "^tools/(bootstrap-deps|build-asset-importers|validate-environment-asset-pipeline-full)\.ps1$"
    )
}

function Test-WindowsDesktopEditorPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        $Path -eq "CMakeLists.txt" -or
        $Path -eq "CMakePresets.json" -or
        $Path -match "^cmake/" -or
        $Path -match "^editor/" -or
        $Path -match "^tests/(unit/editor_.*|fixtures/editor_.*)" -or
        $Path -eq "tools/build-editor.ps1"
    )
}

function Test-WindowsNetworkEnetPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        $Path -eq "CMakeLists.txt" -or
        $Path -eq "CMakePresets.json" -or
        $Path -eq "vcpkg.json" -or
        $Path -match "^cmake/" -or
        $Path -match "^engine/runtime/network/enet/" -or
        $Path -match "^tests/unit/runtime_network_(transport_adapter|enet|production_security)_tests\.cpp$" -or
        $Path -match "^tools/(bootstrap-deps|validate-network-enet)\.ps1$"
    )
}

function Test-PlatformEvidenceFoundationPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        $Path -eq "CMakeLists.txt" -or
        $Path -eq "CMakePresets.json" -or
        $Path -eq "vcpkg.json" -or
        $Path -match "^cmake/" -or
        $Path -match "^engine/(environment|platform|renderer|rhi|runtime_rhi|runtime_scene_rhi|scene_renderer|ui_renderer)/" -or
        $Path -match "^games/sample_(2d_desktop_runtime_package|3d_desktop_runtime_package|desktop_runtime_game)/" -or
        $Path -match "^platform/" -or
        $Path -match "^shaders/" -or
        $Path -match "^tools/(bootstrap-deps|check-shader-toolchain|common|package-desktop-runtime|run-validation-recipe|validate|build)\.ps1$"
    )
}

function Test-LinuxVulkanHostEvidencePath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        (Test-PlatformEvidenceFoundationPath -Path $Path) -or
        $Path -match "^tools/(validate-linux-vulkan-runtime-host|validate-environment-backend-parity-v2|validate-environment-highest-commercial-readiness)\.ps1$"
    )
}

function Test-AppleHostEvidencePath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        (Test-PlatformEvidenceFoundationPath -Path $Path) -or
        $Path -match "^tools/(apple-host-helpers|build-mobile-apple|check-mobile-packaging|generate-environment-metal-optimization-artifacts|smoke-ios-package|validate-apple-metal-platform-host|validate-environment-metal-host-aggregate|validate-environment-weather-metal-solver-host-gate|validate-renderer-metal-apple)\.ps1$"
    )
}

function Test-SourceCodePath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        $Path -match "^(engine|editor|platform|runtime|shaders|examples|tests|games)/" -and
        $Path -match "\.(c|cc|cpp|cxx|c|h|hpp|hxx|ixx|ipp|inl|txx)$"
    )
}

function Test-SanitizerRelevantPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        $Path -match "^(engine|tests)/" -and
        $Path -match "\.(c|cc|cpp|cxx|c|h|hpp|hxx|ixx|ipp|inl|txx)$"
    )
}

function Test-CoverageRelevantPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        $Path -eq "tools/check-coverage.ps1" -or
        $Path -eq "tools/coverage-thresholds.json" -or
        $Path -eq "CMakePresets.json" -or
        ($Path -match "^(tests)/" -and
            $Path -match "\.(c|cc|cpp|cxx|c|h|hpp|hxx|ixx|ipp|inl|txx)$")
    )
}

function Test-Cpp23RelevantPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        $Path -eq "CMakePresets.json" -or
        $Path -eq "tools/evaluate-cpp23.ps1" -or
        $Path -eq "tools/check-ci-matrix.ps1" -or
        $Path -eq "tools/classify-pr-validation-tier.ps1" -or
        $Path -eq "tools/check-cpp-standard-policy.ps1" -or
        $Path -match "^tools/check-cpp-standard-(policy|flags)\.ps1$"
    )
}

function Test-StaticPolicyPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        $Path -eq ".clang-tidy" -or
        $Path -eq ".clang-format" -or
        $Path -eq "tools/check-tidy.ps1"
    )
}

function New-ValidationTierSelection {
    param(
        [string[]]$InputPath = @(),
        [switch]$All
    )

    if ($All) {
        return [pscustomobject][ordered]@{
            windows_msvc = $true
            windows_cpu_profiling_host = $true
            windows_asset_importers = $true
            windows_desktop_editor = $true
            windows_network_enet = $true
            linux_cmake = $true
            linux_vulkan_host = $true
            linux_sanitizers = $true
            linux_coverage = $true
            full_static_analysis = $true
            macos_metal_cmake = $true
            metal_host_evidence = $true
            ios_metal_evidence = $true
            windows_cpp23_release = $true
        }
    }

    $ciOrWorkflow = $false
    $runtimeOrBuild = $false
    $staticPolicy = $false
    $sourceCode = $false
    $sanitizerRelevant = $false
    $coverageRelevant = $false
    $cpp23Relevant = $false
    $windowsCpuProfilingHost = $false
    $windowsAssetImporters = $false
    $windowsDesktopEditor = $false
    $windowsNetworkEnet = $false
    $linuxVulkanHostEvidence = $false
    $appleHostEvidence = $false

    $expandedInputPath = foreach ($rawPath in $InputPath) {
        if ($rawPath.Contains(",")) {
            $rawPath -split ","
        }
        else {
            $rawPath
        }
    }

    foreach ($rawPath in $expandedInputPath) {
        $path = ConvertTo-CiPath -Path $rawPath
        if (Test-CiWorkflowPath -Path $path) {
            $ciOrWorkflow = $true
        }
        if (Test-RuntimeOrBuildPath -Path $path) {
            $runtimeOrBuild = $true
        }
        if (Test-SourceCodePath -Path $path) {
            $sourceCode = $true
        }
        if (Test-SanitizerRelevantPath -Path $path) {
            $sanitizerRelevant = $true
        }
        if (Test-CoverageRelevantPath -Path $path) {
            $coverageRelevant = $true
        }
        if (Test-Cpp23RelevantPath -Path $path) {
            $cpp23Relevant = $true
        }
        if (Test-WindowsCpuProfilingHostPath -Path $path) {
            $windowsCpuProfilingHost = $true
        }
        if (Test-WindowsAssetImportersPath -Path $path) {
            $windowsAssetImporters = $true
        }
        if (Test-WindowsDesktopEditorPath -Path $path) {
            $windowsDesktopEditor = $true
        }
        if (Test-WindowsNetworkEnetPath -Path $path) {
            $windowsNetworkEnet = $true
        }
        if (Test-LinuxVulkanHostEvidencePath -Path $path) {
            $linuxVulkanHostEvidence = $true
        }
        if (Test-AppleHostEvidencePath -Path $path) {
            $appleHostEvidence = $true
        }
        if (Test-StaticPolicyPath -Path $path) {
            $staticPolicy = $true
        }
    }

    $heavyBuildLane = $ciOrWorkflow -or $runtimeOrBuild

    return [pscustomobject][ordered]@{
        windows_msvc = $heavyBuildLane
        windows_cpu_profiling_host = ($ciOrWorkflow -or $windowsCpuProfilingHost)
        windows_asset_importers = ($ciOrWorkflow -or $windowsAssetImporters)
        windows_desktop_editor = ($ciOrWorkflow -or $windowsDesktopEditor)
        windows_network_enet = ($ciOrWorkflow -or $windowsNetworkEnet)
        linux_cmake = $heavyBuildLane
        linux_vulkan_host = ($ciOrWorkflow -or $linuxVulkanHostEvidence)
        linux_sanitizers = ($ciOrWorkflow -or $sanitizerRelevant)
        linux_coverage = $coverageRelevant
        full_static_analysis = ($heavyBuildLane -or $staticPolicy -or $sourceCode)
        macos_metal_cmake = $heavyBuildLane
        metal_host_evidence = ($ciOrWorkflow -or $appleHostEvidence)
        ios_metal_evidence = ($ciOrWorkflow -or $appleHostEvidence)
        windows_cpp23_release = ($ciOrWorkflow -or $cpp23Relevant)
    }
}

function Write-GitHubActionsOutput {
    param(
        [Parameter(Mandatory = $true)][pscustomobject]$Selection,
        [Parameter(Mandatory = $true)][string]$OutputPath
    )

    $lines = @(
        "windows_msvc=$($Selection.windows_msvc.ToString().ToLowerInvariant())",
        "windows_cpu_profiling_host=$($Selection.windows_cpu_profiling_host.ToString().ToLowerInvariant())",
        "windows_asset_importers=$($Selection.windows_asset_importers.ToString().ToLowerInvariant())",
        "windows_desktop_editor=$($Selection.windows_desktop_editor.ToString().ToLowerInvariant())",
        "windows_network_enet=$($Selection.windows_network_enet.ToString().ToLowerInvariant())",
        "linux_cmake=$($Selection.linux_cmake.ToString().ToLowerInvariant())",
        "linux_vulkan_host=$($Selection.linux_vulkan_host.ToString().ToLowerInvariant())",
        "linux_sanitizers=$($Selection.linux_sanitizers.ToString().ToLowerInvariant())",
        "linux_coverage=$($Selection.linux_coverage.ToString().ToLowerInvariant())",
        "full_static_analysis=$($Selection.full_static_analysis.ToString().ToLowerInvariant())",
        "macos_metal_cmake=$($Selection.macos_metal_cmake.ToString().ToLowerInvariant())",
        "metal_host_evidence=$($Selection.metal_host_evidence.ToString().ToLowerInvariant())",
        "ios_metal_evidence=$($Selection.ios_metal_evidence.ToString().ToLowerInvariant())",
        "windows_cpp23_release=$($Selection.windows_cpp23_release.ToString().ToLowerInvariant())"
    )

    Add-Content -LiteralPath $OutputPath -Value $lines -Encoding utf8
}

$selection = New-ValidationTierSelection -InputPath $ChangedPath -All:$RunAll
if (-not [string]::IsNullOrWhiteSpace($GitHubOutputPath)) {
    if ($PSCmdlet.ShouldProcess($GitHubOutputPath, "append GitHub Actions validation tier outputs")) {
        Write-GitHubActionsOutput -Selection $selection -OutputPath $GitHubOutputPath
    }
}

$selection | ConvertTo-Json -Compress
