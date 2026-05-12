---
name: gameengine-debugging
description: Debugs C++ engine crashes, incorrect runtime behavior, ownership and lifetime bugs, undefined behavior, and test failures. Use when investigating engine or unit test code under engine/ or tests/.
paths:
  - "engine/**/*.cpp"
  - "engine/**/*.hpp"
  - "tests/**/*.cpp"
  - "tools/check-tidy.ps1"
  - "tools/check-toolchain.ps1"
---

# GameEngine debugging (Cursor)

Full workflow lives in shared skills. Read these canonical files (ASCII paths):

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-debugging/SKILL.md` |
| Codex | `.agents/skills/cpp-engine-debugging/SKILL.md` |
| Baseline | `AGENTS.md` |

Validation: follow `AGENTS.md` debugging and `tools/check-toolchain.ps1` guidance before claiming root causes.
