#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [ValidateRange(0, 64)]
    [int]$StaticJobs = 0,

    [ValidateRange(1, 86400)]
    [int]$StaticCheckTimeoutSeconds = 1800,

    [switch]$StaticOnly,

    [switch]$SkipStaticChecks,

    [switch]$SkipTidySmoke
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

function Write-ValidateInformation {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string]$Message
    )

    Write-Information $Message -InformationAction Continue
}

function Get-ValidateTask {
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
    Write-ValidateInformation "validate: running $ScriptFileName$displayArguments"
    & (Join-Path $PSScriptRoot $ScriptFileName) @Arguments
}

function Invoke-ValidateBackgroundJob {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string]$Name,

        [Parameter(Mandatory = $true)]
        [scriptblock]$ScriptBlock,

        [object[]]$ArgumentList = @()
    )

    if (Get-Command Start-Job -ErrorAction SilentlyContinue) {
        return Start-Job -Name $Name -ScriptBlock $ScriptBlock -ArgumentList $ArgumentList
    }

    return Start-ThreadJob -Name $Name -ScriptBlock $ScriptBlock -ArgumentList $ArgumentList
}

function Invoke-ValidateToolScriptBatch {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [object[]]$Tasks,

        [Parameter(Mandatory = $true)]
        [ValidateRange(1, 64)]
        [int]$Jobs,

        [Parameter(Mandatory = $true)]
        [ValidateRange(1, 86400)]
        [int]$TaskTimeoutSeconds
    )

    if ($Tasks.Count -eq 0) {
        return
    }

    $effectiveJobs = [Math]::Min($Jobs, $Tasks.Count)
    Write-ValidateInformation "validate: running $($Tasks.Count) independent static checks with $effectiveJobs parallel jobs; per-check timeout ${TaskTimeoutSeconds}s"

    $validateOutputFirstLineCount = 200
    $validateOutputTailLineCount = 200
    $validateJobPollSeconds = 2
    $pwshPath = (Get-Process -Id $PID).Path
    if ([string]::IsNullOrWhiteSpace($pwshPath)) {
        $pwshCommand = Get-Command pwsh -ErrorAction SilentlyContinue | Select-Object -First 1
        if (-not $pwshCommand) {
            Write-Error "validate: could not resolve pwsh for parallel static checks"
        }
        $pwshPath = $pwshCommand.Source
    }

    $repositoryRoot = Get-RepoRoot
    $validationLogRoot = Join-Path $repositoryRoot (Join-Path "out" (Join-Path "validation-logs" ("validate-{0:yyyyMMdd-HHmmss}-{1}" -f (Get-Date), $PID)))
    $null = New-Item -ItemType Directory -Path $validationLogRoot -Force
    Write-ValidateInformation "validate: parallel static check logs: $validationLogRoot"

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
            [Parameter(Mandatory = $true)][string]$OutputLogPath,
            [Parameter(Mandatory = $true)][ValidateRange(0, 10000)][int]$FirstLineCount,
            [Parameter(Mandatory = $true)][ValidateRange(0, 10000)][int]$TailLineCount,
            [Parameter(Mandatory = $true)][ValidateRange(1, 86400)][int]$TimeoutSeconds,
            [string[]]$ChildArguments = @()
        )

        function Get-ValidateOutputCapture {
            [CmdletBinding()]
            param(
                [Parameter(Mandatory = $true)][ValidateRange(0, 10000)][int]$FirstLineCount,
                [Parameter(Mandatory = $true)][ValidateRange(0, 10000)][int]$TailLineCount
            )

            return [pscustomobject]@{
                FirstLines = [System.Collections.Generic.List[string]]::new()
                TailLines = [System.Collections.Generic.Queue[string]]::new()
                TotalLineCount = 0
                FirstLineCount = $FirstLineCount
                TailLineCount = $TailLineCount
            }
        }

        function Add-ValidateOutputLine {
            [CmdletBinding()]
            param(
                [Parameter(Mandatory = $true)]$Capture,
                [Parameter(Mandatory = $true)][string]$Line
            )

            $Capture.TotalLineCount += 1
            if ($Capture.FirstLines.Count -lt $Capture.FirstLineCount) {
                $Capture.FirstLines.Add($Line) | Out-Null
                return
            }

            if ($Capture.TailLineCount -le 0) {
                return
            }

            $Capture.TailLines.Enqueue($Line)
            while ($Capture.TailLines.Count -gt $Capture.TailLineCount) {
                $null = $Capture.TailLines.Dequeue()
            }
        }

        function Write-ValidateCapturedLine {
            [CmdletBinding()]
            param(
                [Parameter(Mandatory = $true)]$Writer,
                [Parameter(Mandatory = $true)]$Capture,
                [Parameter(Mandatory = $true)][string]$Line
            )

            $Writer.WriteLine($Line)
            Add-ValidateOutputLine -Capture $Capture -Line $Line
        }

        function Add-ValidateOutputText {
            [CmdletBinding()]
            param(
                [AllowEmptyString()]
                [string]$Text,

                [Parameter(Mandatory = $true)]$Writer,
                [Parameter(Mandatory = $true)]$Capture
            )

            if ($null -eq $Text) {
                return
            }

            $normalizedText = $Text -replace "`r`n", "`n" -replace "`r", "`n"
            $lines = $normalizedText -split "`n"
            for ($lineIndex = 0; $lineIndex -lt $lines.Count; ++$lineIndex) {
                $line = $lines[$lineIndex]
                if ([string]::IsNullOrEmpty($line) -and $lineIndex -eq ($lines.Count - 1)) {
                    continue
                }
                Write-ValidateCapturedLine -Writer $Writer -Capture $Capture -Line $line
            }
        }

        function Get-ValidateOutputSnapshot {
            [CmdletBinding()]
            param(
                [Parameter(Mandatory = $true)]$Capture,
                [Parameter(Mandatory = $true)][string]$OutputLogPath
            )

            $capturedLines = [System.Collections.Generic.List[string]]::new()
            foreach ($line in $Capture.FirstLines) {
                $capturedLines.Add($line) | Out-Null
            }

            $omittedOutputLineCount = [Math]::Max(0, $Capture.TotalLineCount - $Capture.FirstLines.Count - $Capture.TailLines.Count)
            if ($omittedOutputLineCount -gt 0) {
                $capturedLines.Add("validate: omitted $omittedOutputLineCount output line(s); see $OutputLogPath") | Out-Null
            }

            foreach ($line in $Capture.TailLines) {
                $capturedLines.Add($line) | Out-Null
            }

            return [pscustomobject]@{
                Output = [string[]]$capturedLines.ToArray()
                OutputLineCount = $Capture.TotalLineCount
                OmittedOutputLineCount = $omittedOutputLineCount
            }
        }

        Set-Location -LiteralPath $RepositoryRoot
        $outputLogDirectory = Split-Path -Path $OutputLogPath -Parent
        if (-not [string]::IsNullOrWhiteSpace($outputLogDirectory)) {
            $null = New-Item -ItemType Directory -Path $outputLogDirectory -Force
        }

        $validateProcessExitDrainMilliseconds = 2000
        $validateStreamDrainMilliseconds = 2000
        $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
        $capture = Get-ValidateOutputCapture -FirstLineCount $FirstLineCount -TailLineCount $TailLineCount
        $utf8NoBom = [System.Text.UTF8Encoding]::new($false)
        $writer = [System.IO.StreamWriter]::new($OutputLogPath, $false, $utf8NoBom)
        $process = $null
        try {
            $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
            $startInfo.FileName = $PwshPath
            $startInfo.WorkingDirectory = $RepositoryRoot
            $startInfo.UseShellExecute = $false
            $startInfo.RedirectStandardInput = $true
            $startInfo.RedirectStandardOutput = $true
            $startInfo.RedirectStandardError = $true
            $startInfo.CreateNoWindow = $true
            foreach ($argument in @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $ScriptPath)) {
                $startInfo.ArgumentList.Add($argument)
            }
            foreach ($argument in @($ChildArguments)) {
                $startInfo.ArgumentList.Add($argument)
            }
            if ($ScriptFileName -eq "check-mobile-packaging.ps1" -and @($ChildArguments) -notcontains "-RequireAndroid") {
                $startInfo.Environment["MK_MOBILE_DEVICE_PROBE"] = "0"
            }

            $process = [System.Diagnostics.Process]::new()
            $process.StartInfo = $startInfo

            $null = $process.Start()
            $process.StandardInput.Close()
            $standardOutputTask = $process.StandardOutput.ReadToEndAsync()
            $standardErrorTask = $process.StandardError.ReadToEndAsync()

            $timedOut = $false
            while (-not $process.WaitForExit(250)) {
                if ($stopwatch.Elapsed.TotalSeconds -gt $TimeoutSeconds) {
                    $timedOut = $true
                    Write-ValidateCapturedLine `
                        -Writer $writer `
                        -Capture $capture `
                        -Line "validate: timed out after ${TimeoutSeconds}s while running $ScriptFileName"
                    try {
                        $process.Kill($true)
                    }
                    catch {
                        $process.Kill()
                    }
                    break
                }
            }

            try {
                if (-not $process.WaitForExit($validateProcessExitDrainMilliseconds)) {
                    Write-ValidateCapturedLine `
                        -Writer $writer `
                        -Capture $capture `
                        -Line "validate: process did not exit within ${validateProcessExitDrainMilliseconds}ms after wait/kill for $ScriptFileName"
                }
            }
            catch {
                Write-ValidateCapturedLine `
                    -Writer $writer `
                    -Capture $capture `
                    -Line "validate: failed while waiting for $ScriptFileName to exit after cancellation: $_"
            }

            try {
                if ($standardOutputTask.Wait($validateStreamDrainMilliseconds)) {
                    Add-ValidateOutputText -Text $standardOutputTask.GetAwaiter().GetResult() -Writer $writer -Capture $capture
                }
                else {
                    Write-ValidateCapturedLine `
                        -Writer $writer `
                        -Capture $capture `
                        -Line "validate: stdout stream capture did not finish within ${validateStreamDrainMilliseconds}ms for $ScriptFileName"
                }
            }
            catch {
                Write-ValidateCapturedLine -Writer $writer -Capture $capture -Line "validate: stdout capture failed for ${ScriptFileName}: $_"
            }

            try {
                if ($standardErrorTask.Wait($validateStreamDrainMilliseconds)) {
                    Add-ValidateOutputText -Text $standardErrorTask.GetAwaiter().GetResult() -Writer $writer -Capture $capture
                }
                else {
                    Write-ValidateCapturedLine `
                        -Writer $writer `
                        -Capture $capture `
                        -Line "validate: stderr stream capture did not finish within ${validateStreamDrainMilliseconds}ms for $ScriptFileName"
                }
            }
            catch {
                Write-ValidateCapturedLine -Writer $writer -Capture $capture -Line "validate: stderr capture failed for ${ScriptFileName}: $_"
            }

            if ($timedOut) {
                $exitCode = 124
            }
            elseif ($process.HasExited) {
                $exitCode = $process.ExitCode
            }
            else {
                $exitCode = 1
                Write-ValidateCapturedLine -Writer $writer -Capture $capture -Line "validate: process remained active without timeout status for $ScriptFileName"
            }
        }
        catch {
            $line = [string]$_
            Write-ValidateCapturedLine -Writer $writer -Capture $capture -Line $line
            $exitCode = 1
        }
        finally {
            if ($null -ne $process) {
                $process.Dispose()
            }
            $writer.Dispose()
            $stopwatch.Stop()
        }

        $outputSnapshot = Get-ValidateOutputSnapshot -Capture $capture -OutputLogPath $OutputLogPath

        [pscustomobject]@{
            ScriptFileName = $ScriptFileName
            Arguments = @($ChildArguments)
            ExitCode = $exitCode
            ElapsedSeconds = [Math]::Round($stopwatch.Elapsed.TotalSeconds, 2)
            Output = [string[]]$outputSnapshot.Output
            OutputLineCount = $outputSnapshot.OutputLineCount
            OmittedOutputLineCount = $outputSnapshot.OmittedOutputLineCount
            OutputLogPath = $OutputLogPath
        }
    }

    while ($pending.Count -gt 0 -or $running.Count -gt 0) {
        while ($pending.Count -gt 0 -and $running.Count -lt $effectiveJobs) {
            $entry = $pending.Dequeue()
            $task = $entry.Task
            $displayArguments = if ($task.Arguments.Count -gt 0) { " $($task.Arguments -join ' ')" } else { "" }
            Write-ValidateInformation "validate: starting $($task.ScriptFileName)$displayArguments"
            $scriptPath = Join-Path $PSScriptRoot $task.ScriptFileName
            $safeScriptFileName = $task.ScriptFileName -replace "[^A-Za-z0-9._-]", "_"
            $outputLogPath = Join-Path $validationLogRoot ("{0:D2}-{1}.log" -f ($entry.Order + 1), $safeScriptFileName)
            $job = Invoke-ValidateBackgroundJob `
                -Name "validate-$($entry.Order)-$($task.ScriptFileName)" `
                -ScriptBlock $jobScript `
                -ArgumentList @(
                    $pwshPath,
                    $repositoryRoot,
                    $scriptPath,
                    $task.ScriptFileName,
                    $outputLogPath,
                    $validateOutputFirstLineCount,
                    $validateOutputTailLineCount,
                    $TaskTimeoutSeconds,
                    [string[]]$task.Arguments
                )
            $running.Add([pscustomobject]@{
                Order = $entry.Order
                Task = $task
                Job = $job
                StartedAtUtc = [DateTimeOffset]::UtcNow
                OutputLogPath = $outputLogPath
            }) | Out-Null
        }

        $finishedJob = Wait-Job -Job @($running | ForEach-Object { $_.Job }) -Any -Timeout $validateJobPollSeconds
        if (-not $finishedJob) {
            $nowUtc = [DateTimeOffset]::UtcNow
            foreach ($timedOutRecord in @($running | Where-Object {
                        ($nowUtc - $_.StartedAtUtc).TotalSeconds -ge $TaskTimeoutSeconds
                    })) {
                $elapsedSeconds = [Math]::Round(($nowUtc - $timedOutRecord.StartedAtUtc).TotalSeconds, 2)
                Stop-Job -Job $timedOutRecord.Job -ErrorAction SilentlyContinue
                Remove-Job -Job $timedOutRecord.Job -Force -ErrorAction SilentlyContinue
                $resultsByOrder[$timedOutRecord.Order] = [pscustomobject]@{
                    ScriptFileName = $timedOutRecord.Task.ScriptFileName
                    Arguments = @($timedOutRecord.Task.Arguments)
                    ExitCode = 124
                    ElapsedSeconds = $elapsedSeconds
                    Output = @(
                        "validate: timed out after ${TaskTimeoutSeconds}s while running $($timedOutRecord.Task.ScriptFileName)",
                        "validate: partial log: $($timedOutRecord.OutputLogPath)"
                    )
                    OutputLineCount = 2
                    OmittedOutputLineCount = 0
                    OutputLogPath = $timedOutRecord.OutputLogPath
                }
                $running.Remove($timedOutRecord) | Out-Null
            }
            continue
        }
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
                        OutputLineCount = $received.Count
                        OmittedOutputLineCount = 0
                        OutputLogPath = $record.OutputLogPath
                    })
            }
            if ($result[0].ExitCode -ne 124 -and $result[0].ElapsedSeconds -gt $TaskTimeoutSeconds) {
                $timeoutOutput = @(
                    "validate: timed out after ${TaskTimeoutSeconds}s while running $($record.Task.ScriptFileName)",
                    "validate: completed after timeout; full log: $($record.OutputLogPath)"
                ) + @($result[0].Output)
                $result = @([pscustomobject]@{
                        ScriptFileName = $record.Task.ScriptFileName
                        Arguments = @($record.Task.Arguments)
                        ExitCode = 124
                        ElapsedSeconds = $result[0].ElapsedSeconds
                        Output = [string[]]$timeoutOutput
                        OutputLineCount = $result[0].OutputLineCount
                        OmittedOutputLineCount = $result[0].OmittedOutputLineCount
                        OutputLogPath = $record.OutputLogPath
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
        Write-ValidateInformation "validate: output from $($result.ScriptFileName) (log: $($result.OutputLogPath); lines: $($result.OutputLineCount); omitted: $($result.OmittedOutputLineCount))"
        foreach ($line in @($result.Output)) {
            Write-ValidateInformation $line
        }
        Write-ValidateInformation "validate: finished $($result.ScriptFileName) in $($result.ElapsedSeconds)s"
        if ($result.ExitCode -ne 0) {
            $failedScripts.Add($result.ScriptFileName) | Out-Null
        }
    }

    if ($failedScripts.Count -gt 0) {
        Write-Error "validate: $($failedScripts.Count) static check(s) failed: $($failedScripts -join ', ')"
    }
}

Invoke-ValidateToolScript -ScriptFileName "check-toolchain.ps1"

$staticTasks = @(
    Get-ValidateTask -ScriptFileName "check-license.ps1"
    Get-ValidateTask -ScriptFileName "check-agents.ps1"
    Get-ValidateTask -ScriptFileName "check-json-contracts.ps1"
    Get-ValidateTask -ScriptFileName "check-validation-recipe-runner.ps1"
    Get-ValidateTask -ScriptFileName "check-installed-sdk-validation.ps1"
    Get-ValidateTask -ScriptFileName "check-release-package-artifacts.ps1"
    Get-ValidateTask -ScriptFileName "check-ai-integration.ps1"
    Get-ValidateTask -ScriptFileName "check-production-readiness-audit.ps1"
    Get-ValidateTask -ScriptFileName "check-linux-vulkan-host-gate-closeout.ps1"
    Get-ValidateTask -ScriptFileName "check-metal-apple-host-gate-closeout.ps1"
    Get-ValidateTask -ScriptFileName "check-cpu-profiling-matrix-host-gate-closeout.ps1"
    Get-ValidateTask -ScriptFileName "check-cpu-profiling-host-evidence-collector.ps1"
    Get-ValidateTask -ScriptFileName "check-renderer-metal-memory-profiling-host-evidence-collector.ps1"
    Get-ValidateTask -ScriptFileName "check-renderer-d3d12-commercial-quality-host-evidence.ps1"
    Get-ValidateTask -ScriptFileName "check-renderer-d3d12-commercial-quality-artifact.ps1"
    Get-ValidateTask -ScriptFileName "check-renderer-vulkan-strict-commercial-quality-artifact.ps1"
    Get-ValidateTask -ScriptFileName "check-renderer-apple-metal-commercial-quality-artifact.ps1"
    Get-ValidateTask -ScriptFileName "check-renderer-package-commercial-quality-artifacts.ps1"
    Get-ValidateTask -ScriptFileName "check-renderer-quality-vfx-commercial-artifacts.ps1"
    Get-ValidateTask -ScriptFileName "check-renderer-clean-room-legal-artifact.ps1"
    Get-ValidateTask -ScriptFileName "check-renderer-commercial-readiness-evidence-collector.ps1"
    Get-ValidateTask -ScriptFileName "check-renderer-commercial-readiness-evidence-fixture-guard.ps1"
    Get-ValidateTask -ScriptFileName "check-renderer-commercial-readiness-evidence-metal-memory.ps1"
    Get-ValidateTask -ScriptFileName "check-renderer-commercial-readiness-final-promotion-preflight.ps1"
    Get-ValidateTask -ScriptFileName "check-renderer-commercial-readiness-final-retained-root-assembler.ps1"
    Get-ValidateTask -ScriptFileName "check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1"
    # renderer-metal-memory-profiling-apple-host-artifacts-v1 fail-closed host-gated check.
    Get-ValidateTask -ScriptFileName "generate-renderer-metal-memory-profiling-host-artifacts.ps1"
    Get-ValidateTask -ScriptFileName "check-optional-gpu-compute-review-collector.ps1"
    Get-ValidateTask -ScriptFileName "check-optional-gpu-compute-review-host-gate-closeout.ps1"
    Get-ValidateTask -ScriptFileName "check-android-vulkan-validation-layer-helper.ps1"
    Get-ValidateTask -ScriptFileName "check-android-gameactivity-host-validator.ps1"
    Get-ValidateTask -ScriptFileName "check-ci-matrix.ps1"
    Get-ValidateTask -ScriptFileName "check-dependency-policy.ps1"
    Get-ValidateTask -ScriptFileName "check-vcpkg-environment.ps1"
    Get-ValidateTask -ScriptFileName "check-text-format-contract.ps1"
    Get-ValidateTask -ScriptFileName "check-format.ps1"
    Get-ValidateTask -ScriptFileName "check-cpp-standard-policy.ps1"
    Get-ValidateTask -ScriptFileName "check-coverage-thresholds.ps1"
    Get-ValidateTask -ScriptFileName "check-shader-toolchain.ps1"
    Get-ValidateTask -ScriptFileName "check-apple-host-evidence.ps1"
    Get-ValidateTask -ScriptFileName "check-native-desktop-contracts.ps1"
    Get-ValidateTask -ScriptFileName "check-public-api-boundaries.ps1"
)

if (-not $SkipStaticChecks.IsPresent) {
    Invoke-ValidateToolScriptBatch `
        -Tasks $staticTasks `
        -Jobs (Resolve-ValidateStaticJobCount -Jobs $StaticJobs) `
        -TaskTimeoutSeconds $StaticCheckTimeoutSeconds
    Invoke-ValidateToolScript -ScriptFileName "check-mobile-packaging.ps1"
}

if ($StaticOnly.IsPresent) {
    Write-ValidateInformation "validate: static ok"
    exit 0
}

foreach ($scriptFileName in @(
        "build.ps1",
        "check-generated-msvc-cxx23-mode.ps1"
    )) {
    Invoke-ValidateToolScript -ScriptFileName $scriptFileName
}

if ($SkipTidySmoke.IsPresent) {
    Write-ValidateInformation "validate: skipping check-tidy.ps1 smoke"
}
else {
    Write-ValidateInformation "validate: running check-tidy.ps1 -MaxFiles 1 -ReuseExistingFileApiReply"
    & (Join-Path $PSScriptRoot "check-tidy.ps1") -MaxFiles 1 -ReuseExistingFileApiReply
}
Write-ValidateInformation "validate: running test.ps1 -SkipBuild"
& (Join-Path $PSScriptRoot "test.ps1") -SkipBuild

Write-ValidateInformation "validate: ok"
exit 0
