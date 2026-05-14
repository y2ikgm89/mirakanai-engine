#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

function Test-RepositoryLineEndingContract {
    param([Parameter(Mandatory)][string]$Root)

    $gitAttributesPath = Join-Path $Root ".gitattributes"
    if (-not (Test-Path -LiteralPath $gitAttributesPath -PathType Leaf)) {
        Write-Error "Repository root must include .gitattributes with '* text=auto eol=lf'."
    }

    $gitAttributesText = Get-Content -LiteralPath $gitAttributesPath -Raw
    if ($gitAttributesText -notmatch '(?m)^\*\s+text=auto\s+eol=lf\s*$') {
        Write-Error "Repository root .gitattributes must declare '* text=auto eol=lf'."
    }

    $editorConfigPath = Join-Path $Root ".editorconfig"
    if (-not (Test-Path -LiteralPath $editorConfigPath -PathType Leaf)) {
        Write-Error "Repository root must include .editorconfig aligned with the LF line-ending policy."
    }

    $editorConfigText = Get-Content -LiteralPath $editorConfigPath -Raw
    foreach ($requiredLine in @(
            "root = true",
            "charset = utf-8",
            "end_of_line = lf",
            "insert_final_newline = true"
        )) {
        if ($editorConfigText -notmatch "(?m)^\s*$([regex]::Escape($requiredLine))\s*$") {
            Write-Error "Repository root .editorconfig must declare '$requiredLine'."
        }
    }

    $eolEntries = @(git -C $Root ls-files --eol)
    if ($LASTEXITCODE -ne 0) {
        Write-Error "git ls-files --eol failed while checking repository line endings."
    }

    $nonLfEntries = @($eolEntries | Where-Object { $_ -match '\b[wi]/(crlf|mixed)\b' })
    if ($nonLfEntries.Count -gt 0) {
        $sample = ($nonLfEntries | Select-Object -First 10) -join "`n"
        Write-Error "Tracked files must not have CRLF or mixed line endings under the root LF contract. First entries:`n$sample"
    }
}

Test-RepositoryLineEndingContract -Root $root

function Test-AgentFileSizeBudget {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][int]$MaxBytes,
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][string]$Guidance
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        return
    }

    $bytes = [System.IO.File]::ReadAllBytes($Path).Length
    if ($bytes -gt $MaxBytes) {
        Write-Error ("{0} is {1} bytes, exceeding initial-load budget ({2} bytes). {3}" -f $Label, $bytes, $MaxBytes, $Guidance)
    }
}

$agentsPath = Join-Path $root "AGENTS.md"
Test-AgentFileSizeBudget `
    -Path $agentsPath `
    -MaxBytes (32 * 1024) `
    -Label "AGENTS.md" `
    -Guidance "Move long procedures to skills/docs/subagents/manifest."

$toolsScriptRoot = Join-Path $root "tools"
foreach ($script in Get-ChildItem -LiteralPath $toolsScriptRoot -Filter "*.ps1" -File | Sort-Object Name) {
    $bytes = [System.IO.File]::ReadAllBytes($script.FullName)
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        Write-Error "tools/$($script.Name) must be UTF-8 without BOM (remove EF BB BF prefix; prefer UTF-8 no BOM for cross-host scripts)."
    }

    $lines = [System.IO.File]::ReadAllLines($script.FullName)
    $versionIdx = 0..($lines.Length - 1) | Where-Object { $lines[$_] -eq "#requires -Version 7.0" } | Select-Object -First 1
    if ($null -eq $versionIdx) {
        Write-Error "tools/$($script.Name) must declare exactly: #requires -Version 7.0 (missing)"
    }
    $nextIdx = $versionIdx + 1
    if ($nextIdx -ge $lines.Length -or $lines[$nextIdx] -ne "#requires -PSEdition Core") {
        Write-Error ("tools/{0} must declare #requires -PSEdition Core on the line immediately after #requires -Version 7.0 " -f $script.Name)
    }
}

