#requires -Version 7.0
#requires -PSEdition Core

param(
    [switch]$Strict
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

if (-not $IsLinux) {
    $message = "coverage-check: blocker - coverage collection is currently enabled only for Linux GCC/Clang CI builds."
    if ($Strict) {
        Write-Error $message
    }
    Write-Host $message
    exit 0
}

$tools = Assert-CppBuildTools
$gcov = Get-Command gcov -ErrorAction SilentlyContinue
if (-not $gcov) {
    $message = "coverage-check: blocker - gcov was not found. Install the official GCC coverage tools for the active compiler."
    if ($Strict) {
        Write-Error $message
    }
    Write-Host $message
    exit 0
}

$thresholdPath = Join-Path $root "tools/coverage-thresholds.json"
if (-not (Test-Path -LiteralPath $thresholdPath)) {
    Write-Error "coverage-check: missing tools/coverage-thresholds.json"
}
$thresholds = Get-Content -LiteralPath $thresholdPath -Raw | ConvertFrom-Json
if ($thresholds.schemaVersion -ne 1) {
    Write-Error "coverage-check: tools/coverage-thresholds.json schemaVersion must be 1"
}
$minLineRequired = [double]$thresholds.minLineCoveragePercent

$buildDir = Join-Path $root "out/build/coverage"
$coveragePreset = "coverage"

Invoke-CheckedCommand $tools.CMake "--preset" $coveragePreset
Invoke-CheckedCommand $tools.CMake "--build" "--preset" $coveragePreset
Invoke-CheckedCommand $tools.CTest "--preset" $coveragePreset "--output-on-failure"
Invoke-CheckedCommand $tools.CTest --test-dir $buildDir -T Coverage --output-on-failure

$lcovCmd = Get-Command lcov -ErrorAction SilentlyContinue
if (-not $lcovCmd) {
    $message = "coverage-check: lcov not found — install the distro lcov package (e.g. apt install lcov) for summary and threshold enforcement."
    if ($Strict) {
        Write-Error $message
    }
    Write-Host $message
    Write-Host "coverage-check: ok (tests only; no lcov summary)"
    exit 0
}

$lcovExe = $lcovCmd.Source
$infoRaw = Join-Path $buildDir "coverage-full.info"
Invoke-CheckedCommand $lcovExe @(
    "--capture",
    "--directory", $buildDir,
    "--output-file", $infoRaw,
    "--ignore-errors", "source,gcov,mismatch"
)
$currentInfo = $infoRaw

$patterns = @()
if ($null -ne $thresholds.lcovRemovePatterns) {
    $patterns = @($thresholds.lcovRemovePatterns)
}
if ($patterns.Count -gt 0) {
    $stageIndex = 0
    foreach ($pattern in $patterns) {
        if ([string]::IsNullOrWhiteSpace([string]$pattern)) {
            continue
        }
        $nextInfo = Join-Path $buildDir ("coverage-filtered-{0}.info" -f $stageIndex)
        Invoke-CheckedCommand $lcovExe @("--remove", $currentInfo, [string]$pattern, "-o", $nextInfo)
        $currentInfo = $nextInfo
        $stageIndex++
    }
}

$summaryProcess = [System.Diagnostics.ProcessStartInfo]::new()
$summaryProcess.FileName = $lcovExe
$summaryProcess.WorkingDirectory = (Get-Location).Path
$summaryProcess.UseShellExecute = $false
$summaryProcess.RedirectStandardOutput = $true
$summaryProcess.RedirectStandardError = $true
$summaryProcess.ArgumentList.Add("--summary") | Out-Null
$summaryProcess.ArgumentList.Add($currentInfo) | Out-Null
foreach ($entry in Get-NormalizedProcessEnvironment) {
    $summaryProcess.Environment[$entry.Key] = $entry.Value
}
$p = [System.Diagnostics.Process]::Start($summaryProcess)
$summaryText = $p.StandardOutput.ReadToEnd() + $p.StandardError.ReadToEnd()
$p.WaitForExit()
Write-Host $summaryText

if ($p.ExitCode -ne 0) {
    Write-Error "coverage-check: lcov --summary failed with exit code $($p.ExitCode)"
}

# lcov prints "lines......: NN.N% (...)" (ASCII dots). Accept any run of dots after "lines".
$match = [regex]::Match($summaryText, '(?m)lines\.+:\s+([\d.]+)%')
if (-not $match.Success) {
    if ($Strict) {
        Write-Error "coverage-check: could not parse line coverage percentage from lcov --summary output."
    }
    Write-Host "coverage-check: warning - unparsed lcov summary (non-strict)."
    Write-Host "coverage-check: ok"
    exit 0
}

$pct = [double]$match.Groups[1].Value
Write-Host ("coverage-check: line coverage {0}% (minimum {1}% from tools/coverage-thresholds.json)" -f $pct, $minLineRequired)

if ($pct + 1e-9 -lt $minLineRequired) {
    $msg = ("coverage-check: line coverage {0}% is below minimum {1}%" -f $pct, $minLineRequired)
    if ($Strict) {
        Write-Error $msg
    }
    Write-Host "coverage-check: warning - $msg (non-strict mode continues)"
}

Write-Host "coverage-check: ok"
