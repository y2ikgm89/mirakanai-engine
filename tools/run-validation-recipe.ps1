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

$root = Get-RepoRoot

function New-RunnerDiagnostic {
    param(
        [Parameter(Mandatory = $true)][string]$Severity,
        [Parameter(Mandatory = $true)][string]$Code,
        [Parameter(Mandatory = $true)][string]$Message,
        [string]$ValidationRecipe = "",
        [string]$HostGate = ""
    )

    return [ordered]@{
        severity = $Severity
        code = $Code
        message = $Message
        path = ""
        unsupportedGapId = ""
        validationRecipe = $ValidationRecipe
        hostGate = $HostGate
    }
}

function New-UndoToken {
    return [ordered]@{
        status = "placeholder-only"
        notes = "Validation execution is non-mutating; no repository undo token is produced."
    }
}

function Write-RunnerResult {
    param(
        [Parameter(Mandatory = $true)]$Result,
        [int]$ExitCode = 0
    )

    $Result | ConvertTo-Json -Depth 12
    exit $ExitCode
}

function New-RejectedResult {
    param(
        [string]$RecipeName,
        [Parameter(Mandatory = $true)]$Diagnostic
    )

    return [ordered]@{
        commandId = "run-validation-recipe"
        mode = $Mode
        recipe = $RecipeName
        validationRecipe = $RecipeName
        status = "rejected"
        command = ""
        argv = @()
        hostGates = @()
        diagnostics = @($Diagnostic)
        blockedBy = @()
        validationRecipes = @()
        unsupportedGapIds = @(
            "editor-productization",
            "production-ui-importer-platform-adapters",
            "3d-playable-vertical-slice"
        )
        undoToken = New-UndoToken
    }
}

function Split-HostGateAcknowledgements {
    $values = [System.Collections.Generic.List[string]]::new()
    foreach ($entry in @($HostGateAcknowledgements)) {
        if ([string]::IsNullOrWhiteSpace($entry)) {
            continue
        }
        foreach ($part in $entry.Split(",")) {
            $trimmed = $part.Trim()
            if (-not [string]::IsNullOrWhiteSpace($trimmed)) {
                $values.Add($trimmed) | Out-Null
            }
        }
    }
    return @($values)
}

function Test-SafeGameTarget {
    param([string]$Target)

    if ([string]::IsNullOrWhiteSpace($Target)) {
        return $false
    }
    return $Target -match '^[A-Za-z_][A-Za-z0-9_]*$'
}

function New-CommandPlanEntry {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Command,
        [Parameter(Mandatory = $true)]
        [System.String[]]$Argv
    )

    $argvCopy = @($Argv)
    $argvJoined = [System.String]::Join(' ', $argvCopy)
    $displayText = $Command + ' ' + $argvJoined
    $row = New-Object System.Collections.Specialized.OrderedDictionary
    $row.Add('command', $Command)
    $row.Add('argv', $argvCopy)
    $row.Add('display', $displayText)
    return $row
}

function Get-PwshScriptCommandPlan {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ScriptPath,
        [Parameter()]
        [AllowNull()]
        [System.String[]]$ScriptArguments
    )

    $baseArgv = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $ScriptPath)
    $tailArgv = @()
    if ($null -ne $ScriptArguments) {
        $tailArgv = @($ScriptArguments)
    }
    $mergedArgv = @($baseArgv) + $tailArgv
    return New-CommandPlanEntry -Command "pwsh" -Argv $mergedArgv
}

function Get-RepositoryToolCommandPlan {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ToolScriptName
    )

    $scriptPath = Join-Path $root "tools\$ToolScriptName"
    return Get-PwshScriptCommandPlan -ScriptPath $scriptPath
}

