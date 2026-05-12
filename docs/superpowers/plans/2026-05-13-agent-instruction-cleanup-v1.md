# Agent Instruction Cleanup v1 (2026-05-13)

> **For agentic workers:** REQUIRED SUB-SKILL: Use `gameengine-agent-integration`; use `superpowers:verification-before-completion` before reporting completion. Keep this as a docs/governance slice and do not repoint `currentActivePlan` away from the production-completion master plan.

**Goal:** Audit and tighten repository AI instruction surfaces so Codex, Claude Code, and Cursor guidance stays official-doc aligned, context-efficient, clean-break, and validation-backed.

**Context:** Official Codex documentation says `AGENTS.md` is discovered before work, layered by scope, and capped by `project_doc_max_bytes` at 32 KiB by default. Codex skills use progressive disclosure, Codex subagents consume extra tokens and spawn only on explicit request, Cursor project rules live in `.cursor/rules`, and Claude Code shared project settings belong in `.claude/settings.json` with sensitive reads denied. This repository already centralizes machine-readable truth in `engine/agent/manifest.fragments/` and reusable procedure detail in `docs/agent-operational-reference.md`, so always-loaded instructions should stay short and link outward.

**Constraints:**

- Preserve clean-break policy: no compatibility shims, deprecated aliases, or broad ready claims.
- Keep `AGENTS.md` under the default 32 KiB Codex project-doc budget.
- Keep Codex/Claude skill path frontmatter equivalent and Cursor skills as thin pointers.
- Do not loosen `.codex/rules` or `.claude/settings.json` approval gates.
- Do not hand-edit `engine/agent/manifest.json`; edit fragments and compose only if manifest content changes.

**Done When:**

- `AGENTS.md` is below 32 KiB and points long procedures to docs/skills/subagents/manifest.
- Agent-surface docs cite the official source expectations used for Codex, Claude Code, Cursor, and PowerShell script hygiene.
- Static checks enforce the most important context-budget invariant so drift is caught before completion.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record an explicit environment blocker.

## Task Checklist

- [x] Record official-source alignment findings.
- [x] Reduce always-loaded `AGENTS.md` while preserving enforced needles.
- [x] Add an automated `AGENTS.md` size guard.
- [x] Tighten docs/skills/rules/subagent guidance where it affects context use or official surfaces.
- [x] Run focused agent checks.
- [x] Run full validation and record evidence.

## Official-Source Alignment Findings

- Codex: keep `AGENTS.md` concise because the documented default `project_doc_max_bytes` is 32 KiB; use skills as progressive disclosure; spawn subagents only for explicit delegation because they add separate context and token cost; keep `.codex/rules` narrow with `match` / `not_match` examples.
- Claude Code: keep `CLAUDE.md` as an import surface and shared permissions in `.claude/settings.json`; do not loosen deny/ask gates for secrets, destructive commands, dependency bootstrap, mobile signing/smoke, force push, or prompt-gated PR state changes.
- Cursor: keep `.cursor/skills/gameengine-*` as thin pointers with current `paths:` frontmatter and keep `.cursor/rules/` aligned with `AGENTS.md`.
- PowerShell: keep tracked `tools/*.ps1` PowerShell 7 only, UTF-8 without BOM, approved-verb/PSScriptAnalyzer-friendly, `ShouldProcess` gated for host mutations, and use `Write-Information` instead of `Write-Host`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | `AGENTS.md` is 29,546 bytes, under the 32 KiB guard; agent-surface structure, skill parity, rules shape, and size guard passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command "if (Get-Module -ListAvailable -Name PSScriptAnalyzer) { Invoke-ScriptAnalyzer -Path tools/check-agents.ps1 -Severity Warning,Error } else { 'PSScriptAnalyzer not installed' }"` | Passed | Clean after replacing the existing `Write-Host` status line with `Write-Information`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | AI integration needles and manifest/doc parity passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full gate passed; host-gated Apple/Metal/mobile diagnostics remained diagnostic-only, build succeeded, and CTest reported 51/51 tests passing. |
