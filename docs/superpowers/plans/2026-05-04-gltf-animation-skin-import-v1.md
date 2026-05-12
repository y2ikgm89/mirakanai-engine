# glTF アニメーション・スキン取り込み v1 (2026-05-04)

**計画 ID:** `gltf-animation-skin-import-v1`
**ステータス:** **完了**（ツール層の観測・拒否・`mirakana_animation` 型への決定的インポート API まで。クック済み JSON 行の正典化とランタイムマウントは `gpu-skinning-upload-and-rendering-v1` 側で継続。）
**親:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6
**方針:** [Khronos glTF 2.0 仕様](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html) に沿った読み取り契約。オプションの `fastgltf` 経路のみ。後方互換の shim は置かない（未対応は明示的に拒否または診断）。

## ゴール

- glTF の `skins` / `animations` を、エンジン側の `**mirakana_animation` 公開モデル**（`AnimationSkeletonDesc` / `AnimationSkinPayloadDesc` / `AnimationJointTrackDesc`）へ **ツールから決定的に**マッピングできること。
- スキニング付きメッシュを **誤って静的メッシュとしてクックしない**（サイレント破損の防止）。

## 制約

- インポート実装は `mirakana_tools` の外部アセットアダプタに限定（`manifest.json` の importer レーンと整合）。
- GPU スキニング実行・RHI バッファ契約は `gpu-skinning-upload-and-rendering-v1` に委譲（本計画では実行を主張しない）。
- Lit 接線空間は `MeshSource`/`CookedMesh` v2 + `TANGENT` 必須（[2026-05-04-cooked-mesh-tangent-frame-schema-v1.md](2026-05-04-cooked-mesh-tangent-frame-schema-v1.md)）。

---

## Phase A — 観測と明示拒否（完了）

**完了条件（証跡）**

- `mirakana::inspect_gltf_skin_animation`（`mirakana/tools/gltf_skin_animation_inspect.hpp`）: `skins` / `animations` の要約、三角形プリミティブで `JOINTS_0` かつ `WEIGHTS_0` を持つ件数。
- `mesh_document_from_gltf`: `JOINTS_0` または `WEIGHTS_0` があるプリミティブは `**std::runtime_error`**（診断に `skinning` を含む）。
- `mirakana_tools_tests`: 上記のユニットテスト（インポータ有効時のみ実行）。
- `engine/tools/CMakeLists.txt` / `manifest.json` `mirakana_tools.publicHeaders` にヘッダ登録。
- `cmake --build` → `mirakana_tools` / `mirakana_tools_tests` 成功。

**RED → GREEN:** 合成最小 glTF JSON + data URI バッファでインスペクト件数とインポート失敗を検証。

---

## Phase B — スキン・IBM・ジョイント階層（完了）

- `extract_gltf_skin_bind_data` — ジョイント glTF ノード索引 + IBM（float `MAT4`）→ `mirakana::Mat4`、省略時は単位行列、件数検証、`mirakana_tools_tests`。
- `import_gltf_skin_skeleton_and_skin_payload` — `skins[skin_index]` のノード局所 TRS（`Options::DecomposeNodeMatrices`）から `AnimationSkeletonDesc`（**Z 軸回転成分のみ**のクォータニオンを許可、それ以外は診断拒否）、`JOINTS_0`/`WEIGHTS_0` から `AnimationSkinPayloadDesc`（`normalize_animation_skin_payload` まで）、`validate_animation_skin_payload` ゲート。
- **（後続スライス）** クック済みソース／ランタイム資産としての JSON スキーマ行と `check-json-contracts` — GPU スキニング／パッケージングの文脈で扱う。

---

## Phase C — アニメーションサンプル（完了）

- `import_gltf_animation_joint_tracks_for_skin` — 選択スキンのジョイントノードを対象とする `animations[animation_index]` チャンネルのみ取り込み。`LINEAR` のみ。`translation`→`Vec3Keyframe`、`rotation`→クォータニオンを **Z 軸回転**に射影して `rotation_z_keyframes`、`scale`→正の `Vec3Keyframe`。重複チャンネル・非対象ノード・空結果は拒否。`is_valid_animation_joint_tracks` ゲート。

---

## 検証

- 実装変更後: `mirakana_tools_tests` およびリポジトリ既定の `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`。
- 本ファイル更新時も `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` を記録する。

## 証跡ログ


| 日付         | 内容                                                                                         |
| ---------- | ------------------------------------------------------------------------------------------ |
| 2026-05-04 | Phase A 着地: `gltf_skin_animation_inspect`、メッシュ取り込みのスキニング拒否、`tools_tests`、ビルド成功。            |
| 2026-05-04 | Phase B 部分: `extract_gltf_skin_bind_data`（ジョイントノード索引 + IBM→`Mat4`）、`mirakana_tools_tests` 3 件追加。 |
