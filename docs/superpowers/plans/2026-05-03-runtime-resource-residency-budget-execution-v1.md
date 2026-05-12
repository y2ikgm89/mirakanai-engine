# Runtime Resource Residency Budget Execution v1 Implementation Plan (2026-05-03)

> **For agentic workers:** `gameengine-agent-integration` と `cmake-build-system` を参照。本スライスは **調理済みランタイムパッケージの常駐ビューに対する決定的なバイト／レコード上限の実行チェック**に限定し、GPU アロケータ、OS メモリ圧、非同期エビクション、RHI ティアダウンは扱わない。

**Goal:** `packageStreamingResidencyTargets` 等で表現される **常駐バジェット意図**と同一の定義（`RuntimeAssetRecord::content` の合計バイト）を、`merge_runtime_asset_packages_overlay` 後の **単一マージ済みビュー**に対しても機械的に検証できるようにする。上限超過時は **カタログを変更せず**、診断コードで失敗を返す。

**Architecture:** `estimate_runtime_asset_package_resident_bytes` を `mirakana_runtime` の正規の推定関数とし、ホストゲート済み `execute_selected_runtime_package_streaming_safe_point` と共有する。`RuntimeResourceResidencyBudgetV2` は `std::optional` の `max_resident_content_bytes` と `max_resident_asset_records` を取り、未設定の軸は制限なし。`evaluate_runtime_resource_residency_budget` がマージ結果（または単一パッケージ）に対して評価のみを行う。`build_runtime_resource_catalog_v2_from_resident_mounts_with_budget` はマージ → 予算 → 成功時のみ `build_runtime_resource_catalog_v2` の順で **カタログ非破壊**を保証する。

**Tech Stack:** C++23、`mirakana_runtime`（`asset_runtime` / `resource_runtime` / `package_streaming`）、`tests/unit/asset_identity_runtime_resource_tests.cpp`。

---

## Goal

- 常駐マウント結合後の **コンテンツバイト合計** と **アセット行数** に対し、明示的な上限を **決定的に**検証する。
- ストリーミング実行経路と **同じバイト定義**を共有し、二重実装をなくす。

## Context

- 親: [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 2。
- 先行: [2026-05-02-package-streaming-residency-budget-contract-v1.md](2026-05-02-package-streaming-residency-budget-contract-v1.md)（記述のみ）、[2026-05-03-package-mount-and-resident-cache-v1.md](2026-05-03-package-mount-and-resident-cache-v1.md)（マウント結合 API）。

## Constraints

- エビクション・非同期ストリーミング・GPU／プロセス全体のメモリ強制は行わない。
- 公開 API にネイティブ／RHI ハンドルを追加しない。

## Implementation Steps

- [x] `estimate_runtime_asset_package_resident_bytes` を `asset_runtime` に追加し、`package_streaming` から委譲する。
- [x] `RuntimeResourceResidencyBudgetV2` / `evaluate_runtime_resource_residency_budget` / `build_runtime_resource_catalog_v2_from_resident_mounts_with_budget` を `resource_runtime` に追加する。
- [x] 単体テスト（推定、評価、バンドル成功／失敗とカタログ非更新）を追加する。
- [x] `docs/current-capabilities.md` / `docs/architecture.md` / `engine/agent/manifest.json` / マスター計画チェックリストを同期する。
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` で証跡を残す。

## Tests

- バイト合計の単純加算。
- バイト上限・レコード上限・両方違反時の診断コード。
- 予算失敗時に `RuntimeResourceCatalogV2` の世代が変わらないこと。
- 予算成功時にマウント結合カタログが構築されること。

## Validation Evidence

| コマンド | 結果 | 日付 |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | `validate: ok`、CTest 28/28 Passed | 2026-05-03 |

## Done When

- [x] 上記 API とテストが緑。
- [x] 文書に「バジェット実行はオプションのバイト／レコード上限のみ」と明記できる。

## Non-Goals

- アロケータ連動、VRAM 予算、バックグラウンドエビクション、レンダラ safe-point ティアダウン。

---

*Plan completed: 2026-05-03.*
