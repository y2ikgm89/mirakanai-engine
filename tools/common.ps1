#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"
# Set-StrictMode is intentionally not enabled for the whole tools tree: many validation scripts
# walk dynamic JSON graphs and optional object properties; turning on StrictMode globally breaks
# those scripts until they are audited per-file. Prefer enabling StrictMode in new isolated scripts.

# Returns the absolute repository root as a string (callers must join with Join-Path / string ops; do not use .Path).
function Get-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Get-EnvironmentVariableAnyScope {
    param([Parameter(Mandatory = $true)][string]$Name)

    foreach ($scope in @("Process", "User", "Machine")) {
        $value = [Environment]::GetEnvironmentVariable($Name, $scope)
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            return $value
        }
    }

    return $null
}

function Join-OptionalPath {
    param(
        [AllowNull()][string]$Path,
        [Parameter(Mandatory = $true)][string]$ChildPath
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return $null
    }

    return Join-Path $Path $ChildPath
}

function Get-LocalApplicationDataRoot {
    $localAppData = Get-EnvironmentVariableAnyScope "LOCALAPPDATA"
    if (-not [string]::IsNullOrWhiteSpace($localAppData)) {
        return $localAppData
    }

    $specialFolder = [Environment]::GetFolderPath([Environment+SpecialFolder]::LocalApplicationData)
    if (-not [string]::IsNullOrWhiteSpace($specialFolder)) {
        return $specialFolder
    }

    $home = Get-EnvironmentVariableAnyScope "HOME"
    if (-not [string]::IsNullOrWhiteSpace($home)) {
        return Join-Path $home ".local/share"
    }

    return $null
}

function Get-VcpkgDefaultTriplet {
    $explicitTriplet = Get-EnvironmentVariableAnyScope "VCPKG_DEFAULT_TRIPLET"
    if (-not [string]::IsNullOrWhiteSpace($explicitTriplet)) {
        return $explicitTriplet
    }

    $presetTriplet = Get-EnvironmentVariableAnyScope "VCPKG_TARGET_TRIPLET"
    if (-not [string]::IsNullOrWhiteSpace($presetTriplet)) {
        return $presetTriplet
    }

    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        return "x64-windows"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Linux)) {
        return "x64-linux"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::OSX)) {
        return "x64-osx"
    }

    return "x64-windows"
}

function Get-CombinedPathEntries {
    $seen = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($scope in @("Process", "User", "Machine")) {
        $pathValue = [Environment]::GetEnvironmentVariable("Path", $scope)
        if ([string]::IsNullOrWhiteSpace($pathValue)) {
            continue
        }

        foreach ($entry in $pathValue.Split([IO.Path]::PathSeparator)) {
            if ([string]::IsNullOrWhiteSpace($entry)) {
                continue
            }

            $trimmed = $entry.Trim()
            if ($seen.Add($trimmed)) {
                $trimmed
            }
        }
    }
}

function Find-CommandOnCombinedPath {
    param([Parameter(Mandatory = $true)][string]$Name)

    $fromPath = Get-Command $Name -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($fromPath) {
        return $fromPath.Source
    }

    $extensions = @("")
    if (-not [IO.Path]::HasExtension($Name)) {
        $extensions = @(".exe", ".cmd", ".bat", ".ps1", "")
    }

    foreach ($entry in Get-CombinedPathEntries) {
        foreach ($extension in $extensions) {
            $candidate = Join-Path $entry "$Name$extension"
            if (Test-Path -LiteralPath $candidate -PathType Leaf) {
                return (Resolve-Path -LiteralPath $candidate).Path
            }
        }
    }

    return $null
}

function Set-ProcessEnvironmentFromAnyScope {
    [CmdletBinding(SupportsShouldProcess = $true)]
    param([Parameter(Mandatory = $true)][string]$Name)

    if ([string]::IsNullOrWhiteSpace([Environment]::GetEnvironmentVariable($Name, "Process"))) {
        $value = Get-EnvironmentVariableAnyScope $Name
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            if ($PSCmdlet.ShouldProcess("Process environment variable '$Name'", 'Copy value from User/Machine scope')) {
                [Environment]::SetEnvironmentVariable($Name, $value, "Process")
            }
        }
    }
}

function Add-ProcessPathEntries {
    param([string[]]$Entries)

    $pathKey = if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        "Path"
    } else {
        "PATH"
    }
    $currentPath = [Environment]::GetEnvironmentVariable($pathKey, "Process")
    if ($null -eq $currentPath) {
        $currentPath = [Environment]::GetEnvironmentVariable("Path", "Process")
    }
    if ($null -eq $currentPath) {
        $currentPath = [Environment]::GetEnvironmentVariable("PATH", "Process")
    }

    $seen = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $pathEntries = [System.Collections.Generic.List[string]]::new()
    foreach ($entry in @($Entries)) {
        if ([string]::IsNullOrWhiteSpace($entry)) {
            continue
        }
        $resolved = [System.IO.Path]::GetFullPath($entry)
        if ($seen.Add($resolved)) {
            $pathEntries.Add($resolved) | Out-Null
        }
    }
    if (-not [string]::IsNullOrWhiteSpace($currentPath)) {
        foreach ($entry in $currentPath.Split([IO.Path]::PathSeparator)) {
            if ([string]::IsNullOrWhiteSpace($entry)) {
                continue
            }
            $trimmed = $entry.Trim()
            if ($seen.Add($trimmed)) {
                $pathEntries.Add($trimmed) | Out-Null
            }
        }
    }

    [Environment]::SetEnvironmentVariable($pathKey, ($pathEntries -join [IO.Path]::PathSeparator), "Process")
}

