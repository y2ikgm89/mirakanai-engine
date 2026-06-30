#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$CorpusRoot,

    [Parameter(Mandatory = $true)]
    [string]$OutputRoot,

    [string]$Presets = "",

    [ValidateRange(1, 1000000)]
    [uint64]$RowBudget = 10000,

    [switch]$RequireReady,
    [switch]$WriteCookedOutputs,
    [switch]$CompareExpectedHashes,
    [switch]$CollectPreviewRows,
    [switch]$SkipCorpusCheck
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$pwshCommand = (Get-Command pwsh -ErrorAction Stop).Source
$repoRoot = Get-RepoRoot

function Test-ReviewedRelativePath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return $false
    }
    if ($Path.Contains("://") -or $Path.StartsWith("http://", [System.StringComparison]::OrdinalIgnoreCase) -or
        $Path.StartsWith("https://", [System.StringComparison]::OrdinalIgnoreCase) -or
        $Path.StartsWith("file://", [System.StringComparison]::OrdinalIgnoreCase)) {
        return $false
    }
    if ([System.IO.Path]::IsPathRooted($Path) -or $Path.StartsWith("/") -or $Path.Contains("\") -or $Path.Contains(":")) {
        return $false
    }
    foreach ($segment in $Path.Split("/")) {
        if ([string]::IsNullOrWhiteSpace($segment) -or $segment -eq "." -or $segment -eq "..") {
            return $false
        }
    }
    return $true
}

function Confirm-ReviewedRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Description
    )

    if (-not (Test-ReviewedRelativePath -Path $Path)) {
        Write-Error "$Description must be a reviewed repository-relative path with forward slashes only: $Path"
    }
}

function Join-ReviewedRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Leaf
    )

    Confirm-ReviewedRelativePath -Path $Root -Description "Root path"
    Confirm-ReviewedRelativePath -Path $Leaf -Description "Leaf path"
    if ($Root.EndsWith("/", [System.StringComparison]::Ordinal)) {
        return "$Root$Leaf"
    }
    return "$Root/$Leaf"
}

function Resolve-ReviewedRelativePath {
    param([Parameter(Mandatory = $true)][string]$Path)

    Confirm-ReviewedRelativePath -Path $Path -Description "Corpus runner path"
    return [System.IO.Path]::GetFullPath((Join-Path $repoRoot ($Path.Replace("/", [System.IO.Path]::DirectorySeparatorChar))))
}

function New-Directory {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        $null = New-Item -ItemType Directory -Path $Path -Force
    }
}

function Write-Utf8TextFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][AllowEmptyString()][string]$Text
    )

    $parent = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Directory -Path $parent
    }
    Set-Content -LiteralPath $Path -Value $Text -Encoding utf8NoBOM -NoNewline
}

function ConvertTo-Sha256Text {
    param([Parameter(Mandatory = $true)][string]$Text)

    $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
    $hash = [System.Security.Cryptography.SHA256]::HashData($bytes)
    return "sha256:$([System.Convert]::ToHexString($hash).ToLowerInvariant())"
}

function Get-FileSha256Text {
    param([Parameter(Mandatory = $true)][string]$Path)

    return "sha256:$((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant())"
}

function Get-RunnerExecutable {
    $candidates = @(
        (Join-Path $repoRoot "out/build/dev/Debug/MK_asset_import_regression_runner.exe"),
        (Join-Path $repoRoot "out/build/dev/Release/MK_asset_import_regression_runner.exe"),
        (Join-Path $repoRoot "out/build/dev/RelWithDebInfo/MK_asset_import_regression_runner.exe"),
        (Join-Path $repoRoot "out/build/dev/MK_asset_import_regression_runner.exe"),
        (Join-Path $repoRoot "out/build/dev/MK_asset_import_regression_runner")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return $candidate
        }
    }
    Write-Error "MK_asset_import_regression_runner executable was not found. Run tools/check-asset-import-regression-corpus.ps1 first."
}

