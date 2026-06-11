#requires -Version 7.0
#requires -PSEdition Core

param(
    [AllowEmptyString()]
    [string]$PlanId = "",

    [AllowEmptyString()]
    [string]$SceneId = "",

    [AllowEmptyString()]
    [string]$PackageTarget = "",

    [AllowEmptyString()]
    [string]$ValidationRecipe = "",

    [AllowEmptyString()]
    [string]$BenchmarkCommand = ""
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

function Assert-NonEmptyMavgBenchmarkParameter {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,

        [AllowEmptyString()]
        [string]$Value
    )

    if ([string]::IsNullOrWhiteSpace($Value)) {
        Write-Error "mavg-benchmark-harness: $Name must be non-empty"
    }

    if ($Value -cnotmatch '^[A-Za-z0-9._-]+$') {
        Write-Error "mavg-benchmark-harness: $Name must contain only ASCII letters, digits, dot, underscore, or hyphen"
    }

    $forbiddenTokens = @("native", "handle", "hwnd", "hinstance", "imgui")
    $tokens = $Value.ToLowerInvariant() -split '[^a-z0-9]+'
    foreach ($token in $tokens) {
        if ($token.Length -eq 0) {
            continue
        }
        if ($forbiddenTokens -contains $token -or
            $token.StartsWith("id3d12") -or
            $token.StartsWith("vk") -or
            $token.StartsWith("mtl") -or
            $token.StartsWith("sdl")) {
            Write-Error "mavg-benchmark-harness: $Name must not contain native/backend handle tokens"
        }
    }
}

$requiredParameters = [ordered]@{
    PlanId           = $PlanId
    SceneId          = $SceneId
    PackageTarget    = $PackageTarget
    ValidationRecipe = $ValidationRecipe
    BenchmarkCommand = $BenchmarkCommand
}

foreach ($entry in $requiredParameters.GetEnumerator()) {
    Assert-NonEmptyMavgBenchmarkParameter -Name $entry.Key -Value $entry.Value
}

Write-Output "mavg-benchmark-harness: mode=dry-run"
Write-Output "mavg-benchmark-harness: plan_id=$PlanId"
Write-Output "mavg-benchmark-harness: scene_id=$SceneId"
Write-Output "mavg-benchmark-harness: package_target=$PackageTarget"
Write-Output "mavg-benchmark-harness: validation_recipe=$ValidationRecipe"
Write-Output "mavg-benchmark-harness: benchmark_command=$BenchmarkCommand"
Write-Output "mavg-benchmark-harness: executes_benchmark=false"
Write-Output "mavg-benchmark-harness: writes_artifacts=false"
Write-Output "mavg-benchmark-harness: invokes_profiler=false"
Write-Output "mavg-benchmark-harness: native_handles=false"
