# Native RHI Resource Lifetime Migration v1 Implementation Plan (2026-05-03)

> **For agentic workers:** `rendering-change` と `gameengine-agent-integration` を参照。本計画は Phase 3 の **`native-rhi-resource-lifetime-migration-v1`** エントリであり、**レジストリ主導のリソース寿命を実バックエンドへ広げる前**に、決定論的テスト路である `NullRhiDevice` で接続パターンを固定する。

**Goal:** `RhiResourceLifetimeRegistry` を **`NullRhiDevice` のリソース寿命**（バッファ／テクスチャに加え Phase E からサンプラー・シェーダー・ディスクリプタ／パイプライン。作成、`null_mark_*_released`、`release_transient`、デバイス破棄時の未解放フラッシュ）に結線し、フェンス完了値に基づく `retire_released_resources` を **`submit` / `wait`** から呼び出して遅延解放を確実に消化する。

**Architecture:** `mirakana::rhi::NullRhiDevice` が所有する `RhiResourceLifetimeRegistry` を単一の真実とし、各ハンドル種に対応する `RhiResourceHandle` を並列ベクタで保持する。ネイティブ COM / `VkDevice` 破棄は D3D12／Vulkan の **バッファ／テクスチャ**および Phase F で拡張するパイプライン類で扱う。公開 API に `ID3D12*` 等は出さない。

**Tech Stack:** `engine/rhi`（`null_rhi.cpp`、`rhi.hpp`）、`mirakana/rhi/resource_lifetime.hpp`、`tests/unit/rhi_tests.cpp`。

---

## Phases

### Phase A（本スライス・完了）

- [x] `create_buffer` / `create_texture` で `register_resource` し、並列 `buffer_lifetime_` / `texture_lifetime_` に `RhiResourceHandle` を格納する。
- [x] `null_mark_buffer_released` / `null_mark_texture_released` と `release_transient` で `release_resource_deferred` + `retire_released_resources(completed_fence)` を実行する。
- [x] `submit` / `wait` のフェンス更新後に `retire_resource_lifetime_to_completed_fence()` を呼ぶ。
- [x] `~NullRhiDevice` でアクティブなバッファ／テクスチャを解放フレーム 0 に積み、`UINT64_MAX` で一括 `retire` する。
- [x] `IRhiDevice::resource_lifetime_registry()` をポインタ API に統一（未対応バックエンドは `nullptr`）。`NullRhiDevice` は `override` で返却。
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` 証跡。

### Phase B（D3D12 バッファ／テクスチャ・完了）

- [x] `D3d12RhiDevice::create_buffer` / `create_texture` で `register_resource`、並列 `buffer_lifetime_` / `texture_lifetime_`。
- [x] `DeviceContext::last_submitted_fence` と `release_transient` での `release_resource_deferred`（GPU 完了は「最後にシグナルしたフェンス値」まで待つ契約）。
- [x] `DeviceContext::destroy_committed_resource`（RTV/DSV ヒープと `ID3D12Resource` の解放、スロットはトゥームストーン）。
- [x] `submit` / `wait` 後に `completed_fence` に基づき `retire_deferred_committed_resources`（ネイティブ破棄 → `retire_released_resources`）。
- [x] `~D3d12RhiDevice` で最終フェンス待ち後、未解放をフレーム 0 で積み `UINT64_MAX` で一括 retire。
- [x] `committed_resources_alive` を増分カウンタに変更（破棄と対称）。
- [x] `rhi_tests` 更新、`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`（`mirakana_d3d12_rhi_tests` 含む CTest 28/28）。

### Phase C（Vulkan バッファ／テクスチャ・完了）

- [x] `VulkanRhiDevice::create_buffer` / `create_texture` で `register_resource`、並列 `buffer_lifetime_` / `texture_lifetime_`。
- [x] `release_transient` で `stats_.last_submitted_fence_value` を `release_frame` に使用し、即時 `reset()` を廃止（フェンス完了相当値での `retire_deferred_native_resources`）。
- [x] `submit` / `wait` 後に `retire_deferred_native_resources(stats_.last_completed_fence_value)`（完了値は Phase D で実フェンス待ちに更新）。
- [x] `~VulkanRhiDevice` で Null/D3D12 と同型のフラッシュ（解放フレーム 0 + `UINT64_MAX` retire）。
- [x] `resource_lifetime_registry()` override。
- Metal の `IRhiDevice` 実装は本リポジトリに未搭載のため本計画スコープ外。

### Phase D（Vulkan：`vkWaitForFences` による GPU 完了とタイムライン整合）

- [x] デバイス初期化で `vkWaitForFences` を必須取得し、`VulkanRuntimeDevice` から `wait_for_fence_signaled` で呼び出し可能にする。
- [x] `submit` 成功後にタイムライン値を `pending_gpu_timeline_`（`std::deque`）へ積み、非 present 経路では `VulkanRuntimeFrameSync` を **move** してインフライトフェンスを `wait()` / デストラクタで `vkWaitForFences` するまで保持する。
- [x] `advance_gpu_timeline_completion` で先頭エントリから順に `vkWaitForFences`（無限タイムアウト）し、`stats_.last_completed_fence_value` を **実際の完了** に合わせて更新する（CPU 側の値だけを進める処理は削除）。
- [x] `wait()` は `advance` と `retire_deferred_native_resources` のみとし、`~VulkanRhiDevice` 先頭でタイムラインを `UINT64_MAX` まで drain する。
- [x] 公開ヘッダではフェンスを `std::uint64_t` の不透明ハンドルとして表現し（`NativeVulkanFence` は cpp のみ）、`check-public-api-boundaries` に抵触しないコメントにする。
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`（`validate: ok`、CTest 28/28）。

