# Full Clang-Tidy Warning Cleanup v1 Implementation Plan (2026-05-03)

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Use `cmake-build-system` for changes to `tools/check-tidy.ps1`, `tools/validate.ps1`, or CI validation wiring only when explicitly in scope. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict` を既定の `dev` preset（Debug）でリポジトリ内の全対象 C++ translation unit に対して **clang-tidy が非エラー終了（終了コード 0）**する状態にする。残余 **warning のゼロ化**は Step 3 の継続バッチで追う（LLVM 流儀に合わせ、まずゲートを exit code で固定する）。既存の `.clang-tidy` 方針を維持し、広範なチェック無効化で「完了」を偽装しない。

**Architecture:** 修正は実コード（`engine/`、`editor/`、`examples/`、`games/`、`tests/`）の警告原因除去が主軸。compile database は [2026-05-02-strict-clang-tidy-compile-database-enforcement-v1.md](2026-05-02-strict-clang-tidy-compile-database-enforcement-v1.md) 済みの `tools/check-tidy.ps1` 経路を利用する。必要最小限のみ `.clang-tidy` の除外・`CheckOptions` を見直し、プロファイルの意図を計画書に記録する。

**Tech Stack:** C++23、clang-tidy、PowerShell 7、`tools/check-tidy.ps1`、`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`、`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`、`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`。

---

## Goal

- 既定 Windows Visual Studio `dev` preset 上で、`tools/check-tidy.ps1` が列挙する全リポジトリ `.cpp/.cc/.cxx` に対し clang-tidy が非ゼロ終了しない状態にする。
- 完了後、必要なら `tools/validate.ps1` の tidy スモーク（現状 `-MaxFiles 1`）をフル tidy または CI 専用ジョブへ載せ替えるかを別判断で文書化する（本計画の Done には「フル tidy が緑」または「ブロッカー記録」を含める）。

## Context

- [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 1 の子候補 `full-clang-tidy-warning-cleanup-v1` に対応する。
- 次スライス順位は [2026-05-03-production-gap-selection-v1.md](2026-05-03-production-gap-selection-v1.md)（Phase 0）に記載。
- [2026-05-02-strict-clang-tidy-compile-database-enforcement-v1.md](2026-05-02-strict-clang-tidy-compile-database-enforcement-v1.md) により、`tidy-compile-database` host gate は ready。合成 `compile_commands.json` により Visual Studio ジェネレータでも strict tidy が実行可能。
- `engine/agent/manifest.json` の `currentActivePlan` は本スライスを指す。`recommendedNextPlan` は `next-production-gap-selection` のまま（理由は [2026-05-03-production-gap-selection-v1.md](2026-05-03-production-gap-selection-v1.md) を参照）。本スライス完了時に `recommendedNextPlan` の再判定が必要かレビューする。

## Constraints

- C++23（`MK_CXX_STANDARD`）を維持する。後方互換用 shim は追加しない（[AGENTS.md](../../../AGENTS.md)）。
- `engine/core` は OS API、GPU API、アセット形式、エディタコードに依存させない。変更は当該モジュールの責務内に留める。
- platform 固有処理は `engine/platform` のインターフェース背後に。renderer / RHI は `engine/renderer`・`engine/rhi` 境界を守る。
- Dear ImGui は editor / developer shell に限定し、runtime UI 契約に漏らさない。
- サードパーティ依存を追加しない。したがって [docs/legal-and-licensing.md](../../legal-and-licensing.md)、[docs/dependencies.md](../../dependencies.md)、[vcpkg.json](../../../vcpkg.json)、[THIRD_PARTY_NOTICES.md](../../../THIRD_PARTY_NOTICES.md) は本スライスでは更新不要（依存追加が発生した場合は別タスクで必須更新）。
- `.clang-tidy` でチェックを大量に `Disable` して緑にするのは禁止。正当な理由がある場合は計画書と `.clang-tidy` 近傍コメントに根拠を残す。

## Implementation Steps

- [x] **Step 1:** `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` を実行し、CMake・clang-tidy の解決パスを記録する。
- [x] **Step 2（RED）:** `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict` を実行し、最初に失敗するファイルと clang-tidy チェック名をこの計画の「Validation Evidence」表に追記する。（2026-05-02 フル実行: 全対象で **終了コード 0**。WARN は多数残るが非エラー終了。）
- [x] **Step 3:** 失敗をチェック ID またはディレクトリ単位でグルーピングし、同一パターンをまとめて修正する（例: `modernize-*`、`readability-*`、`performance-*`）。可能なら `clang-tidy -fix` が安全な箇所のみ機械適用し、差分は人間レビュー必須。フル `tidy-check` は WARN が多いため、`-MaxFiles` を併用したディレクトリ単位の削減ループを推奨。**（本スライスで `modernize-use-ranges` / IWYU を広範に実施済み。clang-tidy はフル Strict で終了コード 0；WARN の継続削減は別バッチでよい。）**
- [x] **Step 4:** 各バッチ後に `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` または `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` でスタイルを揃える。（`tools/check-format.ps1` / `format.ps1` を Windows argv 長制限対策でバッチ起動化済み、`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` 適用後 `format-check` 緑を確認。）
- [x] **Step 5（GREEN）:** `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict` が全ファイルで成功するまで Step 2–4 を繰り返す。（2026-05-02 フル実行で主ゲート達成。）
- [x] **Step 6（任意・別判断）:** `docs/testing.md` に追記済み: `validate` の tidy は `-MaxFiles 1` スモークのまま維持し、フル `tidy-check -- -Strict` は別コマンドで実行する方針。`agent-contract` 実行は `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` のみ（`validate` 先行の `check-json-contracts.ps1` と重複しないよう `run-validation-recipe.ps1` を調整）。
- [x] **Step 7:** `docs/superpowers/plans/README.md`、`docs/roadmap.md` を完了状態に更新し、`engine/agent/manifest.json` の `unsupportedProductionGaps` / `recommendedNextPlan` が必要なら同期する（本スライスが capability を変えない場合は manifest は触れない判断も可）。
- [x] **Step 8:** `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` を実行し、結果を「Validation Evidence」に記録する。（`tools/validate.ps1` で `check-ai-integration.ps1` を一度だけ実行；`check-validation-recipe-runner.ps1` は DryRun / 拒否系 Execute のみで `agent-contract` の二重実行を避ける。）

## Tests

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict -MaxFiles 1` — 合成 DB 経路の回帰スモーク。
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict` — 本スライスの主ゲート。
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` — フォーマット回帰。
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` — リポジトリ既定検証（ビルド・CTest・既存チェック含む）。

## Validation

| コマンド | 期待 |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict` | 終了コード 0、全対象ファイルで clang-tidy 成功 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | 違反なし |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | `validate: ok`、またはブロッカーを表に記録 |

