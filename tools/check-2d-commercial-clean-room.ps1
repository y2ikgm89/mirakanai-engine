#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [string]$ScanRootRelative = ""
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$textExtensions = @(".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hh", ".hxx", ".ixx", ".json", ".md", ".txt", ".cmake")
$textFileNames = @("CMakeLists.txt")

$forbiddenTokenPatterns = @(
    "(?<![A-Za-z0-9_])Unity(?:Engine|Editor)?(?![A-Za-z0-9_])",
    "(?<![A-Za-z0-9_])Unreal(?:Engine|Ed)?(?![A-Za-z0-9_])",
    "(?<![A-Za-z0-9_])Godot(?![A-Za-z0-9_])",
    "(?<![A-Za-z0-9_])GameMaker(?![A-Za-z0-9_])",
    "(?<![A-Za-z0-9_])Defold(?![A-Za-z0-9_])",
    "(?<![A-Za-z0-9_])O3DE(?![A-Za-z0-9_])",
    "(?<![A-Za-z0-9_])Blueprint(?:Graph)?(?![A-Za-z0-9_])",
    "(?<![A-Za-z0-9_])WidgetBlueprint(?![A-Za-z0-9_])",
    "(?<![A-Za-z0-9_])UMG(?![A-Za-z0-9_])",
    "(?<![A-Za-z0-9_])Slate(?![A-Za-z0-9_])",
    "(?<![A-Za-z0-9_])UXML(?![A-Za-z0-9_])",
    "(?<![A-Za-z0-9_])USS(?![A-Za-z0-9_])",
    "(?<![A-Za-z0-9_])PackedScene(?![A-Za-z0-9_])",
    "(?<![A-Za-z0-9_])Node2D(?![A-Za-z0-9_])",
    "(?<![A-Za-z0-9_])CanvasLayer(?![A-Za-z0-9_])",
    "(?<![A-Za-z0-9_])ControlNode(?![A-Za-z0-9_])",
    "\.(?:unity|uasset|umap|uproject|tscn|tres|godot)(?![A-Za-z0-9_])"
)

$forbiddenClaimPatterns = @(
    "(?i)\b(?:unity|unreal(?:\s+engine)?|godot|gamemaker|defold|o3de)[\s_-]*(?:compatible|compatibility|equivalent|equivalence|parity|replacement|clone|importer)\b",
    "(?i)\b(?:compatible|compatibility|equivalent|equivalence|parity|replacement|clone|importer)[\s_-]*(?:with|for|to)?[\s_-]*(?:unity|unreal(?:\s+engine)?|godot|gamemaker|defold|o3de)\b"
)

function Test-SafeRepoRelativePath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    if ([string]::IsNullOrWhiteSpace($RelativePath)) {
        return $false
    }
    if ($RelativePath.Contains("\")) {
        return $false
    }
    if ([System.IO.Path]::IsPathRooted($RelativePath)) {
        return $false
    }
    if ($RelativePath -match "^[A-Za-z]:") {
        return $false
    }
    if ($RelativePath.Contains(":")) {
        return $false
    }
    if ($RelativePath -match "(^|/)\.\.(/|$)") {
        return $false
    }
    return $true
}

function Resolve-RepoRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not (Test-SafeRepoRelativePath -RelativePath $RelativePath)) {
        Write-Error "unsafe_relative_path: $Label must be repo-relative without absolute, drive-qualified, colon, backslash, or '..' segments."
    }

    $fullPath = [System.IO.Path]::GetFullPath((Join-Path $root $RelativePath))
    $rootWithSeparator = $root.TrimEnd([System.IO.Path]::DirectorySeparatorChar) +
        [System.IO.Path]::DirectorySeparatorChar
    if (-not $fullPath.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "unsafe_relative_path: $Label must resolve under the repository root."
    }
    return $fullPath
}

