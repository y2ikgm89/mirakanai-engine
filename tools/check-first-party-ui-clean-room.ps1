#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

$forbiddenPublicTokens = @(
    "UXML",
    "USS",
    "UMG",
    "Slate",
    "WidgetBlueprint",
    "UnityEngine",
    "UnityEditor",
    "Godot",
    "CanvasLayer",
    "ControlNode",
    "UnrealEd",
    "BlueprintGraph",
    "DearImGui",
    "ImGui",
    "RmlUi",
    "Noesis",
    "Slint",
    "Qt"
)

$headerExtensions = @(".h", ".hpp", ".hh", ".hxx", ".ixx")
$gameExtensions = @(".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hh", ".hxx", ".ixx", ".json", ".cmake")
$gameFileNames = @("CMakeLists.txt")

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

function Get-FirstPartyUiCleanRoomScanFile {
    $scanFiles = [System.Collections.Generic.List[string]]::new()
    $seenFiles = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $seenRoots = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)

    $engineRoot = Join-Path $root "engine"
    if (Test-Path -LiteralPath $engineRoot -PathType Container) {
        foreach ($engineModule in Get-ChildItem -LiteralPath $engineRoot -Directory) {
            $includeRoot = Join-Path $engineModule.FullName "include"
            if (Test-Path -LiteralPath $includeRoot -PathType Container) {
                $fullIncludeRoot = [System.IO.Path]::GetFullPath($includeRoot)
                if ($seenRoots.Add($fullIncludeRoot)) {
                    Get-ChildItem -LiteralPath $fullIncludeRoot -Recurse -File |
                        Where-Object { $headerExtensions -contains $_.Extension } |
                        ForEach-Object {
                            Add-UniquePath -Paths $scanFiles -SeenPaths $seenFiles -Path $_.FullName
                        }
                }
            }
        }
    }

    foreach ($relativeRoot in @("engine/platform/win32/include", "editor/core/include")) {
        $absoluteRoot = Join-Path $root $relativeRoot
        if (-not (Test-Path -LiteralPath $absoluteRoot -PathType Container)) {
            continue
        }
        $fullRoot = [System.IO.Path]::GetFullPath($absoluteRoot)
        if (-not $seenRoots.Add($fullRoot)) {
            continue
        }
        Get-ChildItem -LiteralPath $fullRoot -Recurse -File |
            Where-Object { $headerExtensions -contains $_.Extension } |
            ForEach-Object {
                Add-UniquePath -Paths $scanFiles -SeenPaths $seenFiles -Path $_.FullName
            }
    }

    $gamesRoot = Join-Path $root "games"
    if (Test-Path -LiteralPath $gamesRoot -PathType Container) {
        Get-ChildItem -LiteralPath $gamesRoot -Recurse -File |
            Where-Object {
                ($gameExtensions -contains $_.Extension) -or ($gameFileNames -contains $_.Name)
            } |
            ForEach-Object {
                Add-UniquePath -Paths $scanFiles -SeenPaths $seenFiles -Path $_.FullName
            }
    }

    return @($scanFiles | Sort-Object)
}

$violations = [System.Collections.Generic.List[string]]::new()
$escapedForbiddenPublicTokens = @($forbiddenPublicTokens | ForEach-Object { [regex]::Escape($_) })
$forbiddenPublicTokenPattern = "(?<![A-Za-z0-9_])(?:$($escapedForbiddenPublicTokens -join '|'))(?![A-Za-z0-9_])"

foreach ($filePath in Get-FirstPartyUiCleanRoomScanFile) {
    $relativePath = Get-RelativeRepoPath -FullPath $filePath -Root $root
    foreach ($lineMatch in Select-String -LiteralPath $filePath -Pattern $forbiddenPublicTokenPattern -AllMatches -CaseSensitive -Encoding utf8) {
        foreach ($tokenMatch in @($lineMatch.Matches)) {
            $violations.Add("forbidden_token=$($tokenMatch.Value) path=$relativePath line=$($lineMatch.LineNumber)") | Out-Null
        }
    }
}

if ($violations.Count -gt 0) {
    foreach ($violation in $violations) {
        Write-Error $violation
    }
}

Write-Information "first-party-ui-clean-room: ok" -InformationAction Continue
