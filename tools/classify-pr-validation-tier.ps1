#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string[]]$ChangedPath = @(),
    [switch]$RunAll,
    [string]$GitHubOutputPath
)

$ErrorActionPreference = "Stop"

$script:ValidationLaneNames = @(
    "windows_msvc",
    "windows_cpu_profiling_host",
    "windows_asset_importers",
    "windows_desktop_editor",
    "windows_network_enet",
    "linux_cmake",
    "linux_vulkan_host",
    "linux_sanitizers",
    "linux_coverage",
    "full_static_analysis",
    "macos_metal_cmake",
    "metal_host_evidence",
    "ios_metal_evidence",
    "windows_cpp23_release"
)

function ConvertTo-CiPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $normalizedPath = $Path -replace "\\", "/"
    while ($normalizedPath.StartsWith("./", [System.StringComparison]::Ordinal)) {
        $normalizedPath = $normalizedPath.Substring(2)
    }

    return $normalizedPath
}

function Test-ValidationWorkflowPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return $Path -eq ".github/workflows/validate.yml"
}

function Test-IosValidationWorkflowPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return $Path -eq ".github/workflows/ios-validate.yml"
}

function Test-OtherCiWorkflowPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        $Path -match "^\.github/workflows/" -and
        -not (Test-ValidationWorkflowPath -Path $Path) -and
        -not (Test-IosValidationWorkflowPath -Path $Path)
    )
}

function Test-CiClassifierPolicyPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        $Path -eq "tools/classify-pr-validation-tier.ps1" -or
        $Path -eq "tools/check-ci-matrix.ps1"
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
        $Path -match "^tools/(apple-host-helpers|build-mobile-apple|check-mobile-packaging|collect-renderer-metal-memory-profiling-host-evidence|check-renderer-metal-memory-profiling-host-evidence|check-renderer-metal-memory-profiling-host-evidence-collector|generate-environment-metal-optimization-artifacts|smoke-ios-package|validate-apple-metal-platform-host|validate-environment-metal-host-aggregate|validate-environment-weather-metal-solver-host-gate|validate-renderer-metal-apple)\.ps1$"
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

function Add-UniqueClassificationReason {
    param(
        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$Reasons,
        [Parameter(Mandatory = $true)][string]$Reason
    )

    if (-not $Reasons.Contains($Reason)) {
        $Reasons.Add($Reason)
    }
}

function Join-CiClassifierValue {
    param([string[]]$Value = @())

    $uniqueValue = @($Value | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)
    if ($uniqueValue.Count -eq 0) {
        return "none"
    }

    return ($uniqueValue -join ",")
}

function New-ValidationTierSelectionObject {
    param(
        [Parameter(Mandatory = $true)][hashtable]$LaneSelection,
        [string[]]$Reason = @()
    )

    $selectedLane = foreach ($laneName in $script:ValidationLaneNames) {
        if ([bool]$LaneSelection[$laneName]) {
            $laneName
        }
    }

    $classificationReason = @($Reason | Sort-Object -Unique)
    if ($classificationReason.Count -eq 0) {
        $classificationReason = @("docs-agent-rules-subagent-only")
    }

    return [pscustomobject][ordered]@{
        windows_msvc = [bool]$LaneSelection["windows_msvc"]
        windows_cpu_profiling_host = [bool]$LaneSelection["windows_cpu_profiling_host"]
        windows_asset_importers = [bool]$LaneSelection["windows_asset_importers"]
        windows_desktop_editor = [bool]$LaneSelection["windows_desktop_editor"]
        windows_network_enet = [bool]$LaneSelection["windows_network_enet"]
        linux_cmake = [bool]$LaneSelection["linux_cmake"]
        linux_vulkan_host = [bool]$LaneSelection["linux_vulkan_host"]
        linux_sanitizers = [bool]$LaneSelection["linux_sanitizers"]
        linux_coverage = [bool]$LaneSelection["linux_coverage"]
        full_static_analysis = [bool]$LaneSelection["full_static_analysis"]
        macos_metal_cmake = [bool]$LaneSelection["macos_metal_cmake"]
        metal_host_evidence = [bool]$LaneSelection["metal_host_evidence"]
        ios_metal_evidence = [bool]$LaneSelection["ios_metal_evidence"]
        windows_cpp23_release = [bool]$LaneSelection["windows_cpp23_release"]
        selected_lanes = (Join-CiClassifierValue -Value $selectedLane)
        classification_reasons = (Join-CiClassifierValue -Value $classificationReason)
    }
}

