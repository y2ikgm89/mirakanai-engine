#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$format = Get-ClangFormatCommand
if (-not $format) {
    Write-Error "clang-format is required. Install the official LLVM clang-format package or Visual Studio Build Tools LLVM tools and re-run this command."
}

$root = Get-RepoRoot
$sourceRoots = @("engine", "editor", "examples", "games", "tests")
$files = foreach ($sourceRoot in $sourceRoots) {
    $path = Join-Path $root $sourceRoot
    if (Test-Path $path) {
        Get-ChildItem $path -Recurse -File -Include *.cpp, *.hpp, *.h
    }
}

if (-not $files) {
    Write-Host "format: no C++ files found"
} else {
    # Same batching rationale as `check-format.ps1` (Windows argv length limit).
    $paths = @($files | ForEach-Object { $_.FullName })
    $batchSize = 40
    for ($i = 0; $i -lt $paths.Length; $i += $batchSize) {
        $end = [Math]::Min($i + $batchSize - 1, $paths.Length - 1)
        $batch = $paths[$i..$end]
        & $format -i @batch
        if ($LASTEXITCODE -ne 0) {
            Write-Error "format failed"
        }
    }
}

& (Join-Path $PSScriptRoot "format-text.ps1")

Write-Host "format: ok"
