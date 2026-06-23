#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function Assert-ContainsText {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Needle,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Text.Contains($Needle)) {
        Write-Error "$Context missing: $Needle"
    }
}

$manifest = Get-Content -LiteralPath (Join-Path $root "engine/agent/manifest.json") -Raw | ConvertFrom-Json
$linuxHostGate = @($manifest.aiOperableProductionLoop.hostGates | Where-Object { $_.id -eq "vulkan-strict-linux" })
if ($linuxHostGate.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must define exactly one vulkan-strict-linux host gate."
}
if ($linuxHostGate[0].status -ne "ready") {
    Write-Error "vulkan-strict-linux host gate must be ready after hosted Linux Vulkan Host Evidence passed."
}
if ($linuxHostGate[0].residualClass -ne "ready") {
    Write-Error "vulkan-strict-linux residualClass must be ready after hosted Linux Vulkan Host Evidence passed."
}
if (@($linuxHostGate[0].validationRecipes) -notcontains "environment-platform-linux-vulkan-package") {
    Write-Error "vulkan-strict-linux must reference environment-platform-linux-vulkan-package."
}

$linuxNotes = [string]$linuxHostGate[0].notes
foreach ($needle in @(
        "Linux Vulkan Host Evidence",
        "vulkaninfo_ready=1",
        "VK_LAYER_KHRONOS_validation_ready=1",
        "dxc_spirv_codegen_ready=1",
        "spirv_val_ready=1",
        "linux_icd_runtime_ready=1",
        "first_party_linux_runtime_host_ready=1",
        "linux_package_script_ready=1",
        "linux_installed_validator_ready=1",
        "linux_package_smoke_ready=1",
        "linux_vulkan_readback_ready=1",
        "linux_vulkan_validation_log_clean=1",
        "environment_platform_linux_vulkan_ready=1",
        "environment_platform_requires_linux_vulkan_host_evidence=0",
        "environment_platform_windows_vulkan_inferred=0",
        "native_handle_access=0"
    )) {
    Assert-ContainsText $linuxNotes $needle "vulkan-strict-linux host gate notes"
}

foreach ($surface in @(
        @{
            Path = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
            Needles = @('"id": "vulkan-strict-linux"', '"status": "ready"', '"residualClass": "ready"', "Linux Vulkan Host Evidence")
        },
        @{
            Path = "docs/current-capabilities.md"
            Needles = @("Linux Vulkan Host Evidence", "environment_platform_linux_vulkan_ready=1", "environment_platform_requires_linux_vulkan_host_evidence=0")
        },
        @{
            Path = "docs/testing.md"
            Needles = @("Linux Vulkan Host Evidence", "linux_package_smoke_ready=1", "linux_vulkan_readback_ready=1", "linux_vulkan_validation_log_clean=1")
        },
        @{
            Path = "docs/superpowers/plans/README.md"
            Needles = @("Linux Vulkan host gate closeout", "vulkan-strict-linux", "environment_platform_linux_vulkan_ready=1")
        },
        @{
            Path = "tools/validate.ps1"
            Needles = @("check-linux-vulkan-host-gate-closeout.ps1")
        }
    )) {
    $surfaceText = Get-Content -LiteralPath (Join-Path $root $surface.Path) -Raw
    foreach ($needle in @($surface.Needles)) {
        Assert-ContainsText $surfaceText $needle $surface.Path
    }
}

Write-Information "linux-vulkan-host-gate-closeout-check: ok" -InformationAction Continue
