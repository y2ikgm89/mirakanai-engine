# 2026-05-27 Audio Production Playback Streaming DSP Spatialization

## Status

Phase 6 selected package evidence is implemented for first-party audio production review, and Phase 7 extends the same selected proof to the generated 3D desktop runtime package. It does not claim broad codec support, middleware parity, HRTF execution, subjective mix quality, background streaming execution, or general commercial audio readiness.

## Official Sources Checked

- Microsoft Learn Win32 Core Audio / WASAPI documentation for `IAudioClient`, `IAudioRenderClient`, shared-mode stream initialization, `GetCurrentPadding`, `GetBuffer`, `ReleaseBuffer`, `AUDCLNT_BUFFERFLAGS_SILENT`, and COM initialization: Windows native playback flows through the default `eRender` / `eConsole` endpoint, private `IAudioClient` and `IAudioRenderClient` ownership, explicit queue-space checks, and `Start` / `Stop` / `Reset` lifecycle controls.
- Existing project dependency policy was checked before this slice: no OpenAL, miniaudio, Steam Audio, codec, or HRTF dependency was selected. Those remain future dependency/legal/host-gated adapter work.

## Implemented Contract

- `mirakana::AudioProductionReviewRequest` gathers reviewed decoded-source, streaming-chunk, format-conversion, voice/bus budget, DSP graph, listener/spatial source, device lifecycle, unsupported-claim, and side-effect evidence.
- `mirakana::review_audio_production_readiness` returns `AudioProductionReadinessStatus::ready`, `host_evidence_required`, or `invalid_request`, plus exact row counters, host-gate booleans, diagnostics, side-effect flags, selected-package readiness, production readiness, and replay hash.
- `mirakana::wasapi_audio_device_lifecycle_evidence` records Windows WASAPI logical-device, shared-mode audio-stream, render-client queueing, pause/resume, clear, callback-free, and native-handle-free lifecycle evidence without exposing COM, endpoint, or WASAPI handles.
- `sample_2d_desktop_runtime_package` and `sample_generated_desktop_runtime_3d_package` emit `audio_production_*` counters for selected package proof. The current package proof intentionally reports `audio_production_status=host_evidence_required`, selected package ready, device/HRTF host evidence missing, side effects zero, diagnostics `2`, and a positive replay hash.

## Non-Claims

- No arbitrary codec decoding, streaming thread, middleware invocation, device callback installation, device IO, HRTF execution, native device handle access, or subjective mix-quality promotion occurs in the review API.
- No dependency/legal records changed because no new audio dependency was selected.
- Real codec adapter selection, HRTF/spatialization implementation, device hotplug/selection, mixer authoring UI, platform parity, and commercial audio quality gates remain future work.

## Validation

Initial focused evidence:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_audio_tests MK_wasapi_audio_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_audio_tests|MK_wasapi_audio_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_2d_desktop_runtime_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "sample_2d_desktop_runtime_package"
pwsh -NoProfile -ExecutionPolicy Bypass -Command "& .\tools\package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -SmokeArgs @('--smoke','--require-config','runtime/sample_generated_desktop_runtime_3d_package.config','--require-scene-package','runtime/sample_generated_desktop_runtime_3d_package.geindex','--require-audio-production')"
```