function Get-ExistingVcpkgToolPathEntries {
    param([Parameter(Mandatory = $true)][string]$DownloadsRoot)

    $toolRoots = @(
        (Join-Path $DownloadsRoot "tools"),
        (Join-Path (Join-Path (Get-RepoRoot) "external/vcpkg/downloads") "tools")
    )
    $entries = [System.Collections.Generic.List[string]]::new()

    foreach ($toolRoot in $toolRoots) {
        if (-not (Test-Path -LiteralPath $toolRoot -PathType Container)) {
            continue
        }

        foreach ($directory in Get-ChildItem -LiteralPath $toolRoot -Directory -ErrorAction SilentlyContinue) {
            $name = $directory.Name
            $path = $directory.FullName
            $candidate = $null
            if ($name -like "7zip-*-windows") {
                $candidate = Join-Path $path "7z.exe"
            } elseif ($name -like "7zr-*-windows") {
                $candidate = Join-Path $path "7zr.exe"
            } elseif ($name -like "ninja-*-windows") {
                $candidate = Join-Path $path "ninja.exe"
            }

            if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
                $entries.Add($path) | Out-Null
            }

            if ($name -like "cmake-*-windows") {
                $cmakeBin = Get-ChildItem -LiteralPath $path -Directory -ErrorAction SilentlyContinue |
                    ForEach-Object { Join-Path $_.FullName "bin" } |
                    Where-Object { Test-Path -LiteralPath (Join-Path $_ "cmake.exe") -PathType Leaf } |
                    Select-Object -First 1
                if ($cmakeBin) {
                    $entries.Add($cmakeBin) | Out-Null
                }
            }
        }
    }

    $cmake = Get-CMakeCommand
    if ($cmake) {
        $entries.Add((Split-Path -Parent $cmake)) | Out-Null
    }
    $ninja = Get-NinjaCommand
    if ($ninja) {
        $entries.Add((Split-Path -Parent $ninja)) | Out-Null
    }

    return @($entries)
}

function Set-MirakanaiVcpkgEnvironment {
    [CmdletBinding(SupportsShouldProcess = $true)]
    param([string]$CacheRoot)

    if ([string]::IsNullOrWhiteSpace($CacheRoot)) {
        $CacheRoot = Join-Path (Get-RepoRoot) "out/vcpkg"
    } elseif (-not [System.IO.Path]::IsPathRooted($CacheRoot)) {
        $CacheRoot = Join-Path (Get-RepoRoot) $CacheRoot
    }

    $cacheRootPath = [System.IO.Path]::GetFullPath($CacheRoot)
    $downloads = [System.IO.Path]::GetFullPath((Join-Path $cacheRootPath "downloads"))
    $binaryCache = [System.IO.Path]::GetFullPath((Join-Path $cacheRootPath "binary-cache"))

    if (-not $PSCmdlet.ShouldProcess(
            "VCPKG cache root '$cacheRootPath' (downloads + binary-cache directories and process-scoped vcpkg variables)",
            'Prepare Mirakanai vcpkg cache environment')) {
        return [pscustomobject]@{
            CacheRoot = $cacheRootPath
            Downloads = $downloads
            BinaryCache = $binaryCache
        }
    }

    foreach ($path in @($downloads, $binaryCache)) {
        if ($path.Contains(";") -or $path.Contains(",")) {
            Write-Error "vcpkg cache paths must not contain ';' or ',' because VCPKG_BINARY_SOURCES uses those delimiters: $path"
        }
        New-Item -ItemType Directory -Path $path -Force | Out-Null
    }

    [Environment]::SetEnvironmentVariable("VCPKG_DOWNLOADS", $downloads, "Process")
    [Environment]::SetEnvironmentVariable("VCPKG_DEFAULT_BINARY_CACHE", $binaryCache, "Process")
    [Environment]::SetEnvironmentVariable("VCPKG_BINARY_SOURCES", "clear;files,$binaryCache,readwrite", "Process")
    [Environment]::SetEnvironmentVariable("VCPKG_DISABLE_METRICS", "1", "Process")
    [Environment]::SetEnvironmentVariable("VCPKG_DEFAULT_TRIPLET", (Get-VcpkgDefaultTriplet), "Process")

    Add-ProcessPathEntries -Entries (Get-ExistingVcpkgToolPathEntries -DownloadsRoot $downloads)
    if ((Find-CommandOnCombinedPath "cmake") -and
        (Find-CommandOnCombinedPath "ninja") -and
        (Find-CommandOnCombinedPath "7z")) {
        [Environment]::SetEnvironmentVariable("VCPKG_FORCE_SYSTEM_BINARIES", "1", "Process")
    } else {
        [Environment]::SetEnvironmentVariable("VCPKG_FORCE_SYSTEM_BINARIES", $null, "Process")
    }

    return [pscustomobject]@{
        CacheRoot = $cacheRootPath
        Downloads = $downloads
        BinaryCache = $binaryCache
    }
}

function Find-AndroidSdkRoot {
    $localAppData = Get-LocalApplicationDataRoot
    $candidates = @(
        $env:ANDROID_HOME,
        $env:ANDROID_SDK_ROOT,
        (Get-EnvironmentVariableAnyScope "ANDROID_HOME"),
        (Get-EnvironmentVariableAnyScope "ANDROID_SDK_ROOT"),
        (Join-OptionalPath $localAppData "Android\Sdk")
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Container) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return $null
}

