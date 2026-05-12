# Broad Dependency Cook Plan v1 Implementation Plan (2026-05-03)

> **For agentic workers:** `gameengine-agent-integration` と `cmake-build-system`（ビルド／テストゲート）を参照。本スライスは **登録済みソース資産レジストリ内**の依存閉包のみを対象とし、Importer・任意シェル・ランタイム側の広域オーケストレーションは扱わない。

**Goal:** `cook-registered-source-assets` / `plan_registered_source_asset_cook_package` に、**同一 `GameEngine.SourceAssetRegistry.v1` 文書内**で辿れる依存キーのみを対象とした **登録レジストリ依存閉包（registry closure）** を追加する。既定は従来どおり **明示的キー選択** のままとし、後方互換の shim は置かない。

**Architecture:** `RegisteredSourceAssetCookPackageRequest` に `dependency_expansion` を追加する。`explicit_dependency_selection` では従来どおり全依存キーを `selected_asset_keys` に含めることを要求する。`registered_source_registry_closure` ではルートとして渡されたキーから **レジストリ上の `dependencies[].key` を BFS で追跡**し、閉包集合を構築する。閉包部分グラフに **有向サイクル** があればエラーとする。クック順序は既存の `build_asset_import_plan` / `execute_asset_import_plan` に委ね、`.geindex` マージと `model_mutations` は既存パイプラインを再利用する。

**Tech Stack:** C++23、`mirakana_tools` / `mirakana_assets`、既存の `validate_source_asset_registry_document`、静的検査（`check-json-contracts` / `check-ai-integration`）、`engine/agent/manifest.json`。

---

## Goal

- AI／エディタが **テクスチャまで個別に列挙しなくても**、マテリアル等のルートキーから **登録済み依存のみ**を自動でクック対象に含められる。
- **レジストリ外**の依存解決、外部 Importer、任意シェル、パッケージストリーミング、RHI 常駐は **引き続き未対応**であることをクレーム文字列で明示する。

## Context

- 親: [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 2 の子計画 `broad-dependency-cook-plan-v1`。
- 前提: [2026-05-01-registered-source-asset-cook-package-command-tooling-v1.md](2026-05-01-registered-source-asset-cook-package-command-tooling-v1.md) の narrow cook 実装。

## Constraints

- ランタイムゲームコードは調理済みパッケージのみを消費する前提は変えない（本変更はツール側のクック計画のみ）。
- **後方互換用の暗黙デフォルト変更は禁止**: 既定は `explicit_dependency_selection`。閉包を使う場合は呼び出し側が `dependency_expansion` と `dependency_cooking` の組み合わせを明示する。
- 任意シェル・自由形式マニフェスト編集は禁止のまま。

## Implementation Steps

- [x] `RegisteredSourceAssetCookDependencyExpansion` と `dependency_expansion` フィールドを `registered_source_asset_cook_package_tool.hpp` に追加する。
- [x] `validate_unsupported_claims` から `dependency_cooking` の単純 `unsupported` 固定を外し、`dependency_expansion` に応じた **`dependency_cooking` センチネル**（`unsupported` / `registry_closure`）検証に置き換える。
- [x] `prepare_selected_rows` に BFS 閉包、サイクル検出、明示選択分岐を実装する。
- [x] `tools_tests.cpp` に閉包成功・センチネル誤りのテストを追加する。
- [x] `engine/agent/manifest.json` の `cook-registered-source-assets` 面、`docs/current-capabilities.md`、`docs/ai-game-development.md` を同期する。
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` で証跡を残す。

## Tests

- `plan_registered_source_asset_cook_package`：閉包モードで **テクスチャキーを列挙せず**マテリアルのみ選択しても、明示選択モードと同等のクック結果になること。
- 明示モード：従来どおり未選択依存は拒否されること。
- `dependency_cooking` センチネルと `dependency_expansion` の不一致は拒否されること。

## Done When

- [x] 上記実装ステップとテストが緑。
- [x] マニフェストの `cook-registered-source-assets` が `dependencyExpansion` と閉包時の `dependencyCooking=registry_closure` 契約を説明している。
- [x] 「広域パッケージ cook（Importer・シーン外・未登録キー）」は **未対応**のまま文書に明記されている。

## Non-Goals

- シーン v2 / Prefab からの自動ルート収集、未登録パスの推論、バックグラウンド cook、パッケージマウント、常駐キャッシュ（別子計画）。

## Validation

| コマンド | 期待 |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | 成功（CTest 含む） |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | manifest / 静的整合 |

## Validation Evidence

| コマンド | 結果 | 日付 |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | `validate: ok`、CTest 28/28 Passed | 2026-05-03 |

---

*Plan completed: 2026-05-03. Evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` green after `RegisteredSourceAssetCookDependencyExpansion` + registry BFS closure in `registered_source_asset_cook_package_tool.cpp`.*
