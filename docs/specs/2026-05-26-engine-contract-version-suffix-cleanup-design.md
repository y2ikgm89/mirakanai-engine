# Engine Contract Version Suffix Cleanup Design (2026-05-26)

## Status

Accepted narrow specification.

## Goal

Before the first public release, remove removable `vN` / `VN` / `_vN` suffixes from current MIRAIKANAI engine-owned contracts so the first shipped API and saved-data surface uses canonical names rather than pre-release generation names.

## Context

The repository is still greenfield and explicitly does not preserve backward compatibility unless a future release policy requires it. Current engine contracts contain many pre-release generation suffixes:

- persisted text formats such as `GameEngine.Scene.v2`, `GameEngine.AssetIdentity.v2`, `GameEngine.CookedPackageIndex.v1`, and `GameEngine.RuntimeInputActions.v4`
- public C++ symbols such as `AssetKey`, `SourceAssetRegistryDocument`, `SceneDocument`, `RuntimeResourceCatalog`, and `compile_frame_graph`
- source filenames such as `schema.hpp`
- AI command schema names such as `GameEngine.AiCommand.CreateGameRecipe.Request.v1`
- dynamic editor ABI names such as `GameEngine.EditorGameModuleDriver.v1` and `mirakana_create_editor_game_module_driver_v1`
- current-truth docs, manifest fragments, JSON schemas, and static checks that enforce those names

Those suffixes were useful during rapid pre-release iteration, but they create the wrong external contract for a first release. The desired release posture is: the current contract is the canonical contract. Future incompatible release work can introduce a deliberate migration policy then, not before the first release.

## Decision

Remove version suffixes from current engine-owned contract names wherever the suffix is acting as a pre-release generation marker.

Examples of canonical replacements:

| Current | Canonical |
| --- | --- |
| `GameEngine.Scene.v2` | `GameEngine.Scene` |
| `GameEngine.CookedPackageIndex.v1` | `GameEngine.CookedPackageIndex` |
| `GameEngine.RuntimeInputActions.v4` | `GameEngine.RuntimeInputActions` |
| `GameEngine.AiCommand.CreateGameRecipe.Request.v1` | `GameEngine.AiCommand.CreateGameRecipe.Request` |
| `AssetKey` | `AssetKey` |
| `SourceAssetRegistryDocument` | `SourceAssetRegistryDocument` |
| `SceneDocument` | `SceneDocument` |
| `RuntimeResourceCatalog` | `RuntimeResourceCatalog` |
| `asset_id_from_key` | `asset_id_from_key` |
| `compile_frame_graph` | `compile_frame_graph` |
| `schema.hpp` | `schema.hpp` |
| `mirakana_create_editor_game_module_driver_v1` | `mirakana_create_editor_game_module_driver` |

Do not add compatibility aliases, fallback parsers, dual writers, or migration shims for the removed names. Repository-owned samples, fixtures, templates, schemas, docs, manifest fragments, static checks, and tests must move in the same candidate as the owning contract.

## Non-Goals And Preserved Version Markers

The cleanup does not remove every textual `v1` in the repository. Preserve suffixes when they are not current engine-owned contract names:

- historical implementation plan, spec, milestone, feature-slice, and master-plan IDs such as `generated-game-studio-v1`, `production-completion-v1`, and dated `*-v1.md` filenames
- historical evidence prose that records what was implemented in the past, unless the prose is also used as current guidance
- JSON numeric fields such as `schemaVersion: 1`, `abi_version = 1`, package schema counters, or release/version fields
- external standards and official names such as Vulkan 1.3, Direct3D 12, C++23, GitHub Actions versions, or package versions
- coordinate and math names where `v` is an axis, such as `uv.v1`, `u1`, `v0`, and `v1`
- public docs that explicitly describe a future compatibility policy rather than the current pre-release engine contract

Historical files may keep their filenames. Current-truth docs must not continue telling users or agents to use removed names.

## Scope

### Persisted Formats

Canonicalize all current repository-owned `format=GameEngine.*.vN` values and matching schema enum literals. This includes asset, source, package, scene, prefab, project, workspace, runtime session, shader, editor evidence, generated package config, AI command, and editor ABI format names.

Old parser branches for `GameEngine.Project.v0` through `.v4`, `GameEngine.RuntimeInputActions.v1` through `.v4`, `GameEngine.Scene.v0` through `.v2`, and similar pre-release generations are removed or converted to one canonical parser path. Tests that currently verify old-format migration must be rewritten to verify canonical-format acceptance and stale-format rejection.

### Public C++ API

