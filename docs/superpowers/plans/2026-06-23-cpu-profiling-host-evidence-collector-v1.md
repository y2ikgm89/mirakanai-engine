# 2026-06-23 CPU Profiling Host Evidence Collector v1 Implementation Plan

Plan ID: `cpu-profiling-host-evidence-collector-v1`
Status: Completed.

## Goal

Add a first-party, host-owned CPU profiling evidence collector/importer for the existing `host-cpu-profiling-matrix` recipe so operators can produce `evidence.json` bundles that `tools/check-cpu-profiling-host-evidence.ps1` validates without adding default profiler dependencies or claiming broad CPU optimization.

## Context

The production loop already exposes `aiOperableProductionLoop.cpuProfilingMatrix`, `cpu-profiling-matrix-host`, and `host-cpu-profiling-matrix` as host-gated evidence contracts. Before this slice, the repository could validate attached official-profiler bundles but did not provide a reviewed first-party script for shaping host-owned WPR/WPA/perf/VTune/uProf artifacts into the required evidence row.

Official source checks:

- Microsoft Learn: Windows Performance Toolkit consists of WPR and WPA, with Xperf retained as previous command-line tooling.
  <https://learn.microsoft.com/en-us/windows-hardware/test/wpt/>
- Microsoft Learn: Windows Performance Recorder records ETW system and application events for later WPA analysis.
  <https://learn.microsoft.com/en-us/windows-hardware/test/wpt/windows-performance-recorder>
- Microsoft Learn: Windows Performance Recorder technical reference covers WPR command-line options and profile XML.
  <https://learn.microsoft.com/en-us/windows-hardware/test/wpt/wpr-reference>
- Context7 was queried for `Windows Performance Toolkit`; no WPT-specific library was available, so Microsoft Learn is the authoritative source. Context7 did resolve `/microsoftdocs/windows-powershell-docs`, but WPT behavior remains sourced from Microsoft Learn.

## Constraints

- Do not execute arbitrary workload commands.
- Do not start or stop WPR sessions from default validation.
- Do not make WPT, perf, VTune, uProf, CUDA, HIP, SYCL, PGO/LTO, Linux affinity, NUMA, or vendor-profiler installation a default validation dependency.
- Do not mark `cpu-profiling-matrix-host` ready from synthetic or local diagnostic-only evidence.
- Do not claim broad CPU/GPU/memory optimization, broad SIMD, scheduler-policy readiness, data-layout readiness, Linux affinity execution, NUMA execution, or PGO/LTO readiness.
- Keep artifacts host-owned and referenced by safe repo-relative paths under the selected evidence root.

## Implementation

- [x] Add `tools/collect-cpu-profiling-host-evidence.ps1` with `Plan` and `Import` modes.
- [x] `Plan` mode reports host/tool availability and the target evidence path without writing evidence.
- [x] `Import` mode accepts existing baseline/candidate trace artifacts plus a profiler summary artifact and writes one `evidence.json` row with CPU host facts, before/after trace pair fields, regression budgets, and explicit zero side-effect counters.
- [x] Add `tools/check-cpu-profiling-host-evidence-collector.ps1` as a default static validation guard using synthetic ignored artifacts under `out/` only.
- [x] Add the collector guard to `tools/validate.ps1`.
- [x] Update manifest recipe text, game-code guidance, docs, plan registry, and static checks.

## Done When

The collector self-test passes, generated synthetic evidence validates with `tools/check-cpu-profiling-host-evidence.ps1`, manifest/static checks require both the collector and validator surfaces, full validation passes, and `unsupportedProductionGaps` remains `[]` with `cpu-profiling-matrix-host` still host-gated until real complete host-class artifact bundles are attached.

## Validation

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpu-profiling-host-evidence-collector.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpu-profiling-host-evidence.ps1 -ExpectedEvidenceCounters cpu_profiling_matrix_host_gated=1 cpu_profiling_matrix_ready=0
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```
