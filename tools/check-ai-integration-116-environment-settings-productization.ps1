#requires -Version 7.0
#requires -PSEdition Core

# Chapter 116 for check-ai-integration.ps1 static contracts.

$environmentSettingsProductizationChecks = @(
    @{
        Path = "editor/core/include/mirakana/editor/environment_authoring.hpp"
        Needles = @(
            "EnvironmentSettingsWorkflowModel",
            "EnvironmentSettingsPreviewHandoffRow",
            "make_environment_settings_workflow_model"
        )
    },
    @{
        Path = "editor/core/src/environment_authoring.cpp"
        Needles = @(
            "environment_settings.preview",
            "environment.command.volume.edit",
            "environment.command.weather_keyframe.add",
            "environment.command.weather_keyframe.remove",
            "environment.command.weather_keyframe.reorder"
        )
    },
    @{
        Path = "editor/core/include/mirakana/editor/editor_panel.hpp"
        Needles = @(
            "environment_settings"
        )
    },
    @{
        Path = "editor/src/first_party_editor_document.cpp"
        Needles = @(
            "editor.panel.environment_settings",
            "editor.panel.environment_settings.workflow",
            "preview."
        )
    },
    @{
        Path = "games/sample_desktop_runtime_game/main.cpp"
        Needles = @(
            "--require-environment-settings-productized",
            "environment_settings_productized_status",
            "options.require_environment_settings_productized && environment_profile.ready",
            "environment_settings_broad_environment_ready_claimed=0"
        )
    },
    @{
        Path = "games/sample_desktop_runtime_game/game.agent.json"
        Needles = @(
            "environment-settings-productized",
            "desktop-runtime-sample-game-environment-settings-productized",
            "environment-settings-productized-evidence"
        )
    },
    @{
        Path = "tools/validate-installed-desktop-runtime.ps1"
        Needles = @(
            "--require-environment-settings-productized",
            "environment_settings_productized_status",
            "environment_settings_broad_environment_ready_claimed",
            "must not claim broad environment_ready from environment settings productization evidence"
        )
    },
    @{
        Path = "tools/validation-recipe-core.ps1"
        Needles = @(
            "Get-SampleDesktopRuntimeGameEnvironmentSettingsProductizedSmokeArgs",
            "--require-environment-settings-productized"
        )
    },
    @{
        Path = "tools/run-validation-recipe-plans.ps1"
        Needles = @(
            "desktop-runtime-sample-game-environment-settings-productized",
            "Get-SampleDesktopRuntimeGameEnvironmentSettingsProductizedSmokeArgs",
            "Environment settings productized validation is restricted"
        )
    },
    @{
        Path = "engine/agent/manifest.json"
        Needles = @(
            "Environment Settings Productization v1",
            "desktop-runtime-sample-game-environment-settings-productized",
            "environment_settings_broad_environment_ready_claimed=0"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Environment Settings Productization v1 coverage",
            "desktop-runtime-sample-game-environment-settings-productized",
            "environment_settings_broad_environment_ready_claimed=0"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Environment Settings Productization v1",
            "environment_settings_productized_status=ready",
            'broad `environment_ready`'
        )
    }
)

foreach ($check in $environmentSettingsProductizationChecks) {
    $fileText = Get-AgentSurfaceText $check.Path
    $missingNeedles = @()
    foreach ($needle in $check.Needles) {
        if (-not (Test-AgentSurfaceContainsText -Text $fileText -Needle $needle)) {
            $missingNeedles += $needle
        }
    }

    if ($missingNeedles.Count -gt 0) {
        $message = "ai-integration-check: {0} missing environment settings productization contract: {1}" -f $check.Path, ($missingNeedles -join ", ")
        Write-Error $message
    }
}
