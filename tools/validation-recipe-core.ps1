#requires -Version 7.0
#requires -PSEdition Core
# Dot-sourced by tools/run-validation-recipe.ps1 and tools/check-validation-recipe-runner.ps1.

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

function New-ValidationRecipeRejectedResult {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet("DryRun", "Execute")]
        [string]$Mode,

        [string]$RecipeName,

        [Parameter(Mandatory = $true)]
        $Diagnostic
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
    param([string[]]$Values = @())

    $splitValues = [System.Collections.Generic.List[string]]::new()
    foreach ($entry in @($Values)) {
        if ([string]::IsNullOrWhiteSpace($entry)) {
            continue
        }
        foreach ($part in $entry.Split(",")) {
            $trimmed = $part.Trim()
            if (-not [string]::IsNullOrWhiteSpace($trimmed)) {
                $splitValues.Add($trimmed) | Out-Null
            }
        }
    }
    return @($splitValues)
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

    $scriptPath = Join-Path (Get-RepoRoot) "tools\$ToolScriptName"
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

function Test-ValidationRecipeRequest {
    param(
        [Parameter(Mandatory = $true)]
        [object]$Plan,

        [Parameter(Mandatory = $true)]
        [ValidateSet("DryRun", "Execute")]
        [string]$Mode,

        [string]$GameTarget = "",
        [ValidateSet("", "D3D12", "Vulkan")]
        [string]$StrictBackend = "",
        [string[]]$HostGateAcknowledgements = @(),
        [string[]]$RemainingArguments = @(),
        [int]$TimeoutSeconds = 0
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

    $acknowledgements = @(Split-HostGateAcknowledgements -Values $HostGateAcknowledgements)
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

function New-ValidationRecipeDryRunResult {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateSet("DryRun", "Execute")]
        [string]$Mode,

        [Parameter(Mandatory = $true)]
        [object]$Plan
    )

    $firstCommand = @($Plan.commandPlan)[0]
    return [ordered]@{
        commandId = "run-validation-recipe"
        mode = $Mode
        recipe = $Plan.recipe
        validationRecipe = $Plan.recipe
        status = "dry-run"
        command = $firstCommand.command
        argv = @($firstCommand.argv)
        commandPlan = @($Plan.commandPlan)
        hostGates = @($Plan.hostGates)
        diagnostics = @($Plan.diagnostics)
        blockedBy = @()
        validationRecipes = @($Plan.recipe)
        unsupportedGapIds = @(
            "editor-productization",
            "production-ui-importer-platform-adapters",
            "3d-playable-vertical-slice"
        )
        undoToken = New-UndoToken
    }
}
