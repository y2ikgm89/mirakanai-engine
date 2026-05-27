#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

Write-Error "The visible editor shell is deferred during SDL3 removal. MK_editor_core remains available through the default validation lane; a future MK_editor shell must use first-party Win32/D3D12 adapters and must not depend on SDL3."
