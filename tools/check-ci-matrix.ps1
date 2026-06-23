#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$repoRoot = Get-RepoRoot
$script:validationTierSelectionCache = @{}

function Read-RequiredText {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $path = Join-Path $repoRoot $RelativePath
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        Write-Error "ci-matrix-check: missing required file: $RelativePath"
    }

    return Get-Content -LiteralPath $path -Raw
}

function Assert-ContainsText {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Needle,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not $Text.Contains($Needle)) {
        Write-Error "ci-matrix-check: $Label missing expected text: $Needle"
    }
}

function Assert-DoesNotContainText {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Needle,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if ($Text.Contains($Needle)) {
        Write-Error "ci-matrix-check: $Label contains forbidden text: $Needle"
    }
}

function Assert-DoesNotContainAny {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string[]]$Needles,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($needle in $Needles) {
        Assert-DoesNotContainText -Text $Text -Needle $needle -Label $Label
    }
}

function Assert-ContainsAll {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string[]]$Needles,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($needle in $Needles) {
        Assert-ContainsText -Text $Text -Needle $needle -Label $Label
    }
}

function Assert-MatchesText {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not [System.Text.RegularExpressions.Regex]::IsMatch($Text, $Pattern, [System.Text.RegularExpressions.RegexOptions]::Multiline)) {
        Write-Error "ci-matrix-check: $Label missing expected pattern: $Pattern"
    }
}

function Assert-CheckoutRetryContract {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Label,
        [switch]$RequiresFetchDepthZero
    )

    Assert-ContainsAll $Text @(
        "name: Checkout",
        "id: checkout",
        "continue-on-error: true",
        "persist-credentials: false",
        "Retry checkout after transient fetch/auth failure",
        "if: steps.checkout.outcome != 'success'"
    ) $Label

    if ($RequiresFetchDepthZero) {
        Assert-ContainsText -Text $Text -Needle "fetch-depth: 0" -Label $Label
    }
}

function Assert-CcacheStatsGuard {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Label
    )

    Assert-ContainsAll $Text @(
        "Show ccache stats",
        "if command -v ccache >/dev/null 2>&1; then",
        "ccache --show-stats",
        "ccache is not installed; skipping stats after an earlier checkout or toolchain setup failure"
    ) $Label
}

