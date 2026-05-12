# ADR 0002: Agent Manifest Fragments and Compose-as-Canonical

## Status

Accepted.

## Context

`engine/agent/manifest.json` is the machine-readable AI and tooling contract. It grew to thousands of lines in a single file, which increases merge conflicts, review cost, and the size of default agent context dumps.

## Decision

1. **Source of truth** is the set of JSON fragment files under `engine/agent/manifest.fragments/`. Each fragment contains one or more top-level keys of the composed manifest. Files are merged in **lexicographic order** by filename (use numeric prefixes to control order).
2. **Canonical artifact** `engine/agent/manifest.json` remains the committed, single-file contract consumed by existing scripts, CI, and `tools/agent-context.ps1`. It **must** equal the semantic output of `tools/compose-agent-manifest.ps1` (deep JSON equality, not necessarily byte-identical whitespace).
3. **Workflow**: edit fragments only; run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` to refresh `manifest.json`; `tools/check-json-contracts.ps1` fails when the canonical file does not match the composed result.

## Rationale

- Keeps backward compatibility for any tool that reads only `engine/agent/manifest.json`.
- Splits ownership by concern (commands, modules, production loop, etc.) without introducing a new runtime dependency for JSON Pointer resolution.
- Semantic equality avoids brittle byte-level diffs from serializer upgrades while still enforcing SSOT.

## Consequences

- Contributors must not hand-edit `manifest.json` for durable changes; edits belong in `manifest.fragments/`.
- Release/install layouts that copy `manifest.json` unchanged require no change.
- A follow-up improvement is optional deterministic formatting of `-Write` output to minimize diffs.

## Rollback

Remove `manifest.fragments/`, delete `compose-agent-manifest.ps1` and the compose check in `check-json-contracts.ps1`, and continue maintaining `manifest.json` directly.
