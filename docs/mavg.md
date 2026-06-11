# MAVG

MIRAIKANAI Adaptive Virtual Geometry (MAVG) is tracked through the current production plans and specs, with implementation evidence kept narrow and explicit.

Primary references:

- [Mirakana Adaptive Virtual Geometry Master Plan v1](superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md)
- [MAVG Architecture v1](specs/2026-06-05-mavg-architecture-v1.md)
- [MAVG Benchmark Methodology v1](specs/2026-06-05-mavg-benchmark-methodology-v1.md)
- `engine/renderer/include/mirakana/renderer/mavg_quality_governor.hpp`
- `tools/benchmark-mavg.ps1`

The current value-only quality gate is MAVG Quality Governor Benchmark Harness v1 (`mavg-quality-governor-benchmark-harness-v1`). Public contract rows include `MavgQualityGovernorResult`, `MavgQualityGovernorCounterRow`, `MavgQualityGovernorLimits`, `evaluate_mavg_quality_governor`, and `has_mavg_quality_governor_diagnostic`.

The benchmark harness is metadata-only by default:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools\benchmark-mavg.ps1 -PlanId mavg-quality-governor-benchmark-harness-v1 -SceneId scene-a-static-dense -PackageTarget sample_desktop_runtime_game -ValidationRecipe default -BenchmarkCommand dry-run
```

It prints deterministic `mavg-benchmark-harness:` rows and reports `executes_benchmark=false`, `writes_artifacts=false`, `invokes_profiler=false`, and `native_handles=false`.

Non-claims: MAVG does not claim Nanite compatibility, Nanite equivalence, Nanite superiority, benchmark superiority, broad CPU/GPU/memory optimization, backend parity, Metal readiness, mesh shader execution, deformation readiness, ray-tracing readiness, DirectStorage execution, persistent/autonomous streaming services, async-overlap proof, or performance proof unless a later plan records measured host evidence. Metal and explicit host-gated rows remain `host_evidence_required`; the dry-run harness rejects control text and native/backend handle tokens.
