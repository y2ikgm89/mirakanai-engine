#requires -Version 7.0
#requires -PSEdition Core

param(
    [string]$Preset = "dev",
    [string]$Configuration = "Debug",
    [switch]$Strict,
    [int]$MaxFiles = 0,
    [string[]]$Files = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
# Get-RepoRoot returns a string path; do not use $root.Path (null on strings and breaks compile database synthesis).
$clangTidy = Get-ClangTidyCommand
$configPath = Join-Path $root ".clang-tidy"

function Resolve-RequestedTidyFiles {
    param(
        [AllowNull()][string[]]$Files,
        [Parameter(Mandatory = $true)][string]$RootPath
    )

    $requested = [System.Collections.Generic.List[string]]::new()
    $seen = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $repoPrefix = $RootPath.Replace('\', '/').TrimEnd('/') + '/'

    foreach ($fileGroup in @($Files)) {
        foreach ($rawFile in ([string]$fileGroup -split ',')) {
            $trimmed = $rawFile.Trim()
            if ([string]::IsNullOrWhiteSpace($trimmed)) {
                continue
            }

            $fullPath = if ([System.IO.Path]::IsPathRooted($trimmed)) {
                [System.IO.Path]::GetFullPath($trimmed)
            } else {
                [System.IO.Path]::GetFullPath((Join-Path $RootPath $trimmed))
            }
            $normalized = $fullPath.Replace('\', '/')

            if (-not $normalized.StartsWith($repoPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
                Write-Error "tidy-check: requested file is outside the repository: $trimmed"
            }
            if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
                Write-Error "tidy-check: requested file does not exist: $trimmed"
            }
            if ($normalized -notmatch '\.(?:cc|cpp|cxx)$') {
                Write-Error "tidy-check: requested file is not a clang-tidy source file (.cc, .cpp, .cxx): $trimmed"
            }

            if ($seen.Add($fullPath)) {
                $requested.Add($fullPath) | Out-Null
            }
        }
    }

    return @($requested)
}

function Write-TidyBlocker {
    param(
        [Parameter(Mandatory = $true)][string]$Message,
        [switch]$RequireStrict
    )

    if ($RequireStrict) {
        Write-Error $Message
    }

    Write-Host $Message
}

function Get-CMakeCacheValue {
    param(
        [Parameter(Mandatory = $true)][string]$BuildDir,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $cachePath = Join-Path $BuildDir "CMakeCache.txt"
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) {
        return $null
    }

    $line = Get-Content -LiteralPath $cachePath |
        Where-Object { $_ -match "^$([regex]::Escape($Name)):" } |
        Select-Object -First 1
    if ([string]::IsNullOrWhiteSpace($line)) {
        return $null
    }

    $separator = $line.IndexOf("=")
    if ($separator -lt 0) {
        return $null
    }

    return $line.Substring($separator + 1)
}

function Test-NativeCompileCommandsGenerator {
    param([AllowNull()][string]$Generator)

    if ([string]::IsNullOrWhiteSpace($Generator)) {
        return $false
    }

    return ($Generator -match "Ninja" -or $Generator -match "Makefiles")
}

function Resolve-FileApiPath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$BasePath
    )

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $BasePath $Path))
}

function Split-CompileCommandFragment {
    param([AllowNull()][string]$Fragment)

    $tokens = [System.Collections.Generic.List[string]]::new()
    if ([string]::IsNullOrWhiteSpace($Fragment)) {
        return $tokens
    }

    $matches = [regex]::Matches($Fragment, '"(?:[^"\\]|\\.)*"|\S+')
    foreach ($match in $matches) {
        $token = $match.Value
        if ($token.Length -ge 2 -and $token.StartsWith('"') -and $token.EndsWith('"')) {
            $token = $token.Substring(1, $token.Length - 2)
        }
        $tokens.Add($token) | Out-Null
    }

    return $tokens
}

function New-FileApiCodemodelQuery {
    param([Parameter(Mandatory = $true)][string]$BuildDir)

    $queryDir = Join-Path $BuildDir ".cmake/api/v1/query"
    New-Item -ItemType Directory -Force -Path $queryDir | Out-Null
    New-Item -ItemType File -Force -Path (Join-Path $queryDir "codemodel-v2") | Out-Null
}

function Get-FileApiCodemodelReply {
    param([Parameter(Mandatory = $true)][string]$BuildDir)

    $replyDir = Join-Path $BuildDir ".cmake/api/v1/reply"
    if (-not (Test-Path -LiteralPath $replyDir -PathType Container)) {
        return $null
    }

    $index = Get-ChildItem -LiteralPath $replyDir -Filter "index-*.json" -File -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTimeUtc -Descending |
        Select-Object -First 1
    if (-not $index) {
        return $null
    }

    $indexJson = Get-Content -LiteralPath $index.FullName -Raw | ConvertFrom-Json
    $codemodel = @($indexJson.objects | Where-Object { $_.kind -eq "codemodel" } | Select-Object -First 1)
    if ($codemodel.Count -eq 0 -or [string]::IsNullOrWhiteSpace([string]$codemodel[0].jsonFile)) {
        return $null
    }

    $codemodelPath = Join-Path $replyDir ([string]$codemodel[0].jsonFile)
    if (-not (Test-Path -LiteralPath $codemodelPath -PathType Leaf)) {
        return $null
    }

    return [pscustomobject]@{
        ReplyDir = $replyDir
        Path = $codemodelPath
        Model = (Get-Content -LiteralPath $codemodelPath -Raw | ConvertFrom-Json)
    }
}

