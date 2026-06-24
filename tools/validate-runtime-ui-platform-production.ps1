#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [switch]$RequireReady,
    [switch]$SkipEditor,
    [switch]$SkipPackage,
    [switch]$StaticOnly
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

function ConvertTo-CounterMap {
    param([Parameter(Mandatory = $true)][string]$Text)

    $counters = @{}
    foreach ($match in [regex]::Matches($Text, "(?<name>[A-Za-z0-9_]+)=(?<value>[^`\s]+)")) {
        $counters[$match.Groups["name"].Value] = $match.Groups["value"].Value
    }
    return $counters
}

function Invoke-CapturedValidationCommand {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$Label,
        [switch]$AllowFailure
    )

    $outputLines = @(& $FilePath @Arguments 2>&1 | ForEach-Object { $_.ToString() })
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $outputText = $outputLines -join "`n"
    if ($exitCode -ne 0 -and -not $AllowFailure.IsPresent) {
        if (-not [string]::IsNullOrWhiteSpace($outputText)) {
            Write-Information $outputText -InformationAction Continue
        }
        Write-Error "$Label failed with exit code $exitCode."
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $outputText
        Counters = ConvertTo-CounterMap -Text $outputText
    }
}

function Resolve-RequiredBuiltExecutable {
    param(
        [Parameter(Mandatory = $true)][string[]]$Candidates,
        [Parameter(Mandatory = $true)][string]$SearchRoot,
        [Parameter(Mandatory = $true)][string]$FileName,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($candidate in $Candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    if (Test-Path -LiteralPath $SearchRoot -PathType Container) {
        $found = Get-ChildItem -LiteralPath $SearchRoot -Recurse -Filter $FileName -File -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($found) {
            return $found.FullName
        }
    }

    Write-Error "$Label executable was not found under $SearchRoot."
}

function Assert-CounterEquals {
    param(
        [Parameter(Mandatory = $true)]$Counters,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Expected
    )

    if (-not $Counters.ContainsKey($Name)) {
        Write-Error "missing required runtime UI platform production counter '$Name'."
    }

    $actual = [string]$Counters[$Name]
    if ($actual -ne $Expected) {
        if ($Name -like "*selected_adapter" -or $Name -eq "runtime_ui_renderer_upload_selected_backend") {
            Write-Error "runtime UI platform production selected row is host gated or unavailable: $Name=$actual expected $Expected."
        }
        Write-Error "runtime UI platform production counter '$Name' was '$actual', expected '$Expected'."
    }
}

if ($RequireReady.IsPresent -and $SkipPackage.IsPresent) {
    Write-Error "-RequireReady cannot be combined with -SkipPackage because package counters are required."
}
if ($RequireReady.IsPresent -and $SkipEditor.IsPresent) {
    Write-Error "-RequireReady cannot be combined with -SkipEditor because editor_runtime_ui_editor_panel_visible=1 is required."
}

$root = Get-RepoRoot
Push-Location $root
try {
$cleanRoomCheckScriptPath = Join-Path $root "tools/check-first-party-ui-clean-room.ps1"
& $cleanRoomCheckScriptPath

if ($StaticOnly.IsPresent) {
    Write-Information "runtime-ui-platform-production-validation: static-only ok" -InformationAction Continue
    return
}

$tools = Assert-CppBuildTools

$devTargets = @(
    "MK_runtime_ui_platform_production_tests",
    "MK_win32_ui_text_font_tests",
    "MK_win32_platform_tests",
    "MK_ui_renderer_tests",
    "MK_runtime_rhi_tests"
)
if (-not $SkipEditor.IsPresent) {
    $devTargets += @(
        "MK_editor_core_tests",
        "MK_editor_native_shell_tests"
    )
}

Invoke-CheckedCommand $tools.CMake --preset dev
$devBuildArgs = @("--build", "--preset", "dev", "--target") + $devTargets
Invoke-CheckedCommand $tools.CMake @devBuildArgs
$ctestPattern = ($devTargets | ForEach-Object { [regex]::Escape($_) }) -join "|"
Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R $ctestPattern

$packageCounters = @{}
if (-not $SkipPackage.IsPresent) {
    Invoke-CheckedCommand $tools.CMake --preset desktop-runtime
    Invoke-CheckedCommand $tools.CMake --build --preset desktop-runtime --target sample_2d_desktop_runtime_package
    $packageBuildRoot = Join-Path $root "out/build/desktop-runtime"
    $packageExe = Resolve-RequiredBuiltExecutable `
        -Candidates @(
            (Join-Path $packageBuildRoot "games/Debug/sample_2d_desktop_runtime_package/sample_2d_desktop_runtime_package.exe"),
            (Join-Path $packageBuildRoot "games/sample_2d_desktop_runtime_package/sample_2d_desktop_runtime_package.exe"),
            (Join-Path $packageBuildRoot "games/sample_2d_desktop_runtime_package/sample_2d_desktop_runtime_package")
        ) `
        -SearchRoot $packageBuildRoot `
        -FileName "sample_2d_desktop_runtime_package.exe" `
        -Label "sample_2d_desktop_runtime_package"
    $packageResult = Invoke-CapturedValidationCommand `
        -FilePath $packageExe `
        -Arguments @(
            "--smoke",
            "--require-config",
            "runtime/sample_2d_desktop_runtime_package.config",
            "--require-scene-package",
            "runtime/sample_2d_desktop_runtime_package.geindex",
            "--require-runtime-ui-platform-package"
        ) `
        -Label "sample_2d_desktop_runtime_package --smoke --require-runtime-ui-platform-package"
    if ($packageResult.ExitCode -ne 0) {
        Write-Information $packageResult.Output -InformationAction Continue
    }
    $packageCounters = $packageResult.Counters
}

$editorCounters = @{}
if (-not $SkipEditor.IsPresent -and $RequireReady.IsPresent) {
    Invoke-CheckedCommand $tools.CMake --preset desktop-editor
    Invoke-CheckedCommand $tools.CMake --build --preset desktop-editor --target MK_editor
    $editorBuildRoot = Join-Path $root "out/build/desktop-editor"
    $editorExe = Resolve-RequiredBuiltExecutable `
        -Candidates @(
            (Join-Path $editorBuildRoot "editor/Debug/MK_editor.exe"),
            (Join-Path $editorBuildRoot "editor/MK_editor.exe"),
            (Join-Path $editorBuildRoot "Debug/MK_editor.exe"),
            (Join-Path $editorBuildRoot "MK_editor.exe")
        ) `
        -SearchRoot $editorBuildRoot `
        -FileName "MK_editor.exe" `
        -Label "MK_editor"
    $editorResult = Invoke-CapturedValidationCommand `
        -FilePath $editorExe `
        -Arguments @("--smoke-frames", "2", "--smoke-resize", "--no-user-config") `
        -Label "MK_editor --smoke-frames 2 --smoke-resize --no-user-config"
    $editorCounters = $editorResult.Counters
}

if ($RequireReady.IsPresent) {
    foreach ($expected in @(
            "runtime_ui_platform_runtime_package_ready=1",
            "runtime_ui_platform_production_ready=0",
            "runtime_ui_platform_clean_room_ready=1",
            "runtime_ui_platform_external_engine_parity_claim=0",
            "runtime_ui_platform_public_native_handles_exposed=0",
            "runtime_ui_text_shaping_selected_adapter=directwrite",
            "runtime_ui_font_rasterization_selected_adapter=directwrite",
            "runtime_ui_ime_selected_adapter=tsf",
            "runtime_ui_accessibility_selected_adapter=uia",
            "runtime_ui_renderer_upload_selected_backend=d3d12"
        )) {
        $parts = $expected.Split("=", 2)
        Assert-CounterEquals -Counters $packageCounters -Name $parts[0] -Expected $parts[1]
    }
    Assert-CounterEquals -Counters $editorCounters -Name "editor_runtime_ui_editor_panel_visible" -Expected "1"
    Write-Information "runtime_ui_platform_production_ready=1 runtime_ui_platform_runtime_package_ready=1 editor_runtime_ui_editor_panel_visible=1" -InformationAction Continue
}

if ($RequireReady.IsPresent) {
    Write-Information "runtime-ui-platform-production-validation: ready ok" -InformationAction Continue
} else {
    Write-Information "runtime-ui-platform-production-validation: package-proof ok" -InformationAction Continue
}
} finally {
    Pop-Location
}