function Find-AndroidPlatformToolCommand {
    param([Parameter(Mandatory = $true)][string]$Name)

    $fromPath = Find-CommandOnCombinedPath $Name
    if ($fromPath) {
        return $fromPath
    }

    $sdk = Find-AndroidSdkRoot
    if ($sdk) {
        $candidate = Join-Path $sdk "platform-tools\$Name.exe"
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return $null
}

function Find-AndroidBuildToolCommand {
    param([Parameter(Mandatory = $true)][string]$Name)

    $fromPath = Find-CommandOnCombinedPath $Name
    if ($fromPath) {
        return $fromPath
    }

    $sdk = Find-AndroidSdkRoot
    if (-not $sdk) {
        return $null
    }

    $buildTools = Join-Path $sdk "build-tools"
    if (-not (Test-Path -LiteralPath $buildTools -PathType Container)) {
        return $null
    }

    $candidate = Get-ChildItem -LiteralPath $buildTools -Directory |
        Sort-Object Name -Descending |
        ForEach-Object {
            Join-Path $_.FullName "$Name.bat"
            Join-Path $_.FullName "$Name.exe"
        } |
        Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
        Select-Object -First 1

    if ($candidate) {
        return (Resolve-Path -LiteralPath $candidate).Path
    }

    return $null
}

function Find-AndroidEmulatorCommand {
    $fromPath = Find-CommandOnCombinedPath "emulator"
    if ($fromPath) {
        return $fromPath
    }

    $sdk = Find-AndroidSdkRoot
    if ($sdk) {
        $candidate = Join-Path $sdk "emulator\emulator.exe"
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return $null
}

function Set-AndroidAvdHomeEnvironment {
    [CmdletBinding(SupportsShouldProcess = $true)]
    param()

    if (-not [string]::IsNullOrWhiteSpace([Environment]::GetEnvironmentVariable("ANDROID_AVD_HOME", "Process"))) {
        return [Environment]::GetEnvironmentVariable("ANDROID_AVD_HOME", "Process")
    }

    $candidates = @(
        (Get-EnvironmentVariableAnyScope "ANDROID_AVD_HOME"),
        $(if (-not [string]::IsNullOrWhiteSpace($env:USERPROFILE)) {
            Join-Path $env:USERPROFILE ".android\avd"
        })
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Container) {
            $resolved = (Resolve-Path -LiteralPath $candidate).Path
            if ($PSCmdlet.ShouldProcess("Process environment variable ANDROID_AVD_HOME", "Set to '$resolved'")) {
                [Environment]::SetEnvironmentVariable("ANDROID_AVD_HOME", $resolved, "Process")
            }
            return $resolved
        }
    }

    return $null
}

function Get-AndroidAvdNames {
    $emulator = Find-AndroidEmulatorCommand
    if (-not $emulator) {
        return @()
    }

    Set-AndroidAvdHomeEnvironment | Out-Null

    $names = @(& $emulator "-list-avds" 2>$null)
    if ($LASTEXITCODE -ne 0) {
        return @()
    }

    return @($names | ForEach-Object { ([string]$_).Trim() } | Where-Object {
        -not [string]::IsNullOrWhiteSpace($_)
    })
}

function Find-JavaToolCommand {
    param([Parameter(Mandatory = $true)][string]$Name)

    $javaHome = Get-EnvironmentVariableAnyScope "JAVA_HOME"
    if (-not [string]::IsNullOrWhiteSpace($javaHome)) {
        $candidate = Join-Path $javaHome "bin\$Name.exe"
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    return Find-CommandOnCombinedPath $Name
}

function Get-VisualStudioInstallationPath {
    $vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        return $null
    }

    $path = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($path)) {
        return $null
    }

    return $path.Trim()
}

