# 生成静的 3D デスクトップゲーム運用レシピ v1 実装計画 (2026-05-05)

> **エージェント向け:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6 の子計画 `generated-static-3d-production-game-recipe-v1` に対応する。`production-gap-selection-v1` は **`currentActivePlan` に置かない**。本スライス完了後の狭い実装スライスは `engine/agent/manifest.json` の `currentActivePlan` を参照する。

## ゴール

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Template DesktopRuntime3DPackage` が生成する **静的メッシュ中心の 3D パッケージゲーム**について、リポジトリが既に提供する **公式コマンド面・マニフェスト契約・検証レシピ**に沿った **再現可能なオペレータ手順書**を単一の日付付き計画に固定する。
- 生成物の `game.agent.json.gameplayContract.productionRecipe` が指す **`3d-playable-desktop-package`**（`engine/agent/manifest.json` の `aiOperableProductionLoop.recipes`）と、生成テンプレート（`tools/new-game.ps1` の `New-DesktopRuntime3DManifest` 等）の **フィールド集合**が矛盾しないことを文書で明示する（新しい後方互換用フィールドは追加しない）。

## コンテキスト

- Phase 6 マスター「Done when」には「`DesktopRuntime3DPackage` can generate and package a static 3D game … and package validation」と子計画 `generated-static-3d-production-game-recipe-v1` が列挙されている。
- 実装面では [2026-05-02-3d-packaged-playable-generation-loop-v1.md](2026-05-02-3d-packaged-playable-generation-loop-v1.md)、[2026-05-02-generated-3d-mesh-telemetry-scaffold-v1.md](2026-05-02-generated-3d-mesh-telemetry-scaffold-v1.md)、[2026-05-02-3d-prefab-scene-package-authoring-v1.md](2026-05-02-3d-prefab-scene-package-authoring-v1.md) などで **スキャフォールド・テレメトリ・記述子**が揃っており、本スライスは **運用レシピとドキュメントの正典化**が主担当である。

## 制約

- **後方互換用 shim は追加しない**（[AGENTS.md](../../../AGENTS.md)）。
- 任意シェル・生のマニフェスト文字列評価・未レビューのパッケージ書き込みは **既存の AI コマンド面**外とみなし、本スライスでは新しい実行経路を増やさない。
- Metal 一般化・strict Vulkan 本番行列の完走を **本スライスの完了条件に含めない**（ホストゲートは `manifest.json` の `hostGates` と各検証レシピに従う）。

## 非ゴール（v1）

- glTF エディタ UX、クック済みメッシュ接線フレーム拡張、GPU スキニング、マテリアルグラフの本番実行など **Phase 6 の後続子計画**に属する内容。
- 広義の依存関係クロージャクック、パッケージストリーミング本番、ゲームプレイからのネイティブ RHI 露出。

## オペレータレシピ（正典手順）

以下は **生成直後の既定レイアウト**（`games/<Name>/`、`runtime/<Name>.geindex` 等）を前提とする。手順の詳細な argv は各コマンドの DryRun を優先する（[docs/workflows.md](../../workflows.md)）。

1. **生成:** `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name <snake_case_game_name> -Template DesktopRuntime3DPackage`
2. **ビルド統合の確認:** `games/CMakeLists.txt` に `mirakana_add_desktop_runtime_game` ブロックが追記されていること、`GAME_MANIFEST` が `games/<Name>/game.agent.json` を指していることを確認する。
3. **マニフェスト契約の読み取り:** `game.agent.json` の `runtimePackageFiles`、`runtimeSceneValidationTargets`、`materialShaderAuthoringTargets`（HLSL 入力と D3D12/Vulkan アーティファクトパス）、`prefabScenePackageAuthoringTargets`、`packageStreamingResidencyTargets`、`validationRecipes` を読み、**unsupportedClaims** と `3d-playable-desktop-package` の `unsupportedClaims` を突き合わせる。
4. **ソースツリー検証:** `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`（ゲーム README / `aiWorkflow.validate` に準拠）。
5. **シェーダーツールチェーン:** `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`（D3D12 パッケージスモークが DXIL を要求する前段）。
6. **インストール済みデスクトップランタイムパッケージ:** `tools/package-desktop-runtime.ps1 -GameTarget <Name>`（`game.agent.json` の `packagingTargets` と CMake メタデータの `PACKAGE_SMOKE_ARGS` が正のソースである）。
7. **ランタイムシーンパッケージ検証:** `runtimeSceneValidationTargets` に列挙された id ごとに `validate-runtime-scene-package` を **DryRun → Execute** の順で走らせる（[docs/workflows.md](../../workflows.md) Editor Playtest Package Review Loop のゲート順と整合）。
8. **ソース資産の変更が発生した場合のみ:** `register-source-asset` → `cook-registered-source-assets` → `migrate-scene-v2-runtime-package` の順で、**選択行のみ**をレビューした上で適用する（ブロードクックやランタイムでのソースパースは unsupported）。
9. **厳密 Vulkan パッケージ:** ホストが `hostGates` を満たす場合のみ、マニフェストが指す strict Vulkan 検証レシピを追加実行する。

## 生成物とサンプルの対応関係（インベントリ）

| 観点 | 生成 `DesktopRuntime3DPackage`（`new-game.ps1`） | 参照サンプル `games/sample_desktop_runtime_game` |
| --- | --- | --- |
| `productionRecipe` | `3d-playable-desktop-package` | 同左（マニフェスト上の正典レシピ id） |
| シーンメッシュ計画テレメトリ | `main.cpp` 内 `scene_mesh_plan_*`（生成テンプレート） | サンプル実装と同一系の計画呼び出し |
| パッケージスモーク引数 | `PACKAGE_SMOKE_ARGS` に D3D12 シェーダー・シーン GPU・ポストプロセス等 | CMake 登録メタデータがソースオブトゥルース |
| プレハブシーン著者記述子 | `prefabScenePackageAuthoringTargets` 行を生成 | コミット済みゲームと同型のループ契約 |

## 検証

| チェック | コマンド / 成果物 |
| --- | --- |
| リポジトリゲート | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` |
| エージェント契約 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`（マニフェスト変更時） |

## ステータス

**完了（2026-05-05）。** v1 スコープ: 上記オペレータレシピの正典化、`docs/ai-game-development.md` および `docs/workflows.md` からの相互参照、`engine/agent/manifest.json` の `currentActivePlan` / `recommendedNextPlan` の次スライスへの移行、本ファイルへの検証証跡記録。

## 実装ステップ

- [x] `new-game.ps1` / 生成 `game.agent.json` / CMake 登録と `3d-playable-desktop-package` の整合インベントリ。
- [x] オペレータ向け手順の本ファイルへの集約と `docs/ai-game-development.md` へのリンク節追加。
- [x] `docs/workflows.md` から本計画への参照追加。
- [x] `docs/superpowers/plans/README.md` のアクティブ／完了テーブル更新。
- [x] `engine/agent/manifest.json` の production-loop ポインタ更新。
- [x] マスター計画 Current Verdict のマニフェスト非同期一文の是正。
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` 証跡の記録。

### Validation evidence

| コマンド | 結果 | 備考 |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Windows MSVC `dev` preset、`validate: ok`（本変更適用直後） |

---

*計画作成: 2026-05-05。スライス id: `generated-static-3d-production-game-recipe-v1`（Phase 6 / 運用正典化）。*
