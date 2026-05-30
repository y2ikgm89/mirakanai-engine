#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

Write-Error "The native MK_editor shell has only the launch-contract skeleton in this phase. The Win32/Dear ImGui/D3D12 host and GUI smoke lane are not wired yet; use MK_editor_native_shell_tests for the current skeleton evidence. The final shell must remain SDL3-free."
