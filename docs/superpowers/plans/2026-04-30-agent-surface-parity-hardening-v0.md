# Agent Surface Parity Hardening v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement future follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Keep Codex and Claude Code agent-facing surfaces behaviorally equivalent and statically validated.

**Architecture:** Preserve `AGENTS.md` as the short shared baseline, keep reusable behavior in skills, and keep specialized parallel work in project subagents. `engine/agent/manifest.json` records required skills/agents/read-only roles, while `tools/check-ai-integration.ps1` verifies the contract.

**Tech Stack:** Markdown/TOML project instructions, PowerShell validation, `engine/agent/manifest.json`, Codex `.agents/skills` and `.codex/agents`, Claude `.claude/skills`, `.claude/rules`, and `.claude/agents`.

---

## Goal

Harden the repository AI integration layer after a full review of `AGENTS.md`, skills, rules, subagents, docs, manifest, and validation scripts.

## Context

Official Codex guidance uses repo `AGENTS.md`, repo-scoped skills, and project `.codex/agents` TOML files with explicit `name`, `description`, and `developer_instructions`. Official Claude Code guidance uses `CLAUDE.md`, `.claude/skills`, `.claude/rules`, and `.claude/agents` Markdown files with YAML frontmatter. Existing docs mentioned `explorer` as a read-only role, but the project did not define project-level explorer agents or validate that role. Codex had agent-integration and CMake skills that Claude lacked, while Claude had a general feature skill that Codex lacked.

## Constraints

- Keep always-loaded instructions concise; move procedures into skills.
- Keep reviewer, explorer, architect, and auditor subagents read-only.
- Do not mark planned or host-gated engine capability as implemented.
- Keep Codex and Claude Code behaviorally equivalent.
- Update validation so future drift fails in `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.

## Tasks

- [x] Add `tools/check-ai-integration.ps1` coverage for required Codex/Claude skills, required agents, read-only explorer agents, and `aiSurfaces` manifest arrays.
- [x] Add Codex `gameengine-feature` skill.
- [x] Add Claude `gameengine-agent-integration` and `gameengine-cmake-build-system` skills.
- [x] Add read-only Codex and Claude `explorer` project subagents.
- [x] Update `AGENTS.md`, `.claude/rules/ai-agent-integration.md`, `docs/ai-integration.md`, `docs/workflows.md`, and `engine/agent/manifest.json`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Validation

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS with existing diagnostic-only host gates for Metal shader tools, Apple packaging on this Windows host, and strict tidy compile database availability.

## Done When

- Codex and Claude Code have equivalent project skills for general feature work, CMake/build-system work, agent integration, debugging, editor, game development, license audit, and rendering.
- Both tools expose the same project subagent roles, with explorer/reviewer/architect/auditor constrained to read-only behavior.
- `engine/agent/manifest.json` machine-readably lists required skills, agents, and read-only roles.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or concrete host blockers are recorded.
