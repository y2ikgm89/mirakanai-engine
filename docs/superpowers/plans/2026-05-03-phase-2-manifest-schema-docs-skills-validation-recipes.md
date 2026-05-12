# Phase 2 manifest, schema, docs, skills, and validation recipe evidence (2026-05-03)

> **For agentic workers:** `gameengine-agent-integration` を参照。本スライスは **Phase 2 マスター計画の残チェック行を証跡付きで閉じる**ことに限定し、新しいランタイム機能やレシピ本体の追加は行わない。

**Goal:** `ai-cook-package-command-surface-v1` 完了後に残っていた「マニフェスト／スキーマ／ドキュメント／Cursor スキル／`run-validation-recipe` 証跡」の一括整合を記録し、マスター Phase 2 の該当チェックボックスを緑の根拠で更新する。

**Architecture:** 能力の真実は引き続き `docs/current-capabilities.md` と `docs/roadmap.md`。アーキテクチャ横断のマニフェスト契約は `docs/architecture.md` の `engine/assets` 節に一文追加し、Cursor 作業者向けは `.cursor/skills/gameengine-cursor-baseline/SKILL.md` で `registeredSourceAssetCookTargets` と prefab 行の整合を明示する。検証は `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` を正とし、加えて `tools/run-validation-recipe.ps1` の **DryRun** と **Execute** を別レシピで記録する。

**Tech Stack:** 既存の `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`、`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1`、リポジトリ検証スクリプト。

---

## Goal

- Phase 2 マスター計画上の「manifest/schema/docs/skills/recipes/tests/validate/recipe runner」行に対応する **完了証跡**を残す。
- 次工程（Phase 3）へ `engine/agent/manifest.json` の `recommendedNextPlan` を進める。

## Context

- 親: [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 2 チェックリスト。
- 先行: [2026-05-03-ai-cook-package-command-surface-v1.md](2026-05-03-ai-cook-package-command-surface-v1.md)（`registeredSourceAssetCookTargets` 本実装）。

## Constraints

- 新規のホスト依存レシピ Execute（長時間 GPU スモーク等）は本スライスの必須証跡にしない。DryRun はコマンド計画の提示のみで完結する。
- マニフェストの `validationRecipes` 配列の意味を変えない（ゲームごとの宣言は先行スライス済み）。

## Implementation Steps

- [x] `docs/architecture.md` に `registeredSourceAssetCookTargets` と prefab 行の関係を追記する。
- [x] `.cursor/skills/gameengine-cursor-baseline/SKILL.md` にマニフェスト整合の注意を追記する。
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` を再実行する。
- [x] `run-validation-recipe` の DryRun（`desktop-game-runtime`）と Execute（`public-api-boundary`）で証跡を残す。
- [x] マスター計画 Phase 2 の該当チェックを `[x]` にし、`engine/agent/manifest.json` の `recommendedNextPlan` を Phase 3 入口に更新する。
- [x] `docs/superpowers/plans/README.md` の Active / Recent Completed を同期する。

## Validation Evidence

| コマンド | 結果 | 日付 |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | `validate: ok`、CTest 28/28 Passed | 2026-05-03 18:26 +09:00 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe desktop-game-runtime` | JSON `status: dry-run`、`commandPlan` に `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` | 2026-05-03 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe public-api-boundary` | JSON `status: passed`、`exitCode: 0`、`stdoutSummary` に `public-api-boundary-check: ok` | 2026-05-03 |

## Done When

- [x] 上記 Evidence 表が埋まり、マスター Phase 2 の該当行がすべて `[x]`。
- [x] `recommendedNextPlan` が Phase 3 の次作業（ネイティブ RHI リソース寿命）を指す。

## Non-Goals

- `native-rhi-resource-lifetime-migration-v1` のコード実装（別子計画）。
- `desktop-game-runtime` の Execute（ウィンドウ／GPU 依存のためホスト別途）。

---

*Plan completed: 2026-05-03.*
