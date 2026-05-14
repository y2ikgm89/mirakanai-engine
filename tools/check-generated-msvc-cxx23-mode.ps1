#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$buildRoot = Join-Path $root "out/build/dev"

if (-not (Test-Path $buildRoot)) {
    Write-Host "generated-msvc-cxx23-mode-check: skipped (dev build directory missing)"
    exit 0
}

$projects = Get-ChildItem -Path $buildRoot -Recurse -Filter "*.vcxproj"
if (-not $projects) {
    Write-Host "generated-msvc-cxx23-mode-check: skipped (not a Visual Studio generator build)"
    exit 0
}

$badLatest = @()
$matchingProjects = @()
$cachePath = Join-Path $buildRoot "CMakeCache.txt"
if (-not (Test-Path $cachePath)) {
    Write-Error "generated-msvc-cxx23-mode-check requires CMakeCache.txt in the dev build directory"
}

$msvcCxx23OptionRegex = [regex]"^MK_MSVC_CXX23_STANDARD_OPTION:[^=]+=(/std:c\+\+23(?:preview)?)$"
$cacheLine = Get-Content -LiteralPath $cachePath |
    Where-Object { $msvcCxx23OptionRegex.IsMatch($_) } |
    Select-Object -First 1
if (-not $cacheLine) {
    Write-Error "CMakeCache.txt must define MK_MSVC_CXX23_STANDARD_OPTION as /std:c++23preview or /std:c++23"
}
$cacheMatch = $msvcCxx23OptionRegex.Match($cacheLine)
$expectedOption = $cacheMatch.Groups[1].Value
$expectedOptionPattern = [regex]::Escape($expectedOption)
$expectedLanguageStandards = if ($expectedOption -eq "/std:c++23") {
    @("stdcpp23")
} else {
    @("stdcpp23preview", "stdcpp23")
}
$latestLanguageStandardPattern = "<LanguageStandard>\s*stdcpp(?:latest|26)\s*</LanguageStandard>"

foreach ($project in $projects) {
    if ($project.FullName -match "\\CMakeFiles\\") {
        continue
    }

    $content = Get-Content -LiteralPath $project.FullName -Raw
    if ($content -notmatch "<ClCompile Include=") {
        continue
    }

    if ($content -match "stdcpplatest" -or
        $content -match "std:c\+\+latest" -or
        $content -match $latestLanguageStandardPattern) {
        $badLatest += $project.FullName
    }
    $matchesExpectedRawOption = $content -match $expectedOptionPattern
    $matchesExpectedLanguageStandard = $false
    $projectXml = [xml]$content
    $languageStandardNodes = $projectXml.SelectNodes("//*[local-name()='LanguageStandard']")
    foreach ($node in $languageStandardNodes) {
        $languageStandard = $node.InnerText.Trim()
        if ($expectedLanguageStandards -contains $languageStandard) {
            $matchesExpectedLanguageStandard = $true
            break
        }
    }

    if ($matchesExpectedRawOption -or $matchesExpectedLanguageStandard) {
        $matchingProjects += $project.FullName
    }
}

if ($badLatest.Count -gt 0) {
    Write-Error "Generated MSVC projects must not use /std:c++latest or stdcpplatest:`n$($badLatest -join "`n")"
}

if ($matchingProjects.Count -eq 0) {
    Write-Error "Generated MSVC projects did not contain $expectedOption or an equivalent C++23 LanguageStandard in any C++ target."
}

Write-Host "generated-msvc-cxx23-mode-check: ok"

