# Production Gap Selection v1 Implementation Plan (2026-05-03)

> **For agentic workers:** REQUIRED SUB-SKILL: `superpowers:writing-plans` でスコープ確定後、`superpowers:subagent-driven-development` または `superpowers:executing-plans` でランク済みスライスを順に実行する。ホストゲート作業には `cmake-build-system` と検証レシピを付ける。チェックボックス（`- [ ]`）で進捗を記録する。

**Goal:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) の Gap Ledger と `engine/agent/manifest.json` の `unsupportedProductionGaps` / `hostGates` を前提に、**次に切るべきホスト実装可能（Windows 既定 `dev` またはリポジトリ内 CMake/スクリプトのみで前進できる）**な実装スライスを **上位 3 件**に固定し、`recommendedNextPlan` が示す「広すぎる候補だけがある」状態を解消する。

**Architecture:** 本計画は **ドキュメントとマニフェストの真実同期** が主成果物である。新しいゲーム機能や ready 主張を広げない。ホストゲート（Vulkan strict / Metal Apple / Android GameActivity）に依存するスライスは本ランキングの **意図的除外** とし、別計画・別証跡で扱う。

**Tech Stack:** `docs/roadmap.md`、`docs/superpowers/plans/README.md`、`engine/agent/manifest.json`、`schemas/engine-agent.schema.json`、`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`、`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`、`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`、`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`。

---

## Goal

- マスター計画 Phase 0 の「次に着手してよい狭いスライス」を、**証跡の取りやすい順**で 3 つに並べる。
- `aiOperableProductionLoop.recommendedNextPlan` が参照する **根拠テキスト**を、本ファイルへのポインタで説明可能にする（`id` は既存の `next-production-gap-selection` のままでよい）。

## Context

- マスター計画 Phase 0 実装チェックリストの「`production-gap-selection-v1` 子計画」に対応する。
- `tidy-compile-database` ホストゲートは ready。ランク済み Phase 1 スライス（tidy / cmake 監査 / sanitizer / **coverage 閾値**）は **完了**。`currentActivePlan` は [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md) に戻る。
- Gap Ledger の「Full repository quality gate」は `partly ready` であり、manifest の `unsupportedProductionGaps` に **同義の行**を持つことで同期する（下記 Implementation Steps）。

## Constraints

- 広範な 3D プレイアブル、エディタ製品化、Metal / strict Vulkan 一般化、Android 実機行列を **本ランキングの「次の 3 つ」に入れない**。
- `currentActivePlan` は **実装中の狭いスライス**にのみ合わせる。文書のみの本計画を `currentActivePlan` にしない。
- 後方互換用 shim は追加しない（[AGENTS.md](../../../AGENTS.md)）。

## Ranked next three host-feasible slices

以下は **「デフォルト Windows + リポジトリ既定 preset で検証可能な作業が中心」**であることを優先して並べる。順序はコストと依存関係に基づく。


| 順位  | 子計画 id                                          | 計画パス                                                                                                                       | 選定理由                                                                                                                                                      |
| --- | ----------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1   | `full-clang-tidy-warning-cleanup-v1`            | [2026-05-03-full-clang-tidy-warning-cleanup-v1.md](2026-05-03-full-clang-tidy-warning-cleanup-v1.md)                       | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict` が合成 `compile_commands.json` 上で実行可能。警告除去は後続フェーズの回帰検知基盤であり、Apple/Android を不要とする。**本スライスは完了**（厳格 tidy 非エラー終了・検証証跡は当該計画ファイル）。 |
| 2   | `cmake-install-export-and-cxx-modules-audit-v1` | [2026-05-03-cmake-install-export-and-cxx-modules-audit-v1.md](2026-05-03-cmake-install-export-and-cxx-modules-audit-v1.md) | ターゲットスコープの CMake `install()` / export / C++ モジュール方針の監査。**本スライスは完了**（`docs/building.md`、ルート CMake STATUS 方針、計画 Evidence 更新）。                               |
| 3   | `sanitizer-and-ci-matrix-hardening-v1`          | [2026-05-03-sanitizer-and-ci-matrix-hardening-v1.md](2026-05-03-sanitizer-and-ci-matrix-hardening-v1.md)                   | 既存 CTest/CI ラインの強化。**本スライスは完了**（`clang-asan-ubsan` / `linux-sanitizers` / `docs/testing.md` マトリクス・計画 Evidence）。                                           |


**意図的に本ランキングから外す（別ゲート必須）:** `3d-playable-vertical-slice`（スコープ過大）、`editor-productization`、`production-ui-importer-platform-adapters`、Vulkan strict / Metal Apple / Android GameActivity の **一般 ready 化**。（参照: チャーター済みの [coverage-threshold-policy-v1](2026-05-03-coverage-threshold-policy-v1.md) は **完了**し、`tools/coverage-thresholds.json` で Linux ライン閾値を強制。）

## Implementation Steps

- Gap Ledger の「Full repository quality gate」行と同等の `unsupportedProductionGaps` エントリを `engine/agent/manifest.json` に追加する。
- `aiOperableProductionLoop.currentActivePlan` を実態に合わせる（Phase 1 子スライス完了後は **coverage-threshold-policy-v1** を経て [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md) に戻す）。
- `recommendedNextPlan.reason` を本計画とランキング表を参照する文言に更新する（`status: blocked` は維持してよい）。
- `docs/superpowers/plans/README.md` に本計画を登録する。
- `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md` の Phase 0 チェックリストを実態に合わせて更新する。
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` をローカルで完走し、`validate: ok` を本ファイルの Validation Evidence に追記する（エージェント環境ではタイムアウトし得る）。

## Tests

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`

## Validation Evidence


| コマンド                    | 結果                                | 証跡                  |
| ----------------------- | --------------------------------- | ------------------- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` | PASS（終了コード 0）                     | 2026-05-03 エージェント実行 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`  | `json-contract-check: ok`         | 同上                  |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`   | `ai-integration-check: ok`        | 同上                  |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`      | `validate: ok`（終了コード 0、CTest 全パス） | 2026-05-03 ローカル完走   |


## Done When

- 本ファイルが Goal / Context / Constraints / Ranked slices / Steps を満たす。
- マスター計画 Phase 0 の対応チェックボックスが実態と一致する。
- `unsupportedProductionGaps` に full-repository-quality-gate 相当が含まれる。
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` の証跡が上表に付く、またはブロッカーを具体記録する。

## Non-Goals

- ランキングに載せなかったギャップの実装開始。
- `recommendedNextPlan.id` のリネーム（スキーマ・静的検査・ドキュメント参照の波及を避ける）。

---

*Plan authored: 2026-05-03. Scope: Phase 0 production gap selection and manifest/roadmap truth sync; no new engine capabilities.*
