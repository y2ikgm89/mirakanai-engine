# ソース画像デコードとアトラスパッキング v1 実装計画 (2026-05-04)

> **エージェント向け:** 実装着手前に [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 4 および [2026-05-03-production-gap-selection-v1.md](2026-05-03-production-gap-selection-v1.md) の方針を確認する。`production-gap-selection-v1` は **`currentActivePlan` に置かない**（文書同期専用）。本ファイルが **実装中の狭いスライス**として `engine/agent/manifest.json` の `currentActivePlan` を指す。

## ゴール

- ランタイム／クック／エディタのいずれかで、**公式に許可した入力形式**（例: PNG 等、リポジトリ方針で許可されたデコーダのみ）から **決定的なピクセルバッファまたは中間表現**へデコードする実行経路を、ホスト実装可能な範囲で整備する。
- **2D スプライト／UI アトラス**向けに、既存のタイルマップ／UI アトラス計画（例: [2026-05-02-2d-atlas-tilemap-package-authoring-v1.md](2026-05-02-2d-atlas-tilemap-package-authoring-v1.md)）と矛盾しない **パッキング契約**（サイズ上限、失敗時診断、再現性）を定義し、テストで固定する。

## コンテキスト

- [2026-05-04-postprocess-stack-v1.md](2026-05-04-postprocess-stack-v1.md) 完了後の Phase 4 バックログ候補としてマスター計画に列挙されている（`source-image-decode-and-atlas-packing-v1`）。
- 既存の「アトラスメタデータ」「タイルマップ著者向け」スライスは **著者／メタデータ中心**であり、本スライスは **デコード実行とパッキング実行**の切り分けを明示する。

## 制約

- **後方互換用 shim は追加しない**（[AGENTS.md](../../../AGENTS.md)）。
- ホストゲート（Metal Apple 一般化、strict Vulkan 本番行列など）に依存する検証を **本スライスの完了条件に含めない**。
- 新規サードパーティ依存は **ライセンス監査ポリシー**に従い、不可ならスコープを縮小する。

## 非ゴール（v1）

- 任意フォーマットの万能イメージローダ、ブラウザ級のサンドボックス、ストリーミングテクスチャングの全般。
- 3D プレイアブル縦スライス一式、エディタ製品化、パイプラインキャッシュ全般（別スライス）。
- `GameEngine.UiAtlas.v1` **メタデータ文書**へのデコード結果や自動パック結果の埋め込み（メタデータは引き続きメタのみ）。

## 検証

| チェック | コマンド / 成果物 |
| --- | --- |
| ユニットテスト | `MK_assets_tests`、`MK_tools_tests`（PNG 合成テストは `MK_ENABLE_ASSET_IMPORTERS=ON` 時のみ） |
| リポジトリゲート | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` |

## ステータス

**完了（2026-05-04）。** v1 スコープ: `mirakana::decode_audited_png_rgba8`（`MK_tools`、libspng、`PngTextureExternalAssetImporter` と共有）、`mirakana::pack_sprite_atlas_rgba8_max_side`（`MK_assets`、決定的シェルフ風行パック＋RGBA8 合成、`std::variant` 診断）、`MK_assets_tests` 追加、`docs/dependencies.md` と `ui_atlas_metadata.hpp` コメントの整合、`engine/agent/manifest.json` 公開ヘッダ登録。

## 実装ステップ

- [x] 対象サブシステムの現状調査と最小 API 面の決定（デコードは `MK_tools`、パックは `MK_assets`）。
- [x] デコード／パックの **失敗時診断**（PNG は例外、パックは `SpriteAtlasPackingDiagnostic`）とテスト。
- [x] `docs/dependencies.md` の現状表現を必要最小限で同期。
- [x] 本ファイルの Validation Evidence 表に `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` を記録。

### Validation evidence

| コマンド | 結果 | 備考 |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_assets_tests MK_tools_tests` | PASS | MSVC `dev` |
| `ctest -C Debug -R "MK_assets_tests|MK_tools_tests"` | PASS | |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Windows 2026-05-04 |

---

*計画作成: 2026-05-04。スライス id: `source-image-decode-and-atlas-packing-v1`（Phase 4 / バックログ実装）。*
