# Renderer RHI Safe Point Resource Teardown v1 Implementation Plan (2026-05-03)

> **For agentic workers:** `gameengine-agent-integration` と `cmake-build-system` を参照。本スライスは **調理済みランタイムシーン GPU バインディング結果のホスト所有 safe-point ティアダウン契約**に限定し、`IRhiDevice` 上のネイティブ破棄 API の追加やゲームプレイ API へのネイティブハンドル露出は行わない。

**Goal:** `RuntimeSceneGpuBindingResult` に保持された RHI ハンドルを、**フレーム境界などホストが選ぶ safe-point** で破棄できるよう、バックエンド中立のティアダウン入口と診断を提供する。実 GPU バックエンドでは `IRhiDevice` に destroy が無いため、**`NullRhiDevice` では決定的にハンドルを無効化**し、それ以外では **ホストがネイティブ側で破棄する必要がある**ことを明示する。

**Architecture:** `mirakana_runtime_scene_rhi` に `execute_runtime_scene_gpu_binding_safe_point_teardown(IRhiDevice&, const RuntimeSceneGpuBindingResult&)` を追加する。`owner_device` 不一致は `ownership_mismatch`。`dynamic_cast<NullRhiDevice*>` が成功した場合のみ `null_mark_*` を決定順序で呼び、`RuntimeSceneGpuSafePointTeardownReport` で完了を返す。非 Null では `host_native_destroy_pipeline_required` と診断メッセージのみ。`mirakana_rhi` の `NullRhiDevice` に sampler / descriptor set(layout) / pipeline layout の active フラグと `null_mark_*` を追加し、`owns_*` が active を参照する。

**Tech Stack:** C++23、`mirakana_rhi`、`mirakana_runtime_scene_rhi`、`tests/unit/runtime_scene_rhi_tests.cpp`。

---

## Goal

- safe-point で **同じ `IRhiDevice` インスタンス**に属するシーン GPU バインディングをホストが片付けられる契約を固定する。
- Null デバイス上で **アップロードバッファ等が無効化される**ことをテストで証明する。

## Context

- 親: [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 2。
- 先行: [2026-05-02-renderer-resource-residency-upload-execution-v1.md](2026-05-02-renderer-resource-residency-upload-execution-v1.md)（アップロード実行）、[2026-05-02-safe-point-package-unload-replacement-execution-v1.md](2026-05-02-safe-point-package-unload-replacement-execution-v1.md)（ランタイム safe-point パッケージ置換）。

## Constraints

- 公開ゲームプレイ API にネイティブ／RHI ハンドルを追加しない。
- 実バックエンドの `IRhiDevice` に汎用 destroy を追加しない（本スライス範囲外）。

## Implementation Steps

- [x] `NullRhiDevice` に active フラグと `null_mark_*` 公開ヘルパーを追加する。
- [x] `execute_runtime_scene_gpu_binding_safe_point_teardown` とレポート型を `mirakana_runtime_scene_rhi` に追加する。
- [x] 単体テスト（Null 上の無効化、失敗バインディングのスキップ）を追加する。
- [x] `docs/current-capabilities.md` / `docs/architecture.md` / `docs/roadmap.md` / `engine/agent/manifest.json` / マスター計画 / 計画レジストリを同期する。
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` で証跡を残す。

## Tests

- safe-point teardown 後に `write_buffer` が `invalid_argument` になる（アップロードバッファ無効化）。
- バインディング構築失敗時に teardown がスキップされる。

## Validation Evidence

| コマンド | 結果 | 日付 |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | `validate: ok`、CTest 28/28 Passed | 2026-05-03 |

## Done When

- [x] 上記 API とテストが緑。
- [x] 文書に「非 Null ではホストネイティブ破棄が必要」と明記できる。

## Non-Goals

- D3D12/Vulkan/Metal の実リソース破棄実装。
- `RhiResourceLifetimeRegistry` によるネイティブ破棄の全面移行（Phase 3 以降の別スライス）。

---

*Plan completed: 2026-05-03.*