function Get-CMakeCommand {
    $fromPath = Get-Command cmake -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    $vsPath = Get-VisualStudioInstallationPath
    if ($vsPath) {
        $candidate = Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

function Get-NinjaCommand {
    $fromPath = Get-Command ninja -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    $vsPath = Get-VisualStudioInstallationPath
    if ($vsPath) {
        $candidate = Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

function Get-CTestCommand {
    $fromPath = Get-Command ctest -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    $cmake = Get-CMakeCommand
    if ($cmake) {
        $candidate = Join-Path (Split-Path -Parent $cmake) "ctest.exe"
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

function Get-CPackCommand {
    $fromPath = Get-Command cpack -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    $cmake = Get-CMakeCommand
    if ($cmake) {
        $candidate = Join-Path (Split-Path -Parent $cmake) "cpack.exe"
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    return $null
}

function Get-ClangFormatCommand {
    $fromPath = Get-Command clang-format -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    $standalone = "C:\Program Files\LLVM\bin\clang-format.exe"
    if (Test-Path $standalone) {
        return $standalone
    }

    $vsPath = Get-VisualStudioInstallationPath
    if ($vsPath) {
        $candidates = @(
            (Join-Path $vsPath "VC\Tools\Llvm\x64\bin\clang-format.exe"),
            (Join-Path $vsPath "VC\Tools\Llvm\bin\clang-format.exe"),
            (Join-Path $vsPath "VC\Tools\Llvm\ARM64\bin\clang-format.exe")
        )
        foreach ($candidate in $candidates) {
            if (Test-Path $candidate) {
                return $candidate
            }
        }
    }

    return $null
}

function Get-ClangTidyCommand {
    $fromPath = Get-Command clang-tidy -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    $standalone = "C:\Program Files\LLVM\bin\clang-tidy.exe"
    if (Test-Path $standalone) {
        return $standalone
    }

    $vsPath = Get-VisualStudioInstallationPath
    if ($vsPath) {
        $candidates = @(
            (Join-Path $vsPath "VC\Tools\Llvm\x64\bin\clang-tidy.exe"),
            (Join-Path $vsPath "VC\Tools\Llvm\bin\clang-tidy.exe"),
            (Join-Path $vsPath "VC\Tools\Llvm\ARM64\bin\clang-tidy.exe")
        )
        foreach ($candidate in $candidates) {
            if (Test-Path $candidate) {
                return $candidate
            }
        }
    }

    return $null
}

function Assert-CppBuildTools {
    $cmake = Get-CMakeCommand
    if (-not $cmake) {
        Write-Error "CMake is required but was not found. Install official CMake 3.30+ or Visual Studio Build Tools with C++ CMake tools."
    }

    $ctest = Get-CTestCommand
    if (-not $ctest) {
        Write-Error "CTest is required but was not found. Install official CMake 3.30+ or Visual Studio Build Tools with C++ CMake tools."
    }

    $cpack = Get-CPackCommand

    $vsPath = $null
    $msbuild = $null
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        $vsPath = Get-VisualStudioInstallationPath
        if (-not $vsPath) {
            Write-Error "Visual Studio Build Tools are required on Windows but were not found. Install official Visual Studio Build Tools with MSVC and MSBuild."
        }

        $msbuild = Join-Path $vsPath "MSBuild\Current\Bin\MSBuild.exe"
        if (-not (Test-Path $msbuild)) {
            Write-Error "MSBuild was not found under Visual Studio Build Tools: $msbuild"
        }
    }

    return @{
        CMake = $cmake
        CTest = $ctest
        CPack = $cpack
        ClangFormat = Get-ClangFormatCommand
        ClangTidy = Get-ClangTidyCommand
        VisualStudio = $vsPath
        MSBuild = $msbuild
    }
}

function Get-NormalizedProcessEnvironment {
    $seen = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $entries = [System.Collections.Generic.List[object]]::new()

    foreach ($entry in [System.Environment]::GetEnvironmentVariables().GetEnumerator()) {
        $key = [string]$entry.Key
        $value = [string]$entry.Value

        if ($key -ieq "Path" -or $key -ceq "PATH") {
            continue
        }

        if ($seen.Add($key)) {
            $entries.Add([pscustomobject]@{
                Key = $key
                Value = $value
            }) | Out-Null
        }
    }

    $pathKey = if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        "Path"
    } else {
        "PATH"
    }
    $pathValue = [Environment]::GetEnvironmentVariable($pathKey, "Process")
    if ($null -eq $pathValue) {
        $pathValue = [Environment]::GetEnvironmentVariable("Path", "Process")
    }
    if ($null -eq $pathValue) {
        $pathValue = [Environment]::GetEnvironmentVariable("PATH", "Process")
    }
    if ($null -ne $pathValue -and $seen.Add($pathKey)) {
        $entries.Add([pscustomobject]@{
            Key = $pathKey
            Value = $pathValue
        }) | Out-Null
    }

    return $entries
}

function Invoke-CheckedCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]]$Arguments
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.WorkingDirectory = (Get-Location).Path
    $startInfo.UseShellExecute = $false
    foreach ($argument in $Arguments) {
        $startInfo.ArgumentList.Add($argument) | Out-Null
    }
    $startInfo.Environment.Clear()
    foreach ($entry in Get-NormalizedProcessEnvironment) {
        $startInfo.Environment[$entry.Key] = $entry.Value
    }

    $process = [System.Diagnostics.Process]::Start($startInfo)
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) {
        Write-Error "Command failed with exit code $($process.ExitCode): $FilePath $($Arguments -join ' ')"
    }
}

function Read-DesktopRuntimeGameMetadata {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Write-Error "Desktop runtime game metadata was not found: $Path"
    }

    $metadata = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    if ($metadata.schemaVersion -ne 1) {
        Write-Error "Desktop runtime game metadata has unsupported schemaVersion: $Path"
    }
    if (-not $metadata.PSObject.Properties.Name.Contains("registeredGames")) {
        Write-Error "Desktop runtime game metadata missing registeredGames: $Path"
    }
    foreach ($game in @($metadata.registeredGames)) {
        if (-not $game.PSObject.Properties.Name.Contains("gameManifest")) {
            Write-Error "Desktop runtime game metadata entry missing gameManifest: $Path"
        }
        Get-NormalizedDesktopRuntimeGameManifestPath -GameMetadata $game | Out-Null
        Assert-DesktopRuntimeGamePackageFileMetadata -GameMetadata $game | Out-Null
    }

    return $metadata
}

function Get-DesktopRuntimeGameMetadata {
    param(
        [Parameter(Mandatory = $true)]$Metadata,
        [Parameter(Mandatory = $true)][string]$GameTarget
    )

    $registeredGamesForTarget = @($Metadata.registeredGames | Where-Object { $_.target -eq $GameTarget })
    if ($registeredGamesForTarget.Count -ne 1) {
        Write-Error "Desktop runtime game metadata must contain exactly one entry for target '$GameTarget'."
    }

    return $registeredGamesForTarget[0]
}

