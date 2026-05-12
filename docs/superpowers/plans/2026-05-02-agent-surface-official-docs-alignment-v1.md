# Agent Surface Official Docs Alignment v1 (2026-05-02)

## Goal

Align GameEngine's Codex and Claude Code guidance with official agent-tool configuration surfaces while keeping the current engine production slice unchanged.

## Context

The existing agent contract already covered `AGENTS.md`, repo skills, custom subagents, Claude rule files, and `engine/agent/manifest.json`. The gap was governance around newer official surfaces: Codex project-local `.codex/rules/*.rules`, Claude Code `.claude/settings.json`, Claude Code memory imports for project rule files, and live official documentation policy for OpenAI/Codex and Claude Code behavior.

## Constraints

- Do not advance or overwrite `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.
- Keep engine capability claims unchanged.
- Keep read-only subagents read-only and builder/fixer roles write-capable only when their task expects edits.
- Keep secrets, signing material, and personal MCP/API configuration out of the repository.
- Keep default completion validation through `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Design

Add `.codex/rules/gameengine.rules` as a narrow prompt-biased command policy for dependency bootstrap, mobile packaging/signing/smoke lanes, destructive Git operations, `rm`, and direct network downloads. The rules use `match` and `not_match` examples and intentionally avoid `allow` decisions.

Add `.claude/settings.json` as the shared Claude Code project permission surface. It asks before destructive, network, dependency-bootstrap, and mobile signing/smoke commands, and denies likely secret-bearing file reads such as `.env`, `secrets/**`, Android keystores, and PKCS12/PFX files.

Make `CLAUDE.md` explicitly import the project rule files using Claude Code memory imports. Update `AGENTS.md`, `docs/ai-integration.md`, `docs/workflows.md`, the agent-integration skills, and Claude AI integration rule guidance so Codex and Claude Code stay behaviorally equivalent.

Follow-up hardening in the same docs/governance slice adds the official Claude Code settings JSON schema, keeps `.claude/settings.local.json`, `.mcp.json`, and `AGENTS.override.md` uncommitted, and expands the Codex/Claude approval surfaces to cover `git restore`, `git checkout`, PowerShell `Remove-Item`, and `Invoke-RestMethod`.

Expose the new surfaces through `engine/agent/manifest.json.aiSurfaces` and `tools/agent-context.ps1`, then harden `tools/check-agents.ps1` and `tools/check-ai-integration.ps1` so future edits cannot silently drop the rules, settings, official documentation policy, or context output.

## Done When

- `AGENTS.md`, `CLAUDE.md`, docs, repo skills, Claude rules, `engine/agent/manifest.json`, and agent validation scripts describe the same Codex/Claude surfaces.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` reports Codex rules and Claude settings.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passes with checks for `.codex/rules/gameengine.rules` and `.claude/settings.json`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete host blocker.

## Evidence

- GREEN: Baseline `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed before edits.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after adding Codex rules, Claude settings, manifest/context exposure, and static checks.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` reported `.codex/rules/gameengine.rules`, `.claude/settings.json`, and `officialAiToolDocs`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. It included license, agent config, AI integration, JSON schema, validation recipe runner, dependency policy, vcpkg environment, toolchain, C++ standard, shader/mobile diagnostic gates, public API boundary, tidy config, CMake configure/build, generated MSVC C++23 mode, and 28/28 CTest tests passing. Diagnostic-only host blockers remained Metal tools missing, Apple packaging requiring macOS/Xcode, Android release signing not configured, Android device smoke not connected, and strict clang-tidy compile database availability.
- GREEN: Final `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after recording validation evidence.
- GREEN: Codex `execpolicy check` reported `decision: "prompt"` for `Remove-Item -LiteralPath out -Recurse -Force`, `git restore .`, and `Invoke-RestMethod -Uri https://example.com/api`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after the schema/local-override/git/PowerShell/network hardening.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` still reported `.codex/rules/gameengine.rules`, `.claude/settings.json`, and `officialAiToolDocs`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after the schema/local-override/git/PowerShell/network hardening. CTest reported 28/28 passing; diagnostic-only host blockers remained Metal tools missing, Apple packaging requiring macOS/Xcode, Android release signing not configured, Android device smoke not connected, and strict clang-tidy compile database availability.
