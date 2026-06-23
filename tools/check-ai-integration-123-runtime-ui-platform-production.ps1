#requires -Version 7.0
#requires -PSEdition Core

# Chapter 12.3 for check-ai-integration.ps1 First-Party Runtime UI And Editor Platform Production v1 contracts.

$runtimeUiPlatformProductionPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-24-first-party-runtime-ui-and-editor-platform-production-v1.md"
$runtimeUiCleanRoomLedgerText = Get-AgentSurfaceText "docs/specs/2026-06-24-first-party-ui-clean-room-source-ledger-v1.md"
$runtimeUiCleanRoomCheckText = Get-AgentSurfaceText "tools/check-first-party-ui-clean-room.ps1"
$dependencyDocsText = Get-AgentSurfaceText "docs/dependencies.md"
$legalDocsText = Get-AgentSurfaceText "docs/legal-and-licensing.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"

foreach ($needle in @(
        "First-Party Runtime UI And Editor Platform Production v1",
        "Clean-Room Source Ledger And Static Guard",
        "tools/check-first-party-ui-clean-room.ps1",
        "docs/specs/2026-06-24-first-party-ui-clean-room-source-ledger-v1.md"
    )) {
    Assert-ContainsText $runtimeUiPlatformProductionPlanText $needle "runtime UI platform production plan"
}

foreach ($section in @(
        "## Allowed Sources",
        "## Forbidden Inputs",
        "## Forbidden Public Names",
        "## Dependency Entry Gate",
        "## Trademark And Marketing Non-Claims",
        "## Review Evidence"
    )) {
    Assert-ContainsText $runtimeUiCleanRoomLedgerText $section "first-party UI clean-room source ledger"
}

foreach ($needle in @(
        "official Unity/Unreal/Godot category docs only",
        "Microsoft SDK docs",
        "Apple SDK docs",
        "HarfBuzz docs",
        "FreeType docs",
        "AT-SPI2 docs",
        "Vulkan docs",
        "W3C accessibility practices",
        "repository-owned code"
    )) {
    Assert-ContainsText $runtimeUiPlatformProductionPlanText $needle "runtime UI platform production plan Task 1"
}

foreach ($needle in @(
        "Unity official category documentation",
        "Unreal Engine official category documentation",
        "Godot official category documentation",
        "Microsoft SDK documentation",
        "Apple SDK documentation",
        "HarfBuzz documentation",
        "FreeType documentation",
        "AT-SPI2 documentation",
        "Vulkan documentation",
        "W3C accessibility practices",
        "Repository-owned code"
    )) {
    Assert-ContainsText $runtimeUiCleanRoomLedgerText $needle "first-party UI clean-room source ledger"
}

foreach ($token in @(
        "UXML",
        "USS",
        "UMG",
        "Slate",
        "WidgetBlueprint",
        "UnityEngine",
        "UnityEditor",
        "Godot",
        "CanvasLayer",
        "ControlNode",
        "UnrealEd",
        "BlueprintGraph",
        "DearImGui",
        "ImGui",
        "RmlUi",
        "Noesis",
        "Slint",
        "Qt"
    )) {
    Assert-ContainsText $runtimeUiCleanRoomLedgerText $token "first-party UI clean-room source ledger forbidden public names"
    Assert-ContainsText $runtimeUiCleanRoomCheckText "`"$token`"" "tools/check-first-party-ui-clean-room.ps1 forbidden token list"
}

foreach ($needle in @(
        "Default themes, fonts, icons, screenshots",
        "Unity UXML/USS imports",
        "Unreal UMG/Widget Blueprint/Slate imports",
        "Godot scene/control tree imports",
        "Visual parity, API parity, workflow parity"
    )) {
    Assert-ContainsText $runtimeUiCleanRoomLedgerText $needle "first-party UI clean-room source ledger forbidden inputs"
}

Assert-ContainsText $dependencyDocsText "First-party runtime UI clean-room source gate" "docs/dependencies.md runtime UI clean-room gate"
Assert-ContainsText $legalDocsText "First-Party Runtime UI Clean-Room Source Gate" "docs/legal-and-licensing.md runtime UI clean-room gate"
foreach ($needle in @(
        "docs/specs/2026-06-24-first-party-ui-clean-room-source-ledger-v1.md",
        "tools/check-first-party-ui-clean-room.ps1"
    )) {
    Assert-ContainsText $dependencyDocsText $needle "docs/dependencies.md runtime UI clean-room gate"
    Assert-ContainsText $legalDocsText $needle "docs/legal-and-licensing.md runtime UI clean-room gate"
}

Assert-ContainsText $currentCapabilitiesText "First-Party UI Clean-Room Source Ledger v1" "docs/current-capabilities.md"
Assert-ContainsText $currentCapabilitiesText "first-party-ui-clean-room: ok" "docs/current-capabilities.md"
Assert-ContainsText $planRegistryText "First-Party Runtime UI And Editor Platform Production v1" "docs/superpowers/plans/README.md"

$cleanRoomCheckScriptPath = Resolve-RequiredAgentPath "tools/check-first-party-ui-clean-room.ps1"
& $cleanRoomCheckScriptPath
