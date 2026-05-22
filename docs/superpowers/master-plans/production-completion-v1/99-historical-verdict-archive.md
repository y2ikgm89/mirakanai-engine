# Production Completion v1 - Historical Verdict Archive

This chapter retains historical verdict text and static-check evidence that used to live at the end of the single master plan. It is not the active execution index; read the parent index and the owning chapter first.

<!-- archived-previous-verdict-snapshot-ci-needles
The snapshot below records the pre-physics-replan ledger, including the latest generated 3D visible proof and earlier completed slices. Treat any older `currentActivePlan` wording in this snapshot as historical evidence only; the live pointer is the Current Verdict plus the composed `engine/agent/manifest.json`.

Generated 3D Visible Production Game Proof v1 is complete through [2026-05-09-generated-3d-visible-production-game-proof-v1.md](../../plans/2026-05-09-generated-3d-visible-production-game-proof-v1.md): the selected D3D12 `--require-visible-3d-production-proof` installed package smoke aggregates package loading, gameplay systems, scene GPU bindings, postprocess, renderer quality gates, playable aggregate evidence, and native UI overlay evidence into fail-closed `visible_3d_*` counters. It reports `visible_3d_status=ready`, `visible_3d_ready=1`, `visible_3d_diagnostics=0`, `visible_3d_expected_frames=2`, `visible_3d_presented_frames=2`, `visible_3d_d3d12_selected=1`, `visible_3d_null_fallback_used=0`, `visible_3d_scene_gpu_ready=1`, `visible_3d_postprocess_ready=1`, `visible_3d_renderer_quality_ready=1`, `visible_3d_playable_ready=1`, and `visible_3d_ui_overlay_ready=1`, without claiming broad generated 3D production readiness, source import execution, broad package streaming, Vulkan/Metal parity, editor productization, public native/RHI handles, or general renderer quality.

`currentActivePlan` now points back to this master plan after completing [2026-05-09-generated-3d-native-ui-textured-sprite-atlas-package-smoke-v1.md](../../plans/2026-05-09-generated-3d-native-ui-textured-sprite-atlas-package-smoke-v1.md), [2026-05-09-generated-3d-native-ui-overlay-package-smoke-v1.md](../../plans/2026-05-09-generated-3d-native-ui-overlay-package-smoke-v1.md), [2026-05-09-generated-3d-shadow-morph-composition-package-smoke-v1.md](../../plans/2026-05-09-generated-3d-shadow-morph-composition-package-smoke-v1.md), [2026-05-08-full-repository-static-analysis-ci-contract-v1.md](../../plans/2026-05-08-full-repository-static-analysis-ci-contract-v1.md), [2026-05-08-runtime-resource-residency-hints-execution-v1.md](../../plans/2026-05-08-runtime-resource-residency-hints-execution-v1.md), [2026-05-08-cpp23-release-package-artifact-ci-evidence-v1.md](../../plans/2026-05-08-cpp23-release-package-artifact-ci-evidence-v1.md), and the earlier runtime UI/input/editor/RHI/2D/3D/renderer-material slices listed below. The latest completed generated 3D native UI textured sprite atlas child plan adds the selected D3D12 `--require-native-ui-textured-sprite-atlas` installed package smoke with cooked `GameEngine.UiAtlas.v1` image-sprite submission through the host-owned native UI texture overlay path, `hud_images=2`, `ui_atlas_metadata_status=ready`, `ui_atlas_metadata_pages=1`, `ui_atlas_metadata_bindings=1`, `ui_texture_overlay_atlas_ready=1`, `ui_texture_overlay_sprites_submitted=2`, `ui_texture_overlay_texture_binds=2`, and `ui_texture_overlay_draws=2` without claiming production text/font/glyph UI, runtime source image decoding, source image atlas packing, Metal, native handles, broad runtime UI readiness, or broad generated 3D readiness. The previous completed generated 3D native UI overlay slice adds the selected D3D12 `--require-native-ui-overlay` installed package smoke with a first-party `MK_ui` HUD box submitted through `MK_ui_renderer`, `hud_boxes=2`, `ui_overlay_ready=1`, `ui_overlay_sprites_submitted=2`, and `ui_overlay_draws=2`; the later textured atlas slice covers generated 3D image-sprite submission, while production text/font/glyph UI, runtime source image decoding, Metal, native handles, and broad generated 3D readiness remain unsupported. The previous completed generated 3D package slice adds the selected D3D12 `--require-shadow-morph-composition` installed package smoke with shifted shadow receiver artifacts, positive `renderer_gpu_morph_draws` / `renderer_morph_descriptor_binds`, `directional_shadow_ready=1`, fixed PCF filtering, `framegraph_passes=3`, `framegraph_passes_executed=6`, `framegraph_render_passes_recorded=6`, and `framegraph_barrier_steps_executed=15` while keeping compute morph + shadow composition, morph-deformed shadow-caster silhouettes, Vulkan/Metal parity, native handles, and broad renderer quality unsupported. The latest completed quality-gate slice adds the reviewed `static-analysis` GitHub Actions lane for strict full-repository tidy execution and artifact evidence while keeping broader build/package CI execution evidence and analyzer profile expansion unsupported. The latest completed runtime-resource slice enforces required preload asset and resident resource kind hints, and records selected resident package count hint evidence, before the selected safe-point package streaming path stages or commits a loaded package; broad/background package streaming, arbitrary eviction, resident caches, allocator/GPU enforcement, renderer/RHI ownership or teardown, hot reload, editor asset browser migration, and broad renderer quality remain unsupported. The prior C++23 release package artifact CI evidence slice extends `tools/evaluate-cpp23.ps1 -Release` so the existing Windows CI release lane validates CPack ZIP/SHA-256/archive payloads through `Assert-ReleasePackageArtifacts` and uploads the `.zip.sha256` sidecar; signing, upload, notarization, deployment, full cross-platform package matrix readiness, and broader static analyzer profile expansion remain unsupported. The historical editor/importer ledger still includes [2026-05-07-editor-content-browser-import-external-copy-review-v1.md](../../plans/2026-05-07-editor-content-browser-import-external-copy-review-v1.md), which reviewed Content Browser import selections before the later [2026-05-07-editor-content-browser-import-codec-adapter-review-v1.md](../../plans/2026-05-07-editor-content-browser-import-codec-adapter-review-v1.md) / `editor-content-browser-import-codec-adapter-review-v1` work through `ExternalAssetImportAdapters` and the optional `asset-importers` lane; broad importer readiness remains unsupported. The historical quality-gate ledger still includes [2026-05-07-ci-matrix-contract-check-v1.md](../../plans/2026-05-07-ci-matrix-contract-check-v1.md), `tools/check-ci-matrix.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` before returning to `next-production-gap-selection`.
Physics Character Controller Collision Authoring v1 is complete through [2026-05-08-physics-character-controller-collision-authoring-v1.md](../../plans/2026-05-08-physics-character-controller-collision-authoring-v1.md): `PhysicsCharacterController3DDesc`, `PhysicsCharacterController3DResult`, and `move_physics_character_controller_3d` provide conservative capsule movement/contact/grounded rows over `PhysicsWorld3D::shape_sweep`; `PhysicsAuthoredCollisionScene3DDesc`, `PhysicsAuthoredCollisionScene3DBuildResult`, and `build_physics_world_3d_from_authored_collision_scene` provide reviewed in-memory collision body rows with duplicate/invalid/native-backend diagnostics. This narrows the physics controller/collision-authoring gap without claiming exact primitive casts, joints, CCD, richer scene/package collision authoring, dynamic push/step policies, Jolt/native backends, middleware, or generated-game integration evidence using audio/navigation/AI together.

Physics Exact Sphere Cast v1 is complete through [2026-05-08-physics-exact-sphere-cast-v1.md](../../plans/2026-05-08-physics-exact-sphere-cast-v1.md): `PhysicsExactSphereCast3DDesc`, `PhysicsExactSphereCast3DResult`, and `PhysicsWorld3D::exact_sphere_cast` provide exact moving-sphere queries against current first-party AABB, sphere, and vertical capsule target primitives with nearest-hit, initial-overlap, ignored-body, trigger-inclusion, collision-mask, and invalid-request diagnostics. This narrows the exact-query gap without claiming broader exact primitive casts, exact shape sweeps, joints, CCD, richer scene/package collision authoring, dynamic push/step policies, Jolt/native backends, or middleware.

Physics Scene Package Collision Authoring v1 is complete through [2026-05-09-physics-scene-package-collision-authoring-v1.md](../../plans/2026-05-09-physics-scene-package-collision-authoring-v1.md): `AssetKind::physics_collision_scene`, `RuntimePhysicsCollisionScene3DPayload`, `runtime_physics_collision_scene_3d_payload`, `build_physics_world_3d_from_runtime_collision_scene`, `PhysicsCollisionPackage*` authoring/update helpers, and the selected generated 3D `--require-scene-collision-package` installed smoke promote reviewed first-party collision rows into package data with `backend.native=unsupported`, material/layer/mask/trigger metadata, `body.N.compound` labels, deterministic validation diagnostics, three body rows, one trigger row, nonzero contact and trigger-overlap evidence, and `gameplay_systems_collision_package_ready=1`. This narrows the scene/package collision authoring gap without claiming broader exact casts/sweeps, contact manifold stability, CCD, joints, dynamic push/step policies, Jolt/native backends, middleware, or broad physics beyond the first-party Physics 1.0 ready surface.

Physics Broader Exact Casts And Sweeps v1 is complete through [2026-05-09-physics-broader-exact-casts-and-sweeps-v1.md](../../plans/2026-05-09-physics-broader-exact-casts-and-sweeps-v1.md): `PhysicsShape3DDesc::aabb`, `PhysicsShape3DDesc::sphere`, `PhysicsShape3DDesc::capsule`, `PhysicsQueryFilter3D`, `PhysicsExactShapeSweep3DDesc`, `PhysicsExactShapeSweep3DResult`, and `PhysicsWorld3D::exact_shape_sweep` provide exact AABB/sphere/vertical-capsule query shapes against current AABB/sphere/capsule targets with conservative false-positive rejection, nearest-hit selection, deterministic epsilon tie-breaking, initial-overlap, disabled-body, ignored-body, trigger-inclusion, collision-mask, and invalid-request diagnostics. `PhysicsWorld3D::exact_sphere_cast` remains as a first-class convenience wrapper over the shared exact sweep path. This narrows the exact-query gap without claiming contact manifold stability, CCD, joints, dynamic push/step policies, Jolt/native backends, oriented boxes, mesh/convex casts, physics benchmarks, middleware, or broad physics beyond the first-party Physics 1.0 ready surface.

Physics Contact Manifold Stability v1 is complete through [2026-05-09-physics-contact-manifold-stability-v1.md](../../plans/2026-05-09-physics-contact-manifold-stability-v1.md): `PhysicsContactPoint3D`, `PhysicsContactManifold3D`, and `PhysicsWorld3D::contact_manifolds` expose deterministic one-point manifold rows for current AABB/sphere/vertical-capsule contacts with stable feature ids, warm-start-safe rows, trigger/disabled exclusion, `contacts()` flattening from the same path, and solver iteration over manifold ordering. This narrows solver-input stability without claiming CCD, joints, dynamic push/step policies, Jolt/native backends, oriented boxes, mesh/convex casts, physics benchmarks, middleware, or broad physics beyond the first-party Physics 1.0 ready surface.

Physics CCD Foundation v1 is complete through [2026-05-09-physics-ccd-foundation-v1.md](../../plans/2026-05-09-physics-ccd-foundation-v1.md): `PhysicsContinuousStep3DConfig`, `PhysicsContinuousStep3DRow`, `PhysicsContinuousStep3DResult`, and `PhysicsWorld3D::step_continuous` expose opt-in deterministic 3D CCD rows for fast dynamic AABB/sphere/vertical-capsule bodies swept against current non-trigger AABB/sphere/capsule targets while `PhysicsWorld3D::step` remains discrete. The tests cover no-tunnel behavior, no-hit discrete equivalence, trigger/mask filtering, dynamic-target exclusion from CCD, tangent velocity preservation, invalid-request diagnostics, deterministic row ordering, and default-step discrete preservation. This narrows accepted fast-moving-body behavior without claiming dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, joints, dynamic push/step policies, Jolt/native backends, oriented boxes, mesh/convex casts, physics benchmarks, middleware, or broad physics beyond the first-party Physics 1.0 ready surface.

Physics Character/Dynamic Interaction Policy v1 is complete through [2026-05-09-physics-character-dynamic-interaction-policy-v1.md](../../plans/2026-05-09-physics-character-dynamic-interaction-policy-v1.md): `PhysicsCharacterDynamicPolicy3DDesc`, `PhysicsCharacterDynamicPolicy3DRowKind`, `PhysicsCharacterDynamicPolicy3DRow`, `PhysicsCharacterDynamicPolicy3DResult`, and `evaluate_physics_character_dynamic_policy_3d` expose deterministic generated-game solid contact, trigger overlap, dynamic push proposal, step-up, and ground-probe rows over current AABB/sphere/vertical-capsule collision primitives without mutating `PhysicsWorld3D`. The tests cover dynamic push proposals with no body mutation, trigger opt-in, reciprocal layer/mask filtering, low-step acceptance, high-step and unsafe-landing rejection, grounded propagation, invalid-request diagnostics, and existing default-step/controller regression. This narrows generated-game character/dynamic semantics without claiming dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, joints, optional Jolt/native backends, oriented boxes, mesh/convex casts, physics benchmarks, middleware, vehicles, ragdolls, or broad physics beyond the first-party Physics 1.0 ready surface.

Physics Joints Foundation v1 is complete through [2026-05-09-physics-joints-foundation-v1.md](../../plans/2026-05-09-physics-joints-foundation-v1.md): `PhysicsJoint3DStatus`, `PhysicsJoint3DDiagnostic`, `PhysicsDistanceJoint3DDesc`, `PhysicsJointSolve3DConfig`, `PhysicsJointSolve3DDesc`, `PhysicsJointSolve3DRow`, `PhysicsJointSolve3DResult`, and `solve_physics_joints_3d` expose explicit first-party distance-joint solving over current 3D body ids and local anchors. The tests cover dynamic/dynamic and static/dynamic solving, non-zero local anchors, disabled rows, deterministic replay, missing body, self-joint, invalid config, invalid anchors/rest distances/tolerance, both-static rejection, no body mutation on invalid requests, velocity preservation, and unchanged default `PhysicsWorld3D::step` behavior. This narrows generated-game joint constraints without claiming benchmark gates, persistent joint assets, vehicles, ragdolls, dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, optional Jolt/native backends, oriented boxes, mesh/convex casts, middleware, or broad physics beyond the first-party Physics 1.0 ready surface.

Physics Benchmark Determinism Gates v1 is complete through [2026-05-09-physics-benchmark-determinism-gates-v1.md](../../plans/2026-05-09-physics-benchmark-determinism-gates-v1.md): `PhysicsDeterminismGate3DStatus`, `PhysicsDeterminismGate3DDiagnostic`, `PhysicsDeterminismGate3DConfig`, `PhysicsDeterminismGate3DCounts`, `PhysicsReplaySignature3D`, `PhysicsDeterminismGate3DResult`, `make_physics_replay_signature_3d`, and `evaluate_physics_determinism_gate_3d` expose dependency-free count budgets, observed body/query/contact/manifold/trigger counts, and stable replay signatures for current first-party 3D physics rows. The tests cover exact-budget pass behavior, budget-exceeded diagnostics for each count, default unlimited config, all-zero invalid config rejection, no mutation during read-only gate evaluation, and duplicated-world replay signatures before and after explicit joint solving. This narrows deterministic evidence without claiming wall-clock performance benchmarks, optional Jolt/native backends, dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, persistent joint assets, vehicles, ragdolls, oriented boxes, mesh/convex casts, middleware, or broad physics beyond the first-party Physics 1.0 ready surface.

