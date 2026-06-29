# 2D Commercial Clean-Room Source Ledger v1

This ledger is an engineering compliance record, not legal advice. Its purpose is to keep the 2D commercial production milestone first-party while allowing official documentation to inform only broad functional categories, platform API requirements, validation expectations, and legal/trademark boundaries.

## Status

Candidate guard for `2d-commercial-production-excellence-v1` Phase 1. It adds source provenance, static scanning, and counsel-ready review input generation only. It does not add a dependency, import external engine projects, copy external source/assets/docs prose, create a compatibility layer, or promote commercial/legal readiness.

## Allowed Source Classes

| Source class | Allowed use | Forbidden use | Evidence |
| --- | --- | --- | --- |
| First-party MIRAIKANAI plans/code/tests | Implement `mirakana` contracts, tests, package counters, and validators. | Reopen completed plans or promote broad readiness from adjacent evidence. | `docs/superpowers/plans/2026-06-29-2d-commercial-production-excellence-v1.md`; tracked repo sources. |
| Context7 Vulkan docs plus Khronos fallback | Use Context7 as a reference mirror to locate current synchronization2, timestamp-query, and validation-layer topics; treat Khronos docs as the official platform source. | Copy code snippets, infer D3D12/Metal readiness, or ship full-pipeline barriers as production policy. | `/khronosgroup/vulkan-docs`; `https://docs.vulkan.org/`. |
| Context7 D3D12 docs plus Microsoft fallback | Use Context7 as a reference mirror to locate current debug-layer, fence, allocator/list reset, barrier, descriptor, PSO, and profiling topics; treat Microsoft Learn as the official platform source. | Copy samples, expose native handles, or infer Vulkan/Metal readiness. | `/websites/learn_microsoft_en-us_windows_win32_direct3d12`; `https://learn.microsoft.com/en-us/windows/win32/direct3d12/`. |
| Context7 MSL mirror plus Apple fallback | Record `/dogukanveziroglu/metal-shading-language-specification` only as a non-official Context7 reference mirror for MSL terminology review; treat Apple Metal documentation and Apple-host evidence as the official source boundary. | Classify the mirror as an official SDK source, claim Apple readiness without Apple-host evidence, or copy shader samples. | `/dogukanveziroglu/metal-shading-language-specification`; `https://developer.apple.com/metal/`. |
| Unity legal/trademark docs | Legal/trademark boundary and category research only. | Copy Unity APIs, editor layout, serialized formats, samples, docs prose, screenshots, asset names, or public compatibility claims. | `https://unity.com/legal/branding-trademarks`; `https://unity.com/legal/terms-of-service`. |
| Unreal Engine legal/trademark docs | EULA/trademark boundary and category research only. | Copy Unreal code, Blueprints, project formats, marketplace/Fab content, editor UI, shaders, docs prose, or public equivalence claims. | `https://www.unrealengine.com/eula/unreal`; `https://dev.epicgames.com/docs/dev-portal/unreal-engine/ue-trademark-license`. |
| Godot license/compliance docs | MIT/license compliance boundary research only. | Copy Godot code/assets/docs prose or adopt node/resource/scene names as public `mirakana` surfaces. | `https://godotengine.org/license/`; `https://docs.godotengine.org/en/stable/about/complying_with_licenses.html`. |
| U.S. copyright/trademark public guidance | Understand idea/expression and source-identifying mark boundaries. | Treat this ledger as legal advice or legal approval. | `https://www.copyright.gov/help/faq/faq-protect.html`; `https://www.uspto.gov/trademarks/basics/what-trademark`. |
| Repository legal/dependency records | Verify dependency, notice, and redistribution policy. | Treat official-doc references as dependency approval or third-party material selection. | `docs/legal-and-licensing.md`; `docs/dependencies.md`; `THIRD_PARTY_NOTICES.md`. |

## Forbidden Public Inputs