function Get-NormalizedDesktopRuntimeGameManifestPath {
    param([Parameter(Mandatory = $true)]$GameMetadata)

    if (-not $GameMetadata.PSObject.Properties.Name.Contains("gameManifest")) {
        Write-Error "Desktop runtime game metadata target '$($GameMetadata.target)' is missing gameManifest."
    }

    $manifestPath = [string]$GameMetadata.gameManifest
    if ([string]::IsNullOrWhiteSpace($manifestPath)) {
        Write-Error "Desktop runtime game metadata target '$($GameMetadata.target)' must declare gameManifest."
    }

    $normalizedPath = $manifestPath.Replace("\", "/")
    if ($normalizedPath.StartsWith("/") -or $normalizedPath -match "^[A-Za-z]:") {
        Write-Error "Desktop runtime game metadata target '$($GameMetadata.target)' gameManifest must be repository-relative: $manifestPath"
    }
    if ($normalizedPath.Split("/") -contains "..") {
        Write-Error "Desktop runtime game metadata target '$($GameMetadata.target)' gameManifest must not contain parent-directory segments: $manifestPath"
    }
    if ($normalizedPath -notmatch "^games/[a-z][a-z0-9_]*/game\.agent\.json$") {
        Write-Error "Desktop runtime game metadata target '$($GameMetadata.target)' gameManifest must point at games/<game_name>/game.agent.json: $manifestPath"
    }

    return $normalizedPath
}

function Resolve-DesktopRuntimeGameManifestPath {
    param(
        [Parameter(Mandatory = $true)]$GameMetadata,
        [Parameter(Mandatory = $true)][string]$Root,
        [switch]$Installed
    )

    $relativePath = Get-NormalizedDesktopRuntimeGameManifestPath -GameMetadata $GameMetadata
    if ($Installed.IsPresent) {
        $sampleRelativePath = $relativePath.Substring("games/".Length)
        return Join-Path (Join-Path $Root "share/Mirakanai/samples") $sampleRelativePath.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
    }

    return Join-Path $Root $relativePath.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
}

function ConvertTo-DesktopRuntimePathKey {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        return $Path.ToUpperInvariant()
    }

    return $Path
}

function ConvertTo-DesktopRuntimeMetadataRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $normalizedPath = $Path.Replace("\", "/")
    if ([string]::IsNullOrWhiteSpace($normalizedPath)) {
        Write-Error "$Description must not be empty."
    }
    if ($normalizedPath.StartsWith("/") -or $normalizedPath -match "^[A-Za-z]:") {
        Write-Error "$Description must be relative: $Path"
    }

    $normalizedSegments = @()
    foreach ($segment in $normalizedPath.Split("/")) {
        if ([string]::IsNullOrWhiteSpace($segment)) {
            Write-Error "$Description must not contain empty path segments: $Path"
        }
        if ($segment -eq ".") {
            continue
        }
        if ($segment -eq "..") {
            Write-Error "$Description must not contain parent-directory segments: $Path"
        }
        $normalizedSegments += $segment
    }
    if ($normalizedSegments.Count -eq 0) {
        Write-Error "$Description must name a file path: $Path"
    }

    return ($normalizedSegments -join "/")
}

function Assert-DesktopRuntimeSnakeCaseRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    foreach ($segment in $Path.Split("/")) {
        if ($segment -notmatch "^[a-z0-9][a-z0-9_]*(\.[a-z0-9][a-z0-9_]*)*$") {
            Write-Error "$Description must use lowercase snake_case path segments: $Path"
        }
    }
}

function Resolve-DesktopRuntimeRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$RelativePath
    )

    $rootPath = [System.IO.Path]::GetFullPath($Root)
    $nativeRelativePath = $RelativePath.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
    return [System.IO.Path]::GetFullPath((Join-Path $rootPath $nativeRelativePath))
}

function Assert-DesktopRuntimePathUnderRoot {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $trimChars = @([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)
    $pathFull = [System.IO.Path]::GetFullPath($Path).TrimEnd($trimChars)
    $rootFull = [System.IO.Path]::GetFullPath($Root).TrimEnd($trimChars)
    $rootPrefix = $rootFull + [System.IO.Path]::DirectorySeparatorChar
    $comparison = [System.StringComparison]::Ordinal
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        $comparison = [System.StringComparison]::OrdinalIgnoreCase
    }
    if (-not $pathFull.Equals($rootFull, $comparison) -and
        -not $pathFull.StartsWith($rootPrefix, $comparison)) {
        Write-Error "$Description escaped its expected root: $Path"
    }
}

