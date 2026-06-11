# 2026-06-11 MAVG DirectStorage Page IO Execution v1

## Goal

Add the narrow runtime boundary for reviewed `.mavgpayload` page byte ranges to execute through a caller-owned DirectStorage byte-range executor without exposing DirectStorage SDK objects, native handles, filesystem fallback, residency mutation, background streaming policy, renderer/RHI handles, or GPU destinations.

## Context

Previous MAVG slices added caller-owned payload byte extraction, first-party filesystem byte-range IO, background package load dispatch, GPU memory pressure residency planning, cluster streaming closeout, safe-point adoption, and streamed cluster GPU upload. DirectStorage remained unclaimed because the runtime had no host-gated execution boundary.

Slice id: `mavg-directstorage-page-io-execution-v1`.

This slice follows Microsoft DirectStorage's memory-destination shape at the engine boundary without adding the Microsoft SDK dependency in this PR. The current host does not provide `dstorage.h` or `dstorage.lib`, so a concrete Win32 DirectStorage SDK adapter remains a later host/dependency-gated slice.

## Constraints

- Keep `dstorage.h`, `dstorage.lib`, `IDStorageFactory`, `IDStorageQueue`, `IDStorageFile`, and status arrays out of `MK_runtime` public API.
- Keep DirectStorage execution host-provided through `IByteRangeIoExecutor` and `ByteRangeIoBackendKind::direct_storage`.
- Validate graph asset, payload path, graph validity, duplicate page requests, format prefix, page read failures, and result row identity before publishing loaded page rows.
- Do not fall back to filesystem reads when the DirectStorage executor is missing, unavailable, wrong-backend, or failing.
- Do not mutate resident mounts/catalogs, run background workers, touch GPU memory policy, upload to RHI, expose renderer/RHI/native handles, prove async-overlap/performance, or claim a first-party Win32 DirectStorage SDK adapter.

## Done When

- `engine/platform/include/mirakana/platform/byte_range_io.hpp` exposes a backend-neutral byte-range executor contract.
- `mavg_payload_page_loader.hpp` exposes `RuntimeMavgPayloadDirectStoragePageLoadDesc` and `load_runtime_mavg_payload_pages_from_direct_storage`.
- `mavg_payload_page_loader.cpp` fails closed for unavailable/wrong DirectStorage executors, validates the payload format prefix before page rows, preserves requested page order, and reports `executed_direct_storage` only after executor invocation.
- `MK_runtime_mavg_payload_page_loader_tests` covers success, unavailable executor, invalid payload prefix, read failure, zero filesystem fallback, and zero mount/background/GPU/RHI side effects.
- Docs, manifest fragments, composed manifest, and static checks distinguish this host-provided DirectStorage byte-range executor evidence from still-unclaimed first-party Win32 SDK adapter, GPU destination, async performance, and broad streaming readiness.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_payload_page_loader_tests` failed after adding tests because `IRuntimeMavgDirectStoragePageReadExecutor`, `RuntimeMavgPayloadDirectStoragePageLoadDesc`, `load_runtime_mavg_payload_pages_from_direct_storage`, and `direct_storage_unavailable` were not defined.
- GREEN focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_payload_page_loader_tests` passed.
- GREEN focused test: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_payload_page_loader_tests"` passed.
- Focused static checks passed: `tools/compose-agent-manifest.ps1 -Write`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, and `tools/check-tidy.ps1 -Files "engine/runtime/src/mavg_payload_page_loader.cpp,tests/unit/runtime_mavg_payload_page_loader_tests.cpp"`.
- Full validation passed after review follow-up: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` with `validate: ok` and 118/118 tests passing. Log root: `out/validation-logs/validate-20260611-230248-19752`.
- Review follow-up: documented `IByteRangeIoExecutor::read_ranges` request lifetime as call-bound and added `direct_storage_result_mismatch` coverage for mismatched page result rows. Follow-up focused build/test/format/tidy passed.

## Remaining Non-Claims

- First-party Win32 DirectStorage SDK adapter and NuGet/vcpkg/dependency/legal packaging.
- GPU buffer/texture DirectStorage destinations, GDeflate/GPU decompression, RHI upload, backend draw execution, mesh shaders, Vulkan/Metal parity, async-overlap/performance proof, persistent/autonomous streaming service, live resident mount/catalog mutation, Nanite equivalence/superiority, and broad optimization.
