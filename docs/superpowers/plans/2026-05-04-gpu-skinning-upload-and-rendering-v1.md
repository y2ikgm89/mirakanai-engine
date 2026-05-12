# GPU スキニングのアップロードと描画 v1 (2026-05-04)

**計画 ID:** `gpu-skinning-upload-and-rendering-v1`
**親:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6
**ステータス:** **完了**（2026-05-04）。D3D12 の可視証跡は `RhiFrameRenderer` offscreen readback と `sample_desktop_runtime_game --require-gpu-skinning` package smoke に限定。Vulkan/Metal の GPU skinning 可視証跡や広い skeletal animation production は未主張。
**前提:** [2026-05-04-gltf-animation-skin-import-v1.md](2026-05-04-gltf-animation-skin-import-v1.md) で `MK_animation` 向けの glTF スキン／アニメ取り込み API（メモリ上の `AnimationSkeletonDesc` / `AnimationSkinPayloadDesc` / `AnimationJointTrackDesc`）が利用可能であること。

## ゴール

- スキニング用パレット行列・頂点ストリームを RHI 経由で **決定的にアップロード**し、シェーダでスキンドメッシュを描画する最小経路。
- ランタイムのリソース寿命・ステージング／アップロード実行ポリシー（マスター計画 Phase 4/6）に整合。

## 制約

- 本スライスは **GPU 実行**にフォーカスする。glTF ファイルのクック済み JSON 行やパッケージマウントの正典化は、必要なら本計画の子タスクまたは後続スライスで扱う。

## 完了条件（起票時点の未着手）

- D3D12 主レーンでスキンド三角形（または既存サンプルメッシュ）の可視化証跡。
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` 緑。

## 実装サマリー（2026-05-04）

- `GameEngine.CookedSkinnedMesh.v1` / `RuntimeSkinnedMeshPayload` を静的 `CookedMesh.v2` から分離。
- `MK_runtime_rhi` で skinned vertex/index/joint palette（256B 整列 CB）をアップロードし `SkinnedMeshGpuBinding` を構築。
- `MK_runtime_scene_rhi::build_runtime_scene_gpu_binding_palette` が joint descriptor set layout を **先頭** にマージし、マテリアル root signature を `[material, joint, …optional]` に整列（ホストが追加する shadow receiver 等は joint の後）。
- D3D12 の `RegisterSpace == set_index` に合わせ、`runtime_scene.hlsl` の shadow receiver リソースを `MK_SAMPLE_SKINNED_SCENE_SHADOW_RECEIVER_PS` 付きで `space2` に再配置した `ps_shadow_receiver` バリアントを追加（`sample_desktop_runtime_game_shadow_receiver_skinned.ps.*`）。
- `RhiDirectionalShadowSmokeFrameRenderer` に `shadow_receiver_descriptor_set_index` を追加し、スキンド併用時は set **2** に shadow receiver をバインド。
- `MK_runtime_scene_rhi_tests` にスキンドパッケージのパレット／アップロードバイト検証を追加。
- `tests/unit/d3d12_rhi_tests.cpp` に `upload_runtime_skinned_mesh` + joint palette descriptor set + skinned VS の offscreen readback 証跡を追加し、中心ピクセルと `gpu_skinning_draws` / `skinned_palette_descriptor_binds` を検証。
- `sample_desktop_runtime_game` のパッケージスモークは既存の `--require-gpu-skinning` を継続（`games/CMakeLists.txt`）。

## 検証証跡（エージェント実行環境）

| コマンド | 結果 | メモ |
| --- | --- | --- |
| `cmake --build out\build\dev --target MK_runtime_scene_rhi_tests` | **PASS** | MSVC Debug。 |
| `out\build\dev\Debug\MK_runtime_scene_rhi_tests.exe` | **PASS** | 全テスト緑（スキンド追加 2 件含む）。 |
| `ctest -C Debug -R MK_runtime_scene_rhi_tests` | **PASS** | 同上。 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` | **PASS** | 既定 CTest 29/29。 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | **PASS** | `format-check: ok`。 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` | **PASS** | desktop-runtime CTest 16/16。 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | **PASS** | `public-api-boundary-check: ok`。 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` | **diagnostic-only** | Metal blockers 明示（期待どおり）。 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | **PASS** | `validate: ok`、既定 CTest 29/29。 |

## 完了条件（更新後）

- [x] D3D12 主レーンでスキンド三角形の描画経路（サンプル `--require-gpu-skinning` + joint/shadow descriptor 整合）。
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` 緑（上記ホスト実行）。

## 証跡ログ

| 日付 | 内容 |
| --- | --- |
| 2026-05-04 | 起票: `gltf-animation-skin-import-v1` 完了に伴い `currentActivePlan` を本スライスへ移行。 |
| 2026-05-04 | 実装: joint/shadow descriptor 順序、`shadow_receiver_skinned` PS バリアント、`runtime_scene_rhi` joint 先頭マージ、`MK_runtime_scene_rhi_tests` 追加。`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` / `api-boundary-check` / `shader-toolchain-check` / `ctest -C Debug -R MK_runtime_scene_rhi_tests` をホストで実行し証跡表を更新。 |
| 2026-05-04 | 完了: D3D12 offscreen readback テストと `sample_desktop_runtime_game --require-gpu-skinning` package smoke を確認。`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` PASS（29/29）、`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` PASS、`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` PASS（16/16）、`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS。 |
