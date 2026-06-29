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

$diagnosticPatterns = @(
    @{
        Kind = "external_code_copied"
        Patterns = @(
            "(?i)`"?(?:external_code_copied|external_engine_code_used|external_code_used|copied_external_code)`"?\s*[:=]\s*(?:true|1)",
            "(?i)\b(?:copied|vendored|imported)[\s_-]*(?:unity|unreal(?:\s+engine)?|godot|gamemaker|defold|o3de)[\s_-]*(?:source|code)\b"
        )
    },
    @{
        Kind = "external_assets_copied"
        Patterns = @(
            "(?i)`"?(?:external_assets_copied|external_engine_asset_used|external_asset_used|copied_external_asset)`"?\s*[:=]\s*(?:true|1)",
            "(?i)\b(?:copied|vendored|imported)[\s_-]*(?:unity|unreal(?:\s+engine)?|godot|gamemaker|defold|o3de)[\s_-]*(?:asset|assets|starter[\s_-]*content|marketplace[\s_-]*content)\b"
        )
    },
    @{
        Kind = "copied_documentation_text"
        Patterns = @(
            "(?i)`"?(?:copied_documentation_text|documentation_expression_copied|copied_docs_prose|copied_external_docs)`"?\s*[:=]\s*(?:true|1)",
            "(?i)\b(?:copied|verbatim|pasted)[\s_-]*(?:unity|unreal(?:\s+engine)?|godot|gamemaker|defold|o3de)[\s_-]*(?:documentation|docs|manual)[\s_-]*(?:text|prose|excerpt)\b"
        )
    },
    @{
        Kind = "external_engine_schema_surface"
        Patterns = @(
            "(?i)`"?(?:external_engine_schema_surface|external_engine_schema_import|external_engine_project_import_used)`"?\s*[:=]\s*(?:true|1)",
            "(?<![A-Za-z0-9_])(?:PackedScene|Node2D|CanvasLayer|ControlNode|BlueprintGraph|WidgetBlueprint|UXML|USS)(?![A-Za-z0-9_])",
            "\.(?:unity|uasset|umap|uproject|tscn|tres|godot)(?![A-Za-z0-9_])"
        )
    },
    @{
        Kind = "third_party_trademark_public_surface"
        CaseSensitive = $true
        Patterns = @(
            "(?<![A-Za-z0-9_])Unity(?:Engine|Editor)?(?![A-Za-z0-9_])",
            "(?<![A-Za-z0-9_])Unreal(?:Engine|Ed)?(?![A-Za-z0-9_])",
            "(?<![A-Za-z0-9_])Godot(?![A-Za-z0-9_])",
            "(?<![A-Za-z0-9_])GameMaker(?![A-Za-z0-9_])",
            "(?<![A-Za-z0-9_])Defold(?![A-Za-z0-9_])",
            "(?<![A-Za-z0-9_])O3DE(?![A-Za-z0-9_])"
        )
    },
    @{
        Kind = "missing_notice_record"
        Patterns = @(
            "(?i)`"?(?:missing_notice_record|third_party_notice_missing|notice_record_missing|license_notice_missing)`"?\s*[:=]\s*(?:true|1)"
        )
    },
    @{
        Kind = "unapproved_dependency_source"
        Patterns = @(
            "(?i)`"?(?:unapproved_dependency_source|dependency_source_unapproved|external_dependency_unapproved|license_unapproved)`"?\s*[:=]\s*(?:true|1)"
        )
    },
    @{
        Kind = "external_engine_compatibility_claim"
        Patterns = @(
            "(?i)`"?(?:external_engine_compatibility_claim|external_engine_equivalence_claim|external_engine_parity_claim)`"?\s*[:=]\s*(?:true|1)",
            "(?i)\b(?:unity|unreal(?:\s+engine)?|godot|gamemaker|defold|o3de)[\s_-]*(?:sdk[\s_-]*)?(?:compatible|compatibility|equivalent|equivalence|parity|replacement|clone|importer)\b",
            "(?i)\b(?:compatible|compatibility|equivalent|equivalence|parity|replacement|clone|importer)[\s_-]*(?:with|for|to)?[\s_-]*(?:unity|unreal(?:\s+engine)?|godot|gamemaker|defold|o3de)\b"
        )
    }
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

function Get-2DCommercialSourceDiagnosticScanFile {
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

$diagnostics = [System.Collections.Generic.List[string]]::new()

foreach ($filePath in Get-2DCommercialSourceDiagnosticScanFile) {
    $relativePath = Get-RelativeRepoPath -FullPath $filePath -Root $root
    foreach ($diagnostic in @($diagnosticPatterns)) {
        $caseSensitive = $diagnostic.ContainsKey("CaseSensitive") -and [bool]$diagnostic.CaseSensitive
        foreach ($pattern in @($diagnostic.Patterns)) {
            $selectArgs = @{
                LiteralPath = $filePath
                Pattern = $pattern
                AllMatches = $true
                Encoding = "utf8"
            }
            if ($caseSensitive) {
                $selectArgs["CaseSensitive"] = $true
            }
            foreach ($lineMatch in Select-String @selectArgs) {
                foreach ($tokenMatch in @($lineMatch.Matches)) {
                    $diagnostics.Add(
                        "2d_commercial_source_diagnostic=$($diagnostic.Kind) token=$($tokenMatch.Value) path=$relativePath line=$($lineMatch.LineNumber)"
                    ) | Out-Null
                }
            }
        }
    }
}

if ($diagnostics.Count -gt 0) {
    Write-Error ($diagnostics -join "`n")
}

Write-Output "2d_commercial_source_diagnostics_ready=1"
Write-Output "2d_commercial_source_diagnostics_scope=retained_markers_and_public_surface_tokens"
Write-Output "external_code_copied_marker_rows=0"
Write-Output "external_assets_copied_marker_rows=0"
Write-Output "copied_documentation_text_marker_rows=0"
Write-Output "external_engine_schema_surface_rows=0"
Write-Output "third_party_trademark_public_surface_rows=0"
Write-Output "missing_notice_marker_rows=0"
Write-Output "unapproved_dependency_source_marker_rows=0"
Write-Output "external_engine_compatibility_claim_rows=0"
Write-Output "external_engine_equivalence_claim_rows=0"
Write-Output "external_engine_parity_claim_rows=0"
Write-Information "2d-commercial-source-diagnostics: ok" -InformationAction Continue
