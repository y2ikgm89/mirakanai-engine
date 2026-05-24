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

function Test-WindowsOnlyValidationPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    return (
        $Path -eq "tools/validate-network-enet.ps1"
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
            windows = $true
            linux = $true
            linux_sanitizers = $true
            linux_coverage = $true
            static_analysis = $true
            macos = $true
            windows_cpp23 = $true
        }
    }

    $ciOrWorkflow = $false
    $runtimeOrBuild = $false
    $staticPolicy = $false
    $sourceCode = $false
    $sanitizerRelevant = $false
    $coverageRelevant = $false
    $cpp23Relevant = $false
    $windowsOnlyValidation = $false

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
        if (Test-WindowsOnlyValidationPath -Path $path) {
            $windowsOnlyValidation = $true
        }
        if (Test-StaticPolicyPath -Path $path) {
            $staticPolicy = $true
        }
    }

    $heavyBuildLane = $ciOrWorkflow -or $runtimeOrBuild

    return [pscustomobject][ordered]@{
        windows = ($heavyBuildLane -or $windowsOnlyValidation)
        linux = $heavyBuildLane
        linux_sanitizers = ($ciOrWorkflow -or $sanitizerRelevant)
        linux_coverage = $coverageRelevant
        static_analysis = ($heavyBuildLane -or $staticPolicy -or $sourceCode)
        macos = $heavyBuildLane
        windows_cpp23 = ($ciOrWorkflow -or $cpp23Relevant)
    }
}

function Write-GitHubActionsOutput {
    param(
        [Parameter(Mandatory = $true)][pscustomobject]$Selection,
        [Parameter(Mandatory = $true)][string]$OutputPath
    )

    $lines = @(
        "windows=$($Selection.windows.ToString().ToLowerInvariant())",
        "linux=$($Selection.linux.ToString().ToLowerInvariant())",
        "linux_sanitizers=$($Selection.linux_sanitizers.ToString().ToLowerInvariant())",
        "linux_coverage=$($Selection.linux_coverage.ToString().ToLowerInvariant())",
        "static_analysis=$($Selection.static_analysis.ToString().ToLowerInvariant())",
        "macos=$($Selection.macos.ToString().ToLowerInvariant())",
        "windows_cpp23=$($Selection.windows_cpp23.ToString().ToLowerInvariant())"
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