### Phase E（Null：`sampler` / `shader` / ディスクリプタ／パイプラインのレジストリ結線・完了）

- [x] `create_sampler` / `create_shader` / `create_descriptor_set_layout` / `allocate_descriptor_set` / `create_pipeline_layout` / `create_graphics_pipeline` で `register_resource` し、並列 `*_lifetime_` ベクタへ格納する。
- [x] `null_mark_sampler_released` ほか既存の `null_mark_*` で `release_resource_deferred` + `retire_resource_lifetime_to_completed_fence()` を実行する。`null_mark_shader_released` / `null_mark_graphics_pipeline_released` を追加する。
- [x] `owns_shader` / `owns_graphics_pipeline` を `*_active_` フラグで判定し、安全点ティアダウンと整合する。
- [x] `~NullRhiDevice` で上記種別の未解放をフレーム 0 に積み `UINT64_MAX` で一括 `retire` する。
- [x] `mirakana_rhi_tests` にレジストリ検証を追加し、`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` 証跡を更新する。

### Phase F（D3D12 / Vulkan：パイプライン類の遅延ネイティブ破棄・完了）

- [x] D3D12：`create_sampler` / `create_shader` / `create_descriptor_set_layout` / `allocate_descriptor_set` / `create_pipeline_layout` / `create_graphics_pipeline` で `register_resource` と並列 `*_lifetime_` / `*_active_` を結線（失敗時はネイティブ行のロールバック付き）。`retire_deferred_committed_resources` は `deferred_d3d12_destroy_rank` で安定ソートし、グラフィックス PSO → ルートシグネチャ → ディスクリプタセット（サブアロケーションはトゥームストーン）→ DSL（CPU のみ）→ シェーダーバイトコード行 → サンプラー（記述のみ）→ コミット済みバッファ／テクスチャの順でネイティブ破棄後に `retire_released_resources`。`~D3d12RhiDevice` は Vulkan と同順で未解放を `release_resource_deferred(..., 0)` に積み `UINT64_MAX` でフラッシュ。`D3d12RhiCommandList` はパイプライン／ルートシグネチャ／ディスクリプタセットの `*_active_` を検証。
- [x] Vulkan：`retire_deferred_native_resources` で `vkDestroyPipeline` / `vkDestroyPipelineLayout` / ディスクリプタセット・DSL / `vkDestroyShaderModule` / `vkDestroySampler` をフェンス完了タイムラインに沿った順で破棄（`vulkan_backend.cpp`）。
- [ ] Metal：`IRhiDevice` 実装がリポジトリに存在してから本計画の子スライスで追従する（Apple ホスト前提）。

## Validation Evidence

| コマンド | 結果 | 日付 |
| --- | --- | --- |
| `ctest -C Debug -R mirakana_rhi_tests` | Passed | 2026-05-03 |
| `cmake --build --preset dev --target mirakana_rhi_d3d12 mirakana_d3d12_rhi_tests mirakana_rhi_vulkan mirakana_rhi_tests` | Passed | 2026-05-03 |
| `ctest -C Debug -R "mirakana_d3d12_rhi_tests|mirakana_rhi_tests"` | Passed | 2026-05-03 |
| `ctest -C Debug -R mirakana_rhi_resource_lifetime_tests` | Passed | 2026-05-03 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | `validate: ok`（exit 0） | 2026-05-03 |

## Done When（Phase A）

- [x] Null バックエンド上でレジストリ行が作成・解放イベントと一致する。
- [x] 既存の `NullRhiDevice` セマンティクス（`owns_*` は非アクティブで偽）を維持する。

## Non-Goals

- Metal の `IRhiDevice` 実装が存在しないため、Metal 向け寿命結線は **Phase F**（Apple ホスト上の別実装）まで保留。
- D3D12 / Vulkan の **パイプライン・ディスクリプタ・サンプラーの遅延ネイティブ破棄**は **Phase F**（アリーナ回収・破棄順序の設計が必要）で扱う。Phase E までで Null の全対象種別はレジストリと一致。

---

*Phase A completed: 2026-05-03. Phase B (D3D12 buffer/texture lifetime) completed: 2026-05-03. Phase C (Vulkan buffer/texture lifetime) completed: 2026-05-03. Phase D (Vulkan `vkWaitForFences` + pending GPU timeline) completed: 2026-05-03. Phase E (Null extended resource kinds lifetime) completed: 2026-05-03. Phase F (D3D12/Vulkan pipeline-class deferred native destroy) completed: 2026-05-03.*
