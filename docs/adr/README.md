# ADR Registry

Architecture Decision Records capture durable, accepted architecture choices. They are not implementation queues; use current docs, the plan registry, and `engine/agent/manifest.json` for active status.

## File Contract

Every ADR Markdown file must contain a `## Status` section near the top. Accepted ADRs remain in this directory while the decision is durable; superseded ADRs must say so in their status line and point to the replacement.

## ADRs At A Glance

| ADR | Status | Decision |
| --- | --- | --- |
| [0001: Core-First C++ Engine](0001-core-first-cpp-engine.md) | Accepted; historical initial milestone completed | C++23 core-first architecture before platform, renderer, editor, and runtime dependency expansion. |
| [0002: Agent Manifest Fragments and Compose-as-Canonical](0002-agent-manifest-fragments-compose.md) | Accepted | Edit `engine/agent/manifest.fragments/*.json`, compose committed `engine/agent/manifest.json`, and reject semantic drift in checks. |
| [0003: Directory Layout Clean-Break](0003-directory-layout-clean-break.md) | Accepted | Keep SDK-style CMake target boundaries while splitting `engine/tools` implementation ownership under one `MK_tools` public target. |
