#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [ValidateRange(0, 64)]
    [int]$StaticJobs = 0,

    [switch]$StaticOnly,

    [switch]$SkipStaticChecks
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

# `tools/check-ai-integration.ps1` runs once below (equivalent to `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`).
# `check-validation-recipe-runner.ps1` exercises DryRun/Execute rejection paths without duplicating that pass.

if ($StaticOnly.IsPresent -and $SkipStaticChecks.IsPresent) {
    Write-Error "validate: -StaticOnly and -SkipStaticChecks cannot be combined"
}

function Resolve-ValidateStaticJobCount {
    [CmdletBinding()]
    param(
        [Parameter()][ValidateRange(0, 64)][int]$Jobs = 0
    )

    if ($Jobs -gt 0) {
        return $Jobs
    }

    return [Math]::Max(1, [Math]::Min(4, [Environment]::ProcessorCount))
}

function New-ValidateTask {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string]$ScriptFileName,

        [string[]]$Arguments = @()
    )

    return [pscustomobject]@{
        ScriptFileName = $ScriptFileName
        Arguments = @($Arguments)
    }
}

function Invoke-ValidateToolScript {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string]$ScriptFileName,

        [string[]]$Arguments = @()
    )

    $displayArguments = if ($Arguments.Count -gt 0) { " $($Arguments -join ' ')" } else { "" }
    Write-Host "validate: running $ScriptFileName$displayArguments"
    & (Join-Path $PSScriptRoot $ScriptFileName) @Arguments
}

