#requires -Version 7.0
#requires -PSEdition Core

# Chapter 5.1 for check-ai-integration.ps1 Generated Game Studio v1 contracts.

$generatedGameStudioChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/generated_game_studio.hpp"
        Needles = @(
            "EditorAiGeneratedGameStudioV1Model",
            "EditorAiGeneratedGameStudioV1LoopRow",
            "EditorAiGeneratedGameStudioV1DesignSpecRow",
            "make_editor_ai_generated_game_studio_v1_model",
            "make_editor_ai_generated_game_studio_v1_ui_model"
        )
    },
    @{
        Path = "editor/core/src/generated_game_studio.cpp"
        Needles = @(
            "generated_game_studio",
            "manifest mutation",
            "validation execution",
            "engine internal mutation",
            "renderer/RHI handle exposure",
            "broad editor productization"
        )
    },
    @{
        Path = "editor/CMakeLists.txt"
        Needles = @(
            "core/src/generated_game_studio.cpp"
        )
    },
    @{
        Path = "editor/CMakeLists.txt"
        Needles = @(
            "Native MK_editor shell skeleton is SDL3-free",
            "MK_editor_shell_common",
            "add_executable(MK_editor"
        )
    },
    @{
        Path = "tests/unit/editor_core_tests.cpp"
        Needles = @(
            "editor ai generated game studio v1 model aggregates design workflow and command panel rows",
            "editor ai generated game studio v1 ui model exposes retained dashboard ids",
            "editor ai generated game studio v1 model rejects mutation execution and native claims",
            "generated_game_studio.loops.arena-2d.status"
        )
    },
    @{
        Path = "docs/superpowers/plans/2026-05-26-generated-game-studio-v1.md"
        Needles = @(
            "Generated Game Studio v1",
            "generated-game-studio-session-contract-v1",
            "generated-game-studio-dashboard-v1",
            "EditorAiGeneratedGameStudioV1Model"
        )
    },
    @{
        Path = "docs/superpowers/plans/README.md"
        Needles = @(
            "Generated Game Studio v1",
            "2026-05-26-generated-game-studio-v1.md",
            "General Purpose Game Production v1"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Generated Game Studio v1",
            "EditorAiGeneratedGameStudioV1Model",
            "generated_game_studio",
            "renderer/RHI residency"
        )
    },
    @{
        Path = "docs/ai-game-development.md"
        Needles = @(
            "Generated Game Studio v1",
            "EditorAiGeneratedGameStudioV1Model",
            "make_editor_ai_generated_game_studio_v1_ui_model",
            "generated_game_studio"
        )
    },
    @{
        Path = "docs/editor.md"
        Needles = @(
            "Generated Game Studio v1",
            "EditorAiGeneratedGameStudioV1Model",
            "make_editor_ai_generated_game_studio_v1_model",
            "generated_game_studio"
        )
    },
    @{
        Path = "docs/roadmap.md"
        Needles = @(
            "Generated Game Studio v1",
            "EditorAiGeneratedGameStudioV1Model",
            "generated_game_studio"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
        Needles = @(
            "generated-game-studio-v1",
            "completed-production-track",
            "2026-05-26-generated-game-studio-v1.md"
        )
    },
    @{
        Path = "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md"
        Needles = @(
            "Generated Game Studio Track",
            "generated-game-studio-v1",
            "generated_game_studio"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "generated-game-studio-v1",
            "Generated Game Studio v1",
            "EditorAiGeneratedGameStudioV1Model",
            "generated_game_studio",
            "broad editor productization"
        )
    },
    @{
        Path = ".agents/skills/editor-change/SKILL.md"
        Needles = @(
            "EditorAiGeneratedGameStudioV1Model",
            "generated_game_studio",
            "renderer/RHI residency"
        )
    },
    @{
        Path = ".claude/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "EditorAiGeneratedGameStudioV1Model",
            "generated_game_studio",
            "renderer/RHI residency"
        )
    },
    @{
        Path = ".cursor/skills/gameengine-editor/SKILL.md"
        Needles = @(
            "EditorAiGeneratedGameStudioV1Model",
            "generated_game_studio",
            "renderer/RHI residency"
        )
    },
    @{
        Path = ".agents/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "Generated Game Studio v1",
            "EditorAiGeneratedGameStudioV1Model",
            "engine-internal edits"
        )
    },
    @{
        Path = ".claude/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "Generated Game Studio v1",
            "EditorAiGeneratedGameStudioV1Model",
            "engine-internal edits"
        )
    },
    @{
        Path = ".cursor/skills/gameengine-game-development/SKILL.md"
        Needles = @(
            "Generated Game Studio v1",
            "EditorAiGeneratedGameStudioV1Model",
            "engine-internal edits"
        )
    }
)
foreach ($check in $generatedGameStudioChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not $fileText.Contains($needle)) {
            $missingNeedles += $needle
        }
    }
    if ($missingNeedles.Count -gt 0) {
        Write-Error "ai-integration-check: $($check.Path) missing generated game studio v1 contract: $($missingNeedles -join ', ')"
    }
}
