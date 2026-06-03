#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

function Test-ToolScriptAutomaticVariableAssignment {
    param(
        [Parameter(Mandatory)][System.Management.Automation.Language.Ast]$Ast,
        [Parameter(Mandatory)][string]$Label
    )

    $forbiddenAutomaticVariableNames = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase
    )
    foreach ($name in @(
            "args",
            "foreach",
            "home",
            "input",
            "IsLinux",
            "IsMacOS",
            "IsWindows",
            "matches"
        )) {
        [void]$forbiddenAutomaticVariableNames.Add($name)
    }

    $assignments = $Ast.FindAll(
        {
            param($node)

            if ($node -isnot [System.Management.Automation.Language.AssignmentStatementAst]) {
                return $false
            }

            $variableExpression = $node.Left
            while ($variableExpression -is [System.Management.Automation.Language.ConvertExpressionAst]) {
                $variableExpression = $variableExpression.Child
            }

            if ($variableExpression -isnot [System.Management.Automation.Language.VariableExpressionAst]) {
                return $false
            }

            return $forbiddenAutomaticVariableNames.Contains($variableExpression.VariablePath.UserPath)
        },
        $true
    )

    foreach ($assignment in @($assignments)) {
        $variableExpression = $assignment.Left
        while ($variableExpression -is [System.Management.Automation.Language.ConvertExpressionAst]) {
            $variableExpression = $variableExpression.Child
        }
        $variableName = $variableExpression.VariablePath.UserPath
        Write-Error ("{0} must not assign to PowerShell automatic variable '{1}' at line {2}; use a distinct local name." -f $Label, $variableName, $assignment.Extent.StartLineNumber)
    }
}

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
            "insert_final_newline = true",
            "trim_trailing_whitespace = true"
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
    if ($lines.Length -lt 2 -or $lines[0] -ne "#requires -Version 7.0") {
        Write-Error "tools/$($script.Name) must declare exactly '#requires -Version 7.0' on line 1."
    }
    if ($lines.Length -lt 2 -or $lines[1] -ne "#requires -PSEdition Core") {
        Write-Error "tools/$($script.Name) must declare exactly '#requires -PSEdition Core' on line 2."
    }

    $parseTokens = $null
    $parseErrors = $null
    $parseAst = [System.Management.Automation.Language.Parser]::ParseFile($script.FullName, [ref]$parseTokens, [ref]$parseErrors)
    if (@($parseErrors).Count -gt 0) {
        $firstParseError = @($parseErrors)[0]
        Write-Error ("tools/{0} has PowerShell parse error at line {1}: {2}" -f $script.Name, $firstParseError.Extent.StartLineNumber, $firstParseError.Message)
    }

    Test-ToolScriptAutomaticVariableAssignment -Ast $parseAst -Label "tools/$($script.Name)"
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

function Assert-RequiredDirectoryChild {
    param(
        [Parameter(Mandatory)][string]$Root,
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Label,
        [string]$RequiredLeaf = ""
    )

    $childPath = Join-Path $Root $Name
    if (-not (Test-Path -LiteralPath $childPath -PathType Container)) {
        Write-Error "$Label missing required directory: $Name"
    }

    if (-not [string]::IsNullOrWhiteSpace($RequiredLeaf)) {
        $leafPath = Join-Path $childPath $RequiredLeaf
        if (-not (Test-Path -LiteralPath $leafPath -PathType Leaf)) {
            Write-Error "$Label required directory '$Name' missing $RequiredLeaf"
        }
    }
}

function Assert-RequiredArrayEntry {
    param(
        [Parameter(Mandatory)][object[]]$Actual,
        [Parameter(Mandatory)][string]$Expected,
        [Parameter(Mandatory)][string]$Label
    )

    if (@($Actual | Where-Object { [string]$_ -eq $Expected }).Count -eq 0) {
        Write-Error "$Label missing required entry: $Expected"
    }
}

$skillRoot = Join-Path $root ".agents/skills"
$agentRoot = Join-Path $root ".codex/agents"
$codexRuleRoot = Join-Path $root ".codex/rules"
$claudeSettingsPath = Join-Path $root ".claude/settings.json"
$claudeSkillRoot = Join-Path $root ".claude/skills"
$claudeAgentRoot = Join-Path $root ".claude/agents"
$cursorSkillRoot = Join-Path $root ".cursor/skills"
$cursorAgentRoot = Join-Path $root ".cursor/agents"
$aiSurfacesFragmentPath = Join-Path $root "engine/agent/manifest.fragments/011-aiSurfaces.json"
$aiSurfacesFragment = Get-Content -LiteralPath $aiSurfacesFragmentPath -Raw | ConvertFrom-Json

