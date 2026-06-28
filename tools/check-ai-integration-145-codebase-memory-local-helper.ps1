#requires -Version 7.0
#requires -PSEdition Core
# Chapter 145 for check-ai-integration.ps1 static contracts.
# Developer-local codebase-memory-mcp policy needles only.

$codebaseMemorySpecRelativePath = "docs/specs/2026-06-28-codebase-memory-local-ai-helper-v1.md"
$codebaseMemoryWslDepsRelativePath = "tools/install-codebase-memory-wsl-deps.ps1"
$codebaseMemorySpecPath = Resolve-RequiredAgentPath $codebaseMemorySpecRelativePath
$codebaseMemoryWslDepsPath = Resolve-RequiredAgentPath $codebaseMemoryWslDepsRelativePath
$codebaseMemorySpecContent = Get-Content -LiteralPath $codebaseMemorySpecPath -Raw
$codebaseMemoryWslDepsContent = Get-Content -LiteralPath $codebaseMemoryWslDepsPath -Raw
$aiIntegrationContent = Get-AgentSurfaceText "docs/ai-integration.md"
$gitIgnoreContent = Get-Content -LiteralPath (Resolve-RequiredAgentPath ".gitignore") -Raw
$commandsManifestContent = Get-Content -LiteralPath (Resolve-RequiredAgentPath "engine/agent/manifest.fragments/002-commands.json") -Raw
$documentationPolicyManifestContent = Get-Content -LiteralPath (Resolve-RequiredAgentPath "engine/agent/manifest.fragments/012-documentationPolicy.json") -Raw

Assert-ContainsText $gitIgnoreContent ".codebase-memory/" ".gitignore"

foreach ($needle in @(
        "Developer-Local Code Intelligence MCP",
        "codebase-memory-mcp",
        "mode=full",
        "persistence=false",
        ".codebase-memory/graph.db.zst",
        "manage_adr",
        "ingest_traces",
        "RuntimeTrace",
        "OBSERVED_EXECUTION",
        "code.function.name",
        "validation.recipe.id",
        "tools/install-codebase-memory-wsl-deps.ps1",
        "zlib1g-dev",
        "clang-format",
        "pkg-config"
    )) {
    Assert-ContainsText $aiIntegrationContent $needle "docs/ai-integration.md codebase-memory-mcp policy"
}

foreach ($needle in @(
        "## Status",
        "Retained policy design record.",
        "developer-local AI assistance tool only",
        "not an engine, editor, runtime, package, CMake, vcpkg, or public API dependency",
        '`mode=fast` is insufficient',
        "persistence=false",
        "persistence=true",
        ".codebase-memory/graph.db.zst",
        "manage_adr",
        "ingest_traces",
        "runtime trace smoke verification",
        "OpenTelemetry-compatible OTLP trace JSON",
        "RuntimeTrace",
        "RuntimeSpan",
        "OBSERVED_EXECUTION",
        "code.function.name",
        'repository-relative `code.file.path`',
        "validation.recipe.id",
        "ge.diagnostic_code",
        "tools/install-codebase-memory-wsl-deps.ps1",
        "zlib1g-dev",
        "clang-format",
        "pkg-config",
        "Full C++ validation is not required"
    )) {
    Assert-ContainsText $codebaseMemorySpecContent $needle $codebaseMemorySpecRelativePath
}

foreach ($needle in @(
        "installCodebaseMemoryWslDeps",
        "tools/install-codebase-memory-wsl-deps.ps1"
    )) {
    Assert-ContainsText $commandsManifestContent $needle "engine/agent/manifest.fragments/002-commands.json"
}

foreach ($needle in @(
        "codebaseMemoryMcp",
        "local-helper-policy-accepted",
        $codebaseMemorySpecRelativePath,
        ".codebase-memory/ is ignored",
        "wslHostSetupCommand",
        "wslHostPackages",
        "zlib1g-dev",
        "clang-format",
        "pkg-config",
        "index_repository mode=full with persistence=false by default",
        "ingest_traces with sanitized OTLP-shaped spans",
        "RuntimeTrace, RuntimeSpan, CONTAINS_SPAN, and OBSERVED_EXECUTION",
        "manage_adr for project decisions",
        "Confirm exact claims with rg, git status, manifest data, tests, and repository validation."
    )) {
    Assert-ContainsText $documentationPolicyManifestContent $needle "engine/agent/manifest.fragments/012-documentationPolicy.json"
}

foreach ($needle in @(
        "#requires -Version 7.0",
        "#requires -PSEdition Core",
        "[CmdletBinding(SupportsShouldProcess = `$true)]",
        "zlib1g-dev",
        "clang-format",
        "pkg-config",
        "sudo apt-get update",
        "apt-get install -y",
        "CheckOnly",
        "NonInteractiveSudo"
    )) {
    Assert-ContainsText $codebaseMemoryWslDepsContent $needle $codebaseMemoryWslDepsRelativePath
}
