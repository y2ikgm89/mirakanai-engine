---
name: gameengine-agent-integration
description: Keeps Codex, Claude Code, Cursor, manifests, skills, rules, and validation scripts aligned with the engine agent contract. Use when editing AGENTS.md, agent manifest fragments, check-ai-integration needles, or cross-tool agent surfaces.
paths:
  - "AGENTS.md"
  - "CLAUDE.md"
  - "engine/agent/**"
  - "schemas/**"
  - "tools/agent-context.ps1"
  - "tools/check-ai-integration.ps1"
  - "tools/check-agents.ps1"
  - "tools/compose-agent-manifest.ps1"
  - ".agents/**"
  - ".claude/**"
  - ".cursor/**"
  - ".codex/**"
  - "docs/workflows.md"
  - "docs/ai-integration.md"
  - "docs/agent-operational-reference.md"
---

# GameEngine agent integration (Cursor)

Full workflow lives in shared skills. Read these canonical files (ASCII paths):

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-agent-integration/SKILL.md` |
| Codex | `.agents/skills/gameengine-agent-integration/SKILL.md` |
| Baseline | `AGENTS.md` |
| Consistency checklist | `docs/workflows.md` (**Repository consistency checklist**) |

`tools/*.ps1` hygiene also includes **UTF-8 without BOM**, contiguous `#requires`, **approved-verb** `function` names, and **PSScriptAnalyzer-friendly** patterns (no automatic-variable reuse such as **`$input` / `$matches`**—including **`foreach ($input in ...)`** and **`$matches = [Regex]::Matches(...)`**—no shadowing **`$Is*`** platform automatics, **`$null =`** for intentional discard, **`Write-Information -InformationAction Continue`** instead of **`Write-Host`** for non-pipeline status, non-empty **`catch`**, **`ShouldProcess`** on host-mutating **`Set-*`** helpers—see checklist step 2 and `AGENTS.md` **Repository command entrypoints**). Run **`Invoke-ScriptAnalyzer`** on edited scripts when the module is installed.

Machine-readable **canonical** contract: `engine/agent/manifest.json` (compose output from `engine/agent/manifest.fragments/` via `tools/compose-agent-manifest.ps1`). New `.claude/skills/gameengine-*` topics require a Codex twin registered in `tools/check-agents.ps1` (`claudeToCodexSkillMap`) and a matching thin `.cursor/skills/gameengine-*` folder unless intentionally Cursor-only (`gameengine-cursor-baseline`, `gameengine-plan-registry`). Keep Codex/Claude/Cursor surfaces aligned when workflow commands change, including Git/GitHub commit, push, merge/delete-branch, post-merge remote-tracking cleanup, force-push, and PR publishing gates. Treat Codex command policy as session-scoped: `.codex/rules` edits may need policy reload or a new session before newly allowed commands are available. If prompt-gated PR commands such as `gh pr create` are blocked because approvals are unavailable, keep the branch pushed and hand off to GitHub Web/Desktop or an approval-capable session.

Git/GitHub authentication stays host-local through Git Credential Manager, GitHub CLI, SSH agent, or a browser session; do not add repository requirements for `GITHUB_TOKEN` or personal access tokens. If warnings mention missing helpers such as `credential-manager-core`, inspect `git config --show-origin --get-all credential.helper`, prefer current Git for Windows GCM helper `manager`, and fix host/user Git config rather than adding repository overrides.

When adding retained editor UI ids or literals enforced by `tools/check-ai-integration.ps1`, extend scoped Needles and sibling skill/manifest text together (see canonical skill §When Changing Integration item 17).

When **`editor/core`** aggregates such as **`ScenePrefabInstanceRefreshPolicy`** gain fields, update **`tests/unit/editor_core_tests.cpp`** designated initializers in the same task (see canonical skill **Rules** and `docs/cpp-style.md` **Unit tests**).

Canonical skill also documents `oneDotZeroCloseoutTier` on each `unsupportedProductionGaps` row and the production-completion master plan **HTML comment** tail used as substring evidence for `check-ai-integration.ps1`; do not strip that tail without running full `validate`.

`MK_tools` implementation `.cpp` paths are under `engine/tools/{shader,gltf,asset,scene}/` (not a flat `engine/tools/src/`). Changing them requires updating `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, and `engine/tools/*/CMakeLists.txt` together; see `docs/specs/2026-05-11-directory-layout-target-v1.md`. New `mirakana/*` deps in a cluster require the matching `MK_*` on that cluster's `OBJECT` target (spec invariant 4).

Windows D3D12 GPU capture: default **operator PIX + AI analysis** pattern is `docs/ai-integration.md` § **Recommended workflow (operator PIX, AI analysis)** (see full `.claude/skills/gameengine-agent-integration/SKILL.md`).
