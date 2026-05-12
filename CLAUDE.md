@AGENTS.md
@.claude/rules/ai-agent-integration.md
@.claude/rules/cpp-engine.md

## Claude Code

- This file intentionally imports `AGENTS.md` so Claude Code and Codex share the same baseline instructions.
- Project rules are imported above with Claude Code memory imports.
- Shared project permissions live in `.claude/settings.json`.
- Project skills live in `.claude/skills/`.
- Project subagents live in `.claude/agents/`.
- Cursor users also load `.cursor/rules/*.mdc` and `.cursor/skills/` (thin `gameengine-*` pointers plus Cursor-only entries such as `gameengine-cursor-baseline`); shared truth remains `AGENTS.md` and the Codex/Claude skill pairs—update pointers when adding or renaming shared skills.
- Before generating game code or changing engine APIs, run or inspect `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` (optional `-ContextProfile Minimal|Standard|Full`, default `Full`). When changing the **engine** agent contract, edit `engine/agent/manifest.fragments/*.json` and run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`; do not hand-edit `engine/agent/manifest.json`.
- Repository `tools/*.ps1` scripts declare `#requires -Version 7.0` and `#requires -PSEdition Core` (see `AGENTS.md`). Use the `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/<script>.ps1` form (PowerShell 7 Core only, not Windows PowerShell 5.1).
- Keep this file short. Put reusable procedures in skills and path-specific guidance in rules.
