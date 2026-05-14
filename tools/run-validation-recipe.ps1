#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet("DryRun", "Execute")]
    [string]$Mode = "DryRun",

    [Alias("ValidationRecipe")]
    [string]$Recipe,

    [string]$GameTarget,

    [ValidateSet("", "D3D12", "Vulkan")]
    [string]$StrictBackend = "",

    [string[]]$HostGateAcknowledgements = @(),

    [int]$TimeoutSeconds = 0,

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$RemainingArguments = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "validation-recipe-core.ps1")

$root = Get-RepoRoot

function Write-RunnerResult {
    param(
        [Parameter(Mandatory = $true)]$Result,
        [int]$ExitCode = 0
    )

    $Result | ConvertTo-Json -Depth 12
    exit $ExitCode
}

function Get-TextSummary {
    param($Text)

    if ($null -eq $Text) {
        return ""
    }

    $normalized = $Text.Trim()
    $maxLength = 6000
    if ($normalized.Length -le $maxLength) {
        return $normalized
    }

    return $normalized.Substring(0, $maxLength) + [Environment]::NewLine + '... <truncated>'
}

function Invoke-ValidationRecipeCommand {
    param([object]$Entry, [int]$Timeout)

    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = [string]$Entry.command
    $startInfo.WorkingDirectory = $root
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in @($Entry.argv)) {
        $startInfo.ArgumentList.Add([string]$argument) | Out-Null
    }

    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $process = [System.Diagnostics.Process]::Start($startInfo)
    # Start async stream reads before WaitForExit to avoid stdout/stderr pipe deadlocks.
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $timedOut = $false
    if ($Timeout -gt 0) {
        if (-not $process.WaitForExit($Timeout * 1000)) {
            $timedOut = $true
            $process.Kill($true)
            $process.WaitForExit()
        }
    } else {
        $process.WaitForExit()
    }
    $stopwatch.Stop()

    [System.Threading.Tasks.Task]::WaitAll(@($stdoutTask, $stderrTask))
    $stdout = $stdoutTask.Result
    $stderr = $stderrTask.Result
    $exitCode = if ($timedOut) { 124 } else { $process.ExitCode }

    $row = New-Object System.Collections.Specialized.OrderedDictionary
    $row.Add('command', $Entry.command)
    $row.Add('argv', @($Entry.argv))
    $row.Add('exitCode', $exitCode)
    $row.Add('durationSeconds', [Math]::Round($stopwatch.Elapsed.TotalSeconds, 3))
    $row.Add('stdoutSummary', (Get-TextSummary -Text $stdout))
    $row.Add('stderrSummary', (Get-TextSummary -Text $stderr))
    $row.Add('timedOut', $timedOut)
    return $row
}

function Invoke-ValidationRecipeCommandPlan {
    param(
        [Parameter(Mandatory = $true)]
        [object]$Plan,
        [Parameter(Mandatory = $true)]
        [int]$Timeout
    )

    $results = New-Object System.Collections.ArrayList
    $stdoutParts = New-Object System.Collections.ArrayList
    $stderrParts = New-Object System.Collections.ArrayList
    $totalDuration = 0.0
    $exitCode = 0

    $planCommands = @($Plan.commandPlan)
    foreach ($entry in $planCommands) {
        $result = Invoke-ValidationRecipeCommand -Entry $entry -Timeout $Timeout
        $null = $results.Add($result)
        $totalDuration += [double]$result.durationSeconds
        if (-not [string]::IsNullOrWhiteSpace($result.stdoutSummary)) {
            $null = $stdoutParts.Add($result.stdoutSummary)
        }
        if (-not [string]::IsNullOrWhiteSpace($result.stderrSummary)) {
            $null = $stderrParts.Add($result.stderrSummary)
        }
        if ($result.exitCode -ne 0) {
            $exitCode = $result.exitCode
            break
        }
    }

    $aggregate = New-Object System.Collections.Specialized.OrderedDictionary
    $aggregate.Add('exitCode', $exitCode)
    $aggregate.Add('durationSeconds', [Math]::Round($totalDuration, 3))
    $aggregate.Add('stdoutSummary', (Get-TextSummary -Text ([string]::Join([Environment]::NewLine, @($stdoutParts)))))
    $aggregate.Add('stderrSummary', (Get-TextSummary -Text ([string]::Join([Environment]::NewLine, @($stderrParts)))))
    $aggregate.Add('commandResults', @($results.ToArray()))
    return $aggregate
}

