#requires -Version 7.0
#requires -PSEdition Core

# Chapter 8.1 for check-ai-integration.ps1 scaffold tool validation contracts.

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