function Get-WorkflowJobText {
    param(
        [Parameter(Mandatory = $true)][string]$WorkflowText,
        [Parameter(Mandatory = $true)][string]$JobName,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $pattern = "(?ms)^  " + [System.Text.RegularExpressions.Regex]::Escape($JobName) + ":\s*\r?\n(?<body>.*?)(?=^  [A-Za-z0-9_-]+:\s*(?:#.*)?\r?$|\z)"
    $match = [System.Text.RegularExpressions.Regex]::Match($WorkflowText, $pattern)
    if (-not $match.Success) {
        Write-Error "ci-matrix-check: $Label missing required job: $JobName"
    }

    return $match.Value
}

function Get-WorkflowStepText {
    param(
        [Parameter(Mandatory = $true)][string]$JobText,
        [Parameter(Mandatory = $true)][string]$StepName,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $pattern = "(?ms)^      - name: " + [System.Text.RegularExpressions.Regex]::Escape($StepName) + "\s*\r?\n(?<body>.*?)(?=^      - name: |\z)"
    $match = [System.Text.RegularExpressions.Regex]::Match($JobText, $pattern)
    if (-not $match.Success) {
        Write-Error "ci-matrix-check: $Label missing required step: $StepName"
    }

    return $match.Value
}

function Get-ValidationTierSelection {
    param(
        [Parameter(Mandatory = $true)][string]$Label,
        [string[]]$ChangedPath = @(),
        [switch]$RunAll
    )

    $cacheKey = "RunAll=$($RunAll.IsPresent);ChangedPath=$([string]::Join('|', $ChangedPath))"
    if ($script:validationTierSelectionCache.ContainsKey($cacheKey)) {
        return $script:validationTierSelectionCache[$cacheKey]
    }

    $classifierPath = Join-Path $repoRoot "tools/classify-pr-validation-tier.ps1"
    if (-not (Test-Path -LiteralPath $classifierPath -PathType Leaf)) {
        Write-Error "ci-matrix-check: missing PR validation tier classifier: tools/classify-pr-validation-tier.ps1"
    }

    $arguments = @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        $classifierPath
    )
    if ($RunAll) {
        $arguments += "-RunAll"
    }
    if ($ChangedPath.Count -gt 0) {
        $arguments += "-ChangedPath"
        $arguments += ($ChangedPath -join ",")
    }

    $output = & pwsh @arguments
    if ($LASTEXITCODE -ne 0) {
        Write-Error "ci-matrix-check: PR validation tier classifier failed for $Label"
    }

    $selection = $output | ConvertFrom-Json
    $script:validationTierSelectionCache[$cacheKey] = $selection
    return $selection
}

function Assert-ValidationTierSelection {
    param(
        [Parameter(Mandatory = $true)][string]$Label,
        [string[]]$ChangedPath = @(),
        [switch]$RunAll,
        [Parameter(Mandatory = $true)][bool]$ExpectedWindowsMsvc,
        [Parameter(Mandatory = $true)][bool]$ExpectedWindowsCpuProfilingHost,
        [Parameter(Mandatory = $true)][bool]$ExpectedWindowsAssetImporters,
        [Parameter(Mandatory = $true)][bool]$ExpectedWindowsDesktopEditor,
        [Parameter(Mandatory = $true)][bool]$ExpectedWindowsNetworkEnet,
        [Parameter(Mandatory = $true)][bool]$ExpectedLinuxCmake,
        [Parameter(Mandatory = $true)][bool]$ExpectedLinuxVulkanHost,
        [Parameter(Mandatory = $true)][bool]$ExpectedLinuxSanitizers,
        [bool]$ExpectedLinuxCoverage = $false,
        [Parameter(Mandatory = $true)][bool]$ExpectedFullStaticAnalysis,
        [bool]$ExpectedWindowsCpp23Release = $false,
        [Parameter(Mandatory = $true)][bool]$ExpectedMacosMetalCmake,
        [Parameter(Mandatory = $true)][bool]$ExpectedMetalHostEvidence,
        [Parameter(Mandatory = $true)][bool]$ExpectedIosMetalEvidence,
        [string]$ExpectedSelectedLanes,
        [string]$ExpectedClassificationReasons
    )

    $selection = Get-ValidationTierSelection -Label $Label -ChangedPath $ChangedPath -RunAll:$RunAll.IsPresent
    $expectations = @{
        windows_msvc = $ExpectedWindowsMsvc
        windows_cpu_profiling_host = $ExpectedWindowsCpuProfilingHost
        windows_asset_importers = $ExpectedWindowsAssetImporters
        windows_desktop_editor = $ExpectedWindowsDesktopEditor
        windows_network_enet = $ExpectedWindowsNetworkEnet
        linux_cmake = $ExpectedLinuxCmake
        linux_vulkan_host = $ExpectedLinuxVulkanHost
        linux_sanitizers = $ExpectedLinuxSanitizers
        linux_coverage = $ExpectedLinuxCoverage
        full_static_analysis = $ExpectedFullStaticAnalysis
        windows_cpp23_release = $ExpectedWindowsCpp23Release
        macos_metal_cmake = $ExpectedMacosMetalCmake
        metal_host_evidence = $ExpectedMetalHostEvidence
        ios_metal_evidence = $ExpectedIosMetalEvidence
    }

    foreach ($expectation in $expectations.GetEnumerator()) {
        $actual = [bool]$selection.($expectation.Key)
        if ($actual -ne $expectation.Value) {
            Write-Error "ci-matrix-check: PR validation tier classifier $Label expected $($expectation.Key)=$($expectation.Value) but got $actual"
        }
    }

    if ($PSBoundParameters.ContainsKey("ExpectedSelectedLanes") -and [string]$selection.selected_lanes -ne $ExpectedSelectedLanes) {
        Write-Error "ci-matrix-check: PR validation tier classifier $Label expected selected_lanes=$ExpectedSelectedLanes but got $($selection.selected_lanes)"
    }
    if ($PSBoundParameters.ContainsKey("ExpectedClassificationReasons") -and [string]$selection.classification_reasons -ne $ExpectedClassificationReasons) {
        Write-Error "ci-matrix-check: PR validation tier classifier $Label expected classification_reasons=$ExpectedClassificationReasons but got $($selection.classification_reasons)"
    }
}

$validateScript = Read-RequiredText "tools/validate.ps1"
Assert-ContainsText $validateScript "check-ci-matrix.ps1" "tools/validate.ps1 default validation hook"
Assert-ContainsAll $validateScript @(
    "[switch]`$StaticOnly",
    "[switch]`$SkipStaticChecks",
    "[switch]`$SkipTidySmoke",
    "validate: -StaticOnly and -SkipStaticChecks cannot be combined"
) "tools/validate.ps1 static/build split contract"

$cpp23EvaluationScript = Read-RequiredText "tools/evaluate-cpp23.ps1"
Assert-ContainsAll $cpp23EvaluationScript @(
    "[switch]`$Debug",
    "[ValidateRange(0, 1024)]",
    "[int]`$Jobs = 0",
    "`$effectiveJobs = Resolve-ParallelJobCount -Jobs `$Jobs",
    "cpp23-verification: cmake/ctest parallel jobs=`$effectiveJobs",
    "`$runDebug = `$Debug.IsPresent -or (-not `$Release.IsPresent -and -not `$Editor.IsPresent)",
    "if (`$runDebug) {",
    "Invoke-CheckedCommand `$tools.CMake --build --preset cpp23-eval --parallel `$effectiveJobs",
    "Invoke-CheckedCommand `$tools.CTest --preset cpp23-eval --output-on-failure --timeout 300 --parallel `$effectiveJobs",
    "release-package-artifacts.ps1",
    "Assert-ReleasePackageArtifacts",
    "cpp23-release-preset-eval",
    "Invoke-CheckedCommand `$tools.CMake --build --preset cpp23-release-eval --parallel `$effectiveJobs",
    "Invoke-CheckedCommand `$tools.CTest --preset cpp23-release-eval --output-on-failure --timeout 300 --parallel `$effectiveJobs",
    "cpp23-desktop-editor-eval",
    "Invoke-CheckedCommand `$tools.CMake --build --preset cpp23-desktop-editor-eval --parallel `$effectiveJobs",
    "Invoke-CheckedCommand `$tools.CTest --preset cpp23-desktop-editor-eval --output-on-failure --timeout 300 --parallel `$effectiveJobs"
) "tools/evaluate-cpp23.ps1 release artifact validation"
Assert-DoesNotContainText $cpp23EvaluationScript "Resolve-Cpp23EvaluationJobCount" "tools/evaluate-cpp23.ps1 shared parallel job helper"
Assert-DoesNotContainText $cpp23EvaluationScript "[Environment]::ProcessorCount" "tools/evaluate-cpp23.ps1 shared parallel job helper"
$cpp23CpackCallIndex = $cpp23EvaluationScript.IndexOf('Invoke-CheckedCommand $tools.CPack --preset cpp23-release-eval', [System.StringComparison]::Ordinal)
$cpp23ArtifactAssertIndex = $cpp23EvaluationScript.IndexOf('Assert-ReleasePackageArtifacts -BuildDir $releaseBuildDir', [System.StringComparison]::Ordinal)
if ($cpp23CpackCallIndex -lt 0 -or $cpp23ArtifactAssertIndex -lt 0 -or $cpp23ArtifactAssertIndex -lt $cpp23CpackCallIndex) {
    Write-Error "ci-matrix-check: tools/evaluate-cpp23.ps1 must run Assert-ReleasePackageArtifacts after CPack for the C++23 release lane"
}

$releasePackageArtifactsScript = Read-RequiredText "tools/release-package-artifacts.ps1"
Assert-ContainsAll $releasePackageArtifactsScript @(
    "Assert-ReleasePackageHasNoForbiddenRuntimeDlls",
    "SDL3.dll",
    "Release package archive must not ship SDL3.dll"
) "tools/release-package-artifacts.ps1 forbidden runtime DLL guard"

Assert-ValidationTierSelection `
    -Label "docs-only PR" `
    -ChangedPath @("docs/testing.md", "AGENTS.md", ".agents/skills/gameengine-agent-integration/SKILL.md") `
    -ExpectedWindowsMsvc $false `
    -ExpectedWindowsCpuProfilingHost $false `
    -ExpectedWindowsAssetImporters $false `
    -ExpectedWindowsDesktopEditor $false `
    -ExpectedWindowsNetworkEnet $false `
    -ExpectedLinuxCmake $false `
    -ExpectedLinuxVulkanHost $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedFullStaticAnalysis $false `
    -ExpectedWindowsCpp23Release $false `
    -ExpectedMacosMetalCmake $false `
    -ExpectedMetalHostEvidence $false `
    -ExpectedIosMetalEvidence $false `
    -ExpectedSelectedLanes "none" `
    -ExpectedClassificationReasons "docs-agent-rules-subagent-only"

Assert-ValidationTierSelection `
    -Label "static policy PR" `
    -ChangedPath @(".clang-tidy") `
    -ExpectedWindowsMsvc $false `
    -ExpectedWindowsCpuProfilingHost $false `
    -ExpectedWindowsAssetImporters $false `
    -ExpectedWindowsDesktopEditor $false `
    -ExpectedWindowsNetworkEnet $false `
    -ExpectedLinuxCmake $false `
    -ExpectedLinuxVulkanHost $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedFullStaticAnalysis $true `
    -ExpectedWindowsCpp23Release $false `
    -ExpectedMacosMetalCmake $false `
    -ExpectedMetalHostEvidence $false `
    -ExpectedIosMetalEvidence $false

Assert-ValidationTierSelection `
    -Label "runtime PR" `
    -ChangedPath @("engine/core/src/example.cpp") `
    -ExpectedWindowsMsvc $true `
    -ExpectedWindowsCpuProfilingHost $false `
    -ExpectedWindowsAssetImporters $false `
    -ExpectedWindowsDesktopEditor $false `
    -ExpectedWindowsNetworkEnet $false `
    -ExpectedLinuxCmake $true `
    -ExpectedLinuxVulkanHost $false `
    -ExpectedLinuxSanitizers $true `
    -ExpectedLinuxCoverage $false `
    -ExpectedFullStaticAnalysis $true `
    -ExpectedWindowsCpp23Release $false `
    -ExpectedMacosMetalCmake $true `
    -ExpectedMetalHostEvidence $false `
    -ExpectedIosMetalEvidence $false `
    -ExpectedSelectedLanes "windows_msvc,linux_cmake,linux_sanitizers,full_static_analysis,macos_metal_cmake" `
    -ExpectedClassificationReasons "runtime-or-build,sanitizer-relevant,source-code"

Assert-ValidationTierSelection `
    -Label "workflow PR" `
    -ChangedPath @(".github/workflows/validate.yml") `
    -ExpectedWindowsMsvc $true `
    -ExpectedWindowsCpuProfilingHost $true `
    -ExpectedWindowsAssetImporters $true `
    -ExpectedWindowsDesktopEditor $true `
    -ExpectedWindowsNetworkEnet $true `
    -ExpectedLinuxCmake $true `
    -ExpectedLinuxVulkanHost $true `
    -ExpectedLinuxSanitizers $true `
    -ExpectedLinuxCoverage $true `
    -ExpectedFullStaticAnalysis $true `
    -ExpectedWindowsCpp23Release $true `
    -ExpectedMacosMetalCmake $true `
    -ExpectedMetalHostEvidence $true `
    -ExpectedIosMetalEvidence $true `
    -ExpectedSelectedLanes "windows_msvc,windows_cpu_profiling_host,windows_asset_importers,windows_desktop_editor,windows_network_enet,linux_cmake,linux_vulkan_host,linux_sanitizers,linux_coverage,full_static_analysis,macos_metal_cmake,metal_host_evidence,ios_metal_evidence,windows_cpp23_release" `
    -ExpectedClassificationReasons "ci-validation-workflow"

Assert-ValidationTierSelection `
    -Label "iOS workflow PR" `
    -ChangedPath @(".github/workflows/ios-validate.yml") `
    -ExpectedWindowsMsvc $false `
    -ExpectedWindowsCpuProfilingHost $false `
    -ExpectedWindowsAssetImporters $false `
    -ExpectedWindowsDesktopEditor $false `
    -ExpectedWindowsNetworkEnet $false `
    -ExpectedLinuxCmake $false `
    -ExpectedLinuxVulkanHost $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedFullStaticAnalysis $false `
    -ExpectedWindowsCpp23Release $false `
    -ExpectedMacosMetalCmake $false `
    -ExpectedMetalHostEvidence $false `
    -ExpectedIosMetalEvidence $true `
    -ExpectedSelectedLanes "ios_metal_evidence" `
    -ExpectedClassificationReasons "ci-ios-workflow"

Assert-ValidationTierSelection `
    -Label "classifier policy PR" `
    -ChangedPath @("tools/classify-pr-validation-tier.ps1") `
    -ExpectedWindowsMsvc $false `
    -ExpectedWindowsCpuProfilingHost $false `
    -ExpectedWindowsAssetImporters $false `
    -ExpectedWindowsDesktopEditor $false `
    -ExpectedWindowsNetworkEnet $false `
    -ExpectedLinuxCmake $false `
    -ExpectedLinuxVulkanHost $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedFullStaticAnalysis $false `
    -ExpectedWindowsCpp23Release $false `
    -ExpectedMacosMetalCmake $false `
    -ExpectedMetalHostEvidence $false `
    -ExpectedIosMetalEvidence $false `
    -ExpectedSelectedLanes "none" `
    -ExpectedClassificationReasons "ci-classifier-policy"

Assert-ValidationTierSelection `
    -Label "ci matrix policy PR" `
    -ChangedPath @("tools/check-ci-matrix.ps1") `
    -ExpectedWindowsMsvc $false `
    -ExpectedWindowsCpuProfilingHost $false `
    -ExpectedWindowsAssetImporters $false `
    -ExpectedWindowsDesktopEditor $false `
    -ExpectedWindowsNetworkEnet $false `
    -ExpectedLinuxCmake $false `
    -ExpectedLinuxVulkanHost $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedFullStaticAnalysis $false `
    -ExpectedWindowsCpp23Release $false `
    -ExpectedMacosMetalCmake $false `
    -ExpectedMetalHostEvidence $false `
    -ExpectedIosMetalEvidence $false `
    -ExpectedSelectedLanes "none" `
    -ExpectedClassificationReasons "ci-classifier-policy"

Assert-ValidationTierSelection `
    -Label "cpp23 policy infra PR" `
    -ChangedPath @("tools/check-cpp-standard-policy.ps1") `
    -ExpectedWindowsMsvc $false `
    -ExpectedWindowsCpuProfilingHost $false `
    -ExpectedWindowsAssetImporters $false `
    -ExpectedWindowsDesktopEditor $false `
    -ExpectedWindowsNetworkEnet $false `
    -ExpectedLinuxCmake $false `
    -ExpectedLinuxVulkanHost $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedFullStaticAnalysis $false `
    -ExpectedWindowsCpp23Release $true `
    -ExpectedMacosMetalCmake $false `
    -ExpectedMetalHostEvidence $false `
    -ExpectedIosMetalEvidence $false

Assert-ValidationTierSelection `
    -Label "coverage infra PR" `
    -ChangedPath @("tools/check-coverage.ps1") `
    -ExpectedWindowsMsvc $false `
    -ExpectedWindowsCpuProfilingHost $false `
    -ExpectedWindowsAssetImporters $false `
    -ExpectedWindowsDesktopEditor $false `
    -ExpectedWindowsNetworkEnet $false `
    -ExpectedLinuxCmake $false `
    -ExpectedLinuxVulkanHost $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $true `
    -ExpectedFullStaticAnalysis $false `
    -ExpectedWindowsCpp23Release $false `
    -ExpectedMacosMetalCmake $false `
    -ExpectedMetalHostEvidence $false `
    -ExpectedIosMetalEvidence $false

Assert-ValidationTierSelection `
    -Label "cpp23 evaluate infra PR" `
    -ChangedPath @("tools/evaluate-cpp23.ps1") `
    -ExpectedWindowsMsvc $false `
    -ExpectedWindowsCpuProfilingHost $false `
    -ExpectedWindowsAssetImporters $false `
    -ExpectedWindowsDesktopEditor $false `
    -ExpectedWindowsNetworkEnet $false `
    -ExpectedLinuxCmake $false `
    -ExpectedLinuxVulkanHost $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedFullStaticAnalysis $false `
    -ExpectedWindowsCpp23Release $true `
    -ExpectedMacosMetalCmake $false `
    -ExpectedMetalHostEvidence $false `
    -ExpectedIosMetalEvidence $false

Assert-ValidationTierSelection `
    -Label "optional ENet validation wrapper PR" `
    -ChangedPath @("tools/validate-network-enet.ps1") `
    -ExpectedWindowsMsvc $false `
    -ExpectedWindowsCpuProfilingHost $false `
    -ExpectedWindowsAssetImporters $false `
    -ExpectedWindowsDesktopEditor $false `
    -ExpectedWindowsNetworkEnet $true `
    -ExpectedLinuxCmake $false `
    -ExpectedLinuxVulkanHost $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedFullStaticAnalysis $false `
    -ExpectedWindowsCpp23Release $false `
    -ExpectedMacosMetalCmake $false `
    -ExpectedMetalHostEvidence $false `
    -ExpectedIosMetalEvidence $false

Assert-ValidationTierSelection `
    -Label "optional asset importers validation wrapper PR" `
    -ChangedPath @("tools/build-asset-importers.ps1") `
    -ExpectedWindowsMsvc $false `
    -ExpectedWindowsCpuProfilingHost $false `
    -ExpectedWindowsAssetImporters $true `
    -ExpectedWindowsDesktopEditor $false `
    -ExpectedWindowsNetworkEnet $false `
    -ExpectedLinuxCmake $false `
    -ExpectedLinuxVulkanHost $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedFullStaticAnalysis $false `
    -ExpectedWindowsCpp23Release $false `
    -ExpectedMacosMetalCmake $false `
    -ExpectedMetalHostEvidence $false `
    -ExpectedIosMetalEvidence $false

Assert-ValidationTierSelection `
    -Label "native desktop editor validation wrapper PR" `
    -ChangedPath @("tools/build-editor.ps1") `
    -ExpectedWindowsMsvc $false `
    -ExpectedWindowsCpuProfilingHost $false `
    -ExpectedWindowsAssetImporters $false `
    -ExpectedWindowsDesktopEditor $true `
    -ExpectedWindowsNetworkEnet $false `
    -ExpectedLinuxCmake $false `
    -ExpectedLinuxVulkanHost $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedFullStaticAnalysis $false `
    -ExpectedWindowsCpp23Release $false `
    -ExpectedMacosMetalCmake $false `
    -ExpectedMetalHostEvidence $false `
    -ExpectedIosMetalEvidence $false

Assert-ValidationTierSelection `
    -Label "CPU profiling host validation wrapper PR" `
    -ChangedPath @("tools/validate-cpu-profiling-matrix-host-gate.ps1") `
    -ExpectedWindowsMsvc $false `
    -ExpectedWindowsCpuProfilingHost $true `
    -ExpectedWindowsAssetImporters $false `
    -ExpectedWindowsDesktopEditor $false `
    -ExpectedWindowsNetworkEnet $false `
    -ExpectedLinuxCmake $false `
    -ExpectedLinuxVulkanHost $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedFullStaticAnalysis $false `
    -ExpectedWindowsCpp23Release $false `
    -ExpectedMacosMetalCmake $false `
    -ExpectedMetalHostEvidence $false `
    -ExpectedIosMetalEvidence $false

Assert-ValidationTierSelection `
    -Label "Linux Vulkan host evidence PR" `
    -ChangedPath @("tools/validate-linux-vulkan-runtime-host.ps1") `
    -ExpectedWindowsMsvc $false `
    -ExpectedWindowsCpuProfilingHost $false `
    -ExpectedWindowsAssetImporters $false `
    -ExpectedWindowsDesktopEditor $false `
    -ExpectedWindowsNetworkEnet $false `
    -ExpectedLinuxCmake $false `
    -ExpectedLinuxVulkanHost $true `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedFullStaticAnalysis $false `
    -ExpectedWindowsCpp23Release $false `
    -ExpectedMacosMetalCmake $false `
    -ExpectedMetalHostEvidence $false `
    -ExpectedIosMetalEvidence $false

Assert-ValidationTierSelection `
    -Label "Apple Metal host evidence PR" `
    -ChangedPath @("tools/validate-apple-metal-platform-host.ps1") `
    -ExpectedWindowsMsvc $false `
    -ExpectedWindowsCpuProfilingHost $false `
    -ExpectedWindowsAssetImporters $false `
    -ExpectedWindowsDesktopEditor $false `
    -ExpectedWindowsNetworkEnet $false `
    -ExpectedLinuxCmake $false `
    -ExpectedLinuxVulkanHost $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedFullStaticAnalysis $false `
    -ExpectedWindowsCpp23Release $false `
    -ExpectedMacosMetalCmake $false `
    -ExpectedMetalHostEvidence $true `
    -ExpectedIosMetalEvidence $true

Assert-ValidationTierSelection `
    -Label "non-PR run" `
    -RunAll `
    -ExpectedWindowsMsvc $true `
    -ExpectedWindowsCpuProfilingHost $true `
    -ExpectedWindowsAssetImporters $true `
    -ExpectedWindowsDesktopEditor $true `
    -ExpectedWindowsNetworkEnet $true `
    -ExpectedLinuxCmake $true `
    -ExpectedLinuxVulkanHost $true `
    -ExpectedLinuxSanitizers $true `
    -ExpectedLinuxCoverage $true `
    -ExpectedFullStaticAnalysis $true `
    -ExpectedWindowsCpp23Release $true `
    -ExpectedMacosMetalCmake $true `
    -ExpectedMetalHostEvidence $true `
    -ExpectedIosMetalEvidence $true `
    -ExpectedSelectedLanes "windows_msvc,windows_cpu_profiling_host,windows_asset_importers,windows_desktop_editor,windows_network_enet,linux_cmake,linux_vulkan_host,linux_sanitizers,linux_coverage,full_static_analysis,macos_metal_cmake,metal_host_evidence,ios_metal_evidence,windows_cpp23_release" `
    -ExpectedClassificationReasons "non-pr-full-matrix"

$validateWorkflow = Read-RequiredText ".github/workflows/validate.yml"
$checkoutActionRef = "actions/checkout@de0fac2e4500dabe0009e67214ff5f5447ce83dd"
$cacheActionRef = "actions/cache@27d5ce7f107fe9357f9df03efb73ab90386fccae"
$cacheRestoreActionRef = "actions/cache/restore@27d5ce7f107fe9357f9df03efb73ab90386fccae"
$cacheSaveActionRef = "actions/cache/save@27d5ce7f107fe9357f9df03efb73ab90386fccae"
$uploadArtifactActionRef = "actions/upload-artifact@043fb46d1a93c77aae656e7c1c64a875d1fc6a0a"
Assert-ContainsText -Text $validateWorkflow -Needle $cacheActionRef -Label ".github/workflows/validate.yml cache action pin"
Assert-ContainsText -Text $validateWorkflow -Needle $cacheRestoreActionRef -Label ".github/workflows/validate.yml cache restore action pin"
Assert-ContainsText -Text $validateWorkflow -Needle $cacheSaveActionRef -Label ".github/workflows/validate.yml cache save action pin"
Assert-DoesNotContainAny $validateWorkflow @(
    "uses: actions/cache@v5",
    "uses: actions/checkout@v6",
    "uses: actions/upload-artifact@v7",
    "actions/checkout@v4",
    "actions/cache@v4",
    "actions/upload-artifact@v4",
    "if: always()",
    "runs-on: windows-latest",
    "FORCE_JAVASCRIPT_ACTIONS_TO_NODE24",
    "ACTIONS_ALLOW_USE_UNSECURE_NODE_VERSION"
) ".github/workflows/validate.yml Node 24 and VS2026 CI contract"
Assert-DoesNotContainAny $validateWorkflow @(
    "hashFiles('vcpkg.json', 'tools/bootstrap-deps.ps1', 'tools/common.ps1')",
    "key: `${{ runner.os }}-dev-build-`${{ hashFiles('CMakePresets.json', 'vcpkg.json', '**/CMakeLists.txt') }}",
    "key: `${{ runner.os }}-cpp23-build-`${{ hashFiles('CMakePresets.json', 'vcpkg.json', '**/CMakeLists.txt') }}",
    "hashFiles('CMakePresets.json', '**/CMakeLists.txt', 'tools/build.ps1', 'tools/test.ps1', 'tools/common.ps1')",
    "hashFiles('CMakePresets.json', '**/CMakeLists.txt', 'tools/evaluate-cpp23.ps1', 'tools/common.ps1', 'tools/release-package-artifacts.ps1', 'tools/build.ps1', 'tools/test.ps1')",
    "hashFiles('CMakePresets.json', 'tools/build.ps1', 'tools/test.ps1', 'tools/common.ps1')",
    "hashFiles('CMakePresets.json', 'tools/check-coverage.ps1', 'tools/common.ps1')",
    "hashFiles('CMakePresets.json', '**/CMakeLists.txt', 'tools/check-coverage.ps1', 'tools/common.ps1')",
    "hashFiles('CMakePresets.json', 'tools/test.ps1', 'tools/common.ps1')",
    "hashFiles('CMakePresets.json', '**/CMakeLists.txt', 'tools/test.ps1', 'tools/common.ps1')",
    "hashFiles('CMakePresets.json', 'tools/check-tidy.ps1', 'tools/common.ps1')",
    "hashFiles('CMakePresets.json', '**/CMakeLists.txt', 'tools/check-tidy.ps1', 'tools/common.ps1')"
) ".github/workflows/validate.yml cache invalidation drift"
Assert-ContainsAll $validateWorkflow @(
    "name: Validate",
    "push:",
    "branches:",
    "- main",
    "- master",
    "pull_request:",
    "merge_group:",
    "workflow_dispatch:",
    "permissions:",
    "contents: read",
    "concurrency:",
    'group: ${{ github.workflow }}-${{ github.event.pull_request.number || github.ref }}',
    "cancel-in-progress: true"
) ".github/workflows/validate.yml triggers"

$changesJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "changes" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $changesJob @(
    "name: Select PR validation tier",
    "runs-on: ubuntu-latest",
    "timeout-minutes: 10",
    "windows_msvc: `${{ steps.classify.outputs.windows_msvc }}",
    "windows_cpu_profiling_host: `${{ steps.classify.outputs.windows_cpu_profiling_host }}",
    "windows_asset_importers: `${{ steps.classify.outputs.windows_asset_importers }}",
    "windows_desktop_editor: `${{ steps.classify.outputs.windows_desktop_editor }}",
    "windows_network_enet: `${{ steps.classify.outputs.windows_network_enet }}",
    "linux_cmake: `${{ steps.classify.outputs.linux_cmake }}",
    "linux_vulkan_host: `${{ steps.classify.outputs.linux_vulkan_host }}",
    "linux_sanitizers: `${{ steps.classify.outputs.linux_sanitizers }}",
    "linux_coverage: `${{ steps.classify.outputs.linux_coverage }}",
    "full_static_analysis: `${{ steps.classify.outputs.full_static_analysis }}",
    "macos_metal_cmake: `${{ steps.classify.outputs.macos_metal_cmake }}",
    "metal_host_evidence: `${{ steps.classify.outputs.metal_host_evidence }}",
    "ios_metal_evidence: `${{ steps.classify.outputs.ios_metal_evidence }}",
    "windows_cpp23_release: `${{ steps.classify.outputs.windows_cpp23_release }}",
    "selected_lanes: `${{ steps.classify.outputs.selected_lanes }}",
    "classification_reasons: `${{ steps.classify.outputs.classification_reasons }}",
    "fetch-depth: 0",
    "persist-credentials: false",
    "name: Classify touched surfaces",
    'github.event_name',
    'git diff --name-only $base $head',
    "tools/classify-pr-validation-tier.ps1 -RunAll -GitHubOutputPath `$env:GITHUB_OUTPUT",
    "tools/classify-pr-validation-tier.ps1 -ChangedPath `$files -GitHubOutputPath `$env:GITHUB_OUTPUT",
    "PR validation tier selection",
    "selected_lanes",
    "classification_reasons",
    "GITHUB_STEP_SUMMARY"
) ".github/workflows/validate.yml changes job"

$windowsJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "windows" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $windowsJob @(
    "name: Windows MSVC",
    "needs: changes",
    "if: needs.changes.outputs.windows_msvc == 'true'",
    "runs-on: windows-2025-vs2026",
    "timeout-minutes: 120",
    $checkoutActionRef,
    "persist-credentials: false",
    "Restore vcpkg tool checkout cache",
    "path: external/vcpkg",
    "steps.restore-vcpkg-tool.outputs.cache-hit",
    "Test-Path external/vcpkg/.git",
    "external/vcpkg restored from cache",
    "steps.restore-vcpkg-tool.outcome == 'success'",
    "Update vcpkg tool baseline",
    "git -C external/vcpkg fetch --depth 1 origin",
    "git -C external/vcpkg checkout --force",
    "Capture Windows toolchain cache identity",
    "id: windows-toolchain-cache",
    "Microsoft Visual Studio\Installer\vswhere.exe",
    "ImageVersion",
    "windows-toolchain-cache-identity",
    "Restore vcpkg installed cache",
    "Restore vcpkg package cache",
    "id: restore-vcpkg-package",
    "id: restore-vcpkg-installed",
    "path: out/vcpkg",
    "path: vcpkg_installed",
    "Restore Windows dev build cache",
    $cacheRestoreActionRef,
    "Save vcpkg tool checkout cache",
    "key: `${{ steps.restore-vcpkg-tool.outputs.cache-primary-key }}",
    "path: out/build/dev",
    'key: ${{ runner.os }}-dev-build-${{ steps.windows-toolchain-cache.outputs.identity }}-${{ hashFiles(''CMakePresets.json'', ''vcpkg.json'', ''**/CMakeLists.txt'') }}-${{ github.sha }}',
    '${{ runner.os }}-dev-build-${{ steps.windows-toolchain-cache.outputs.identity }}-${{ hashFiles(''CMakePresets.json'', ''vcpkg.json'', ''**/CMakeLists.txt'') }}-',
    '${{ runner.os }}-dev-build-${{ steps.windows-toolchain-cache.outputs.identity }}-',
    "restore-dev-build",
    "run: ./tools/bootstrap-deps.ps1 -Feature desktop-runtime",
    "run: ./tools/validate.ps1 -SkipStaticChecks -SkipTidySmoke",
    "Save vcpkg package cache",
    "Save vcpkg installed cache",
    "steps.restore-vcpkg-package.outputs.cache-hit != 'true'",
    "steps.restore-vcpkg-installed.outputs.cache-hit != 'true'",
    "steps.restore-vcpkg-package.outcome == 'success'",
    "steps.restore-vcpkg-installed.outcome == 'success'",
    "key: `${{ steps.restore-vcpkg-package.outputs.cache-primary-key }}",
    "key: `${{ steps.restore-vcpkg-installed.outputs.cache-primary-key }}",
    "Save Windows dev build cache",
    "continue-on-error: true",
    $cacheSaveActionRef,
    "steps.restore-dev-build.outputs.cache-hit != 'true'",
    "steps.restore-dev-build.outcome == 'success'",
    "key: `${{ steps.restore-dev-build.outputs.cache-primary-key }}",
    'if: ${{ failure() && !cancelled() }}',
    $uploadArtifactActionRef,
    "name: windows-test-logs",
    "retention-days: 14",
    "include-hidden-files: false",
    "out/build/dev/Testing/**/*.log",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml windows job"
Assert-DoesNotContainAny $windowsJob @(
    "Validate CPU profiling matrix host gate",
    "run: ./tools/validate-cpu-profiling-matrix-host-gate.ps1",
    "run: ./tools/build-asset-importers.ps1",
    "run: ./tools/build-editor.ps1",
    "run: ./tools/validate-network-enet.ps1",
    "out/build/asset-importers/Testing/**/*.log"
) ".github/workflows/validate.yml windows job optional validation split"

$windowsCacheRestoreSteps = @(
    "Restore vcpkg tool checkout cache",
    "Restore vcpkg package cache",
    "Restore vcpkg installed cache",
    "Restore Windows dev build cache"
)
foreach ($restoreStepName in $windowsCacheRestoreSteps) {
    $restoreStepText = Get-WorkflowStepText -JobText $windowsJob -StepName $restoreStepName -Label ".github/workflows/validate.yml windows job"
    Assert-ContainsAll $restoreStepText @(
        "continue-on-error: true",
        $cacheRestoreActionRef
    ) ".github/workflows/validate.yml windows job $restoreStepName"
}

$windowsVcpkgToolSaveStep = Get-WorkflowStepText -JobText $windowsJob -StepName "Save vcpkg tool checkout cache" -Label ".github/workflows/validate.yml windows job"
Assert-ContainsAll $windowsVcpkgToolSaveStep @(
    "continue-on-error: true",
    $cacheSaveActionRef,
    "steps.restore-vcpkg-tool.outcome == 'success'",
    "key: `${{ steps.restore-vcpkg-tool.outputs.cache-primary-key }}"
) ".github/workflows/validate.yml windows job Save vcpkg tool checkout cache"

$windowsCpuProfilingJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "windows-cpu-profiling-host" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $windowsCpuProfilingJob @(
    "name: Windows CPU Profiling Host Evidence",
    "needs: changes",
    "if: needs.changes.outputs.windows_cpu_profiling_host == 'true'",
    "runs-on: windows-2025-vs2026",
    "timeout-minutes: 60",
    $checkoutActionRef,
    "persist-credentials: false",
    "Validate CPU profiling matrix host gate",
    "run: ./tools/validate-cpu-profiling-matrix-host-gate.ps1",
    'if: ${{ failure() && !cancelled() }}',
    $uploadArtifactActionRef,
    "name: windows-cpu-profiling-host-evidence",
    "retention-days: 14",
    "include-hidden-files: false",
    "out/cpu-profiling-matrix-host-gate/**",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml windows-cpu-profiling-host job"

$windowsAssetImportersJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "windows-asset-importers" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $windowsAssetImportersJob @(
    "name: Windows Optional Asset Importers",
    "needs: changes",
    "if: needs.changes.outputs.windows_asset_importers == 'true'",
    "runs-on: windows-2025-vs2026",
    "timeout-minutes: 90",
    $checkoutActionRef,
    "persist-credentials: false",
    "Restore asset importers vcpkg tool checkout cache",
    "id: restore-asset-importers-vcpkg-tool",
    "Restore asset importers vcpkg package cache",
    "id: restore-asset-importers-vcpkg-package",
    "Restore asset importers vcpkg installed cache",
    "id: restore-asset-importers-vcpkg-installed",
    "Restore asset importers build cache",
    "id: restore-asset-importers-build",
    "run: ./tools/bootstrap-deps.ps1 -Feature asset-importers",
    "Validate optional asset importers",
    "run: ./tools/build-asset-importers.ps1",
    "Save asset importers vcpkg tool checkout cache",
    "Save asset importers vcpkg package cache",
    "Save asset importers vcpkg installed cache",
    "Save asset importers build cache",
    "key: `${{ steps.restore-asset-importers-vcpkg-tool.outputs.cache-primary-key }}",
    "key: `${{ steps.restore-asset-importers-vcpkg-package.outputs.cache-primary-key }}",
    "key: `${{ steps.restore-asset-importers-vcpkg-installed.outputs.cache-primary-key }}",
    "key: `${{ steps.restore-asset-importers-build.outputs.cache-primary-key }}",
    'if: ${{ failure() && !cancelled() }}',
    $uploadArtifactActionRef,
    "name: windows-asset-importers-test-logs",
    "retention-days: 14",
    "include-hidden-files: false",
    "out/build/asset-importers/Testing/**/*.log",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml windows-asset-importers job"

$windowsDesktopEditorJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "windows-desktop-editor" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $windowsDesktopEditorJob @(
    "name: Windows Native Desktop Editor",
    "needs: changes",
    "if: needs.changes.outputs.windows_desktop_editor == 'true'",
    "runs-on: windows-2025-vs2026",
    "timeout-minutes: 90",
    $checkoutActionRef,
    "persist-credentials: false",
    "Capture Windows toolchain cache identity",
    "id: windows-toolchain-cache",
    "windows-toolchain-cache-identity",
    "Restore desktop editor build cache",
    "id: restore-desktop-editor-build",
    "Validate native desktop editor",
    "run: ./tools/build-editor.ps1",
    "Save desktop editor build cache",
    "key: `${{ steps.restore-desktop-editor-build.outputs.cache-primary-key }}",
    'if: ${{ failure() && !cancelled() }}',
    $uploadArtifactActionRef,
    "name: windows-desktop-editor-test-logs",
    "retention-days: 14",
    "include-hidden-files: false",
    "out/build/desktop-editor/Testing/**/*.log",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml windows-desktop-editor job"

$windowsNetworkEnetJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "windows-network-enet" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $windowsNetworkEnetJob @(
    "name: Windows Optional ENet Network Adapter",
    "needs: changes",
    "if: needs.changes.outputs.windows_network_enet == 'true'",
    "runs-on: windows-2025-vs2026",
    "timeout-minutes: 90",
    $checkoutActionRef,
    "persist-credentials: false",
    "Restore ENet vcpkg tool checkout cache",
    "id: restore-network-enet-vcpkg-tool",
    "Restore ENet vcpkg package cache",
    "id: restore-network-enet-vcpkg-package",
    "Restore ENet vcpkg installed cache",
    "id: restore-network-enet-vcpkg-installed",
    "Restore ENet build cache",
    "id: restore-network-enet-build",
    "run: ./tools/bootstrap-deps.ps1 -Feature network-enet",
    "Validate optional ENet network adapter",
    "run: ./tools/validate-network-enet.ps1",
    "Save ENet vcpkg tool checkout cache",
    "Save ENet vcpkg package cache",
    "Save ENet vcpkg installed cache",
    "Save ENet build cache",
    "key: `${{ steps.restore-network-enet-vcpkg-tool.outputs.cache-primary-key }}",
    "key: `${{ steps.restore-network-enet-vcpkg-package.outputs.cache-primary-key }}",
    "key: `${{ steps.restore-network-enet-vcpkg-installed.outputs.cache-primary-key }}",
    "key: `${{ steps.restore-network-enet-build.outputs.cache-primary-key }}",
    'if: ${{ failure() && !cancelled() }}',
    $uploadArtifactActionRef,
    "name: windows-network-enet-test-logs",
    "retention-days: 14",
    "include-hidden-files: false",
    "out/build/network-enet/Testing/**/*.log",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml windows-network-enet job"

$windowsCpp23Job = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "windows-cpp23" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $windowsCpp23Job @(
    "name: Windows C++23 Release Evaluation",
    "needs: changes",
    "if: needs.changes.outputs.windows_cpp23_release == 'true'",
    "runs-on: windows-2025-vs2026",
    "timeout-minutes: 150",
    $checkoutActionRef,
    "persist-credentials: false",
    "Restore vcpkg tool checkout cache",
    "path: external/vcpkg",
    "steps.restore-vcpkg-tool.outputs.cache-hit",
    "Test-Path external/vcpkg/.git",
    "external/vcpkg restored from cache",
    "steps.restore-vcpkg-tool.outcome == 'success'",
    "Update vcpkg tool baseline",
    "git -C external/vcpkg fetch --depth 1 origin",
    "git -C external/vcpkg checkout --force",
    "Capture Windows toolchain cache identity",
    "id: windows-toolchain-cache",
    "Microsoft Visual Studio\Installer\vswhere.exe",
    "ImageVersion",
    "windows-toolchain-cache-identity",
    "Restore vcpkg installed cache",
    "Restore vcpkg package cache",
    "id: restore-vcpkg-package",
    "id: restore-vcpkg-installed",
    "path: out/vcpkg",
    "path: vcpkg_installed",
    "Restore Windows C++23 build cache",
    $cacheRestoreActionRef,
    "Save vcpkg tool checkout cache",
    "key: `${{ steps.restore-vcpkg-tool.outputs.cache-primary-key }}",
    "out/build/cpp23-release-preset-eval",
    "out/build/installed-consumer-cpp23-release-eval",
    "out/install/cpp23-release-eval",
    'key: ${{ runner.os }}-cpp23-build-${{ steps.windows-toolchain-cache.outputs.identity }}-${{ hashFiles(''CMakePresets.json'', ''vcpkg.json'', ''**/CMakeLists.txt'') }}-${{ github.sha }}',
    '${{ runner.os }}-cpp23-build-${{ steps.windows-toolchain-cache.outputs.identity }}-${{ hashFiles(''CMakePresets.json'', ''vcpkg.json'', ''**/CMakeLists.txt'') }}-',
    '${{ runner.os }}-cpp23-build-${{ steps.windows-toolchain-cache.outputs.identity }}-',
    "restore-cpp23-build",
    "run: ./tools/bootstrap-deps.ps1 -Feature desktop-runtime",
    "run: ./tools/evaluate-cpp23.ps1 -Release",
    "Save vcpkg package cache",
    "Save vcpkg installed cache",
    "steps.restore-vcpkg-package.outputs.cache-hit != 'true'",
    "steps.restore-vcpkg-installed.outputs.cache-hit != 'true'",
    "steps.restore-vcpkg-package.outcome == 'success'",
    "steps.restore-vcpkg-installed.outcome == 'success'",
    "key: `${{ steps.restore-vcpkg-package.outputs.cache-primary-key }}",
    "key: `${{ steps.restore-vcpkg-installed.outputs.cache-primary-key }}",
    "Save Windows C++23 build cache",
    "continue-on-error: true",
    $cacheSaveActionRef,
    "steps.restore-cpp23-build.outputs.cache-hit != 'true'",
    "steps.restore-cpp23-build.outcome == 'success'",
    "key: `${{ steps.restore-cpp23-build.outputs.cache-primary-key }}",
    'if: ${{ failure() && !cancelled() }}',
    $uploadArtifactActionRef,
    "name: windows-cpp23-test-logs",
    "retention-days: 14",
    "include-hidden-files: false",
    "out/build/cpp23-release-preset-eval/Testing/**/*.log",
    "if-no-files-found: warn",
    "name: windows-packages",
    "retention-days: 14",
    "compression-level: 0",
    "include-hidden-files: false",
    "out/build/cpp23-release-preset-eval/*.zip",
    "out/build/cpp23-release-preset-eval/*.zip.sha256",
    "if-no-files-found: error",
    'if: ${{ success() && !cancelled() }}'
) ".github/workflows/validate.yml windows-cpp23 job"

$windowsCpp23CacheRestoreSteps = @(
    "Restore vcpkg tool checkout cache",
    "Restore vcpkg package cache",
    "Restore vcpkg installed cache",
    "Restore Windows C++23 build cache"
)
foreach ($restoreStepName in $windowsCpp23CacheRestoreSteps) {
    $restoreStepText = Get-WorkflowStepText -JobText $windowsCpp23Job -StepName $restoreStepName -Label ".github/workflows/validate.yml windows-cpp23 job"
    Assert-ContainsAll $restoreStepText @(
        "continue-on-error: true",
        $cacheRestoreActionRef
    ) ".github/workflows/validate.yml windows-cpp23 job $restoreStepName"
}

$windowsCpp23VcpkgToolSaveStep = Get-WorkflowStepText -JobText $windowsCpp23Job -StepName "Save vcpkg tool checkout cache" -Label ".github/workflows/validate.yml windows-cpp23 job"
Assert-ContainsAll $windowsCpp23VcpkgToolSaveStep @(
    "continue-on-error: true",
    $cacheSaveActionRef,
    "steps.restore-vcpkg-tool.outcome == 'success'",
    "key: `${{ steps.restore-vcpkg-tool.outputs.cache-primary-key }}"
) ".github/workflows/validate.yml windows-cpp23 job Save vcpkg tool checkout cache"

$agentStaticJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "agent-static" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $agentStaticJob @(
    "name: Agent Static Guards",
    "needs: changes",
    "runs-on: windows-2025-vs2026",
    "timeout-minutes: 20",
    $checkoutActionRef,
    "persist-credentials: false",
    "fetch-depth: 0",
    'if: ${{ github.event_name == ''pull_request'' }}',
    'git diff --check ${{ github.event.pull_request.base.sha }} ${{ github.event.pull_request.head.sha }}',
    "Run static validation",
    "run: ./tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120"
) ".github/workflows/validate.yml agent-static job"

$linuxJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "linux" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $linuxJob @(
    "name: Linux CMake",
    "needs: changes",
    "if: needs.changes.outputs.linux_cmake == 'true'",
    "runs-on: ubuntu-latest",
    "timeout-minutes: 60",
    $checkoutActionRef,
    "persist-credentials: false",
    "CCACHE_DIR: `${{ github.workspace }}/.ccache",
    "CCACHE_BASEDIR: `${{ github.workspace }}",
    "CCACHE_COMPRESS: true",
    "CCACHE_COMPRESSLEVEL: 6",
    "CCACHE_MAXSIZE: 2G",
    "CMAKE_CXX_COMPILER_LAUNCHER: ccache",
    "CMAKE_C_COMPILER_LAUNCHER: ccache",
    "Restore Linux ccache",
    "path: .ccache",
    "restore-linux-ccache",
    "Restore Linux CMake build cache",
    "path: out/build/ci-linux-clang",
    "restore-linux-build",
    "Verify preinstalled toolchain and install ccache",
    'command -v clang >/dev/null 2>&1 || { echo "clang is required on ubuntu-latest runner images"; exit 1; }',
    'command -v g++ >/dev/null 2>&1 || { echo "g++ is required on ubuntu-latest runner images"; exit 1; }',
    'command -v ninja >/dev/null 2>&1 || { echo "ninja is required on ubuntu-latest runner images"; exit 1; }',
    'if ! command -v ccache >/dev/null 2>&1; then',
    "sudo apt-get install -y --no-install-recommends --no-install-suggests ccache",
    "Prepare ccache",
    "ccache --zero-stats",
    "cmake --preset ci-linux-clang",
    'cmake --build --preset ci-linux-clang --parallel "$(nproc)"',
    'ctest --preset ci-linux-clang --output-on-failure --parallel "$(nproc)"',
    "Show ccache stats",
    "ccache --show-stats",
    'if: ${{ always() && !cancelled() }}',
    'if: ${{ failure() && !cancelled() }}',
    $uploadArtifactActionRef,
    "name: linux-test-logs",
    "retention-days: 14",
    "include-hidden-files: false",
    "out/build/ci-linux-clang/Testing/**/*.log",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml linux job"

$linuxVulkanJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "linux-vulkan" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $linuxVulkanJob @(
    "name: Linux Vulkan Host Evidence",
    "needs: changes",
    "if: needs.changes.outputs.linux_vulkan_host == 'true'",
    "runs-on: ubuntu-latest",
    "timeout-minutes: 60",
    $checkoutActionRef,
    "persist-credentials: false",
    "Restore Linux Vulkan vcpkg tool checkout cache",
    "id: restore-linux-vulkan-vcpkg-tool",
    "path: external/vcpkg",
    "Clone Linux Vulkan vcpkg tool checkout",
    "external/vcpkg restored from cache",
    "Update Linux Vulkan vcpkg tool baseline",
    "git -C external/vcpkg fetch --depth 1 origin",
    "git -C external/vcpkg checkout --force",
    "Bootstrap Linux Vulkan vcpkg executable",
    "Set-MirakanaiVcpkgEnvironment",
    "bootstrap-vcpkg.sh -disableMetrics",
    "Install Linux Vulkan diagnostic packages",
    "lunarg-signing-key-pub.asc",
    "lunarg-vulkan-`${lunarg_codename}.list",
    "libxcb-cursor-dev",
    "libxcb-xinerama0",
    "libxcb-xinput0",
    "mesa-vulkan-drivers",
    "vulkan-sdk",
    "xauth",
    "xvfb",
    "command -v dxc",
    "command -v vulkaninfo",
    "command -v spirv-val",
    "Capture Vulkan host summaries",
    "vulkaninfo --summary",
    "vulkaninfo --json",
    "Validate Linux Vulkan host evidence gate",
    "xvfb-run -a pwsh",
    "./tools/validate-linux-vulkan-runtime-host.ps1 -RequireReady -PackageSmokeTimeoutSeconds 2400 -ExpectedEvidenceCounters `$expected",
    "validation_recipe=environment-platform-linux-vulkan-package",
    "linux_package_smoke_ready=1",
    "linux_vulkan_readback_ready=1",
    "linux_vulkan_validation_log_clean=1",
    "environment_platform_linux_vulkan_ready=1",
    "environment_platform_requires_linux_vulkan_host_evidence=0",
    "environment_all_platform_unconditional_ready=0",
    "Save Linux Vulkan vcpkg tool checkout cache",
    "steps.restore-linux-vulkan-vcpkg-tool.outcome == 'success'",
    "Upload Linux Vulkan host evidence",
    'if: ${{ always() && !cancelled() }}',
    $uploadArtifactActionRef,
    "name: linux-vulkan-host-evidence",
    "retention-days: 14",
    "compression-level: 0",
    "include-hidden-files: false",
    "artifacts/environment/platform/linux-vulkan-host/**",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml linux-vulkan job"

$linuxCoverageJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "linux-coverage" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $linuxCoverageJob @(
    "name: Linux Coverage",
    "needs: changes",
    "if: needs.changes.outputs.linux_coverage == 'true'",
    "runs-on: ubuntu-latest",
    "timeout-minutes: 60",
    $checkoutActionRef,
    "persist-credentials: false",
    "CCACHE_DIR: `${{ github.workspace }}/.ccache",
    "CCACHE_BASEDIR: `${{ github.workspace }}",
    "CCACHE_COMPRESS: true",
    "CCACHE_COMPRESSLEVEL: 6",
    "CCACHE_MAXSIZE: 2G",
    "CMAKE_CXX_COMPILER_LAUNCHER: ccache",
    "CMAKE_C_COMPILER_LAUNCHER: ccache",
    "Restore Linux coverage ccache",
    "path: .ccache",
    "restore-linux-coverage-ccache",
    "Restore Linux coverage build cache",
    "path: out/build/coverage",
    "restore-linux-coverage-build",
    "Verify preinstalled toolchain and install coverage extras",
    'command -v clang >/dev/null 2>&1 || { echo "clang is required on ubuntu-latest runner images"; exit 1; }',
    'command -v ninja >/dev/null 2>&1 || { echo "ninja is required on ubuntu-latest runner images"; exit 1; }',
    "missing_packages=()",
    'command -v ccache >/dev/null 2>&1 || missing_packages+=("ccache")',
    'command -v lcov >/dev/null 2>&1 || missing_packages+=("lcov")',
    'if [ "${#missing_packages[@]}" -gt 0 ]; then',
    'sudo apt-get install -y --no-install-recommends --no-install-suggests "${missing_packages[@]}"',
    "Prepare ccache",
    "ccache --zero-stats",
    "run: ./tools/check-coverage.ps1 -Strict",
    "Show ccache stats",
    "ccache --show-stats",
    'if: ${{ always() && !cancelled() }}',
    'if: ${{ failure() && !cancelled() }}',
    $uploadArtifactActionRef,
    "name: linux-coverage",
    "retention-days: 14",
    "include-hidden-files: false",
    "out/build/coverage/Testing/**",
    "out/build/coverage/coverage*.info",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml linux-coverage job"

$sanitizerJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "linux-sanitizers" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $sanitizerJob @(
    "name: Linux Clang ASan/UBSan",
    "needs: changes",
    "if: needs.changes.outputs.linux_sanitizers == 'true'",
    "runs-on: ubuntu-latest",
    "timeout-minutes: 60",
    $checkoutActionRef,
    "persist-credentials: false",
    "CCACHE_DIR: `${{ github.workspace }}/.ccache",
    "CCACHE_BASEDIR: `${{ github.workspace }}",
    "CCACHE_COMPRESS: true",
    "CCACHE_COMPRESSLEVEL: 6",
    "CCACHE_MAXSIZE: 2G",
    "CMAKE_CXX_COMPILER_LAUNCHER: ccache",
    "CMAKE_C_COMPILER_LAUNCHER: ccache",
    "Restore Linux sanitizer ccache",
    "path: .ccache",
    "restore-linux-sanitizer-ccache",
    "Restore Linux sanitizer build cache",
    "path: out/build/clang-asan-ubsan",
    "restore-linux-sanitizer-build",
    "Verify preinstalled toolchain and install ccache",
    'command -v clang >/dev/null 2>&1 || { echo "clang is required on ubuntu-latest runner images"; exit 1; }',
    'command -v ninja >/dev/null 2>&1 || { echo "ninja is required on ubuntu-latest runner images"; exit 1; }',
    'if ! command -v ccache >/dev/null 2>&1; then',
    "sudo apt-get install -y --no-install-recommends --no-install-suggests ccache",
    "Prepare ccache",
    "ccache --zero-stats",
    "cmake --preset clang-asan-ubsan",
    'cmake --build --preset clang-asan-ubsan --parallel "$(nproc)"',
    "UBSAN_OPTIONS: print_stacktrace=1",
    'ctest --preset clang-asan-ubsan --output-on-failure --parallel "$(nproc)"',
    "Show ccache stats",
    "ccache --show-stats",
    'if: ${{ always() && !cancelled() }}',
    'if: ${{ failure() && !cancelled() }}',
    $uploadArtifactActionRef,
    "name: linux-sanitizer-test-logs",
    "retention-days: 14",
    "include-hidden-files: false",
    "out/build/clang-asan-ubsan/Testing/**/*.log",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml linux-sanitizers job"

$macosJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "macos" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $macosJob @(
    "name: macOS Metal CMake",
    "needs: changes",
    "if: needs.changes.outputs.macos_metal_cmake == 'true'",
    "runs-on: macos-latest",
    "timeout-minutes: 90",
    $checkoutActionRef,
    "persist-credentials: false",
    "CCACHE_DIR: `${{ github.workspace }}/.ccache",
    "CCACHE_BASEDIR: `${{ github.workspace }}",
    "CCACHE_COMPRESS: true",
    "CCACHE_COMPRESSLEVEL: 6",
    "CCACHE_MAXSIZE: 2G",
    "CMAKE_CXX_COMPILER_LAUNCHER: ccache",
    "CMAKE_C_COMPILER_LAUNCHER: ccache",
    "Restore macOS ccache",
    "path: .ccache",
    "restore-macos-ccache",
    "Restore macOS build cache",
    "path: out/build/ci-macos-appleclang",
    "restore-macos-build",
    "name: Ensure build tools are available",
    'command -v ninja >/dev/null 2>&1 || { echo "ninja is required on macos-latest runner images"; exit 1; }',
    "ninja --version",
    "ccache --version",
    "brew install ccache",
    "Prepare ccache",
    "ccache --zero-stats",
    "Environment Metal aggregate host evidence recipe and optimization artifacts",
    "if: needs.changes.outputs.metal_host_evidence == 'true'",
    '$jobs = [int](& sysctl -n hw.logicalcpu)',
    "./tools/generate-environment-metal-optimization-artifacts.ps1 -Jobs `$jobs -RequireReady",
    "Upload Metal optimization artifacts",
    "name: metal-host-optimization-artifacts",
    "compression-level: 0",
    "artifacts/environment/optimization/2026-06-19-metal-host-xctrace-smoke/metal_apple_host/**",
    "Validate Metal weather solver host gate",
    "./tools/validate-environment-weather-metal-solver-host-gate.ps1",
    "cmake --preset ci-macos-appleclang",
    'cmake --build --preset ci-macos-appleclang --parallel "$(sysctl -n hw.logicalcpu)"',
    'ctest --preset ci-macos-appleclang --output-on-failure --parallel "$(sysctl -n hw.logicalcpu)"',
    "Show ccache stats",
    "ccache --show-stats",
    'if: ${{ always() && !cancelled() }}',
    'if: ${{ failure() && !cancelled() }}',
    $uploadArtifactActionRef,
    "name: macos-test-logs",
    "retention-days: 14",
    "include-hidden-files: false",
    "out/build/ci-macos-appleclang/Testing/**/*.log",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml macos job"

$iosMetalJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "ios-metal" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $iosMetalJob @(
    "name: iOS Metal Evidence",
    "needs: changes",
    "if: needs.changes.outputs.ios_metal_evidence == 'true'",
    "runs-on: macos-26",
    "timeout-minutes: 60",
    $checkoutActionRef,
    "persist-credentials: false",
    "xcodebuild -version",
    "xcode-select -p",
    "xcrun --sdk iphonesimulator --show-sdk-path",
    "xcrun simctl list runtimes",
    "xcrun simctl list devices available",
    "./tools/check-mobile-packaging.ps1 -RequireApple",
    "Build iOS Simulator package",
    "./tools/build-mobile-apple.ps1 -Game sample_headless -Configuration Debug -Platform Simulator",
    "Validate iOS Metal platform evidence",
    "./tools/validate-apple-metal-platform-host.ps1 -Platform ios -RequireReady -SkipIosBuild -ExpectedEvidenceCounters `$expected",
    "validation_recipe=environment-platform-ios-metal-package",
    "host=macos",
    "xcode_ios_sdk_ready=1",
    "ios_simulator_or_device_ready=1",
    "ios_metal_feature_set_checked=1",
    "ios_package_smoke_ready=1",
    "ios_metal_command_queue_ready=1",
    "ios_metal_pipeline_ready=1",
    "ios_metal_command_buffer_ready=1",
    "ios_metal_readback_ready=1",
    "environment_platform_ios_metal_ready=1",
    "environment_platform_requires_ios_metal_host_evidence=0",
    "environment_all_platform_unconditional_ready=0",
    'if: ${{ failure() && !cancelled() }}',
    $uploadArtifactActionRef,
    "name: ios-metal-evidence-build",
    "retention-days: 14",
    "include-hidden-files: false",
    "out/build/ios-Simulator-sample_headless-Debug/**/*.app",
    "out/build/ios-Simulator-sample_headless-Debug/**/*.log",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml ios-metal job"

$staticAnalysisJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "static-analysis" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $staticAnalysisJob @(
    "name: Full Repository Static Analysis",
    "needs: changes",
    "if: needs.changes.outputs.full_static_analysis == 'true'",
    "runs-on: ubuntu-latest",
    "timeout-minutes: 90",
    "strategy:",
    "fail-fast: false",
    "shard_index: 0",
    "shard_index: 1",
    "shard_index: 2",
    "shard_index: 3",
    $checkoutActionRef,
    "persist-credentials: false",
    "CCACHE_DIR: `${{ github.workspace }}/.ccache",
    "CCACHE_BASEDIR: `${{ github.workspace }}",
    "CCACHE_COMPRESS: true",
    "CCACHE_COMPRESSLEVEL: 6",
    "CCACHE_MAXSIZE: 2G",
    "CMAKE_CXX_COMPILER_LAUNCHER: ccache",
    "CMAKE_C_COMPILER_LAUNCHER: ccache",
    "Restore Linux tidy ccache",
    "path: .ccache",
    "restore-linux-tidy-ccache",
    "Restore Linux tidy build cache",
    "path: out/build/ci-linux-tidy",
    "restore-linux-tidy-build",
    "Verify preinstalled toolchain and install ccache",
    'command -v clang >/dev/null 2>&1 || { echo "clang is required on ubuntu-latest runner images"; exit 1; }',
    'command -v clang-tidy >/dev/null 2>&1 || { echo "clang-tidy is required on ubuntu-latest runner images"; exit 1; }',
    'command -v ninja >/dev/null 2>&1 || { echo "ninja is required on ubuntu-latest runner images"; exit 1; }',
    'if ! command -v ccache >/dev/null 2>&1; then',
    "sudo apt-get install -y --no-install-recommends --no-install-suggests ccache",
    "Prepare ccache",
    "ccache --zero-stats",
    "./tools/check-tidy.ps1 -Strict -Preset ci-linux-tidy",
    "-ShardCount 4",
    "-ShardIndex `${{ matrix.shard_index }}",
    "-Jobs 0",
    "Show ccache stats",
    "ccache --show-stats",
    'if: ${{ always() && !cancelled() }}',
    'if: ${{ failure() && !cancelled() }}',
    $uploadArtifactActionRef,
    'name: static-analysis-tidy-logs-${{ matrix.shard_index }}',
    "retention-days: 14",
    "include-hidden-files: false",
    "out/build/ci-linux-tidy/compile_commands.json",
    "out/build/ci-linux-tidy/.cmake/api/v1/reply/*.json",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml static-analysis job"

$prGateJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "pr-gate" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $prGateJob @(
    "name: PR Gate",
    "- changes",
    "- agent-static",
    "- windows",
    "- windows-cpu-profiling-host",
    "- windows-asset-importers",
    "- windows-desktop-editor",
    "- windows-network-enet",
    "- linux",
    "- linux-vulkan",
    "- windows-cpp23",
    "- linux-coverage",
    "- linux-sanitizers",
    "- static-analysis",
    "- macos",
    "- ios-metal",
    'if: ${{ always() && !cancelled() }}',
    "timeout-minutes: 10",
    "`$laneJobs = [ordered]@{",
    "windows_msvc = `"windows`"",
    "ios_metal_evidence = `"ios-metal`"",
    "`$selected = [string]`$outputs.`$laneName -eq `"true`"",
    "`$jobResult -ne `"success`"",
    "selected lane did not complete successfully",
    'toJson(needs)',
    'failure',
    'cancelled',
    "PR validation gate passed"
) ".github/workflows/validate.yml pr-gate job"

$iosWorkflow = Read-RequiredText ".github/workflows/ios-validate.yml"
Assert-DoesNotContainAny $iosWorkflow @(
    "uses: actions/checkout@v6",
    "uses: actions/upload-artifact@v7",
    "actions/checkout@v4",
    "actions/upload-artifact@v4",
    "if: always()",
    "FORCE_JAVASCRIPT_ACTIONS_TO_NODE24",
    "ACTIONS_ALLOW_USE_UNSECURE_NODE_VERSION"
) ".github/workflows/ios-validate.yml Node 24 CI contract"
Assert-ContainsAll $iosWorkflow @(
    "name: iOS Validate",
    "workflow_dispatch:",
    "pull_request:",
    "push:",
    "branches:",
    "- main",
    "- master",
    '".github/workflows/ios-validate.yml"',
    '"platform/ios/**"',
    '"tools/apple-host-helpers.ps1"',
    '"tools/build-mobile-apple.ps1"',
    '"tools/check-mobile-packaging.ps1"',
    '"tools/smoke-ios-package.ps1"',
    '"tools/validate-apple-metal-platform-host.ps1"',
    "permissions:",
    "contents: read",
    "concurrency:",
    'group: ${{ github.workflow }}-${{ github.event.pull_request.number || github.ref }}',
    "cancel-in-progress: true"
) ".github/workflows/ios-validate.yml triggers and path filters"

$iosJob = Get-WorkflowJobText -WorkflowText $iosWorkflow -JobName "simulator-smoke" -Label ".github/workflows/ios-validate.yml"
Assert-ContainsAll $iosJob @(
    "name: iOS Simulator smoke",
    "runs-on: macos-26",
    "timeout-minutes: 60",
    $checkoutActionRef,
    "persist-credentials: false",
    "xcodebuild -version",
    "xcode-select -p",
    "xcrun --sdk iphonesimulator --show-sdk-path",
    "xcrun simctl list runtimes",
    "xcrun simctl list devices available",
    "./tools/check-mobile-packaging.ps1 -RequireApple",
    "Build iOS Simulator package",
    "./tools/build-mobile-apple.ps1 -Game sample_headless -Configuration Debug -Platform Simulator",
    "Validate iOS Metal platform evidence",
    "./tools/validate-apple-metal-platform-host.ps1 -Platform ios -RequireReady -SkipIosBuild -ExpectedEvidenceCounters `$expected",
    "validation_recipe=environment-platform-ios-metal-package",
    "host=macos",
    "xcode_ios_sdk_ready=1",
    "ios_simulator_or_device_ready=1",
    "ios_metal_feature_set_checked=1",
    "ios_package_smoke_ready=1",
    "ios_metal_command_queue_ready=1",
    "ios_metal_pipeline_ready=1",
    "ios_metal_command_buffer_ready=1",
    "ios_metal_readback_ready=1",
    "environment_platform_ios_metal_ready=1",
    "environment_platform_requires_ios_metal_host_evidence=0",
    "environment_all_platform_unconditional_ready=0",
    'if: ${{ failure() && !cancelled() }}',
    $uploadArtifactActionRef,
    "name: ios-simulator-build",
    "retention-days: 14",
    "include-hidden-files: false",
    "out/build/ios-Simulator-sample_headless-Debug/**/*.app",
    "out/build/ios-Simulator-sample_headless-Debug/**/*.log",
    "if-no-files-found: warn"
) ".github/workflows/ios-validate.yml simulator-smoke job"

Assert-CheckoutRetryContract -Text $changesJob -Label ".github/workflows/validate.yml changes checkout retry" -RequiresFetchDepthZero
Assert-CheckoutRetryContract -Text $agentStaticJob -Label ".github/workflows/validate.yml agent-static checkout retry" -RequiresFetchDepthZero
Assert-CheckoutRetryContract -Text $windowsJob -Label ".github/workflows/validate.yml windows checkout retry"
Assert-CheckoutRetryContract -Text $windowsCpuProfilingJob -Label ".github/workflows/validate.yml windows-cpu-profiling-host checkout retry"
Assert-CheckoutRetryContract -Text $windowsAssetImportersJob -Label ".github/workflows/validate.yml windows-asset-importers checkout retry"
Assert-CheckoutRetryContract -Text $windowsDesktopEditorJob -Label ".github/workflows/validate.yml windows-desktop-editor checkout retry"
Assert-CheckoutRetryContract -Text $windowsNetworkEnetJob -Label ".github/workflows/validate.yml windows-network-enet checkout retry"
Assert-CheckoutRetryContract -Text $windowsCpp23Job -Label ".github/workflows/validate.yml windows-cpp23 checkout retry"
Assert-CheckoutRetryContract -Text $linuxJob -Label ".github/workflows/validate.yml linux checkout retry"
Assert-CheckoutRetryContract -Text $linuxVulkanJob -Label ".github/workflows/validate.yml linux-vulkan checkout retry"
Assert-CheckoutRetryContract -Text $linuxCoverageJob -Label ".github/workflows/validate.yml linux-coverage checkout retry"
Assert-CheckoutRetryContract -Text $sanitizerJob -Label ".github/workflows/validate.yml linux-sanitizers checkout retry"
Assert-CheckoutRetryContract -Text $staticAnalysisJob -Label ".github/workflows/validate.yml static-analysis checkout retry"
Assert-CheckoutRetryContract -Text $macosJob -Label ".github/workflows/validate.yml macos checkout retry"
Assert-CheckoutRetryContract -Text $iosMetalJob -Label ".github/workflows/validate.yml ios-metal checkout retry"
Assert-CheckoutRetryContract -Text $iosJob -Label ".github/workflows/ios-validate.yml simulator-smoke checkout retry"

Assert-CcacheStatsGuard -Text $linuxJob -Label ".github/workflows/validate.yml linux ccache stats guard"
Assert-CcacheStatsGuard -Text $linuxCoverageJob -Label ".github/workflows/validate.yml linux-coverage ccache stats guard"
Assert-CcacheStatsGuard -Text $sanitizerJob -Label ".github/workflows/validate.yml linux-sanitizers ccache stats guard"
Assert-CcacheStatsGuard -Text $staticAnalysisJob -Label ".github/workflows/validate.yml static-analysis ccache stats guard"
Assert-CcacheStatsGuard -Text $macosJob -Label ".github/workflows/validate.yml macos ccache stats guard"

Assert-MatchesText $validateWorkflow "^  windows:\s*$" ".github/workflows/validate.yml windows job id"
Assert-MatchesText $validateWorkflow "^  windows-cpu-profiling-host:\s*$" ".github/workflows/validate.yml windows-cpu-profiling-host job id"
Assert-MatchesText $validateWorkflow "^  windows-asset-importers:\s*$" ".github/workflows/validate.yml windows-asset-importers job id"
Assert-MatchesText $validateWorkflow "^  windows-desktop-editor:\s*$" ".github/workflows/validate.yml windows-desktop-editor job id"
Assert-MatchesText $validateWorkflow "^  windows-network-enet:\s*$" ".github/workflows/validate.yml windows-network-enet job id"
Assert-MatchesText $validateWorkflow "^  windows-cpp23:\s*$" ".github/workflows/validate.yml windows-cpp23 job id"
Assert-MatchesText $validateWorkflow "^  agent-static:\s*$" ".github/workflows/validate.yml agent-static job id"
Assert-MatchesText $validateWorkflow "^  linux:\s*$" ".github/workflows/validate.yml linux job id"
Assert-MatchesText $validateWorkflow "^  linux-vulkan:\s*$" ".github/workflows/validate.yml linux-vulkan job id"
Assert-MatchesText $validateWorkflow "^  linux-coverage:\s*$" ".github/workflows/validate.yml linux-coverage job id"
Assert-MatchesText $validateWorkflow "^  linux-sanitizers:\s*$" ".github/workflows/validate.yml linux-sanitizers job id"
Assert-DoesNotContainText $validateWorkflow "command -v ninja-build" ".github/workflows/validate.yml command probes"
Assert-DoesNotContainText $validateWorkflow "brew install ninja ccache" ".github/workflows/validate.yml macOS bootstrap"
Assert-MatchesText $validateWorkflow "^  static-analysis:\s*$" ".github/workflows/validate.yml static-analysis job id"
Assert-MatchesText $validateWorkflow "^  macos:\s*$" ".github/workflows/validate.yml macos job id"
Assert-MatchesText $validateWorkflow "^  ios-metal:\s*$" ".github/workflows/validate.yml ios-metal job id"
Assert-MatchesText $validateWorkflow "^  pr-gate:\s*$" ".github/workflows/validate.yml pr-gate job id"
Assert-MatchesText $iosWorkflow "^  simulator-smoke:\s*$" ".github/workflows/ios-validate.yml simulator-smoke job id"

Write-Host "ci-matrix-check: ok"
