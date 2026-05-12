# AI Cook Package Command Surface v1 Implementation Plan (2026-05-03)

> **For agentic workers:** `gameengine-agent-integration` と `cmake-build-system` を参照。本スライスは **`cook-registered-source-assets` の入力をマニフェスト上のデスクリプタで固定する**ことに限定し、新しいシェルコマンドやレジストリ外の広義クックは追加しない。

**Goal:** 3D デスクトップパッケージ等で、エージェントが `GameEngine.SourceAssetRegistry.v1` のどのキーをどの `dependencyExpansion` / `dependencyCooking` でクックするかを **`game.agent.json.registeredSourceAssetCookTargets`** として宣言し、既存の `prefabScenePackageAuthoringTargets` 行と **同一の `sourceRegistryPath` / `packageIndexPath`** であることを静的検証で保証する。

**Architecture:** JSON Schema に `registeredSourceAssetCookTargets` を追加する。各行は `descriptor-only-cook-registered-source-assets` モード、`cookCommandId=cook-registered-source-assets`、`prefabScenePackageAuthoringTargetId`、ルート `selectedAssetKeys`、`dependencyExpansion`（明示選択または `registered_source_registry_closure`）、対応する `dependencyCooking`（`unsupported` または `registry_closure`）、および既存どおりの unsupported センチネル列を持つ。`tools/check-json-contracts.ps1` が prefab 行とのパス整合・キー存在・拡張/クック契約の組合せを検証する。`tools/new-game.ps1` の `DesktopRuntime3DPackage` テンプレートと `games/sample_desktop_runtime_game/game.agent.json` に代表行を追加する。`engine/agent/manifest.json` の Editor AI / 3D prefab authoring ループに `requiredManifestFields` と `descriptorFields` を拡張する。

**Tech Stack:** JSON Schema、`tools/check-json-contracts.ps1`、`tools/check-ai-integration.ps1`、`tools/new-game.ps1`、`engine/agent/manifest.json`、ドキュメント。

---

## Goal

- `cook-registered-source-assets` 向けの **レビュー済みキー選択と閉包モード**をマニフェストデータとして表現する。
- Prefab オーサリング行と **矛盾しない**クック意図を機械的に検証する。

## Context

- 親: [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 2。
- 先行: [2026-05-03-broad-dependency-cook-plan-v1.md](2026-05-03-broad-dependency-cook-plan-v1.md)（レジストリ閉包クック）、[2026-05-01-registered-source-asset-cook-package-command-tooling-v1.md](2026-05-01-registered-source-asset-cook-package-command-tooling-v1.md)（C++ cook 面）。

## Constraints

- デスクリプタは **データのみ**。シェル文字列・任意 argv・ランタイムソース解析は禁止。
- レジストリ外の広義依存クックや外部 Importer 実行は宣言しない。

## Implementation Steps

- [x] `schemas/game-agent.schema.json` に `registeredSourceAssetCookTargets` を追加する。
- [x] `Assert-RegisteredSourceAssetCookTargets` を `check-json-contracts.ps1` に追加し、3D 必須ゲームで検証する。
- [x] `Assert-RegisteredSourceAssetCookTarget` とサンプル／足場検証を `check-ai-integration.ps1` に追加する。
- [x] `sample_desktop_runtime_game` と `new-game.ps1` の 3D テンプレートに代表行を追加する。
- [x] `engine/agent/manifest.json` の review loop / `3d-prefab-scene-package-authoring` を同期する。
- [x] `docs/current-capabilities.md` / `docs/ai-game-development.md` / `docs/roadmap.md` を更新する。
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` で証跡を残す。

## Validation Evidence

| コマンド | 結果 | 日付 |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | `validate: ok`、CTest 28/28 Passed | 2026-05-03 18:26 +09:00 |

## Done When

- [x] 3D パッケージマニフェストが cook デスクリプタを必須とし、静的チェックが緑。
- [x] エージェント向けドキュメントに `registeredSourceAssetCookTargets` の意味が書ける。

## Non-Goals

- `cook-runtime-package` の実装やシェル経由の広義パッケージビルド。
- エディタ UI からの cook ウィザード。

---

*Plan completed: 2026-05-03.*