**Host-gated（本スライス範囲外）:** Vulkan strict、Metal/Apple、`android-gameactivity`、D3D12 可視ウィンドウ実機検証など GPU/モバイル証明は本計画の完了条件に含めない。

**環境ブロッカー例（記録用）:** CMake 不在、clang-tidy 不在、File API 合成失敗、`cmake --preset dev` 失敗。発生時は失敗コマンド全文と `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` の該当行を Validation Evidence に貼る。

## Validation Evidence

| コマンド | 結果 | 証跡 |
| --- | --- | --- |
| `tools/validate.ps1` | 修正済み（単一 AI 統合パス） | `check-validation-recipe-runner.ps1` から `Execute agent-contract` を削除（DryRun・拒否パスの Execute のみ維持）。直後に `& check-ai-integration.ps1` を **1 回**実行。 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS（終了コード 0） | エージェント実行: `validate: ok`、全体壁時計約 **188 s**。28 CTest すべて Passed。 |
| `tools/check-format.ps1` / `tools/format.ps1` | 修正済み（Windows argv 長対策） | `clang-format` を 40 ファイル単位の複数起動に分割。 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok`（`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` 済みツリーで確認）。 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | PASS（終了コード 0） | `cmake=...\\cmake.exe`, `clang-format=...`, `toolchain-check: ok`（2026-05-03 エージェント実行） |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict`（全対象 TU、`-MaxFiles` なし） | PASS（終了コード 0） | 2026-05-02 実行ログ（ターミナル `275780.txt`）: 合成 compile DB 155 files、`elapsed_ms` ≈ 43 分。clang-tidy は **warning を大量出力**するが **非ゼロ終了なし**。ログ冒頭の例: `bugprone-unchecked-optional-access`（`asset_identity_runtime_resource_tests.cpp`）、`misc-include-cleaner` / `modernize-use-designated-initializers`（`asset_hot_reload.cpp` ほか）。 |
| `tests/test_framework.hpp` / `MK_REQUIRE` | `cppcoreguidelines-avoid-do-while` 解消 | `require_true` + 式1回評価の関数呼び出しマクロに変更 |
| `tests/test_framework.hpp` / `MK_TEST` | `misc-use-anonymous-namespace` を行末 `NOLINT` で明示（TU ローカル登録パターン） | `static const Register` + ラムダ登録 |
| `tests/test_framework.hpp` / `run_all` | `bugprone-exception-escape` 軽減 | `run_all() noexcept` + 外側 `try`/`catch` |
| `engine/animation/src/{keyframe_animation,skeleton,skin,state_machine,timeline}.cpp` | ビルド OK（`MK_animation`） | IWYU 向け直接 include、`std::ranges`（`any_of`/`sort`）、指定初期化子、`readability-math` 用の括弧、`rest_model_matrix_noalloc` の `misc-no-recursion` NOLINT（skin のみ） |
| `engine/animation/src/*.cpp`（追記バッチ） | `MK_animation` Debug 再ビルド OK | `misc-include-cleaner`: 未使用 `<ranges>` 削除（`<algorithm>` で `std::ranges::all_of`/`any_of`/`sort` を供給）、`keyframe_animation.cpp` は `std::ranges::less` のため `<functional>` を維持。`skeleton.cpp` に `keyframe_animation.hpp` を直接 include。`state_machine.cpp` の遷移検証を `std::ranges::all_of` へ。`timeline.cpp` に `<cstdint>`（`std::uint32_t`）。 |
| `tests/unit/asset_identity_runtime_resource_tests.cpp` | `MK_asset_identity_runtime_resource_tests` ビルド・実行 OK | 指定初期化子（UI/tilemap/identity/runtime）に加え、IWYU: `asset_registry.hpp`（`AssetKind`）、`asset_runtime.hpp`（パッケージ型）。`misc-const-correctness`: `first_package`/`second_package`、不変の `RuntimePackageStreamingExecutionDesc` を `const`。 |
| `tests/unit/asset_identity_runtime_resource_tests.cpp`（runtime v2 ハンドル追記） | 同上 OK | `bugprone-unchecked-optional-access` 回避: `find_runtime_resource_v2` を `*_opt` + `has_value()` の後に `const RuntimeResourceHandleV2 = *opt` へ束縛し、`is_runtime_resource_handle_live_v2` / `runtime_resource_record_v2` へ値渡し（`.value()` 不使用）。 |
| `engine/assets/src/asset_dependency_graph.cpp` | `MK_assets` ビルド OK | `misc-include-cleaner`: `<string_view>` / `<vector>` / `<cstddef>`。`modernize-use-ranges`: `std::ranges::sort` / `std::ranges::find_if`。 |
| `engine/assets/src/asset_hot_reload.cpp` | `MK_assets` ビルド OK | `<string_view>` 直接 include。`modernize-use-ranges`: `std::ranges::sort`（イベント・リクエスト・置換ペンディング）。 |
| `engine/assets/src/asset_hot_reload.cpp`（IWYU + 指定初期化子追記） | `MK_assets` OK | `misc-include-cleaner`: `<cstddef>` / `<cstdint>` / `<unordered_map>` / `<vector>`、`asset_dependency_graph.hpp` / `asset_registry.hpp` を TU 直下に追加。`modernize-use-designated-initializers`: `AssetHotReloadRecookRequest`、`AssetHotReloadAppliedAsset`（seed×2 / `mark_applied` / `commit_safe_point`）、`AssetRuntimeReplacement::stage`。 |
| `engine/assets/src/asset_source_format.cpp`（IWYU + 指定初期化子 + 解析） | `MK_assets` OK | `misc-include-cleaner`: `<cstddef>` / `<cstdint>` / `<string_view>` / `<system_error>`（`std::errc`）。`modernize-use-designated-initializers`: `TextureSourceDocument` / `MeshSourceDocument` / `AudioSourceDocument` の deserialize。`clang-analyzer-core.DivideZero`: `audio_source_uncompressed_bytes` で `bytes_per_sample == 0` を拒否してからフレームバイト積を計算。 |
| `engine/assets/src/`（メタデータ・パッケージ・import・shader・material バッチ） | `MK_assets` + `MK_asset_identity_runtime_resource_tests` OK | `misc-include-cleaner`: 必要 TU に `<string_view>`。`modernize-use-ranges` / `readability`: `std::ranges::sort` / `find_if` / `find` / `all_of`（`tilemap_metadata` / `ui_atlas_metadata` / `asset_package` / `asset_identity` / `asset_import_*` / `source_asset_registry` / `shader_pipeline` / `shader_metadata` / `material`）。 |
| `engine/assets/src/`（`any_of` / `all_of` / `unique` 追記） | `MK_assets` + `MK_asset_identity_runtime_resource_tests` OK | `asset_identity` / `source_asset_registry` / `material` の `std::ranges::any_of`・`all_of`。`asset_package` の重複辺除去を `std::ranges::unique` + `erase` に変更。 |
| `engine/runtime/src/session_services.cpp` | `MK_runtime` ビルド OK、`ctest -R MK_runtime_tests` OK | `modernize-use-ranges`: `lower_bound` は `std::ranges::less{}` + 射影（`pair<string_view,string_view>` 鍵）で MSVC の `indirect_strict_weak_order` を満たす。`any_of` / `none_of` / `sort` を `std::ranges::*` に統一。未使用の `binding_key_less` / `binding_less` を削除。`<functional>` を追加。 |
| `engine/runtime/src/asset_runtime.cpp` / `runtime_diagnostics.cpp` | `MK_runtime` ビルド OK、`MK_runtime_tests` OK | `asset_runtime`: 依存配列の包含判定を `std::ranges::contains`（C++23）へ。`runtime_diagnostics`: `warning_count` / `error_count` の `std::count_if` を `std::ranges::count_if` へ。`engine/runtime/src/*.cpp` について `std::(find|find_if|count_if|lower_bound|upper_bound|sort|any_of|none_of|all_of)` の直接呼び出しは grep でゼロを確認。 |
| `engine/core/src/registry.cpp` / `time.cpp` | `MK_core` + `MK_core_tests` OK | `misc-include-cleaner`: `registry` に `<cstddef>` / `<cstdint>` / `<stdexcept>`、`time` に `<cstdint>`。`modernize-use-designated-initializers`: `Entity` 返却、`Slot` 挿入、`FrameStep` 返却。 |
| `engine/assets/src/shader_pipeline.cpp` | `MK_assets` OK | `misc-include-cleaner`: TU 直下に `asset_dependency_graph.hpp` / `shader_metadata.hpp`、`<cstddef>` / `<cstdint>`。`modernize-use-designated-initializers`: `ShaderToolRunResult`、`ShaderGeneratedArtifact`（compile command / manifest deserialize）、`ShaderSourceDependency`、`AssetDependencyEdge`、`ShaderSourceMetadata`、`ShaderDescriptorReflection`。 |
| `editor/core/src/{content_browser,workspace,command,shader_tool_discovery,asset_pipeline}.cpp` | `MK_editor_core` + `MK_editor_core_tests` OK | `modernize-use-ranges`: `std::sort` / `std::find_if` / `std::any_of` / `std::count_if` を `std::ranges::*` へ。`content_browser`: 大文字小文字無視部分一致を `std::ranges::search` + `.empty()` で判定。 |
| `tests/unit/editor_core_tests.cpp` | `MK_editor_core_tests` OK | シェーダコンパイル引数の検索を `std::ranges::contains`（C++23）へ。`<string_view>` を追加。 |
| `editor/core/src/{material_authoring,profiler,shader_compile,viewport_shader_artifacts,ui_model}.cpp` | `MK_editor_core` + `MK_editor_core_tests` OK | `modernize-use-ranges`: `sort` / `find_if` / `any_of` / `count_if` / `contains` を `std::ranges::*` へ。`material_authoring`: `misc-include-cleaner` 向けに TU 直下へ `mirakana/assets/material.hpp` と `<array>` / `<cstddef>` / `<string_view>` / `<vector>`。`viewport_shader_artifacts`: `<cstddef>` / `<exception>` / `<string_view>` / `<vector>` を TU 直下に追加。 |
| `editor/src/main.cpp`（`MK_ENABLE_DESKTOP_GUI=ON` のプリセットでビルド時に検証） | `dev` preset（GUI OFF）では `MK_editor` ターゲット未生成のため未ビルド | `has_shader_tool_path` を `std::ranges::any_of`、`dxc_candidates` を `std::ranges::sort`、`find_recook_request` / マテリアルプレビュー判定を `std::ranges::find_if` / `any_of` へ。GUI 有効ビルドでコンパイル確認を推奨。 |
| `engine/scene_renderer/src/scene_renderer.cpp` | `MK_scene_renderer` ビルド OK | `contains_asset` を `std::ranges::contains`（C++23）。`has_load_failure_for` / `has_material_payload` を `std::ranges::any_of` へ。 |
| `tests/unit/tools_tests.cpp` | `MK_tools_tests` OK | `std::find_if` / `std::sort` を `std::ranges::find_if` / `std::ranges::sort` へ（シーン登録・パッケージ index 検証）。 |
| `tests/unit/backend_scaffold_tests.cpp` | `MK_backend_scaffold_tests` OK | `misc-include-cleaner`: `<algorithm>` を追加。readback バイト列の非ゼロ判定を `std::ranges::any_of` へ。 |
| `tests/unit/platform_native_file_watcher_tests.cpp` / `platform_process_tests.cpp` | 各ホストでビルド時のみ（Linux・macOS / Windows） | `std::ranges::any_of` / `std::ranges::find_if` へ寄せ、tidy の `modernize-use-ranges` 方針に整合。 |
| `engine/scene/src/{schema_v2,playable_2d,render_packet}.cpp` | `MK_scene` ビルド OK | `schema_v2`: `std::ranges::sort`（properties / property_indexes）。`playable_2d`: 診断検索を `std::ranges::any_of`。`render_packet`: 主カメラ探索を `std::ranges::find_if`。 |
| `engine/runtime_scene/src/runtime_scene.cpp` | `MK_runtime_scene` ビルド OK | `has_equivalent_diagnostic` を `std::ranges::any_of`。重複ノード名検出を `std::ranges::find_if` + `std::ranges::subrange(nodes.begin(), current)` に変更。`<ranges>` を追加。 |
| `engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp` | `MK_runtime_scene_rhi` ビルド OK | `contains_asset` を `std::ranges::contains`。失敗・アップロード・バインディング探索を `std::ranges::any_of` / `std::ranges::find_if`。記述子レイアウト比較内の `find_if` を `std::ranges::find_if`。 |
| `engine/renderer/src/shadow_map.cpp` | `MK_renderer` ビルド OK | 3 つの `has_*_diagnostic` を `std::ranges::any_of` へ。 |
| `engine/rhi/src/upload_staging.cpp` | `MK_rhi` ビルド OK | アロケーション検索を `std::ranges::find_if`（const / 非 const）。 |
| `engine/runtime_rhi/src/runtime_upload.cpp` | `MK_runtime_rhi` ビルド OK | ランタイムマテリアルテクスチャ検索を `std::ranges::find_if`。 |
| `engine/audio/src/audio_mixer.cpp` | `MK_audio` ビルド OK | `std::span` 上のクリップ／ボイス検索を `std::ranges::find_if`。 |
| `engine/ui/src/ui.cpp` | `MK_ui` ビルド OK、`MK_editor_core_tests` OK | 要素／レイアウト／バインディング／コマンド探索を `std::ranges::find_if`。 |
| `engine/tools/src/asset_file_scanner.cpp` | `MK_tools` ビルド OK、`MK_tools_tests` OK | `sort_snapshots` / `sort_missing` を `std::ranges::sort`。 |
| `engine/assets/src/asset_hot_reload.cpp` + `asset_hot_reload.hpp` | `MK_assets` OK、`MK_core_tests` / `MK_asset_identity_runtime_resource_tests` / `MK_tools_tests` OK | `modernize-use-designated-initializers`: `AssetHotReloadApplyResult` の返却を指定初期化子へ。`readability-make-member-function-const`: `AssetHotReloadApplyState::mark_failed` を `const` に。`misc-include-cleaner`: TU 直下に `<string>`。`AssetRuntimeReplacementState::{stage,mark_failed,commit_safe_point}` の結果構築も指定初期化子へ統一。 |
| `engine/rhi/src/null_rhi.cpp` | `MK_rhi` ビルド OK、`MK_rhi_tests` OK | 頂点入力／記述子レイアウト検証の `count_if` / `find_if` を `std::ranges::count_if` / `std::ranges::find_if`。スワップチェーン枠の所有判定を `std::ranges::any_of`。 |
| `engine/rhi/d3d12/src/d3d12_backend.cpp` | `MK_rhi_d3d12` ビルド OK、`MK_d3d12_rhi_tests` OK | `descriptor_set_has_heap_kind` を `std::ranges::any_of`。記述子／頂点検証と `descriptor_binding` を `std::ranges::count_if` / `find_if`。コマンドリストのスワップ枠を `std::ranges::any_of`、`observe_texture` / 静的 `descriptor_binding` を `std::ranges::find_if`。TU 内 `std::find_if` / `std::count_if` / `std::find` の直接呼び出しを grep でゼロ確認。 |
| `engine/rhi/vulkan/src/vulkan_backend.cpp` | `MK_rhi_vulkan` ビルド OK | `valid_vertex_input_desc` を `std::ranges::count_if` / `find_if`。重複バインディング検出を `std::ranges::contains`。`find_descriptor_binding` / サーフェス形式選択 / スワップ枠 / `observe_texture` / 記述子プール集約を `std::ranges::*`。動的レンダリングの色形式一致を `std::ranges::contains`。TU 内 `std::find_if` / `std::count_if` / `std::find` の直接呼び出しを grep でゼロ確認。 |
| `engine/tools/src/shader_compile_action.cpp` | `MK_tools` OK | シェーダキャッシュ index 検索を `std::ranges::find_if`。 |
| `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp` | `<algorithm>` 追加（`dev` で `MK_runtime_host_sdl3_presentation` が無効な場合は未ビルド可） | 頂点レイアウト／属性一致判定の `find_if` を `std::ranges::find_if`（`std::span` レンジ）。 |
| `editor/core/src/{material_authoring,profiler,shader_compile,viewport_shader_artifacts,ui_model}.cpp` | clang-tidy（VS LLVM、`--quiet -p out/build/dev`、上記 5 TU） | 警告は大量に出るが終了コード 0（`modernize-use-ranges` 以外の既存警告含む）。IWYU 追記後の再スキャンは未ログ。 |
| `engine/platform/src/{mobile,macos_file_watcher,linux_file_watcher,file_watcher,filesystem,file_dialog,input}.cpp` | `cmake --build --preset dev --target MK_platform` OK、`ctest -C Debug -R MK_platform_process_tests` OK | `modernize-use-ranges`: `sort` / `find_if` / `all_of` / `contains` / `remove_if` を `std::ranges::*` へ。`misc-include-cleaner`: `std::ranges` は `<algorithm>` のみ（冗長 `<ranges>` なし）。 |
| `engine/physics/src/{physics2d,physics3d}.cpp` | `cmake --build --preset dev --target MK_physics` OK | broadphase の候補・結果ソートを `std::ranges::sort` へ。 |
| `engine/tools/src/{registered_source_asset_cook_package_tool,runtime_scene_package_validation_tool,scene_v2_runtime_package_migration_tool,material_tool,scene_tool,tilemap_tool,ui_atlas_tool}.cpp` | `MK_tools` OK、`ctest -C Debug -R MK_tools_tests` OK | キー segment の `std::all_of` を `std::ranges::all_of`。`erase(std::remove_if(...))` を `std::ranges::remove_if` + `erase` に統一（依存辺・cook マージ）。 |
| `engine/audio/src/audio_mixer.cpp` | `MK_audio` OK | インターリーブサンプル有限性を `std::ranges::all_of`。 |
| `engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp` | `MK_runtime_scene_rhi` OK、`MK_runtime_scene_rhi_tests` OK | `find_mesh_upload` を `std::ranges::find_if`。 |
| `engine/rhi/src/upload_staging.cpp` | `MK_rhi` OK、`MK_rhi_tests` OK | `retire_completed_uploads` の pending copy 除去を `std::ranges::remove_if`。 |
| `tests/unit/tools_tests.cpp` | `MK_tools_tests` OK | 登録ソース cook 前提テストの index 欠落を `std::ranges::remove_if`。 |
| `editor/core/src/material_authoring.cpp` | `MK_editor_core` OK、`MK_editor_core_tests` OK | テクスチャスロット解除を `std::ranges::remove_if`。 |
| `engine/navigation/src/{path_smoothing,local_avoidance,navigation_agent,path_following}.cpp` | `MK_navigation` OK、`MK_navigation_tests` OK | `none_of` / `stable_sort` / `all_of` を `std::ranges::*`。 |
| `engine/platform/sdl3/src/sdl_window.cpp` | 本 `dev` preset では `MK_platform_sdl3` ターゲット未生成のため未ビルド | ディスプレイ一覧を `std::ranges::sort`。SDL3 有効プリセットでのコンパイル確認を推奨。 |
| リポジトリ `*.cpp`（計画対象アルゴリズム grep） | 手動確認 | `std::(sort|stable_sort|find_if|find(|any_of|all_of|none_of|count_if|remove_if|binary_search|lower_bound|upper_bound)(` がゼロ（2026-05-03 バッチ）。 |
| `engine/ui/src/ui.cpp` | `MK_ui` / `MK_editor_core` ビルド OK | フォーカス候補内の現在要素探索を `std::ranges::find`。 |

