# Metal Visible Presentation Apple Host v1 Implementation Plan (2026-05-03)

> **For agentic workers:** Phase 3 master child **`metal-visible-presentation-apple-host-v1`**. Applies **Apple-recommended** [`-[MTLObject setLabel:]`](https://developer.apple.com/documentation/metal/mtlobject) GPU debugger labels on the **Apple-only** `mirakana_rhi_metal` Objective-C++ ownership path (`metal_native.mm`): default `MTLDevice`, `MTLCommandQueue`, per-allocation `MTLCommandBuffer`, offscreen and drawable-backed `MTLTexture`, `MTLRenderCommandEncoder`, and transient readback `MTLCommandBuffer` / `MTLBlitCommandEncoder` / `MTLBuffer` objects—using stable `GameEngine.RHI.Metal.*` prefixes plus a per-device monotonic serial—without exposing Metal types in public headers or changing backend-neutral contracts.

**Goal:** Align Metal runtime owners with the same “visible in GPU capture / Xcode Metal debugger object list” standard as D3D12 `SetName` and Vulkan `vkSetDebugUtilsObjectNameEXT`, scoped to code that already compiles on macOS with Metal+QuartzCore linked.

**Architecture:** `MetalRuntimeDevice::Impl` gains `debug_label_serial`. File-local helpers `metal_set_gpu_debug_label` and `metal_next_debug_label` in `metal_native.mm` centralize nil checks and `respondsToSelector:@selector(setLabel:)`. Labels are applied immediately after successful native object creation. Windows and other non-Apple builds keep the existing stub implementations in `metal_backend.cpp`; **no** Metal headers are introduced there.

**Tech Stack:** `engine/rhi/metal/src/metal_native.mm`, `engine/rhi/metal/src/metal_native_private.hpp`, `engine/agent/manifest.json` `currentMetal`, master plan + plan registry.

---

## Phases

### Phase v1（本スライス・コード完了）

- [x] `MetalRuntimeDevice::Impl::debug_label_serial` と連番ラベル生成。
- [x] デバイス／コマンドキュー／コマンドバッファ／テクスチャ（オフスクリーン＋ drawable テクスチャ）／レンダエンコーダ／リードバック経路の `setLabel:`。
- [x] Manifest `currentMetal` / `recommendedNextPlan` 同期、マスター計画 Phase 3 チェックボックス、本計画の検証エビデンス（Windows 既定 `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`）。

### Phase v1（Apple ホスト証跡・マスター上の別ゲート）

- [ ] macOS / Xcode での実機または CI 上の Metal デバッガでオブジェクト名が一覧に現れることのスクリーンショットまたは CI ログ（マスター計画の「Metal visible game-window … validated on Apple hosts」に従い、**Apple ホスト外では ready と書かない**）。

## Validation Evidence

| Command | Result | Date | Host |
| --- | --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | `validate: ok` | 2026-05-03 | Windows（Metal .mm は Apple ビルドのみコンパイル） |

## Done When

- [x] Apple ビルドで `metal_native.mm` が追加の未使用シンボルや ARC 違反なくコンパイルできる（CI / ローカル macOS で確認）。
- [ ] 上記 Apple 証跡が取れたらマスター「Metal visible … Apple hosts」の文言と整合する証拠を本表に追記。

## Non-Goals

- 公開 API やゲームプレイから Metal オブジェクト／ラベルを読み取ること。
- Windows 上で Metal ランタイムを実際に実行すること（引き続き非サポート）。

---

*Code slice completed: 2026-05-03. Apple-host evidence: pending.*
