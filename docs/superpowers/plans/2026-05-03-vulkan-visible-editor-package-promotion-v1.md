# Vulkan Visible Editor Package Promotion v1 Implementation Plan (2026-05-03)

> **For agentic workers:** Phase 3 master child **`vulkan-visible-editor-package-promotion-v1`**. Aligns the SDK-independent Vulkan RHI path with **Khronos `VK_EXT_debug_utils`** practice: resolve optional `vkSetDebugUtilsObjectNameEXT` via `vkGetDeviceProcAddr` and apply **best-effort** `GameEngine.RHI.Vulkan.*` labels to the logical device, graphics/present queues, swapchains, images, image views, buffers, command pools, primary command buffers, shader modules, pipeline layouts, graphics pipelines, samplers, and related runtime-owned handles—without exposing `Vk*` types in public headers, changing IRhiDevice contracts, or requiring the proc when the loader or ICD omits it.

**Goal:** Give RenderDoc, Validation Layers, and vendor GPU tools stable object names on the same paths exercised by `mirakana_rhi_vulkan` runtime owners and strict desktop smokes, mirroring the D3D12 `SetName` hardening slice.

**Architecture:** `VulkanRuntimeDevice::Impl` stores `VulkanSetDebugUtilsObjectName set_debug_utils_object_name` (nullable), a monotonic `debug_utils_name_serial`, and `apply_debug_utils_object_name` which fills a `NativeVulkanDebugUtilsObjectNameInfoExt` POD matching `VkDebugUtilsObjectNameInfoEXT` on 64-bit ABIs. `create_runtime_device` (Win32 + Linux) resolves the proc after required device commands pass; `vulkan_apply_debug_utils_names_for_device_and_queues` names device and queues (present queue skipped when identical to graphics). File-scope helpers `vulkan_label_runtime_object` / serial suffixes name per-resource creates. `vulkan_backend_command_requests` lists `vkSetDebugUtilsObjectNameEXT` as **optional** (`required: false`) so command-resolution plans stay green when the ICD does not export the entry point.

**Tech Stack:** `engine/rhi/vulkan/src/vulkan_backend.cpp`, `engine/agent/manifest.json` `currentVulkan`, master plan + plan registry.

**Official reference:** [Vulkan Specification — VK_EXT_debug_utils](https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_debug_utils.html) (`vkSetDebugUtilsObjectNameEXT`).

---

## Phases

### Phase v1（本スライス・完了）

- [x] `vkSetDebugUtilsObjectNameEXT` のデバイス側解決と `VulkanRuntimeDevice::Impl` への格納。
- [x] デバイス／グラフィックス／プレゼントキューの固定ラベル付与（同一キューは二重命名回避）。
- [x] コマンドプール、プライマリコマンドバッファ、バッファ、テクスチャ（画像＋ビュー）、リードバックバッファ、サンプラ、シェーダモジュール、パイプラインレイアウト、グラフィックスパイプライン、スワップチェーン＋スワップチェーン画像／ビューへの連番ラベル（proc が null ならスキップ）。
- [x] `vulkan_backend_command_requests` への任意コマンド登録。
- [x] Manifest `currentVulkan` / `recommendedNextPlan` 同期、マスター計画 Phase 3 チェックボックス、本計画の検証エビデンス。

### Follow-ups（別子／次フェーズ）

- デスクリプタプール／セット、セマフォ、フェンス、サーフェス等への拡張。
- エディタ専用パッケージレーンでの可視性検証（ホストゲートのまま）。

## Validation Evidence

| Command | Result | Date |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | `validate: ok` | 2026-05-03 |

## Done When

- [x] `create_runtime_device` が proc を解決できない環境でも必須コマンド計画を失敗させない。
- [x] proc が有効な環境で、上記ランタイム所有者作成直後にオブジェクト名が付与される（ベストエフォート）。

## Non-Goals

- ゲームプレイや公開 API から名前を読み取ること。
- Metal のデバッグラベル同等実装（`metal-visible-presentation-apple-host-v1` へ委譲）。

---

*Completed: 2026-05-03.*