function Assert-DesktopRuntimeGamePackageFileMetadata {
    param([Parameter(Mandatory = $true)]$GameMetadata)

    if (-not $GameMetadata.PSObject.Properties.Name.Contains("packageFiles")) {
        Write-Error "Desktop runtime game metadata target '$($GameMetadata.target)' is missing packageFiles."
    }

    $manifestPath = Get-NormalizedDesktopRuntimeGameManifestPath -GameMetadata $GameMetadata
    $gameDirectory = $manifestPath.Substring(0, $manifestPath.LastIndexOf("/"))
    $seenSourcePaths = @{}
    $seenInstallPaths = @{}
    foreach ($packageFile in @($GameMetadata.packageFiles)) {
        foreach ($property in @("source", "install")) {
            if (-not $packageFile.PSObject.Properties.Name.Contains($property)) {
                Write-Error "Desktop runtime game metadata target '$($GameMetadata.target)' packageFiles entry missing $property."
            }
        }

        $sourcePath = ConvertTo-DesktopRuntimeMetadataRelativePath `
            -Path ([string]$packageFile.source) `
            -Description "Desktop runtime game metadata target '$($GameMetadata.target)' package source path"
        $installPath = ConvertTo-DesktopRuntimeMetadataRelativePath `
            -Path ([string]$packageFile.install) `
            -Description "Desktop runtime game metadata target '$($GameMetadata.target)' package install path"
        if (-not $sourcePath.StartsWith("$gameDirectory/", [System.StringComparison]::Ordinal)) {
            Write-Error "Desktop runtime game metadata target '$($GameMetadata.target)' package source must stay under $gameDirectory/: $sourcePath"
        }
        if (-not $installPath.StartsWith("bin/", [System.StringComparison]::Ordinal) -or $installPath -eq "bin/") {
            Write-Error "Desktop runtime game metadata target '$($GameMetadata.target)' package install path must stay under bin/: $installPath"
        }

        $sourceKey = ConvertTo-DesktopRuntimePathKey -Path $sourcePath
        if ($seenSourcePaths.ContainsKey($sourceKey)) {
            Write-Error "Desktop runtime game metadata target '$($GameMetadata.target)' package source path is duplicated after normalization: $sourcePath"
        }
        $seenSourcePaths[$sourceKey] = $true

        $installKey = ConvertTo-DesktopRuntimePathKey -Path $installPath
        if ($seenInstallPaths.ContainsKey($installKey)) {
            Write-Error "Desktop runtime game metadata target '$($GameMetadata.target)' package install path is duplicated after normalization: $installPath"
        }
        $seenInstallPaths[$installKey] = $true
    }
}

function Get-DesktopRuntimeManifestRuntimePackageFiles {
    param(
        [Parameter(Mandatory = $true)]$Manifest,
        [Parameter(Mandatory = $true)][string]$ManifestDescription
    )

    if (-not $Manifest.PSObject.Properties.Name.Contains("runtimePackageFiles")) {
        return @()
    }
    if ($null -eq $Manifest.runtimePackageFiles -or $Manifest.runtimePackageFiles -isnot [System.Array]) {
        Write-Error "$ManifestDescription runtimePackageFiles must be an array."
    }

    $runtimePackageFiles = @()
    $seenRuntimePackageFiles = @{}
    foreach ($entry in $Manifest.runtimePackageFiles) {
        if ($entry -isnot [string]) {
            Write-Error "$ManifestDescription runtimePackageFiles entries must be strings."
        }
        $packageFile = ConvertTo-DesktopRuntimeMetadataRelativePath `
            -Path $entry `
            -Description "$ManifestDescription runtimePackageFiles entry"
        if ($packageFile.StartsWith("games/", [System.StringComparison]::Ordinal)) {
            Write-Error "$ManifestDescription runtimePackageFiles entries must be game-relative, not repository-relative: $packageFile"
        }
        if ($packageFile -match ";") {
            Write-Error "$ManifestDescription runtimePackageFiles entries must not contain CMake list separators: $packageFile"
        }
        Assert-DesktopRuntimeSnakeCaseRelativePath `
            -Path $packageFile `
            -Description "$ManifestDescription runtimePackageFiles entry"

        $packageFileKey = ConvertTo-DesktopRuntimePathKey -Path $packageFile
        if ($seenRuntimePackageFiles.ContainsKey($packageFileKey)) {
            Write-Error "$ManifestDescription runtimePackageFiles entry is duplicated after normalization: $packageFile"
        }
        $seenRuntimePackageFiles[$packageFileKey] = $true
        $runtimePackageFiles += $packageFile
    }

    return @($runtimePackageFiles)
}

function Assert-DesktopRuntimeGamePackageFilesMatchManifest {
    param(
        [Parameter(Mandatory = $true)]$GameMetadata,
        [Parameter(Mandatory = $true)]$Manifest,
        [Parameter(Mandatory = $true)][string]$ManifestDescription
    )

    if (-not $GameMetadata.PSObject.Properties.Name.Contains("packageFiles")) {
        Write-Error "Desktop runtime game metadata target '$($GameMetadata.target)' is missing packageFiles."
    }

    $manifestPath = Get-NormalizedDesktopRuntimeGameManifestPath -GameMetadata $GameMetadata
    $gameDirectory = $manifestPath.Substring(0, $manifestPath.LastIndexOf("/"))
    $expectedPackageFiles = @()
    foreach ($runtimePackageFile in Get-DesktopRuntimeManifestRuntimePackageFiles -Manifest $Manifest -ManifestDescription $ManifestDescription) {
        $expectedPackageFiles += [pscustomobject]@{
            source = "$gameDirectory/$runtimePackageFile"
            install = "bin/$runtimePackageFile"
        }
    }

    $actualPackageFiles = @()
    foreach ($packageFile in @($GameMetadata.packageFiles)) {
        foreach ($property in @("source", "install")) {
            if (-not $packageFile.PSObject.Properties.Name.Contains($property)) {
                Write-Error "Desktop runtime game metadata target '$($GameMetadata.target)' packageFiles entry missing $property."
            }
        }
        $actualPackageFiles += [pscustomobject]@{
            source = ConvertTo-DesktopRuntimeMetadataRelativePath `
                -Path ([string]$packageFile.source) `
                -Description "Desktop runtime game metadata target '$($GameMetadata.target)' package source path"
            install = ConvertTo-DesktopRuntimeMetadataRelativePath `
                -Path ([string]$packageFile.install) `
                -Description "Desktop runtime game metadata target '$($GameMetadata.target)' package install path"
        }
    }

    if ($expectedPackageFiles.Count -ne $actualPackageFiles.Count) {
        Write-Error "Desktop runtime game metadata target '$($GameMetadata.target)' packageFiles count ($($actualPackageFiles.Count)) does not match manifest runtimePackageFiles count ($($expectedPackageFiles.Count))."
    }

    $expectedKeys = @{}
    foreach ($expected in $expectedPackageFiles) {
        $expectedKeys["$($expected.source)|$($expected.install)"] = $true
    }
    $actualKeys = @{}
    foreach ($actual in $actualPackageFiles) {
        $key = "$($actual.source)|$($actual.install)"
        $actualKeys[$key] = $true
        if (-not $expectedKeys.ContainsKey($key)) {
            Write-Error "Desktop runtime game metadata target '$($GameMetadata.target)' packageFiles entry '$($actual.source)' -> '$($actual.install)' is not declared by $ManifestDescription runtimePackageFiles."
        }
    }
    foreach ($expected in $expectedPackageFiles) {
        $key = "$($expected.source)|$($expected.install)"
        if (-not $actualKeys.ContainsKey($key)) {
            Write-Error "$ManifestDescription runtimePackageFiles entry '$($expected.source)' -> '$($expected.install)' is missing from desktop runtime package metadata for target '$($GameMetadata.target)'."
        }
    }
}

