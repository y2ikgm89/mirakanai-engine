---
name: gameengine-agent-integration
description: Keeps Codex, Claude, Cursor, manifests, skills, rules, and validation scripts aligned with the engine agent contract. Use for AGENTS.md, manifest fragments, check-ai-integration needles, or cross-tool agent surfaces.
paths:
  - "AGENTS.md"
  - "CLAUDE.md"
  - "engine/agent/**"
  - "schemas/**"
  - "tools/agent-context.ps1"
  - "tools/check-ai-integration*.ps1"
  - "tools/check-json-contracts*.ps1"
  - "tools/static-contract-ledger.ps1"
  - "tools/check-agents.ps1"
  - "tools/check-text-format.ps1"
  - "tools/check-text-format-contract.ps1"
  - "tools/format-text.ps1"
  - "tools/text-format-core.ps1"
  - "tools/prepare-worktree.ps1"
  - "tools/check-publication-preflight.ps1"
  - "tools/remove-merged-worktree.ps1"
  - "tools/ready-task-pr.ps1"
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

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-agent-integration/SKILL.md` |
| Codex | `.agents/skills/gameengine-agent-integration/SKILL.md` |
| Baseline | `AGENTS.md` |
| Consistency checklist | `docs/workflows.md` (**Repository consistency checklist**) |

Read the Claude skill for manifest compose, publication preflight, worktree cleanup, drift checks, stale deferred-status cleanup, `check-ai-integration` / `check-json-contracts` Needle updates, and [static-contract chapter ownership](.claude/skills/gameengine-agent-integration/references/static-contract-chapters.md). Run `tools/compose-agent-manifest.ps1 -Write` after manifest fragment edits; include an agent-surface drift check before completion.

Publication: run `tools/check-publication-preflight.ps1` before staging/push/PR/merge; if `publication-preflight: blocked`, switch session/host context.
