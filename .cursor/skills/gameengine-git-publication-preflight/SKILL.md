---
name: gameengine-git-publication-preflight
description: Use before publishing GameEngine branches or diagnosing linked-worktree Git/GitHub auth blockers.
paths:
  - "AGENTS.md"
  - "docs/workflows.md"
  - "tools/check-publication-preflight.ps1"
  - ".codex/rules/gameengine.rules"
  - ".claude/settings.json"
---

# GameEngine Git Publication Preflight

Use `.claude/skills/gameengine-git-publication-preflight/SKILL.md` and `.agents/skills/gameengine-git-publication-preflight/SKILL.md` as the canonical workflow. Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1 [-PullRequest <pr>] [-RequireClean]` before staging, push, PR creation/update, ready conversion, or auto-merge registration. If `publication-preflight: blocked` appears, switch session/user/host context; do not create a publication temp clone or bypass GitHub Flow.