function Get-SkillFrontmatterBlock {
    param([Parameter(Mandatory)][string]$SkillMdPath)
    $lines = Get-Content -LiteralPath $SkillMdPath
    if ($lines.Length -lt 2 -or $lines[0].Trim() -ne "---") {
        Write-Error "Skill must start with YAML frontmatter (---): $SkillMdPath"
    }
    $end = 1
    for (; $end -lt $lines.Length; $end++) {
        if ($lines[$end].Trim() -eq "---") {
            break
        }
    }
    if ($end -ge $lines.Length) {
        Write-Error "Skill frontmatter missing closing --- : $SkillMdPath"
    }
    return ($lines[1..($end - 1)] -join "`n")
}

function Test-SkillFrontmatter {
    param(
        [Parameter(Mandatory)][string]$SkillMdPath,
        [Parameter(Mandatory)][string]$ExpectedName,
        [Parameter(Mandatory)][bool]$RequirePaths,
        [Parameter(Mandatory)][bool]$ForbidGlobs
    )
    $fm = Get-SkillFrontmatterBlock -SkillMdPath $SkillMdPath

    if (-not ($fm -match '(?m)^name:\s*([a-z0-9-]+)')) {
        Write-Error "Skill frontmatter must include 'name:' (lowercase letters, digits, hyphens only): $SkillMdPath"
    }
    $parsedName = $matches[1].Trim()
    if ($parsedName -ne $ExpectedName) {
        Write-Error "Skill 'name:' ($parsedName) must match directory name '$ExpectedName': $SkillMdPath"
    }

    if ($fm -notmatch '(?m)^description:\s*\S') {
        Write-Error "Skill frontmatter must include non-empty 'description:' (start of value on same line, or folded block): $SkillMdPath"
    }

    if ($ForbidGlobs -and ($fm -match '(?m)^globs\s*:')) {
        Write-Error "Skill must not use legacy 'globs:' frontmatter (use 'paths:' per Cursor Agent Skills): $SkillMdPath"
    }

    if ($RequirePaths) {
        $hasPaths = $false
        if ($fm -match '(?ms)^paths:\s*\n(\s*-\s*\S)') {
            $hasPaths = $true
        }
        elseif ($fm -match '(?m)^paths:\s*\S') {
            $hasPaths = $true
        }
        if (-not $hasPaths) {
            Write-Error "Skill must declare non-empty 'paths:' (list or inline globs) per repository policy: $SkillMdPath"
        }
    }
}

function Test-SkillReferenceTarget {
    param([Parameter(Mandatory)][string]$SkillMdPath)

    $content = Get-Content -LiteralPath $SkillMdPath -Raw
    $referenceMatches = [System.Text.RegularExpressions.Regex]::Matches(
        $content,
        '(?<!/)references/[A-Za-z0-9._/-]+'
    )
    foreach ($referenceMatch in $referenceMatches) {
        $relativeReference = $referenceMatch.Value.TrimEnd(".", ",", ";", ":")
        $localReference = $relativeReference.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
        $referencePath = Join-Path (Split-Path -Parent $SkillMdPath) $localReference
        if (-not (Test-Path -LiteralPath $referencePath)) {
            Write-Error "Skill references missing local file '$relativeReference': $SkillMdPath"
        }
    }
}

$skillRoot = Join-Path $root ".agents/skills"
$agentRoot = Join-Path $root ".codex/agents"
$codexRuleRoot = Join-Path $root ".codex/rules"
$claudeSettingsPath = Join-Path $root ".claude/settings.json"
$claudeSkillRoot = Join-Path $root ".claude/skills"
$claudeAgentRoot = Join-Path $root ".claude/agents"
$cursorSkillRoot = Join-Path $root ".cursor/skills"