function Invoke-RunnerProcess {
    param(
        [Parameter(Mandatory = $true)][string]$RunnerExe,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$StdoutPath,
        [Parameter(Mandatory = $true)][string]$StderrPath
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $RunnerExe
    $startInfo.WorkingDirectory = $repoRoot
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in $Arguments) {
        $startInfo.ArgumentList.Add($argument) | Out-Null
    }

    $process = [System.Diagnostics.Process]::Start($startInfo)
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()

    Write-Utf8TextFile -Path $StdoutPath -Text $stdout
    Write-Utf8TextFile -Path $StderrPath -Text $stderr

    return [pscustomobject]@{
        ExitCode = $process.ExitCode
        Stdout = $stdout
        Stderr = $stderr
        Lines = @((ConvertTo-LfText $stdout).Split("`n") | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    }
}

$corpusRootRelative = $CorpusRoot.Replace("\", "/")
$outputRootRelative = $OutputRoot.Replace("\", "/")
Confirm-ReviewedRelativePath -Path $corpusRootRelative -Description "CorpusRoot"
Confirm-ReviewedRelativePath -Path $outputRootRelative -Description "OutputRoot"
if (-not [string]::IsNullOrWhiteSpace($Presets)) {
    $Presets = $Presets.Replace("\", "/")
    Confirm-ReviewedRelativePath -Path $Presets -Description "Presets"
}

if (-not $SkipCorpusCheck) {
    $checkArguments = @("-CorpusRoot", $corpusRootRelative)
    $checkCommandArguments = @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        (Join-Path $PSScriptRoot "check-asset-import-regression-corpus.ps1")
    ) + $checkArguments
    Invoke-CheckedCommand -FilePath $pwshCommand -Arguments $checkCommandArguments
}

$runnerExe = Get-RunnerExecutable
$reportRelative = Join-ReviewedRelativePath -Root $corpusRootRelative -Leaf "report.gereport"
$retainedRunRelative = Join-ReviewedRelativePath -Root $corpusRootRelative -Leaf "retained/runner-$PID"
$freshReportRelative = Join-ReviewedRelativePath -Root $retainedRunRelative -Leaf "fresh-process-report.gereport"
$retainedRunPath = Resolve-ReviewedRelativePath -Path $retainedRunRelative
New-Directory -Path $retainedRunPath

$runnerArguments = [System.Collections.Generic.List[string]]::new()
foreach ($argument in @(
        "--corpus-root", $corpusRootRelative,
        "--output-root", $outputRootRelative,
        "--write-report", $reportRelative,
        "--row-budget", ([string]$RowBudget)
    )) {
    $runnerArguments.Add($argument) | Out-Null
}
if (-not [string]::IsNullOrWhiteSpace($Presets)) {
    $runnerArguments.Add("--presets") | Out-Null
    $runnerArguments.Add($Presets) | Out-Null
}
if ($WriteCookedOutputs) {
    $runnerArguments.Add("--write-cooked-outputs") | Out-Null
}
if ($CompareExpectedHashes) {
    $runnerArguments.Add("--compare-expected-hashes") | Out-Null
}
if ($CollectPreviewRows) {
    $runnerArguments.Add("--collect-preview-rows") | Out-Null
}

$firstStdoutPath = Join-Path $retainedRunPath "runner-stdout.txt"
$firstStderrPath = Join-Path $retainedRunPath "runner-stderr.txt"
$first = Invoke-RunnerProcess `
    -RunnerExe $runnerExe `
    -Arguments ([string[]]$runnerArguments) `
    -StdoutPath $firstStdoutPath `
    -StderrPath $firstStderrPath
if ($first.ExitCode -ne 0) {
    Write-Error "asset import regression runner failed with exit code $($first.ExitCode): $($first.Stderr)"
}

$freshArguments = [System.Collections.Generic.List[string]]::new()
foreach ($argument in $runnerArguments) {
    $freshArguments.Add($argument) | Out-Null
}
$writeReportIndex = $freshArguments.IndexOf("--write-report")
if ($writeReportIndex -lt 0 -or $writeReportIndex + 1 -ge $freshArguments.Count) {
    Write-Error "Internal error: runner arguments are missing --write-report."
}
$freshArguments[$writeReportIndex + 1] = $freshReportRelative

$freshStdoutPath = Join-Path $retainedRunPath "fresh-runner-stdout.txt"
$freshStderrPath = Join-Path $retainedRunPath "fresh-runner-stderr.txt"
$fresh = Invoke-RunnerProcess `
    -RunnerExe $runnerExe `
    -Arguments ([string[]]$freshArguments) `
    -StdoutPath $freshStdoutPath `
    -StderrPath $freshStderrPath
if ($fresh.ExitCode -ne 0) {
    Write-Error "asset import regression fresh-process runner failed with exit code $($fresh.ExitCode): $($fresh.Stderr)"
}

$reportPath = Resolve-ReviewedRelativePath -Path $reportRelative
$freshReportPath = Resolve-ReviewedRelativePath -Path $freshReportRelative
$reportText = ConvertTo-LfText (Get-Content -LiteralPath $reportPath -Encoding utf8 -Raw)
$freshReportText = ConvertTo-LfText (Get-Content -LiteralPath $freshReportPath -Encoding utf8 -Raw)
if ($reportText -ne $freshReportText) {
    Write-Error "asset import regression fresh-process report mismatch: $reportRelative vs $freshReportRelative"
}

$hashRows = [System.Collections.Generic.List[string]]::new()
$hashRows.Add("report=$((ConvertTo-Sha256Text -Text $reportText))") | Out-Null
$hashRows.Add("fresh_report=$((ConvertTo-Sha256Text -Text $freshReportText))") | Out-Null
$hashRows.Add("runner_stdout=$((Get-FileSha256Text -Path $firstStdoutPath))") | Out-Null
$hashRows.Add("runner_stderr=$((Get-FileSha256Text -Path $firstStderrPath))") | Out-Null
$hashRows.Add("fresh_runner_stdout=$((Get-FileSha256Text -Path $freshStdoutPath))") | Out-Null
$hashRows.Add("fresh_runner_stderr=$((Get-FileSha256Text -Path $freshStderrPath))") | Out-Null
foreach ($relative in @(
        (Join-ReviewedRelativePath -Root $corpusRootRelative -Leaf "corpus.gecorpus"),
        (Join-ReviewedRelativePath -Root $corpusRootRelative -Leaf "notices/THIRD_PARTY_ASSET_NOTICES.md")
    )) {
    $path = Resolve-ReviewedRelativePath -Path $relative
    if (Test-Path -LiteralPath $path -PathType Leaf) {
        $hashRows.Add("$relative=$((Get-FileSha256Text -Path $path))") | Out-Null
    }
}
Write-Utf8TextFile -Path (Join-Path $retainedRunPath "retained-hashes.gehashes") -Text (($hashRows -join "`n") + "`n")
Write-Utf8TextFile -Path (Join-Path $retainedRunPath "runner-version.txt") -Text "MK_asset_import_regression_runner`n"

foreach ($line in $first.Lines) {
    Write-Output $line
}
Write-Output "asset_import_regression_fresh_process_match=1"
Write-Output "asset_import_regression_fresh_process_report=$freshReportRelative"
Write-Output "asset_import_regression_retained_run_root=$retainedRunRelative"

$validatorArguments = [System.Collections.Generic.List[string]]::new()
$validatorArguments.Add("-CorpusRoot") | Out-Null
$validatorArguments.Add($corpusRootRelative) | Out-Null
if ($RequireReady) {
    $validatorArguments.Add("-RequireReady") | Out-Null
}
$validatorLines = @(& $pwshCommand `
        -NoProfile `
        -ExecutionPolicy Bypass `
        -File (Join-Path $PSScriptRoot "validate-asset-import-regression-corpus.ps1") `
        @validatorArguments 2>&1)
if ($LASTEXITCODE -ne 0) {
    Write-Error "asset import regression final validator failed with exit code $LASTEXITCODE`: $($validatorLines -join "`n")"
}
foreach ($line in $validatorLines) {
    Write-Output $line
}
if ($RequireReady -and -not ($validatorLines -contains "asset_import_regression_corpus_ready=1")) {
    Write-Error "asset import regression RequireReady wrapper run did not produce asset_import_regression_corpus_ready=1."
}