function Add-UniquePath {
    param(
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][System.Collections.Generic.List[string]]$Paths,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][System.Collections.Generic.HashSet[string]]$SeenPaths,
        [Parameter(Mandatory = $true)][string]$Path
    )

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    if ($SeenPaths.Add($fullPath)) {
        $Paths.Add($fullPath) | Out-Null
    }
}

function Add-TextFilesUnderRoot {
    param(
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][System.Collections.Generic.List[string]]$Paths,
        [Parameter(Mandatory = $true)][AllowEmptyCollection()][System.Collections.Generic.HashSet[string]]$SeenPaths,
        [Parameter(Mandatory = $true)][string]$RootPath
    )

    if (-not (Test-Path -LiteralPath $RootPath -PathType Container)) {
        return
    }

    Get-ChildItem -LiteralPath $RootPath -Recurse -File |
        Where-Object { ($textExtensions -contains $_.Extension) -or ($textFileNames -contains $_.Name) } |
        ForEach-Object {
            Add-UniquePath -Paths $Paths -SeenPaths $SeenPaths -Path $_.FullName
        }
}

function Get-2DCommercialCleanRoomScanFile {
    $scanFiles = [System.Collections.Generic.List[string]]::new()
    $seenFiles = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)

    if (-not [string]::IsNullOrWhiteSpace($ScanRootRelative)) {
        $scanRoot = Resolve-RepoRelativePath -RelativePath $ScanRootRelative -Label "ScanRootRelative"
        Add-TextFilesUnderRoot -Paths $scanFiles -SeenPaths $seenFiles -RootPath $scanRoot
        return @($scanFiles | Sort-Object)
    }

    $engineRoot = Join-Path $root "engine"
    if (Test-Path -LiteralPath $engineRoot -PathType Container) {
        foreach ($engineModule in Get-ChildItem -LiteralPath $engineRoot -Directory) {
            $includeRoot = Join-Path $engineModule.FullName "include"
            Add-TextFilesUnderRoot -Paths $scanFiles -SeenPaths $seenFiles -RootPath $includeRoot
        }
    }

    foreach ($relativeRoot in @("engine/platform/win32/include", "editor/core/include", "games")) {
        Add-TextFilesUnderRoot -Paths $scanFiles -SeenPaths $seenFiles -RootPath (Join-Path $root $relativeRoot)
    }

    foreach ($relativeFile in @("schemas/game-agent.schema.json", "schemas/engine-agent.schema.json")) {
        $path = Join-Path $root $relativeFile
        if (Test-Path -LiteralPath $path -PathType Leaf) {
            Add-UniquePath -Paths $scanFiles -SeenPaths $seenFiles -Path $path
        }
    }

    return @($scanFiles | Sort-Object)
}

$violations = [System.Collections.Generic.List[string]]::new()

foreach ($filePath in Get-2DCommercialCleanRoomScanFile) {
    $relativePath = Get-RelativeRepoPath -FullPath $filePath -Root $root
    foreach ($pattern in @($forbiddenTokenPatterns)) {
        foreach ($lineMatch in Select-String -LiteralPath $filePath -Pattern $pattern -AllMatches -CaseSensitive -Encoding utf8) {
            foreach ($tokenMatch in @($lineMatch.Matches)) {
                $violations.Add("forbidden_2d_commercial_token=$($tokenMatch.Value) path=$relativePath line=$($lineMatch.LineNumber)") |
                    Out-Null
            }
        }
    }
    foreach ($pattern in @($forbiddenClaimPatterns)) {
        foreach ($lineMatch in Select-String -LiteralPath $filePath -Pattern $pattern -AllMatches -Encoding utf8) {
            foreach ($tokenMatch in @($lineMatch.Matches)) {
                $violations.Add("forbidden_2d_commercial_token=$($tokenMatch.Value) path=$relativePath line=$($lineMatch.LineNumber)") |
                    Out-Null
            }
        }
    }
}

if ($violations.Count -gt 0) {
    foreach ($violation in $violations) {
        Write-Error $violation
    }
}

Write-Information "2d-commercial-clean-room: ok" -InformationAction Continue