function Get-SampleDesktopRuntimeGameVulkanSmokeArgs {
    $list = New-Object System.Collections.ArrayList
    $list.Add('--smoke') | Out-Null
    $list.Add('--require-config') | Out-Null
    $list.Add('runtime/sample_desktop_runtime_game.config') | Out-Null
    $list.Add('--require-scene-package') | Out-Null
    $list.Add('runtime/sample_desktop_runtime_game.geindex') | Out-Null
    $list.Add('--require-vulkan-scene-shaders') | Out-Null
    $list.Add('--video-driver') | Out-Null
    $list.Add('windows') | Out-Null
    $list.Add('--require-vulkan-renderer') | Out-Null
    $list.Add('--require-scene-gpu-bindings') | Out-Null
    $list.Add('--require-postprocess') | Out-Null
    $list.Add('--require-postprocess-depth-input') | Out-Null
    $list.Add('--require-directional-shadow') | Out-Null
    $list.Add('--require-directional-shadow-filtering') | Out-Null
    $list.Add('--require-native-ui-overlay') | Out-Null
    $list.Add('--require-native-ui-textured-sprite-atlas') | Out-Null
    return $list.ToArray()
}

function Get-GeneratedMaterialShaderScaffoldPackageVulkanSmokeArgs {
    return @(
        '--smoke',
        '--require-config',
        'runtime/sample_generated_desktop_runtime_material_shader_package.config',
        '--require-scene-package',
        'runtime/sample_generated_desktop_runtime_material_shader_package.geindex',
        '--require-vulkan-scene-shaders',
        '--video-driver',
        'windows',
        '--require-vulkan-renderer',
        '--require-scene-gpu-bindings',
        '--require-postprocess'
    )
}

function New-RecipePlanRow {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Recipe,
        [Parameter(Mandatory = $true)]
        $CommandPlan,
        [Parameter(Mandatory = $false)]
        $HostGates,
        [Parameter(Mandatory = $false)]
        $RequiredAcknowledgements,
        [Parameter(Mandatory = $false)]
        $AllowedGameTargets,
        [Parameter(Mandatory = $false)]
        $AllowedStrictBackend,
        [Parameter(Mandatory = $false)]
        $Diagnostics
    )

    $hg = @()
    if ($null -ne $HostGates) {
        $hg = @($HostGates)
    }
    $req = @()
    if ($null -ne $RequiredAcknowledgements) {
        $req = @($RequiredAcknowledgements)
    }
    $agt = @()
    if ($null -ne $AllowedGameTargets) {
        $agt = @($AllowedGameTargets)
    }
    $asb = @()
    if ($null -ne $AllowedStrictBackend) {
        $asb = @($AllowedStrictBackend)
    }
    $diag = @()
    if ($null -ne $Diagnostics) {
        $diag = @($Diagnostics)
    }

    $row = New-Object System.Collections.Specialized.OrderedDictionary
    $row.Add('recipe', $Recipe)
    $row.Add('commandPlan', $CommandPlan)
    $row.Add('hostGates', $hg)
    $row.Add('requiredAcknowledgements', $req)
    $row.Add('allowedGameTargets', $agt)
    $row.Add('allowedStrictBackend', $asb)
    $row.Add('diagnostics', $diag)
    return $row
}

