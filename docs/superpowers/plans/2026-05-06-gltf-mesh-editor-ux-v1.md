# glTF メッシュエディタ UX v1 実装計画 (2026-05-06)

> **エージェント向け:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6 子計画 `gltf-mesh-editor-ux-v1`（**Phase A+B 完了**）。`engine/agent/manifest.json` の `currentActivePlan` は完了後スライス（現状: [2026-05-04-gpu-skinning-upload-and-rendering-v1.md](2026-05-04-gpu-skinning-upload-and-rendering-v1.md)）を指す。本ファイルは実装証跡の正典。

## ゴール

- エディタ上で glTF 由来メッシュを **閲覧・選択・マテリアル割当**できる製品水準の UX のうち、**`MK_editor_core` で表現可能な最小モデル**から段階的に実装する。

## コンテキスト

- Phase 6 マスターは「glTF mesh editor UX and material assignment are productized」を完了定義に含む。
- ランタイム側の静的メッシュパッケージ道は [2026-05-05-generated-static-3d-production-game-recipe-v1.md](2026-05-05-generated-static-3d-production-game-recipe-v1.md) および既存 3D パッケージ著者ループと接続されるが、**本スライスはエディタ UX が主担当**である。
- `MK_tools` は optional `fastgltf` 経由で glTF メッシュの **クック用インポート**を既に提供する（`GltfMeshExternalAssetImporter`）。本スライスはそれと分離した **読み取り専用の列挙面**を追加し、エディタがクックやシーン変更なしにプレビュー行を組み立てられるようにする。

## 制約

- **後方互換用 shim は追加しない**。
- `MK_editor_core` は GUI 非依存モデルに留め、**公開ヘッダやコメントに Dear ImGui を露出しない**（`check-public-api-boundaries`）。
- 任意シェル・未レビューのパッケージスクリプト実行・生コマンド評価は editor/AI 契約上 forbidden。

## 非ゴール（v1 全体）

- フル機能 glTF アニメーション／スキン取り込み（別子計画 `gltf-animation-skin-import-v1`）。
- GPU スキニング・ランタイムでの接線空間 **スキンメッシュ** 本番完了（`cooked-mesh-tangent-frame-schema-v1` で静的クック済み接線契約は完了済み）。

## Phase A（インフラ・完了）

- [x] `mirakana::inspect_gltf_mesh_primitives`（`engine/tools/include/mirakana/tools/gltf_mesh_inspect.hpp` + `gltf_mesh_inspect.cpp`）: 三角形プリミティブごとにメッシュ名・頂点数・索引有無・NORMAL/TEXCOORD_0 の有無を列挙。`MK_ENABLE_ASSET_IMPORTERS=OFF` 時は `parse_succeeded=false` と安定診断文字列のみ返す。
- [x] `mirakana::editor::gltf_mesh_inspect_report_to_inspector_rows`（`gltf_mesh_catalog.hpp` / `.cpp`）: `EditorPropertyRow` への決定的写像（失敗時は 1 行の status、成功時は mesh + layout 行、警告は各行）。
- [x] `MK_tools_tests` / `MK_editor_core_tests` にインポータ有効時の三角形 glTF 合成データによる検証（無効時は早期 return）。
- [x] `engine/agent/manifest.json` の `MK_tools` / `MK_editor_core` `publicHeaders` 登録。
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`（`validate: ok`）。

## Phase B（完了）

- [x] `MK_editor`（Dear ImGui）から glTF パス選択 → `inspect` → インスペクタ行の表示（既存インスペクタパネルとの結線）。プロジェクトルート相対パス、`.gltf` はテキストストア経由、`.glb` はバイナリ読み取り。Assets で `.gltf`/`.glb` が選択されていればパスを流し込み可能。
- [x] `SceneAuthoringDocument` の `mesh_renderer` へのマテリアル／メッシュ割当と `make_scene_authoring_component_edit_action` / `UndoStack` 統合（コンテンツブラウザで mesh / material を選択した状態からワンクリック割当）。

## 検証

| チェック | コマンド / 成果物 |
| --- | --- |
| ユニットテスト | `MK_tools_tests`（`gltf mesh inspect lists indexed triangle primitive when importers are enabled`）、`MK_editor_core_tests`（カタログ写像 2 本） |
| リポジトリゲート | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` |

## ステータス

**Phase A + B 完了（2026-05-04 セッション）:** Phase A に加え、`editor/src/main.cpp` の Inspector で glTF インスペクトと `gltf_mesh_inspect_report_to_inspector_rows` の表示、Mesh Renderer の Assets 選択からの mesh/material 割当（Undo 対応）を実装。**検証:** `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS。

### Validation evidence

| コマンド | 結果 | 備考 |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Windows MSVC `dev` プリセット |

---

*計画起票: 2026-05-06。スライス id: `gltf-mesh-editor-ux-v1`（Phase 6）。*