Tidy Targeted File Lane v1 is complete through [2026-05-08-tidy-targeted-file-lane-v1.md](../../plans/2026-05-08-tidy-targeted-file-lane-v1.md): this quality-gate slice adds wrapper-owned `tools/check-tidy.ps1 -Files` changed-file analysis so agents can keep clang-tidy on the repository toolchain/compile-database path when full-repository tidy is too broad for a coherent slice. It preserves default `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` behavior, rejects invalid/out-of-repository/non-source/compile-database-missing requests explicitly, and keeps requested `-Files` order before `-MaxFiles`. It must not mark the full repository quality gate ready or replace full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` where a full analyzer run is selected.

Full Repository Static Analysis CI Contract v1 is complete through [2026-05-08-full-repository-static-analysis-ci-contract-v1.md](../../plans/2026-05-08-full-repository-static-analysis-ci-contract-v1.md): `.github/workflows/validate.yml` now includes a `static-analysis` job on `ubuntu-latest` that verifies the runner-provided Ubuntu toolchain, installs `ccache` only when missing, runs `tools/check-tidy.ps1 -Strict -Preset ci-linux-tidy -Jobs 0`, and uploads `static-analysis-tidy-logs`. `tools/check-ci-matrix.ps1` / `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` statically guard the lane without executing GitHub Actions locally. This narrows `full-repository-quality-gate` without claiming broader build/package CI execution evidence, broader analyzer profile expansion, or full production readiness.

Runtime Resource Safe-Point Unload v1 is complete through [2026-05-08-runtime-resource-safe-point-unload-v1.md](../../plans/2026-05-08-runtime-resource-safe-point-unload-v1.md): `RuntimeAssetPackageStore::unload_safe_point`, `RuntimePackageSafePointUnloadResult`, and `commit_runtime_package_safe_point_unload` add the explicit active-package unload half of the host-owned safe-point package loop. The helper commits an empty `RuntimeResourceCatalogV2` only when an active package exists, invalidates old handles through a generation change, reports previous/committed counts and `discarded_pending_package`, and preserves pending packages when there is no active package. This narrows the `runtime-resource-v2` gap without claiming broad/background streaming, arbitrary eviction, resident caches, allocator/GPU budget enforcement, renderer/RHI teardown, native handles, Metal readiness, hot reload, editor asset browser migration, or broad renderer quality.

Runtime Resource Residency Hints Execution v1 is complete through [2026-05-08-runtime-resource-residency-hints-execution-v1.md](../../plans/2026-05-08-runtime-resource-residency-hints-execution-v1.md): `RuntimePackageStreamingExecutionDesc` now carries `required_preload_assets`, `resident_resource_kinds`, and `max_resident_packages`; `execute_selected_runtime_package_streaming_safe_point` rejects missing required preload assets and disallowed resident resource kinds with `residency_hint_failed` before staging or committing, preserving the active package/catalog on failure and reporting deterministic preload, resource-kind, and selected resident package-count counters on success. This further narrows `runtime-resource-v2` without claiming broad/background streaming, arbitrary eviction, resident caches, allocator/GPU enforcement, renderer/RHI resource ownership or teardown, public native/RHI handles, Metal readiness, hot reload, editor asset browser migration, or broad renderer quality.

Frame Graph Pass Callback Execution v1 is complete through [2026-05-08-frame-graph-pass-callback-execution-v1.md](../../plans/2026-05-08-frame-graph-pass-callback-execution-v1.md): `FrameGraphPassExecutionBinding`, `FrameGraphExecutionCallbacks`, `FrameGraphExecutionResult`, and `execute_frame_graph_v1_schedule` dispatch caller-owned barrier/pass callbacks over a linear Frame Graph v1 schedule with deterministic missing/failing callback diagnostics. This further narrows `frame-graph-v1` without claiming production graph ownership, renderer-wide pass migration, overlapping native alias execution, multi-queue scheduling, package streaming, or renderer-wide manual-transition removal.

Frame Graph RHI Texture Schedule Execution v1 is complete through [2026-05-08-frame-graph-rhi-texture-schedule-execution-v1.md](../../plans/2026-05-08-frame-graph-rhi-texture-schedule-execution-v1.md): `FrameGraphRhiTextureExecutionDesc`, `FrameGraphRhiTextureExecutionResult`, and `execute_frame_graph_rhi_texture_schedule` execute caller-owned RHI texture transitions and pass callbacks in deterministic `FrameGraphExecutionStep` order. This further narrows `frame-graph-v1` without claiming production graph ownership, renderer-wide pass migration, overlapping native alias execution, multi-queue scheduling, package streaming, or renderer-wide manual-transition removal.

Frame Graph Render Pass Envelope v1 is complete through [2026-05-17-frame-graph-render-pass-envelope-v1.md](../../plans/2026-05-17-frame-graph-render-pass-envelope-v1.md): `FrameGraphRhiRenderPassDesc`, `FrameGraphRhiRenderPassColorAttachment`, `FrameGraphRhiRenderPassDepthAttachment`, and `render_passes_recorded` let `execute_frame_graph_rhi_texture_schedule` prevalidate selected render pass envelopes, own `begin_render_pass` / `end_render_pass` around callbacks, and let `RhiPostprocessFrameRenderer` keep callbacks to scene/postprocess pass-body recording. This further narrows `frame-graph-v1` without claiming production graph ownership, broader renderer pass migration, package streaming, multi-queue scheduling, Vulkan/Metal memory alias allocation, or broad renderer quality.

Frame Graph Directional Shadow Render Pass Envelope v1 is complete through [2026-05-17-frame-graph-directional-shadow-render-pass-envelope-v1.md](../../plans/2026-05-17-frame-graph-directional-shadow-render-pass-envelope-v1.md): `RhiDirectionalShadowSmokeFrameRenderer` now declares shadow-depth, scene-receiver, and postprocess `FrameGraphRhiRenderPassDesc` envelopes, keeps callbacks to pass-body recording, and prepares native UI overlay outside executor-owned render pass scopes. This preserves the existing directional-shadow pass/barrier budgets and still leaves production graph ownership, broader production graph pass ownership, package streaming, multi-queue scheduling, Vulkan/Metal memory alias allocation, and broad renderer quality unsupported.

Frame Graph Remaining Render Pass Envelopes v1 is complete through [2026-05-17-frame-graph-remaining-render-pass-envelopes-v1.md](../../plans/2026-05-17-frame-graph-remaining-render-pass-envelopes-v1.md): `RhiFrameRenderer` now declares the raw `primary_color` `FrameGraphRhiRenderPassDesc` envelope while keeping queued primary/native-overlay recording in the callback, and `RhiViewportSurface::render_clear_frame()` now declares a `viewport.clear` envelope plus pass target-state/final-state rows. This closes the remaining high-level primary and viewport clear begin/end scopes without claiming production graph ownership, broader production graph pass ownership beyond selected executor slices, package streaming, multi-queue scheduling, Vulkan/Metal memory alias allocation, or broad renderer quality.

Frame Graph Multi-Queue Package Evidence v1 is complete through [2026-05-17-frame-graph-multiqueue-package-evidence-v1.md](../../plans/2026-05-17-frame-graph-multiqueue-package-evidence-v1.md): `execute_frame_graph_rhi_multi_queue_package_evidence` now packages a transient alias-group lease proof over a host-owned `IRhiDevice`, `sample_desktop_runtime_game --require-framegraph-multiqueue-evidence` emits `framegraph_multiqueue_*` command-list, queue-wait, texture-barrier, aliasing-barrier, submitted-fence, submit-count, and graphics-waited-for-copy fields, and installed validation checks those fields. The package helper exercises automatic aliasing barriers and an alias-induced cross-queue wait from the previous alias lifetime last use to the later alias lifetime first use. This keeps production graph adoption, async overlap/performance evidence, native queue/fence/semaphore exposure, public RHI handles, Vulkan/Metal production multi-queue readiness, and broad renderer quality unsupported.

Runtime Upload Frame Graph Transition Evidence v1 is complete through [2026-05-17-runtime-upload-frame-graph-transition-evidence-v1.md](../../plans/2026-05-17-runtime-upload-frame-graph-transition-evidence-v1.md): byte-backed `upload_runtime_texture` now routes `undefined -> copy_destination` pass target-state preparation, the copy callback, and `copy_destination -> shader_read` final-state restoration through `execute_frame_graph_rhi_texture_schedule`, reporting `RuntimeTextureUploadResult` Frame Graph barrier and callback counters. Metadata-only texture allocation still reports zero Frame Graph evidence, and mesh/material upload paths, broad/background streaming, upload staging rings, production graph ownership, async overlap/performance, and public native/RHI handles remain unsupported.

Runtime RHI Upload Submission Fence Rows v1 is complete through [2026-05-08-runtime-rhi-upload-submission-fence-rows-v1.md](../../plans/2026-05-08-runtime-rhi-upload-submission-fence-rows-v1.md): runtime texture, mesh, skinned mesh, morph mesh, and material-factor upload results now retain submitted `mirakana::rhi::FenceValue` rows, `RuntimeSceneGpuBindingResult::submitted_upload_fences` preserves actual scene upload submit order, and `RuntimeSceneGpuUploadExecutionReport` derives `submitted_upload_fence_count` plus `last_submitted_upload_fence`. This further narrows `upload-staging-v1` without claiming native async upload execution, runtime-wide ring-backed upload integration beyond reviewed texture/buffer uploads, package streaming, backend overlap, or renderer ownership changes.

RHI Upload Stale Generation Diagnostics v1 is complete through [2026-05-08-rhi-upload-stale-generation-diagnostics-v1.md](../../plans/2026-05-08-rhi-upload-stale-generation-diagnostics-v1.md): `RhiUploadStagingPlan` and `RhiUploadRing` now return the existing `RhiUploadDiagnosticCode::stale_generation` row for known allocation ids with mismatched generations, and ring ownership/byte-offset lookup require both id and generation. This further narrows `upload-staging-v1` without claiming native async upload execution, staging-ring production readiness, package streaming, backend overlap, or renderer ownership changes.

Runtime Ring-Backed Texture Upload v1 is complete through [2026-05-18-upload-staging-v1-runtime-ring-backed-texture-upload-v1.md](../../plans/2026-05-18-upload-staging-v1-runtime-ring-backed-texture-upload-v1.md): `RuntimeTextureUploadOptions::upload_ring` lets byte-backed `upload_runtime_texture` use caller-owned `RhiUploadRing` / `RhiUploadStagingPlan` staging, reuse one `copy_source` ring buffer, preserve the existing Frame Graph/fence counters, fail before texture upload side effects on ring exhaustion, and let package streaming texture transactions inherit the path through existing source upload options. Runtime Buffer Ring-Backed Uploads v1 is complete through [2026-05-18-upload-staging-v1-runtime-buffer-ring-backed-uploads-v1.md](../../plans/2026-05-18-upload-staging-v1-runtime-buffer-ring-backed-uploads-v1.md): `RuntimeMeshUploadOptions::upload_ring`, `RuntimeSkinnedMeshUploadOptions::upload_ring`, and `RuntimeMorphMeshUploadOptions::upload_ring` let byte-backed runtime mesh/skinned/morph uploads reuse caller-owned `RhiUploadRing` / `RhiUploadStagingPlan` staging, preserve Frame Graph/fence counters, fail before destination buffer or upload side effects on ring exhaustion, and keep runtime-scene teardown from releasing caller-owned ring buffers. Package Static Mesh Upload Binding Transaction v1 is complete through [2026-05-18-upload-staging-v1-package-static-mesh-upload-binding-transaction-v1.md](../../plans/2026-05-18-upload-staging-v1-package-static-mesh-upload-binding-transaction-v1.md): `upload_runtime_package_streaming_mesh_gpu_bindings` validates committed package streaming state, live resident static mesh rows, and explicit `RuntimeMeshPayload` rows before returning upload evidence plus renderer `MeshGpuBinding` rows. Package Skinned/Morph Upload Binding Transaction v1 is complete through [2026-05-18-upload-staging-v1-package-skinned-morph-upload-binding-transaction-v1.md](../../plans/2026-05-18-upload-staging-v1-package-skinned-morph-upload-binding-transaction-v1.md): `upload_runtime_package_streaming_skinned_mesh_gpu_bindings` and `upload_runtime_package_streaming_morph_mesh_gpu_bindings` validate matching resident skinned/morph rows and explicit payloads before returning upload evidence plus renderer GPU binding rows, submitted fences, uploaded-byte totals, Frame Graph command-list counters, and graphics-queue upload waits. RHI Native Async Upload Execution v1 is complete through [2026-05-18-upload-staging-v1-native-async-upload-execution-v1.md](../../plans/2026-05-18-upload-staging-v1-native-async-upload-execution-v1.md): `execute_upload_gpu_batch_async` submits caller-owned staging batches on a selected backend-neutral queue without helper-owned CPU waits. Runtime Upload Queue Wait v1 is complete through [2026-05-18-upload-staging-v1-runtime-upload-queue-wait-v1.md](../../plans/2026-05-18-upload-staging-v1-runtime-upload-queue-wait-v1.md): `wait_for_runtime_uploads_on_queue` and package upload `upload_queue_waits_recorded` rows let package transactions record graphics-queue waits for async copy-queue upload fences while leaving CPU waits to callers. Staging Pool Lease Adoption v1 is complete through [2026-05-18-upload-staging-v1-staging-pool-lease-adoption-v1.md](../../plans/2026-05-18-upload-staging-v1-staging-pool-lease-adoption-v1.md): explicit `RhiStagingBufferLease` chunks can back caller-owned `RhiUploadRing` uploads. Selected Package Upload Evidence v1 is complete through [2026-05-18-upload-staging-v1-selected-package-upload-evidence-v1.md](../../plans/2026-05-18-upload-staging-v1-selected-package-upload-evidence-v1.md): `RuntimePackageUploadStagingEvidence`, `execute_runtime_package_upload_staging_evidence`, and `--require-package-upload-staging` prove selected D3D12 generated 3D texture/static/skinned/morph package upload transactions over four pool lease-backed rings with submitted fences and graphics waits for copy-queue uploads. Package Resource Update Readiness v1 is complete through [2026-05-18-upload-staging-v1-async-ready-resource-updates-v1.md](../../plans/2026-05-18-upload-staging-v1-async-ready-resource-updates-v1.md): `RuntimePackageResourceUpdate`, `RuntimePackageResourceUpdateReadinessResult`, and `make_runtime_package_resource_update_readiness` publish selected texture/static/skinned/morph resource update rows only after committed streaming, live catalog, submitted upload fences, and graphics-queue wait evidence; `--require-package-upload-staging` now validates these resource update counters through installed package smoke. These slices close `upload-staging-v1` without claiming async overlap/performance, renderer-owned residency, public native handles, runtime-wide staging-pool ownership beyond explicit leases, or allocator/GPU budget enforcement.

Audio Device Streaming Baseline v1 is complete through [2026-05-08-audio-device-streaming-baseline-v1.md](../../plans/2026-05-08-audio-device-streaming-baseline-v1.md): `AudioDeviceStreamRequest`, `AudioDeviceStreamPlan`, `plan_audio_device_stream`, and `render_audio_device_stream_interleaved_float` provide a device-independent queue-fill/render pump over `AudioMixer` PCM so host adapters can render bounded chunks starting at `device_frame + queued_frames`, report `no_work`, and reject invalid formats, queue targets, render budgets, resampling quality, and device-frame overflow. This narrows the audio output/streaming baseline without claiming codec integration, streaming decode threads, HRTF, DSP graphs, device hotplug/selection, mixer authoring, SDL3/AAudio handles, or generated-game audio/navigation/AI composition evidence.

Historical physics sequence note: Physics Benchmark Determinism Gates v1 led to the completed Jolt/native adapter gate and Physics 1.0 closeout; the live `recommendedNextPlan.id` is `next-production-gap-selection`. The completed compute morph path already proves D3D12 POSITION output readback, renderer consumption, generated package POSITION/NORMAL/TANGENT counters, public RHI queue-to-queue ordering for graphics consumption without a CPU wait, package-visible queue-wait and async sequencing counters, runtime/renderer skin+compute composition, generated D3D12 skin+compute smoke, Vulkan native compute pipeline/bind/dispatch proof, Vulkan Runtime RHI compute morph POSITION readback, Vulkan renderer consumption of the compute-written POSITION output, Vulkan NORMAL/TANGENT output readback, generated Vulkan POSITION package smoke, generated Vulkan NORMAL/TANGENT package smoke, and generated Vulkan skin+compute package smoke. The completed 2D native slice promotes existing sprite batch planning and package telemetry into renderer-owned native sprite batch execution counters without changing sprite order, exposing native/RHI handles, or claiming runtime source parsing, renderer texture upload, Metal, package streaming execution, broad production sprite batching readiness, or broad renderer quality. The completed 2D Sprite Animation Package v1 slice adds deterministic generated `DesktopRuntime2DPackage` sprite animation frame sampling/application counters. The completed 2D Tilemap Editor Runtime UX v1 slice adds package-visible tilemap runtime/editor counters and diagnostics over existing deterministic `GameEngine.Tilemap.v1` package metadata through `mirakana::runtime::sample_runtime_tilemap_visible_cells`, `mirakana::editor::make_editor_tilemap_package_diagnostics_model`, and `--require-tilemap-runtime-ux`, without claiming source image decoding, production atlas packing, a full tilemap editor, package streaming execution, public native/RHI handles, Metal readiness, broad production sprite batching readiness, or broad renderer quality. The completed Runtime Input Rebinding Capture Contract v1 slice adds `RuntimeInputRebindingCaptureRequest`, `RuntimeInputRebindingCaptureResult`, and `capture_runtime_input_rebinding_action` for deterministic digital action capture from first-party key, pointer, and gamepad-button state. The completed Runtime Input Rebinding Focus Consumption v1 slice adds `RuntimeInputRebindingFocusCaptureRequest`, `RuntimeInputRebindingFocusCaptureResult`, and `capture_runtime_input_rebinding_action_with_focus` for armed/focused/modal capture ownership, `focus_retained`, and `gameplay_input_consumed` across waiting, captured, and pressed-but-rejected candidates without claiming full runtime/game rebinding panels, input glyphs, axis capture, device assignment, native handles, or file/cloud/binary save mutation. The completed Runtime Input Rebinding Presentation Rows v1 slice adds `RuntimeInputRebindingPresentationToken`, `RuntimeInputRebindingPresentationRow`, `RuntimeInputRebindingPresentationModel`, `present_runtime_input_action_trigger`, `present_runtime_input_axis_source`, and `make_runtime_input_rebinding_presentation` for deterministic reviewed base/profile action and axis display rows with symbolic glyph lookup keys, without claiming full runtime/game rebinding panels, real platform input glyph generation/assets/rendering, keyboard-layout localization, axis capture, device assignment, native handles, or file/cloud/binary save mutation. The completed Editor Input Rebinding Action Capture Panel v1 slice adds `EditorInputRebindingCaptureModel`, retained `input_rebinding.capture` rows, and visible editor action-row capture controls that apply captured candidates to the in-memory profile only, without claiming full runtime/game rebinding panels outside the reviewed editor action capture lane, broad UI focus integration/global input consumption beyond focused rebinding capture, glyph generation, axis capture, device assignment, native handles, or file/cloud/binary save mutation. The completed Shader Hot Reload And Pipeline Cache v1 slice adds selected-request batch planning and first-party cache-index reconciliation through `MK_tools` without invoking shader compilers, writing shader artifacts, exposing native PSO/Vulkan/Metal cache blobs, or claiming live shader generation, renderer/RHI residency, package streaming, public native handles, or broad renderer quality. The completed Renderer Package Quality Gates v1 slice adds `evaluate_sdl_desktop_presentation_quality_gate` plus `renderer_quality_*` installed smoke fields over existing scene GPU/postprocess/depth/shadow/framegraph counters without adding GPU timestamps, native handles, backend stats, cross-backend performance parity, or general production renderer quality. The completed Installed SDK Release Metadata Validation v1 slice adds `Assert-InstalledSdkMetadata` and `check-installed-sdk-validation` so `validate-installed-sdk` proves installed CMake package config/version files, parseable AI manifest/schemas, non-empty tools/examples/samples/docs/notices/license payloads, and the clean installed consumer build without claiming signing, upload, notarization, symbol publication, crash/telemetry backends, selected desktop-runtime game smoke, Android device matrix, or Apple-host readiness. The completed Desktop Release Package Evidence v1 slice adds `Assert-ReleasePackageArtifacts` and `check-release-package-artifacts` so `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1` proves the current CPack ZIP, SHA-256 sidecar, and archived SDK metadata/tools/docs/notices/license entries without claiming signing, upload, notarization, symbol publication, crash/telemetry backends, selected desktop-runtime package proof, Android device matrix, or Apple-host readiness. The completed Crash Telemetry Trace Ops v1 slice adds `DiagnosticsOpsPlan` and `build_diagnostics_ops_plan` so `MK_core` reports ready diagnostic summary and Chrome Trace Event JSON artifacts, host-gated native crash dump review through Debugging Tools for Windows, and unsupported telemetry upload without a caller-provided backend, without adding native dump writing, crash-report backends, telemetry SDK/upload execution, symbol publication, trace import tooling, flame graph UI, or native handles. The completed Android Release Device Matrix v1 slice proves `sample_headless` Debug APK build, Release APK build/signature verification with a user-local non-repository validation key, upload certificate export, and `Mirakanai_API36` API 36 emulator install/launch smoke after replacing Android-unsupported floating-point `std::from_chars` parsing in first-party metadata/session parsers with portable classic-locale parsing; it does not claim Play upload, production signing material, physical-device coverage, broader ABI/device matrix coverage, Apple/iOS readiness, or repository-owned keys. The completed Apple Metal iOS Host Evidence v1 slice adds `apple-host-evidence-check` diagnostic output for macOS/full-Xcode, `xcrun`, iOS Simulator, `metal`, `metallib`, and workflow coverage; it records this Windows host as `apple-host-evidence-check: host-gated` without claiming Apple/iOS/Metal readiness, signing, notarization, device coverage, or native Metal presentation. The completed Production 1.0 Readiness Audit v1 slice adds `production-readiness-audit-check` over `unsupportedProductionGaps` row shape, duplicate ids, status vocabulary, required-before-ready claims, and notes; it audits known non-ready rows without claiming 1.0 readiness.

Generated 3D Gameplay Systems Package Smoke v1 is complete through [2026-05-08-generated-3d-gameplay-systems-package-smoke-v1.md](../../plans/2026-05-08-generated-3d-gameplay-systems-package-smoke-v1.md): `sample_generated_desktop_runtime_3d_package` now supports `--require-gameplay-systems` and reports `gameplay_systems_status=ready`, `gameplay_systems_navigation_plan_status=ready`, `gameplay_systems_blackboard_status=ready`, `gameplay_systems_behavior_status=success`, and `gameplay_systems_audio_status=ready` over selected public `MK_physics`, `MK_navigation`, `MK_ai`, `MK_audio`, and `MK_animation` APIs. This narrows generated package gameplay composition evidence without claiming scene/physics perception integration, navmesh/crowd, middleware, codec streaming, native audio devices, dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, joints, Metal, editor productization, native/RHI handles, or broad generated 3D production readiness.

Runtime UI Accessibility Publish Plan v1 is complete for the host-independent `MK_ui` accessibility publish contract: `AccessibilityPublishPlan`, `AccessibilityPublishResult`, `plan_accessibility_publish`, and `publish_accessibility_payload` validate first-party `AccessibilityPayload` rows and block invalid bounds or prior diagnostics before `IAccessibilityAdapter` dispatch. It does not claim UI Automation, NSAccessibility, AT-SPI, Android or iOS accessibility publication, native accessibility objects, platform SDK calls, font rasterization, IME, image decoding, third-party adapters, or middleware readiness.

Runtime UI IME Composition Publish Plan v1 is complete for the host-independent `MK_ui` IME composition publish contract: `ImeCompositionPublishPlan`, `ImeCompositionPublishResult`, `plan_ime_composition_update`, and `publish_ime_composition` validate first-party `ImeComposition` target/cursor rows and block invalid target ids or out-of-range cursor indexes before `IImeAdapter` dispatch. It does not claim Win32/TSF, Cocoa text input, ibus/Fcitx, Android or iOS IME sessions, platform SDK calls, font rasterization, text shaping, image decoding, third-party adapters, middleware, or native handle readiness.

Runtime UI Platform Text Input Session Plan v1 is complete for the host-independent `MK_ui` platform text-input session contract: `PlatformTextInputSessionPlan`, `PlatformTextInputSessionResult`, `PlatformTextInputEndPlan`, `PlatformTextInputEndResult`, `plan_platform_text_input_session`, `begin_platform_text_input`, `plan_platform_text_input_end`, and `end_platform_text_input` validate first-party text-input begin/end target and bounds rows before `IPlatformIntegrationAdapter` dispatch. It does not claim Win32/TSF, Cocoa text input, ibus/Fcitx, Android or iOS IME sessions, virtual keyboard behavior, native text-input object/session ownership, platform SDK calls, text shaping, font rasterization, OS accessibility bridge publication, image decoding, third-party adapters, middleware, or native handle readiness.

Runtime UI Text Shaping Request Plan v1 is complete for the host-independent `MK_ui` text shaping request contract: `TextShapingRequestPlan`, `TextShapingResult`, `plan_text_shaping_request`, and `shape_text_run` validate first-party `TextLayoutRequest` values and adapter-returned `TextLayoutRun` rows before and after `ITextShapingAdapter` dispatch. It does not claim production text shaping implementation, bidirectional reordering, production line breaking, font fallback, platform SDK calls, renderer texture upload, third-party adapters, middleware, or native handle readiness.

Runtime UI Font Rasterization Request Plan v1 is complete for the host-independent `MK_ui` font request contract: `FontRasterizationRequestPlan`, `FontRasterizationResult`, `plan_font_rasterization_request`, and `rasterize_font_glyph` validate first-party `FontRasterizationRequest` font-family, glyph, pixel-size, and adapter-returned `GlyphAtlasAllocation` rows before and after `IFontRasterizerAdapter` dispatch. It does not claim font loading/rasterization implementation, glyph atlas generation, renderer texture upload, text shaping, bidi reordering, platform SDK calls, third-party adapters, middleware, or native handle readiness.

Runtime UI Image Decode Request Plan v1 is complete for the host-independent `MK_ui` image decode request contract: `ImageDecodeRequestPlan`, `ImageDecodeDispatchResult`, `ImageDecodePixelFormat`, `plan_image_decode_request`, and `decode_image_request` validate first-party `ImageDecodeRequest` asset URI/source bytes and adapter-returned RGBA8 `ImageDecodeResult` dimensions/pixel bytes before and after `IImageDecodingAdapter` dispatch. It does not claim runtime image decoding, source image codecs, SVG/vector parsing, atlas packing, renderer texture upload, platform SDK calls, third-party adapters, middleware, or native handle readiness.

Runtime UI PNG Image Decoding Adapter v1 is complete for the optional `MK_tools` bridge from first-party UI image decode requests to the reviewed PNG decoder lane: `PngImageDecodingAdapter` implements `mirakana::ui::IImageDecodingAdapter`, routes bytes through `decode_audited_png_rgba8` / `libspng` when `asset-importers` is enabled, maps RGBA8 bytes to `ImageDecodePixelFormat::rgba8_unorm`, and fails closed through `invalid_image_decode_result` when importers are disabled or decode output is invalid. It does not claim runtime-owned source image decoding, SVG/vector parsing, atlas packing, renderer texture upload, platform SDK calls, new third-party dependencies, middleware, or native handle readiness.

Runtime UI Decoded Image Atlas Package Bridge v1 is complete for the host-independent `MK_tools` bridge from validated `mirakana::ui::ImageDecodeResult` rows to cooked package artifacts: `PackedUiAtlasAuthoringDesc`, `author_packed_ui_atlas_from_decoded_images`, `plan_packed_ui_atlas_package_update`, and `apply_packed_ui_atlas_package_update` validate RGBA8 pixel byte counts, call `pack_sprite_atlas_rgba8_max_side`, emit one `GameEngine.CookedTexture.v1` atlas page, author `GameEngine.UiAtlas.v1` rows with `source.decoding=decoded-image-adapter` and `atlas.packing=deterministic-sprite-atlas-rgba8-max-side`, and synchronize the `.geindex` texture/ui_atlas entries plus `ui_atlas_texture` dependency rows. It does not claim runtime source parsing, SVG/vector parsing, renderer texture upload, platform SDK calls, new third-party dependencies, middleware, or broad production UI renderer quality.

Runtime UI Glyph Atlas Package Bridge v1 is complete for the host-independent bridge from already-rasterized RGBA8 glyph pixels to cooked package artifacts and renderer glyph bindings: `UiAtlasMetadataGlyph`, `RuntimeUiAtlasGlyph`, `PackedUiGlyphAtlasAuthoringDesc`, `author_packed_ui_glyph_atlas_from_rasterized_glyphs`, `plan_packed_ui_glyph_atlas_package_update`, `apply_packed_ui_glyph_atlas_package_update`, and `build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas` validate glyph identity rows, call the existing deterministic RGBA8 atlas packer, emit one `GameEngine.CookedTexture.v1` atlas page, author `GameEngine.UiAtlas.v1` rows with `source.decoding=rasterized-glyph-adapter` and `atlas.packing=deterministic-glyph-atlas-rgba8-max-side`, synchronize `.geindex` texture/ui_atlas entries, and build `UiRendererGlyphAtlasPalette` bindings from loaded runtime metadata. It does not claim font loading/rasterization implementation, text shaping, bidi reordering, native IME/text-input sessions, renderer texture upload, platform SDK calls, third-party adapters, middleware, or broad production UI renderer quality.

Runtime UI Font Image Adapter v1 note: completed UI runtime work now adds stable glyph ids to deterministic `MonospaceTextLayoutPolicy` glyph rows and lets `MK_ui_renderer` resolve them through `UiRendererGlyphAtlasPalette` / `UiRendererGlyphAtlasBinding` into atlas-backed sprite submissions with `text_glyph_sprites_submitted` counters. This does not claim font loading/rasterization, glyph atlas generation, shaping, bidi reordering, IME, OS accessibility publication, runtime image decoding, production atlas packing, SDL3, Dear ImGui, UI middleware, native handles, or editor-private runtime APIs.

Runtime UI Accessibility Publish Plan v1 note: completed UI runtime work now adds host-independent accessibility publication planning and result reporting before the existing `IAccessibilityAdapter` boundary. This does not claim OS accessibility bridge publication, native accessibility handles/objects, platform SDK calls, IME, font rasterization, image decoding, UI middleware, SDL3, Dear ImGui, or editor-private runtime APIs.

Runtime UI IME Composition Publish Plan v1 note: completed UI runtime work now adds host-independent IME composition publication planning and result reporting before the existing `IImeAdapter` boundary. This does not claim native IME/text-input sessions, platform SDK calls, text shaping, font rasterization, OS accessibility bridge publication, image decoding, UI middleware, SDL3, Dear ImGui, or editor-private runtime APIs.

Runtime UI Platform Text Input Session Plan v1 note: completed UI runtime work now adds host-independent text-input begin/end session planning and result reporting before the existing `IPlatformIntegrationAdapter` boundary. This does not claim native IME/text-input sessions, native text-input object/session ownership, virtual keyboard behavior, platform SDK calls, text shaping, font rasterization, OS accessibility bridge publication, image decoding, UI middleware, SDL3, Dear ImGui, or editor-private runtime APIs.

Runtime UI SDL3 Platform Text Input Adapter v1, Runtime UI SDL3 Text Input Event Translation v1, Runtime UI SDL3 Text Editing Candidate Event Translation v1, Runtime UI SDL3 IME Composition Event Publish v1, Runtime UI SDL3 Committed Text Edit Apply v1, Runtime UI Text Edit Command Apply v1, Runtime UI SDL3 Text Edit Command Key Events v1, and Runtime UI Clipboard Text Request v1 notes: completed child work adds an optional `MK_platform_sdl3` implementation of `IPlatformIntegrationAdapter` for `SdlWindow` text-input area/start/stop dispatch, copied first-party `SdlWindowEvent` rows for `SDL_EVENT_TEXT_INPUT`, `SDL_EVENT_TEXT_EDITING`, and `SDL_EVENT_TEXT_EDITING_CANDIDATES` committed text, composition text/range metadata, candidate strings, selected candidate index, and candidate-list orientation, `sdl3_ime_composition_from_window_event` / `publish_sdl3_ime_composition_event` for publishing copied SDL3 text-editing composition rows through `IImeAdapter` after converting SDL UTF-8 character cursor positions to byte offsets, `TextEditState` / `CommittedTextInput` / `plan_committed_text_input` / `apply_committed_text_input` and `sdl3_committed_text_from_window_event` / `apply_sdl3_committed_text_event` for validated committed UTF-8 insertion/replacement, `TextEditCommandKind` / `TextEditCommand` / `plan_text_edit_command` / `apply_text_edit_command` for host-independent scalar-boundary cursor movement and backward/forward deletion, `IClipboardTextAdapter` / `plan_clipboard_text_write` / `read_clipboard_text` for host-independent text clipboard rows, `SdlClipboardTextAdapter` for optional SDL3 text clipboard dispatch, plus `sdl3_text_edit_command_from_window_event` / `apply_sdl3_text_edit_command_event` for selected SDL3 key-down row command mapping. This still does not claim cross-platform native IME/text-input services, full text editing widget behavior, platform key mapping beyond those selected command keys, key repeat policy, selection UI, copy/cut/paste widget behavior, rich clipboard formats, word movement, grapheme-cluster editing, candidate UI, virtual keyboard behavior, native text-input object/session ownership, text shaping, font rasterization, OS accessibility bridge publication, image decoding, UI middleware, Dear ImGui, or editor-private runtime APIs.

Runtime UI Font Rasterization Request Plan v1 note: completed UI runtime work now adds host-independent font rasterization request planning and result reporting before the existing `IFontRasterizerAdapter` boundary. This does not claim real font loading/rasterization, glyph atlas generation, renderer texture upload, text shaping, native IME/text-input sessions, OS accessibility bridge publication, image decoding, UI middleware, SDL3, Dear ImGui, or editor-private runtime APIs.

Runtime UI Image Decode Request Plan v1 note: completed UI runtime work now adds host-independent image decode request planning and result reporting before the existing `IImageDecodingAdapter` boundary. This does not claim real source image decoding, SVG/vector parsing, atlas packing, renderer texture upload, platform SDK calls, UI middleware, SDL3, Dear ImGui, or editor-private runtime APIs.

Runtime UI PNG Image Decoding Adapter v1 note: completed UI runtime work now adds an optional `MK_tools` `PngImageDecodingAdapter` over `decode_audited_png_rgba8` so reviewed PNG bytes can satisfy `IImageDecodingAdapter` requests in the `asset-importers` lane while failing closed in default builds. This does not claim broader source image codecs, SVG/vector parsing, atlas packing, renderer texture upload, platform SDK calls, UI middleware, SDL3, Dear ImGui, or editor-private runtime APIs.

Queue synchronization API note: completed compute-morph queue ordering uses `IRhiDevice::wait_for_queue` for graphics consumption without a host-side CPU wait.

Async overlap evidence note: completed D3D12 overlap evidence is limited to `RhiAsyncOverlapReadinessDiagnostics`, which reports `not_proven_serial_dependency` for the current same-frame graphics wait.

Async telemetry package note: generated D3D12 packages use `--require-compute-morph-async-telemetry` to expose first-party `scene_gpu_compute_morph_async_` counters only.

RHI D3D12 Per-Queue Fence Synchronization v1, RHI D3D12 Queue Timestamp Measurement Foundation v1, RHI D3D12 Queue Clock Calibration Foundation v1, Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12 v1, RHI D3D12 Submitted Command Calibrated Timing Scopes v1, Runtime RHI Compute Morph Submitted Overlap Diagnostics D3D12 v1, RHI Vulkan Compute Dispatch Foundation v1, Runtime RHI Compute Morph Vulkan Proof v1, Runtime RHI Compute Morph Renderer Consumption Vulkan v1, Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan v1, Generated 3D Compute Morph Package Smoke Vulkan v1, Generated 3D Compute Morph NORMAL/TANGENT Package Smoke Vulkan v1, Generated 3D Compute Morph Skin Package Smoke Vulkan v1, Generated 3D Package Streaming Safe Point Smoke v1, Generated 3D Renderer Quality Package Smoke v1, Generated 3D Playable Package Smoke v1, Generated 3D Postprocess Depth Package Smoke v1, Generated 3D Directional Shadow Package Smoke v1, 2D Native Sprite Batching Execution v1, 2D Sprite Animation Package v1, 2D Tilemap Editor Runtime UX v1, Shader Hot Reload And Pipeline Cache v1, Renderer Package Quality Gates v1, Desktop Release Package Evidence v1, Crash Telemetry Trace Ops v1, Generated 3D Committed Package Sample v1, Editor Runtime Host Playtest Launch v1, Editor Content Browser Import Native Dialog v1, Editor Content Browser Import External Copy Review v1, Editor Content Browser Import Codec Adapter Review v1, CI Matrix Contract Check v1, C++23 Release Package Artifact CI Evidence v1, Full Repository Static Analysis CI Contract v1, Runtime Resource Residency Hints Execution v1, Frame Graph Pass Callback Execution v1, Frame Graph RHI Texture Schedule Execution v1, Frame Graph RHI Pass Target State Execution v1, RHI Upload Stale Generation Diagnostics v1, Runtime UI Accessibility Publish Plan v1, Runtime UI IME Composition Publish Plan v1, Runtime UI Platform Text Input Session Plan v1, Runtime UI SDL3 Platform Text Input Adapter v1, Runtime UI SDL3 Text Input Event Translation v1, Runtime UI SDL3 Text Editing Candidate Event Translation v1, Runtime UI SDL3 IME Composition Event Publish v1, Runtime UI SDL3 Committed Text Edit Apply v1, Runtime UI Text Edit Command Apply v1, Runtime UI SDL3 Text Edit Command Key Events v1, Runtime UI Clipboard Text Request v1, Runtime UI Text Shaping Request Plan v1, Runtime UI Font Rasterization Request Plan v1, Runtime UI Image Decode Request Plan v1, Runtime UI PNG Image Decoding Adapter v1, Runtime UI Decoded Image Atlas Package Bridge v1, and Runtime UI Glyph Atlas Package Bridge v1 are complete; `next-production-gap-selection` is the current selection gate for the next master-plan gap.

Generated 3D Committed Package Sample v1, Generated 3D Compute Morph NORMAL/TANGENT Package Smoke Vulkan v1, Generated 3D Compute Morph Skin Package Smoke Vulkan v1, Generated 3D Package Streaming Safe Point Smoke v1, Generated 3D Renderer Quality Package Smoke v1, Generated 3D Playable Package Smoke v1, Generated 3D Postprocess Depth Package Smoke v1, Generated 3D Directional Shadow Package Smoke v1, and Generated 3D Shadow Morph Composition Package Smoke v1 add `games/sample_generated_desktop_runtime_3d_package` as a committed `DesktopRuntime3DPackage` proof with cooked-only runtime payloads, manifest-derived `MK_add_desktop_runtime_game` package metadata, camera/controller source-tree smoke, transform/morph/quaternion package smokes, selected safe-point package streaming counters, selected scene GPU plus depth-aware postprocess renderer quality counters with `framegraph_passes=2`, `framegraph_passes_executed=4`, `framegraph_render_passes_recorded=4`, `framegraph_barrier_steps_executed=9`, `renderer_quality_expected_framegraph_render_passes=4`, `renderer_quality_framegraph_render_passes_ok=1`, and `renderer_quality_expected_framegraph_barrier_steps=9`, selected directional shadow package smoke counters with fixed PCF 3x3 filtering, `framegraph_passes=3`, `framegraph_passes_executed=6`, `framegraph_render_passes_recorded=6`, and `framegraph_barrier_steps_executed=15`, selected graphics morph + directional shadow receiver package smoke counters through `--require-shadow-morph-composition` with `renderer_gpu_morph_draws`, `renderer_morph_descriptor_binds`, `framegraph_render_passes_recorded=6`, and `framegraph_barrier_steps_executed=15`, selected `playable_3d_*` aggregate counters, D3D12 compute morph skin+async telemetry metadata, and Vulkan POSITION/NORMAL/TANGENT plus skin+compute morph toolchain-gated metadata. This advances `3d-playable-vertical-slice` beyond scaffold-only telemetry, but broad generated 3D production readiness, dependency cooking, broad async/background package streaming, production directional-shadow quality, broad shadow+morph composition beyond the selected receiver smoke, morph-deformed shadow-caster silhouettes, production material/shader graph and live shader generation, Vulkan async overlap/performance, Metal parity, editor productization, graphics morph+skin composition, and general renderer quality remain unsupported.

Editor Profiler Native Trace Open Dialog v1 adds `EditorProfilerTraceOpenDialogModel`, `make_editor_profiler_trace_open_dialog_request`, `make_editor_profiler_trace_open_dialog_model`, retained `profiler.trace_open_dialog` rows, and visible `Browse Trace JSON` wiring through `mirakana::SdlFileDialogService` so accepted in-project `.json` selections reuse the existing project-relative Profiler Trace JSON file import review. Editor Profiler Native Trace Save Dialog v1 adds `EditorProfilerTraceSaveDialogModel`, `make_editor_profiler_trace_save_dialog_request`, `make_editor_profiler_trace_save_dialog_model`, retained `profiler.trace_save_dialog` rows, and visible `Browse Save Trace JSON` wiring through `mirakana::SdlFileDialogService` so accepted in-project `.json` selections update `Trace Path` and reuse the existing project-relative Profiler Trace JSON file save path. Editor Profiler Trace Import Reconstruction v1 adds `DiagnosticsTraceImportResult`, `import_diagnostics_trace_json`, reconstructed capture rows on `EditorProfilerTraceImportReviewModel` / `EditorProfilerTraceFileImportResult`, and retained `profiler.trace_import.reconstructed_*` UI rows for the first-party exported `M`/`i`/`C`/`X` Trace Event JSON subset. These complete the narrow native Profiler Trace JSON save/open paths plus first-party trace capture reconstruction while broader editor native save/open dialogs outside Profiler, Scene, and Prefab Variant, arbitrary trace conversion beyond that first-party subset, telemetry SDK/upload/backend follow-ups beyond Profiler telemetry handoff, crash-report backends, native handles, and production flame graphs remain unsupported.

Play-In-Editor Session Isolation v1 adds `MK_editor_core` `EditorPlaySession` and `EditorPlaySessionReport` as a GUI-independent source-vs-simulation scene isolation model. It snapshots `SceneAuthoringDocument` into an isolated simulation scene, reports source-scene edit blocking while active, tracks simulation ticks, and discards simulation edits on stop. This advances `editor-productization` from planned to partly-ready for the narrow isolation contract only; gameplay execution inside Play-In-Editor, package script execution, renderer/RHI handle exposure, nested prefab propagation/merge resolution UX, resource panels, shared AI command diagnostics, Unity/UE-like editor UX, and general renderer quality remain unsupported.

Play-In-Editor Gameplay Driver v1 adds `IEditorPlaySessionDriver` and `EditorPlaySessionTickContext` so caller-supplied begin/tick/end callbacks can mutate only the isolated simulation scene during an editor-core play session. This advances Play-In-Editor from pure isolation to a narrow reviewed callback execution contract. Dynamic game module loading, `DesktopGameRunner` embedding, package scripts, validation recipe execution, renderer/RHI uploads, native handles, hot reload, package streaming, nested prefab propagation/merge resolution UX, full editor productization, and general renderer quality remain unsupported.

Play-In-Editor Visible Viewport Wiring v1 adds `EditorPlaySessionControlsModel`, visible `MK_editor` Run menu and Viewport toolbar commands over `EditorPlaySession`, simulation-scene viewport rendering while active, per-viewport-frame play ticks, and source-scene authoring plus undo/redo blocking. Dynamic game module loading, `DesktopGameRunner` embedding, package scripts, validation recipe execution, renderer/RHI uploads from editor core, native handles, hot reload, package streaming, nested prefab propagation/merge resolution UX, full editor productization, and general renderer quality remain unsupported.

Editor Runtime Host Playtest Launch v1 adds `EditorRuntimeHostPlaytestLaunchDesc`, `EditorRuntimeHostPlaytestLaunchModel`, `make_editor_runtime_host_playtest_launch_model`, retained `MK_ui` `play_in_editor.runtime_host` rows, and visible `MK_editor` Run/Viewport `Execute Runtime Host` controls for reviewed external runtime-host commands. `MK_editor_core` validates caller-supplied argv tokens, working directory, host gates, acknowledgement, and unsupported claims before exposing a safe `mirakana::ProcessCommand`; the optional shell owns explicit host-gate acknowledgement, Windows `mirakana::Win32ProcessRunner` execution, and transient exit/stdout/stderr evidence. Dynamic game-module loading, in-process runtime-host embedding, package scripts, validation recipe execution from editor core, arbitrary shell, raw manifest command evaluation, free-form manifest edits, renderer/RHI/native handles, hot reload, package streaming, nested prefab propagation/merge resolution UX, full editor productization, and general renderer quality remain unsupported.

Editor Resource Panel Diagnostics v1 adds `EditorResourcePanelModel`, a hidden-by-default workspace `resources` panel, retained `MK_ui` resource panel output, and visible `MK_editor` Resources diagnostics over active viewport RHI stats, memory diagnostics, and optional lifetime registry summaries. Editor Resource Capture Request v1 adds reviewed `resources.capture_requests` rows plus visible transient acknowledgement/log state for PIX GPU capture and D3D12 debug-layer/GPU-validation host handoff. Editor Resource Capture Execution Evidence v1 adds `EditorResourceCaptureExecutionInput`, `EditorResourceCaptureExecutionRow`, retained `resources.capture_execution` rows, and visible host-gated waiting evidence for acknowledged capture requests. Residency enforcement, allocator policy, eviction, package streaming, PIX/debug-layer/ETW process execution, GPU capture execution, backend destruction migration, public native handles, full editor productization, and general renderer quality remain unsupported.

Editor AI Command Diagnostics Panel v1 adds `EditorAiCommandPanelModel`, a hidden-by-default workspace `ai_commands` panel, retained `MK_ui` AI Commands output, and visible `MK_editor` AI Commands diagnostics over existing AI package/preflight/readiness/handoff/evidence/remediation/operator workflow rows. Validation recipe execution, arbitrary shell, raw manifest command evaluation, free-form manifest edits, package scripts, dynamic game-module loading or in-process runtime-host embedding, renderer/RHI handles, Metal readiness, full editor productization, and broad renderer quality remain unsupported.

Editor AI Evidence Import Review v1 adds `EditorAiPlaytestEvidenceImportModel`, `EditorAiPlaytestEvidenceImportReviewRow`, `make_editor_ai_playtest_evidence_import_model`, retained `MK_ui` `ai_evidence_import` output, and visible `MK_editor` AI Commands paste/review/import controls for externally supplied `GameEngine.EditorAiPlaytestEvidence.v1` rows. Imported evidence remains transient editor state and feeds the existing evidence summary/workflow models. Host-gated AI command execution, arbitrary shell, raw manifest command evaluation, free-form manifest edits, package scripts, dynamic game-module loading or in-process runtime-host embedding, renderer/RHI handles, Metal readiness, full editor productization, and broad renderer quality remain unsupported.

Editor AI Reviewed Validation Execution v1 adds `EditorAiReviewedValidationExecutionModel`, `EditorAiReviewedValidationExecutionDesc`, `make_editor_ai_reviewed_validation_execution_plan`, retained `MK_ui` `ai_commands.execution` output, and visible `MK_editor` AI Commands execution controls for host-gate-free reviewed `run-validation-recipe` rows. Host-Gated Validation Execution Ack v1 (`editor-ai-host-gated-validation-execution-ack-v1`) extends that path with explicit per-row host-gate acknowledgement, retained acknowledgement ids, and reviewed `-HostGateAcknowledgements` command arguments for acknowledged host-gated rows. Editor AI Reviewed Validation Batch Execution v1 adds `EditorAiReviewedValidationExecutionBatchModel`, `make_editor_ai_reviewed_validation_execution_batch`, retained `ai_commands.execution.batch` summary rows, and visible `Execute Ready` for all currently executable reviewed rows. Results are stored as transient evidence and feed the existing evidence summary/workflow models. Unacknowledged host-gated AI command execution, arbitrary shell, raw manifest command evaluation, free-form manifest edits, broad package script execution, dynamic game-module loading or in-process runtime-host embedding, renderer/RHI handles, Metal readiness, full editor productization, and broad renderer quality remain unsupported.

Editor Content Browser Import Diagnostics v1 adds `EditorContentBrowserImportPanelModel`, retained `MK_ui` Assets panel output, and visible `MK_editor` Assets panel rendering over Content Browser rows, selected asset details, import queue/progress, diagnostics, dependencies, thumbnail requests, material preview rows, and hot-reload summaries. Import/recook/hot reload execution, arbitrary shell, package scripts, validation execution, free-form manifest edits, package streaming, renderer/RHI/native handles, dynamic game-module loading or in-process runtime-host embedding, full editor productization, and broad renderer quality remain unsupported.

Editor Source Registry Visible Content Browser v1 is recorded by [2026-05-12-editor-source-registry-visible-content-browser-v1.md](../../plans/2026-05-12-editor-source-registry-visible-content-browser-v1.md) and adds project-owned `GameEngine.SourceAssetRegistry.v1` loading plus the visible `Reload Source Registry` control without changing the unsupported editor execution claims.

Editor Content Browser Import Native Dialog v1 adds `EditorContentBrowserImportOpenDialogModel`, `make_content_browser_import_open_dialog_request`, `make_content_browser_import_open_dialog_model`, `make_content_browser_import_open_dialog_ui_model`, retained `MK_ui` `content_browser_import.open_dialog` rows, and visible `MK_editor` `Browse Import Sources` controls through `mirakana::SdlFileDialogService`. Accepted in-project `.texture`, `.mesh`, `.material`, `.scene`, `.audio_source`, `.png`, `.gltf`, `.glb`, `.wav`, `.mp3`, and `.flac` selections replace the deterministic import plan without executing import automatically.

Editor Content Browser Import External Copy Review v1 adds `EditorContentBrowserImportExternalSourceCopyModel`, `make_content_browser_import_external_source_copy_model`, `make_content_browser_import_external_source_copy_ui_model`, retained `MK_ui` `content_browser_import.external_copy` rows, and visible `MK_editor` `Copy External Sources` controls. External `.texture`, `.mesh`, `.material`, `.scene`, `.audio_source`, `.png`, `.gltf`, `.glb`, `.wav`, `.mp3`, and `.flac` selections are reviewed, block existing targets, copy under the project asset root at `imported_sources/<filename>` only after the explicit copy action, and then rebuild the deterministic import plan from copied project-relative source paths.

Editor Content Browser Import Codec Adapter Review v1 maps reviewed `.png` selections to texture import actions, `.gltf` / `.glb` selections to mesh import actions, and `.wav` / `.mp3` / `.flac` selections to audio import actions. Visible `MK_editor` explicit `Import Assets` and recook execution pass `mirakana::ExternalAssetImportAdapters::options()` into `execute_asset_import_plan`, using the optional `asset-importers` lane when available and stable disabled-adapter diagnostics otherwise. Arbitrary importer adapters, arbitrary file import, automatic import execution, package scripts, validation recipe execution, arbitrary shell, shader compiler execution, renderer/RHI/native handles, package streaming, manifest mutation, dynamic game-module loading, full editor productization, and broad importer readiness remain unsupported.

Editor Material Asset Preview Diagnostics v1 adds `EditorMaterialAssetPreviewPanelModel`, retained `MK_ui` selected-material output, and visible `MK_editor` Assets panel rendering over selected cooked material metadata, factor rows, texture dependency diagnostics, typed GPU payload texture rows, and D3D12/Vulkan material-preview shader readiness. Editor Material GPU Preview Execution Evidence v1 adds `EditorMaterialGpuPreviewExecutionSnapshot`, `apply_editor_material_gpu_preview_execution_snapshot`, retained `material_asset_preview.gpu.execution` rows, and visible `MK_editor` host-owned cache snapshot reporting for selected-material GPU preview status/backend/display/frame evidence. Shader compiler execution, RHI upload/display from editor core, arbitrary shell, package scripts, validation execution, free-form manifest edits, package streaming, renderer/RHI/native handles, Vulkan display parity, Metal readiness, full editor productization, and broad renderer quality remain unsupported.

Editor Input Rebinding Profile Panel v1 adds `EditorInputRebindingProfilePanelModel`, retained `MK_ui` `input_rebinding` output, workspace `input_rebinding` panel state, and visible `MK_editor` Input Rebinding panel rendering over reviewed `GameEngine.RuntimeInputRebindingProfile.v1` base/profile rows, save readiness, conflict diagnostics, and unsupported-claim rows. Runtime Input Rebinding Focus Consumption v1 adds `RuntimeInputRebindingFocusCaptureRequest`, `RuntimeInputRebindingFocusCaptureResult`, and `capture_runtime_input_rebinding_action_with_focus` for host-independent armed/focused/modal digital capture ownership, `focus_retained`, and `gameplay_input_consumed` without editor, SDL3, Dear ImGui, or native handles. Runtime Input Rebinding Presentation Rows v1 adds `RuntimeInputRebindingPresentationToken`, `RuntimeInputRebindingPresentationRow`, `RuntimeInputRebindingPresentationModel`, `present_runtime_input_action_trigger`, `present_runtime_input_axis_source`, and `make_runtime_input_rebinding_presentation` for deterministic reviewed base/profile action and axis display rows with symbolic glyph lookup keys, without platform input glyph generation, keyboard-layout localization, editor code, SDL3, Dear ImGui, or native handles. Editor Input Rebinding Action Capture Panel v1 adds `EditorInputRebindingCaptureModel`, `make_editor_input_rebinding_capture_action_model`, `make_input_rebinding_capture_action_ui_model`, retained `input_rebinding.capture` rows, visible action-row capture controls, and in-memory profile candidate application for captured first-party digital action triggers. Full runtime/game rebinding panels outside the reviewed editor action capture lane, axis capture, file mutation, command execution, broad UI focus integration/global input consumption beyond focused rebinding capture, real platform input glyph generation/assets/rendering, multiplayer device assignment, SDL3/native handles, cloud/binary saves, full editor productization, and broad input middleware remain unsupported.

Editor Prefab Variant Conflict Review v1 adds `PrefabVariantConflictReviewModel`, `PrefabVariantConflictRow`, `make_prefab_variant_conflict_review_model`, retained `MK_ui` `prefab_variant_conflicts` output, and visible `MK_editor` Prefab Variant Authoring Conflict Review rows over existing `PrefabVariantAuthoringDocument` override data. Editor Prefab Variant Reviewed Resolution v1 adds reviewed cleanup metadata, `resolve_prefab_variant_conflict`, `make_prefab_variant_conflict_resolution_action`, retained resolution labels, and visible `Apply` controls for redundant and later duplicate override rows. Editor Prefab Variant Missing Node Cleanup v1 adds `deserialize_prefab_variant_definition_for_review`, review-cleanup-repairable authoring documents, retained missing-node resolution labels, and visible `Apply` cleanup for stale overrides whose target node is absent from the base prefab. Editor Prefab Variant Node Retarget Review v1 adds optional `PrefabNodeOverride::source_node_name` / `override.N.source_node_name` hints, editor-authored hint population, retained resolution kind/target labels, and visible `Apply` retarget for stale missing-node overrides only when the source-node hint uniquely matches one current base-prefab node without creating a duplicate node/kind override. Editor Prefab Variant Batch Resolution Review v1 adds `PrefabVariantConflictBatchResolutionPlan`, `PrefabVariantConflictBatchResolutionResult`, `resolve_prefab_variant_conflicts`, `make_prefab_variant_conflict_batch_resolution_action`, retained `prefab_variant_conflicts.batch_resolution` rows, and visible `Apply All Reviewed` controls that apply currently reviewed cleanup/retarget/accept-current rows as one undoable action after recomputing the review model per row. Editor Prefab Variant Source Mismatch Retarget Review v1 adds blocking `source_node_mismatch` rows for existing-node overrides whose non-empty hint no longer matches the current indexed base node, keeps strict `MK_scene` composition index-based, and offers the same reviewed `Retarget override to node N` path only when the hint uniquely matches a different current node without creating a duplicate node/kind override. Editor Prefab Variant Source Mismatch Accept Current Review v1 adds reviewed `accept_current_node` / `Accept current node N` metadata for existing-node source mismatch rows when no safe unique retarget exists and updates only the `PrefabNodeOverride::source_node_name` hint to the current indexed node name. Editor Prefab Variant Native Dialog v1 adds `EditorPrefabVariantFileDialogModel`, `make_prefab_variant_open_dialog_request`, `make_prefab_variant_save_dialog_request`, `make_prefab_variant_open_dialog_model`, `make_prefab_variant_save_dialog_model`, retained `prefab_variant_file_dialog.open` / `prefab_variant_file_dialog.save` rows, and visible `Browse Load Variant` / `Browse Save Variant` wiring through `mirakana::SdlFileDialogService` for in-project `.prefabvariant` selections. Editor Prefab Variant Base Refresh Merge Review v1 adds `PrefabVariantBaseRefreshPlan`, `PrefabVariantBaseRefreshRow`, `PrefabVariantBaseRefreshResult`, `plan_prefab_variant_base_refresh`, `apply_prefab_variant_base_refresh`, `make_prefab_variant_base_refresh_action`, and `make_prefab_variant_base_refresh_ui_model` for retained `prefab_variant_base_refresh` rows and explicit embedded-base refresh apply. The lane reports blocking missing-node, duplicate-override, source-node-mismatch rows, missing base-refresh hints, ambiguous refreshed source-node matches, and duplicate base-refresh target keys, non-blocking redundant/component-family warnings, can-compose state, deterministic diagnostics, explicit undoable cleanup/retarget/accept-current/batch/base-refresh apply, and deterministic native `.prefabvariant` open/save review without mutating files, scenes, manifests, packages, or runtime state from the editor-core models. Nested prefab propagation, fuzzy matching, automatic merge/rebase/resolution UX, package scripts, validation recipes, dynamic game-module loading or in-process runtime-host embedding, renderer/RHI uploads, native handles, package streaming, broader editor native save/open dialogs outside Profiler, Scene, and Prefab Variant, full editor productization, and general renderer quality remain unsupported.

Editor Scene Native Dialog v1 adds `EditorSceneFileDialogModel`, `make_scene_open_dialog_request`, `make_scene_save_dialog_request`, `make_scene_open_dialog_model`, `make_scene_save_dialog_model`, `make_scene_file_dialog_ui_model`, retained `scene_file_dialog.open` / `scene_file_dialog.save` rows, and visible `Open Scene...`, `Save Scene As...`, `Browse Open Scene`, and `Browse Save Scene As` controls through `mirakana::SdlFileDialogService` for in-project `.scene` selections. The optional shell converts accepted native paths back to safe project-relative scene paths, updates `SceneAuthoringDocument::set_scene_path` for save-as, reuses existing scene load/save helpers, and preserves the existing scene replacement path that stops active Play-In-Editor and clears document undo state. SDL3/native handles, filesystem IO, path conversion, dynamic game-module loading or in-process runtime-host embedding, renderer/RHI uploads, package scripts, validation recipes, arbitrary project browsing, and broader editor native save/open dialogs outside Profiler, Scene, and Prefab Variant remain unsupported from editor core.

Editor Project Native Dialog v1 adds `EditorProjectFileDialogModel`, `EditorProjectFileDialogMode`, `make_project_open_dialog_request`, `make_project_save_dialog_request`, `make_project_open_dialog_model`, `make_project_save_dialog_model`, `make_project_file_dialog_ui_model`, retained `project_file_dialog.open` / `project_file_dialog.save` rows, and visible `Open Project...` / `Save Project As...` controls through `mirakana::SdlFileDialogService` for in-store `.geproject` selections. The optional shell converts accepted native paths back to safe store-relative project paths, infers companion workspace/startup-scene bundle paths, reuses existing `load_project_bundle` / `save_project_bundle` helpers, and preserves the existing project/scene replacement paths. SDL3/native handles, filesystem IO, path conversion, dynamic game-module loading or in-process runtime-host embedding, renderer/RHI uploads, package scripts, validation recipes, arbitrary project browsing beyond the selected store, and broader editor native save/open dialogs outside Profiler, Scene, Prefab Variant, and Project remain unsupported from editor core.
-->

Runtime UI and input ledger note: RuntimeInputRebindingPresentationModel; platform input glyph generation.

Editor profiler trace export contract retained evidence: Profiler trace JSON copy export; project-relative Profiler trace JSON file save; native Profiler Trace JSON save dialog; read-only Profiler telemetry handoff rows; pasted Profiler Trace JSON review; project-relative Profiler Trace JSON file import review/reconstruction; native Profiler Trace JSON open dialog; broader editor native save/open dialogs outside Profiler; arbitrary trace conversion beyond first-party exported Trace Event JSON reconstruction; telemetry SDK/upload/backend follow-ups.

Editor Resource Capture Request v1 retained evidence: reviewed Resources capture request handoff rows; resource management/capture execution beyond host-owned evidence rows.

Editor resource capture execution evidence contract retained evidence: host-owned Resources capture execution evidence rows.

Editor AI Command Diagnostics Panel v1 retained evidence: unacknowledged or automatic host-gated AI command execution; raw manifest command evaluation.

## Retained static-check needle ledger

The following literals are retained so `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` can keep historical plan evidence discoverable after the master plan split. These are not a substitute for the active chapter guidance above.

- `
        )
    },
    @{
        Path = `
- `
        Needles = @(
            `
- `
        if (Test-Path -LiteralPath $splitRoot) {
            Get-ChildItem -LiteralPath $splitRoot -Filter `
- `
        }
    }
}
$editorInputRebindingActionCaptureDocs = @{
    `
- `
        }
    }
}
$geUiHeaderText = Get-Content -LiteralPath (Join-Path $root `
- `
        }
    }
}
$runtimeInputRebindingCaptureDocs = @{
    `
- `
        }
    }
}
$runtimeInputRebindingFocusConsumptionDocs = @{
    `
- `
        }
    }
}
$runtimeInputRebindingPresentationRowsDocs = @{
    `
- `
        }
    }
}
$runtimeUiDecodedAtlasDocs = @{
    `
- `
        }
    }
}
$runtimeUiGlyphAtlasDocs = @{
    `
- `
        }
    }
}
$runtimeUiPngDocs = @{
    `
- `
}
#requires -Version 7.0
#requires -PSEdition Core