if (Test-Path $skillRoot) {
    Get-ChildItem -Path $skillRoot -Directory | ForEach-Object {
        $skillFile = Join-Path $_.FullName "SKILL.md"
        if (-not (Test-Path $skillFile)) {
            Write-Error "Skill folder missing SKILL.md: $($_.FullName)"
        }
        Test-AgentFileSizeBudget `
            -Path $skillFile `
            -MaxBytes (24 * 1024) `
            -Label ".agents/skills/$($_.Name)/SKILL.md" `
            -Guidance "Keep SKILL.md as a concise trigger/router; move detailed procedures to references/*.md or docs."
        Test-SkillFrontmatter -SkillMdPath $skillFile -ExpectedName $_.Name -RequirePaths $true -ForbidGlobs $false
        Test-SkillReferenceTarget -SkillMdPath $skillFile
    }
}

if (Test-Path $agentRoot) {
    Get-ChildItem -Path $agentRoot -Filter "*.toml" | ForEach-Object {
        Test-AgentFileSizeBudget `
            -Path $_.FullName `
            -MaxBytes (16 * 1024) `
            -Label ".codex/agents/$($_.Name)" `
            -Guidance "Keep custom agents narrowly scoped and move reusable procedures to skills/docs."
        $content = Get-Content -LiteralPath $_.FullName -Raw
        foreach ($field in @("name", "description", "developer_instructions")) {
            if ($content -notmatch "(?m)^$field\s*=") {
                Write-Error "Custom agent missing required field '$field': $($_.FullName)"
            }
        }
    }
}

if (Test-Path $codexRuleRoot) {
    Get-ChildItem -Path $codexRuleRoot -Filter "*.rules" | ForEach-Object {
        $content = Get-Content -LiteralPath $_.FullName -Raw
        if ($content -notmatch "prefix_rule\(" -or $content -notmatch "match\s*=" -or $content -notmatch "not_match\s*=") {
            Write-Error "Codex rule files must use prefix_rule with match and not_match examples: $($_.FullName)"
        }
        $allowRuleBlocks = [System.Text.RegularExpressions.Regex]::Matches(
            $content,
            'prefix_rule\([\s\S]*?decision\s*=\s*"allow"[\s\S]*?\)'
        )
        $approvedAllowPatterns = @(
            'pattern\s*=\s*\["gh",\s*"pr",\s*"view"\]',
            'pattern\s*=\s*\["gh",\s*"pr",\s*"create"\]',
            'pattern\s*=\s*\["gh",\s*"pr",\s*"merge",\s*"--auto",\s*"--merge",\s*"--delete-branch"\]'
        )
        foreach ($allowRuleBlock in $allowRuleBlocks) {
            $approvedAllow = $false
            foreach ($approvedAllowPattern in $approvedAllowPatterns) {
                if ($allowRuleBlock.Value -match $approvedAllowPattern) {
                    $approvedAllow = $true
                    break
                }
            }
            if (-not $approvedAllow) {
                Write-Error "Project Codex rules must not add broad allow decisions: $($_.FullName)"
            }
        }
    }
}

if (Test-Path $claudeSettingsPath) {
    $settings = Get-Content -LiteralPath $claudeSettingsPath -Raw | ConvertFrom-Json
    if (-not $settings.PSObject.Properties.Name.Contains('$schema')) {
        Write-Error "Claude settings must include the official JSON schema: $claudeSettingsPath"
    }
    if (-not $settings.PSObject.Properties.Name.Contains("permissions")) {
        Write-Error "Claude settings must define permissions: $claudeSettingsPath"
    }
    if (-not $settings.permissions.PSObject.Properties.Name.Contains("deny") -or @($settings.permissions.deny).Count -lt 1) {
        Write-Error "Claude settings must deny secret-bearing reads: $claudeSettingsPath"
    }
    if (-not $settings.permissions.PSObject.Properties.Name.Contains("ask") -or @($settings.permissions.ask).Count -lt 1) {
        Write-Error "Claude settings must ask before sensitive commands: $claudeSettingsPath"
    }
}

