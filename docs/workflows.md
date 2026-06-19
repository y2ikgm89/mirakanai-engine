# Workflows

## Standard Task Prompt

Every AI task should include:

- Goal
- Context
- Constraints
- Done when

Extended validation, editor-shell, plan lifecycle, production-completion, and game-lane checklists live in [`docs/agent-operational-reference.md`](agent-operational-reference.md) (indexed from `AGENTS.md`).

## Default Implementation Flow

1. Read relevant docs and AGENTS instructions.
2. Update or create a spec for non-trivial changes.
3. Update or create a plan for multi-step work.
4. Add tests first when the local toolchain can run them.
5. Implement the smallest coherent change.
6. Run a targeted agent-surface drift check: compare the changed behavior/API/workflow against the owning docs, skills, rules, subagents, manifest fragments, schemas, and static checks. If durable behavior, public APIs, architecture, workflow, validation, packaging, permissions, tool expectations, AI-operable capability claims, or repeated agent failure modes drift, update the relevant `AGENTS.md`, `CLAUDE.md`, docs, `.agents/skills/`, `.claude/skills/`, `.cursor/skills/`, `.codex/agents/`, `.claude/agents/`, `.cursor/agents/`, `.codex/rules`, `.claude/rules`, `.claude/settings.json`, manifest fragments + compose output, schemas, and static checks in the same task; do not broad-load every agent surface when no durable guidance changed.
7. Run the narrowest validation tier that proves the touched surface. Use focused checks for docs/agent-only/non-runtime slices; use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` for C++/runtime/build/packaging/public-contract slice gates.
8. For production-readiness changes, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`; Production 1.0 Readiness Audit v1 verifies manifest `unsupportedProductionGaps` rows are well-formed, reports `production-readiness-audit-check: ok`, and does not treat known non-ready gaps as complete.
9. For release-facing SDK changes, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1`; it also validates a clean installed consumer example. For editor-independent desktop runtime release changes, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` for the default sample shell or `tools/package-desktop-runtime.ps1 -GameTarget <target>` for a registered desktop runtime game target; the script builds the `MK_desktop_runtime_package_build` closure for installable libraries, the selected runtime game, and required package smoke tests rather than unrelated release test executables. Registered targets must declare `GAME_MANIFEST` and source-tree smoke args, package smoke args and package files come from the registered CMake metadata unless `-SmokeArgs` is supplied, shader-artifact requirements come from metadata, explicit `-RequireD3d12Shaders` and `-RequireVulkanShaders` are only valid for targets whose metadata declares those shader artifacts, and installed validation rejects metadata selected-target mismatches while verifying the selected installed game manifest, package files, declared shader artifacts, and the required selected-game status line and presentation report smoke fields. When a selected smoke includes `--require-postprocess-depth-input`, installed validation also requires `postprocess_depth_input_ready=1` on a ready scene GPU path. When a selected smoke includes `--require-directional-shadow`, installed validation also requires `directional_shadow_status=ready`, `directional_shadow_ready=1`, `framegraph_passes=3`, `framegraph_passes_executed=6`, `framegraph_render_passes_recorded=6`, and `framegraph_barrier_steps_executed=15`. When a selected smoke includes `--require-directional-shadow-filtering`, installed validation also requires `directional_shadow_filter_mode=fixed_pcf_3x3`, `directional_shadow_filter_taps=9`, and `directional_shadow_filter_radius_texels=1`; this proves only the current package-visible fixed sampled-depth 3x3 PCF shadow smoke, not hardware comparison samplers, cascades, atlases, Metal presentation, or production shadow authoring. When a selected smoke includes `--require-renderer-quality-gates`, installed validation also requires `renderer_quality_expected_framegraph_render_passes`, `renderer_quality_framegraph_render_passes_ok`, `renderer_quality_expected_framegraph_barrier_steps`, `renderer_quality_framegraph_barrier_steps_ok`, and `renderer_quality_framegraph_execution_budget_ok` to match the selected postprocess/depth/shadow budget. When a selected smoke includes `--require-package-upload-staging`, installed validation requires `package_upload_staging_status=ready`, `package_upload_staging_ready=1`, four selected package upload transactions, four pool lease-backed ring uploads, submitted fences, graphics waits for copy-queue uploads, async-ready resource update counters, no diagnostics, and positive uploaded bytes. When a selected smoke includes `--require-native-ui-overlay`, installed validation requires `ui_overlay_requested=1`, `ui_overlay_status=ready`, `ui_overlay_ready=1`, and positive `ui_overlay_sprites_submitted` / `ui_overlay_draws`; this proves only the renderer-owned colored box/image-placeholder overlay path, not production text shaping, font rasterization, image decoding, real atlases, IME, OS accessibility bridges, Metal overlay readiness, or general renderer quality. Source-tree desktop runtime builds stage each registered target beside a target-specific executable directory so generated package payloads and shader artifacts cannot overwrite another target's smoke inputs. For `DesktopRuntimeMaterialShaderPackage` targets, keep `source/materials/*.material` and `shaders/*.hlsl` as authoring inputs outside `runtimePackageFiles`; when DXC is available the source-tree lane also runs a target-specific shader-artifact smoke, while the installed package lane remains the real renderer/GPU proof and installs only runtime package files plus host-built selected shader artifacts. After package draft review, use the editor Assets panel `Apply Package Registration` button or `tools/register-runtime-package-files.ps1 -GameManifest games/<game_name>/game.agent.json -RuntimePackageFile <game-relative-file>` to add safe game-relative `runtimePackageFiles` entries. For Editor Playtest Package Review Loop v1, then select the package/scene pair from `game.agent.json.runtimeSceneValidationTargets`, run `validate-runtime-scene-package`, and only after that run the selected host-gated desktop smoke; package validation remains the final authority.
   - `--require-lighting-shadow-policy` package smokes validate only backend-neutral packaged-scene policy counters: `lighting_shadow_policy_status=ready`, `lighting_shadow_policy_ready=1`, `lighting_shadow_policy_diagnostics=0`, one directional shadowed light, `lighting_shadow_policy_light_rows=1`, a 1024x1024 policy atlas, and `lighting_shadow_policy_ready_frames=<expected smoke frames>`. They do not prove additional rendered lights, Vulkan/Metal visual parity, or production shadow quality.
   - `--require-runtime-ui-production-stack` package smokes validate only first-party runtime UI production stack evidence counters: shaping direction/script/language rows, glyph ids/clusters/advances/offsets/fallback/bidi/line-break rows, glyph bitmap/metrics/pixel-format rows, atlas placement/budget/eviction rows, renderer upload handoff, dependency/host gates, unsupported-claim rejection, `runtime_ui_production_stack_status=host_evidence_required`, six rows, four ready rows, two host-gated rows, text contract and selected package evidence ready, IME/accessibility host evidence required but absent, zero adapter/native/renderer-upload invocation fields, zero diagnostics, and a positive replay hash. They do not prove production text shaping, real font loading/rasterization, native IME parity, OS accessibility publication, renderer texture upload execution, UI middleware, or broad runtime UI readiness.
