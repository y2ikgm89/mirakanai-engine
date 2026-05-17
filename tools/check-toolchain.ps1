#requires -Version 7.0
#requires -PSEdition Core

param(
    [switch]$RequireDirectCMake,
    [switch]$RequireVcpkgToolchain
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

function ConvertTo-ComparablePath {
    param(
        [Parameter(Mandatory = $true)][string]$Path
    )

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    return $fullPath.TrimEnd([char[]]@([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar))
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

function Write-WorktreeReadiness {
    param(
        [switch]$RequireVcpkgToolchain
    )

    $root = Get-RepoRoot
    $gitCommand = Get-Command git -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($gitCommand) {
        $gitDir = (& git -C $root rev-parse --path-format=absolute --git-dir 2>$null | Select-Object -First 1)
        if ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace($gitDir)) {
            $gitCommonDir = (& git -C $root rev-parse --path-format=absolute --git-common-dir 2>$null | Select-Object -First 1)
            if ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace($gitCommonDir)) {
                $isLinkedWorktree = (ConvertTo-ComparablePath -Path $gitDir) -ne (ConvertTo-ComparablePath -Path $gitCommonDir)
                Write-Host "toolchain-check: linked-worktree=$($isLinkedWorktree.ToString().ToLowerInvariant())"
            }
        }
    } else {
        Write-Warning "Git was not found on PATH; linked worktree status is unavailable."
    }

    $vcpkgRoot = Join-Path $root "external/vcpkg"
    $vcpkgToolchain = Join-Path $vcpkgRoot "scripts/buildsystems/vcpkg.cmake"
    if (Test-Path -LiteralPath $vcpkgToolchain -PathType Leaf) {
        $vcpkgItem = Get-Item -LiteralPath $vcpkgRoot -Force
        $linkType = if ($vcpkgItem.PSObject.Properties.Name.Contains("LinkType") -and -not [string]::IsNullOrWhiteSpace([string]$vcpkgItem.LinkType)) {
            [string]$vcpkgItem.LinkType
        } else {
            "Directory"
        }
        Write-Host "toolchain-check: external-vcpkg=ready"
        Write-Host "toolchain-check: external-vcpkg-kind=$linkType"
    } else {
        Write-Host "toolchain-check: external-vcpkg=missing"
        if ($RequireVcpkgToolchain) {
            Write-Error "Missing external/vcpkg/scripts/buildsystems/vcpkg.cmake. Run pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1 in linked worktrees, or clone and bootstrap the official Microsoft vcpkg checkout at external/vcpkg."
        }
    }

    $installedRoot = Join-Path $root "vcpkg_installed"
    if (Test-Path -LiteralPath $installedRoot -PathType Container) {
        Write-Host "toolchain-check: vcpkg-installed=ready"
    } else {
        Write-Host "toolchain-check: vcpkg-installed=missing"
    }
}

function Write-ProcessPathCasingStatus {
    param(
        [switch]$RequireDirectCMake
    )

    $result = [pscustomobject]@{
        DirectCMakeEnvironmentReady = $true
    }

    $pathKeys = @(
        [System.Environment]::GetEnvironmentVariables().Keys |
            Where-Object { [string]$_ -ieq "PATH" } |
            ForEach-Object { [string]$_ }
    )

    $uniquePathKeys = @($pathKeys | Sort-Object -Unique)
    if ($uniquePathKeys.Count -gt 1) {
        Write-Host "toolchain-check: process-path-casing=duplicate"
        Write-Host "toolchain-check: process-path-keys=$($uniquePathKeys -join ',')"
        Write-Host "toolchain-check: direct-cmake-visual-studio-path-casing=covered-by-preset"
        Write-Host "toolchain-check: direct-cmake-visual-studio-path-note=CMakePresets.json unsets Path and inherits PATH for visible configure/build presets; repository wrappers additionally normalize child process environments before launching CMake or CTest."
    } elseif ($uniquePathKeys.Count -eq 1) {
        Write-Host "toolchain-check: process-path-casing=$($uniquePathKeys[0])"
        if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows) -and
            $uniquePathKeys[0] -cne "Path") {
            Write-Host "toolchain-check: direct-cmake-visual-studio-path-casing=covered-by-preset"
            Write-Host "toolchain-check: direct-cmake-visual-studio-path-note=CMakePresets.json normalizes visible configure/build preset environments by inheriting PATH and unsetting Path; repository wrappers additionally normalize child process environments before launching CMake or CTest."
        } else {
            Write-Host "toolchain-check: direct-cmake-visual-studio-path-casing=ready"
        }
    } else {
        Write-Host "toolchain-check: process-path-casing=missing"
        if ($RequireDirectCMake) {
            Write-Error "Direct 'cmake --preset ...' requires PATH/Path in the process environment."
        }
        $result.DirectCMakeEnvironmentReady = $false
    }

    return $result
}