if (Test-Path $claudeSkillRoot) {
    Get-ChildItem -Path $claudeSkillRoot -Directory | ForEach-Object {
        $skillFile = Join-Path $_.FullName "SKILL.md"
        if (-not (Test-Path $skillFile)) {
            Write-Error "Claude skill folder missing SKILL.md: $($_.FullName)"
        }
        Test-AgentFileSizeBudget `
            -Path $skillFile `
            -MaxBytes (24 * 1024) `
            -Label ".claude/skills/$($_.Name)/SKILL.md" `
            -Guidance "Keep SKILL.md as a concise trigger/router; move detailed procedures to references/*.md or docs."
        Test-SkillFrontmatter -SkillMdPath $skillFile -ExpectedName $_.Name -RequirePaths $true -ForbidGlobs $false
        Test-SkillReferenceTarget -SkillMdPath $skillFile
    }
}

if (Test-Path $cursorSkillRoot) {
    Get-ChildItem -Path $cursorSkillRoot -Directory | ForEach-Object {
        $skillFile = Join-Path $_.FullName "SKILL.md"
        if (-not (Test-Path $skillFile)) {
            Write-Error "Cursor skill folder missing SKILL.md: $($_.FullName)"
        }
        Test-AgentFileSizeBudget `
            -Path $skillFile `
            -MaxBytes (24 * 1024) `
            -Label ".cursor/skills/$($_.Name)/SKILL.md" `
            -Guidance "Keep SKILL.md as a concise trigger/router; move detailed procedures to references/*.md or shared skill references."
        Test-SkillFrontmatter -SkillMdPath $skillFile -ExpectedName $_.Name -RequirePaths $true -ForbidGlobs $true
        Test-SkillReferenceTarget -SkillMdPath $skillFile
    }
}

# Codex <-> Claude `gameengine-*` skill twins (behavioral parity surface).
# When adding a new `.claude/skills/gameengine-*` folder, register it here with the matching `.agents/skills/<folder>` name.
$claudeToCodexSkillMap = @{
    "gameengine-agent-integration"  = "gameengine-agent-integration"
    "gameengine-cmake-build-system" = "cmake-build-system"
    "gameengine-debugging"          = "cpp-engine-debugging"
    "gameengine-editor"             = "editor-change"
    "gameengine-feature"            = "gameengine-feature"
    "gameengine-game-development"   = "gameengine-game-development"
    "gameengine-license-audit"      = "license-audit"
    "gameengine-rendering"          = "rendering-change"
}

function Get-FrontmatterPathsSection {
    param([string]$FrontmatterBlock)
    $lines = $FrontmatterBlock -split "`r?`n"
    $out = [System.Collections.Generic.List[string]]::new()
    $inPaths = $false
    foreach ($line in $lines) {
        if ($line -match '^\s*paths:\s*') {
            $inPaths = $true
            [void]$out.Add($line.TrimEnd())
            continue
        }
        if ($inPaths) {
            # Next top-level YAML key (not a list continuation).
            if ($line.Length -gt 0 -and $line -match '^\S' -and $line -notmatch '^\s*-\s') {
                break
            }
            [void]$out.Add($line.TrimEnd())
        }
    }
    return ($out -join "`n").TrimEnd()
}

