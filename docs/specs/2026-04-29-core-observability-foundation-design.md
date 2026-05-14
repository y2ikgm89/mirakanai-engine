# Core Observability Foundation Design

## Goal

Add a dependency-free observability foundation in `mirakana_core` that engine modules can use to record diagnostics, counters, and profile zones without depending on runtime, renderer, platform, editor, SDL3, or native APIs.

## Scope

This slice builds the shared data model and recorder only. It does not add GPU timestamp queries, native GPU markers, crash dumps, telemetry upload, Chrome trace export, allocator hooks, thread sampling, or a `mirakana_core` recorder-backed editor profiler panel. The existing editor Profiler panel is only a lightweight status/debug counter panel. Recorder-backed profiler UI belongs in a later adapter built on this foundation.

## Public Contract

Add `engine/core/include/mirakana/core/diagnostics.hpp` with:

- `DiagnosticSeverity`
- `DiagnosticEvent`
- `CounterSample`
- `ProfileSample`
- `DiagnosticCapture`
- `DiagnosticSummary`
- `ManualProfileClock`
- `SteadyProfileClock`
- `DiagnosticsRecorder`
- `ScopedProfileZone`

The recorder owns a bounded ring buffer for events, counters, and profile samples. It has no global singleton. Callers can record diagnostic events, counter samples, and nested profile zones with explicit `frame_index` values. Tests and deterministic systems can use `ManualProfileClock`; production host/editor code can use `SteadyProfileClock` or provide explicit timestamps.

## Architecture

`mirakana_core` is the correct layer because observability is cross-cutting. `mirakana_runtime_host` and `mirakana_renderer` already depend on `mirakana_core`, while `mirakana_runtime` must not become a dependency of renderer or host modules. Putting the common contract in `mirakana_core` lets later slices add `InstrumentedRenderer`, desktop host frame markers, runtime report bridges, editor profiler panels, and backend marker adapters without reversing dependency direction.

The first implementation uses standard library containers and `std::chrono::steady_clock` only. It does not own platform timers, native handles, renderer objects, or backend state. All recorded data is value-based and safe to snapshot for tests or UI models.

## Error Handling

The recorder should not throw for normal invalid observability calls. Invalid names are ignored and logged as warning diagnostic events when possible. Capacity overflow drops the oldest entries because observability must not grow unbounded during long-running sessions.

## Testing

Add core unit tests for:

- bounded event/counter/profile ring buffer behavior
- manual-clock nested profile scopes and deterministic durations
- summary aggregation for frames, scopes, counters, events, and warnings
- scoped profile zones closing through RAII

Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` before reporting completion.
