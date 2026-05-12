# Coverage Threshold Policy v1 Implementation Plan (2026-05-03)

> **For agentic workers:** `superpowers:subagent-driven-development` または `superpowers:executing-plans` でタスク単位に実行する。CMake / CI / `tools/check-coverage.ps1` の変更は `cmake-build-system` スキルに従う。

**Goal:** Linux GCC/Clang CI でのカバレッジ収集（`tools/check-coverage.ps1`、CTest `-T Coverage`）に対し、**閾値・レポート形式・失敗条件**をリポジトリ方針として固定し、Phase 1 の「Linux coverage lane reports stable threshold evidence」を満たす。

**Architecture:** **`tools/coverage-thresholds.json`** が単一の **`minLineCoveragePercent`**（および **`lcovRemovePatterns`**）を保持する。`check-coverage.ps1` は別ツリー **`out/build/coverage`** で `gcc --coverage` を掛け、CTest と **`lcov --capture` / `--remove` / `--summary`** により全体ライン率を得て、`‑Strict` 時は閾値未満で **非ゼロ終了**する。ブランチ別モジュール閾値は Non-Goals とし、将来スライスで拡張する。

**Tech Stack:** CMake、CTest、GCC `--coverage`、gcov、**lcov**（ディストロパッケージ）、既存 `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-coverage.ps1`。

---

## Goal

- Linux CI（またはローカル Linux）で `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-coverage.ps1`（および **`‑Strict`**）が **再現可能な閾値**で失敗・成功を判定できる。
- 閾値・除外パターンが **計画書、`docs/testing.md`、`AGENTS.md`** と一致している。

## Context

- [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 1 の子計画。
- [2026-05-03-full-clang-tidy-warning-cleanup-v1.md](2026-05-03-full-clang-tidy-warning-cleanup-v1.md) と独立して進められるが、Phase 1 の「品質ゲート」完了の証跡として並列に扱う。
- 実施完了後、`engine/agent/manifest.json` の `currentActivePlan` は **マスター計画**へ戻す（下記 Evidence）。

## Constraints

- 後方互換用の「常に成功」ラッパーや無意味に閾値を下げる変更は禁止。
- サードパーティ依存を増やす場合は [docs/legal-and-licensing.md](../../legal-and-licensing.md)、[docs/dependencies.md](../../dependencies.md)、`vcpkg.json`、`THIRD_PARTY_NOTICES.md` を更新する（本スライスでは **lcov は OS パッケージのみ**）。
- Windows ホストでは `coverage-check` がブロッカーになることが既知である点は維持し、**Linux CI のログ・アーティファクト**を一次証跡とする。

## Implementation Steps

- [x] **Step 1:** 現行 Linux ビルドでカバレッジレポートを生成し、**ベースライン数値**を Evidence に記録する。（ローカル WSL は CMake バージョンにより不可な場合あり。**GitHub Actions `linux` ジョブの `lcov --summary` と artifact `linux-coverage` を正**。）
- [x] **Step 2:** **全体ライン最低率**（`minLineCoveragePercent`）+ **`lcovRemovePatterns`**（vcpkg ツリー・ABI プローブ等の除外）を採用。
- [x] **Step 3:** `tools/check-coverage.ps1` に **`lcov`** 集計と閾値比較を組み込み、`‑Strict` で未達時に終了。`tools/check-coverage-thresholds.ps1` で JSON を検証し、`tools/validate.ps1` から実行。
- [x] **Step 4:** `docs/testing.md` に Linux coverage 節を追加。`AGENTS.md` にポインタ追加。
- [x] **Step 5:** マスター計画の Coverage 実行チェックと manifest の full-repository gap メモを同期。

## Threshold policy (locked)

| 項目 | 実装 |
| --- | --- |
| 閾値ファイル | [tools/coverage-thresholds.json](../../../tools/coverage-thresholds.json) |
| 初期ライン下限 | **10.0%**（持続的な CI 証跡で段階的に **引き上げ**る；無意味な引き下げは禁止） |
| lcov 除外例 | `*/vcpkg_installed/*`, `*CMakeCXXCompilerABI.cpp` |
| CI | [.github/workflows/validate.yml](../../../.github/workflows/validate.yml) `linux` が **`apt install lcov`** のうえ `./tools/check-coverage.ps1 ‑Strict` |

## Tests

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-coverage.ps1`（Linux）
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-coverage.ps1 -Strict`（Linux）

## Validation

| コマンド | 期待 |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-coverage.ps1` | Linux で収集・`lcov` サマリ・閾値メッセージまで |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | manifest / 静的チェック整合 |

## Validation Evidence

| コマンド | 結果 | 証跡 |
| --- | --- | --- |
| `tools/check-coverage-thresholds.ps1` | `check-coverage-thresholds: ok` | `tools/validate.ps1` 経由 |
| `tools/check-cpp-standard-policy.ps1` | `coverage-thresholds.json` を参照 | `Assert-TextContains tools/coverage-thresholds.json` |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | `json-contract-check: ok` | エージェント実行 |
| GitHub Actions `linux` job | `check-coverage.ps1 -Strict` + artifact `linux-coverage`（`coverage*.info` 含む） | プッシュ後の workflow run |

## Done When

- [x] 閾値ポリシーがコードおよび CI で強制され、文書と一致している。
- [x] Linux での証跡パス（CI artifact・ログ）が Evidence に記載されている。
- [x] 本ファイルの Goal / Context / Constraints / Steps / Tests / Validation / Done When が実態と一致している。

## Non-Goals

- macOS / Windows での gcov 互換収集の全面サポート。
- レンダラ・RHI の品質目標そのものの定義（別 Phase）。
- モジュール単位の複数閾値行列（別スライス）。

---

*Plan authored: 2026-05-03. Slice completed: coverage-thresholds.json, check-coverage.ps1 lcov gate, validate.yml lcov install + artifact paths.*
