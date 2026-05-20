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

function Assert-ValidationTierSelection {
    param(
        [Parameter(Mandatory = $true)][string]$Label,
        [string[]]$ChangedPath = @(),
        [switch]$RunAll,
        [Parameter(Mandatory = $true)][bool]$ExpectedWindows,
    [Parameter(Mandatory = $true)][bool]$ExpectedLinux,
    [Parameter(Mandatory = $true)][bool]$ExpectedLinuxSanitizers,
    [bool]$ExpectedLinuxCoverage = $false,
    [Parameter(Mandatory = $true)][bool]$ExpectedStaticAnalysis,
    [bool]$ExpectedWindowsCpp23 = $false,
    [Parameter(Mandatory = $true)][bool]$ExpectedMacos
    )

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
    $expectations = @{
        windows = $ExpectedWindows
        linux = $ExpectedLinux
        linux_sanitizers = $ExpectedLinuxSanitizers
        linux_coverage = $ExpectedLinuxCoverage
        static_analysis = $ExpectedStaticAnalysis
        macos = $ExpectedMacos
        windows_cpp23 = $ExpectedWindowsCpp23
    }

    foreach ($expectation in $expectations.GetEnumerator()) {
        $actual = [bool]$selection.($expectation.Key)
        if ($actual -ne $expectation.Value) {
            Write-Error "ci-matrix-check: PR validation tier classifier $Label expected $($expectation.Key)=$($expectation.Value) but got $actual"
        }
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
    "Resolve-Cpp23EvaluationJobCount",
    "`$effectiveJobs = Resolve-Cpp23EvaluationJobCount -Jobs `$Jobs",
    "cpp23-verification: cmake/ctest parallel jobs=`$effectiveJobs",
    "`$runDebug = `$Debug.IsPresent -or (-not `$Release.IsPresent -and -not `$Gui.IsPresent)",
    "if (`$runDebug) {",
    "Invoke-CheckedCommand `$tools.CMake --build --preset cpp23-eval --parallel `$effectiveJobs",
    "Invoke-CheckedCommand `$tools.CTest --preset cpp23-eval --output-on-failure --timeout 300 --parallel `$effectiveJobs",
    "release-package-artifacts.ps1",
    "Assert-ReleasePackageArtifacts",
    "cpp23-release-preset-eval",
    "Invoke-CheckedCommand `$tools.CMake --build --preset cpp23-release-eval --parallel `$effectiveJobs",
    "Invoke-CheckedCommand `$tools.CTest --preset cpp23-release-eval --output-on-failure --timeout 300 --parallel `$effectiveJobs",
    "Invoke-CheckedCommand `$tools.CMake --build --preset cpp23-desktop-gui-eval --parallel `$effectiveJobs",
    "Invoke-CheckedCommand `$tools.CTest --preset cpp23-desktop-gui-eval --output-on-failure --timeout 300 --parallel `$effectiveJobs"
) "tools/evaluate-cpp23.ps1 release artifact validation"
$cpp23CpackCallIndex = $cpp23EvaluationScript.IndexOf('Invoke-CheckedCommand $tools.CPack --preset cpp23-release-eval', [System.StringComparison]::Ordinal)
$cpp23ArtifactAssertIndex = $cpp23EvaluationScript.IndexOf('Assert-ReleasePackageArtifacts -BuildDir $releaseBuildDir', [System.StringComparison]::Ordinal)
if ($cpp23CpackCallIndex -lt 0 -or $cpp23ArtifactAssertIndex -lt 0 -or $cpp23ArtifactAssertIndex -lt $cpp23CpackCallIndex) {
    Write-Error "ci-matrix-check: tools/evaluate-cpp23.ps1 must run Assert-ReleasePackageArtifacts after CPack for the C++23 release lane"
}

Assert-ValidationTierSelection `
    -Label "docs-only PR" `
    -ChangedPath @("docs/testing.md", "AGENTS.md", ".agents/skills/gameengine-agent-integration/SKILL.md") `
    -ExpectedWindows $false `
    -ExpectedLinux $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedStaticAnalysis $false `
    -ExpectedWindowsCpp23 $false `
    -ExpectedMacos $false

Assert-ValidationTierSelection `
    -Label "static policy PR" `
    -ChangedPath @(".clang-tidy") `
    -ExpectedWindows $false `
    -ExpectedLinux $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedStaticAnalysis $true `
    -ExpectedWindowsCpp23 $false `
    -ExpectedMacos $false

Assert-ValidationTierSelection `
    -Label "runtime PR" `
    -ChangedPath @("engine/core/src/example.cpp") `
    -ExpectedWindows $true `
    -ExpectedLinux $true `
    -ExpectedLinuxSanitizers $true `
    -ExpectedLinuxCoverage $false `
    -ExpectedStaticAnalysis $true `
    -ExpectedWindowsCpp23 $false `
    -ExpectedMacos $true

Assert-ValidationTierSelection `
    -Label "workflow PR" `
    -ChangedPath @(".github/workflows/validate.yml") `
    -ExpectedWindows $true `
    -ExpectedLinux $true `
    -ExpectedLinuxSanitizers $true `
    -ExpectedLinuxCoverage $false `
    -ExpectedStaticAnalysis $true `
    -ExpectedWindowsCpp23 $true `
    -ExpectedMacos $true

Assert-ValidationTierSelection `
    -Label "classifier policy PR" `
    -ChangedPath @("tools/classify-pr-validation-tier.ps1") `
    -ExpectedWindows $false `
    -ExpectedLinux $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedStaticAnalysis $false `
    -ExpectedWindowsCpp23 $true `
    -ExpectedMacos $false

Assert-ValidationTierSelection `
    -Label "ci matrix policy PR" `
    -ChangedPath @("tools/check-ci-matrix.ps1") `
    -ExpectedWindows $false `
    -ExpectedLinux $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedStaticAnalysis $false `
    -ExpectedWindowsCpp23 $true `
    -ExpectedMacos $false

Assert-ValidationTierSelection `
    -Label "cpp23 policy infra PR" `
    -ChangedPath @("tools/check-cpp-standard-policy.ps1") `
    -ExpectedWindows $false `
    -ExpectedLinux $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedStaticAnalysis $false `
    -ExpectedWindowsCpp23 $true `
    -ExpectedMacos $false

Assert-ValidationTierSelection `
    -Label "coverage infra PR" `
    -ChangedPath @("tools/check-coverage.ps1") `
    -ExpectedWindows $false `
    -ExpectedLinux $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $true `
    -ExpectedStaticAnalysis $false `
    -ExpectedWindowsCpp23 $false `
    -ExpectedMacos $false

Assert-ValidationTierSelection `
    -Label "cpp23 evaluate infra PR" `
    -ChangedPath @("tools/evaluate-cpp23.ps1") `
    -ExpectedWindows $false `
    -ExpectedLinux $false `
    -ExpectedLinuxSanitizers $false `
    -ExpectedLinuxCoverage $false `
    -ExpectedStaticAnalysis $false `
    -ExpectedWindowsCpp23 $true `
    -ExpectedMacos $false

Assert-ValidationTierSelection `
    -Label "non-PR run" `
    -RunAll `
    -ExpectedWindows $true `
    -ExpectedLinux $true `
    -ExpectedLinuxSanitizers $true `
    -ExpectedLinuxCoverage $true `
    -ExpectedStaticAnalysis $true `
    -ExpectedWindowsCpp23 $true `
    -ExpectedMacos $true

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
    "workflow_dispatch:",
    "permissions:",
    "contents: read",
    "concurrency:",
    'group: ${{ github.workflow }}-${{ github.event.pull_request.number || github.ref }}',
    "cancel-in-progress: `${{ github.event_name == 'pull_request' }}"
) ".github/workflows/validate.yml triggers"

$changesJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "changes" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $changesJob @(
    "name: Select PR validation tier",
    "runs-on: ubuntu-latest",
    "timeout-minutes: 10",
    "windows: `${{ steps.classify.outputs.windows }}",
    "linux: `${{ steps.classify.outputs.linux }}",
    "linux_sanitizers: `${{ steps.classify.outputs.linux_sanitizers }}",
    "linux_coverage: `${{ steps.classify.outputs.linux_coverage }}",
    "static_analysis: `${{ steps.classify.outputs.static_analysis }}",
    "windows_cpp23: `${{ steps.classify.outputs.windows_cpp23 }}",
    "fetch-depth: 0",
    "persist-credentials: false",
    "name: Classify touched surfaces",
    'github.event_name',
    'git diff --name-only $base $head',
    "tools/classify-pr-validation-tier.ps1 -RunAll -GitHubOutputPath `$env:GITHUB_OUTPUT",
    "tools/classify-pr-validation-tier.ps1 -ChangedPath `$files -GitHubOutputPath `$env:GITHUB_OUTPUT"
) ".github/workflows/validate.yml changes job"

$windowsJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "windows" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $windowsJob @(
    "name: Windows MSVC",
    "needs: changes",
    "if: needs.changes.outputs.windows == 'true'",
    "runs-on: windows-2025-vs2026",
    "timeout-minutes: 120",
    $checkoutActionRef,
    "persist-credentials: false",
    "Restore vcpkg tool checkout cache",
    "path: external/vcpkg",
    "steps.restore-vcpkg-tool.outputs.cache-hit",
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
    "path: out/vcpkg",
    "path: vcpkg_installed",
    "Restore Windows dev build cache",
    $cacheRestoreActionRef,
    "path: out/build/dev",
    'key: ${{ runner.os }}-dev-build-${{ steps.windows-toolchain-cache.outputs.identity }}-${{ hashFiles(''CMakePresets.json'', ''vcpkg.json'', ''**/CMakeLists.txt'') }}-${{ github.sha }}',
    '${{ runner.os }}-dev-build-${{ steps.windows-toolchain-cache.outputs.identity }}-${{ hashFiles(''CMakePresets.json'', ''vcpkg.json'', ''**/CMakeLists.txt'') }}-',
    '${{ runner.os }}-dev-build-${{ steps.windows-toolchain-cache.outputs.identity }}-',
    "restore-dev-build",
    "run: ./tools/bootstrap-deps.ps1",
    "run: ./tools/validate.ps1 -SkipStaticChecks -SkipTidySmoke",
    "Save Windows dev build cache",
    "continue-on-error: true",
    $cacheSaveActionRef,
    "steps.restore-dev-build.outputs.cache-hit != 'true'",
    "key: `${{ steps.restore-dev-build.outputs.cache-primary-key }}",
    'if: ${{ failure() && !cancelled() }}',
    $uploadArtifactActionRef,
    "name: windows-test-logs",
    "retention-days: 14",
    "include-hidden-files: false",
    "out/build/dev/Testing/**/*.log",
    "if-no-files-found: warn"
) ".github/workflows/validate.yml windows job"

