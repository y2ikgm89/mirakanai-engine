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
        $Path -match "^\.github/workflows/" -or
        $Path -eq "tools/classify-pr-validation-tier.ps1"
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
        $Path -match "^tools/(validate|build|test|bootstrap-deps|check-toolchain|check-tidy|check-format|check-coverage|check-dependency-policy|check-cpp-standard-policy|evaluate-cpp23|package|package-desktop-runtime|check-public-api-boundaries|check-shader-toolchain|run-validation-recipe|common)\.ps1$"
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
            static_analysis = $true
            macos = $true
        }
    }

    $ciOrWorkflow = $false
    $runtimeOrBuild = $false
    $staticPolicy = $false

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
        if (Test-StaticPolicyPath -Path $path) {
            $staticPolicy = $true
        }
    }

    $heavyBuildLane = $ciOrWorkflow -or $runtimeOrBuild

    return [pscustomobject][ordered]@{
        windows = $heavyBuildLane
        linux = $heavyBuildLane
        linux_sanitizers = $heavyBuildLane
        static_analysis = ($heavyBuildLane -or $staticPolicy)
        macos = $heavyBuildLane
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
        "static_analysis=$($Selection.static_analysis.ToString().ToLowerInvariant())",
        "macos=$($Selection.macos.ToString().ToLowerInvariant())"
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
