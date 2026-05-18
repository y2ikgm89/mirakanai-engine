# Guarded PR Ready Automation v1 - 2026-05-19

## Goal

Allow unattended agents to move a task-owned draft pull request to ready-for-review after final validation evidence exists, without broadly allowing raw PR state changes.

## Context

- PR #120 is open, draft, hosted checks are green, `PR Gate` is `SUCCESS`, and `mergeStateStatus` is `CLEAN`, but current Codex policy keeps raw `gh pr ready` prompt-gated and approvals are unavailable.
- GitHub documents draft pull requests as non-mergeable until marked ready for review, and documents `gh pr ready` as the CLI command that marks a pull request ready for review.
- GitHub auto-merge is intended to proceed only after required reviews and required status checks are satisfied, so the repository should keep ready conversion behind a fresh PR preflight rather than a broad `gh pr ready` allow rule.

Official sources:

- GitHub CLI manual: <https://cli.github.com/manual/gh_pr_ready>
- GitHub Docs, changing PR stage: <https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/changing-the-stage-of-a-pull-request>
- GitHub Docs, pull requests and draft behavior: <https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/about-pull-requests>
- GitHub Docs, auto-merge: <https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/incorporating-changes-from-a-pull-request/automatically-merging-a-pull-request>

## Constraints

- Clean break: do not add a compatibility alias or broad raw `gh pr ready` allowance.
- Keep raw `gh pr ready`, `gh pr edit`, `gh pr close`, `gh pr reopen`, and immediate merge commands prompt-gated.
- The automatic path must be a repository-owned PowerShell 7 wrapper with narrow preflights and `ShouldProcess`.
- Require clean local worktree, current local/remote head, task-owned branch, open draft PR, expected base, clean merge state, no requested changes, no failed or pending checks, and `PR Gate` `SUCCESS` before calling `gh pr ready`.
- Keep Codex, Claude, Cursor, docs, skills, rules, settings, and static checks synchronized.

## Plan

- [x] Add a RED static guard that requires the guarded ready wrapper and policy/docs synchronization.
- [x] Implement `tools/ready-task-pr.ps1` with strict local Git and live PR preflight.
- [x] Allow only the guarded wrapper in Codex/Claude command policy while leaving raw `gh pr ready` prompt-gated.
- [x] Update GitHub Flow docs and Codex/Claude/Cursor agent guidance.
- [x] Run focused static validation and update evidence.

## Done When

- `tools/ready-task-pr.ps1` rejects stale, dirty, non-task-owned, non-draft, non-main-base, conflicted, requested-changes, failed-check, pending-check, or missing-`PR Gate` PRs before mutation.
- Codex and Claude policy allow the wrapper but still require approval for raw PR state changes.
- Agent-facing docs and skills describe the new ready preflight and keep auto-merge registration separate.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and related static checks pass, or a concrete host blocker is recorded.

## Validation Evidence

- Official practice check: GitHub CLI documents `gh pr ready` as marking a pull request ready for review; GitHub Docs document changing a draft PR to ready for review; GitHub Flow keeps changes on a branch and PR before merge; GitHub auto-merge waits for merge requirements such as required reviews/checks.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed before implementation because `.agents/skills/gameengine-agent-integration/SKILL.md` did not contain `tools/ready-task-pr.ps1`.
- PASS: PowerShell parser check for `tools/ready-task-pr.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ready-task-pr.ps1 -PullRequest 120 -ExpectedBase main -WhatIf` refused to continue on a dirty worktree with `Refusing to mark PR ready: local worktree is not clean`.
- PASS: `Invoke-ScriptAnalyzer` on `tools/ready-task-pr.ps1`, `tools/check-agents.ps1`, and `tools/check-ai-integration-070-production-ledger.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- NOTE: Full `tools/validate.ps1` was not rerun because this slice changes docs/agent command policy and PowerShell static contracts only, with no C++/runtime/build/package/public API behavior change.