function New-ValidationTierSelection {
    param(
        [string[]]$InputPath = @(),
        [switch]$All
    )

    if ($All) {
        return New-ValidationTierSelectionObject -LaneSelection @{
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
        } -Reason @("non-pr-full-matrix")
    }

    $validationWorkflow = $false
    $iosValidationWorkflow = $false
    $otherCiWorkflow = $false
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
    $classificationReasons = [System.Collections.Generic.List[string]]::new()

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
        if (Test-ValidationWorkflowPath -Path $path) {
            $validationWorkflow = $true
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "ci-validation-workflow"
        }
        if (Test-IosValidationWorkflowPath -Path $path) {
            $iosValidationWorkflow = $true
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "ci-ios-workflow"
        }
        if (Test-OtherCiWorkflowPath -Path $path) {
            $otherCiWorkflow = $true
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "ci-workflow"
        }
        if (Test-CiClassifierPolicyPath -Path $path) {
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "ci-classifier-policy"
        }
        if (Test-RuntimeOrBuildPath -Path $path) {
            $runtimeOrBuild = $true
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "runtime-or-build"
        }
        if (Test-SourceCodePath -Path $path) {
            $sourceCode = $true
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "source-code"
        }
        if (Test-SanitizerRelevantPath -Path $path) {
            $sanitizerRelevant = $true
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "sanitizer-relevant"
        }
        if (Test-CoverageRelevantPath -Path $path) {
            $coverageRelevant = $true
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "coverage-relevant"
        }
        if (Test-Cpp23RelevantPath -Path $path) {
            $cpp23Relevant = $true
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "cpp23-relevant"
        }
        if (Test-WindowsCpuProfilingHostPath -Path $path) {
            $windowsCpuProfilingHost = $true
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "windows-cpu-profiling-host"
        }
        if (Test-WindowsAssetImportersPath -Path $path) {
            $windowsAssetImporters = $true
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "windows-asset-importers"
        }
        if (Test-WindowsDesktopEditorPath -Path $path) {
            $windowsDesktopEditor = $true
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "windows-desktop-editor"
        }
        if (Test-WindowsNetworkEnetPath -Path $path) {
            $windowsNetworkEnet = $true
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "windows-network-enet"
        }
        if (Test-LinuxVulkanHostEvidencePath -Path $path) {
            $linuxVulkanHostEvidence = $true
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "linux-vulkan-host-evidence"
        }
        if (Test-AppleHostEvidencePath -Path $path) {
            $appleHostEvidence = $true
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "apple-host-evidence"
        }
        if (Test-StaticPolicyPath -Path $path) {
            $staticPolicy = $true
            Add-UniqueClassificationReason -Reasons $classificationReasons -Reason "static-policy"
        }
    }

    $fullValidationWorkflowLane = $validationWorkflow -or $otherCiWorkflow
    $heavyBuildLane = $fullValidationWorkflowLane -or $runtimeOrBuild

    return New-ValidationTierSelectionObject -LaneSelection @{
        windows_msvc = $heavyBuildLane
        windows_cpu_profiling_host = ($fullValidationWorkflowLane -or $windowsCpuProfilingHost)
        windows_asset_importers = ($fullValidationWorkflowLane -or $windowsAssetImporters)
        windows_desktop_editor = ($fullValidationWorkflowLane -or $windowsDesktopEditor)
        windows_network_enet = ($fullValidationWorkflowLane -or $windowsNetworkEnet)
        linux_cmake = $heavyBuildLane
        linux_vulkan_host = ($fullValidationWorkflowLane -or $linuxVulkanHostEvidence)
        linux_sanitizers = ($fullValidationWorkflowLane -or $sanitizerRelevant)
        linux_coverage = ($fullValidationWorkflowLane -or $coverageRelevant)
        full_static_analysis = ($heavyBuildLane -or $staticPolicy -or $sourceCode)
        macos_metal_cmake = $heavyBuildLane
        metal_host_evidence = ($fullValidationWorkflowLane -or $appleHostEvidence)
        ios_metal_evidence = ($fullValidationWorkflowLane -or $iosValidationWorkflow -or $appleHostEvidence)
        windows_cpp23_release = ($fullValidationWorkflowLane -or $cpp23Relevant)
    } -Reason $classificationReasons.ToArray()
}

function Write-GitHubActionsOutput {
    param(
        [Parameter(Mandatory = $true)][pscustomobject]$Selection,
        [Parameter(Mandatory = $true)][string]$OutputPath
    )

    $lines = foreach ($laneName in $script:ValidationLaneNames) {
        "$laneName=$($Selection.$laneName.ToString().ToLowerInvariant())"
    }
    $lines += @(
        "selected_lanes=$($Selection.selected_lanes)",
        "classification_reasons=$($Selection.classification_reasons)"
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