$windowsCpp23Job = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "windows-cpp23" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $windowsCpp23Job @(
    "name: Windows C++23 Release Evaluation",
    "needs: changes",
    "if: needs.changes.outputs.windows_cpp23 == 'true'",
    "runs-on: windows-2025-vs2026",
    "timeout-minutes: 150",
    $checkoutActionRef,
    "persist-credentials: false",
    "Restore vcpkg tool checkout cache",
    "path: external/vcpkg",
    "steps.restore-vcpkg-tool.outputs.cache-hit",
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
    "path: out/vcpkg",
    "path: vcpkg_installed",
    "Restore Windows C++23 build cache",
    $cacheRestoreActionRef,
    "out/build/cpp23-release-preset-eval",
    "out/build/installed-consumer-cpp23-release-eval",
    "out/install/cpp23-release-eval",
    'key: ${{ runner.os }}-cpp23-build-${{ steps.windows-toolchain-cache.outputs.identity }}-${{ hashFiles(''CMakePresets.json'', ''vcpkg.json'', ''**/CMakeLists.txt'') }}-${{ github.sha }}',
    '${{ runner.os }}-cpp23-build-${{ steps.windows-toolchain-cache.outputs.identity }}-${{ hashFiles(''CMakePresets.json'', ''vcpkg.json'', ''**/CMakeLists.txt'') }}-',
    '${{ runner.os }}-cpp23-build-${{ steps.windows-toolchain-cache.outputs.identity }}-',
    "restore-cpp23-build",
    "run: ./tools/bootstrap-deps.ps1",
    "run: ./tools/evaluate-cpp23.ps1 -Release",
    "Save Windows C++23 build cache",
    "continue-on-error: true",
    $cacheSaveActionRef,
    "steps.restore-cpp23-build.outputs.cache-hit != 'true'",
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
    "include-hidden-files: false",
    "out/build/cpp23-release-preset-eval/*.zip",
    "out/build/cpp23-release-preset-eval/*.zip.sha256",
    "if-no-files-found: error",
    'if: ${{ success() && !cancelled() }}'
) ".github/workflows/validate.yml windows-cpp23 job"

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
    "run: ./tools/validate.ps1 -StaticOnly -StaticJobs 1"
) ".github/workflows/validate.yml agent-static job"

$linuxJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "linux" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $linuxJob @(
    "name: Linux CMake",
    "needs: changes",
    "if: needs.changes.outputs.linux == 'true'",
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
    "if: needs.changes.outputs.macos == 'true'",
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

$staticAnalysisJob = Get-WorkflowJobText -WorkflowText $validateWorkflow -JobName "static-analysis" -Label ".github/workflows/validate.yml"
Assert-ContainsAll $staticAnalysisJob @(
    "name: Full Repository Static Analysis",
    "needs: changes",
    "if: needs.changes.outputs.static_analysis == 'true'",
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
    "name: static-analysis-tidy-logs",
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
    "- linux",
    "- windows-cpp23",
    "- linux-coverage",
    "- linux-sanitizers",
    "- static-analysis",
    "- macos",
    'if: ${{ always() && !cancelled() }}',
    "timeout-minutes: 10",
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
    '"tools/build-mobile-apple.ps1"',
    '"tools/check-mobile-packaging.ps1"',
    '"tools/smoke-ios-package.ps1"',
    "permissions:",
    "contents: read",
    "concurrency:",
    'group: ${{ github.workflow }}-${{ github.event.pull_request.number || github.ref }}',
    "cancel-in-progress: `${{ github.event_name == 'pull_request' }}"
) ".github/workflows/ios-validate.yml triggers and path filters"

$iosJob = Get-WorkflowJobText -WorkflowText $iosWorkflow -JobName "simulator-smoke" -Label ".github/workflows/ios-validate.yml"
Assert-ContainsAll $iosJob @(
    "name: iOS Simulator smoke",
    "runs-on: macos-26",
    "timeout-minutes: 30",
    $checkoutActionRef,
    "persist-credentials: false",
    "xcodebuild -version",
    "xcode-select -p",
    "xcrun --sdk iphonesimulator --show-sdk-path",
    "xcrun simctl list runtimes",
    "./tools/check-mobile-packaging.ps1 -RequireApple",
    "./tools/smoke-ios-package.ps1 -Game sample_headless -Configuration Debug -BootTimeoutSeconds 420 -BootAttempts 2",
    'if: ${{ failure() && !cancelled() }}',
    $uploadArtifactActionRef,
    "name: ios-simulator-build",
    "retention-days: 14",
    "include-hidden-files: false",
    "out/build/ios-Simulator-sample_headless-Debug/**/*.app",
    "out/build/ios-Simulator-sample_headless-Debug/**/*.log",
    "if-no-files-found: warn"
) ".github/workflows/ios-validate.yml simulator-smoke job"

Assert-MatchesText $validateWorkflow "^  windows:\s*$" ".github/workflows/validate.yml windows job id"
Assert-MatchesText $validateWorkflow "^  windows-cpp23:\s*$" ".github/workflows/validate.yml windows-cpp23 job id"
Assert-MatchesText $validateWorkflow "^  agent-static:\s*$" ".github/workflows/validate.yml agent-static job id"
Assert-MatchesText $validateWorkflow "^  linux:\s*$" ".github/workflows/validate.yml linux job id"
Assert-MatchesText $validateWorkflow "^  linux-coverage:\s*$" ".github/workflows/validate.yml linux-coverage job id"
Assert-MatchesText $validateWorkflow "^  linux-sanitizers:\s*$" ".github/workflows/validate.yml linux-sanitizers job id"
Assert-DoesNotContainText $validateWorkflow "command -v ninja-build" ".github/workflows/validate.yml command probes"
Assert-DoesNotContainText $validateWorkflow "brew install ninja ccache" ".github/workflows/validate.yml macOS bootstrap"
Assert-MatchesText $validateWorkflow "^  static-analysis:\s*$" ".github/workflows/validate.yml static-analysis job id"
Assert-MatchesText $validateWorkflow "^  macos:\s*$" ".github/workflows/validate.yml macos job id"
Assert-MatchesText $validateWorkflow "^  pr-gate:\s*$" ".github/workflows/validate.yml pr-gate job id"
Assert-MatchesText $iosWorkflow "^  simulator-smoke:\s*$" ".github/workflows/ios-validate.yml simulator-smoke job id"

Write-Host "ci-matrix-check: ok"
