#requires -Version 7.0
#requires -PSEdition Core

# Chapter 8.1 for check-ai-integration.ps1 scaffold tool validation contracts.

$newGameTemplatesText = Get-AgentSurfaceText "tools/new-game-templates.ps1"
Assert-ContainsText $newGameTemplatesText "function Get-DesktopRuntimeHostCommonCpp" "tools/new-game-templates.ps1"
Assert-ContainsText $newGameTemplatesText "function Get-DesktopRuntimeHostArgsCpp" "tools/new-game-templates.ps1"
foreach ($sharedCppSignature in @(
        "void print_usage()",
        "[[nodiscard]] bool parse_args(int argc, char** argv, DesktopRuntimeOptions& options)",
        "[[nodiscard]] bool parse_positive_uint32(std::string_view text, std::uint32_t& value) noexcept",
        "[[nodiscard]] std::filesystem::path executable_directory(const char* executable_path)",
        "[[nodiscard]] bool verify_required_config(const char* executable_path, std::string_view config_path)",
        "[[nodiscard]] std::string_view status_name(mirakana::DesktopRunStatus status) noexcept"
    )) {
    $sharedCppSignatureMatches =
        [System.Text.RegularExpressions.Regex]::Matches($newGameTemplatesText, [System.Text.RegularExpressions.Regex]::Escape($sharedCppSignature))
    if ($sharedCppSignatureMatches.Count -ne 1) {
        Write-Error "tools/new-game-templates.ps1 must define shared desktop runtime C++ helper '$sharedCppSignature' exactly once; found $($sharedCppSignatureMatches.Count)"
    }
}

$invalidDisplayNameRoot = New-ScaffoldCheckRoot
try {
    Assert-NewGameFailure @(
        "-Name",
        "bad_display_game",
        "-DisplayName",
        "Bad`nDisplay",
        "-RepositoryRoot",
        $invalidDisplayNameRoot
    ) "new-game control-character DisplayName validation"
} finally {
    Remove-ScaffoldCheckRoot $invalidDisplayNameRoot
}

$invalidGameNameRoot = New-ScaffoldCheckRoot
try {
    Assert-NewGameFailure @(
        "-Name",
        "bad-hyphen-game",
        "-RepositoryRoot",
        $invalidGameNameRoot
    ) "new-game lowercase snake_case Name validation"
} finally {
    Remove-ScaffoldCheckRoot $invalidGameNameRoot
}

$targetCollisionRoot = New-ScaffoldCheckRoot
try {
    Set-Content -LiteralPath (Join-Path $targetCollisionRoot "games/CMakeLists.txt") -Value @"
MK_add_game(collision_game
    SOURCES
        collision_game/main.cpp
)
"@ -NoNewline
    Assert-NewGameFailure @(
        "-Name",
        "collision_game",
        "-RepositoryRoot",
        $targetCollisionRoot
    ) "new-game target collision validation"
} finally {
    Remove-ScaffoldCheckRoot $targetCollisionRoot
}