function Invoke-ValidateToolScriptBatch {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [object[]]$Tasks,

        [Parameter(Mandatory = $true)]
        [ValidateRange(1, 64)]
        [int]$Jobs
    )

    if ($Tasks.Count -eq 0) {
        return
    }

    if ($Jobs -eq 1 -or $Tasks.Count -eq 1) {
        foreach ($task in $Tasks) {
            Invoke-ValidateToolScript -ScriptFileName $task.ScriptFileName -Arguments $task.Arguments
        }
        return
    }

    $effectiveJobs = [Math]::Min($Jobs, $Tasks.Count)
    Write-Host "validate: running $($Tasks.Count) independent static checks with $effectiveJobs parallel jobs"

    $pwshPath = (Get-Process -Id $PID).Path
    if ([string]::IsNullOrWhiteSpace($pwshPath)) {
        $pwshCommand = Get-Command pwsh -ErrorAction SilentlyContinue | Select-Object -First 1
        if (-not $pwshCommand) {
            Write-Error "validate: could not resolve pwsh for parallel static checks"
        }
        $pwshPath = $pwshCommand.Source
    }

    $repositoryRoot = Get-RepoRoot
    $pending = [System.Collections.Generic.Queue[object]]::new()
    for ($taskIndex = 0; $taskIndex -lt $Tasks.Count; ++$taskIndex) {
        $pending.Enqueue([pscustomobject]@{
            Order = $taskIndex
            Task = $Tasks[$taskIndex]
        })
    }

    $running = [System.Collections.Generic.List[object]]::new()
    $resultsByOrder = @{}
    $jobScript = {
        param(
            [Parameter(Mandatory = $true)][string]$PwshPath,
            [Parameter(Mandatory = $true)][string]$RepositoryRoot,
            [Parameter(Mandatory = $true)][string]$ScriptPath,
            [Parameter(Mandatory = $true)][string]$ScriptFileName,
            [string[]]$ChildArguments = @()
        )

        Set-Location -LiteralPath $RepositoryRoot
        $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
        $output = @(& $PwshPath -NoProfile -ExecutionPolicy Bypass -File $ScriptPath @ChildArguments 2>&1 |
                ForEach-Object { [string]$_ })
        $exitCode = $LASTEXITCODE
        $stopwatch.Stop()

        [pscustomobject]@{
            ScriptFileName = $ScriptFileName
            Arguments = @($ChildArguments)
            ExitCode = $exitCode
            ElapsedSeconds = [Math]::Round($stopwatch.Elapsed.TotalSeconds, 2)
            Output = @($output)
        }
    }

    while ($pending.Count -gt 0 -or $running.Count -gt 0) {
        while ($pending.Count -gt 0 -and $running.Count -lt $effectiveJobs) {
            $entry = $pending.Dequeue()
            $task = $entry.Task
            $displayArguments = if ($task.Arguments.Count -gt 0) { " $($task.Arguments -join ' ')" } else { "" }
            Write-Host "validate: starting $($task.ScriptFileName)$displayArguments"
            $scriptPath = Join-Path $PSScriptRoot $task.ScriptFileName
            $job = Start-Job `
                -Name "validate-$($entry.Order)-$($task.ScriptFileName)" `
                -ScriptBlock $jobScript `
                -ArgumentList @($pwshPath, $repositoryRoot, $scriptPath, $task.ScriptFileName, [string[]]$task.Arguments)
            $running.Add([pscustomobject]@{
                Order = $entry.Order
                Task = $task
                Job = $job
            }) | Out-Null
        }

        $finishedJob = Wait-Job -Job @($running | ForEach-Object { $_.Job }) -Any
        foreach ($completedJob in @($finishedJob)) {
            $record = @($running | Where-Object { $_.Job.Id -eq $completedJob.Id } | Select-Object -First 1)[0]
            $jobErrors = @()
            $received = @(Receive-Job -Job $completedJob -ErrorAction SilentlyContinue -ErrorVariable jobErrors)
            if ($jobErrors.Count -gt 0) {
                $received += @($jobErrors | ForEach-Object { [string]$_ })
            }
            $result = @($received | Where-Object {
                    $_.PSObject.Properties.Name -contains "ExitCode" -and
                    $_.PSObject.Properties.Name -contains "ScriptFileName"
                } | Select-Object -Last 1)
            if ($result.Count -eq 0) {
                $result = @([pscustomobject]@{
                        ScriptFileName = $record.Task.ScriptFileName
                        Arguments = @($record.Task.Arguments)
                        ExitCode = 1
                        ElapsedSeconds = 0
                        Output = @($received | ForEach-Object { [string]$_ })
                    })
            }
            $resultsByOrder[$record.Order] = $result[0]
            Remove-Job -Job $completedJob
            $running.Remove($record) | Out-Null
        }
    }

    $failedScripts = [System.Collections.Generic.List[string]]::new()
    for ($taskIndex = 0; $taskIndex -lt $Tasks.Count; ++$taskIndex) {
        $result = $resultsByOrder[$taskIndex]
        Write-Host "validate: output from $($result.ScriptFileName)"
        foreach ($line in @($result.Output)) {
            $line | Out-Host
        }
        Write-Host "validate: finished $($result.ScriptFileName) in $($result.ElapsedSeconds)s"
        if ($result.ExitCode -ne 0) {
            $failedScripts.Add($result.ScriptFileName) | Out-Null
        }
    }

    if ($failedScripts.Count -gt 0) {
        Write-Error "validate: $($failedScripts.Count) static check(s) failed: $($failedScripts -join ', ')"
    }
}

foreach ($scriptFileName in @(
        "check-license.ps1",
        "check-toolchain.ps1"
    )) {
    Invoke-ValidateToolScript -ScriptFileName $scriptFileName
}

$staticTasks = @(
    New-ValidateTask -ScriptFileName "check-agents.ps1"
    New-ValidateTask -ScriptFileName "check-json-contracts.ps1"
    New-ValidateTask -ScriptFileName "check-validation-recipe-runner.ps1"
    New-ValidateTask -ScriptFileName "check-installed-sdk-validation.ps1"
    New-ValidateTask -ScriptFileName "check-release-package-artifacts.ps1"
    New-ValidateTask -ScriptFileName "check-ai-integration.ps1"
    New-ValidateTask -ScriptFileName "check-production-readiness-audit.ps1"
    New-ValidateTask -ScriptFileName "check-ci-matrix.ps1"
    New-ValidateTask -ScriptFileName "check-dependency-policy.ps1"
    New-ValidateTask -ScriptFileName "check-vcpkg-environment.ps1"
    New-ValidateTask -ScriptFileName "check-text-format-contract.ps1"
    New-ValidateTask -ScriptFileName "check-format.ps1"
    New-ValidateTask -ScriptFileName "check-cpp-standard-policy.ps1"
    New-ValidateTask -ScriptFileName "check-coverage-thresholds.ps1"
    New-ValidateTask -ScriptFileName "check-shader-toolchain.ps1"
    New-ValidateTask -ScriptFileName "check-mobile-packaging.ps1"
    New-ValidateTask -ScriptFileName "check-apple-host-evidence.ps1"
    New-ValidateTask -ScriptFileName "check-public-api-boundaries.ps1"
)

if (-not $SkipStaticChecks.IsPresent) {
    Invoke-ValidateToolScriptBatch -Tasks $staticTasks -Jobs (Resolve-ValidateStaticJobCount -Jobs $StaticJobs)
}

if ($StaticOnly.IsPresent) {
    Write-Host "validate: static ok"
    exit 0
}

foreach ($scriptFileName in @(
        "build.ps1",
        "check-generated-msvc-cxx23-mode.ps1"
    )) {
    Invoke-ValidateToolScript -ScriptFileName $scriptFileName
}

Write-Host "validate: running check-tidy.ps1 -MaxFiles 1 -ReuseExistingFileApiReply"
& (Join-Path $PSScriptRoot "check-tidy.ps1") -MaxFiles 1 -ReuseExistingFileApiReply
Write-Host "validate: running test.ps1 -SkipBuild"
& (Join-Path $PSScriptRoot "test.ps1") -SkipBuild

Write-Host "validate: ok"
exit 0
