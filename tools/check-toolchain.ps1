#requires -Version 7.0
#requires -PSEdition Core

param(
    [switch]$RequireDirectCMake
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

function Get-ToolVersion {
    param(
        [Parameter(Mandatory = $true)][string]$ToolName,
        [Parameter(Mandatory = $true)][string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Write-Error "$ToolName path does not exist: $Path"
    }

    $output = & $Path --version 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Error "$ToolName --version failed with exit code $LASTEXITCODE at $Path"
    }

    $text = (@($output) -join "`n")
    if ($text -notmatch "(\d+)\.(\d+)\.(\d+)") {
        Write-Error "$ToolName --version did not contain a semantic version: $text"
    }

    return [version]$Matches[0]
}

function Assert-VersionAtLeast {
    param(
        [Parameter(Mandatory = $true)][string]$ToolName,
        [Parameter(Mandatory = $true)][version]$Actual,
        [Parameter(Mandatory = $true)][version]$Minimum
    )

    if ($Actual -lt $Minimum) {
        Write-Error "$ToolName $Minimum or newer is required, but found $Actual."
    }
}

function Test-VisualStudioDeveloperShell {
    foreach ($name in @("VSCMD_VER", "VSINSTALLDIR", "DevEnvDir", "VCToolsInstallDir")) {
        if (-not [string]::IsNullOrWhiteSpace([Environment]::GetEnvironmentVariable($name, "Process"))) {
            return $true
        }
    }

    return $false
}

function Write-ToolPath {
    param(
        [Parameter(Mandatory = $true)][string]$Label,
        [AllowNull()][string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        Write-Host "toolchain-check: ${Label}=missing"
    } else {
        Write-Host "toolchain-check: ${Label}=$Path"
    }
}

function Assert-NormalizedBuildPresetEnvironment {
    $presetsPath = Join-Path (Get-RepoRoot) "CMakePresets.json"
    $presets = Get-Content -LiteralPath $presetsPath -Raw | ConvertFrom-Json
    $environmentPreset = $presets.buildPresets |
        Where-Object { $_.name -eq "normalized-build-environment" } |
        Select-Object -First 1
    if (-not $environmentPreset) {
        Write-Error "CMakePresets.json buildPresets must define hidden normalized-build-environment preset"
    }
    if ($environmentPreset.hidden -ne $true) {
        Write-Error "normalized-build-environment build preset must be hidden"
    }
    if ($environmentPreset.environment.Path -ne '$penv{PATH}${pathListSep}$penv{Path}') {
        Write-Error "normalized-build-environment must rebuild Path from parent PATH/Path with `${pathListSep}"
    }

    foreach ($preset in @($presets.buildPresets | Where-Object { $_.hidden -ne $true })) {
        $inherits = @()
        if ($preset.PSObject.Properties.Name.Contains("inherits")) {
            if ($preset.inherits -is [System.Array]) {
                $inherits = @($preset.inherits)
            } else {
                $inherits = @([string]$preset.inherits)
            }
        }
        if ($inherits -notcontains "normalized-build-environment") {
            Write-Error "CMakePresets.json build preset '$($preset.name)' must inherit normalized-build-environment"
        }
    }
}

$minimumCmake = [version]"3.30.0"
$tools = Assert-CppBuildTools
Assert-NormalizedBuildPresetEnvironment
Write-Host "toolchain-check: normalized-build-environment=ready"

$cmakeVersion = Get-ToolVersion -ToolName "cmake" -Path $tools.CMake
$ctestVersion = Get-ToolVersion -ToolName "ctest" -Path $tools.CTest
Assert-VersionAtLeast -ToolName "CMake" -Actual $cmakeVersion -Minimum $minimumCmake
Assert-VersionAtLeast -ToolName "CTest" -Actual $ctestVersion -Minimum $minimumCmake

Write-ToolPath "cmake" $tools.CMake
Write-Host "toolchain-check: cmake-version=$cmakeVersion"
Write-ToolPath "ctest" $tools.CTest
Write-Host "toolchain-check: ctest-version=$ctestVersion"

if ($tools.CPack) {
    $cpackVersion = Get-ToolVersion -ToolName "cpack" -Path $tools.CPack
    Assert-VersionAtLeast -ToolName "CPack" -Actual $cpackVersion -Minimum $minimumCmake
    Write-ToolPath "cpack" $tools.CPack
    Write-Host "toolchain-check: cpack-version=$cpackVersion"
} else {
    Write-ToolPath "cpack" $null
    Write-Warning "CPack was not found. Build/test lanes can continue, but package lanes will fail until CPack is installed."
}

Write-ToolPath "clang-format" $tools.ClangFormat
if ($tools.ClangFormat) {
    $clangFormatVersion = Get-ToolVersion -ToolName "clang-format" -Path $tools.ClangFormat
    Write-Host "toolchain-check: clang-format-version=$clangFormatVersion"
} else {
    Write-Warning "clang-format was not found. Format lanes will fail until official LLVM clang-format or Visual Studio Build Tools LLVM tools are installed."
}

$directCmake = Get-Command cmake -ErrorAction SilentlyContinue | Select-Object -First 1
if ($directCmake) {
    Write-Host "toolchain-check: direct-cmake=$($directCmake.Source)"
    Write-Host "toolchain-check: direct-cmake-status=ready"
} else {
    Write-Host "toolchain-check: direct-cmake=missing"
    Write-Host "toolchain-check: direct-cmake-status=unavailable"
}

if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
    Write-ToolPath "visual-studio" $tools.VisualStudio
    Write-ToolPath "msbuild" $tools.MSBuild
    $shellState = if (Test-VisualStudioDeveloperShell) { "developer-shell" } else { "ordinary-shell" }
    Write-Host "toolchain-check: windows-shell=$shellState"

    if (-not $directCmake) {
        $message = "Direct 'cmake --preset ...' commands require CMake on PATH. On Windows, use Visual Studio Developer PowerShell/Command Prompt or install official CMake 3.30+ on PATH. Repository wrappers use the resolved CMake path above."
        if ($RequireDirectCMake) {
            Write-Error $message
        }
        Write-Host "toolchain-check: direct-cmake-note=$message"
    }
}

$directClangFormat = Get-Command clang-format -ErrorAction SilentlyContinue | Select-Object -First 1
if ($directClangFormat) {
    Write-Host "toolchain-check: direct-clang-format=$($directClangFormat.Source)"
    Write-Host "toolchain-check: direct-clang-format-status=ready"
} else {
    Write-Host "toolchain-check: direct-clang-format=missing"
    Write-Host "toolchain-check: direct-clang-format-status=unavailable"
    if ($tools.ClangFormat) {
        $message = "Direct 'clang-format ...' commands require clang-format on PATH. Use the repository format wrappers or add the resolved clang-format directory above to PATH."
        Write-Host "toolchain-check: direct-clang-format-note=$message"
    }
}

Write-Host "toolchain-check: ok"