foreach ($skillName in @($aiSurfacesFragment.aiSurfaces.codex.requiredSkills)) {
    Assert-RequiredDirectoryChild -Root $skillRoot -Name ([string]$skillName) -Label "Codex required skills from 011-aiSurfaces.json" -RequiredLeaf "SKILL.md"
}
foreach ($skillName in @($aiSurfacesFragment.aiSurfaces.claudeCode.requiredSkills)) {
    Assert-RequiredDirectoryChild -Root $claudeSkillRoot -Name ([string]$skillName) -Label "Claude required skills from 011-aiSurfaces.json" -RequiredLeaf "SKILL.md"
}
foreach ($skillName in @($aiSurfacesFragment.aiSurfaces.cursor.requiredSkills)) {
    Assert-RequiredDirectoryChild -Root $cursorSkillRoot -Name ([string]$skillName) -Label "Cursor required skills from 011-aiSurfaces.json" -RequiredLeaf "SKILL.md"
}

if (Test-Path -LiteralPath $skillRoot) {
    Get-ChildItem -LiteralPath $skillRoot -Directory | ForEach-Object {
        $skillFile = Join-Path $_.FullName "SKILL.md"
        if (-not (Test-Path -LiteralPath $skillFile)) {
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

if (Test-Path -LiteralPath $agentRoot) {
    Get-ChildItem -LiteralPath $agentRoot -Filter "*.toml" | ForEach-Object {
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

if (Test-Path -LiteralPath $codexRuleRoot) {
    Get-ChildItem -LiteralPath $codexRuleRoot -Filter "*.rules" | ForEach-Object {
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
            'pattern\s*=\s*\["gh",\s*"pr",\s*"merge",\s*"--auto",\s*"--merge",\s*"--match-head-commit"\]',
            'pattern\s*=\s*\["pwsh",\s*"-NoProfile",\s*"-ExecutionPolicy",\s*"Bypass",\s*"-File",\s*"tools/check-publication-preflight\.ps1"\]',
            'pattern\s*=\s*\["pwsh",\s*"-NoProfile",\s*"-ExecutionPolicy",\s*"Bypass",\s*"-File",\s*"tools/remove-merged-worktree\.ps1"\]',
            'pattern\s*=\s*\["pwsh",\s*"-NoProfile",\s*"-ExecutionPolicy",\s*"Bypass",\s*"-File",\s*"tools/ready-task-pr\.ps1"\]'
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

$cursorRuleRoot = Join-Path $root ".cursor/rules"
if (Test-Path -LiteralPath $cursorRuleRoot) {
    Get-ChildItem -LiteralPath $cursorRuleRoot -Filter "*.mdc" -File -Recurse | ForEach-Object {
        $lineCount = @(Get-Content -LiteralPath $_.FullName).Count
        if ($lineCount -gt 500) {
            Write-Error "Cursor rule exceeds official 500-line guidance: $($_.FullName)"
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

    foreach ($requiredDenyEntry in @(
            "Bash(git push origin main:*)",
            "Bash(git push origin master:*)",
            "Bash(git push --force origin main:*)",
            "Bash(git push --force-with-lease origin main:*)",
            "Bash(git push origin HEAD:main:*)",
            "Read(./.env)",
            "Read(./.env.*)",
            "Read(./.mcp.json)",
            "Read(./secrets/**)",
            "Read(./.codex/auth.json)",
            "Read(./.claude/settings.local.json)",
            "Read(./platform/android/*.jks)",
            "Read(./platform/android/*.keystore)",
            "Read(./**/*.p12)",
            "Read(./**/*.pfx)",
            "Read(./**/*.pem)",
            "Read(./**/*.key)"
        )) {
        Assert-RequiredArrayEntry -Actual @($settings.permissions.deny) -Expected $requiredDenyEntry -Label ".claude/settings.json permissions.deny"
    }

    foreach ($requiredAskEntry in @(
            "Bash(git reset:*)",
            "Bash(git restore:*)",
            "Bash(git clean:*)",
            "Bash(git clone:*)",
            "Bash(git worktree remove:*)",
            "Bash(git push --force:*)",
            "Bash(git push --force-with-lease:*)",
            "Bash(gh pr merge --auto --merge --delete-branch:*)",
            "Bash(gh pr ready:*)",
            "Bash(gh pr close:*)",
            "Bash(gh pr reopen:*)",
            "Bash(rm:*)",
            "Bash(Remove-Item:*)",
            "Bash(curl:*)",
            "Bash(wget:*)",
            "Bash(Invoke-WebRequest:*)",
            "Bash(Invoke-RestMethod:*)",
            "Bash(pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1:*)",
            "Bash(pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-android.ps1:*)",
            "Bash(pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-android-package.ps1:*)",
            "Bash(pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-apple.ps1:*)",
            "Bash(pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-ios-package.ps1:*)"
        )) {
        Assert-RequiredArrayEntry -Actual @($settings.permissions.ask) -Expected $requiredAskEntry -Label ".claude/settings.json permissions.ask"
    }

    foreach ($requiredAllowEntry in @(
            "Bash(pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1:*)",
            "Bash(pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1:*)",
            "Bash(pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ready-task-pr.ps1:*)"
        )) {
        Assert-RequiredArrayEntry -Actual @($settings.permissions.allow) -Expected $requiredAllowEntry -Label ".claude/settings.json permissions.allow"
    }
}

if (Test-Path -LiteralPath $claudeSkillRoot) {
    Get-ChildItem -LiteralPath $claudeSkillRoot -Directory | ForEach-Object {
        $skillFile = Join-Path $_.FullName "SKILL.md"
        if (-not (Test-Path -LiteralPath $skillFile)) {
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

if (Test-Path -LiteralPath $cursorSkillRoot) {
    Get-ChildItem -LiteralPath $cursorSkillRoot -Directory | ForEach-Object {
        $skillFile = Join-Path $_.FullName "SKILL.md"
        if (-not (Test-Path -LiteralPath $skillFile)) {
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
    "gameengine-git-publication-preflight" = "gameengine-git-publication-preflight"
    "gameengine-license-audit"      = "license-audit"
    "gameengine-performance-optimization" = "performance-optimization-change"
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

function Get-AgentFrontmatterBlock {
    param([Parameter(Mandatory)][string]$AgentPath)
    $lines = Get-Content -LiteralPath $AgentPath
    if ($lines.Length -lt 2 -or $lines[0].Trim() -ne "---") {
        Write-Error "Agent must start with YAML frontmatter (---): $AgentPath"
    }
    $end = 1
    for (; $end -lt $lines.Length; $end++) {
        if ($lines[$end].Trim() -eq "---") {
            break
        }
    }
    if ($end -ge $lines.Length) {
        Write-Error "Agent frontmatter missing closing --- : $AgentPath"
    }
    return ($lines[1..($end - 1)] -join "`n")
}

function Test-CursorAgentFrontmatter {
    param(
        [Parameter(Mandatory)][string]$AgentPath,
        [Parameter(Mandatory)][string]$ExpectedName,
        [Parameter(Mandatory)][string[]]$ReadOnlyAgentNames,
        [Parameter(Mandatory)][string]$RequiredModel
    )

    $fm = Get-AgentFrontmatterBlock -AgentPath $AgentPath
    $nameMatch = [System.Text.RegularExpressions.Regex]::Match($fm, '(?m)^name:\s*([a-z0-9-]+)\s*$')
    if (-not $nameMatch.Success) {
        Write-Error "Cursor agent frontmatter must include 'name:' (lowercase letters, digits, hyphens only): $AgentPath"
    }
    $parsedName = $nameMatch.Groups[1].Value.Trim()
    if ($parsedName -ne $ExpectedName) {
        Write-Error "Cursor agent 'name:' ($parsedName) must match file base name '$ExpectedName': $AgentPath"
    }
    if ($fm -notmatch '(?m)^description:\s*\S') {
        Write-Error "Cursor agent frontmatter must include non-empty 'description:': $AgentPath"
    }
    if ($fm -notmatch "(?m)^model:\s*$([regex]::Escape($RequiredModel))\s*$") {
        Write-Error "Cursor agent frontmatter must declare model: ${RequiredModel}: $AgentPath"
    }
    if ($ReadOnlyAgentNames -contains $ExpectedName -and $fm -notmatch '(?m)^readonly:\s*true\s*$') {
        Write-Error "Cursor read-only agent must declare readonly: true: $AgentPath"
    }
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

        $cursorLineCount = (Get-Content -LiteralPath $cursorSkillFile).Count
        if ($cursorLineCount -gt 45) {
            Write-Error "Cursor thin-pointer skill '$cursorFolderName' exceeds 45 lines ($cursorLineCount). Keep router skills concise; use Claude/Codex canonical bodies."
        }

        $disableModelInvocation = $null
        if ($cursorFm -match '(?m)^disable-model-invocation:\s*(true|false)\s*$') {
            $disableModelInvocation = $Matches[1]
        }
        if ($null -ne $disableModelInvocation -and $disableModelInvocation -eq 'true') {
            Write-Error "Cursor skill '$cursorFolderName' must not set disable-model-invocation: true (reserved for gameengine-plan-registry)."
        }
    }
}

$cursorPlanRegistrySkillFile = Join-Path $cursorSkillRoot "gameengine-plan-registry/SKILL.md"
if (Test-Path -LiteralPath $cursorPlanRegistrySkillFile) {
    $planRegistryFm = Get-SkillFrontmatterBlock -SkillMdPath $cursorPlanRegistrySkillFile
    if ($planRegistryFm -notmatch '(?m)^disable-model-invocation:\s*true\s*$') {
        Write-Error "Cursor skill 'gameengine-plan-registry' must set disable-model-invocation: true."
    }
}

$aiSurfacesFragmentPath = Join-Path $root "engine/agent/manifest.fragments/011-aiSurfaces.json"
if (-not (Test-Path -LiteralPath $aiSurfacesFragmentPath)) {
    Write-Error "Missing manifest fragment for Cursor agent list: engine/agent/manifest.fragments/011-aiSurfaces.json"
}
$aiSurfacesFragment = Get-Content -LiteralPath $aiSurfacesFragmentPath -Raw | ConvertFrom-Json
$requiredCursorAgents = @($aiSurfacesFragment.aiSurfaces.cursor.requiredAgents)
$cursorReadOnlyAgents = @($aiSurfacesFragment.aiSurfaces.cursor.readOnlyAgents)
$cursorRequiredAgentModel = "composer-2.5-fast"

if (Test-Path -LiteralPath $claudeAgentRoot) {
    Get-ChildItem -LiteralPath $claudeAgentRoot -Filter "*.md" | ForEach-Object {
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

if (-not (Test-Path -LiteralPath $cursorAgentRoot)) {
    Write-Error "Cursor project subagents must live under .cursor/agents per Cursor Subagents documentation."
}
foreach ($agentName in $requiredCursorAgents) {
    $codexAgentPath = Join-Path $agentRoot "$agentName.toml"
    if (-not (Test-Path -LiteralPath $codexAgentPath)) {
        Write-Error "Shared agent missing required Codex project subagent: .codex/agents/$agentName.toml"
    }
    $claudeAgentPath = Join-Path $claudeAgentRoot "$agentName.md"
    if (-not (Test-Path -LiteralPath $claudeAgentPath)) {
        Write-Error "Shared agent missing required Claude project subagent: .claude/agents/$agentName.md"
    }
    $cursorAgentPath = Join-Path $cursorAgentRoot "$agentName.md"
    if (-not (Test-Path -LiteralPath $cursorAgentPath)) {
        Write-Error "Cursor agent missing required project subagent: .cursor/agents/$agentName.md"
    }
}
if (Test-Path -LiteralPath $cursorAgentRoot) {
    Get-ChildItem -LiteralPath $cursorAgentRoot -Filter "*.md" | ForEach-Object {
        $agentName = [System.IO.Path]::GetFileNameWithoutExtension($_.Name)
        Test-AgentFileSizeBudget `
            -Path $_.FullName `
            -MaxBytes (16 * 1024) `
            -Label ".cursor/agents/$($_.Name)" `
            -Guidance "Keep Cursor subagents narrowly scoped and move reusable procedures to skills/docs."
        Test-CursorAgentFrontmatter -AgentPath $_.FullName -ExpectedName $agentName -ReadOnlyAgentNames $cursorReadOnlyAgents -RequiredModel $cursorRequiredAgentModel
        if (Test-Path -LiteralPath $claudeAgentRoot) {
            $pairedClaudeAgentPath = Join-Path $claudeAgentRoot "$agentName.md"
            if (-not (Test-Path -LiteralPath $pairedClaudeAgentPath)) {
                Write-Error "Cursor agent '$agentName' must have a matching .claude/agents/$agentName.md role for cross-tool parity."
            }
        }
        if (Test-Path -LiteralPath $agentRoot) {
            $pairedCodexAgentPath = Join-Path $agentRoot "$agentName.toml"
            if (-not (Test-Path -LiteralPath $pairedCodexAgentPath)) {
                Write-Error "Cursor agent '$agentName' must have a matching .codex/agents/$agentName.toml role for cross-tool parity."
            }
        }
    }
}

Write-Information "agent-config-check: ok" -InformationAction Continue