function Assert-DesktopRuntimeGamePackageFiles {
    param(
        [Parameter(Mandatory = $true)]$GameMetadata,
        [Parameter(Mandatory = $true)][string]$Root,
        [switch]$Installed
    )

    Assert-DesktopRuntimeGamePackageFileMetadata -GameMetadata $GameMetadata | Out-Null
    $manifestPath = Get-NormalizedDesktopRuntimeGameManifestPath -GameMetadata $GameMetadata
    $gameDirectory = $manifestPath.Substring(0, $manifestPath.LastIndexOf("/"))
    $gameDirectoryPath = Resolve-DesktopRuntimeRelativePath -Root $Root -RelativePath $gameDirectory
    $binDirectoryPath = Resolve-DesktopRuntimeRelativePath -Root $Root -RelativePath "bin"
    $seenSourcePaths = @{}
    $seenInstallPaths = @{}
    foreach ($packageFile in @($GameMetadata.packageFiles)) {
        $sourcePath = ConvertTo-DesktopRuntimeMetadataRelativePath `
            -Path ([string]$packageFile.source) `
            -Description "Desktop runtime game metadata target '$($GameMetadata.target)' package source path"
        $installPath = ConvertTo-DesktopRuntimeMetadataRelativePath `
            -Path ([string]$packageFile.install) `
            -Description "Desktop runtime game metadata target '$($GameMetadata.target)' package install path"
        if ($Installed.IsPresent) {
            $path = Resolve-DesktopRuntimeRelativePath -Root $Root -RelativePath $installPath
            Assert-DesktopRuntimePathUnderRoot `
                -Path $path `
                -Root $binDirectoryPath `
                -Description "Installed desktop runtime package file '$installPath'"
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
                Write-Error "Installed desktop runtime package file was not found for target '$($GameMetadata.target)': $path"
            }
            $item = Get-Item -LiteralPath $path
            if (-not ($item -is [System.IO.FileInfo])) {
                Write-Error "Installed desktop runtime package path is not a file for target '$($GameMetadata.target)': $path"
            }
            if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
                Write-Error "Installed desktop runtime package file must not be a reparse point for target '$($GameMetadata.target)': $path"
            }
            if ($item.Length -le 0) {
                Write-Error "Installed desktop runtime package file is empty for target '$($GameMetadata.target)': $path"
            }
            $resolvedPath = [System.IO.Path]::GetFullPath((Resolve-Path -LiteralPath $path).ProviderPath)
            $installKey = ConvertTo-DesktopRuntimePathKey -Path $resolvedPath
            if ($seenInstallPaths.ContainsKey($installKey)) {
                Write-Error "Installed desktop runtime package file resolves to a duplicate path for target '$($GameMetadata.target)': $path"
            }
            $seenInstallPaths[$installKey] = $true
        } else {
            $path = Resolve-DesktopRuntimeRelativePath -Root $Root -RelativePath $sourcePath
            Assert-DesktopRuntimePathUnderRoot `
                -Path $path `
                -Root $gameDirectoryPath `
                -Description "Desktop runtime package source file '$sourcePath'"
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
                Write-Error "Desktop runtime package source file was not found for target '$($GameMetadata.target)': $path"
            }
            $item = Get-Item -LiteralPath $path
            if (-not ($item -is [System.IO.FileInfo])) {
                Write-Error "Desktop runtime package source path is not a file for target '$($GameMetadata.target)': $path"
            }
            if (($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
                Write-Error "Desktop runtime package source file must not be a reparse point for target '$($GameMetadata.target)': $path"
            }
            $resolvedPath = [System.IO.Path]::GetFullPath((Resolve-Path -LiteralPath $path).ProviderPath)
            $sourceKey = ConvertTo-DesktopRuntimePathKey -Path $resolvedPath
            if ($seenSourcePaths.ContainsKey($sourceKey)) {
                Write-Error "Desktop runtime package source file resolves to a duplicate path for target '$($GameMetadata.target)': $path"
            }
            $seenSourcePaths[$sourceKey] = $true
        }
    }
}