# Reviewed helper identifiers for tools/check-ai-integration.ps1 / manifest docs:
# Get-ValidationRecipeCommandPlan Invoke-ValidationRecipeCommandPlan
. (Join-Path $PSScriptRoot 'run-validation-recipe-plans.ps1')

if ([string]::IsNullOrWhiteSpace($Recipe)) {
    Write-RunnerResult -Result (New-ValidationRecipeRejectedResult -Mode $Mode -RecipeName "" -Diagnostic (New-RunnerDiagnostic -Severity "error" -Code "missing-recipe" -Message "Recipe is required.")) -ExitCode 2
}

$plan = Get-ValidationRecipeCommandPlan -RecipeName $Recipe -SelectedGameTarget $GameTarget -SelectedStrictBackend $StrictBackend
if ($null -eq $plan) {
    Write-RunnerResult -Result (New-ValidationRecipeRejectedResult -Mode $Mode -RecipeName $Recipe -Diagnostic (New-RunnerDiagnostic -Severity "error" -Code "unknown-recipe" -Message "Validation recipe '$Recipe' is not in the reviewed run-validation-recipe allowlist." -ValidationRecipe $Recipe)) -ExitCode 2
}

$requestDiagnostic = Test-ValidationRecipeRequest `
    -Plan $plan `
    -Mode $Mode `
    -GameTarget $GameTarget `
    -StrictBackend $StrictBackend `
    -HostGateAcknowledgements $HostGateAcknowledgements `
    -RemainingArguments $RemainingArguments `
    -TimeoutSeconds $TimeoutSeconds
if ($null -ne $requestDiagnostic) {
    $result = New-ValidationRecipeRejectedResult -Mode $Mode -RecipeName $Recipe -Diagnostic $requestDiagnostic
    $result.hostGates = @($plan.hostGates)
    $result.validationRecipes = @($Recipe)
    Write-RunnerResult -Result $result -ExitCode 2
}

$firstCommand = @($plan.commandPlan)[0]
if ($Mode -eq "DryRun") {
    Write-RunnerResult -Result (New-ValidationRecipeDryRunResult -Mode $Mode -Plan $plan)
}

$execution = Invoke-ValidationRecipeCommandPlan -Plan $plan -Timeout $TimeoutSeconds
$diagnostics = @($plan.diagnostics)
$combinedOutput = $execution.stdoutSummary + [Environment]::NewLine + $execution.stderrSummary
if ($combinedOutput.Contains("CreateFileW stdin failed with 5")) {
    $diagnostics += New-RunnerDiagnostic -Severity "error" -Code "host-sandbox-vcpkg-stdin" -Message "Validation hit the known vcpkg/7zip CreateFileW stdin failed with 5 host or sandbox blocker; rerun the same package command with approved local privileges." -ValidationRecipe $Recipe
}

$status = if ($execution.exitCode -eq 0) { "passed" } else { "failed" }
Write-RunnerResult -Result ([ordered]@{
        commandId = "run-validation-recipe"
        mode = $Mode
        recipe = $Recipe
        validationRecipe = $Recipe
        status = $status
        command = $firstCommand.command
        argv = @($firstCommand.argv)
        commandPlan = @($plan.commandPlan)
        exitCode = $execution.exitCode
        durationSeconds = $execution.durationSeconds
        stdoutSummary = $execution.stdoutSummary
        stderrSummary = $execution.stderrSummary
        commandResults = @($execution.commandResults)
        hostGates = @($plan.hostGates)
        diagnostics = @($diagnostics)
        validationRecipes = @($Recipe)
        unsupportedGapIds = @(
            "editor-productization",
            "production-ui-importer-platform-adapters",
            "3d-playable-vertical-slice"
        )
        undoToken = New-UndoToken
    }) -ExitCode $execution.exitCode
