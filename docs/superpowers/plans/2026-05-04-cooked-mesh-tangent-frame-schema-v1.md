# Cooked mesh tangent frame schema v1 実装計画 (2026-05-04)

> **エージェント向け:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6 子計画 `cooked-mesh-tangent-frame-schema-v1`。接線空間メッシュの **ソース／クック／ランタイム／RHI／サンプルシェーダ** を **後方互換なし** で v2 に統一する。`engine/agent/manifest.json` の `currentActivePlan` は本完了スライスを指す（次スライスは `recommendedNextPlan`）。

## ゴール

- **Lit メッシュ**（`NORMAL` + `TEXCOORD_0` を持つ glTF プリミティブ、および対応する `MeshSourceDocument`）は、**接線フレーム必須**（`has_tangent_frame == true`）とし、頂点は **48 バイト／頂点**の接線空間インターリーブ（position `float3`、normal `float3`、uv `float2`、tangent `float4`、`**w` に従法線の符号**＝[Khronos glTF 2.0](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes) の `TANGENT` と整合）。
- **位置のみメッシュ**は `has_normals == has_uvs == has_tangent_frame == false`、ストライド **12**。
- `**GameEngine.MeshSource.v2` / `GameEngine.CookedMesh.v2` のみ**を正とし、v1 文字列・レジストリ・テスト期待から **shim を置かずに除去**する。

## コンテキスト

- Phase 6 マスターは「tangent-space material rendering requires cooked mesh tangent-frame schema」を前提としていた。PBR／サンプル `runtime_scene.hlsl` は既に lit 頂点入力を持つため、**クック済みバイト列とメタデータの契約**を先に固定する必要がある。
- [2026-05-06-gltf-mesh-editor-ux-v1.md](2026-05-06-gltf-mesh-editor-ux-v1.md) の非ゴールだった「接線スキーマ」は本スライスで扱う（エディタ glTF UX と独立して完了可能）。

## 制約

- **後方互換用 shim は追加しない**（v1 ペイロードの受理・自動変換は行わない）。
- glTF 側は **オーサリング済み `TANGENT`（`VEC4` / `FLOAT`）必須**とし、MikkTSpace 再計算やランタイム生成は本スライス非ゴール（将来スライスで検討）。
- ゲームプレイ公開 API に RHI／中間表現の生ハンドルを出さない（既存方針の維持）。

## 非ゴール（本スライス）

- GPU スキニング、CPU スキン結果のランタイムメッシュへの書き戻し、パッケージストリーミング。
- `gltf-animation-skin-import-v1` 相当のアニメーション／スキン取り込み（マニフェスト上の次候補）。

## 実装チェックリスト（完了）

- `mirakana_assets`: `MeshSourceDocument::has_tangent_frame`、`mesh_source_vertex_stride_bytes()`、シリアライズ／デシリアライズ、`is_valid_mesh_source_document`（法線と UV の両方が真のとき接線必須など）。
- `GameEngine.MeshSource.v2` / `GameEngine.CookedMesh.v2` レジストリとランタイム `RuntimeMeshPayload` の `has_tangent_frame`。
- glTF 取り込み: lit で `TANGENT` 欠落は明確なエラー；クック出力は v2。
- `mirakana_runtime_rhi`: 接線レイアウト（ストライド 48）、部分レイアウト拒否、サンプルシェーダ `VsIn` の `TANGENT`。
- サンプルゲーム資産、`tools/new-game.ps1`、`check-ai-integration` / `check-json-contracts`、エディタ既定 `default.mesh`（位置のみ）、ユニットテスト群の v2 化。
- `engine/agent/manifest.json` の importer 既定ドキュメント／`mirakana_runtime_rhi` purpose 文言のストライド記述更新。
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`（`validate: ok`）。

## 検証


| チェック     | コマンド / 成果物                                                                                                                |
| -------- | ------------------------------------------------------------------------------------------------------------------------- |
| リポジトリゲート | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`（ビルド + CTest + 静的チェック）                                                                                  |
| ユニット     | `mirakana_core_tests`（メッシュソース）、`mirakana_tools_tests`（クック／glTF）、`mirakana_runtime_rhi_tests` / `mirakana_runtime_scene_rhi_tests`、関連 `mirakana_*_tests` |


## ステータス

**完了（2026-05-04）:** `MeshSource` / `CookedMesh` v2 と接線必須規則、ランタイム RHI 48 バイト接線空間頂点、サンプル・スクリプト・マニフェスト整合を実装済み。v1 形式はリポジトリ契約から除去。

### Validation evidence


| コマンド               | 結果   | 備考                                   |
| ------------------ | ---- | ------------------------------------ |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Windows MSVC `dev` プリセット、CTest 29/29 |


---

*計画起票・完了記録: 2026-05-04。スライス id: `cooked-mesh-tangent-frame-schema-v1`（Phase 6）。*