# Chapter 4 for check-json-contracts.ps1 static contracts.

foreach ($check in @(
    @{
        Path = `
- ` -File |
                Sort-Object Name |
                ForEach-Object { $parts.Add((Get-Content -LiteralPath $_.FullName -Raw)) }
        }
    }
    return $parts -join `
- ` = @(`
- `)
    `
- `)
}
foreach ($docPath in $editorInputRebindingActionCaptureDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $editorInputRebindingActionCaptureDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error `
- `)
}
foreach ($docPath in $runtimeInputRebindingCaptureDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $runtimeInputRebindingCaptureDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
        Write-Error `
- `)
}
foreach ($docPath in $runtimeInputRebindingFocusConsumptionDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $runtimeInputRebindingFocusConsumptionDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error `
- `)
}
foreach ($docPath in $runtimeInputRebindingPresentationRowsDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $runtimeInputRebindingPresentationRowsDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error `
- `)
}
foreach ($docPath in $runtimeUiDecodedAtlasDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $runtimeUiDecodedAtlasDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error `
- `)
}
foreach ($docPath in $runtimeUiDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $runtimeUiDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error `
- `)
}
foreach ($docPath in $runtimeUiGlyphAtlasDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $runtimeUiGlyphAtlasDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error `
- `)
}
foreach ($docPath in $runtimeUiPngDocs.Keys) {
    $docText = Get-JsonContractSurfaceText $docPath
    foreach ($needle in $runtimeUiPngDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error `
- `) -Raw
$geUiSourceText = Get-Content -LiteralPath (Join-Path $root `
- `) -Raw
$sourceImageDecodeHeaderText = Get-Content -LiteralPath (Join-Path $root `
- `) -Raw
$sourceImageDecodeSourceText = Get-Content -LiteralPath (Join-Path $root `
- `) -Raw
$uiAtlasToolHeaderText = Get-Content -LiteralPath (Join-Path $root `
- `) {
        $splitRoot = Join-Path $root `
- `,
            `
- `, `
- `--require-compute-morph-async-telemetry`
- `2D Native Sprite Batching Execution v1`
- `2D Sprite Animation Package v1`
- `2D Tilemap Editor Runtime UX v1`
- `Android Release Device Matrix v1`
- `Apple Metal iOS Host Evidence v1`
- `Completed gap burn-down`
- `ComputePipelineDesc`
- `DiagnosticsOpsPlan`
- `Execute this master plan by burning down one`
- `GameEngine.RuntimeInputRebindingProfile.v1`
- `Gap Burn-down Execution Strategy`
- `Generated 3D Compute Morph Async Telemetry Package Smoke D3D12`
- `Generated 3D Compute Morph NORMAL/TANGENT Package Smoke D3D12`
- `Generated 3D Compute Morph Package Smoke Vulkan`
- `Generated 3D Compute Morph Skin Package Smoke D3D12`
- `Generated 3D Compute Morph Skin Package Smoke Vulkan`
- `IFontRasterizerAdapter`
- `IRhiCommandList::dispatch`
- `IRhiDevice::wait_for_queue`
- `MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_VERTEX_SPV`
- `MK_VULKAN_TEST_COMPUTE_MORPH_SPV`
- `MK_VULKAN_TEST_COMPUTE_MORPH_TANGENT_FRAME_SPV`
- `Mirakanai_API36`
- `PngImageDecodingAdapter`
- `Production 1.0 Readiness Audit v1`
- `RHI D3D12 Calibrated Queue Timing Diagnostics`
- `RHI D3D12 Per-Queue Fence Synchronization`
- `RHI D3D12 Queue Clock Calibration Foundation`
- `RHI D3D12 Queue Timestamp Measurement Foundation`
- `RHI D3D12 Submitted Command Calibrated Timing Scopes`
- `RHI Upload Stale Generation Diagnostics v1`
- `RHI Vulkan Compute Dispatch Foundation`
- `Renderer RHI Resource Foundation 1.0 Scope Closeout v1`
- `RhiAsyncOverlapReadinessDiagnostics`
- `Runtime Input Rebinding Capture Contract v1`
- `Runtime Input Rebinding Focus Consumption v1`
- `Runtime Input Rebinding Presentation Rows v1`
- `Runtime RHI Compute Morph Async Overlap Evidence D3D12`
- `Runtime RHI Compute Morph Async Telemetry D3D12`
- `Runtime RHI Compute Morph Calibrated Overlap Diagnostics D3D12`
- `Runtime RHI Compute Morph NORMAL/TANGENT Output D3D12`
- `Runtime RHI Compute Morph NORMAL/TANGENT Output Vulkan`
- `Runtime RHI Compute Morph Pipelined Output Ring D3D12`
- `Runtime RHI Compute Morph Pipelined Scheduling D3D12`
- `Runtime RHI Compute Morph Queue Synchronization D3D12`
- `Runtime RHI Compute Morph Renderer Consumption Vulkan`
- `Runtime RHI Compute Morph Skin Composition D3D12`
- `Runtime RHI Compute Morph Submitted Overlap Diagnostics D3D12`
- `Runtime RHI Compute Morph Vulkan Proof`
- `Runtime Resource v2 1.0 Scope Closeout v1`
- `Runtime Scene RHI Compute Morph Skin Palette D3D12`
- `Runtime UI Font Rasterization Request Plan v1`
- `Runtime UI PNG Image Decoding Adapter v1`
- `Runtime UI and input ledger note:`
- `RuntimeInputRebindingFocusCaptureRequest`
- `RuntimeInputRebindingPresentationModel`
- `RuntimeMorphMeshComputeBinding`
- `UiRendererGlyphAtlasPalette`
- `android-release-device-matrix-v1`
- `animation-float-transform-application-v1`
- `animation-transform-binding-source-v1`
- `apple-host-evidence-check`
- `apple-metal-ios-host-evidence-v1`
- `capture_runtime_input_rebinding_action`
- `capture_runtime_input_rebinding_action_with_focus`
- `cooked-animation-quaternion-clip-v1`
- `crash-telemetry-trace-ops-v1`
- `create_runtime_morph_mesh_compute_binding`
- `desktop-release-package-evidence-v1`
- `gameplay_input_consumed`
- `generated-3d-compute-morph-async-telemetry-package-smoke-d3d12-v1`
- `generated-3d-compute-morph-normal-tangent-package-smoke-d3d12-v1`
- `generated-3d-compute-morph-package-smoke-d3d12-v1`
- `generated-3d-compute-morph-package-smoke-vulkan-v1`
- `generated-3d-compute-morph-queue-sync-package-smoke-d3d12-v1`
- `generated-3d-compute-morph-skin-package-smoke-d3d12-v1`
- `generated-3d-morph-gpu-palette-smoke-v1`
- `generated-3d-morph-normal-tangent-package-smoke-v1`
- `generated-3d-morph-package-consumption-v1`
- `generated-3d-transform-animation-scaffold-v1`
- `gltf-node-transform-animation-binding-source-bridge-v1`
- `gltf-node-transform-animation-float-clip-bridge-v1`
- `gltf-node-transform-animation-import-v1`
- `gpu-morph-d3d12-proof-v1`
- `installed-sdk-release-metadata-validation-v1`
- `make_runtime_compute_morph_output_mesh_gpu_binding`
- `make_runtime_input_rebinding_presentation`
- `not_proven_serial_dependency`
- `package-visible`
- `physics-1-0-collision-system-closeout-v1`
- `platform input glyph generation`
- `production-1-0-readiness-audit-v1`
- `production-readiness-audit-check`
- `registered asset watch-tick orchestration`
- `renderer-rhi-resource-foundation`
- `rhi-compute-dispatch-foundation-v1`
- `rhi-d3d12-calibrated-queue-timing-diagnostics-v1`
- `rhi-d3d12-per-queue-fence-synchronization-v1`
- `rhi-d3d12-queue-clock-calibration-foundation-v1`
- `rhi-d3d12-queue-timestamp-measurement-foundation-v1`
- `rhi-d3d12-submitted-command-calibrated-timing-scopes-v1`
- `rhi-vulkan-compute-dispatch-foundation-v1`
- `runtime-rhi-compute-morph-async-overlap-evidence-d3d12-v1`
- `runtime-rhi-compute-morph-async-telemetry-d3d12-v1`
- `runtime-rhi-compute-morph-calibrated-overlap-diagnostics-d3d12-v1`
- `runtime-rhi-compute-morph-d3d12-proof-v1`
- `runtime-rhi-compute-morph-normal-tangent-output-d3d12-v1`
- `runtime-rhi-compute-morph-normal-tangent-output-vulkan-v1`
- `runtime-rhi-compute-morph-pipelined-output-ring-d3d12-v1`
- `runtime-rhi-compute-morph-pipelined-scheduling-d3d12-v1`
- `runtime-rhi-compute-morph-queue-synchronization-d3d12-v1`
- `runtime-rhi-compute-morph-renderer-consumption-d3d12-v1`
- `runtime-rhi-compute-morph-renderer-consumption-vulkan-v1`
- `runtime-rhi-compute-morph-skin-composition-d3d12-v1`
- `runtime-rhi-compute-morph-submitted-overlap-diagnostics-d3d12-v1`
- `runtime-rhi-compute-morph-vulkan-proof-v1`
- `runtime-scene-animation-transform-binding-v1`
- `runtime-scene-rhi-compute-morph-skin-palette-d3d12-v1`
- `runtime-scene-rhi-morph-gpu-palette-v1`
- `runtime-ui-font-image-adapter-v1`
- `sample_headless`
- `scene_gpu_compute_morph_async_`
- `scene_gpu_compute_morph_queue_waits`
- `stale_generation`
- `symbolic glyph lookup keys`
- `telemetry`
- `upload-staging-v1`
- `

$productionLoop = $manifest.aiOperableProductionLoop
Assert-JsonProperty $productionLoop @(`
- `

Assert-ContainsText (Get-Content -LiteralPath $currentCapabilitiesPath -Raw) `
- `
            $planIdMatch = [regex]::Match($text, '\*\*Plan ID:\*\*\s*`([^`]+)`')
            if ($planIdMatch.Success) {
                $planId = $planIdMatch.Groups[1].Value
            }
            $activePlans += [pscustomobject]@{
                path = $relativePath
                fileName = $_.Name
                planId = $planId
            }
        }
    }

    return @($activePlans)
}

