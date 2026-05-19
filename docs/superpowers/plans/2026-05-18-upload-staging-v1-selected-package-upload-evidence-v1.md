# Upload Staging v1 Selected Package Upload Evidence v1 - 2026-05-18

**Status:** Completed.

**Goal:** Prove selected generated 3D package upload staging through the existing runtime RHI package upload transactions, pool lease-backed upload rings, submitted upload fences, and graphics-queue waits without exposing native handles or broadening background streaming claims.

**Context:** `upload-staging-v1` already has ring-backed runtime texture/mesh/skinned/morph uploads, package texture/static/skinned/morph upload binding transactions, RHI no-wait upload batch execution, runtime/package upload queue waits, and staging-pool lease-backed upload rings. The remaining package-evidence blocker is an installed package smoke that exercises those pieces together on a selected generated 3D package path.

**Implementation surface:**
- Add `RuntimePackageUploadStagingEvidence` and `execute_runtime_package_upload_staging_evidence` in `MK_runtime_rhi` to synthesize committed resident texture/static/skinned/morph package rows, upload them through the package transaction helpers, and aggregate package transaction, ring, fence, queue-wait, and byte counters.
- Add RED-first unit coverage in `tests/unit/runtime_rhi_tests.cpp` for four package transactions, four pool lease-backed caller-owned upload rings, submitted fences, three copy-queue uploads consumed by graphics waits, no CPU fence waits, and positive uploaded bytes.
- Add `--require-package-upload-staging` to the committed generated 3D package sample and installed validation so `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` proves the selected D3D12 package path.
- Update the generated 3D package manifest, docs, manifest fragments, skills, and static guards so selected package upload evidence is no longer listed as a remaining `upload-staging-v1` blocker.

**Constraints:**
- Keep gameplay and manifest surfaces free of public `IRhiDevice` or native handle access; the sample gets the host-owned scene RHI device only through `SdlDesktopPresentation`.
- Keep texture upload on the graphics queue for the selected D3D12 smoke because the current texture final-state path requires graphics-capable transition recording; static/skinned/morph buffer package uploads remain copy-queue submissions with graphics-queue waits.
- Do not add runtime-wide staging-pool ownership beyond explicit leases, allocator/GPU budget enforcement, broad/background streaming, async-ready resource updates, async overlap/performance claims, Vulkan/Metal package upload staging parity, or 2D package parity.

**Done when:**
- RED build fails before implementation because `execute_runtime_package_upload_staging_evidence` is missing.
- Focused runtime RHI tests and the generated 3D source-tree package smoke pass with `package_upload_staging_status=ready`, four package transactions, four ring-backed uploads, submitted fences, graphics waits for copy-queue uploads, and no diagnostics.
- `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` passes with metadata-derived `--require-package-upload-staging` installed smoke args.
- Docs, manifest fragments, skills, and static guards record selected D3D12 package upload staging evidence while leaving async-ready resource updates as the remaining `upload-staging-v1` blocker.
- Focused static checks pass, then full `tools/validate.ps1` passes for the coherent runtime/build/package/public-contract slice.

**Validation evidence:**
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` failed before implementation because `execute_runtime_package_upload_staging_evidence` was not a member of `mirakana::runtime_rhi`.
- Focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests sample_generated_desktop_runtime_3d_package` passed.
- Focused tests: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_runtime_rhi_tests$|^sample_generated_desktop_runtime_3d_package_smoke$"` passed.
- Focused D3D12 source-tree smoke: `sample_generated_desktop_runtime_3d_package.exe --smoke ... --require-package-upload-staging` passed with `package_upload_staging_status=ready`, `package_upload_staging_package_transactions=4`, `package_upload_staging_ring_backed_uploads=4`, `package_upload_staging_submitted_fences=4`, `package_upload_staging_queue_waits=3`, `package_upload_staging_fence_waits=0`, and `package_upload_staging_graphics_waited_for_copy=1`.
- Installed package smoke: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` passed with metadata-derived `--require-package-upload-staging` installed validation.
