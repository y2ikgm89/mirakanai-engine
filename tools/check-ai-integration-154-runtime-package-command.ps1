#requires -Version 7.0
#requires -PSEdition Core

# Chapter 154 for check-ai-integration.ps1 static contracts.
# Runtime Package Command Plan v1.

$runtimePackageCommandHeaderText = Get-AgentSurfaceText "engine/tools/include/mirakana/tools/runtime_package_command_tool.hpp"
$runtimePackageCommandSourceText = Get-AgentSurfaceText "engine/tools/asset/runtime_package_command_tool.cpp"
$runtimePackageCommandCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$runtimePackageCommandGameDevText = Get-AgentSurfaceText "docs/ai-game-development.md"
$runtimePackageCommandManifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$runtimePackageCommandLoop = ($runtimePackageCommandManifestText | ConvertFrom-Json -Depth 100).aiOperableProductionLoop
$runtimePackageCommandSurface = @($runtimePackageCommandLoop.commandSurfaces | Where-Object { $_.id -eq "cook-runtime-package" })

if ($runtimePackageCommandSurface.Count -ne 1 -or $runtimePackageCommandSurface[0].status -ne "ready") {
    Write-Error "engine/agent/manifest.json must expose one ready cook-runtime-package command surface"
} else {
    $runtimePackageModes = @{}
    foreach ($mode in @($runtimePackageCommandSurface[0].requestModes)) { $runtimePackageModes[$mode.id] = $mode }
    if (-not $runtimePackageModes.ContainsKey("dry-run") -or $runtimePackageModes["dry-run"].status -ne "ready") { Write-Error "cook-runtime-package dry-run mode must remain ready" }
    if (-not $runtimePackageModes.ContainsKey("apply") -or $runtimePackageModes["apply"].status -ne "blocked") { Write-Error "cook-runtime-package apply mode must remain blocked" }
    foreach ($field in @("validationRecipes", "packageBuildExecution", "packageScripts", "shaderCompilerExecution", "runtimePackageLoadExecution", "rendererRhiResidency", "packageStreaming", "publicNativeRhiHandles", "legalApproval", "externalEngineCompatibility", "externalEngineProjectImport", "externalEngineAssets", "arbitraryShell", "freeFormEdit")) { if (@($runtimePackageCommandSurface[0].requestShape.optionalFields) -notcontains $field) { Write-Error "cook-runtime-package requestShape missing optional field: $field" } }
    foreach ($needle in @("games/<game_name>/game.agent.json", "^[A-Za-z_][A-Za-z0-9_]*$", "runtime/", "no aliases or duplicates")) { if (-not $runtimePackageCommandSurface[0].requestShape.pathPolicy.Contains($needle)) { Write-Error "cook-runtime-package pathPolicy missing: $needle" } }
    foreach ($recipe in @("agent-contract", "public-api-boundary", "desktop-game-runtime", "default")) { if (@($runtimePackageCommandSurface[0].validationRecipes) -notcontains $recipe) { Write-Error "cook-runtime-package validationRecipes missing: $recipe" } }
    foreach ($needle in @("mirakana::plan_runtime_package_command", "runtime_package_command_tool.hpp/cpp", "does not execute package builds", "does not provide legal advice or legal approval", "Unity, Unreal Engine, Godot")) { if (-not $runtimePackageCommandSurface[0].notes.Contains($needle)) { Write-Error "cook-runtime-package notes missing: $needle" } }
}

foreach ($needle in @("RuntimePackageCommandRequest", "RuntimePackageCommandResult", "RuntimePackageCommandMode", "RuntimePackageCommandBackend", "plan_runtime_package_command", "package_build_execution", "external_engine_compatibility", "legal_approval")) { Assert-ContainsText $runtimePackageCommandHeaderText $needle "runtime package command public header" }
foreach ($needle in @("package_build_execution_not_command_owned", "package_apply_not_ready", "external engine compatibility is not supported", "legal approval is not provided", "arbitrary shell execution is not supported", "runtime_package_files", "tools/package-desktop-runtime.ps1")) { Assert-ContainsText $runtimePackageCommandSourceText $needle "runtime package command implementation" }
foreach ($doc in @(@{ Text = $runtimePackageCommandCapabilitiesText; Label = "docs/current-capabilities.md" }, @{ Text = $runtimePackageCommandGameDevText; Label = "docs/ai-game-development.md" })) { Assert-ContainsText $doc.Text "mirakana::plan_runtime_package_command" $doc.Label; Assert-ContainsText $doc.Text "cook-runtime-package" $doc.Label; Assert-ContainsText $doc.Text "apply remains blocked" $doc.Label; Assert-ContainsText $doc.Text "Unity/Unreal" $doc.Label; Assert-ContainsText $doc.Text "Godot" $doc.Label }