10. For mobile-facing changes, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1`; use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-android.ps1 -Game <game_name> -Configuration Debug`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-android-release-package.ps1 -Game <game_name> -UseLocalValidationKey`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-android-package.ps1 -Game <game_name> -Configuration Release -SkipBuild -StartEmulator -AvdName Mirakanai_API36` on Android-ready hosts, and use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-apple.ps1 -Game <game_name> -Configuration Debug -Platform Simulator` or `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-ios-package.ps1 -Game <game_name> -Configuration Debug` only on macOS/Xcode hosts.
11. Record validation blockers explicitly.

## Documentation And Plan Lifecycle

- Start documentation navigation from `docs/README.md`.
- Use English for current-truth docs, agent-facing guidance, public API explanations, build and validation procedures, and new documentation. Historical dated specs and plans may retain their original language unless they are promoted to current truth, actively edited, or named by the active manifest/registry pointers.
- Use `docs/current-capabilities.md` for a concise human-readable capability summary. The machine-readable source of truth remains the composed `engine/agent/manifest.json` (maintain via `engine/agent/manifest.fragments/` + `tools/compose-agent-manifest.ps1 -Write`).
- Use `docs/roadmap.md` for current status and priorities. Do not use it as a detailed task log.
- Use `docs/superpowers/plans/README.md` as the implementation plan registry.
- Use `docs/specs/README.md` to classify design records. Specs are design context, not live task lists.
- Keep completed plans only while they remain referenced evidence for current docs, manifests, checks, or active decisions. Do not append unrelated follow-up tasks to completed plans; remove unreferenced historical noise in explicit cleanup and rely on Git history for deleted records.
- Keep the live plan stack shallow: one active roadmap, one active gap-cluster burn-down or milestone, and at most one active phase/child plan selected by `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.
- Treat dated plan files as capability/gap-cluster/milestone records. Use phases inside that plan for phase behavior/API/validation boundary decisions, and link the plan from the registry when work starts.
- Plan-file width and PR/phase width are different: a plan may cover a capability/gap-cluster/milestone with multiple phase checkpoints, while each phase and PR stays one reviewable purpose with focused validation evidence.
- **Date prefix:** use the authoritative authoring `YYYY-MM-DD` (session `Today's date`, operator confirmation, or local `Get-Date -Format yyyy-MM-dd`); keep filename and heading aligned. See `docs/superpowers/plans/README.md` § Dated plan and spec filenames.
- Do not create new plan files for validation-only follow-up, docs/manifest/static-check synchronization, small mechanical cleanup, or substeps that fit the current active plan checklist.
- Prefer a gap-level burn-down or phase-gated milestone when one production-readiness gap needs several linked phases; create child plans only when the next phase has a different architecture decision, public API family, validation surface, ownership/review boundary, or is too large to execute safely in one context.
- For **generated static 3D desktop package** operator sequencing (`DesktopRuntime3DPackage`, recipe `3d-playable-desktop-package`), follow the **3D Desktop Package Foundation** section in [ai-game-development.md](ai-game-development.md), the committed `games/sample_generated_desktop_runtime_3d_package` proof, and the active plan registry. The selected installed D3D12 package smoke may include `--require-gltf-scene-import-review`; treat that as reviewed package evidence only, requiring `gltf_scene_import_review_status=ready`, `gltf_scene_import_review_ready=1`, zero external network/runtime source parsing/parser type/native handle/package mutation counters, no broad scene import claim, and a positive replay hash. The retired `Generated Static 3D Production Game Recipe v1` record is traceability evidence only; current readiness comes from `engine/agent/manifest.json`, the plan registry, and validation evidence for the selected target.
- Use a phase-gated milestone plan when multiple tightly related slices share one end-to-end capability/gap-cluster/milestone objective and repeated tiny active-plan hops would hide the decision. Each phase still needs Goal, Context, Constraints, Done When, RED -> GREEN evidence for behavior changes, and validation evidence. Do not use a milestone to append unrelated work, weaken host gates, or broaden ready claims.
- This is a greenfield engine: prefer clean replacement, consolidation, or retirement of obsolete planned slices over compatibility shims unless a future release policy explicitly requires compatibility.
- Keep host-gated work explicit. A task blocked by macOS/Xcode, Android devices/signing, Vulkan shader tools, Metal tools, or missing local clang-tidy/CMake tooling remains blocked until a local or CI lane proves it.
- After a plan completes, update subsystem docs, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, and the engine agent manifest (fragments + `compose-agent-manifest.ps1 -Write`) in the same task when capabilities changed.
- **`MK_tools` source paths:** Implementation translation units live under `engine/tools/{shader,gltf,asset,scene}/` (see `docs/specs/2026-05-11-directory-layout-target-v1.md` and `docs/adr/0003-directory-layout-clean-break.md`). Relocating those `.cpp` files requires updating `tools/check-json-contracts.ps1` and `tools/check-ai-integration.ps1` path strings and the relevant `engine/tools/*/CMakeLists.txt` fragments together; public `#include <mirakana/tools/...>` paths do not move with this v1 split.

## Production Completion Prompt

When executing [Production Completion Master Plan v1](superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md), keep the operating prompt short and evidence-driven:

- Use `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`, `recommendedNextPlan`, and `unsupportedProductionGaps` as the execution index.
- Treat `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md` as a lightweight index; load only the needed chapter from the four-file `production-completion-v1` corpus, and load `99-historical-verdict-archive.md` only for static-check history or migration audits.
- Each `unsupportedProductionGaps` row includes `oneDotZeroCloseoutTier` (`foundation-follow-up`, `package-evidence`, or `closeout-wedge`) for 1.0 closeout grouping; change it through `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`, not by editing `engine/agent/manifest.json` directly.
- The production-completion historical verdict archive now lives in `docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md` and preserves retained static-check evidence. Do not remove or hollow it without running `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` afterward.
- Re-read the master plan, registry, and manifest after user edits or context resumes before changing files.
- Burn down one selected production gap at a time until it is implemented, host-gated, blocked with evidence, or explicitly excluded from the 1.0 ready surface.
- Prefer official documentation, Context7, project skills, and clean breaking greenfield designs over compatibility shims, broad ready claims, or undocumented shortcuts.
- Use focused build/test/static checks during implementation, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the coherent slice-closing gate.
- Use subagents only for bounded independent investigation, plan lifecycle audit, review, build-failure triage, or disjoint implementation that improves speed or confidence; keep immediate blocking work local.
- Before completion, reconcile code, tests, docs, plans, manifest, static checks, completed gap, remaining gaps, next active plan, and host-gated blockers against actual validation evidence.

## Static Analysis And API Boundaries

- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` for clang-tidy. The default validation path runs independent PowerShell static checks through bounded parallel jobs with per-check timeouts, writes complete per-script logs under ignored `out/validation-logs/`, retains only bounded excerpts in console output, verifies `.clang-tidy`, prepares a CMake File API codemodel query before `build.ps1`, reuses the generated reply for the `validate.ps1` `-MaxFiles 1` smoke unless `-SkipTidySmoke` is explicitly passed, and still lets standalone `check-tidy.ps1` configure on demand when native `compile_commands.json` is unavailable. It synthesizes `compile_commands.json` for the Windows Visual Studio `dev` preset when the generator does not emit one, and reports explicit CMake/clang-tidy tool blockers instead of silently skipping analysis. Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/src/physics3d.cpp` for wrapper-owned changed-file analysis when a full repository run is too broad for the current slice. `-Strict` passes `--warnings-as-errors=*`, `-Jobs` parallelizes independent translation units, and `-Jobs 0` is the default automatic CPU-count throttle. `-ShardCount` / `-ShardIndex` split a full source list deterministically for hosted matrix jobs while each shard still runs strict diagnostics. `.clang-tidy` keeps `HeaderFilterRegex` absolute-path aware so CI and Visual Studio File API compile databases analyze first-party headers under `engine`, `editor`, `examples`, `games`, and `tests`. The wrapper preserves actionable diagnostics and exit codes while suppressing clang frontend summary lines like `NN warnings generated.`. GitHub Actions has a reviewed sharded `static-analysis` lane that runs `tools/check-tidy.ps1 -Strict -Preset ci-linux-tidy -ShardCount 4 -ShardIndex <matrix> -Jobs 0` on `ubuntu-latest`, and reviewed workflows use full-length SHA-pinned first-party actions, `actions/checkout` with `persist-credentials: false`, `permissions: contents: read`, one retry for transient checkout fetch/auth failures, and same-ref PR/push/manual concurrency cancellation for least-privilege and stale-run control. Hosted `Agent Static Guards` owns repository static validation through `tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120` on the supported Windows toolchain family so hosted Android diagnostic daemons cannot hold a parallel job pipe open; hosted PR `Windows MSVC` owns MSVC build/test through `tools/validate.ps1 -SkipStaticChecks -SkipTidySmoke` because the dedicated static lanes own full static-analysis evidence. Local `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` verifies those lanes exist but does not execute CI locally.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-coverage.ps1` for coverage. Coverage is currently enforced only on Linux GCC/Clang CI where gcov coverage is stable; other hosts report an explicit blocker instead of producing partial data.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` after touching public headers or backend interop. Public engine/editor headers must not expose native D3D12/DXGI/Win32 symbols, COM pointers, or native graphics headers.
- Use backend PIMPL types or first-party opaque handles for native OS/GPU interop until a written interop design accepts a narrower exception.

## GitHub Actions Repository Settings

These settings are operator-applied in GitHub and are not fully enforceable from the repository alone. Keep the reviewed workflows and repository settings aligned:

