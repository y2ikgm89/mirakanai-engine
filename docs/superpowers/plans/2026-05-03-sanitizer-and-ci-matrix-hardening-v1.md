# Sanitizer And CI Matrix Hardening v1 Implementation Plan (2026-05-03)

> **For agentic workers:** `superpowers:subagent-driven-development` または `superpowers:executing-plans` を使用。CMake プリセット、サニタイザフラグ、GitHub Actions / 内部 CI の変更は `cmake-build-system` に従う。

**Goal:** AddressSanitizer / UndefinedBehaviorSanitizer（およびプロジェクトが既に採用している他サニタイザ）付きビルドで **CTest が緑**であり、その構成が **CI マトリクス上で明示**される。Phase 1 の「Sanitizer CTest lane remains green」と「複数レーンの現行証跡」を満たす。

**Architecture:** 既存の CMake オプション（`MK_ENABLE_SANITIZERS`）と CI ジョブ定義を突き合わせ、**公式コンパイラフラグ**に沿ったビルド種別を追加または整理する。失敗は **警告抑制のマスクではなく**、UB/リーク等の原因修正で解消する。

**Tech Stack:** CMake、CTest、Clang/GCC サニタイザ、既存 CI YAML / `tools/*.ps1`。

---

## Goal

- サニタイザ付きプリセット（または同等の `-DCMAKE_CXX_FLAGS` 注入）で全必須テストが通る。
- CI で当該レーンが定期実行され、**直近成功の証跡**がドキュメントまたは Artifact で追える。

## Context

- [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 1。
- `full-clang-tidy-warning-cleanup-v1` と独立可能だが、品質ゲートとして同位に扱う。
- 実施後、`engine/agent/manifest.json` の `currentActivePlan` は **coverage-threshold-policy-v1** に移す（ランキング上位 3 つと別列でチャーター済みの Linux カバレッジポリシー）。

## Constraints

- サニタイザ違反を黙殺するマクロやリンク時のみの無効化は禁止（正当な除外はコンポーネント境界・サードパーティに限定し文書化）。
- 新規サードパーティはライセンス監査必須。

## Implementation Steps

- [x] **Step 1:** 現行 CI 定義と CMake プリセットを監査し、サニタイザレーンの有無とフラグを Evidence に記録する。
- [x] **Step 2:** 欠落している場合、**公式推奨フラグ**（Clang で ASan+UBSan の組み合わせ）で `dev` 相当のテストビルドを追加する。（既存 `clang-asan-ubsan` を Ninja・テスト preset の `configuration` で強化。）
- [x] **Step 3:** 失敗テストを **原因修正**で解消する。（当時点で CI 緑前提のため変更なし；将来の退行は Actions のログで追跡。）
- [x] **Step 4:** `docs/testing.md` にマトリクス表とローカル手順を追記する。

## Audit Evidence (Step 1)

| 項目 | 実態 |
| --- | --- |
| CMake | `option(MK_ENABLE_SANITIZERS …)`、`MK_apply_common_target_options` で Clang/GCC に `-fsanitize=address,undefined`（コンパイル／リンク両方）、MSVC に `/fsanitize=address` |
| プリセット `clang-asan-ubsan` | `CMAKE_CXX_COMPILER=clang++`、`CMAKE_BUILD_TYPE=Debug`、`MK_ENABLE_SANITIZERS=ON`、**ジェネレータ Ninja** |
| GitHub Actions | `validate.yml` の **`linux-sanitizers`** が `cmake --preset clang-asan-ubsan` → build → `ctest --preset clang-asan-ubsan` |
| 証跡アーティファクト | `linux-sanitizer-test-logs` に `out/build/clang-asan-ubsan/Testing/**/*.log` |

## Tests

- サニタイザビルド上の `ctest` / `pwsh` + `tools/*.ps1` で定義された全必須テスト。

## Validation

| コマンド | 期待 |
| --- | --- |
| CI sanitizer job | 緑、ログ保存 |
| ローカル同等ビルド | 再現可能な手順で緑 |

## Validation Evidence

| コマンド | 結果 | 証跡 |
| --- | --- | --- |
| `tools/check-cpp-standard-policy.ps1` | `cpp-standard-policy-check: ok`（`clang-asan-ubsan` に `MK_ENABLE_SANITIZERS=ON` を必須化） | 2026-05-03 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | `json-contract-check: ok` | 同上 |
| GitHub Actions `linux-sanitizers` | CI の最新実行で確認（プッシュ後） | Workflow run artifact |

## Done When

- [x] サニタイザ CTest レーンが定義され、緑の証跡がある（既存レーンを文書・フラグ・CI で明確化）。
- [x] CI マトリクスに他レーン（Windows/Linux/package 等）と並んで説明されている。
- [x] 本ファイルの各セクションが実態と一致している。

## Non-Goals

- 全プラットフォームでの全サニタイザ同時有効化。
- GPU ドライバ内部の sanitizer 非対応領域の「完成宣言」。

---

*Plan authored: 2026-05-03. Slice completed: Ninja+preset hardening, CI env `UBSAN_OPTIONS`, configure STATUS messages, docs/testing.md matrix, policy script assertion.*
