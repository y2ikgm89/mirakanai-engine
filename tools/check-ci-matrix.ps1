#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$repoRoot = Get-RepoRoot

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

$validateScript = Read-RequiredText "tools/validate.ps1"
Assert-ContainsText $validateScript "check-ci-matrix.ps1" "tools/validate.ps1 default validation hook"

$cpp23EvaluationScript = Read-RequiredText "tools/evaluate-cpp23.ps1"
Assert-ContainsAll $cpp23EvaluationScript @(
    "release-package-artifacts.ps1",
    "Assert-ReleasePackageArtifacts",
    "cpp23-release-preset-eval"
) "tools/evaluate-cpp23.ps1 release artifact validation"
$cpp23CpackCallIndex = $cpp23EvaluationScript.IndexOf('Invoke-CheckedCommand $tools.CPack --preset cpp23-release-eval', [System.StringComparison]::Ordinal)
$cpp23ArtifactAssertIndex = $cpp23EvaluationScript.IndexOf('Assert-ReleasePackageArtifacts -BuildDir $releaseBuildDir', [System.StringComparison]::Ordinal)
if ($cpp23CpackCallIndex -lt 0 -or $cpp23ArtifactAssertIndex -lt 0 -or $cpp23ArtifactAssertIndex -lt $cpp23CpackCallIndex) {
    Write-Error "ci-matrix-check: tools/evaluate-cpp23.ps1 must run Assert-ReleasePackageArtifacts after CPack for the C++23 release lane"
}

$validateWorkflow = Read-RequiredText ".github/workflows/validate.yml"
Assert-ContainsAll $validateWorkflow @(
    "name: Validate",
    "push:",
    "branches:",
    "- main",
    "- master",
    "pull_request:"
) ".github/workflows/validate.yml triggers"

$windowsJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "windows" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $windowsJob @(
    "name: Windows MSVC",
    "runs-on: windows-latest",
    "actions/checkout@v4",
    "git clone --filter=blob:none https://github.com/microsoft/vcpkg.git external/vcpkg",
    "git -C external/vcpkg checkout `$manifest.'builtin-baseline'",
    "run: ./tools/bootstrap-deps.ps1",
    "run: ./tools/validate.ps1",
    "run: ./tools/evaluate-cpp23.ps1 -Release",
    "actions/upload-artifact@v4",
    "name: windows-test-logs",
    "out/build/dev/Testing/**/*.log",
    "out/build/cpp23-eval/Testing/**/*.log",
    "out/build/cpp23-release-preset-eval/Testing/**/*.log",
    "name: windows-packages",
    "out/build/cpp23-release-preset-eval/*.zip",
    "out/build/cpp23-release-preset-eval/*.zip.sha256",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml windows job"

$linuxJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "linux" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $linuxJob @(
    "name: Linux CMake",
    "runs-on: ubuntu-latest",
    "sudo apt-get update && sudo apt-get install -y clang g++ lcov ninja-build",
    "cmake --preset ci-linux-clang",
    "cmake --build --preset ci-linux-clang",
    "ctest --preset ci-linux-clang --output-on-failure",
    "./tools/check-coverage.ps1 -Strict",
    "name: linux-test-logs",
    "out/build/ci-linux-clang/Testing/**/*.log",
    "out/build/coverage/Testing/**/*.log",
    "name: linux-coverage",
    "out/build/coverage/coverage*.info",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml linux job"

$sanitizerJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "linux-sanitizers" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $sanitizerJob @(
    "name: Linux Clang ASan/UBSan",
    "runs-on: ubuntu-latest",
    "sudo apt-get update && sudo apt-get install -y clang ninja-build",
    "cmake --preset clang-asan-ubsan",
    "cmake --build --preset clang-asan-ubsan",
    "UBSAN_OPTIONS: print_stacktrace=1",
    "ctest --preset clang-asan-ubsan --output-on-failure",
    "name: linux-sanitizer-test-logs",
    "out/build/clang-asan-ubsan/Testing/**/*.log",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml linux-sanitizers job"

$macosJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "macos" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $macosJob @(
    "name: macOS Metal CMake",
    "runs-on: macos-latest",
    "brew install ninja",
    "cmake --preset ci-macos-appleclang",
    "cmake --build --preset ci-macos-appleclang",
    "ctest --preset ci-macos-appleclang --output-on-failure",
    "name: macos-test-logs",
    "out/build/ci-macos-appleclang/Testing/**/*.log",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml macos job"

$staticAnalysisJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "static-analysis" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $staticAnalysisJob @(
    "name: Full Repository Static Analysis",
    "runs-on: ubuntu-latest",
    "actions/checkout@v4",
    "sudo apt-get update && sudo apt-get install -y clang clang-tidy ninja-build",
    "./tools/check-tidy.ps1 -Strict -Preset ci-linux-clang",
    "actions/upload-artifact@v4",
    "name: static-analysis-tidy-logs",
    "out/build/ci-linux-clang/compile_commands.json",
    "out/build/ci-linux-clang/.cmake/api/v1/reply/*.json",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml static-analysis job"

$iosWorkflow = Read-RequiredText ".github/workflows/ios-validate.yml"
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
    '"tools/build-mobile-apple.ps1"',
    '"tools/check-mobile-packaging.ps1"',
    '"tools/smoke-ios-package.ps1"'
) ".github/workflows/ios-validate.yml triggers and path filters"

$iosJob = Get-WorkflowJobText -WorkflowText $iosWorkflow -JobName "simulator-smoke" -Label ".github/workflows/ios-validate.yml"
Assert-ContainsAll $iosJob @(
    "name: iOS Simulator smoke",
    "runs-on: macos-26",
    "timeout-minutes: 30",
    "xcodebuild -version",
    "xcode-select -p",
    "xcrun --sdk iphonesimulator --show-sdk-path",
    "xcrun simctl list runtimes",
    "./tools/check-mobile-packaging.ps1 -RequireApple",
    "./tools/smoke-ios-package.ps1 -Game sample_headless -Configuration Debug",
    "actions/upload-artifact@v4",
    "name: ios-simulator-build",
    "out/build/ios-Simulator-sample_headless-Debug/**/*.app",
    "out/build/ios-Simulator-sample_headless-Debug/**/*.log",
    "if-no-files-found: warn"
) ".github/workflows/ios-validate.yml simulator-smoke job"

Assert-MatchesText $validateWorkflow "^  windows:\s*$" ".github/workflows/validate.yml windows job id"
Assert-MatchesText $validateWorkflow "^  linux:\s*$" ".github/workflows/validate.yml linux job id"
Assert-MatchesText $validateWorkflow "^  linux-sanitizers:\s*$" ".github/workflows/validate.yml linux-sanitizers job id"
Assert-MatchesText $validateWorkflow "^  static-analysis:\s*$" ".github/workflows/validate.yml static-analysis job id"
Assert-MatchesText $validateWorkflow "^  macos:\s*$" ".github/workflows/validate.yml macos job id"
Assert-MatchesText $iosWorkflow "^  simulator-smoke:\s*$" ".github/workflows/ios-validate.yml simulator-smoke job id"

Write-Host "ci-matrix-check: ok"