- Under `Settings -> Actions -> General -> Actions permissions`, prefer `Allow OWNER, and select non-OWNER, actions and reusable workflows`, then allow GitHub-authored actions. This repository currently relies on first-party `actions/*` workflows only; introducing non-GitHub actions should be an explicit reviewed change.
- Enable `Require actions to be pinned to a full-length commit SHA`. Reviewed workflows already pin `actions/checkout`, `actions/cache`, `actions/cache/restore`, `actions/cache/save`, and `actions/upload-artifact`, and `tools/check-ci-matrix.ps1` guards against regression to floating major tags.
- Under `Workflow permissions`, keep the default `GITHUB_TOKEN` at the restricted read-only setting for `contents` and `packages`. Workflows in this repository also set `permissions: contents: read` explicitly, keep `actions/checkout` on `persist-credentials: false`, and retry checkout once rather than broadening token scope when hosted fetch/auth fails transiently.
- Keep `Allow GitHub Actions to create and approve pull requests` disabled. This repository uses human-initiated PR publishing and guarded merge automation outside workflow writes.
- If the repository is public and accepts fork pull requests, keep workflow approval requirements enabled for outside contributors at least at GitHub's default baseline. Raise that bar if runner abuse or workflow churn becomes a problem.

## Toolchain Preflight

- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` to report the CMake, CTest, optional CPack, `clang-format`, Visual Studio, and MSBuild paths that repository wrappers will use.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` includes `toolchain-check` before CMake configure/build/test work, runs independent static checks in a bounded parallel group with a per-check timeout, stores complete parallel-check logs under ignored `out/validation-logs/`, runs one `build.ps1` configure/build gate with automatic CMake parallelism, reuses that gate's CMake File API reply for the clang-tidy smoke unless `-SkipTidySmoke` is explicitly passed, and then runs `test.ps1 -SkipBuild` with automatic CMake/CTest parallelism. Pass `-StaticJobs <N>` to `tools/validate.ps1` only when a local host needs a lower or higher static-check throttle; pass `-StaticCheckTimeoutSeconds <N>` only when diagnosing host-specific static-check hangs or intentionally lowering the timeout for a focused probe. Hosted CI uses `-StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120` for the dedicated static lane and `-SkipStaticChecks -SkipTidySmoke` for the Windows MSVC lane; local slice gates should keep the default full path unless a narrower check is explicitly justified. Pass `-Jobs <N>` to `tools/build.ps1` or `tools/test.ps1` only when a local host needs an explicit lower throttle.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 ...` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 ...` for focused local CMake/CTest loops. The wrappers resolve the official tools and launch them with one canonical `Path` on Windows (`PATH` elsewhere) before Visual Studio generator compiler detection or MSBuild tool tasks start.
- Direct `cmake --preset ...` commands assume CMake is available on `PATH`. Prefer repository wrappers unless a task explicitly tests raw command behavior; checked-in configure/build presets still normalize the Visual Studio/MSBuild `PATH`/`Path` handoff for raw preset commands. Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake` to make the raw-command precondition fail fast.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireVcpkgToolchain` when a selected vcpkg-backed configure lane must fail fast on a missing `external/vcpkg` checkout.
- Checked-in CMake configure/build presets inherit `normalized-configure-environment` / `normalized-build-environment` so preset-defined environments remain explicit and verifiable; keep these hidden presets inherited by every visible configure/build preset and keep their `PATH` / `Path` normalization aligned with CMake Presets environment support.
- Direct `clang-format --dry-run ...` commands assume `clang-format` is available on `PATH`. Use the repository `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` / `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` wrappers when `toolchain-check` reports a resolved `clang-format` path but `direct-clang-format-status=unavailable`, or add the reported `clang-format` directory to `PATH`. These wrappers also check/apply tracked text normalization (LF, UTF-8 without BOM, a single final newline, and no trailing EOF blank lines); use `tools/check-text-format.ps1` / `tools/format-text.ps1` for text-only loops.
- Keep project-wide build settings in `CMakePresets.json`; keep local developer overrides in ignored `CMakeUserPresets.json`.

## Git Local Configuration

Follow Git's ignore-file ownership model instead of mixing host-specific rules into tracked project files:

- Commit shared project ignore rules in `.gitignore`.
- Keep repository-local, unshared ignore rules in `.git/info/exclude`; this file is not committed.
- Keep user-wide editor, backup, and temporary-file patterns in `core.excludesFile`. Git defaults that setting to `$XDG_CONFIG_HOME/git/ignore`, or `$HOME/.config/git/ignore` when `XDG_CONFIG_HOME` is unset or empty.

If sandboxed Git cannot read the default user-wide ignore file and reports `Permission denied` for `$HOME/.config/git/ignore`, leave `.gitignore` unchanged and configure this repository to use the local exclude file instead:

```powershell
$excludeFile = git rev-parse --path-format=absolute --git-path info/exclude
git config --local core.excludesFile $excludeFile
git config --show-origin --get-all core.excludesFile
git status --short --branch
```

This setting belongs in `.git/config` and must not be committed. It removes the sandbox-only warning without loosening host permissions, changing global Git state, or adding machine-specific paths to shared ignore rules.