function Assert-DesktopRuntimeGameManifestContract {
    param(
        [Parameter(Mandatory = $true)]$GameMetadata,
        [Parameter(Mandatory = $true)][string]$Root,
        [string[]]$RequiredPackagingTargets = @(),
        [switch]$Installed
    )

    $manifestPath = Resolve-DesktopRuntimeGameManifestPath -GameMetadata $GameMetadata -Root $Root -Installed:$Installed
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        Write-Error "Desktop runtime game manifest was not found for target '$($GameMetadata.target)': $manifestPath"
    }

    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ($manifest.target -ne $GameMetadata.target) {
        Write-Error "Desktop runtime game manifest target '$($manifest.target)' does not match metadata target '$($GameMetadata.target)': $manifestPath"
    }
    Assert-DesktopRuntimeGamePackageFilesMatchManifest `
        -GameMetadata $GameMetadata `
        -Manifest $manifest `
        -ManifestDescription "Desktop runtime game manifest '$manifestPath'" | Out-Null

    foreach ($requiredTarget in $RequiredPackagingTargets) {
        if (-not (@($manifest.packagingTargets) -contains $requiredTarget)) {
            Write-Error "Desktop runtime game manifest for target '$($GameMetadata.target)' must declare packaging target '$requiredTarget': $manifestPath"
        }
    }

    return $manifest
}

function Initialize-RepoExclusiveToolMutex {
    <#
    .SYNOPSIS
        Acquires a machine-local named mutex keyed by repository root and tool id.

    .DESCRIPTION
        Serializes concurrent runs of the same tool against the same repository clone (for example
        multiple terminals or agents launching `tools/check-ai-integration.ps1` at once). Uses
        `System.Threading.Mutex`, which is the standard .NET cross-process lock primitive.

        On Windows the mutex lives in the `Global\` namespace so different logon sessions still
        contend on the same name. On non-Windows hosts the name omits `Global\` because it is not
        supported there.
    #>
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$RepositoryRoot,
        [Parameter(Mandatory = $true)][ValidatePattern('^[a-z][a-z0-9-]{0,63}$')][string]$ToolId,
        [Parameter()][ValidateRange(1, 1440)][double]$WaitTimeoutMinutes = 120.0,
        [Parameter()][ValidateRange(1, 600)][int]$StatusMessageIntervalSeconds = 15
    )

    $normalizedRoot = [System.IO.Path]::GetFullPath($RepositoryRoot).TrimEnd(
        [char[]]@([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar))
    $hashBytes = [System.Security.Cryptography.SHA256]::HashData(
        [System.Text.Encoding]::UTF8.GetBytes($normalizedRoot.ToLowerInvariant()))
    $rootFingerprint = ([BitConverter]::ToString($hashBytes) -replace '-', '').Substring(0, 32).ToLowerInvariant()
    $mutexSegment = "Mirakana_GameEngine_{0}_{1}" -f $ToolId, $rootFingerprint
    if ($mutexSegment.Length -gt 240) {
        throw "Internal error: computed mutex segment is unexpectedly long."
    }

    $mutexName = if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
            [System.Runtime.InteropServices.OSPlatform]::Windows)) {
        "Global\$mutexSegment"
    } else {
        $mutexSegment
    }

    $mutex = [System.Threading.Mutex]::new($false, $mutexName)
    try {
        try {
            if ($mutex.WaitOne(0)) {
                return $mutex
            }
        } catch [System.Threading.AbandonedMutexException] {
            return $mutex
        }

        Write-Information "tools/${ToolId}: waiting for exclusive repository mutex (another instance is running)..." -InformationAction Continue

        $deadlineUtc = [datetime]::UtcNow.AddMinutes($WaitTimeoutMinutes)
        while ($true) {
            $remaining = $deadlineUtc - [datetime]::UtcNow
            if ($remaining.Ticks -le 0) {
                $mutex.Dispose()
                Write-Error (
                    "Timed out after {0} minute(s) waiting for another process to release the exclusive repository mutex for tool '{1}'. Close duplicate terminals or agent hosts running the same script against this clone, or wait for the other run to finish." -f
                    $WaitTimeoutMinutes, $ToolId)
            }

            $sliceTicks = [Math]::Max(1L, [Math]::Min($remaining.Ticks, [TimeSpan]::FromSeconds($StatusMessageIntervalSeconds).Ticks))
            $slice = [TimeSpan]::FromTicks($sliceTicks)
            try {
                if ($mutex.WaitOne($slice)) {
                    return $mutex
                }
            } catch [System.Threading.AbandonedMutexException] {
                return $mutex
            }

            Write-Information "tools/${ToolId}: still waiting for exclusive repository mutex..." -InformationAction Continue
        }
    } catch {
        $mutex.Dispose()
        throw
    }
}

function Clear-RepoExclusiveToolMutex {
    [CmdletBinding()]
    param(
        [Parameter()][AllowNull()][System.Threading.Mutex]$Mutex
    )

    if ($null -eq $Mutex) {
        return
    }

    try {
        try {
            [void]$Mutex.ReleaseMutex()
        } catch {
            # ReleaseMutex throws when this process does not own the mutex; ignore and continue disposal.
            $null = $_.Exception
        }
    } finally {
        $Mutex.Dispose()
    }
}