if ((Test-Path -LiteralPath $claudeSkillRoot) -and (Test-Path -LiteralPath $skillRoot)) {
    Get-ChildItem -LiteralPath $claudeSkillRoot -Directory | ForEach-Object {
        $claudeFolderName = $_.Name
        if ($claudeFolderName -notmatch '^gameengine-') {
            return
        }
        if (-not $claudeToCodexSkillMap.ContainsKey($claudeFolderName)) {
            Write-Error "Claude skill '$claudeFolderName' is missing from claudeToCodexSkillMap in tools/check-agents.ps1; add the matching .agents/skills folder name."
        }
    }

    foreach ($claudeFolderName in @($claudeToCodexSkillMap.Keys)) {
        $codexFolderName = $claudeToCodexSkillMap[$claudeFolderName]
        $claudeDir = Join-Path $claudeSkillRoot $claudeFolderName
        $codexDir = Join-Path $skillRoot $codexFolderName
        if (-not (Test-Path -LiteralPath $claudeDir)) {
            Write-Error "Claude skill folder missing for parity entry '${claudeFolderName}': $claudeDir"
        }
        if (-not (Test-Path -LiteralPath $codexDir)) {
            Write-Error "Codex skill folder missing for parity entry '${claudeFolderName}' -> '${codexFolderName}': $codexDir"
        }
        $claudeSkillFile = Join-Path $claudeDir "SKILL.md"
        $codexSkillFile = Join-Path $codexDir "SKILL.md"
        if (-not (Test-Path -LiteralPath $claudeSkillFile)) {
            Write-Error "Missing Claude SKILL.md: $claudeSkillFile"
        }
        if (-not (Test-Path -LiteralPath $codexSkillFile)) {
            Write-Error "Missing Codex SKILL.md: $codexSkillFile"
        }

        $claudeFm = Get-SkillFrontmatterBlock -SkillMdPath $claudeSkillFile
        $codexFm = Get-SkillFrontmatterBlock -SkillMdPath $codexSkillFile
        $claudePaths = Get-FrontmatterPathsSection -FrontmatterBlock $claudeFm
        $codexPaths = Get-FrontmatterPathsSection -FrontmatterBlock $codexFm
        if ($claudePaths -ne $codexPaths) {
            Write-Error "Claude/Codex skill 'paths:' sections must match for parity ('$claudeFolderName' vs '$codexFolderName').`nClaude:`n$claudePaths`n`nCodex:`n$codexPaths"
        }
    }
}

# Cursor thin pointers: each `gameengine-*` folder (except Cursor-only entries) must mirror a Claude skill folder name.
$cursorThinPointerExceptions = @(
    "gameengine-cursor-baseline",
    "gameengine-plan-registry"
)
if ((Test-Path -LiteralPath $cursorSkillRoot) -and (Test-Path -LiteralPath $claudeSkillRoot)) {
    Get-ChildItem -LiteralPath $cursorSkillRoot -Directory | ForEach-Object {
        $cursorFolderName = $_.Name
        if ($cursorThinPointerExceptions -contains $cursorFolderName) {
            return
        }
        if ($cursorFolderName -notmatch '^gameengine-') {
            return
        }
        $pairedClaudeDir = Join-Path $claudeSkillRoot $cursorFolderName
        if (-not (Test-Path -LiteralPath $pairedClaudeDir)) {
            Write-Error "Cursor skill '$cursorFolderName' must have a matching .claude/skills/$cursorFolderName directory (thin-pointer parity)."
        }

        $cursorSkillFile = Join-Path $_.FullName "SKILL.md"
        $claudeSkillFile = Join-Path $pairedClaudeDir "SKILL.md"
        $cursorFm = Get-SkillFrontmatterBlock -SkillMdPath $cursorSkillFile
        $claudeFm = Get-SkillFrontmatterBlock -SkillMdPath $claudeSkillFile
        $cursorPaths = Get-FrontmatterPathsSection -FrontmatterBlock $cursorFm
        $claudePaths = Get-FrontmatterPathsSection -FrontmatterBlock $claudeFm
        if ($cursorPaths -ne $claudePaths) {
            Write-Error "Cursor thin skill 'paths:' must match Claude skill for '$cursorFolderName'.`nCursor:`n$cursorPaths`n`nClaude:`n$claudePaths"
        }
    }
}

if (Test-Path $claudeAgentRoot) {
    Get-ChildItem -Path $claudeAgentRoot -Filter "*.md" | ForEach-Object {
        Test-AgentFileSizeBudget `
            -Path $_.FullName `
            -MaxBytes (16 * 1024) `
            -Label ".claude/agents/$($_.Name)" `
            -Guidance "Keep subagents narrowly scoped and move reusable procedures to skills/docs."
        $head = (Get-Content -LiteralPath $_.FullName -TotalCount 8) -join "`n"
        if ($head -notmatch "---\s*\nname:\s*[a-z0-9-]+" -or $head -notmatch "description:\s*.+") {
            Write-Error "Claude agent frontmatter must include name and description: $($_.FullName)"
        }
    }
}

Write-Information "agent-config-check: ok" -InformationAction Continue
