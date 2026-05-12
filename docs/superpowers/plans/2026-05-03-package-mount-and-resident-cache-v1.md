# Package Mount And Resident Cache v1 Implementation Plan (2026-05-03)

> **For agentic workers:** `gameengine-agent-integration` と `cmake-build-system` を参照。本スライスは **ランタイム側の決定的マウント結合**に限定し、非同期ストリーミング・任意エビクション・RHI ティアダウンは扱わない。

**Goal:** 複数の調理済み `RuntimeAssetPackage` を **明示的なマウント順**でオーバーレイ結合し、結果を **単一の `RuntimeResourceCatalogV2` 再構築**に渡す。生成世代（`RuntimeResourceHandleV2::generation`）は既存の `build_runtime_resource_catalog_v2` 契約のまま **置換時に無効化**される。

**Architecture:** `RuntimePackageMountOverlay` で同一 `AssetId` が複数マウントに出現したときの勝者を定義する（`first_mount_wins` / `last_mount_wins`）。`merge_runtime_asset_packages_overlay` がレコードを `AssetId` 単位で統合し、ハンドルを決定的に振り直し、生存 `AssetId` 集合に閉じた依存辺を重複除去する。`build_runtime_resource_catalog_v2_from_resident_mounts` は結合結果に対して既存のカタログ構築を呼ぶ。

**Tech Stack:** C++23、`mirakana_runtime`（`asset_runtime` / `resource_runtime`）、`tests/unit/asset_identity_runtime_resource_tests.cpp`。

---

## Goal

- 「常駐キャッシュ＝複数パッケージを同時に解決に載せる」ための **最小の型安全な結合 API** を提供する。
- AI／ゲームコードが **マウント順とオーバーレイ規則を明示**でき、暗黙の後方互換挙動を増やさない。

## Context

- 親: [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 2。
- 前提: 単一 `RuntimeAssetPackageStore` の safe-point 置換は既存。本スライスは **複数パッケージの同時常駐ビュー**を追加し、既存ストア契約を壊さない。

## Constraints

- ランタイムは調理済みパッケージのみ。結合はメモリ上の `RuntimeAssetPackage` に対する決定的処理のみ。
- 非同期ロード、バックグラウンドエビクション、GPU/RHI ティアダウンは別子計画。

## Implementation Steps

- [x] `RuntimePackageMountOverlay` と `merge_runtime_asset_packages_overlay` を `asset_runtime` に追加する。
- [x] `build_runtime_resource_catalog_v2_from_resident_mounts` を `resource_runtime` に追加する。
- [x] `asset_identity_runtime_resource_tests.cpp` にオーバーレイ／辺統合のテストを追加する。
- [x] `docs/current-capabilities.md` / `docs/architecture.md` を最小更新する。
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` で証跡を残す。

## Tests

- `first_mount_wins` と `last_mount_wins` で同一 `AssetId` の勝者が入れ替わること。
- マウント間の依存辺が生存アセットに制限され、重複が除去されること。

## Validation Evidence

| コマンド | 結果 | 日付 |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | `validate: ok`、CTest 28/28 Passed | 2026-05-03 |

## Done When

- [x] 上記 API とテストが緑。
- [x] 文書に「マウント結合は明示オーバーレイ規則必須」と書ける。

## Non-Goals

- `RuntimeAssetPackageStore` の多段マウント置換、ディスク上の VFS マウント、ストリーミング実行。

---

*Plan completed: 2026-05-03.*