- External engine source, sample code, shader code, assets, themes, icons, fonts, screenshots, starter content, marketplace content, or docs prose.
- External engine project/scene/schema imports, including Unity `.unity` / `.prefab` / `.asset` / `.meta` when identified as Unity formats; Unreal `.uasset`, `.umap`, `.uproject`; Godot `.tscn`, `.tres`, `.godot`; and equivalent product schema names. MIRAIKANAI-owned first-party file extensions with the same generic English word, such as a first-party `.prefab`, are allowed only when they are documented and implemented as MIRAIKANAI schemas with no external-engine compatibility claim.
- Public compatibility, equivalence, parity, replacement, clone, or importer claims for Unity, Unreal Engine, Godot, GameMaker, Defold, O3DE, or middleware.
- External engine trademarks, product marks, editor feature names, API shapes, visual layouts, or workflow names in public `mirakana` APIs, game manifests, package counters, sample labels, schema ids, or editor UI labels.

## Static Guard

`tools/check-2d-commercial-clean-room.ps1` scans public engine headers, editor-core public headers, game/sample code and manifests, and selected public schemas. Documentation, legal records, source ledgers, and retained clean-room artifacts may mention the names only as provenance, non-claim, or prohibition text.

The guard reports `2d-commercial-clean-room: ok` only when scanned public surfaces avoid forbidden external-engine product marks, schema names, and compatibility/equivalence/parity claim phrases.

## Review Input Generator

`tools/generate-2d-commercial-clean-room-review-input.ps1` writes `GameEngine.TwoDCommercialCleanRoomReviewInput.v1` and an official source summary under `artifacts/2d-commercial/clean-room-review/<root>`. Official/first-party/legal/notice rows are retained under `official_sources`; Context7 lookup rows, including the MSL mirror, are retained separately under `reference_sources` as `context7_documentation_mirror`. The generated record is counsel-ready engineering input only:

- `legal_advice=false`
- `fixture_only=false`
- `legal_counsel_review_required=true`
- `legal_approval=false`
- `commercial_ready=false`
- all external-engine code/asset/schema/API/compatibility/equivalence/parity flags are false

## Dependency Entry Gate

No dependency enters through this ledger. Any future third-party dependency, distributable asset, font, icon, texture, shader, sample, or generated asset requires `license-audit`, dependency docs, legal docs, notices, bootstrap/policy checks where applicable, and a separate implementation slice before use.

## Source Diagnostics Guard

`tools/check-2d-commercial-source-diagnostics.ps1` adds the second fail-closed Phase 1 guard. It reports deterministic `2d_commercial_source_diagnostic=<kind>` rows for retained evidence markers or product-facing text that indicates copied external code, copied external assets, copied documentation prose, external engine schema/project surfaces, third-party trademark public surfaces, missing notice records, unapproved dependency sources, or external-engine compatibility/equivalence/parity claims. Successful output is scoped with `2d_commercial_source_diagnostics_scope=retained_markers_and_public_surface_tokens` and only proves the corresponding `*_marker_rows=0` / public-surface claim rows over scanned inputs.

The guard is intentionally not a legal conclusion, plagiarism detector, or dependency approval engine. It verifies explicit retained markers and public/product-facing text under the same scanned surfaces as the clean-room guard, while allowing existing first-party fail-closed diagnostic ids such as `missing_third_party_notice` to remain public API names.

## Review Evidence

| Date | Evidence | Result |
| --- | --- | --- |
| 2026-06-30 | Context7 queries against Vulkan, D3D12, and Metal Shading Language documentation plus official fallbacks. | Official platform/API sources are recorded as category and validation-boundary research only. |
| 2026-06-30 | `tools/check-2d-commercial-clean-room.ps1` and `tools/generate-2d-commercial-clean-room-review-input.ps1` added with `tools/check-2d-commercial-clean-room-contract.ps1`. | Public/package/schema surfaces fail closed on forbidden external-engine names and claims; generated review input preserves legal and commercial non-claims. |
| 2026-06-30 | `tools/check-2d-commercial-source-diagnostics.ps1` added with `tools/check-2d-commercial-source-diagnostics-contract.ps1`. | Copied-material, external-schema, trademark, missing-notice, unapproved-dependency, and external-engine claim markers fail closed while preserving legal and commercial non-claims. |
