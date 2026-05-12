#requires -Version 7.0
#requires -PSEdition Core

param(
    [ValidateSet("Full", "Standard", "Minimal")]
    [string] $ContextProfile = "Full"
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$manifestPath = Join-Path $root "engine/agent/manifest.json"

if (-not (Test-Path $manifestPath)) {
    Write-Error "Missing engine agent manifest: $manifestPath"
}

$manifestFull = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json

function New-StandardManifestView([PSCustomObject]$full) {
    $clone = $full | ConvertTo-Json -Depth 100 | ConvertFrom-Json
    foreach ($m in $clone.modules) {
        $t = [string]$m.purpose
        if ($t.Length -gt 500) {
            $m.purpose = $t.Substring(0, 500) + " ... [truncated; use -ContextProfile Full or read engine/agent/manifest.fragments/004-modules.json]"
        }
    }

    return $clone
}

function New-MinimalManifestView([PSCustomObject]$full) {
    $loop = $full.aiOperableProductionLoop
    $recipes = @($loop.recipes | ForEach-Object {
            [PSCustomObject]@{
                id     = $_.id
                status = $_.status
                summary = $_.summary
            }
        })
    $surfaces = @($loop.commandSurfaces | ForEach-Object {
            [PSCustomObject]@{
                id              = $_.id
                status          = $_.status
                summary         = $_.summary
                requiredModules = @($_.requiredModules)
                capabilityGateIds = @($_.capabilityGates | ForEach-Object { $_.id })
            }
        })
    $gaps = @($loop.unsupportedProductionGaps | ForEach-Object {
            [PSCustomObject]@{
                id     = $_.id
                status = $_.status
            }
        })
    $hgs = @($loop.hostGates | ForEach-Object {
            [PSCustomObject]@{
                id     = $_.id
                status = $_.status
            }
        })
    return [PSCustomObject]@{
        schemaVersion = $full.schemaVersion
        engine        = $full.engine
        commands      = $full.commands
        modules       = @($full.modules | ForEach-Object {
                [PSCustomObject]@{
                    name         = $_.name
                    path         = $_.path
                    status       = $_.status
                    dependencies = @($_.dependencies)
                }
            })
        aiOperableProductionLoop = [PSCustomObject]@{
            currentActivePlan   = $loop.currentActivePlan
            recommendedNextPlan = $loop.recommendedNextPlan
            recipes             = $recipes
            commandSurfaces     = $surfaces
            unsupportedProductionGaps = $gaps
            hostGates           = $hgs
        }
    }
}

$manifestForContext = switch ($ContextProfile) {
    "Full" { $manifestFull }
    "Standard" { New-StandardManifestView $manifestFull }
    "Minimal" { New-MinimalManifestView $manifestFull }
    Default { $manifestFull }
}

function Get-ManifestPublicHeaders($manifest) {
    $headers = @()
    foreach ($module in $manifest.modules) {
        foreach ($header in $module.publicHeaders) {
            $headers += $header
        }
    }

    return @($headers | Sort-Object -Unique)
}

function Get-ModuleOwnership($manifest) {
    $modules = @()
    foreach ($module in $manifest.modules) {
        $modules += [ordered]@{
            name          = $module.name
            path          = $module.path
            status        = $module.status
            dependencies  = @($module.dependencies)
            publicHeaders = @($module.publicHeaders)
            purpose       = $module.purpose
        }
    }

    return @($modules | Sort-Object { $_.name })
}

function Get-RelativeFiles($path, $filter) {
    $fullPath = Join-Path $root $path
    if (-not (Test-Path $fullPath)) {
        return @()
    }

    return @(Get-ChildItem -Path $fullPath -Recurse -File -Filter $filter | ForEach-Object {
            $_.FullName.Substring($root.Length + 1).Replace("\", "/")
        })
}

function Get-GameManifestSummaries() {
    $gameManifestPaths = @(Get-ChildItem -Path (Join-Path $root "games") -Recurse -File -Filter "game.agent.json")
    $summaries = @()
    foreach ($gameManifestPath in $gameManifestPaths) {
        $relativePath = $gameManifestPath.FullName.Substring($root.Length + 1).Replace("\", "/")
        $game = Get-Content -LiteralPath $gameManifestPath.FullName -Raw | ConvertFrom-Json
        $summaries += [ordered]@{
            manifest             = $relativePath
            name                 = $game.name
            displayName          = $game.displayName
            target               = $game.target
            entryPoint           = $game.entryPoint
            engineModules        = @($game.engineModules)
            backendReadiness     = $game.backendReadiness
            importerRequirements = $game.importerRequirements
            packagingTargets     = @($game.packagingTargets)
            validationRecipes    = @($game.validationRecipes)
        }
    }

    return @($summaries | Sort-Object { $_.name })
}

$context = [ordered]@{
    generatedBy               = "tools/agent-context.ps1"
    contextProfile            = $ContextProfile
    manifestSourceOfTruthHint = "engine/agent/manifest.fragments (compose via tools/compose-agent-manifest.ps1 -Write)"
    root                      = $root
    manifest                  = $manifestForContext
    instructions              = [ordered]@{
        shared = "AGENTS.md"
        claude = "CLAUDE.md"
    }
    documentation             = $manifestFull.documentationPolicy.entrypoints
    docs                      = @(Get-RelativeFiles "docs" "*.md")
    publicHeaders             = @(Get-ManifestPublicHeaders $manifestFull)
    moduleOwnership           = @(Get-ModuleOwnership $manifestFull)
    games                     = @(Get-RelativeFiles "games" "game.agent.json")
    sampleGames               = @(Get-GameManifestSummaries)
    assets                    = $manifestFull.importerCapabilities
    platformTargets           = @($manifestFull.packagingTargets)
    validationRecipes         = @($manifestFull.validationRecipes)
    windowsDiagnosticsToolchain = $manifestFull.windowsDiagnosticsToolchain
    productionLoop            = $manifestFull.aiOperableProductionLoop
    productionRecipes         = @($manifestFull.aiOperableProductionLoop.recipes)
    aiCommandSurfaces         = @($manifestFull.aiOperableProductionLoop.commandSurfaces)
    unsupportedProductionGaps = @($manifestFull.aiOperableProductionLoop.unsupportedProductionGaps)
    hostGates                 = @($manifestFull.aiOperableProductionLoop.hostGates)
    recommendedNextPlan       = $manifestFull.aiOperableProductionLoop.recommendedNextPlan
    codexSkills               = @(Get-RelativeFiles ".agents/skills" "SKILL.md")
    codexAgents               = @(Get-RelativeFiles ".codex/agents" "*.toml")
    codexRules                = @(Get-RelativeFiles ".codex/rules" "*.rules")
    claudeSettings            = @(Get-RelativeFiles ".claude" "settings.json")
    claudeRules               = @(Get-RelativeFiles ".claude/rules" "*.md")
    claudeSkills              = @(Get-RelativeFiles ".claude/skills" "SKILL.md")
    claudeAgents              = @(Get-RelativeFiles ".claude/agents" "*.md")
    validation                = [ordered]@{
        required  = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1"
        agentOnly = "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1"
    }
}

$context | ConvertTo-Json -Depth 16
