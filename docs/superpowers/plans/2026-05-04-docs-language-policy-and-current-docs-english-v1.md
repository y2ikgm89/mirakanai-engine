# Docs Language Policy And Current Docs English v1 Implementation Plan (2026-05-04)

> **For agentic workers:** REQUIRED SUB-SKILL: Use `gameengine-agent-integration` and `superpowers:executing-plans` for inline execution. Use subagents only when the user explicitly asks for delegation. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Establish an English-first policy for current documentation and translate the current-truth Japanese documentation that is not merely historical evidence.

**Architecture:** This is a docs/governance slice. It updates the documentation entrypoint, workflow lifecycle rules, the current build/capability docs, and the plan registry while leaving dated historical plan/spec records in place.

**Tech Stack:** Markdown, `rg`, repository static checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Context

- The project uses English API names, C++/CMake/vcpkg terminology, CI job names, and machine-readable agent contracts.
- Some dated plans and older evidence records were written in Japanese.
- Current-truth docs and agent-facing navigation are read by both humans and AI agents, so they should be English by default.

## Constraints

- Do not change engine behavior, CMake behavior, validation scripts, or manifest active-slice pointers.
- Do not mass-translate every historical dated plan or spec; preserve implementation evidence in place.
- Translate historical text only when it is promoted into current-truth docs, actively edited for a new slice, or named by the active manifest/registry pointers.
- Keep this docs/governance slice separate from `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.

## Files

- Modify: `docs/README.md`
- Modify: `docs/workflows.md`
- Modify: `docs/building.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/superpowers/plans/README.md`
- Create: `docs/superpowers/plans/2026-05-04-docs-language-policy-and-current-docs-english-v1.md`

## Implementation Steps

- [x] Add the English-first documentation language policy to `docs/README.md`.
- [x] Add the same lifecycle rule to `docs/workflows.md`.
- [x] Translate `docs/building.md` into English while preserving the CMake/install/package contract.
- [x] Translate the Japanese `cook-registered-source-assets` capability bullets in `docs/current-capabilities.md`.
- [x] Translate current plan-registry Japanese rows and add this docs/governance slice to the registry.
- [x] Run focused static checks for Japanese text in current docs and plan-registry text.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Done When

- Current-truth docs and the plan registry state the English-first policy.
- Top-level current docs outside historical specs/plans no longer contain Japanese operational text.
- The active engine slice remains unchanged.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes, or any blocker is recorded with the exact failing command.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `rg -n "[\\p{Hiragana}\\p{Katakana}\\p{Han}]" docs --glob "!**/plans/**" --glob "!**/specs/**"` | PASS | No top-level current-doc Japanese text reported. |
| `rg -n "[\\p{Hiragana}\\p{Katakana}\\p{Han}]" docs/superpowers/plans/README.md` | PASS | No plan-registry Japanese text reported. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Windows MSVC `dev` preset, CTest 29/29 PASS, `validate: ok`. Metal shader tools and Apple packaging remain diagnostic-only host blockers on this Windows host. |
