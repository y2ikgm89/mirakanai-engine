# CMake Install Export And C++ Modules Audit v1 Implementation Plan (2026-05-03)

> **For agentic workers:** `cmake-build-system` を必ず参照。`import std` / CXX モジュールスキャンはツールチェーン依存が強いため、変更は **根拠とゲート**をセットで残す。

**Goal:** `cmake --install` / `export()` / `install(EXPORT …)`（またはプロジェクトが採用する同等の消費者向け CMake パッケージ）と **C++20/23 モジュール**（`MK_ENABLE_CXX_MODULE_SCANNING`、`MK_ENABLE_IMPORT_STD` 等）の設定を監査し、**公式 CMake / コンパイラドキュメント**に沿って一貫した推奨状態にする。Phase 1 の「後続 Phase がビルド上で迷子にならない」下地を整える。

**Architecture:** 現状 Windows VS プリセットでは `import std` がジェネレータにより無効化されるメッセージがある（既知）。本スライスでは **インストールツリー、Imported ターゲット、モジュール単位のビルド境界**を整理し、不要な二重定義や非推奨パターンを除去する。

**Tech Stack:** CMake 3.x、Ninja / Visual Studio ジェネレータ、MSVC / Clang、既存 `CMakePresets.json`。

---

## Goal

- `install` + `export`（または明示的な「インストール非サポート」方針）が **ドキュメントと CMake で一致**している。
- C++ モジュール関連オプションが **サポートされる組み合わせのみ**で有効化され、非対応組み合わせは **設定時に明確なメッセージ**で失敗または自動オフになる。

## Context

- [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 1。
- [2026-05-02-strict-clang-tidy-compile-database-enforcement-v1.md](2026-05-02-strict-clang-tidy-compile-database-enforcement-v1.md) の File API 合成と整合する。
- `engine/agent/manifest.json` の `currentActivePlan` は本ファイルを指す（2026-05-03、`full-clang-tidy-warning-cleanup-v1` 完了後の次ランク）。

## Constraints

- 後方互換のための「空の install ターゲット」や無効な `INTERFACE` 偽装は禁止。
- 公開インストールを追加する場合、**ライセンス・再配布ポリシー**に適合する成果物のみ。

## Implementation Steps

- **Step 1:** 現行の `install()` / `export()` / `CMakePackageConfigHelpers` の有無を監査し、Evidence に一覧化する。
- **Step 2:** 消費者（内部ツール / 将来の editor / package）が必要とする **最小の Exported ターゲット集合**を定義する。
- **Step 3:** `MK_ENABLE_CXX_MODULE_SCANNING` / `MK_ENABLE_IMPORT_STD` とジェネレータ・コンパイラの組み合わせを表形式で整理し、非対応時の `message()` 方針を統一する。
- **Step 4:** `AGENTS.md` / `docs/building.md`（存在すれば）にインストール手順とモジュール前提を追記する。

## Audit Evidence (Step 1)

| 場所 | 内容 |

| --- | --- |

| ルート `CMakeLists.txt` | `include(CMakePackageConfigHelpers)`、`install(TARGETS ${MK_LIBRARY_TARGETS} EXPORT GameEngineTargets …)`、`install(EXPORT GameEngineTargets … NAMESPACE mirakana::)`、`configure_package_config_file(cmake/GameEngineConfig.cmake.in …)`、`write_basic_package_version_file(… ExactVersion)`、`install(FILES … GameEngineConfig*.cmake)`、`install(DIRECTORY … include/` ほか条件付き SDL/RHI ヘッダ）、`install(TARGETS ${MK_INSTALL_RUNTIME_TARGETS})`（ランタイムのみ・EXPORT なし）、`install(DIRECTORY docs examples games schemas tools LICENSES …)`、CPack |

| `cmake/GameEngineConfig.cmake.in` | `@PACKAGE_INIT@`、`find_dependency(SDL3|SPNG|fastgltf)` をビルド時フラグでゲート、`include(GameEngineTargets.cmake)` |

| `games/CMakeLists.txt` | 生成ゲームごとの `install(FILES …)`（テンプレート／設定の選別インストール）。エンジンの EXPORT とは別レイヤー |

| `export()` 単体 | 未使用。公式推奨に沿い `**install(EXPORT …)` のみ**でエクスポート設定を生成 |

## Minimal exported target set (Step 2)

`GameEngineTargets.cmake` に載るのは `**MK_LIBRARY_TARGETS`** のみ（`install(… EXPORT GameEngineTargets)`）。固定コアは `MK_ai` … `MK_ui_renderer`（ルート `CMakeLists.txt` のリスト）。存在時のみ追加: `MK_rhi_d3d12`, `MK_audio_sdl3`, `MK_platform_sdl3`, `MK_runtime_host_sdl3`, `MK_runtime_host_sdl3_presentation`, `MK_editor_core`。短い `**EXPORT_NAME**`（例 `mirakana::core`）が `MK_set_export_name` で設定済み。

ランタイム実行ファイルは `**install(TARGETS ${MK_INSTALL_RUNTIME_TARGETS})` に EXPORT を付けず**バイナリのみ（find_package コンシューマがリンクしない前提）。

## Tests

- `cmake --preset dev`（または対象プリセット）での設定・ビルド。
- インストールを有効にする場合: `cmake --install` スモーク（プレフィックス指定）。

## Validation

| コマンド | 期待 |

| --- | --- |

| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` 内の CMake ステップ | 既存ポリシーに沿って成功 |

| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | manifest / ドキュメント整合（変更時） |

## Validation Evidence

| コマンド | 結果 | 証跡 |

| --- | --- | --- |

| `cmake` configure（モジュール方針ログ） | `GameEngine CXX modules` / `GameEngine import std` の STATUS 行が出力 | ルート `CMakeLists.txt` 更新 |

| `docs/building.md` | 追加済み（インストール・find_package・マトリクス） | 本リポジトリ |

| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | 実行して確認（環境で完走） | ローカル |

| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | `json-contract-check: ok` | エージェント実行時 |

## Done When

- インストール／エクスポート方針が CMake と文書で一致。
- C++ モジュール関連フラグの **支持マトリクス**が文書化され、誤用時に早期に気づける。
- 本ファイルの各セクションが実態と一致している。

## Non-Goals

- vcpkg 全体のポリシー変更。
- 全ターゲットの `CXX_MODULES` 化（別ロードマップ）。

---

*Plan authored: 2026-05-03. Slice completed: docs/building.md、AGENTS.md、`CMakeLists.txt` module/install comments aligned.*