Canonicalize removable `V0`, `V1`, `V2`, `V3`, `V4`, `_v0`, `_v1`, `_v2`, `_v3`, and `_v4` markers in current public API and current internal helper names. This includes public headers, source definitions, tests, callers, docs, static checks, and manifest prose.

No compatibility typedefs or forwarding functions are added. If a name is changed, all repository callers move to the canonical name.

### Editor Dynamic ABI

The editor game-module ABI should use canonical symbol and contract names:

- `editor_game_module_driver_abi_name`
- `editor_game_module_driver_factory_symbol`
- `mirakana_create_editor_game_module_driver`
- `GameEngine.EditorGameModuleDriver`

Keep `editor_game_module_driver_abi_version = 1` as a numeric ABI layout check. The numeric value is not a suffix and remains the correct place to express ABI layout compatibility.

### Docs, Manifest, Schemas, And Static Checks

Current docs, manifest fragments, JSON schemas, static checks, and AI-facing guidance must be updated with the same canonical names. `engine/agent/manifest.json` remains generated output only; edit fragments and run `tools/compose-agent-manifest.ps1 -Write`.

Historical evidence remains mostly unchanged unless a static guard relies on a literal as current truth. When a current static guard must retain a historical literal, the literal should point at a current canonical contract or move to the historical archive.

## Candidate Boundaries

Implementation should be serial unless a working linked worktree and disjoint write scopes are available. A prior session could not create a linked worktree because Git metadata writes under `.git/refs/heads/*.lock` and `.git/worktrees/*` were denied; this session confirmed branch/worktree creation is available again. If the blocker returns, stop implementation rather than weakening the design.

Recommended candidate sequence:

1. **Planning and selection**
   - Add this design and the implementation plan.
   - Register the spec and plan in current docs without changing runtime code.
   - Do not change runtime code.
   - Do not switch `currentActivePlan` until implementation work starts on a branch/worktree that can be published.

2. **Saved-format canonicalization**
   - Rename `GameEngine.*.vN` saved formats to canonical `GameEngine.*`.
   - Update serializers, deserializers, package payload validators, source data, generated game templates, sample runtime/source files, schemas, manifest fragments, static checks, current docs, and tests in the same candidate.
   - Remove stale pre-release migration parser branches.

3. **Public C++ contract canonicalization**
   - Rename `*VN` and `*_vN` C++ types/functions/files that expose or implement current contracts.
   - Update all dependent engine, editor, tools, games, tests, CMake references, and public API boundary checks.

4. **Editor ABI and tool-surface canonicalization**
   - Canonicalize editor dynamic ABI symbol names and retained tool/evidence contract ids where they are current operational surfaces.
   - Keep numeric ABI version fields.
   - Update load tests, editor shell usage, manifest fragments, static checks, and current docs in the same candidate.

5. **Agent/docs closeout**
   - Audit current docs, manifest fragments, schema enums, static checks, skills, and generated manifest output after the owning candidates have already been updated.
   - Run the full validation gate and publish the final review evidence.

Each candidate may split into smaller commits when useful, but each PR must leave the repository coherent and validated.

## Validation Requirements

Planning-only changes require:

```powershell
git diff --check
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Contract-changing candidates require focused checks first:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_identity_runtime_resource_tests MK_scene_schema_tests MK_runtime_tests MK_runtime_scene_tests MK_tools_tests MK_editor_core_tests MK_renderer_tests MK_runtime_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_(asset_identity_runtime_resource_tests|scene_schema_tests|runtime_tests|runtime_scene_tests|tools_tests|editor_core_tests|renderer_tests|runtime_rhi_tests)"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Any candidate that changes runtime package samples or generated package templates also requires the matching package validation recipe or `tools/package-desktop-runtime.ps1 -GameTarget <target>` evidence.

Every slice-closing contract PR requires:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

## Completion Audit

The cleanup is complete only when these current-state checks all pass:

- no current source, test, game sample, schema, tool, manifest fragment, generated manifest, or current-truth doc contains a removable `GameEngine.*.vN` saved-format literal
- no current public C++ API exposes removable `*VN` or `*_vN` names for canonical contracts
- stale pre-release parser branches are removed or changed into explicit rejection tests
- all repository-owned sample files and templates emit canonical saved formats
- editor dynamic module loading resolves the canonical factory symbol and still verifies numeric ABI version
- static checks enforce the canonical names and no longer enforce removed names as current truth
- historical plan IDs and external/non-contract version markers remain intact
- focused validation and full `tools/validate.ps1` complete or a concrete host/toolchain blocker is recorded
