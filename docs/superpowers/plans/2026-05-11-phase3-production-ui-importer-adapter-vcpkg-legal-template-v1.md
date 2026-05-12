# Phase 3 Production UI Importer Adapter Vcpkg Legal Template v1 (2026-05-11)

**Plan ID:** `phase3-production-ui-importer-adapter-vcpkg-legal-template-v1`  
**Gap:** `production-ui-importer-platform-adapters`  
**Parent:** [2026-05-10-unsupported-production-gaps-orchestration-program-v1.md](2026-05-10-unsupported-production-gaps-orchestration-program-v1.md)  
**Status:** Template / program ledger  

## Goal

Provide a **repeatable slice template** for shipping real `MK_ui` adapter implementations (text shaping, font rasterization, IME bridges, accessibility publication, image codecs, importers) without breaking repository dependency policy.

## Per-adapter slice checklist (copy into each dated child plan)

1. **Capability** — which `I*` adapter boundary is implemented; what remains explicitly unsupported.
2. **Dependencies** — `vcpkg.json` feature flags, `builtin-baseline` pin discipline, `VCPKG_MANIFEST_INSTALL=OFF` preset compliance.
3. **Legal** — `docs/legal-and-licensing.md`, `docs/dependencies.md`, `THIRD_PARTY_NOTICES.md` updates in the **same** task.
4. **Tests** — host-independent unit tests for planning/validation paths; optional `MK_tools` / `MK_platform_*` integration tests with explicit skips when tools missing.
5. **Manifest** — `engine/agent/manifest.json` honest notes; no broad `ready` without evidence.
6. **Validation** — `tools/validate.ps1` at slice close; `tools/check-dependency-policy.ps1` when deps change.

## Architecture constraints

- Keep `MK_ui` free of SDL / Dear ImGui / OS headers; adapters live in `MK_platform_*`, `MK_tools`, or audited bridges as today.
- Prefer official SDKs or vcpkg ports with SPDX-compatible records.

## Linked slices

| Adapter topic | Plan | Status |
| --- | --- | --- |
| (pending) | — | Add row per landed adapter slice |
