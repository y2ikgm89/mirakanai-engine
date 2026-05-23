#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$format = Get-ClangFormatCommand
if (-not $format) {
    Write-Error "clang-format is required. Install the official LLVM clang-format package or Visual Studio Build Tools LLVM tools and re-run this command."
}

$root = Get-RepoRoot
$files = Get-CxxSourceFile -Root $root

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