**Line endings:** the repository root `.gitattributes` declares `* text=auto eol=lf` for Git-detected text (see `gitattributes(5)` and GitHub's line-ending guidance). Treat that file as the repository contract; prefer `core.autocrlf` unset or `false` for clones of this tree so local diagnostics and untracked-file behavior stay aligned with the same LF policy. Root `.editorconfig` keeps editors aligned with that contract.

If Git author identity is missing on a local workspace, set it locally unless the user asks for a global identity:

```powershell
git config --local user.name "<name>"
git config --local user.email "<email>"
```

Before creating a baseline or release-facing commit, default to the full repository validation gates and check the staged patch:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1
git diff --cached --check
```

Documentation-only or similarly narrow non-runtime slices should use the cheapest validation tier that still proves the changed contract. The PR body must name the commands that ran and explain why the narrower tier is the relevant signal. At minimum, run `git diff --check` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`; for agent-facing docs, skills, rules, settings, subagents, or workflow policy text, also run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` and the matching static guard such as `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`. Add `tools/check-json-contracts.ps1` for manifest fragments/schemas and `tools/check-ci-matrix.ps1` for workflow/CI policy changes. Do not spend Windows/MSVC, macOS, or full repository clang-tidy PR minutes on docs/agent-only changes unless the edit also changes code, build/test/package tooling, CI execution, validation scripts that run builds, or public/runtime contracts. Record concrete toolchain blockers instead of treating missing validation as success.

### Commit, Push, And Pull Request Workflow

Use GitHub's official GitHub Flow for agent publishing: make a separate topic branch for each unrelated change, commit isolated complete changes, create a pull request for review, merge only after required reviews/checks pass, then delete the merged branch. Cadence is purpose/checkpoint-based, not count-based: commit complete validated phases, push validated checkpoints for remote backup/CI/review/handoff, and keep one PR per focused capability/gap-cluster/milestone unless the work is unrelated. Multiple local commits may share one push or PR when they form one validated, reviewable purpose; split unrelated gaps into separate branches and PRs. Publish only task-owned changes and keep branch selection conservative.

Treat publishing as a slice-closing gate, not an optional epilogue. Unless the user explicitly asks for local-only/no-PR work, do not report a task complete while task-owned changes only exist locally after validation. Complete branch creation, task-owned staging, commit, non-forced push, and PR creation/update with validation evidence in the same turn; if any step is blocked by permissions, authentication, command policy, branch protection, or dirty unrelated files that prevent safe staging, report that exact blocker. If the preferred `codex/<topic>` branch form conflicts with existing ref namespaces, use a conservative non-default fallback such as `codex-<topic>`.

Publication-capable Codex sessions: before promising commit/push/PR/merge, confirm the current session can reach every official GitHub Flow surface. Linked worktrees store per-worktree Git metadata such as `HEAD` and `index` under the parent repository's `.git/worktrees/<name>` directory, so a session that can edit the worktree files but cannot create the Git `index.lock` there is not publication-capable. Use a trusted local/full-access session with network enabled and host-local Git/GitHub authentication available for publication. If a sandboxed or approval-free session cannot write Git metadata, read GitHub CLI configuration, or reach the remote, stop with that blocker after local validation evidence and continue publication in an approval-capable or full-access session; do not bypass GitHub Flow by hand-writing GitHub REST/MCP blobs, commits, trees, or PR state as the normal path.

Recommended preflight before staging or publishing from a Codex-controlled worktree:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1 -PullRequest <pr> -RequireClean
```

The wrapper prints `publication-preflight: ok` only after it confirms the current branch, `git status --short --branch`, `git rev-parse --path-format=absolute --git-path index.lock`, linked-worktree Git admin writability, `git update-index -q --refresh`, `git ls-remote --heads origin <branch>`, `Test-NetConnection` to GitHub, GitHub CLI config readability, `gh auth status`, and optional `gh pr view <pr> --json headRefOid,statusCheckRollup,url`. If it prints `publication-preflight: blocked`, switch session mode or host context instead of changing repository permissions ad hoc, creating a publication temp clone, or bypassing GitHub Flow.

Checkpoint guidance:

| Situation | Commit when | Push when |
| --- | --- | --- |
| Runtime, C++, build, packaging, or validation behavior | A complete behavior/API/validation phase is task-owned and has the focused validation needed for the touched surface. | The phase or slice-closing validation has passed, or a concrete environment blocker is recorded for the PR body. |
| Documentation, agent, rules, settings, or subagent-only work | The staged patch is task-owned and the narrow static checks for the changed contract pass. | The lightweight static validation tier proves the contract, or the blocker is recorded. |
| Review feedback | Each follow-up is an isolated, understandable fix or a small batch of related fixes. | The response batch is ready for reviewers; the PR will update automatically after the push. |
| End of session or handoff | Only if the local state is coherent enough to preserve as history. | Push a task-owned topic branch when remote backup, CI, or review visibility is useful; do not push known-broken intermediate work just because a commit exists. |

PR cadence is purpose-based, not commit-based: one PR should represent one reviewable capability/gap-cluster/milestone phase, one full small milestone, or one unrelated fix. Do not open a PR for every commit, checklist item, or validation-only follow-up; keep related phase commits in the same draft PR until final validation, and split only when review, architecture, or validation boundaries diverge.

For large or review-sensitive slices, open a draft PR after the first coherent validated push rather than waiting for the final local polish pass. Draft PRs are the preferred early feedback surface; keep them draft while follow-up checkpoint commits are still expected, and move to ready-for-review only after final validation, branch preflight, and task-owned PR evidence are in place. In unattended Codex sessions, use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ready-task-pr.ps1 -PullRequest <pr> -ExpectedBase main` for guarded task-owned draft-to-ready conversion; raw `gh pr ready` and other PR state changes remain prompt-gated.

1. Inspect the branch and worktree before staging:

```powershell
git status --short --branch
git diff --stat
git diff
```

2. Stage only task-owned files, then review the staged patch:

```powershell
git add <task-owned-files>
git diff --cached --stat
git diff --cached --check
```

3. Commit only a coherent, validated slice. Do not include unrelated user changes, ignored scratch output, generated logs, credentials, signing keys, local overrides, `.claude/settings.local.json`, `.mcp.json`, or `AGENTS.override.md`. Use a concise imperative commit subject and avoid AI-generated trailers unless the user asks for them.

4. Push only a reviewed topic branch at a validated checkpoint. Prefer `codex/<topic>` for Codex-created branches unless the user asks for another name. Before pushing, refresh or inspect the remote state so the push is based on the current upstream view:

```powershell
git branch --show-current
git remote -v
git fetch origin
git push -u origin <branch>
```

If the task edits `.codex/rules/*.rules`, treat Codex command policy as session-scoped. Do not assume newly allowed commands are available until policy reload or a new session; when the active policy still requires a prompt and approvals are unavailable (for example `Approval policy: never`), record the blocker instead of retrying or weakening rules.

5. Run or refresh validation before opening the PR. If the relevant gate already ran immediately before the commit and no files changed afterward, cite that evidence; otherwise run the appropriate tier now. For C++/runtime/build/packaging/public-contract slices, the default full gate is:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Run `tools/build.ps1` separately only when the slice or release evidence explicitly needs a standalone build log beyond `validate.ps1`. For documentation-only or other narrow non-runtime slices, use the narrower tier described above and include the justification in the PR body. Do not use the narrow tier for C++, build scripts, packaging, runtime assets, agent manifests, or validation policy changes unless the matching static checks prove the touched surface.

### Hosted PR Check Selection

Use an always-running required gate for branch protection. Path-filtered workflows must not be branch-protection-required directly because GitHub can leave skipped checks pending; if hosted cost needs reduction, keep heavy jobs conditional behind a required aggregate gate or use non-required supplemental workflows.

The `Validate` workflow implements this with `changes` (`Select PR validation tier`) and `pr-gate` (`PR Gate`). `changes` always runs and delegates PR file-diff classification to `tools/classify-pr-validation-tier.ps1`; `tools/check-ci-matrix.ps1` verifies docs-only, static-policy, runtime, workflow, and non-PR classifications. `pr-gate` always runs after the matrix and is the stable aggregate check intended for branch protection.

Run the full hosted matrix for `main` push, release, scheduled/nightly, and `workflow_dispatch` runs. For PRs, choose the cheapest tier that proves the touched surface:

- **Docs/agent/rules/subagent-only:** the hosted `agent-static` job keeps the required gate meaningful with PR diff whitespace and `tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120`, including formatting, agents, JSON contracts, AI integration, CI matrix, public API, package artifact, production-readiness, and host-gate static checks. Do not run `Windows MSVC`, `macOS Metal CMake`, or `Full Repository Static Analysis` just because instructions, skills, rules, settings, subagents, or prose docs changed.
- **CI/build/tooling policy:** run the matching static guard and the affected hosted lane(s). A workflow edit that changes macOS setup should prove macOS; a clang-tidy policy change should prove static analysis; a Windows runner/toolchain change should prove Windows.
- **Runtime/build behavior:** run the relevant build/test/static lanes for changes under `CMakeLists.txt`, `CMakePresets.json`, `cmake/`, `vcpkg.json`, build/test/package scripts, `engine/`, `editor/`, `games/`, packaging, runtime assets, public headers, or public/runtime contracts.
- **Windows MSVC cache refresh:** selected Windows build lanes restore the `external/vcpkg` tool checkout, vcpkg package/install caches, and the newest compatible build tree with `actions/cache/restore`; restore transport failures are non-blocking and fall back to cloning/bootstrap/build work. After validation succeeds, the lanes attempt vcpkg/tool/build cache refreshes with non-blocking `actions/cache/save`. Keep configure/dependency/toolchain inputs in the restore prefix so stale object trees are rebuilt by MSBuild rather than trusted as validation evidence; cache transport or archive failures must not make validation red.

The PR body must record the selected tier, exact commands/checks, and why omitted heavy lanes are unrelated. If branch protection needs a single stable required result, put the tier decision behind the always-running aggregate gate and keep heavy jobs conditional or non-required.

Keep required job names unique across workflows. If branch protection or merge queue is enabled, the required check surface must stay stable while the underlying heavy lanes can evolve behind the aggregate gate.

6. Do not push directly to the default branch or protected branches; that is outside the official GitHub Flow publishing path. Do not use `--force`; use `--force-with-lease` only when the user explicitly requests history rewriting and the branch is known to be task-owned.

7. Prefer a GitHub pull request for shared or release-facing work:

```powershell
gh pr create --base <base-branch> --head <branch> --title "<title>" --body "<validation summary>"
```

In unattended Codex sessions, `gh pr create` may run automatically after the branch is pushed and the validation evidence is ready. Use `--draft` for large slices or early feedback; create a ready PR only when the work is ready for review. The PR body must include actual validation evidence or blockers, and the head branch must be task-owned.

The PR can also be created or updated through GitHub Web or GitHub Desktop. If authentication, branch protection, required reviews, required status checks, or remote permissions block the push or PR, report the blocker and stop instead of bypassing policy or asking to weaken safeguards.

Push and PR publishing depend on host-local GitHub authentication such as Git Credential Manager, GitHub CLI, SSH agent, or a browser session. This repository must not require or store `GITHUB_TOKEN`, personal access tokens, or credential helper state for routine publishing.

When a task-owned draft PR is ready for review, unattended sessions may run the guarded wrapper instead of raw `gh pr ready`:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ready-task-pr.ps1 -PullRequest <pr> -ExpectedBase main
```

The wrapper refreshes the remote, inspects `gh pr view <pr> --json state,isDraft,baseRefName,headRefName,headRefOid,mergeable,mergeStateStatus,reviewDecision,statusCheckRollup,autoMergeRequest,url`, and only invokes `gh pr ready` when the local worktree is clean, the current branch is `codex/` or `codex-` task-owned, local/remote/PR heads match, the PR is open and still draft, the base branch is expected, `mergeable=MERGEABLE`, `mergeStateStatus=CLEAN`, no review requested changes, no failed or pending checks, and `PR Gate` reports `SUCCESS`. If any preflight fails, stop and report the blocker; do not call raw `gh pr ready` in an approval-free session.

8. Merge only after branch protection, required checks, and required reviews are satisfied. Before any unattended merge or auto-merge registration, inspect the live PR state:

```powershell
gh pr view <pr> --json state,isDraft,baseRefName,headRefName,headRefOid,mergeable,mergeStateStatus,reviewDecision,statusCheckRollup,autoMergeRequest,url
```

Continue only when the local worktree is clean, the PR is open, not draft, targets the expected base branch, uses a task-owned head branch, has fresh local validation evidence, is not conflicting, has no requested changes, and has no `FAILURE`, `CANCELLED`, `TIMED_OUT`, or `ACTION_REQUIRED` status/check conclusion. Treat `DIRTY` and `UNKNOWN` `mergeStateStatus` values as blockers. Re-run this preflight after every commit or push; a stale `headRefOid` invalidates the merge decision. For runtime/C++/build/toolchain/public-contract PRs, the latest head must have no selected hosted check in `IN_PROGRESS`, `QUEUED`, or missing state, and `PR Gate` must report `SUCCESS`; do not register auto-merge while `Full Repository Static Analysis` or another selected build/static lane is still pending. For docs-only PRs, a pending review requirement may still block auto-merge registration, but ambiguous or incomplete hosted checks are blockers.

For unattended Codex sessions, prefer GitHub auto-merge registration so GitHub performs the final `main` merge only after requirements are met. The command must include `--match-head-commit <headRefOid>` from the immediately preceding PR state query so a later push cannot be merged by the earlier decision:

```powershell
gh pr merge --auto --merge --match-head-commit <headRefOid>
```

Run the merge command from the task branch whose PR was preflighted, but do not pass `--delete-branch` in linked-worktree sessions. GitHub CLI deletes the local and remote branch for that flag, and Git refuses local branch deletion while the branch is checked out by another linked worktree. Leave branch cleanup to the guarded repository script after merge evidence exists. GitHub auto-merge must be enabled for the repository and PR; if `gh` reports auto-merge is unavailable, if branch protection or required checks are absent in a way that would merge pending/failing work immediately, or if the PR state is ambiguous, stop and report the blocker. Immediate merge commands such as `gh pr merge --merge --delete-branch`, `--squash`, `--rebase`, or `--admin` are not part of the unattended agent path and remain prompt-gated unless the user explicitly requests them in an approval-capable session with fresh passing evidence.

After merge, verify what actually reached `main` instead of relying on the old PR object. Run `git fetch origin --prune`, confirm the checkout is on the expected base, and compare the preflighted head against `origin/main`:

```powershell
git status --short --branch
git log --oneline origin/main..<headRefOid>
```

The second command should be empty. If it is not empty, or if a commit was pushed to the same head branch after the PR had already merged, do not assume the merged PR absorbed it; open or update a separate task-owned PR for the remaining commits. Local and remote branch cleanup must wait until this reachability check passes.

9. After the PR head is reachable from `origin/main`, run guarded post-merge branch and worktree cleanup for task-owned linked worktrees that are no longer needed. The cleanup command fetches and prunes the remote, fast-forwards the local checkout on the base branch, verifies local and remote branch ancestry, then removes the merged worktree and selected branches. GitHub `delete_branch_on_merge` is enabled on this repository, so merged PR head branches are removed on GitHub automatically; local cleanup still must run through the guarded script path. Prefer:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/post-merge-task-cleanup.ps1 -WorktreePath <path> -HeadRefOid <headRefOid>
```

Equivalent lower-level entrypoint:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1 -WorktreePath <path> -BaseRef origin/main -BaseBranch main -Remote origin -DeleteLocalBranch -DeleteRemoteBranch
```

`tools/remove-merged-worktree.ps1` is the automatic cleanup path for merged work under standard `.worktrees/` or `.claude/worktrees/` roots for the script checkout, `-LocalCheckoutPath`, or the Git main worktree record reported by `git worktree list --porcelain`: it runs `git fetch --prune <remote>`, requires the local checkout to be from the same repository, clean, attached to `-BaseBranch`, and fast-forwardable to `-BaseRef`, then refuses the current/default worktree, locked or detached worktrees, dirty status, unknown worktree records, and branches not proven merged into `-BaseRef`. With `-DeleteRemoteBranch`, it also requires the remote-tracking branch to match the local branch commit and be merged into `-BaseRef`; if the remote branch is already absent after GitHub cleanup, it reports that as already handled. Before invoking `git worktree remove`, it unlinks worktree-local `external/vcpkg` and `vcpkg_installed` reparse points so Git cannot traverse into shared ignored dependency/tool trees. On Windows, if `git worktree remove` fails only because of long paths, the script may use its internal long-path deletion fallback after these safety checks without following reparse points, then prune worktree metadata. If the local checkout is dirty, on another feature branch, detached, or diverged, stop and report the blocker; do not stash, merge into an active feature branch, force-delete, or copy files by hand.

### Hosted PR Check Failure Triage

When a hosted PR check fails, inspect the latest PR head before editing:

```powershell
gh pr view <pr> --json headRefOid,statusCheckRollup,url
```

Open the failing job log for that `headRefOid`, reproduce the narrowest matching local lane, and fix the root cause. If checkout itself fails with a transient GitHub fetch/auth error, keep `permissions: contents: read` and `persist-credentials: false`, use the reviewed retry-checkout contract guarded by `tools/check-ci-matrix.ps1`, and avoid PATs or broader workflow permissions. If the root cause is a repository contract that can drift, add or extend a static guard in the same slice. If all jobs fail before checkout with a GitHub account billing/spending-limit annotation, report it as a hosted account blocker. Do not diagnose against stale workflow runs, loosen branch protection, or broaden Codex/Claude command permissions to make the PR merge.

For hosted `static-analysis` failures, use the repository `tools/check-tidy.ps1` path first. Keep `.clang-tidy` `HeaderFilterRegex` compatible with absolute Windows/Linux compile-database paths, keep strict CI on `--warnings-as-errors=*`, treat unsupported checks as hosted toolchain-version drift, and only suppress `NN warnings generated.` summary lines after real diagnostics and exit codes are preserved.

If Git prints credential helper warnings such as `git: 'credential-manager-core' is not a git command`, inspect all helper sources first:

```powershell
git config --show-origin --get-all credential.helper
git credential-manager --version
```

On Git for Windows, the current Git Credential Manager helper is `manager`. Remove stale user-level helper entries such as `manager-core` only after confirming `manager` is still configured by system or user Git config. Do not commit repository-level `credential.helper` overrides, token requirements, or checked-in credential state to hide host configuration drift.

These rules follow the Git documentation for `.gitignore`, `$GIT_DIR/info/exclude`, `core.excludesFile`, and [`git worktree remove` / `git worktree prune`](https://git-scm.com/docs/git-worktree), and GitHub documentation for pull requests, [draft PR stage changes](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/changing-the-stage-of-a-pull-request), [protected branches](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches), [auto-merge](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/incorporating-changes-from-a-pull-request/automatically-merging-a-pull-request), [`gh pr ready`](https://cli.github.com/manual/gh_pr_ready), and [`gh pr merge`](https://cli.github.com/manual/gh_pr_merge). For linked worktree metadata, see Git's [`git worktree`](https://git-scm.com/docs/git-worktree) and [repository layout](https://git-scm.com/docs/gitrepository-layout) documentation. For the branch plus PR workflow, see GitHub's official [GitHub flow](https://docs.github.com/en/get-started/using-github/github-flow). For credential helpers, see GitHub's [credential caching guidance](https://docs.github.com/en/get-started/git-basics/caching-your-github-credentials-in-git?platform=windows) and the Git [gitcredentials documentation](https://git-scm.com/docs/gitcredentials.html). For Codex approval modes, use OpenAI's official Codex guidance: Full Auto is sandboxed and network-disabled, so publication requires a mode/session whose filesystem, network, and host authentication cover the publishing path.

## Windows Diagnostics Toolchain

- Use Debugging Tools for Windows from the official Windows SDK for native crash, dump, and debugger PATH investigations. Verify with `cdb -version`; configure Microsoft public symbols through `_NT_SYMBOL_PATH=srv*C:\Symbols*https://msdl.microsoft.com/download/symbols`.
- Use Windows Graphics Tools (`Tools.Graphics.DirectX~~~~0.0.1.0`) when a D3D12 task depends on the debug layer. Verify `d3d12SDKLayers.dll` under both `C:\Windows\System32` and `C:\Windows\SysWOW64` on Windows hosts.
- Use PIX on Windows for D3D12 GPU captures, timing, and counter work. Verify the command-line tool with `pixtool --help`. Optional repository helper: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/launch-pix-host-helper.ps1` creates a non-repository `%LocalAppData%` scratch directory and starts the PIX UI (`WinPix.exe` under `Program Files\Microsoft PIX\<version>\`, or legacy `PIX.exe`) when installed; use `-SkipLaunch` to verify resolution only. **Default AI + operator split:** follow `docs/ai-integration.md` § **Recommended workflow (operator PIX, AI analysis)**—agent bundles CLI check and helper steps; operator runs capture; agent analyzes pasted or attached evidence.
- Use Windows Performance Toolkit for ETW, CPU, and system performance work. Verify with `wpr -help` and `xperf -help`; open traces in `wpa.exe` when a GUI trace review is required.
- For first-party diagnostics operations handoff, use `mirakana::build_diagnostics_ops_plan` over a `DiagnosticCapture` to produce a `DiagnosticsOpsPlan`: summary and Chrome Trace Event JSON rows are ready through `summarize_diagnostics` and `export_diagnostics_trace_json`, pasted supported Trace Event JSON can be reviewed through `review_diagnostics_trace_json`, native crash dump review is host-gated by Debugging Tools for Windows, and telemetry upload stays unsupported unless a caller-provided backend is explicitly configured.
- Treat Debugging Tools for Windows, Windows Graphics Tools, PIX on Windows, and Windows Performance Toolkit as host diagnostics. They are not required for the default build, are not installed by CMake configure, and are blockers only for tasks that explicitly need native debugging, D3D12 debug-layer validation, GPU capture, or ETW/performance evidence.
- After Machine `PATH` or Machine `_NT_SYMBOL_PATH` changes, open a new terminal before rerunning PATH-based checks. Existing shells can keep stale environment blocks.

## Native Editor GUI Validation Lane

- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` before optional vcpkg-backed dependency lanes when `vcpkg_installed/` is missing. Bootstrap is the only supported dependency acquisition path; vcpkg-backed CMake presets set `VCPKG_MANIFEST_INSTALL=OFF` and consume `${sourceDir}/vcpkg_installed` without configure-time package installation.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1` as the supported validation lane for the optional Windows-native `MK_editor` shell. It configures, builds, and tests the dependency-free `desktop-editor` preset, including `MK_editor_native_shell_tests` and deterministic `MK_editor_smoke` counters for Win32/first-party retained UI/Direct3D 12 host readiness, core-backed panels, diagnostic-only viewport display, and diagnostic-only material preview.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1 -Editor` for the `cpp23-desktop-editor-eval` C++23 editor lane. Keep it aligned with `tools/build-editor.ps1` and the `desktop-editor` validation recipe.
- GUI validation is Windows-native editor/developer-shell tooling only. It does not claim macOS/Linux visible editor parity, runtime game UI middleware readiness, SDL3 restoration, or public native handle exposure.

## C++ Standard

- The required language baseline is C++23.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1` after changing CMake standard policy, manifests, schemas, or AI guidance.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1 -Release` for release packaging. Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1 -Editor` for the native `cpp23-desktop-editor-eval` editor lane. The script uses automatic CMake/CTest parallelism by default; pass `-Jobs <N>` only to throttle a constrained host.
- Add C++ modules through CMake `FILE_SET CXX_MODULES`; do not bypass CMake module scanning.
- Keep CMake module scanning off for non-module test/probe/sample/game executables; engine/library targets keep the preset-level module scan policy.
- Use `import std;` only when CMake reports C++23 standard-library module support for the active generator/toolchain.

## Subagent Use

Use subagents only when the user explicitly asks for subagent delegation or parallel agent work. Use them for independent work:

- `agent-surface-auditor`: read-only Codex/Claude/Cursor guidance, manifest, rules, and validation-policy drift audit
- `explorer`: read-only codebase exploration
- `cpp-reviewer`: C++ lifetime, ownership, and API review
- `build-fixer`: build/test failure triage
- `engine-architect`: read-only architecture exploration and scoped design review
- `gameplay-builder`: C++ sample game or gameplay implementation against public APIs
- `planning-auditor`: read-only plan lifecycle audit for plan necessity, active plan alignment, and production gap sequencing
- `rendering-auditor`: rendering/RHI/shader changes

Use fixed Codex model defaults only where the role's cost/quality profile is stable: `explorer` and `agent-surface-auditor` use `gpt-5.4-mini` with medium reasoning, and `cpp-reviewer`, `engine-architect`, `planning-auditor`, and `rendering-auditor` use `gpt-5.5` with high reasoning. Leave write-capable `build-fixer` and `gameplay-builder` on inherited models unless the parent uses a per-invocation override for a specific delegated task. Keep immediate blocking work in the parent session and delegate only independent side work with clear ownership. Close completed, obsolete, or no-longer-needed delegated agents promptly after consuming their final output and before starting replacements; do not wait for the subagent limit to force cleanup. Keep agents that are still doing useful independent work.

## Worktree And Parallel Agent Workflow

- Prefer native product worktree support before manual Git commands: Codex app Worktree/Handoff for Codex threads, and Claude Code `--worktree` or subagent `isolation: worktree` for Claude sessions that need file isolation.
- Use worktrees for parallel write-capable sessions so Local stays the foreground checkout. Keep immediate blocking work local, split parallel work by file ownership, and avoid two agents editing the same file.
- `.worktrees/` and `.claude/worktrees/` are ignored repository-local worktree roots. After entering a manual linked worktree, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1`; it verifies required ignore rules, links an existing local `external/vcpkg` checkout, and links an existing local `vcpkg_installed/` package tree when available so preset configure does not download dependencies or duplicate Microsoft vcpkg artifacts. Do not copy secrets into tracked files; use `.worktreeinclude` only for gitignored local files that an operator intentionally wants copied into new Claude worktrees.
- Manual fallback is `git worktree list`, `git worktree add <path> -b <branch> [<base>]`, `git worktree remove <path>`, and `git worktree prune`. Treat raw remove/prune/repair as cleanup operations that require status inspection; do not delete worktree directories with `Remove-Item`. For merged task work, prefer `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1 -WorktreePath <path> [-BaseRef origin/main] [-BaseBranch main] [-Remote origin] [-LocalCheckoutPath <path>] [-DeleteLocalBranch] [-DeleteRemoteBranch]` so fetch/prune, local checkout fast-forward, branch merge, clean status, standard worktree-root checks from official porcelain records, local/remote branch ancestry checks, worktree-local dependency reparse-point unlinking, and non-reparse-following Windows long-path fallback handling run inside the guarded script before deletion.

## Agent Surface Governance

- Keep Codex, Claude Code, and Cursor behavior synchronized through `AGENTS.md`, `CLAUDE.md`, `.agents/skills/`, `.codex/agents/`, `.codex/rules/`, `.claude/settings.json`, `.claude/rules/`, `.claude/skills/`, `.claude/agents/`, `.cursor/rules/`, `.cursor/skills/`, `.cursor/agents/`, `engine/agent/manifest.fragments/` + composed `engine/agent/manifest.json`, and `tools/check-ai-integration.ps1`.
- Treat the agent-surface drift check as a required part of implementation work. Do not defer stale guidance, stale manifest claims, stale rules, stale subagent instructions, or repeated agent failure guidance to a separate follow-up when the current change exposes the mismatch.
- Keep drift checks targeted: compare the changed behavior/API/workflow against owning surfaces, and do not load every agent surface when the change is local and no durable guidance changed.
- Use `agent-surface-auditor` only for bounded read-only drift audits; it reports affected paths and validation commands, while the parent agent remains responsible for edits, integration, and final validation.
- Use `planning-auditor` only for bounded read-only plan lifecycle audits; it recommends whether to create a plan, update the active plan, or proceed without a plan file, while the parent agent remains responsible for edits and validation.
- Use the OpenAI developer documentation MCP, or official OpenAI documentation when MCP is unavailable, for OpenAI API, Codex, ChatGPT Apps SDK, OpenAI agent, and OpenAI model behavior. Use official Anthropic documentation for Claude Code memory, settings, permissions, hooks, skills, and subagents.
- Keep always-loaded instructions specific, concise, verifiable, and durable. Keep `AGENTS.md` under Codex's default 32 KiB `project_doc_max_bytes` budget, keep selected `SKILL.md` bodies as concise routers, and keep subagents narrowly scoped; put long procedures in skill-local `references/*.md` or docs, path-specific guidance in rules, specialized behavior in subagents, and machine-readable capability/status claims in the composed `engine/agent/manifest.json` (edit `engine/agent/manifest.fragments/*.json`, then `tools/compose-agent-manifest.ps1 -Write`).
- Keep Codex project rules narrow with `match` / `not_match` examples. Cover Windows PowerShell deletion/network/host-servicing commands as well as POSIX-like spellings. Do not add broad allow rules for shells, package managers, network tools, destructive commands, force-pushes, or immediate PR merges; direct default-branch pushes must stay forbidden.
- Keep Claude Code shared project permissions in `.claude/settings.json` with the official JSON schema: deny secret-bearing files and direct default-branch pushes, allow read-only PR preflight, task-owned PR creation, guarded `tools/ready-task-pr.ps1` draft-to-ready conversion, auto-merge registration after validation checkpoints plus official GitHub Flow PR preflight, and the guarded merged-worktree cleanup script; require approval for raw destructive, network, dependency-bootstrap, mobile signing/smoke, force-push, raw `gh pr ready`, and non-auto PR state-change commands.
- Keep local override and credential-bearing config uncommitted: `.claude/settings.local.json`, `.mcp.json`, and `AGENTS.override.md`.

## Repository consistency checklist (recommended)

Run these periodically—especially before merge or after touching agent surfaces, public headers, manifests, or validation scripts—to keep Cursor, Codex, Claude Code, and machine-readable contracts aligned:

1. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` when CMake, formatters, or PATH resolution is in doubt. In a manual linked worktree, first run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` so ignored worktree roots and `external/vcpkg` are ready before configure.
2. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` — validates `AGENTS.md` stays under 32 KiB, selected Codex/Claude/Cursor `SKILL.md` files stay within the 24 KiB initial-load budget, Codex/Claude/Cursor subagent instructions stay within the 16 KiB delegated-context budget, `tools/*.ps1` UTF-8 no BOM, `#requires` pairs, PowerShell parse errors, skill/agent frontmatter, **Codex ↔ Claude `gameengine-*` skill folder twins**, **Cursor thin-pointer folders** (45-line router budget except `gameengine-cursor-baseline` / `gameengine-plan-registry`), **five scoped `.cursor/rules/*.mdc` files**, and Cursor project subagent parity with shared roles across `.codex/agents/*.toml`, `.claude/agents/*.md`, and `.cursor/agents/*.md` (see `claudeToCodexSkillMap` in `tools/check-agents.ps1`). When adding `.claude/skills/gameengine-<topic>/`, register the matching `.agents/skills/<codex-folder>/` name in that map and add or update the thin `.cursor/skills/gameengine-<topic>/SKILL.md` router unless the skill is Cursor-only (`gameengine-cursor-baseline`, `gameengine-plan-registry`). Include **`gameengine-git-publication-preflight`** when touching GitHub Flow guidance. When adding shared subagent roles, keep `.codex/agents/`, `.claude/agents/`, `.cursor/agents/`, and `engine/agent/manifest.fragments/011-aiSurfaces.json` aligned. When authoring or refactoring `tools/*.ps1`, name `function` cmdlets with [PowerShell approved verbs](https://learn.microsoft.com/powershell/scripting/developer/cmdlet/approved-verbs-for-windows-powershell-commands) so local **PSScriptAnalyzer** rules such as `PSAvoidUsingUnapprovedVerbs` stay satisfied; also follow **`AGENTS.md` → Repository command entrypoints** for automatic-variable hygiene (`$input`, `$matches`, `$Is*`), including **never using `$input` as a `foreach` iterator name** and **never binding `[Regex]::Matches` results to `$matches`**, plus **`$null =`** discard-only calls, **`Write-Information -InformationAction Continue`** instead of **`Write-Host`** for non-pipeline status, non-empty **`catch`**, and **`ShouldProcess`** on host-mutating helpers. Static contract entrypoints (`tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`) should stay thin dispatchers; share cross-entry assertions in `tools/static-contract-common.ps1`, add/rename numeric chapter files under `tools/`, and keep line budgets in `tools/static-contract-ledger.ps1` so dispatcher lists stay data-driven without hand-listed chapter rosters. Optionally run **`Invoke-ScriptAnalyzer`** on touched scripts when the module is installed.
   For PR branches with multiple commits already pushed, especially docs/plan churn, also run `git diff --check origin/main...HEAD` before hosted re-runs and before guarded ready/merge preflight. `git diff --cached --check` proves only the current staged slice, and `tools/check-text-format.ps1` / `tools/format-text.ps1` do not replace that PR-wide whitespace guard.
3. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` after manifest, schema, game agent JSON, shared skill content, or **new Needles** enforced there change (including retained editor UI ids, manifest slices, editor-shell literals, or other strings added to a `check-ai-integration-*` / `check-json-contracts-*` chapter). For literal Needle strings with Markdown backticks, use single-quoted PowerShell strings so backticks stay literal. Keep chapter ownership honest (details in `.agents/skills/gameengine-agent-integration/references/static-contract-chapters.md`): `tools/check-ai-integration-010-agent-baseline.ps1` and `tools/check-json-contracts-040-agent-surfaces.ps1` cover agent/docs/plan/tooling baseline needles only; load reusable `Get-AgentSurfaceText` for engine/RHI/runtime/renderer contracts in `tools/check-ai-integration-020-engine-manifest.ps1` and `Get-JsonContractSurfaceText` for runtime UI/tools C++ contracts in `tools/check-json-contracts-050-generated-games.ps1` instead of preloading them in chapter 1 or 4. Durable changes to `engine/agent/manifest.json` must go through `engine/agent/manifest.fragments/` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`; `tools/check-json-contracts.ps1` enforces semantic parity via `compose-agent-manifest.ps1 -Verify`.
4. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` after editing public headers or backend interop.
5. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` only at coherent C++/runtime/build/packaging/public-contract slice boundaries, or when narrower checks cannot prove the changed surface.
6. After documenting or automating **worktree setup or cleanup**, keep guidance aligned with `AGENTS.md`: **`external/vcpkg` is a required vcpkg tool checkout** (preset toolchain path)—do not treat it like `out/`; `vcpkg_installed/` may be linked by `tools/prepare-worktree.ps1` and removed only when you intend to rerun `tools/bootstrap-deps.ps1`. Use `tools/prepare-worktree.ps1` for linked-worktree setup and `tools/remove-merged-worktree.ps1` for guarded post-merge local checkout sync plus worktree cleanup; cleanup must unlink worktree-local `external/vcpkg` / `vcpkg_installed` reparse points before Git removal and must not follow their targets. Keep raw `git worktree remove` / `git worktree prune` prompt-gated.

This checklist does not replace subsystem-specific validation (packaging, mobile, shaders); combine it with **Default Implementation Flow** above.

## Dependency Flow

1. Confirm the dependency is necessary.
2. Prefer permissive licenses.
3. Record the dependency in `THIRD_PARTY_NOTICES.md`.
4. Keep third-party code isolated.
5. Add validation for the integration.

## CI

See [testing.md](testing.md) for the **CI validation matrix** (job ids, runners, sanitizer env). Summary:

GitHub Actions runs:

- Windows PR lane: `tools/validate.ps1 -SkipStaticChecks -SkipTidySmoke` after `Agent Static Guards` and `Full Repository Static Analysis` cover static evidence; local/default full validation remains `tools/validate.ps1`
- Windows C++23 release: `tools/evaluate-cpp23.ps1 -Release`; D3D12 unit tests use Microsoft WARP for driver-independent CI evidence.
- Linux: `cmake --preset ci-linux-clang`, build/CTest
- Linux coverage: `tools/check-coverage.ps1 -Strict`
- Linux sanitizers: `cmake --preset clang-asan-ubsan`, build, and CTest
- macOS: `tools/validate-renderer-metal-apple.ps1 -Jobs <logical-cpu-count>` for the reviewed Metal host-evidence recipe, then `cmake --preset ci-macos-appleclang`, build/CTest, including Apple-only Metal Objective-C++ sources
- iOS Validate: `tools/validate-apple-metal-platform-host.ps1 -Platform ios -RequireReady` on a pinned `macos-26` hosted runner, delegating to `tools/smoke-ios-package.ps1` to build the iOS Simulator bundle and run `xcrun simctl install`, `get_app_container`, `launch`, app-written Metal evidence-file checks, and cleanup for `sample_headless`

Local Linux validation should use the CI preset with Ninja and Clang:

```bash
cmake --preset ci-linux-clang
cmake --build --preset ci-linux-clang --parallel "$(nproc)"
ctest --preset ci-linux-clang --output-on-failure --parallel "$(nproc)"
```

CI uploads diagnostic test logs after failed validation steps, Linux coverage diagnostics after failed coverage steps, and Windows release package ZIP artifacts after successful release evaluation. The Windows release evaluation installs the package and builds `examples/installed_consumer` against the installed `mirakana::` CMake targets before publishing the ZIP.

C++23 verification must stay covered by default validation, `tools/check-generated-msvc-cxx23-mode.ps1`, and release/editor-specific checks.

macOS CI is defined for Metal host coverage. Local macOS validation should mirror the hosted lane:

```bash
cmake --preset ci-macos-appleclang
cmake --build --preset ci-macos-appleclang --parallel "$(sysctl -n hw.logicalcpu)"
ctest --preset ci-macos-appleclang --output-on-failure --parallel "$(sysctl -n hw.logicalcpu)"
```

When hard Apple/Metal evidence is required, run the reviewed renderer recipe first:

```powershell
$jobs = [int](& sysctl -n hw.logicalcpu)
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-renderer-metal-apple.ps1 -Jobs $jobs
```

## Mobile Packaging

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1` is included in default validation as a diagnostic-only gate. It verifies Android/iOS template presence and reports Android SDK, Android NDK, JDK, Gradle, `adb`, `apksigner`, Android Emulator, configured Android AVDs, connected device/emulator smoke readiness, macOS, full Xcode selection, `xcrun`, iPhoneOS/iPhone Simulator SDK availability, iOS Simulator runtime availability, Android Release signing, and Apple signing diagnostics without claiming package readiness. Blocker lines include stable `kind=<slug>` fields for agent/CI classification. Its Android Emulator `-list-avds` probe is bounded so a hosted or local static validation lane reports `probe-timeout` instead of hanging on host tool state. The `adb devices` probe is skipped in GitHub Actions static validation unless `-RequireAndroid` or `MK_MOBILE_DEVICE_PROBE=1` explicitly requests a live device probe, reporting `android-device-smoke=probe-skipped-ci` instead of starting the official adb background server. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1` adds the Apple Metal iOS Host Evidence v1 view over the same boundary plus Xcode-resolved `metal`/`metallib` readiness and repository iOS/macOS workflow coverage; on non-Apple hosts it should report `apple-host-evidence-check: host-gated` without failing default validation.

Android package attempts use `tools/build-mobile-android.ps1`, the `platform/android` Gradle template, Android Gradle Plugin 9.1.0, Gradle 9.3.1, JDK 17, Android SDK Platform 36.1, Build Tools 36.0.0, NDK 28.2.13676358, Android SDK CMake 4.1.2, Prefab, AndroidX AppCompat, AndroidX Core, GameActivity, the NDK Vulkan loader, and the NDK AAudio platform library. The template packages `games/<game_name>/game.agent.json` under app assets, copies `games/<game_name>/assets` when present, restricts the native package build to `mirakanai_android`, creates Android Vulkan surfaces through a private `VK_KHR_android_surface` entry point, and starts/stops a private low-latency AAudio float32 output stream with lifecycle events. Release builds require `MK_ANDROID_KEYSTORE`, `MK_ANDROID_KEYSTORE_PASSWORD`, `MK_ANDROID_KEY_ALIAS`, and `MK_ANDROID_KEY_PASSWORD`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-android-release-package.ps1 -UseLocalValidationKey` can generate a local non-repository PKCS12 upload key, build Release, export the upload certificate, and verify the APK with `apksigner`. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-android-package.ps1 -Configuration Release -SkipBuild -StartEmulator -AvdName Mirakanai_API36` normalizes `ANDROID_AVD_HOME` to the user Android AVD directory when needed, starts the local API 36 emulator, installs the APK, launches `MirakanaiActivity`, verifies the app process, and stops the app/emulator. Gradle and emulator lanes write user SDK/cache/AVD state, so sandboxed agents must run those commands with the appropriate local approval rather than treating user-cache write failures as engine failures.

Android Release Device Matrix v1 current evidence uses `sample_headless` on the Windows Android-ready host: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-android.ps1 -Game sample_headless -Configuration Debug`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-android-release-package.ps1 -Game sample_headless -UseLocalValidationKey`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-android-package.ps1 -Game sample_headless -Configuration Release -SkipBuild -StartEmulator -AvdName Mirakanai_API36` passed. This proves the template can build Debug/Release APKs, verify a locally signed Release APK, export the upload certificate, and install/launch on the API 36 emulator; it does not claim Play upload, production signing material, physical-device coverage, broader ABI/device matrix coverage, Apple/iOS readiness, or repository-owned keys.

Apple package attempts use `tools/build-mobile-apple.ps1` and the `platform/ios` CMake/Xcode bundle template. The template packages `games/<game_name>/game.agent.json` into bundle resources, creates a Metal-backed UIKit view, compiles `IosMetalEvidence.metal` through `xcrun metal` / `xcrun metallib` into bundled `default.metallib`, maps Application Support/Caches/Documents into first-party save/cache/shared storage roots, supports `-Platform Simulator|Device`, disables simulator signing when no team is supplied, and accepts `MK_IOS_BUNDLE_IDENTIFIER`, `MK_IOS_DEVELOPMENT_TEAM`, and `MK_IOS_CODE_SIGN_IDENTITY` or the matching script parameters for Xcode signing. The Apple package script configures the package tree with `BUILD_TESTING=OFF` and builds only the `MirakanaiIOS` bundle target, keeping iOS smoke focused on the app package instead of Xcode `ALL_BUILD` unit-test targets. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-ios-package.ps1 -Game sample_headless -Configuration Debug` builds the Simulator bundle, selects an available iPhone Simulator, boots it when needed, installs the app, verifies the app container, launches the bundle, reads `Library/Caches/mirakanai_ios_metal_evidence.txt` from the app data container, requires `ios_metal_feature_set_checked=1`, `ios_metal_command_queue_ready=1`, `ios_metal_pipeline_ready=1`, `ios_metal_command_buffer_ready=1`, and `ios_metal_readback_ready=1`, terminates the app, and shuts down only the simulator booted by the script. These scripts validate `games/<game_name>/game.agent.json` before building.

Apple Metal iOS Host Evidence v1 current Windows evidence is host-gated: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1` reports `host=windows`, `xcode=blocked`, `ios-simulator=blocked`, `metal-library=blocked`, missing `xcodebuild`, missing `xcrun`, missing iOS SDK/runtime access, missing `metal`, missing `metallib`, and present iOS Simulator/macOS Metal workflow coverage. `tools/validate-apple-metal-platform-host.ps1 -Platform ios -RequireReady` is the strict iOS Metal platform gate: it runs only on macOS/full-Xcode hosts, is required by the `validate.yml` `ios-metal` job, and requires the iOS smoke evidence-file counters before `environment_platform_ios_metal_ready=1`. Run `tools/check-apple-host-evidence.ps1 -RequireReady` only on a macOS/full-Xcode host when the task requires hard Apple-ready evidence.
