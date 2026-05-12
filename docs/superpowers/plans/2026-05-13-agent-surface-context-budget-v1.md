# Agent Surface Context Budget v1 (2026-05-13)

> **For agentic workers:** REQUIRED SUB-SKILL: Use `gameengine-agent-integration`; use `superpowers:verification-before-completion` before reporting completion. This is a docs/governance slice and must not repoint `currentActivePlan`.

**Goal:** Reduce skill/subagent context and rate-limit cost while preserving official best-practice alignment, clean-break implementation guidance, and validation-backed agent-surface parity.

**Context:** Codex skills use progressive disclosure: the model sees skill metadata first and loads `SKILL.md` only when selected. Large `SKILL.md` bodies still become costly after selection, so long historical/procedural detail should live in `references/` and be loaded only when the task needs it. Codex subagents add separate context/token cost and should remain short role contracts unless explicitly delegated. Codex rules should stay narrow, covered by examples, and guarded by static checks.

**Constraints:**

- Keep `AGENTS.md` under the 32 KiB guard added in the previous docs/governance slice.
- Preserve Codex/Claude/Cursor behavioral parity and existing path frontmatter.
- Do not loosen `.codex/rules` or `.claude/settings.json`.
- Do not hand-edit `engine/agent/manifest.json`.
- Preserve `tools/check-ai-integration.ps1` needle coverage by treating `SKILL.md` plus `references/*.md` as one checked skill surface.

**Done When:**

- The largest Codex/Claude skills become short router files with detailed guidance moved to `references/full-guidance.md`.
- `tools/check-ai-integration.ps1` reads skill reference files when checking guidance needles.
- `tools/check-agents.ps1` enforces reasonable initial-load budgets for `SKILL.md` and custom agent instruction files.
- The plan registry records this docs/governance slice.
- `check-agents`, `check-ai-integration`, and `validate.ps1` pass or record an explicit environment blocker.

## Task Checklist

- [x] Move large skill bodies into references and replace them with concise routers.
- [x] Keep Codex and Claude skill twins equivalent.
- [x] Update validation scripts for reference-aware guidance checks and size budgets.
- [x] Update docs/registry evidence.
- [x] Run focused agent checks and full validation.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Pass | Skill/subagent size budgets and parity. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command 'if (Get-Module -ListAvailable -Name PSScriptAnalyzer) { foreach ($scriptPath in @("tools/check-agents.ps1", "tools/check-ai-integration.ps1")) { Invoke-ScriptAnalyzer -Path $scriptPath -Severity Warning,Error } } else { "PSScriptAnalyzer not installed" }'` | Pass | No warnings or errors on touched PowerShell scripts. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | Reference-aware guidance needles. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Slice-closing validation; 51/51 CTest tests passed. Metal/Apple checks remained diagnostic/host-gated on Windows as expected. |
