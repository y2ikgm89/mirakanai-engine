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
    Write-Host "format-check: no C++ files found"
} else {
    # Windows `CreateProcess` has a short command-line limit; passing every file in one argv
    # triggers "The command line is too long." Split into batches (LLVM/clang-format best practice:
    # invoke the tool repeatedly with bounded argv).
    $paths = @($files | ForEach-Object { $_.FullName })
    $batchSize = 40
    $batches = [System.Collections.Generic.List[string[]]]::new()
    for ($i = 0; $i -lt $paths.Length; $i += $batchSize) {
        $end = [Math]::Min($i + $batchSize - 1, $paths.Length - 1)
        $batches.Add([string[]]$paths[$i..$end]) | Out-Null
    }

    $formatJobs = Resolve-ParallelJobCount -Jobs 0 -MaximumJobs 4
    $formatJobs = [Math]::Min($formatJobs, $batches.Count)
    $pending = [System.Collections.Generic.Queue[object]]::new()
    for ($batchIndex = 0; $batchIndex -lt $batches.Count; ++$batchIndex) {
        $pending.Enqueue([pscustomobject]@{
            Index = $batchIndex
            Paths = $batches[$batchIndex]
        })
    }

    $running = [System.Collections.Generic.List[object]]::new()
    $formatScript = {
        param(
            [Parameter(Mandatory = $true)][string]$Format,
            [Parameter(Mandatory = $true)][string[]]$Batch
        )

        & $Format --dry-run --Werror @Batch
        return $LASTEXITCODE
    }

    while ($pending.Count -gt 0 -or $running.Count -gt 0) {
        while ($pending.Count -gt 0 -and $running.Count -lt $formatJobs) {
            $entry = $pending.Dequeue()
            $job = Start-ThreadJob -ScriptBlock $formatScript -ArgumentList $format, $entry.Paths
            $running.Add([pscustomobject]@{
                Index = $entry.Index
                Job = $job
            }) | Out-Null
        }

        $finishedJobs = Wait-Job -Job @($running | ForEach-Object { $_.Job }) -Any
        foreach ($finishedJob in @($finishedJobs)) {
            $record = @($running | Where-Object { $_.Job.Id -eq $finishedJob.Id } | Select-Object -First 1)[0]
            $output = @(Receive-Job -Job $finishedJob)
            $exitCode = @($output | Select-Object -Last 1)[0]
            $diagnostics = @($output | Select-Object -SkipLast 1)
            Remove-Job -Job $finishedJob
            $running.Remove($record) | Out-Null
            foreach ($line in $diagnostics) {
                $line | Out-Host
            }
            if ($exitCode -ne 0) {
                Write-Error "format-check failed: clang-format failed for batch $($record.Index). Run pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1 to apply clang-format."
            }
        }
    }
}

& (Join-Path $PSScriptRoot "check-text-format.ps1")

Write-Host "format-check: ok"
