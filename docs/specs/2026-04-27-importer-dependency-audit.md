# Production Importer Dependency Audit

## Goal

Select dependency candidates for production PNG, glTF, and audio source importers without adding default-build third-party dependencies or letting runtime game code parse external source formats directly.

## Decision

Add an optional vcpkg manifest feature named `asset-importers` with:

- `libspng` for PNG decoding.
- `fastgltf` for glTF 2.0 parsing.
- `miniaudio` for WAV/MP3/FLAC decoding.

Implementation update on 2026-04-27: these libraries are now wired only into the optional `MK_ENABLE_ASSET_IMPORTERS` `MK_tools` adapter path. The default build remains free of these dependencies. The adapters convert external source documents into first-party source documents before the existing cooked artifact execution path runs.

## Rationale

The default build must remain third-party-free. The existing first-party source document importers are deterministic and sufficient for current tests, but production asset workflows need audited decoders/parsers for real files.

`libspng` is preferred for PNG because it is security-focused, fuzz-tested, BSD-2-Clause licensed, and has a simpler API than libpng while still supporting robust PNG decoding.

`fastgltf` is preferred for glTF because it is MIT licensed, modern C++17/C++20-friendly, vcpkg-packaged, and supports glTF 2.0 with accessor tooling. It pulls `simdjson`, which is acceptable for an optional importer feature but must stay out of the default build.

`miniaudio` is preferred for common audio source decoding because its decoder API supports WAV, MP3, and FLAC, it is single-file, and its license choice is permissive. Vorbis/Opus can be evaluated later if game requirements need them.

## Required Adapter Boundary

- Keep dependencies behind a vcpkg feature; do not add them to default dependencies.
- Add importer implementation under `MK_tools`, not runtime game modules.
- Runtime games consume cooked artifacts through `MK_assets` and `MK_audio`; they do not include libspng, fastgltf, miniaudio, simdjson, or zlib headers.
- Validate image dimensions, decoded byte sizes, glTF buffer/accessor bounds, and audio frame counts before allocating output buffers.
- Surface dependency-origin diagnostics in editor import results.
- Update notices and dependency docs whenever dependency versions or features change.

## References

- libspng: https://libspng.org/
- libspng decode safety notes: https://libspng.org/docs/decode/
- fastgltf upstream: https://github.com/spnda/fastgltf
- fastgltf docs: https://fastgltf.readthedocs.io/latest/overview.html
- fastgltf vcpkg metadata: https://vcpkg.roundtrip.dev/ports/fastgltf
- cgltf vcpkg metadata: https://vcpkg.roundtrip.dev/ports/cgltf
- miniaudio: https://miniaud.io/
- miniaudio decoder docs: https://miniaud.io/docs/manual/index.html