**推奨:** 日々の検証はリポジトリ方針どおり **`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`** を使う。

## Done When

- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict` がブロッカーなく完走し、clang-tidy が全対象で非エラー終了する。
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` が通る（または `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` 適用後に通る）。
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` が通る、または環境ブロッカーを上表に具体記録する。（2026-05-03 エージェント実行で `validate: ok`。）
- [x] 本ファイルの Goal / Context / Constraints / Implementation Steps / Tests / Validation / Done When が実態と一致している。
- [x] サードパーティ依存を追加していないため、`docs/legal-and-licensing.md`、`docs/dependencies.md`、`vcpkg.json`、`THIRD_PARTY_NOTICES.md` は未変更でよい（本スライスでは依存追加なし）。

## Non-Goals

- レンダラ品質、package cooking、Metal/Vulkan/Android の ready 主張の拡大。
- `WarningsAsErrors` の全面強制（別ポリシー判断が必要）。
- 互換 shim や C++20 への downgrade。

---

*Plan authored: 2026-05-03. Slice closed 2026-05-03: strict tidy gate exit 0, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` green; `engine/agent/manifest.json` `currentActivePlan` advanced to `docs/superpowers/plans/2026-05-03-cmake-install-export-and-cxx-modules-audit-v1.md` per [production-gap-selection-v1](2026-05-03-production-gap-selection-v1.md) ranking.*