function Assert-ActiveProductionPlanDrift($productionLoop) {
    $masterPlanPath = `
- `
            $referenceFiles = @()
            if (Test-Path -LiteralPath $specificReference) {
                $referenceFiles = @(Get-Item -LiteralPath $specificReference)
            }
        }

        foreach ($referenceFile in $referenceFiles) {
            $parts.Add((Get-Content -LiteralPath $referenceFile.FullName -Raw))
        }
    }

    $text = $parts -join `
- `
        )
    }
)
foreach ($check in $cpp23ReleasePackageArtifactCiEvidenceChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorAiCommandPanelChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorAiEvidenceImportReviewChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorAiReviewedValidationBatchExecutionChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorAiReviewedValidationExecutionChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorContentBrowserImportCodecAdapterCompletedPlanChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorContentBrowserImportExternalCopyReviewChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorContentBrowserImportNativeDialogChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorResourceCaptureExecutionChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorResourceCaptureRequestChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorRuntimeHostPlaytestLaunchChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        }
        if (-not $activeSliceRow.Value.Contains($activePlan.fileName) -and
            ([string]::IsNullOrWhiteSpace($activePlan.planId) -or -not $activeSliceRow.Value.Contains($activePlan.planId))) {
            Write-Error `
- `
        }
        return
    }

    if ($productionLoop.currentActivePlan -ne $masterPlanPath) {
        Write-Error `
- `
    $activePlans = @()

    Get-ChildItem -LiteralPath $plansRoot -Filter `
- `
    $planRegistryPath = `
- `
    $planRegistryText = Get-AgentSurfaceText $planRegistryPath
    $activeSliceRow = [regex]::Match($planRegistryText, '(?m)^\| Active slice \(`currentActivePlan`\) \|.*$')
    if (-not $activeSliceRow.Success) {
        Write-Error `
- `
    $plansRoot = Resolve-RequiredAgentPath `
- `
    $script:agentSurfaceTextCache[$cacheKey] = $text
    return $text
}

function Assert-SkillFrontmatter($skillFile) {
    $head = (Get-Content -LiteralPath $skillFile -TotalCount 8) -join `
- `
    if ($head -notmatch `
- `
    if (Test-Path -LiteralPath $referenceRoot) {
        $leafName = Split-Path -Leaf $path
        if ($leafName -eq `
- `
    }

    $activeChildPlans = @(Get-ActiveChildProductionPlan)
    if ($activeChildPlans.Count -gt 1) {
        Write-Error `
- `
    }

    if ($activeChildPlans.Count -eq 1) {
        $activePlan = $activeChildPlans[0]
        if ($productionLoop.currentActivePlan -ne $activePlan.path) {
            Write-Error `
- `
    }
    if ($productionLoop.recommendedNextPlan.id -ne `
- `
    }
}

$agentsContent = Get-Content -LiteralPath $agents -Raw
if ($agentsContent -notmatch `
- `
    }
}

$editorAiCommandPanelChecks = @(
    @{
        Path = `
- `
    }
}

$editorAiEvidenceImportReviewChecks = @(
    @{
        Path = `
- `
    }
}

$editorAiReviewedValidationBatchExecutionChecks = @(
    @{
        Path = `
- `
    }
}

$editorAiReviewedValidationExecutionChecks = @(
    @{
        Path = `
- `
    }
}

$editorContentBrowserImportCodecAdapterCompletedPlanChecks = @(
    @{
        Path = `
- `
    }
}

$editorContentBrowserImportExternalCopyReviewChecks = @(
    @{
        Path = `
- `
    }
}

$editorContentBrowserImportPanelChecks = @(
    @{
        Path = `
- `
    }
}

$editorMaterialAssetPreviewPanelChecks = @(
    @{
        Path = `
- `
    }
}

function Assert-ClaudeAgentFrontmatter($agentFile) {
    $head = (Get-Content -LiteralPath $agentFile -TotalCount 8) -join `
- `
    }
}

function Assert-CodexReadOnlyAgent($relativePath) {
    $path = Resolve-RequiredAgentPath $relativePath
    $content = Get-Content -LiteralPath $path -Raw
    if ($content -notmatch '(?m)^sandbox_mode\s*=\s*`
- `
    }
}

function Assert-NewGameFailure($arguments, $label) {
    try {
        & (Join-Path $PSScriptRoot `
- `
$geUiSourceText = Get-AgentSurfaceText `
- `
$physicsBenchmarkPlanText = Get-AgentSurfaceText `
- `
$physicsCloseoutPlanText = Get-AgentSurfaceText `
- `
$physicsJointsPlanText = Get-AgentSurfaceText `
- `
$physicsJoltPlanText = Get-AgentSurfaceText `
- `
$removeMergedWorktreeToolPath = Resolve-RequiredAgentPath `
- `
$sourceImageDecodeHeaderText = Get-AgentSurfaceText `
- `
$sourceImageDecodeSourceText = Get-AgentSurfaceText `
- `
$toolsTestsText = Get-AgentSurfaceText `
- `
$uiAtlasToolHeaderText = Get-AgentSurfaceText `
- `
$uiAtlasToolSourceText = Get-AgentSurfaceText `
- `
$uiRendererHeaderText = Get-AgentSurfaceText `
- `
$uiRendererSourceText = Get-AgentSurfaceText `
- `
$uiRendererTestsText = Get-AgentSurfaceText `
- `
)) {
    $directionalShadowGameDevelopmentText = Get-AgentSurfaceText $directionalShadowGameDevelopmentGuidance
    Assert-ContainsText $directionalShadowGameDevelopmentText `
- `
)) {
    $directionalShadowStatusText = Get-AgentSurfaceText $directionalShadowStatusGuidance
    Assert-ContainsText $directionalShadowStatusText `
- `
)) {
    $renderPassPackageText = Get-AgentSurfaceText $renderPassPackageGuidance
    Assert-ContainsText $renderPassPackageText `
- `
)) {
    $runtimeUiDecodedAtlasText = Get-AgentSurfaceText $runtimeUiDecodedAtlasGuidance
    Assert-ContainsText $runtimeUiDecodedAtlasText `
- `
)) {
    $runtimeUiPngText = Get-AgentSurfaceText $runtimeUiPngGuidance
    Assert-ContainsText $runtimeUiPngText `
- `
Assert-ContainsText $agentsContent `
- `
Assert-ContainsText $geUiHeaderText `
- `
Assert-ContainsText $installedDesktopRuntimeValidation `
- `
Assert-ContainsText $masterPlanText `
- `
Assert-ContainsText $physicsBenchmarkPlanText '**Status:** Completed.' `
- `
Assert-ContainsText $physicsBenchmarkPlanText 'Plan ID:** `physics-benchmark-determinism-gates-v1`' `
- `
Assert-ContainsText $physicsJointsPlanText '**Status:** Completed.' `
- `
Assert-ContainsText $physicsJointsPlanText 'Gap:** `physics-1-0-collision-system` Phase P1' `
- `
Assert-ContainsText $planRegistryText `
- `
Assert-ContainsText $productionCompletionMasterPlanContent `
- `
Assert-ContainsText $rendererQualityCMakeText `
- `
Assert-ContainsText (Get-AgentSurfaceText `
- `
Assert-ContainsText (Get-Content -LiteralPath $aiGameDevelopmentPath -Raw) `
- `
Assert-ContainsText (Get-Content -LiteralPath $currentCapabilitiesPath -Raw) `
- `
Assert-ContainsText (Get-Content -LiteralPath $manifestPath -Raw) `
- `
Assert-ContainsText (Get-Content -LiteralPath $planRegistryPath -Raw) `
- `
Assert-ContainsText (Get-Content -LiteralPath $releasePath -Raw) `
- `
Assert-ContainsText (Get-Content -LiteralPath $roadmapPath -Raw) `
- `
Assert-ContainsText (Get-Content -LiteralPath $testingPath -Raw) 'validates the current `CPACK_PACKAGE_FILE_NAME` ZIP' `
- `
Assert-ContainsText ([string]$physicsBackendAdapterDecision[0].decision) `
- `
Assert-ContainsText ([string]$physicsBackendAdapterDecision[0].futureGate) `
- `
Assert-ContainsText ([string]$physicsBackendAdapterDecision[0].status) `
- `
Assert-DoesNotContainText $productionCompletionMasterPlanContent `
- `
foreach ($claim in @(`
- `
foreach ($directionalShadowGameDevelopmentGuidance in @(
    `
- `
foreach ($directionalShadowStatusGuidance in @(
    `
- `
foreach ($masterPlanCadenceNeedle in @(`
- `
foreach ($textFormatToolPath in @(`
- `
if ($agentsContent -notmatch `
- `
if ($productionLoop.schemaVersion -ne 1) {
    Write-Error `
- `
}

function Assert-RegisterRuntimePackageFileFailure($arguments, $label) {
    try {
        & (Join-Path $PSScriptRoot `
- `
}
Assert-ActiveProductionPlanDrift $productionLoop
Assert-NoGameSourceRawAssetIdFromName
$physicsBackendAdapterDecision = @($productionLoop.physicsBackendAdapterDecisions | Where-Object { $_.id -eq `
- `
}
Assert-ContainsText $agentsContent `
- `
}
Assert-ContainsText $masterPlanText `
- `
}
Assert-JsonProperty $physicsBackendAdapterDecision[0] @(`
- `
}
foreach ($importPath in @(`
- `
}
foreach ($needle in @(`
- ` $directionalShadowGameDevelopmentGuidance
    Assert-ContainsText $directionalShadowGameDevelopmentText `
- ` $directionalShadowStatusGuidance
    Assert-ContainsText $directionalShadowStatusText `
- ` $directionalShadowStatusGuidance
}
$installedDesktopRuntimeValidation = Get-AgentSurfaceText `
- ` $renderPassPackageGuidance
    Assert-ContainsText $renderPassPackageText `
- ` $renderPassPackageGuidance
}
$rendererQualityCMakeText = Get-AgentSurfaceText `
- ` $runtimeUiDecodedAtlasGuidance
    Assert-ContainsText $runtimeUiDecodedAtlasText `
- ` $runtimeUiDecodedAtlasGuidance
}
$geUiHeaderText = Get-AgentSurfaceText `
- ` $runtimeUiPngGuidance
    Assert-ContainsText $runtimeUiPngText `
- ` $runtimeUiPngGuidance
}
foreach ($runtimeUiDecodedAtlasGuidance in @(
    `
- ` -File |
                Sort-Object Name |
                ForEach-Object { $parts.Add((Get-Content -LiteralPath $_.FullName -Raw)) }
        }
    }

    $referenceRoot = Join-Path (Split-Path -Parent $path) `
- ` -File | ForEach-Object {
        $relativePath = Get-RelativeRepoPath $_.FullName
        if ($relativePath -eq $masterPlanPath) {
            return
        }

        $text = Get-Content -LiteralPath $_.FullName -Raw
        if ($text.Contains(`
- ` -File | Sort-Object FullName
        }
        else {
            $baseName = [System.IO.Path]::GetFileNameWithoutExtension($path)
            $specificReference = Join-Path $referenceRoot `
- ` -or $agentsContent -notmatch `
- ` -or $head -notmatch `
- ` })
if ($physicsBackendAdapterDecision.Count -ne 1) {
    Write-Error `
- `) `
- `) @arguments | Out-Null
    } catch {
        return
    }
    Write-Error `
- `) {
            $referenceFiles = Get-ChildItem -LiteralPath $referenceRoot -Filter `
- `) {
        Write-Error `
- `) {
    Write-Error `
- `)) {
            $planId = `
- `)) {
        Write-Error `
- `)) {
    Assert-ContainsText $agentsContent $needle `
- `)) {
    Assert-ContainsText $masterPlanText $masterPlanCadenceNeedle `
- `)) {
    Resolve-RequiredAgentPath $textFormatToolPath | Out-Null
}

$claudeContent = Get-Content -LiteralPath $claude -Raw
if ($claudeContent -notmatch `
- `)) {
    if ($claudeContent -notmatch [System.Text.RegularExpressions.Regex]::Escape(`
- `,
    `
- `,
            # manifest.json is emitted with ASCII escapes for '+' (e.g. language C\u002B\u002B23).
            `
- `, 'one fresh `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`', 'tools/build.ps1` only when standalone build/install evidence is requested', `
- `\s*$') {
        Write-Error `
- `read-only``

## Additional retained master-path contract needles

- `

$removeMergedWorktreeToolContent = Get-Content -LiteralPath $removeMergedWorktreeToolPath -Raw
Assert-ContainsText $removeMergedWorktreeToolContent `
- `
        )
    }
)
foreach ($check in $ciMatrixContractCheckCompletedPlanChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorAiPackageDiagnosticsChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorAiPlaytestReadinessReportChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorAiValidationRecipePreflightChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorContentBrowserImportPanelChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorInProcessRuntimeHostReviewChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorInputRebindingProfilePanelChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorMaterialAssetPreviewPanelChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorProfilerTraceExportChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorProjectNativeDialogChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorResourcePanelChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $editorSceneNativeDialogChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $prefabVariantAuthoringChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $prefabVariantConflictReviewChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        )
    }
)
foreach ($check in $visiblePrefabVariantGuiChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error `
- `
        Needles = @(
            'PanelToken{.id = PanelId::ai_commands, .token = `
- `
        Needles = @(
            'PanelToken{.id = PanelId::resources, .token = `
- `
        if ($_.mutates -ne $false) {
            Write-Error `
- `
        }
        $_.id
    })
    if (($actualEditorAiDiagnosticsSteps -join `
- `
        }
        $_.id
    })
    if (($actualEditorAiOperatorHandoffSteps -join `
- `
        }
        if ($_.executes -ne $false) {
            Write-Error `
- `
        }
        if (-not ((@($editorAiPackageDiagnosticsLoop[0].unsupportedClaims) -join `
- `
        }
        if (@($manifest.runtimePackageFiles) -contains $shaderSourcePath) {
            Write-Error `
- `
        }
    }

    $newGameScript = Resolve-RequiredAgentPath `
- `
        }
    }
    foreach ($artifactPath in @($targets[0].d3d12ShaderArtifactPaths)) {
        if (-not ([string]$artifactPath).EndsWith(`
- `
        }
    }
    foreach ($artifactPath in @($targets[0].vulkanShaderArtifactPaths)) {
        if (-not ([string]$artifactPath).EndsWith(`
- `
        }
    }
    foreach ($claim in @(`
- `
        }
    }
    foreach ($expectedManifestInput in @(`
- `
        }
    }
    foreach ($field in @(`
- `
        }
    }
    foreach ($forbiddenField in @(`
- `
        }
    }
    if ($editorPlaytestReviewLoop[0].preSmokeGate -ne `
- `
        }
    }
    if ($targets[0].mode -ne `
- `
        }
    }
    if ($targets[0].validateMaterialTextures -ne $true) {
        Write-Error `
- `
        }
    }
    if (@($manifest.runtimePackageFiles) -contains $sourceMaterialPath) {
        Write-Error `
- `
        }
    }
}

function Assert-AtlasTilemapAuthoringTarget($manifest, [string]$label, [string]$id, [string]$packageIndexPath, [string]$tilemapPath, [string]$atlasTexturePath) {
    if (-not $manifest.PSObject.Properties.Name.Contains(`
- `
        }
    }
}

function Assert-MaterialShaderAuthoringTarget($manifest, [string]$label, [string]$id, [string]$sourceMaterialPath, [string]$runtimeMaterialPath, [string]$packageIndexPath) {
    if (-not $manifest.PSObject.Properties.Name.Contains(`
- `
        }
    }
}

function Assert-NoGameSourceRawAssetIdFromName {
    $rawAssetIdMatches = @()
    $gamesRoot = Resolve-RequiredAgentPath `
- `
        }
    }
}

function Assert-PackageStreamingResidencyTarget($manifest, [string]$label, [string]$id, [string]$packageIndexPath, [string]$runtimeSceneValidationTargetId) {
    if (-not $manifest.PSObject.Properties.Name.Contains(`
- `
        }
    }
}
$editorAiPackageDiagnosticsLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq `
- `
        }
    }
}
$editorAiPlaytestOperatorHandoffLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq `
- `
    )
    $actualEditorAiDiagnosticsSteps = @($editorAiPackageDiagnosticsLoop[0].orderedSteps | ForEach-Object {
        Assert-JsonProperty $_ @(`
- `
    )
    $actualEditorAiOperatorHandoffSteps = @($editorAiPlaytestOperatorHandoffLoop[0].orderedSteps | ForEach-Object {
        Assert-JsonProperty $_ @(`
- `
    )
    $actualEditorReviewSteps = @($editorPlaytestReviewLoop[0].orderedSteps | ForEach-Object { $_.id })
    if (($actualEditorReviewSteps -join `
- `
    Assert-MatchesText $fileText `
- `
    foreach ($match in Select-String -LiteralPath $newGameScript -Pattern `
- `
    foreach ($sourceFile in Get-ChildItem -LiteralPath $gamesRoot -Filter `
- `
    if ($editorAiPackageDiagnosticsLoop[0].status -ne `
- `
    if ($editorAiPlaytestOperatorHandoffLoop[0].status -ne `
- `
    if ($editorPlaytestReviewLoop[0].status -ne `
- `
    }

    if ($rawAssetIdMatches.Count -gt 0) {
        Write-Error `
- `
    }
    $expectedEditorAiDiagnosticsSteps = @(
        `
- `
    }
    $expectedEditorAiOperatorHandoffSteps = @(
        `
- `
    }
    $expectedEditorReviewSteps = @(
        `
- `
    }
    $targets = @($manifest.atlasTilemapAuthoringTargets | Where-Object { $_.id -eq $id })
    if ($targets.Count -ne 1) {
        Write-Error `
- `
    }
    $targets = @($manifest.materialShaderAuthoringTargets | Where-Object { $_.id -eq $id })
    if ($targets.Count -ne 1) {
        Write-Error `
- `
    }
    $targets = @($manifest.packageStreamingResidencyTargets | Where-Object { $_.id -eq $id })
    if ($targets.Count -ne 1) {
        Write-Error `
- `
    }
    $targets = @($manifest.runtimeSceneValidationTargets | Where-Object { $_.id -eq $id })
    if ($targets.Count -ne 1) {
        Write-Error `
- `
    }
    Assert-MatchesText $fileText `
- `
    }
    Resolve-RequiredAgentPath $manifest.aiDrivenGameWorkflow.$field | Out-Null
}

Resolve-RequiredAgentPath `
- `
    }
    foreach ($field in @(`
- `
    }
    foreach ($forbiddenField in @(`
- `
    }
    foreach ($recipe in @(`
- `
    }
    foreach ($recipeId in @($targets[0].preflightRecipeIds)) {
        if (@($manifest.validationRecipes | ForEach-Object { $_.name }) -notcontains $recipeId) {
            Write-Error `
- `
    }
    foreach ($runtimeFile in @($packageIndexPath, $tilemapPath, $atlasTexturePath)) {
        if (@($manifest.runtimePackageFiles) -notcontains $runtimeFile) {
            Write-Error `
- `
    }
    foreach ($runtimeFile in @($runtimeMaterialPath, $packageIndexPath)) {
        if (@($manifest.runtimePackageFiles) -notcontains $runtimeFile) {
            Write-Error `
- `
    }
    foreach ($shaderSourcePath in @(`
- `
    }
    if ($targets[0].PSObject.Properties.Name.Contains(`
- `
    }
    if ($targets[0].atlasPacking -ne `
- `
    }
    if ($targets[0].atlasTexturePath -ne $atlasTexturePath) {
        Write-Error `
- `
    }
    if ($targets[0].mode -ne `
- `
    }
    if ($targets[0].nativeGpuSpriteBatching -ne `
- `
    }
    if ($targets[0].packageIndexPath -ne $packageIndexPath) {
        Write-Error `
- `
    }
    if ($targets[0].requireUniqueNodeNames -ne $true) {
        Write-Error `
- `
    }
    if ($targets[0].runtimeMaterialPath -ne $runtimeMaterialPath) {
        Write-Error `
- `
    }
    if ($targets[0].runtimeSceneValidationTargetId -ne $runtimeSceneValidationTargetId) {
        Write-Error `
- `
    }
    if ($targets[0].safePointRequired -ne $true) {
        Write-Error `
- `
    }
    if ($targets[0].sceneAssetKey -ne $sceneAssetKey) {
        Write-Error `
- `
    }
    if ($targets[0].sourceDecoding -ne `
- `
    }
    if ($targets[0].sourceMaterialPath -ne $sourceMaterialPath) {
        Write-Error `
- `
    }
    if ($targets[0].tilemapPath -ne $tilemapPath) {
        Write-Error `
- `
    }
    if ($targets[0].validateAssetReferences -ne $true) {
        Write-Error `
- `
    }
    if ($targets[0].validateShaderArtifacts -ne $true) {
        Write-Error `
- `
    }
    if (-not ($targets[0].residentBudgetBytes -is [int] -or $targets[0].residentBudgetBytes -is [long]) -or
        [int64]$targets[0].residentBudgetBytes -lt 1) {
        Write-Error `
- `
    }
}

$ciMatrixContractCheckCompletedPlanChecks = @(
    @{
        Path = `
- `
    }
}

$cpp23ReleasePackageArtifactCiEvidenceChecks = @(
    @{
        Path = `
- `
    }
}

$editorAiPackageDiagnosticsChecks = @(
    @{
        Path = `
- `
    }
}

$editorAiPlaytestOperatorHandoffChecks = @(
    @{
        Path = `
- `
    }
}

$editorAiPlaytestReadinessReportChecks = @(
    @{
        Path = `
- `
    }
}

$editorAiValidationRecipePreflightChecks = @(
    @{
        Path = `
- `
    }
}

$editorContentBrowserImportNativeDialogChecks = @(
    @{
        Path = `
- `
    }
}

$editorGameModuleDriverLoadChecks = @(
    @{
        Path = `
- `
    }
}

$editorInProcessRuntimeHostReviewChecks = @(
    @{
        Path = `
- `
    }
}

$editorResourceCaptureRequestChecks = @(
    @{
        Path = `
- `
    }
}

$editorResourcePanelChecks = @(
    @{
        Path = `
- `
    }
}

$prefabVariantConflictReviewChecks = @(
    @{
        Path = `
- `
    }
}

$visiblePrefabVariantGuiChecks = @(
    @{
        Path = `
- `
    }
}

Get-ChildItem -Path (Join-Path $root `
- `
    }
}

function Assert-ContainsText($text, $needle, $label) {
    if (-not $text.Contains($needle)) {
        Write-Error `
- `
    }
}

function Assert-DoesNotContainText($text, $needle, $label) {
    if ($text.Contains($needle)) {
        Write-Error `
- `
    }
}

function Assert-JsonProperty($object, [string[]]$properties, $label) {
    foreach ($property in $properties) {
        if (-not $object.PSObject.Properties.Name.Contains($property)) {
            Write-Error `
- `
    }
}

function Assert-MatchesText($text, $pattern, $label) {
    if (-not [System.Text.RegularExpressions.Regex]::IsMatch($text, $pattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)) {
        Write-Error `
- `
    }
}

function Get-ActiveChildProductionPlan {
    $masterPlanPath = `
- `
    }
}
if (-not $productionLoop.PSObject.Properties.Name.Contains(`
- `
$aiGameDevelopmentText = Get-AgentSurfaceText `
- `
$androidReleaseDeviceMatrixPlanText = Get-AgentSurfaceText `
- `
$appleHostEvidenceScriptText = Get-AgentSurfaceText `
- `
$appleHostHelpersText = Get-AgentSurfaceText `
- `
$appleMetalIosHostEvidencePlanText = Get-AgentSurfaceText `
- `
$computeMorphAsyncTelemetryPlanText = Get-AgentSurfaceText `
- `
$computeMorphSkinPlanText = Get-AgentSurfaceText `
- `
$coreDiagnosticsHeaderText = Get-AgentSurfaceText `
- `
$coreTestsText = Get-AgentSurfaceText `
- `
$crashTelemetryTraceOpsPlanText = Get-AgentSurfaceText `
- `
$currentCapabilitiesText = Get-AgentSurfaceText `
- `
$d3d12RhiHeaderText = Get-AgentSurfaceText `
- `
$d3d12RhiSourceText = Get-AgentSurfaceText `
- `
$editorSceneAuthoringNeedles = @(
    '#include `
- `
$generatedComputeMorphAsyncTelemetryPackagePlanText = Get-AgentSurfaceText `
- `
$generatedComputeMorphPackageVulkanPlanText = Get-AgentSurfaceText `
- `
$generatedComputeMorphSkinPackagePlanText = Get-AgentSurfaceText `
- `
$generatedComputeMorphSkinPackageVulkanPlanText = Get-AgentSurfaceText `
- `
$inputRebindingProfileUxPlanText = Get-AgentSurfaceText `
- `
$mobilePackagingScriptText = Get-AgentSurfaceText `
- `
$nativeSpriteBatchingExecutionPlanText = Get-AgentSurfaceText `
- `
$nullRhiSourceText = Get-AgentSurfaceText `
- `
$productionReadinessAuditPlanText = Get-AgentSurfaceText `
- `
$productionReadinessAuditScriptText = Get-AgentSurfaceText `
- `
$queueSyncPackagePlanText = Get-AgentSurfaceText `
- `
$queueSyncPlanText = Get-AgentSurfaceText `
- `
$rhiAsyncOverlapSourceText = Get-AgentSurfaceText `
- `
$rhiD3d12CalibratedQueueTimingPlanText = Get-AgentSurfaceText `
- `
$rhiD3d12PerQueueFencePlanText = Get-AgentSurfaceText `
- `
$rhiD3d12QueueClockCalibrationPlanText = Get-AgentSurfaceText `
- `
$rhiD3d12QueueTimestampPlanText = Get-AgentSurfaceText `
- `
$rhiD3d12SubmittedCommandTimingPlanText = Get-AgentSurfaceText `
- `
$rhiPublicHeaderText = Get-AgentSurfaceText `
- `
$rhiVulkanComputeDispatchPlanText = Get-AgentSurfaceText `
- `
$roadmapText = Get-AgentSurfaceText `
- `
$runtimeComputeMorphAsyncOverlapPlanText = Get-AgentSurfaceText `
- `
$runtimeComputeMorphCalibratedOverlapPlanText = Get-AgentSurfaceText `
- `
$runtimeComputeMorphNormalTangentOutputVulkanPlanText = Get-AgentSurfaceText `
- `
$runtimeComputeMorphPipelinedOutputRingPlanText = Get-AgentSurfaceText `
- `
$runtimeComputeMorphPipelinedSchedulingPlanText = Get-AgentSurfaceText `
- `
$runtimeComputeMorphRendererConsumptionVulkanPlanText = Get-AgentSurfaceText `
- `
$runtimeComputeMorphSubmittedOverlapPlanText = Get-AgentSurfaceText `
- `
$runtimeComputeMorphVulkanProofPlanText = Get-AgentSurfaceText `
- `
$runtimeInputRebindingCapturePlanText = Get-AgentSurfaceText `
- `
$runtimeInputRebindingFocusConsumptionPlanText = Get-AgentSurfaceText `
- `
$runtimeInputRebindingPresentationRowsPlanText = Get-AgentSurfaceText `
- `
$runtimeRhiHeaderText = Get-AgentSurfaceText `
- `
$runtimeSceneComputeMorphSkinPlanText = Get-AgentSurfaceText `
- `
$runtimeUiFontImageAdapterPlanText = Get-AgentSurfaceText `
- `
$runtimeUiFontRasterizationRequestPlanText = Get-AgentSurfaceText `
- `
$runtimeUiImageDecodeRequestPlanText = Get-AgentSurfaceText `
- `
$runtimeUiPlatformTextInputSessionPlanText = Get-AgentSurfaceText `
- `
$runtimeUiPngImageDecodingAdapterPlanText = Get-AgentSurfaceText `
- `
$runtimeUiTextShapingRequestPlanText = Get-AgentSurfaceText `
- `
$sessionServicesSourceText = Get-AgentSurfaceText `
- `
$spriteAnimationPackagePlanText = Get-AgentSurfaceText `
- `
$tilemapEditorRuntimeUxPlanText = Get-AgentSurfaceText `
- `
$tilemapMetadataSourceText = Get-AgentSurfaceText `
- `
$uiAtlasMetadataSourceText = Get-AgentSurfaceText `
- `
$vulkanRhiHeaderText = Get-AgentSurfaceText `
- `
$vulkanRhiSourceText = Get-AgentSurfaceText `
- `
)
$missingEditorSceneAuthoringNeedles = @()
foreach ($needle in $editorSceneAuthoringNeedles) {
    if (-not $editorShell.Contains($needle)) {
        $missingEditorSceneAuthoringNeedles += $needle
    }
}
if ($missingEditorSceneAuthoringNeedles.Count -gt 0) {
    Write-Error `
- `
)) {
    $stableLightSpaceApiText = Get-AgentSurfaceText $stableLightSpaceApiGuidance
    Assert-ContainsText $stableLightSpaceApiText `
- `
)) {
    $stableLightSpaceText = Get-AgentSurfaceText $stableLightSpaceGuidance
    Assert-ContainsText $stableLightSpaceText `
- `
)) {
    if (-not $manifest.aiDrivenGameWorkflow.PSObject.Properties.Name.Contains($field)) {
        Write-Error `
- `
Assert-ContainsText $agentsContent 'Runtime/C++/build/toolchain/public-contract commits need one fresh `tools/validate.ps1`' `
- `
Assert-ContainsText $agentsContent 'VCPKG_INSTALLED_DIR=${sourceDir}/vcpkg_installed' `
- `
Assert-ContainsText $agentsContent 'run `tools/build.ps1` only when standalone build evidence is requested' `
- `
Assert-ContainsText $geUiSourceText `
- `
Assert-ContainsText $physicsBenchmarkPlanText 'Gap:** `physics-1-0-collision-system` Phase P2' `
- `
Assert-ContainsText $physicsCloseoutPlanText '**Status:** Completed.' `
- `
Assert-ContainsText $physicsCloseoutPlanText 'Gap:** `physics-1-0-collision-system` Phase P4' `
- `
Assert-ContainsText $physicsCloseoutPlanText 'Plan ID:** `physics-1-0-collision-system-closeout-v1`' `
- `
Assert-ContainsText $physicsJoltPlanText `
- `
Assert-ContainsText $physicsJoltPlanText '**Status:** Completed.' `
- `
Assert-ContainsText $physicsJoltPlanText 'Gap:** `physics-1-0-collision-system` Phase P3' `
- `
Assert-ContainsText $physicsJoltPlanText 'Plan ID:** `physics-jolt-adapter-gate-v1`' `
- `
Assert-ContainsText $removeMergedWorktreeToolContent `
- `
Assert-ContainsText $removeMergedWorktreeToolContent '`
- `
Assert-ContainsText $removeMergedWorktreeToolContent '$mainWorktreeRecord' `
- `
Assert-ContainsText $removeMergedWorktreeToolContent '$standardWorktreeBasePaths' `
- `
Assert-ContainsText $sourceImageDecodeHeaderText `
- `
Assert-ContainsText $sourceImageDecodeSourceText `
- `
Assert-ContainsText $toolsTestsText `
- `
Assert-ContainsText $uiAtlasToolHeaderText `
- `
Assert-ContainsText $uiAtlasToolSourceText `
- `
Assert-ContainsText $workflowsContent `
- `
foreach ($gitCadenceNeedle in @(`
- `
foreach ($planVolumeNeedle in @(`
- `
foreach ($windowsDiagnosticsNeedle in @(`
- `
}

$editorSceneNativeDialogChecks = @(
    @{
        Path = `
- `
}

$workflowsContent = Get-Content -LiteralPath $workflowsPath -Raw
Assert-ContainsText $workflowsContent `
- `
}

function Assert-RuntimeSceneValidationTarget($manifest, [string]$label, [string]$id, [string]$packageIndexPath, [string]$sceneAssetKey) {
    if (-not $manifest.PSObject.Properties.Name.Contains(`
- `
}
$editorPlaytestReviewLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq `
- `
}
Assert-ContainsText $workflowsContent `
- `
}
foreach ($field in @(
    `
- `
}
foreach ($productionPromptNeedle in @(`
- `
}
if ($editorAiPackageDiagnosticsLoop.Count -eq 1) {
    Assert-JsonProperty $editorAiPackageDiagnosticsLoop[0] @(`
- `
}
if ($editorAiPlaytestOperatorHandoffLoop.Count -eq 1) {
    Assert-JsonProperty $editorAiPlaytestOperatorHandoffLoop[0] @(`
- `
}
if ($editorPlaytestReviewLoop.Count -eq 1) {
    Assert-JsonProperty $editorPlaytestReviewLoop[0] @(`
- ` $directionalShadowGameDevelopmentGuidance
}
foreach ($stableLightSpaceGuidance in @(
    `
- ` $stableLightSpaceApiGuidance
}

if (-not $manifest.PSObject.Properties.Name.Contains(`
- ` $stableLightSpaceGuidance
}
foreach ($stableLightSpaceApiGuidance in @(
    `
- ` -Recurse -File) {
        foreach ($match in Select-String -LiteralPath $sourceFile.FullName -Pattern `
- ` -SimpleMatch) {
            $rawAssetIdMatches += `
- ` -SimpleMatch) {
        $rawAssetIdMatches += `
- ` | Out-Null

$editorShell = Get-AgentSurfaceText `
- ` | Out-Null
Resolve-RequiredAgentPath `
- ` })
if ($editorAiPackageDiagnosticsLoop.Count -ne 1) {
    Write-Error `
- ` })
if ($editorAiPlaytestOperatorHandoffLoop.Count -ne 1) {
    Write-Error `
- ` })
if ($editorPlaytestReviewLoop.Count -ne 1) {
    Write-Error `
- `'
        )
    },
    @{
        Path = `
- `',
    `
- `',
            `
- `) -Recurse -Filter `
- `) -ne ($expectedEditorAiDiagnosticsSteps -join `
- `) -ne ($expectedEditorAiOperatorHandoffSteps -join `
- `) -ne ($expectedEditorReviewSteps -join `
- `)) {
        if ($targets[0].PSObject.Properties.Name.Contains($forbiddenField)) {
            Write-Error `
- `)) {
        if (-not ((@($editorAiPackageDiagnosticsLoop[0].blockedExecution) -join `
- `)) {
        if (-not ((@($editorAiPlaytestOperatorHandoffLoop[0].commandFields) -join `
- `)) {
        if (-not ((@($editorAiPlaytestOperatorHandoffLoop[0].handoffInputs) -join `
- `)) {
        if (@($editorAiPackageDiagnosticsLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error `
- `)) {
        if (@($editorAiPlaytestOperatorHandoffLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error `
- `)) {
        if (@($editorPlaytestReviewLoop[0].hostGatedSmokeRecipes) -notcontains $recipe) {
            Write-Error `
- `)) {
        if (@($editorPlaytestReviewLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error `
- `)) {
        if (@($targets[0].shaderSourcePaths) -notcontains $shaderSourcePath) {
            Write-Error `
- `)) {
    Assert-ContainsText $agentsContent $gitCadenceNeedle `
- `)) {
    Assert-ContainsText $agentsContent $planVolumeNeedle `
- `)) {
    Assert-ContainsText $agentsContent $productionPromptNeedle `
- `)) {
    Assert-ContainsText $agentsContent $windowsDiagnosticsNeedle `
- `)) {
    Assert-ContainsText $workflowsContent $planVolumeNeedle `
- `)) {
    Assert-ContainsText $workflowsContent $productionPromptNeedle `
- `)) {
    Write-Error `
- `)) {
    if (@($physicsBackendAdapterDecision[0].unsupportedClaims) -notcontains $claim) {
        Write-Error `
- `).Contains($claim))) {
            Write-Error `
- `).Contains($expectedManifestInput))) {
            Write-Error `
- `).Contains($field))) {
            Write-Error `
- `,
            '.name = `
- `,
            'ImGui::Checkbox(`
- `,
            'already-linked `IEditorPlaySessionDriver`',
            `
- `,
            'append_rows(document, root, `
- `,
            'strict `MK_scene` prefab variant validation and composition index-based',
            `
- `,
            'workspace `ai_commands` panel state',
            `
- `,
            'workspace `ai_commands` panel',
            `
- `, .pattern = `
- `}',
            `

## Structured retained master-path needles

- `$(Get-RelativeRepoPath $match.Path):$($match.LineNumber)`
- `$baseName.md`
- `$label atlasTilemapAuthoringTargets '$id' atlasPacking must remain unsupported`
- `$label atlasTilemapAuthoringTargets '$id' atlasTexturePath must be $atlasTexturePath`
- `$label atlasTilemapAuthoringTargets '$id' mode must be deterministic-package-data`
- `$label atlasTilemapAuthoringTargets '$id' must not expose unsupported field: $forbiddenField`
- `$label atlasTilemapAuthoringTargets '$id' nativeGpuSpriteBatching must remain unsupported`
- `$label atlasTilemapAuthoringTargets '$id' packageIndexPath must be $packageIndexPath`
- `$label atlasTilemapAuthoringTargets '$id' preflightRecipeIds must reference validationRecipes: $recipeId`
- `$label atlasTilemapAuthoringTargets '$id' runtime file must be in runtimePackageFiles: $runtimeFile`
- `$label atlasTilemapAuthoringTargets '$id' sourceDecoding must remain unsupported`
- `$label atlasTilemapAuthoringTargets '$id' tilemapPath must be $tilemapPath`
- `$label atlasTilemapAuthoringTargets must contain exactly one '$id' row`
- `$label contained forbidden text: $needle`
- `$label did not contain expected text: $needle`
- `$label did not match expected pattern: $pattern`
- `$label materialShaderAuthoringTargets '$id' D3D12 artifact must end in .dxil: $artifactPath`
- `$label materialShaderAuthoringTargets '$id' Vulkan artifact must end in .spv: $artifactPath`
- `$label materialShaderAuthoringTargets '$id' must not expose unsupported field: $forbiddenField`
- `$label materialShaderAuthoringTargets '$id' must validate material textures`
- `$label materialShaderAuthoringTargets '$id' must validate shader artifacts`
- `$label materialShaderAuthoringTargets '$id' packageIndexPath must be $packageIndexPath`
- `$label materialShaderAuthoringTargets '$id' runtime file must be in runtimePackageFiles: $runtimeFile`
- `$label materialShaderAuthoringTargets '$id' runtimeMaterialPath must be $runtimeMaterialPath`
- `$label materialShaderAuthoringTargets '$id' shader source must not be in runtimePackageFiles: $shaderSourcePath`
- `$label materialShaderAuthoringTargets '$id' shaderSourcePaths missing $shaderSourcePath`
- `$label materialShaderAuthoringTargets '$id' sourceMaterialPath must be $sourceMaterialPath`
- `$label materialShaderAuthoringTargets '$id' sourceMaterialPath must not be in runtimePackageFiles`
- `$label materialShaderAuthoringTargets must contain exactly one '$id' row`
- `$label missing required property: $property`
- `$label must declare atlasTilemapAuthoringTargets`
- `$label must declare materialShaderAuthoringTargets`
- `$label must declare packageStreamingResidencyTargets`
- `$label must declare runtimeSceneValidationTargets`
- `$label packageStreamingResidencyTargets '$id' mode must be host-gated-safe-point`
- `$label packageStreamingResidencyTargets '$id' must omit contentRoot when package index entries already include runtime-relative paths`
- `$label packageStreamingResidencyTargets '$id' must reference runtimeSceneValidationTargets`
- `$label packageStreamingResidencyTargets '$id' packageIndexPath must be $packageIndexPath`
- `$label packageStreamingResidencyTargets '$id' packageIndexPath must be in runtimePackageFiles`
- `$label packageStreamingResidencyTargets '$id' preflightRecipeIds must reference validationRecipes: $recipeId`
- `$label packageStreamingResidencyTargets '$id' residentBudgetBytes must be positive`
- `$label packageStreamingResidencyTargets '$id' runtimeSceneValidationTargetId must be $runtimeSceneValidationTargetId`
- `$label packageStreamingResidencyTargets '$id' safePointRequired must be true`
- `$label packageStreamingResidencyTargets must contain exactly one '$id' row`
- `$label runtimeSceneValidationTargets '$id' must not expose unsupported field: $forbiddenField`
- `$label runtimeSceneValidationTargets '$id' must omit contentRoot when package index entries already include runtime-relative paths`
- `$label runtimeSceneValidationTargets '$id' must require unique node names`
- `$label runtimeSceneValidationTargets '$id' must validate asset references`
- `$label runtimeSceneValidationTargets '$id' packageIndexPath must be $packageIndexPath`
- `$label runtimeSceneValidationTargets '$id' sceneAssetKey must be $sceneAssetKey`
- `$label runtimeSceneValidationTargets must contain exactly one '$id' row`
- `$label should have failed.`
- `$newGameToolText`n$newGameTemplatesText`
- `$planRegistryPath Active slice row must mention active child plan id or path: $($activePlan.path)`
- `$planRegistryPath must contain an Active slice currentActivePlan row`
- `**Status:** Active.`
- `*.cpp`
- `*.md`
- `---\s*\nname:\s*[a-z0-9-]+`
- `--match-head-commit <headRefOid>`
- `--porcelain`
- `--require-directional-shadow`
- `--require-directional-shadow-filtering`
- `--require-renderer-quality-gates`
- `--warnings-as-errors=*`
- `.agents/skills/gameengine-game-development/SKILL.md`
- `.agents/skills/rendering-change/SKILL.md`
- `.claude/rules/ai-agent-integration.md`
- `.claude/rules/cpp-engine.md`
- `.claude/settings.json`
- `.claude/settings.local.json`
- `.claude/skills/gameengine-game-development/SKILL.md`
- `.claude/skills/gameengine-rendering/SKILL.md`
- `.claude/worktrees/`
- `.codex/rules`
- `.dxil`
- `.mcp.json`
- `.spv`
- `2026-*.md`
- `2026-05-02-2d-packaged-playable-generation-loop-v1.md`
- `2026-05-02-editor-playtest-package-review-loop-v1.md`
- `2026-05-02-renderer-resource-residency-upload-execution-v1.md`
- `2026-05-05-animation-float-transform-application-v1.md`
- `2026-05-05-animation-transform-binding-source-v1.md`
- `2026-05-05-cooked-animation-quaternion-clip-v1.md`
- `2026-05-05-generated-3d-morph-gpu-palette-smoke-v1.md`
- `2026-05-05-generated-3d-morph-normal-tangent-package-smoke-v1.md`
- `2026-05-05-generated-3d-morph-package-consumption-v1.md`
- `2026-05-05-generated-3d-transform-animation-scaffold-v1.md`
- `2026-05-05-gltf-node-transform-animation-binding-source-bridge-v1.md`
- `2026-05-05-gltf-node-transform-animation-float-clip-bridge-v1.md`
- `2026-05-05-gltf-node-transform-animation-import-v1.md`
- `2026-05-05-gpu-morph-d3d12-proof-v1.md`
- `2026-05-05-gpu-morph-normal-tangent-d3d12-proof-v1.md`
- `2026-05-05-rhi-compute-dispatch-foundation-v1.md`
- `2026-05-05-runtime-rhi-compute-morph-d3d12-proof-v1.md`
- `2026-05-05-runtime-scene-animation-transform-binding-v1.md`
- `2026-05-05-runtime-scene-rhi-morph-gpu-palette-v1.md`
- `2026-05-06-2d-native-sprite-batching-execution-v1.md`
- `2026-05-06-2d-sprite-animation-package-v1.md`
- `2026-05-06-2d-tilemap-editor-runtime-ux-v1.md`
- `2026-05-06-android-release-device-matrix-v1.md`
- `2026-05-06-apple-metal-ios-host-evidence-v1.md`
- `2026-05-06-crash-telemetry-trace-ops-v1.md`
- `2026-05-06-desktop-release-package-evidence-v1.md`
- `2026-05-06-generated-3d-compute-morph-async-telemetry-package-smoke-d3d12-v1.md`
- `2026-05-06-generated-3d-compute-morph-normal-tangent-package-smoke-d3d12-v1.md`
- `2026-05-06-generated-3d-compute-morph-package-smoke-d3d12-v1.md`
- `2026-05-06-generated-3d-compute-morph-package-smoke-vulkan-v1.md`
- `2026-05-06-generated-3d-compute-morph-queue-sync-package-smoke-d3d12-v1.md`
- `2026-05-06-generated-3d-compute-morph-skin-package-smoke-d3d12-v1.md`
- `2026-05-06-installed-sdk-release-metadata-validation-v1.md`
- `2026-05-06-production-1-0-readiness-audit-v1.md`
- `2026-05-06-rhi-d3d12-calibrated-queue-timing-diagnostics-v1.md`
- `2026-05-06-rhi-d3d12-per-queue-fence-synchronization-v1.md`
- `2026-05-06-rhi-d3d12-queue-clock-calibration-foundation-v1.md`
- `2026-05-06-rhi-d3d12-queue-timestamp-measurement-foundation-v1.md`
- `2026-05-06-rhi-d3d12-submitted-command-calibrated-timing-scopes-v1.md`
- `2026-05-06-rhi-vulkan-compute-dispatch-foundation-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-async-overlap-evidence-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-async-telemetry-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-calibrated-overlap-diagnostics-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-normal-tangent-output-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-normal-tangent-output-vulkan-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-pipelined-output-ring-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-pipelined-scheduling-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-queue-synchronization-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-renderer-consumption-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-renderer-consumption-vulkan-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-skin-composition-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-submitted-overlap-diagnostics-d3d12-v1.md`
- `2026-05-06-runtime-rhi-compute-morph-vulkan-proof-v1.md`
- `2026-05-06-runtime-scene-quaternion-animation-transform-binding-v1.md`
- `2026-05-06-runtime-scene-rhi-compute-morph-skin-palette-d3d12-v1.md`
- `2026-05-06-runtime-ui-font-image-adapter-v1.md`
- `2026-05-07-generated-3d-compute-morph-skin-package-smoke-vulkan-v1.md`
- `2026-05-07-runtime-ui-font-rasterization-request-plan-v1.md`
- `2026-05-08-runtime-input-rebinding-capture-contract-v1.md`
- `2026-05-08-runtime-ui-png-image-decoding-adapter-v1.md`
- `2026-05-09-editor-nested-prefab-refresh-resolution-v1.md`
- `2026-05-09-editor-prefab-instance-local-child-refresh-resolution-v1.md`
- `2026-05-09-editor-prefab-instance-stale-node-refresh-resolution-v1.md`
- `2026-05-09-editor-scene-prefab-instance-refresh-review-v1.md`
- `: $relativePath`
- `@$importPath`
- `@AGENTS\.md`
- `AGENTS.md`
- `AGENTS.md must document the Context7 documentation lookup policy`
- `AGENTS.md must document the docs entrypoint and implementation plan registry`
- `AGENTS.override.md`
- `AccessibilityPublishPlan::ready`
- `AccessibilityPublishResult::succeeded`
- `Active gap burn-down`
- `Agent Surface Governance`
- `Animation Float Transform Application v1`
- `Animation Transform Binding Source v1`
- `AnimationJointTrack3dByteSource`
- `AnimationQuaternionClipSourceDocument`
- `AnimationTransformBindingSourceDocument`
- `Apply Reviewed Prefab Refresh`
- `AssetId::from_name(`
- `CLAUDE.md must import $importPath`
- `CLAUDE.md must import AGENTS.md`
- `CMake File API`
- `CMake File API codemodel`
- `CMake configure must not install, restore, or download vcpkg packages`
- `CapturingAccessibilityAdapter`
- `CapturingFontRasterizerAdapter`
- `CapturingImageDecodingAdapter`
- `CapturingImeAdapter`
- `CapturingTextShapingAdapter`
- `Claude agent frontmatter must include lowercase name and description: $agentFile`
- `Clear batch selection`
- `Codex app Worktree/Handoff`
- `Commit, Push, And Pull Request Workflow`
- `Context7`
- `ConvertTo-ExtendedLengthPath`
- `Cooked Animation Quaternion Clip v1`
- `Crash Telemetry Trace Ops plan`
- `Desktop SDK release artifact validation now checks`
- `DiagnosticsOpsArtifactStatus`
- `Direct default-branch pushes are outside the official GitHub Flow path`
- `DirectionalShadowLightSpacePlan`
- `Do not open a PR for every commit`
- `Docs/agent/rules/subagent-only`
- `Docs/non-runtime slices may record narrower justified checks`
- `Documentation-only or similarly narrow non-runtime slices`
- `Editor Nested Prefab Refresh Resolution v1`
- `Editor Playtest Package Review Loop v1`
- `Editor Prefab Instance Local Child Refresh Resolution v1`
- `Editor Prefab Instance Stale Node Refresh Resolution v1`
- `Editor Scene Prefab Instance Refresh Review v1`
- `EditorAiPlaytestOperatorHandoffModel`
- `EditorAiPlaytestReadinessReportModel`
- `EditorAiValidationRecipePreflightModel`
- `FontRasterizationRequestPlan::ready`
- `FontRasterizationResult::succeeded`
- `GITHUB_TOKEN`
- `GPU Morph D3D12 Proof v1`
- `GPU Morph NORMAL/TANGENT D3D12 Proof v1`
- `Game sources and tools/new-game.ps1 must derive asset references from AssetKeyV2 via asset_id_from_key_v2 instead of AssetId::from_name: $($rawAssetIdMatches -join ', ')`
- `GameEngine.AnimationTransformBindingSource.v1`
- `GameEngine.CookedAnimationQuaternionClip.v1`
- `GameEngine.RuntimeInputActions.v4`
- `Generated 3D Morph NORMAL/TANGENT Package Smoke v1`
- `Get-ClangFormatCommand`
- `GitHub Desktop`
- `GitHub account billing/spending-limit`
- `GitHub flow`
- `GltfNodeTransformAnimationBindingSourceImportReport`
- `GltfNodeTransformAnimationQuaternionClipImportReport`
- `HeaderFilterRegex`
- `Hosted PR Check Failure Triage`
- `Hosted PR Check Selection`
- `ImageDecodeDispatchResult::succeeded`
- `ImageDecodeRequestPlan::ready`
- `ImeCompositionPublishPlan::ready`
- `ImeCompositionPublishResult::succeeded`
- `Installed SDK release validation now checks installed metadata`
- `Instruction Hygiene`
- `InvalidFontRasterizerAdapter`
- `Jolt runtime integration`
- `Keep Local Children`
- `Keep Nested Prefab Instances`
- `Keep Stale Source Nodes`
- `Keep drift checks targeted`
- `MCP connection state`
- `MK_animation skeleton public header`
- `MK_assets asset source public header`
- `MK_renderer public header`
- `MK_renderer rhi frame renderer public header`
- `MK_runtime`
- `MK_runtime module purpose`
- `MK_runtime public header`
- `MK_runtime_host_sdl3 public header`
- `MK_runtime_rhi public header`
- `MK_runtime_scene public header`
- `MK_runtime_scene_rhi public header`
- `MK_scene_renderer public header`
- `MK_tools gltf node animation import public header`
- `MK_tools source image decode public header`
- `MK_tools source image decode source`
- `MK_tools tests`
- `MK_tools ui atlas tool public header`
- `MK_tools ui atlas tool source`
- `MK_ui public header`
- `MK_ui source`
- `MK_ui_renderer public header`
- `MK_ui_renderer source`
- `MK_ui_renderer tests`
- `MeshCommand::gpu_morphing`
- `MorphMeshGpuBinding`
- `NN warnings generated.`
- `Only one active child production plan is allowed: $(@($activeChildPlans | ForEach-Object { $_.path }) -join ', ')`
- `OpenAI developer documentation MCP`
- `PIX on Windows`
- `POSITION/NORMAL/TANGENT morph delta buffers`
- `PR CI selection`
- `PackedUiAtlasPackageUpdateDesc`
- `PackedUiGlyphAtlasPackageUpdateDesc`
- `Path-filtered workflows must not be branch-protection-required`
- `Plan width may be broader than PR/phase width`
- `Plan-file width and PR/phase width are different`
- `PlatformTextInputEndPlan::ready`
- `PlatformTextInputEndResult::succeeded`
- `PlatformTextInputSessionPlan::ready`
- `PlatformTextInputSessionResult::succeeded`
- `PngImageDecodingAdapter::decode_image`
- `Production Completion Execution`
- `Production Completion Prompt`
- `Read-only Codex agent must declare sandbox_mode = ``
- `Refresh Prefab Instance`
- `Remove-WorktreeLocalReparsePointBeforeGitRemoval`
- `Renderer Resource Residency Upload Execution v1`
- `ReparsePoint`
- `Review batch prefab refresh`
- `Runtime Scene Animation Transform Binding v1`
- `Runtime Scene Quaternion Animation Transform Binding v1`
- `RuntimeAnimationQuaternionClipPayload`
- `RuntimeInputContextStack`
- `RuntimeInputRebindingAxisCaptureRequest`
- `RuntimeInputStateView`
- `RuntimeSceneAnimationTransformBindingResolution`
- `RuntimeSceneGpuBindingOptions::morph_mesh_assets`
- `SKILL.md`
- `SceneMorphGpuBindingPalette`
- `Skill frontmatter must include lowercase name and description: $skillFile`
- `Stable Directional Light-Space Policy v0`
- `TextAdapterGlyphPlaceholder`
- `TextShapingRequestPlan::ready`
- `TextShapingResult::succeeded`
- `Treat publishing as a slice-closing gate`
- `VCPKG_MANIFEST_INSTALL=OFF`
- `Validate installed SDK metadata payloads`
- `Vulkan/Metal material-preview display parity`
- `Windows Graphics Tools`
- `Windows Performance Toolkit`
- `Windows fallback guarded/non-following`
- `Worktree And Parallel Agent Workflow`
- `_NT_SYMBOL_PATH`
- ``n`n`
- `accepts standard roots`
- `active gap-cluster burn-down or milestone`
- `adapter.begin_text_input`
- `adapter.decode_image`
- `adapter.end_text_input`
- `adapter.publish_nodes`
- `adapter.rasterize_glyph`
- `adapter.shape_text`
- `adapter.update_composition`
- `agent-surface drift check`
- `ai-integration-check: editor shell missing SceneAuthoringDocument wiring: $($missingEditorSceneAuthoringNeedles -join ', ')`
- `aiNavigationSample`
- `always-running required gate`
- `apply-reviewed-runtime-package-files`
- `apply_float_animation_samples_to_transform3d`
- `apply_runtime_input_rebinding_profile`
- `apply_runtime_scene_animation_pose_3d`
- `apply_runtime_scene_animation_transform_samples`
- `approval-capable session`
- `argv plan data`
- `assemble-reviewed-operator-command-rows`
- `authoringSurfaces`
- `axis_value`
- `before final report`
- `bind_gamepad_axis`
- `bind_gamepad_button`
- `bind_gamepad_button_in_context`
- `bind_key_axis_in_context`
- `bind_key_in_context`
- `bind_pointer_in_context`
- `blocked reasons`
- `blockers`
- `branch creation, task-owned staging, commit, non-forced push, and PR creation/update`
- `broad package cooking`
- `budget gates`
- `build_prefab_from_selected_node`
- `capability/gap-cluster/milestone`
- `capture_runtime_input_rebinding_axis`
- `catch (...)`
- `cdb -version`
- `check-release-package-artifacts.ps1`
- `checkpoint-based, not commit-count-based`
- `classify-remediation-rows`
- `clean breaking greenfield designs`
- `clean local worktree`
- `codex-<topic>`
- `collect-editor-package-candidate-diagnostics`
- `collect-evidence-summary`
- `collect-external-validation-evidence`
- `collect-operator-handoff`
- `collect-readiness-report`
- `collect-validation-preflight-commands`
- `commandSurfaces`
- `commit complete validated phases`
- `commit validated phases`
- `commit was pushed to the same head branch after the PR had already merged`
- `commits pushed after a PR merged need a new PR`
- `completed gap, remaining gaps, next active plan`
- `contentRoot`
- `count-based`
- `credential-manager-core`
- `current active plan checklist`
- `currentInput`
- `current_cooked_scene_path`
- `current_package_index_path`
- `current_prefab_path`
- `dated capability/gap-cluster/milestone plan`
- `decoded image must be RGBA8`
- `description:\s*.+`
- `design`
- `desktop SDK release artifact gate`
- `desktop-game-runtime`
- `desktop-runtime-release-target`
- `desktop-runtime-release-target-vulkan-directional-shadow-toolchain-gated`
- `desktop-runtime-release-target-vulkan-toolchain-gated`
- `desktopRuntime3dMorphGpuPaletteSmoke`
- `desktopRuntime3dMorphNormalTangentPackageSmoke`
- `desktopRuntime3dMorphPackageConsumptionScaffold`
- `desktopRuntime3dTransformAnimationScaffold`
- `desktopRuntimeGameSample`
- `desktopRuntimeShellSample`
- `deterministic-package-data`
- `diagnosticInputs`
- `diagnostics ops plan reports trace summary and unsupported upload boundaries`
- `did not emit the required`
- `direct default-branch pushes must stay forbidden`
- `direct-clang-format-status`
- `directional_shadow_filter_mode`
- `directional_shadow_filter_radius_texels`
- `directional_shadow_filter_taps`
- `directional_shadow_status`
- `do not broad-load every agent surface`
- `do not report a task complete while task-owned changes only exist locally after validation`
- `do not stash, merge into an active feature branch`
- `docs/README.md`
- `docs/README\.md`
- `docs/agent/rules/subagent-only changes`
- `docs/ai-game-development.md`
- `docs/architecture.md`
- `docs/current-capabilities.md`
- `docs/dependencies.md`
- `docs/release.md`
- `docs/rhi.md`
- `docs/roadmap.md`
- `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- `docs/specs/README.md`
- `docs/specs/game-prompt-pack.md`
- `docs/specs/game-template.md`
- `docs/specs/generated-game-validation-scenarios.md`
- `docs/superpowers/master-plans/production-completion-v1`
- `docs/superpowers/plans`
- `docs/superpowers/plans/2026-05-06-2d-native-sprite-batching-execution-v1.md`
- `docs/superpowers/plans/2026-05-06-2d-sprite-animation-package-v1.md`
- `docs/superpowers/plans/2026-05-06-2d-tilemap-editor-runtime-ux-v1.md`
- `docs/superpowers/plans/2026-05-06-android-release-device-matrix-v1.md`
- `docs/superpowers/plans/2026-05-06-apple-metal-ios-host-evidence-v1.md`
- `docs/superpowers/plans/2026-05-06-crash-telemetry-trace-ops-v1.md`
- `docs/superpowers/plans/2026-05-06-generated-3d-compute-morph-async-telemetry-package-smoke-d3d12-v1.md`
- `docs/superpowers/plans/2026-05-06-generated-3d-compute-morph-package-smoke-vulkan-v1.md`
- `docs/superpowers/plans/2026-05-06-generated-3d-compute-morph-queue-sync-package-smoke-d3d12-v1.md`
- `docs/superpowers/plans/2026-05-06-generated-3d-compute-morph-skin-package-smoke-d3d12-v1.md`
- `docs/superpowers/plans/2026-05-06-input-rebinding-profile-ux-v1.md`
- `docs/superpowers/plans/2026-05-06-production-1-0-readiness-audit-v1.md`
- `docs/superpowers/plans/2026-05-06-rhi-d3d12-calibrated-queue-timing-diagnostics-v1.md`
- `docs/superpowers/plans/2026-05-06-rhi-d3d12-per-queue-fence-synchronization-v1.md`
- `docs/superpowers/plans/2026-05-06-rhi-d3d12-queue-clock-calibration-foundation-v1.md`
- `docs/superpowers/plans/2026-05-06-rhi-d3d12-queue-timestamp-measurement-foundation-v1.md`
- `docs/superpowers/plans/2026-05-06-rhi-d3d12-submitted-command-calibrated-timing-scopes-v1.md`
- `docs/superpowers/plans/2026-05-06-rhi-vulkan-compute-dispatch-foundation-v1.md`
- `docs/superpowers/plans/2026-05-06-runtime-rhi-compute-morph-async-overlap-evidence-d3d12-v1.md`
- `docs/superpowers/plans/2026-05-06-runtime-rhi-compute-morph-async-telemetry-d3d12-v1.md`
- `docs/superpowers/plans/2026-05-06-runtime-rhi-compute-morph-calibrated-overlap-diagnostics-d3d12-v1.md`
- `docs/superpowers/plans/2026-05-06-runtime-rhi-compute-morph-normal-tangent-output-vulkan-v1.md`
- `docs/superpowers/plans/2026-05-06-runtime-rhi-compute-morph-pipelined-output-ring-d3d12-v1.md`
- `docs/superpowers/plans/2026-05-06-runtime-rhi-compute-morph-pipelined-scheduling-d3d12-v1.md`
- `docs/superpowers/plans/2026-05-06-runtime-rhi-compute-morph-queue-synchronization-d3d12-v1.md`
- `docs/superpowers/plans/2026-05-06-runtime-rhi-compute-morph-renderer-consumption-vulkan-v1.md`
- `docs/superpowers/plans/2026-05-06-runtime-rhi-compute-morph-skin-composition-d3d12-v1.md`
- `docs/superpowers/plans/2026-05-06-runtime-rhi-compute-morph-submitted-overlap-diagnostics-d3d12-v1.md`
- `docs/superpowers/plans/2026-05-06-runtime-rhi-compute-morph-vulkan-proof-v1.md`
- `docs/superpowers/plans/2026-05-06-runtime-scene-rhi-compute-morph-skin-palette-d3d12-v1.md`
- `docs/superpowers/plans/2026-05-06-runtime-ui-font-image-adapter-v1.md`
- `docs/superpowers/plans/2026-05-07-generated-3d-compute-morph-skin-package-smoke-vulkan-v1.md`
- `docs/superpowers/plans/2026-05-07-runtime-ui-font-rasterization-request-plan-v1.md`
- `docs/superpowers/plans/2026-05-08-runtime-input-rebinding-capture-contract-v1.md`
- `docs/superpowers/plans/2026-05-08-runtime-input-rebinding-focus-consumption-v1.md`
- `docs/superpowers/plans/2026-05-08-runtime-input-rebinding-presentation-rows-v1.md`
- `docs/superpowers/plans/2026-05-08-runtime-ui-image-decode-request-plan-v1.md`
- `docs/superpowers/plans/2026-05-08-runtime-ui-platform-text-input-session-plan-v1.md`
- `docs/superpowers/plans/2026-05-08-runtime-ui-png-image-decoding-adapter-v1.md`
- `docs/superpowers/plans/2026-05-08-runtime-ui-text-shaping-request-plan-v1.md`
- `docs/superpowers/plans/2026-05-09-physics-1-0-collision-system-closeout-v1.md`
- `docs/superpowers/plans/2026-05-09-physics-benchmark-determinism-gates-v1.md`
- `docs/superpowers/plans/2026-05-09-physics-joints-foundation-v1.md`
- `docs/superpowers/plans/2026-05-09-physics-jolt-adapter-gate-v1.md`
- `docs/superpowers/plans/README.md`
- `docs/superpowers/plans/README\.md`
- `docs/testing.md`
- `docs/ui.md`
- `docs/workflows.md`
- `editor-ai-package-authoring-diagnostics`
- `editor-ai-playtest-evidence-summary`
- `editor-ai-playtest-operator-handoff`
- `editor-ai-playtest-remediation-queue`
- `editor-ai-reviewed-validation-execution-v1`
- `editor-content-browser-and-import-diagnostics-v1`
- `editor-input-rebinding-action-capture-panel-v1`
- `editor-input-rebinding-profile-panel-v1`
- `editor-material-asset-preview-diagnostics-v1`
- `editor-material-gpu-preview-execution-evidence-v1`
- `editor-playtest-package-review-loop`
- `editor/src/main.cpp`
- `empty_image_decode_bytes`
- `engine/agent/manifest.json MK_runtime status must advertise the closed Runtime Resource v2 safe-point/controller surface honestly`
- `engine/agent/manifest.json aiDrivenGameWorkflow missing required sample field: $field`
- `engine/agent/manifest.json aiOperableProductionLoop`
- `engine/agent/manifest.json aiOperableProductionLoop missing reviewLoops`
- `engine/agent/manifest.json aiOperableProductionLoop must expose one editor-ai-package-authoring-diagnostics review loop`
- `engine/agent/manifest.json aiOperableProductionLoop must expose one editor-ai-playtest-evidence-summary review loop`
- `engine/agent/manifest.json aiOperableProductionLoop must expose one editor-ai-playtest-operator-handoff review loop`
- `engine/agent/manifest.json aiOperableProductionLoop must expose one editor-ai-playtest-remediation-queue review loop`
- `engine/agent/manifest.json aiOperableProductionLoop must expose one editor-playtest-package-review-loop review loop`
- `engine/agent/manifest.json aiOperableProductionLoop must record one physics-1-0-jolt-native-adapter decision`
- `engine/agent/manifest.json aiOperableProductionLoop physicsBackendAdapterDecisions`
- `engine/agent/manifest.json aiOperableProductionLoop physicsBackendAdapterDecisions unsupportedClaims missing: $claim`
- `engine/agent/manifest.json aiOperableProductionLoop.currentActivePlan must be the master plan when no child plan is active`
- `engine/agent/manifest.json aiOperableProductionLoop.currentActivePlan must match active child plan: $($activePlan.path)`
- `engine/agent/manifest.json aiOperableProductionLoop.recommendedNextPlan.id must be next-production-gap-selection when no child plan is active`
- `engine/agent/manifest.json aiOperableProductionLoop.schemaVersion must be 1`
- `engine/agent/manifest.json editor-ai-package-authoring-diagnostics`
- `engine/agent/manifest.json editor-ai-package-authoring-diagnostics blockedExecution missing: $claim`
- `engine/agent/manifest.json editor-ai-package-authoring-diagnostics must be ready as diagnostics-only editor-core model`
- `engine/agent/manifest.json editor-ai-package-authoring-diagnostics ordered step`
- `engine/agent/manifest.json editor-ai-package-authoring-diagnostics orderedSteps must be: $($expectedEditorAiDiagnosticsSteps -join ', ')`
- `engine/agent/manifest.json editor-ai-package-authoring-diagnostics requiredManifestFields missing: $field`
- `engine/agent/manifest.json editor-ai-package-authoring-diagnostics step '$($_.id)' must not execute`
- `engine/agent/manifest.json editor-ai-package-authoring-diagnostics step '$($_.id)' must not mutate`
- `engine/agent/manifest.json editor-ai-package-authoring-diagnostics unsupportedClaims missing: $claim`
- `engine/agent/manifest.json editor-ai-playtest-evidence-summary`
- `engine/agent/manifest.json editor-ai-playtest-evidence-summary blockedExecution missing: $claim`
- `engine/agent/manifest.json editor-ai-playtest-evidence-summary evidenceFields missing: $field`
- `engine/agent/manifest.json editor-ai-playtest-evidence-summary evidenceInputs missing: $expectedManifestInput`
- `engine/agent/manifest.json editor-ai-playtest-evidence-summary must be ready as read-only editor-core evidence summary model`
- `engine/agent/manifest.json editor-ai-playtest-evidence-summary ordered step`
- `engine/agent/manifest.json editor-ai-playtest-evidence-summary orderedSteps must be: $($expectedEditorAiEvidenceSummarySteps -join ', ')`
- `engine/agent/manifest.json editor-ai-playtest-evidence-summary requiredManifestFields missing: $field`
- `engine/agent/manifest.json editor-ai-playtest-evidence-summary step '$($_.id)' must not execute`
- `engine/agent/manifest.json editor-ai-playtest-evidence-summary step '$($_.id)' must not mutate`
- `engine/agent/manifest.json editor-ai-playtest-evidence-summary unsupportedClaims missing: $claim`
- `engine/agent/manifest.json editor-ai-playtest-operator-handoff`
- `engine/agent/manifest.json editor-ai-playtest-operator-handoff blockedExecution missing: $claim`
- `engine/agent/manifest.json editor-ai-playtest-operator-handoff commandFields missing: $field`
- `engine/agent/manifest.json editor-ai-playtest-operator-handoff handoffInputs missing: $expectedManifestInput`
- `engine/agent/manifest.json editor-ai-playtest-operator-handoff must be ready as read-only editor-core handoff model`
- `engine/agent/manifest.json editor-ai-playtest-operator-handoff ordered step`
- `engine/agent/manifest.json editor-ai-playtest-operator-handoff orderedSteps must be: $($expectedEditorAiOperatorHandoffSteps -join ', ')`
- `engine/agent/manifest.json editor-ai-playtest-operator-handoff requiredManifestFields missing: $field`
- `engine/agent/manifest.json editor-ai-playtest-operator-handoff step '$($_.id)' must not execute`
- `engine/agent/manifest.json editor-ai-playtest-operator-handoff step '$($_.id)' must not mutate`
- `engine/agent/manifest.json editor-ai-playtest-operator-handoff unsupportedClaims missing: $claim`
- `engine/agent/manifest.json editor-ai-playtest-remediation-queue`
- `engine/agent/manifest.json editor-ai-playtest-remediation-queue must be ready as read-only editor-core remediation queue model`
- `engine/agent/manifest.json editor-playtest-package-review-loop`
- `engine/agent/manifest.json editor-playtest-package-review-loop hostGatedSmokeRecipes missing: $recipe`
- `engine/agent/manifest.json editor-playtest-package-review-loop must be ready inside its reviewed scope`
- `engine/agent/manifest.json editor-playtest-package-review-loop orderedSteps must be: $($expectedEditorReviewSteps -join ', ')`
- `engine/agent/manifest.json editor-playtest-package-review-loop preSmokeGate must be validate-runtime-scene-package`
- `engine/agent/manifest.json editor-playtest-package-review-loop requiredManifestFields missing: $field`
- `engine/agent/manifest.json must expose aiDrivenGameWorkflow`
- `engine/agent/manifest.json must expose exactly one MK_runtime module`
- `engine/agent/manifest.json must expose gameCodeGuidance.currentInput`
- `engine/animation/include/mirakana/animation/skeleton.hpp`
- `engine/assets/include/mirakana/assets/asset_source_format.hpp`
- `engine/assets/src/tilemap_metadata.cpp`
- `engine/assets/src/ui_atlas_metadata.cpp`
- `engine/core/include/mirakana/core/diagnostics.hpp`
- `engine/renderer/include/mirakana/renderer/renderer.hpp`
- `engine/renderer/include/mirakana/renderer/rhi_frame_renderer.hpp`
- `engine/renderer/src/rhi_frame_renderer.cpp`
- `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- `engine/rhi/d3d12/src/d3d12_backend.cpp`
- `engine/rhi/include/mirakana/rhi/rhi.hpp`
- `engine/rhi/src/async_overlap.cpp`
- `engine/rhi/src/null_rhi.cpp`
- `engine/rhi/vulkan/include/mirakana/rhi/vulkan/vulkan_backend.hpp`
- `engine/rhi/vulkan/src/vulkan_backend.cpp`
- `engine/runtime/include/mirakana/runtime/asset_runtime.hpp`
- `engine/runtime/src/session_services.cpp`
- `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp`
- `engine/runtime_host/sdl3/src/scene_gpu_binding_injecting_renderer.hpp`
- `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`
- `engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp`
- `engine/runtime_rhi/src/runtime_upload.cpp`
- `engine/runtime_scene/include/mirakana/runtime_scene/runtime_scene.hpp`
- `engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp`
- `engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp`
- `engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp`
- `engine/tools/asset/source_image_decode.cpp`
- `engine/tools/asset/ui_atlas_tool.cpp`
- `engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp`
- `engine/tools/include/mirakana/tools/source_image_decode.hpp`
- `engine/tools/include/mirakana/tools/ui_atlas_tool.hpp`
- `engine/ui/include/mirakana/ui/ui.hpp`
- `engine/ui/src/ui.cpp`
- `engine/ui_renderer/include/mirakana/ui_renderer/ui_renderer.hpp`
- `engine/ui_renderer/src/ui_renderer.cpp`
- `evidenceFields`
- `evidenceInputs`
- `excluded-from-1-0-ready-surface`
- `exit code`
- `exit code or summary text`
- `explicit 1.0 exclusion`
- `external/vcpkg`
- `externally supplied run-validation-recipe execute results`
- `fail-closed capability negotiation`
- `fast-forwards it`
- `fast-forwards the local checkout`
- `first-party MK_physics only`
- `foundationPlan`
- `function Format-CppSourceText`
- `function New-DesktopRuntime3DMainCpp`
- `function New-DesktopRuntime3DPackageFiles`
- `gameplayFoundationSample`
- `games/CMakeLists.txt`
- `games/sample_desktop_runtime_game/README.md`
- `games/sample_desktop_runtime_game/game.agent.json`
- `games/sample_headless/game.agent.json`
- `generated 3D NORMAL/TANGENT morph package smoke`
- `generated 3D morph GPU palette smoke`
- `generatedDesktopRuntimeCookedScenePackageSample`
- `generatedDesktopRuntimeMaterialShaderPackageSample`
- `generatedDesktopRuntimePackageSample`
- `gh pr create`
- `gh pr merge --auto --merge --delete-branch`
- `gh pr merge --merge --delete-branch`
- `gh pr view <pr> --json`
- `gh pr view <pr> --json headRefOid,statusCheckRollup,url`
- `git config --show-origin --get-all credential.helper`
- `git fetch --prune <remote>`
- `git log --oneline origin/main..<headRefOid>`
- `glTF Node Transform Animation Binding Source Bridge v1`
- `glTF Node Transform Animation Float Clip Bridge v1`
- `glTF Node Transform Animation Import v1`
- `historical implementation evidence`
- `host-gated-safe-point`
- `hostGates`
- `hosted PR/CI check failures`
- `import_gltf_node_transform_animation_binding_source`
- `import_gltf_node_transform_animation_float_clip`
- `import_gltf_node_transform_animation_quaternion_clip`
- `import_gltf_node_transform_animation_tracks`
- `input game guidance`
- `input rebinding profile guidance`
- `inputRendererSample`
- `inspect-runtime-package-payload-diagnostics`
- `installed SDK payload gate`
- `installed desktop runtime validation`
- `installed-d3d12-3d-directional-shadow-smoke`
- `installed-d3d12-3d-package-smoke`
- `installed-d3d12-3d-shadow-morph-composition-smoke`
- `installed-sdk-validation-check: ok`
- `interactive runtime/game rebinding panels`
- `invalid_accessibility_bounds`
- `invalid_font_allocation`
- `invalid_font_family`
- `invalid_font_glyph`
- `invalid_font_pixel_size`
- `invalid_image_decode_uri`
- `invalid_ime_cursor`
- `invalid_ime_target`
- `invalid_platform_text_input_bounds`
- `invalid_platform_text_input_target`
- `invalid_text_shaping_font_family`
- `invalid_text_shaping_max_width`
- `invalid_text_shaping_result`
- `invalid_text_shaping_text`
- `isolation: worktree`
- `keep_local_child`
- `keep_nested_prefab_instance`
- `keep_stale_source_node_as_local`
- `lcov --ignore-errors unused`
- `lightweight static checks for docs/agent-only slices`
- `live plan stack shallow`
- `liveShaderGeneration`
- `load_prefab_authoring_document`
- `long-path deletion fallback`
- `long-path-delete-fallback=enabled`
- `machine-readable capability/status claims`
- `make_animation_joint_tracks_3d_from_f32_bytes`
- `make_scene_authoring_component_edit_action`
- `make_scene_authoring_create_node_action`
- `make_scene_authoring_delete_node_action`
- `make_scene_authoring_duplicate_subtree_action`
- `make_scene_authoring_transform_edit_action`
- `make_scene_package_candidate_rows`
- `make_scene_prefab_instance_refresh_action`
- `make_scene_prefab_instance_refresh_batch_action`
- `make_ui_text_glyph_sprite_command`
- `materialGraph`
- `mergeStateStatus`
- `metalDevice`
- `metalReady`
- `middleware type exposure`
- `mirakana/editor/scene_authoring.hpp`
- `mirakana::editor::SceneAuthoringDocument`
- `morph_graphics_pipeline`
- `morph_mesh_assets`
- `native physics handles in public gameplay APIs`
- `nativeGpuOutput`
- `nativeHandle`
- `new-game-helpers.ps1`
- `new-game-templates.ps1`
- `new-game.ps1`
- `normal_delta_buffer`
- `normalized-build-environment`
- `normalized-configure-environment`
- `not PR/task-count units`
- `not Windows/MSVC, macOS, or full repository clang-tidy`
- `not publication-complete after local validation alone`
- `official Anthropic documentation`
- `official GitHub Flow`
- `one PR per focused capability/gap-cluster/milestone`
- `one reviewable purpose`
- `open draft PR`
- `packageStreamingReady`
- `packageSurfaces`
- `packed runtime UI atlas apply leaves existing files unchanged when validation fails`
- `packed runtime UI atlas authoring maps decoded images into texture page and metadata`
- `packed runtime UI atlas package update writes texture page metadata and package index`
- `packed runtime UI atlas rejects invalid decoded images and package path collisions`
- `packed runtime UI glyph atlas apply leaves existing files unchanged when validation fails`
- `packed runtime UI glyph atlas authoring maps rasterized glyphs into texture page and metadata`
- `packed runtime UI glyph atlas package update writes texture page metadata and package index`
- `packed runtime UI glyph atlas rejects invalid glyph pixels and package path collisions`
- `path-filtered required workflows`
- `per-device profiles`
- `phase behavior/API/validation`
- `phase behavior/API/validation boundary`
- `phase-gated milestone plan`
- `physics-1-0-jolt-native-adapter`
- `pixtool --help`
- `play-in-editor productization`
- `policy reload`
- `post-merge worktree cleanup`
- `prefabScenePackageAuthoringTargets`
- `production completion master plan`
- `productionAtlasPacking`
- `productionSpriteBatching`
- `protected branches`
- `purpose/checkpoint-based`
- `push validated checkpoints`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/src/physics3d.cpp`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format-text.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1 -WorktreePath <path> [-BaseRef origin/main] [-BaseBranch main] [-Remote origin] [-LocalCheckoutPath <path>] [-DeleteLocalBranch]`
- `quaternion scene-transform smoke`
- `quaternion_animation_scene_rotation_z`
- `queueFields`
- `queueInputs`
- `radial stick deadzones`
- `rasterized glyph must be RGBA8`
- `raw manifest command strings`
- `raw/free-form command evaluation`
- `readiness dependency`
- `ready-runtime-resource-v2-safe-point-controller`
- `recipe id`
- `recipe status`
- `recipeStatusEnum`
- `references`
- `refresh_batch_prefab_instances_review`
- `refresh_selected_prefab_instance`
- `register-runtime-package-files.ps1`
- `registeredSourceAssetCookTargets`
- `release-package-artifacts-check: ok`
- `remote state`
- `renderer-resource-residency-upload-execution`
- `rendererBackend`
- `rendererQualityClaim`
- `report-evidence-blockers-and-unsupported-claims`
- `report-host-gated-desktop-smoke-preflight`
- `report-host-gates-blockers-and-unsupported-claims`
- `resolve_runtime_scene_animation_transform_bindings`
- `resolve_ui_text_glyph_binding`
- `review-editor-package-candidates`
- `reviewed command display`
- `rhiHandle`
- `run-host-gated-desktop-smoke`
- `run-validation-recipe dry-run command plan data`
- `runtime UI PNG image decoding adapter fails closed when importers are disabled`
- `runtime UI PNG image decoding adapter returns rgba8 image when importers are enabled`
- `runtime scene RHI morph GPU palette bridge`
- `runtime/.gitattributes`
- `runtimeImageDecoding`
- `runtimeMorphMeshGpuBinding`
- `runtimeSceneRhiMorphGpuPalette`
- `runtime_animation_quaternion_clip_payload`
- `sample2dDesktopRuntimePackageSample`
- `sampleGame`
- `sample_and_apply_runtime_scene_render_animation_float_clip`
- `sample_and_apply_runtime_scene_render_animation_pose_3d`
- `sample_runtime_morph_mesh_cpu_animation_float_clip`
- `save_prefab_authoring_document`
- `scene_gpu_morph_mesh_uploads`
- `scene_prefab_instance_refresh`
- `select-runtime-scene-validation-target`
- `selected hosted checks to complete`
- `selected linked prefab root`
- `serialize_animation_transform_binding_source_document`
- `shaderGraph`
- `shaders/runtime_postprocess.hlsl`
- `shaders/runtime_scene.hlsl`
- `sourceAssetPath`
- `sourceImagePath`
- `sourceScenePath`
- `sourceTilemapPath`
- `span.glyph`
- `specific, concise, verifiable`
- `split unrelated work`
- `standard worktree-root checks from official porcelain records`
- `status passed failed blocked host-gated missing`
- `std::uint32_t glyph`
- `summarize-evidence-status`
- `summarize-manifest-descriptor-rows`
- `summarize-validation-recipe-status`
- `summary text`
- `tangent_delta_buffer`
- `tests/shaders/vulkan_compute_morph_position.hlsl`
- `tests/shaders/vulkan_compute_morph_renderer_position.hlsl`
- `tests/shaders/vulkan_compute_morph_tangent_frame.hlsl`
- `tests/unit/backend_scaffold_tests.cpp`
- `tests/unit/core_tests.cpp`
- `tests/unit/d3d12_rhi_tests.cpp`
- `tests/unit/rhi_tests.cpp`
- `tests/unit/runtime_host_sdl3_public_api_compile.cpp`
- `tests/unit/runtime_host_sdl3_tests.cpp`
- `tests/unit/runtime_rhi_tests.cpp`
- `tests/unit/tools_tests.cpp`
- `tests/unit/ui_renderer_tests.cpp`
- `text_glyphs_missing`
- `tilemapEditorUX`
- `tools/apple-host-helpers.ps1`
- `tools/check-apple-host-evidence.ps1`
- `tools/check-installed-sdk-validation.ps1`
- `tools/check-mobile-packaging.ps1`
- `tools/check-production-readiness-audit.ps1`
- `tools/check-release-package-artifacts.ps1`
- `tools/check-text-format-contract.ps1`
- `tools/check-text-format.ps1`
- `tools/format-text.ps1`
- `tools/installed-sdk-validation.ps1`
- `tools/new-game-helpers.ps1`
- `tools/new-game-templates.ps1`
- `tools/new-game.ps1`
- `tools/release-package-artifacts.ps1`
- `tools/remove-merged-worktree.ps1`
- `tools/static-contract-ledger.ps1`
- `tools/text-format-core.ps1`
- `tools/validate-installed-desktop-runtime.ps1`
- `tools/validate-installed-sdk.ps1`
- `ui accessibility publish plan blocks invalid nodes before adapter`
- `ui accessibility publish plan dispatches validated nodes to adapter`
- `ui font rasterization request plan blocks invalid request before adapter`
- `ui font rasterization request plan dispatches valid request to adapter`
- `ui font rasterization result reports invalid adapter allocation`
- `ui image decode request plan blocks invalid request before adapter`
- `ui image decode request plan dispatches valid request to adapter`
- `ui image decode result reports missing or invalid adapter output`
- `ui ime composition publish plan blocks invalid composition before adapter`
- `ui ime composition publish plan dispatches valid composition to adapter`
- `ui renderer reports missing glyph atlas bindings without fake sprites`
- `ui renderer submits monospace text glyphs through glyph atlas palette`
- `ui text shaping request plan blocks invalid request before adapter`
- `ui text shaping request plan dispatches valid request to adapter`
- `ui text shaping result reports invalid adapter runs`
- `uiAudioAssetsSample`
- `unlinks worktree-local vcpkg links`
- `upload_runtime_morph_mesh_cpu`
- `uploaded_morph_bytes`
- `uploaded_normal_delta_bytes`
- `uploaded_tangent_delta_bytes`
- `utf8_scalar_glyph`
- `validate the current ZIP SHA-256 sidecar`
- `validate-runtime-scene-package`
- `validate_scene_authoring_references`
- `validated commit checkpoints`
- `validates installed SDK metadata before the installed consumer build`
- `validation-only follow-up`
- `validationRecipeMap`
- `vcpkg manifest feature`
- `without following reparse points`
- `worktree`
- `wpr -help`
- `xperf -help`
