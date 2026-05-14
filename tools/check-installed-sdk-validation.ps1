#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$helper = Join-Path $PSScriptRoot "installed-sdk-validation.ps1"
if (-not (Test-Path -LiteralPath $helper -PathType Leaf)) {
    Write-Error "Missing installed SDK validation helper: $helper"
}
. $helper

function Write-TextFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Text
    )

    New-Item -ItemType Directory -Path (Split-Path -Parent $Path) -Force | Out-Null
    Set-Content -LiteralPath $Path -Value $Text -Encoding UTF8
}

function New-FakeInstalledSdk {
    param([Parameter(Mandatory = $true)][string]$InstallPrefix)

    Write-TextFile (Join-Path $InstallPrefix "lib/cmake/Mirakanai/MirakanaiConfig.cmake") "set(Mirakanai_FOUND TRUE)"
    Write-TextFile (Join-Path $InstallPrefix "lib/cmake/Mirakanai/MirakanaiConfigVersion.cmake") "set(PACKAGE_VERSION 0.1.0)"
    Write-TextFile (Join-Path $InstallPrefix "share/Mirakanai/manifest.json") '{"schemaVersion":1,"engine":{"name":"Mirakanai"}}'
    Write-TextFile (Join-Path $InstallPrefix "share/Mirakanai/schemas/engine-agent.schema.json") '{"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object"}'
    Write-TextFile (Join-Path $InstallPrefix "share/Mirakanai/schemas/engine-agent/ai-operable-production-loop.schema.json") '{"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object"}'
    Write-TextFile (Join-Path $InstallPrefix "share/Mirakanai/schemas/game-agent.schema.json") '{"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object"}'
    Write-TextFile (Join-Path $InstallPrefix "share/Mirakanai/tools/validate.ps1") 'Write-Host "validate"'
    Write-TextFile (Join-Path $InstallPrefix "share/Mirakanai/tools/agent-context.ps1") 'Write-Host "agent-context"'
    Write-TextFile (Join-Path $InstallPrefix "share/Mirakanai/examples/installed_consumer/CMakeLists.txt") "cmake_minimum_required(VERSION 3.30)"
    Write-TextFile (Join-Path $InstallPrefix "share/Mirakanai/examples/installed_consumer/main.cpp") "int main(){return 0;}"
    Write-TextFile (Join-Path $InstallPrefix "share/Mirakanai/samples/sample_headless/game.agent.json") '{"schemaVersion":1,"target":"sample_headless"}'
    Write-TextFile (Join-Path $InstallPrefix "share/doc/Mirakanai/README.md") "# Mirakanai"
    Write-TextFile (Join-Path $InstallPrefix "share/Mirakanai/THIRD_PARTY_NOTICES.md") "# Notices"
    Write-TextFile (Join-Path $InstallPrefix "share/Mirakanai/LICENSES/LicenseRef-Proprietary.txt") "Proprietary"
}

function Assert-Fails {
    param(
        [Parameter(Mandatory = $true)][scriptblock]$Script,
        [Parameter(Mandatory = $true)][string]$ExpectedMessage,
        [Parameter(Mandatory = $true)][string]$Label
    )

    try {
        & $Script
    } catch {
        $message = $_.Exception.Message
        if ($message -notlike "*$ExpectedMessage*") {
            Write-Error "$Label failed with unexpected message: $message"
        }
        return
    }

    Write-Error "$Label should have failed."
}

$testRoot = Join-Path (Get-RepoRoot) "out/tmp/installed-sdk-validation-check"
Reset-RepoTmpDirectory -Path $testRoot

$validInstall = Join-Path $testRoot "valid"
New-FakeInstalledSdk -InstallPrefix $validInstall
Assert-InstalledSdkMetadata -InstallPrefix $validInstall

$missingManifest = Join-Path $testRoot "missing-manifest"
New-FakeInstalledSdk -InstallPrefix $missingManifest
Remove-Item -LiteralPath (Join-Path $missingManifest "share/Mirakanai/manifest.json") -Force
Assert-Fails -Label "missing manifest" -ExpectedMessage "Installed Mirakanai AI manifest was not found" -Script {
    Assert-InstalledSdkMetadata -InstallPrefix $missingManifest
}

$malformedSchema = Join-Path $testRoot "malformed-schema"
New-FakeInstalledSdk -InstallPrefix $malformedSchema
Set-Content -LiteralPath (Join-Path $malformedSchema "share/Mirakanai/schemas/game-agent.schema.json") -Value "not-json" -Encoding UTF8
Assert-Fails -Label "malformed schema" -ExpectedMessage "Installed Mirakanai game agent schema is not valid JSON" -Script {
    Assert-InstalledSdkMetadata -InstallPrefix $malformedSchema
}

$emptyTool = Join-Path $testRoot "empty-tool"
New-FakeInstalledSdk -InstallPrefix $emptyTool
Set-Content -LiteralPath (Join-Path $emptyTool "share/Mirakanai/tools/validate.ps1") -Value "" -Encoding UTF8
Assert-Fails -Label "empty tool" -ExpectedMessage "Installed Mirakanai validation tool is empty" -Script {
    Assert-InstalledSdkMetadata -InstallPrefix $emptyTool
}

Write-Host "installed-sdk-validation-check: ok"