function Get-PresetInheritance {
    param(
        [Parameter(Mandatory = $true)]$Preset
    )

    if (-not (Test-JsonProperty -Object $Preset -Property "inherits")) {
        return @()
    }

    $inherits = Get-JsonPropertyValue -Object $Preset -Property "inherits"
    if ($inherits -is [System.Array]) {
        return @($inherits | ForEach-Object { [string]$_ })
    }

    return @([string]$inherits)
}

function Assert-HiddenNormalizedEnvironmentPreset {
    param(
        [Parameter(Mandatory = $true)]$Preset,
        [Parameter(Mandatory = $true)][string]$PresetKind,
        [Parameter(Mandatory = $true)][string]$PresetName
    )

    if (-not $Preset) {
        Write-Error "CMakePresets.json $PresetKind must define hidden $PresetName preset"
    }
    if ($Preset.hidden -ne $true) {
        Write-Error "$PresetName $PresetKind preset must be hidden"
    }
    $environment = Get-JsonPropertyValue -Object $Preset -Property "environment"
    if ((Get-JsonPropertyValue -Object $environment -Property "PATH") -ne '$penv{PATH}') {
        Write-Error "$PresetName must inherit parent PATH without duplicating PATH/Path expansions"
    }
    if (-not (Test-JsonProperty -Object $environment -Property "Path") -or $null -ne (Get-JsonPropertyValue -Object $environment -Property "Path")) {
        Write-Error "$PresetName must set mixed-case Path to null so CMake removes duplicate parent Path on Windows"
    }
}

function Assert-VisiblePresetsInheritEnvironment {
    param(
        [Parameter(Mandatory = $true)][object[]]$Presets,
        [Parameter(Mandatory = $true)][string]$PresetKind,
        [Parameter(Mandatory = $true)][string]$PresetName
    )

    foreach ($preset in @($Presets | Where-Object { $_.hidden -ne $true })) {
        $inherits = @(Get-PresetInheritance -Preset $preset)
        if ($inherits -notcontains $PresetName) {
            Write-Error "CMakePresets.json $PresetKind preset '$($preset.name)' must inherit $PresetName"
        }
    }
}

function Assert-NormalizedCMakePresetEnvironment {
    $presets = Read-CMakePresets

    $configureEnvironmentPreset = $presets.configurePresets |
        Where-Object { $_.name -eq "normalized-configure-environment" } |
        Select-Object -First 1
    Assert-HiddenNormalizedEnvironmentPreset `
        -Preset $configureEnvironmentPreset `
        -PresetKind "configurePresets" `
        -PresetName "normalized-configure-environment"
    Assert-VisiblePresetsInheritEnvironment `
        -Presets @($presets.configurePresets) `
        -PresetKind "configurePresets" `
        -PresetName "normalized-configure-environment"

    $buildEnvironmentPreset = $presets.buildPresets |
        Where-Object { $_.name -eq "normalized-build-environment" } |
        Select-Object -First 1
    Assert-HiddenNormalizedEnvironmentPreset `
        -Preset $buildEnvironmentPreset `
        -PresetKind "buildPresets" `
        -PresetName "normalized-build-environment"
    Assert-VisiblePresetsInheritEnvironment `
        -Presets @($presets.buildPresets) `
        -PresetKind "buildPresets" `
        -PresetName "normalized-build-environment"
}

$minimumCmake = [version]"3.30.0"
$tools = Assert-CppBuildTools
Assert-NormalizedCMakePresetEnvironment
Write-Host "toolchain-check: normalized-configure-environment=ready"
Write-Host "toolchain-check: normalized-build-environment=ready"
Write-WorktreeReadiness -RequireVcpkgToolchain:$RequireVcpkgToolchain
$pathCasingStatus = Write-ProcessPathCasingStatus -RequireDirectCMake:$RequireDirectCMake

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
        if ($pathCasingStatus.DirectCMakeEnvironmentReady) {
            Write-Host "toolchain-check: direct-cmake-status=ready"
        } else {
            Write-Host "toolchain-check: direct-cmake-status=host-path-missing"
        }
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