function Test-RecipeRequest {
    param(
        [Parameter(Mandatory = $true)]
        [object]$Plan
    )

    $remainingArgCount = @($RemainingArguments).Count
    if ($remainingArgCount -gt 0) {
        return New-RunnerDiagnostic -Severity "error" -Code "unsupported-arguments" -Message "run-validation-recipe rejects free-form command arguments; select a reviewed recipe id and typed options instead." -ValidationRecipe $Plan.recipe
    }

    if ($TimeoutSeconds -lt 0) {
        return New-RunnerDiagnostic -Severity "error" -Code "invalid-timeout" -Message "timeoutSeconds must be zero or a positive integer." -ValidationRecipe $Plan.recipe
    }

    if (@($Plan.allowedGameTargets).Count -eq 0 -and -not [string]::IsNullOrWhiteSpace($GameTarget)) {
        return New-RunnerDiagnostic -Severity "error" -Code "unsupported-game-target" -Message "gameTarget is not supported for validation recipe '$($Plan.recipe)'." -ValidationRecipe $Plan.recipe
    }

    if (-not [string]::IsNullOrWhiteSpace($GameTarget)) {
        if (-not (Test-SafeGameTarget -Target $GameTarget)) {
            return New-RunnerDiagnostic -Severity "error" -Code "unsafe-game-target" -Message "gameTarget must be a registered target id, not a path or free-form command fragment." -ValidationRecipe $Plan.recipe
        }
        if (@($Plan.allowedGameTargets) -notcontains $GameTarget) {
            return New-RunnerDiagnostic -Severity "error" -Code "unsupported-game-target" -Message "gameTarget '$GameTarget' is not allowlisted for validation recipe '$($Plan.recipe)'." -ValidationRecipe $Plan.recipe
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($StrictBackend) -and @($Plan.allowedStrictBackend) -notcontains $StrictBackend) {
        return New-RunnerDiagnostic -Severity "error" -Code "unsupported-strict-backend" -Message "strictBackend '$StrictBackend' is not supported for validation recipe '$($Plan.recipe)'." -ValidationRecipe $Plan.recipe
    }

    $acknowledgements = @(Split-HostGateAcknowledgements)
    foreach ($acknowledgement in $acknowledgements) {
        if (@($Plan.hostGates) -notcontains $acknowledgement) {
            return New-RunnerDiagnostic -Severity "error" -Code "unknown-host-gate-acknowledgement" -Message "hostGateAcknowledgements contains '$acknowledgement', which is not used by validation recipe '$($Plan.recipe)'." -ValidationRecipe $Plan.recipe -HostGate $acknowledgement
        }
    }

    if ($Mode -eq "Execute") {
        foreach ($requiredAcknowledgement in @($Plan.requiredAcknowledgements)) {
            if ($acknowledgements -notcontains $requiredAcknowledgement) {
                return New-RunnerDiagnostic -Severity "error" -Code "missing-host-gate-acknowledgement" -Message "Validation recipe '$($Plan.recipe)' requires hostGateAcknowledgements to include '$requiredAcknowledgement' before execution." -ValidationRecipe $Plan.recipe -HostGate $requiredAcknowledgement
            }
        }
    }

    return $null
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
    Write-RunnerResult -Result (New-RejectedResult -RecipeName "" -Diagnostic (New-RunnerDiagnostic -Severity "error" -Code "missing-recipe" -Message "Recipe is required.")) -ExitCode 2
}

$plan = Get-ValidationRecipeCommandPlan -RecipeName $Recipe -SelectedGameTarget $GameTarget -SelectedStrictBackend $StrictBackend
if ($null -eq $plan) {
    Write-RunnerResult -Result (New-RejectedResult -RecipeName $Recipe -Diagnostic (New-RunnerDiagnostic -Severity "error" -Code "unknown-recipe" -Message "Validation recipe '$Recipe' is not in the reviewed run-validation-recipe allowlist." -ValidationRecipe $Recipe)) -ExitCode 2
}

$requestDiagnostic = Test-RecipeRequest -Plan $plan
if ($null -ne $requestDiagnostic) {
    $result = New-RejectedResult -RecipeName $Recipe -Diagnostic $requestDiagnostic
    $result.hostGates = @($plan.hostGates)
    $result.validationRecipes = @($Recipe)
    Write-RunnerResult -Result $result -ExitCode 2
}

$firstCommand = @($plan.commandPlan)[0]
if ($Mode -eq "DryRun") {
    Write-RunnerResult -Result ([ordered]@{
            commandId = "run-validation-recipe"
            mode = $Mode
            recipe = $Recipe
            validationRecipe = $Recipe
            status = "dry-run"
            command = $firstCommand.command
            argv = @($firstCommand.argv)
            commandPlan = @($plan.commandPlan)
            hostGates = @($plan.hostGates)
            diagnostics = @($plan.diagnostics)
            blockedBy = @()
            validationRecipes = @($Recipe)
            unsupportedGapIds = @(
                "editor-productization",
                "production-ui-importer-platform-adapters",
                "3d-playable-vertical-slice"
            )
            undoToken = New-UndoToken
        })
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