function Convert-FileApiCodemodelToCompileDatabase {
    param(
        [Parameter(Mandatory = $true)]$CodemodelReply,
        [Parameter(Mandatory = $true)][string]$SourceDir,
        [Parameter(Mandatory = $true)][string]$BuildDir,
        [Parameter(Mandatory = $true)][string]$Configuration,
        [Parameter(Mandatory = $true)][string]$OutputPath
    )

    $configurationModel = @($CodemodelReply.Model.configurations |
        Where-Object { $_.name -eq $Configuration } |
        Select-Object -First 1)
    if ($configurationModel.Count -eq 0) {
        return 0
    }

    $entries = [System.Collections.Generic.List[object]]::new()
    $seenFiles = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    foreach ($target in @($configurationModel[0].targets)) {
        if ([string]::IsNullOrWhiteSpace([string]$target.jsonFile)) {
            continue
        }

        $targetPath = Join-Path $CodemodelReply.ReplyDir ([string]$target.jsonFile)
        if (-not (Test-Path -LiteralPath $targetPath -PathType Leaf)) {
            continue
        }

        $targetJson = Get-Content -LiteralPath $targetPath -Raw | ConvertFrom-Json
        if (-not $targetJson.PSObject.Properties.Name.Contains("compileGroups")) {
            continue
        }

        foreach ($source in @($targetJson.sources)) {
            if (-not $source.PSObject.Properties.Name.Contains("compileGroupIndex")) {
                continue
            }

            $sourcePath = Resolve-FileApiPath -Path ([string]$source.path) -BasePath $SourceDir
            $normalized = $sourcePath.Replace('\', '/')
            if ($normalized -match '/(?:out|external|vcpkg_installed)/') {
                continue
            }
            if ($normalized -notmatch '\.(?:cc|cpp|cxx)$') {
                continue
            }
            if (-not $seenFiles.Add($sourcePath)) {
                continue
            }

            $compileGroup = $targetJson.compileGroups[[int]$source.compileGroupIndex]
            $arguments = [System.Collections.Generic.List[string]]::new()
            $arguments.Add("cl.exe") | Out-Null
            $arguments.Add("/nologo") | Out-Null
            $arguments.Add("/TP") | Out-Null

            foreach ($define in @($compileGroup.defines)) {
                if (-not [string]::IsNullOrWhiteSpace([string]$define.define)) {
                    $arguments.Add("/D$($define.define)") | Out-Null
                }
            }

            foreach ($include in @($compileGroup.includes)) {
                if (-not [string]::IsNullOrWhiteSpace([string]$include.path)) {
                    $includePath = Resolve-FileApiPath -Path ([string]$include.path) -BasePath $SourceDir
                    $arguments.Add("/I$includePath") | Out-Null
                }
            }

            foreach ($fragment in @($compileGroup.compileCommandFragments)) {
                foreach ($token in (Split-CompileCommandFragment -Fragment ([string]$fragment.fragment))) {
                    if ($token -eq "/std:c++23preview") {
                        $arguments.Add("/std:c++latest") | Out-Null
                    } else {
                        $arguments.Add($token) | Out-Null
                    }
                }
            }

            $arguments.Add("/c") | Out-Null
            $arguments.Add($sourcePath) | Out-Null

            $entries.Add([pscustomobject]@{
                directory = $BuildDir
                arguments = @($arguments)
                file = $sourcePath
            }) | Out-Null
        }
    }

    if ($entries.Count -eq 0) {
        return 0
    }

    $entries | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $OutputPath -Encoding utf8NoBOM
    return $entries.Count
}

function Ensure-ClangTidyCompileDatabase {
    param(
        [Parameter(Mandatory = $true)][string]$Preset,
        [Parameter(Mandatory = $true)][string]$Configuration,
        [Parameter(Mandatory = $true)][string]$BuildDir,
        [Parameter(Mandatory = $true)][string]$CompileCommands,
        [switch]$RequireStrict
    )

    $generator = Get-CMakeCacheValue -BuildDir $BuildDir -Name "CMAKE_GENERATOR"
    if ((Test-Path -LiteralPath $CompileCommands -PathType Leaf) -and (Test-NativeCompileCommandsGenerator -Generator $generator)) {
        return $true
    }

    New-FileApiCodemodelQuery -BuildDir $BuildDir

    $cmake = Get-CMakeCommand
    if (-not $cmake) {
        $message = "tidy-check: blocker - CMake was not found, so clang-tidy could not generate File API compile database data for preset '$Preset'."
        Write-TidyBlocker -Message $message -RequireStrict:$RequireStrict
        return $false
    }

    Invoke-CheckedCommand $cmake "--preset" $Preset

    $generator = Get-CMakeCacheValue -BuildDir $BuildDir -Name "CMAKE_GENERATOR"
    if ((Test-Path -LiteralPath $CompileCommands -PathType Leaf) -and (Test-NativeCompileCommandsGenerator -Generator $generator)) {
        return $true
    }

    $reply = Get-FileApiCodemodelReply -BuildDir $BuildDir
    if ($reply) {
        $entryCount = Convert-FileApiCodemodelToCompileDatabase `
            -CodemodelReply $reply `
            -SourceDir $root `
            -BuildDir $BuildDir `
            -Configuration $Configuration `
            -OutputPath $CompileCommands
        if ($entryCount -gt 0) {
            Write-Host "tidy-check: generated compile database from CMake File API for preset '$Preset' configuration '$Configuration' ($entryCount files)"
            return $true
        }
    }

    $message = "tidy-check: blocker - compile_commands.json missing for preset '$Preset' at $CompileCommands and CMake File API codemodel data could not synthesize one."
    Write-TidyBlocker -Message $message -RequireStrict:$RequireStrict
    return $false
}

if (-not (Test-Path $configPath)) {
    Write-Error "Missing clang-tidy profile: .clang-tidy"
}

if (-not $clangTidy) {
    $message = "tidy-check: blocker - clang-tidy was not found. Install official LLVM or Visual Studio Build Tools LLVM components."
    if ($Strict) {
        Write-Error $message
    }
    Write-Host $message
    exit 0
}

& $clangTidy "--verify-config" "--config-file=$configPath" | Out-Host
if ($LASTEXITCODE -ne 0) {
    Write-Error "clang-tidy profile validation failed"
}

$presetsPath = Join-Path $root "CMakePresets.json"
$presets = Get-Content -LiteralPath $presetsPath -Raw | ConvertFrom-Json
$configurePreset = $presets.configurePresets | Where-Object { $_.name -eq $Preset } | Select-Object -First 1
if (-not $configurePreset) {
    Write-Error "Unknown CMake configure preset for clang-tidy: $Preset"
}

$buildDir = ([string]$configurePreset.binaryDir).Replace('${sourceDir}', $root)
$compileCommands = Join-Path $buildDir "compile_commands.json"
if (-not (Ensure-ClangTidyCompileDatabase -Preset $Preset -Configuration $Configuration -BuildDir $buildDir -CompileCommands $compileCommands -RequireStrict:$Strict)) {
    Write-Host "tidy-check: config ok"
    exit 0
}

$entries = Get-Content -LiteralPath $compileCommands -Raw | ConvertFrom-Json
$repoPrefix = $root.Replace('\', '/').TrimEnd('/') + '/'
$requestedFiles = @(Resolve-RequestedTidyFiles -Files $Files -RootPath $root)
$requestedSet = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
foreach ($requestedFile in $requestedFiles) {
    $requestedSet.Add($requestedFile) | Out-Null
}
$matchedRequested = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$tidyFiles = [System.Collections.Generic.List[string]]::new()
foreach ($entry in $entries) {
    $file = [string]$entry.file
    if ([string]::IsNullOrWhiteSpace($file)) {
        continue
    }

    $normalized = $file.Replace('\', '/')
    if (-not $normalized.StartsWith($repoPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        continue
    }
    if ($normalized -match '/(?:out|external|vcpkg_installed)/') {
        continue
    }
    if ($normalized -notmatch '\.(?:cc|cpp|cxx)$') {
        continue
    }
    if ($requestedSet.Count -gt 0) {
        if ($requestedSet.Contains($file)) {
            $matchedRequested.Add($file) | Out-Null
        }
        continue
    }
    if (-not $tidyFiles.Contains($file)) {
        $tidyFiles.Add($file) | Out-Null
    }
}

if ($requestedSet.Count -gt 0) {
    foreach ($requestedFile in $requestedFiles) {
        if (-not $matchedRequested.Contains($requestedFile)) {
            Write-Error "tidy-check: requested file is not present in the compile database for preset '$Preset': $requestedFile"
        }
        if (-not $tidyFiles.Contains($requestedFile)) {
            $tidyFiles.Add($requestedFile) | Out-Null
        }
    }
}

if ($MaxFiles -gt 0 -and $tidyFiles.Count -gt $MaxFiles) {
    $tidyFiles = [System.Collections.Generic.List[string]]($tidyFiles | Select-Object -First $MaxFiles)
}

foreach ($file in $tidyFiles) {
    & $clangTidy "--quiet" "-p" $buildDir $file | Out-Host
    if ($LASTEXITCODE -ne 0) {
        Write-Error "clang-tidy failed for $file"
    }
}

Write-Host "tidy-check: ok ($($tidyFiles.Count) files)"
