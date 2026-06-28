#requires -Version 7.0
#requires -PSEdition Core
# Chapter 146 for check-ai-integration.ps1 static contracts.
# Windows MSVC Developer PowerShell helper policy needles only.

$msvcDevShellScriptPath = Resolve-RequiredAgentPath "tools/invoke-msvc-dev-shell.ps1"
$msvcDevShellScriptContent = Get-Content -LiteralPath $msvcDevShellScriptPath -Raw
$commonToolContent = Get-Content -LiteralPath (Resolve-RequiredAgentPath "tools/common.ps1") -Raw
$toolchainCheckContent = Get-Content -LiteralPath (Resolve-RequiredAgentPath "tools/check-toolchain.ps1") -Raw
$buildingContent = Get-AgentSurfaceText "docs/building.md"
$agentOperationalReferenceContent = Get-AgentSurfaceText "docs/agent-operational-reference.md"
$codexLocalEnvironmentContent = Get-AgentSurfaceText "docs/codex-local-environment.md"
$commandsManifestContent = Get-Content -LiteralPath (Resolve-RequiredAgentPath "engine/agent/manifest.fragments/002-commands.json") -Raw

foreach ($needle in @(
        "Get-VisualStudioDevShellScript",
        "cl",
        "nmake",
        "MSBuild",
        "cmake",
        "Invoke-CheckedCommand"
    )) {
    Assert-ContainsText $msvcDevShellScriptContent $needle "tools/invoke-msvc-dev-shell.ps1"
}

foreach ($needle in @(
        "Get-VisualStudioCppInstallationPath",
        "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
        "Get-VisualStudioDevShellScript",
        "Launch-VsDevShell.ps1",
        "Get-VisualStudioDevCmdScript",
        "VsDevCmd.bat"
    )) {
    Assert-ContainsText $commonToolContent $needle "tools/common.ps1"
}

foreach ($needle in @(
        "msvc-dev-shell",
        "direct-cl-status",
        "direct-nmake-status",
        "developer-shell-required",
        "tools/invoke-msvc-dev-shell.ps1"
    )) {
    Assert-ContainsText $toolchainCheckContent $needle "tools/check-toolchain.ps1"
}

foreach ($needle in @(
        "tools/invoke-msvc-dev-shell.ps1",
        "Launch-VsDevShell.ps1",
        "cl",
        "nmake",
        'GNU `make` is not a Windows-side prerequisite',
        "WSL",
        "zlib1g-dev"
    )) {
    Assert-ContainsText $buildingContent $needle "docs/building.md"
}

foreach ($needle in @(
        "tools/invoke-msvc-dev-shell.ps1",
        'raw Windows `cl` or `nmake`',
        "Launch-VsDevShell.ps1",
        'do not add GNU `make` as a Windows-side GameEngine prerequisite',
        "POSIX-only external C projects"
    )) {
    Assert-ContainsText $agentOperationalReferenceContent $needle "docs/agent-operational-reference.md"
}

foreach ($needle in @(
        "MSVC Dev Shell",
        "tools/invoke-msvc-dev-shell.ps1"
    )) {
    Assert-ContainsText $codexLocalEnvironmentContent $needle "docs/codex-local-environment.md"
}

foreach ($needle in @(
        "msvcDevShell",
        "tools/invoke-msvc-dev-shell.ps1"
    )) {
    Assert-ContainsText $commandsManifestContent $needle "engine/agent/manifest.fragments/002-commands.json"
}
