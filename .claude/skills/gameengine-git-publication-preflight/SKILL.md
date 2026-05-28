---
name: gameengine-git-publication-preflight
description: Run and interpret the repository Git/GitHub publication preflight before staging, push, PR creation/update, ready conversion, auto-merge registration, or when diagnosing linked-worktree index.lock, GitHub CLI auth/config, network, or PR head SHA blockers.
paths:
  - "AGENTS.md"
  - "docs/workflows.md"
  - "tools/check-publication-preflight.ps1"
  - ".codex/rules/gameengine.rules"
  - ".claude/settings.json"
---

# GameEngine Git Publication Preflight

## Workflow

Run the guarded wrapper first:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1 -PullRequest <pr> -RequireClean
```

Proceed to staging, push, PR creation/update, `tools/ready-task-pr.ps1`, or `gh pr merge --auto --merge --match-head-commit <headRefOid>` only when it prints `publication-preflight: ok`.

## Manual Evidence

Use these probes only to explain `publication-preflight: blocked`:

```powershell
whoami
git status --short --branch
git rev-parse --path-format=absolute --git-path index.lock
git update-index -q --refresh
Test-NetConnection github.com -Port 443
gh auth status
gh pr view <pr> --json headRefOid,statusCheckRollup,url
```

If `index.lock`, `git update-index -q --refresh`, `Test-NetConnection`, GitHub CLI config, `gh auth status`, or `gh pr view` is blocked, switch session/user/host context. Do not publish by creating a publication temp clone, hand-writing GitHub REST/MCP commits, broadening rules, or bypassing GitHub Flow.
