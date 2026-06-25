#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function Assert-PathUnderDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Directory,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $pathFull = [System.IO.Path]::GetFullPath($Path)
    $directoryFull = [System.IO.Path]::GetFullPath($Directory).TrimEnd(
        [char[]]@([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar))
    $directoryPrefix = $directoryFull + [System.IO.Path]::DirectorySeparatorChar
    if ($pathFull -ne $directoryFull -and
        -not $pathFull.StartsWith($directoryPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        Write-Error "$Description escaped expected directory '$directoryFull': $pathFull"
    }
}

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Join-Path $root ($RelativePath -replace "/", [System.IO.Path]::DirectorySeparatorChar)
}

function Assert-LinePresent {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$ExpectedLine,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Lines.Contains($ExpectedLine)) {
        Write-Error "$Context missing expected line: $ExpectedLine"
    }
}

function Assert-TextPresent {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Needle,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Text.Contains($Needle)) {
        Write-Error "$Context missing expected text: $Needle"
    }
}

$artifactRootRelative = "artifacts/renderer/commercial-readiness-evidence/fixture-guard-self-test-$PID"
$artifactRoot = ConvertTo-LocalPath $artifactRootRelative
$approvedArtifactRoot = ConvertTo-LocalPath "artifacts/renderer/commercial-readiness-evidence"
$fixtureRoot = ConvertTo-LocalPath "tests/fixtures/renderer/commercial-readiness-evidence/ready"
$validator = Join-Path $root "tools/validate-renderer-commercial-readiness-evidence.ps1"

Assert-PathUnderDirectory -Path $artifactRoot -Directory $approvedArtifactRoot `
    -Description "Renderer commercial readiness fixture guard self-test root"

try {
    if (Test-Path -LiteralPath $artifactRoot) {
        Assert-PathUnderDirectory -Path $artifactRoot -Directory $approvedArtifactRoot `
            -Description "Renderer commercial readiness stale self-test root"
        Remove-Item -LiteralPath $artifactRoot -Recurse -Force
    }

    $null = New-Item -ItemType Directory -Path $artifactRoot -Force
    Copy-Item -Path (Join-Path $fixtureRoot "*") -Destination $artifactRoot -Recurse -Force

    $evidencePath = Join-Path $artifactRoot "evidence.json"
    $evidence = Get-Content -LiteralPath $evidencePath -Raw | ConvertFrom-Json
    foreach ($groupName in @("backend_rows", "package_rows", "quality_rows", "metal_memory_profiling_rows")) {
        $group = $evidence.$groupName
        foreach ($property in @($group.PSObject.Properties)) {
            $leaf = Split-Path -Leaf ([string]$property.Value.artifact_path)
            $property.Value.artifact_path = "$artifactRootRelative/$leaf"
        }
    }
    $evidence | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $evidencePath -Encoding utf8NoBOM

    $validationOutput = @(& pwsh -NoProfile -ExecutionPolicy Bypass -File $validator `
            -RequireReady `
            -ArtifactRootRelative $artifactRootRelative 2>&1)
    $exitCode = $LASTEXITCODE
    $validationLines = @($validationOutput | ForEach-Object { [string]$_ })
    $validationText = [string]::Join("`n", $validationLines)

    if ($exitCode -eq 0) {
        Write-Error "Copied fixture artifacts unexpectedly promoted from a retained artifact root."
    }
    Assert-TextPresent -Text $validationText -Needle "fixture_artifact_rejected" `
        -Context "fixture artifact guard validation"
    Assert-LinePresent -Lines $validationLines `
        -ExpectedLine "renderer_commercial_readiness_fixture_artifacts_rejected=11" `
        -Context "fixture artifact guard validation"
    Assert-LinePresent -Lines $validationLines `
        -ExpectedLine "renderer_commercial_readiness=0" `
        -Context "fixture artifact guard validation"
} finally {
    if (Test-Path -LiteralPath $artifactRoot) {
        Assert-PathUnderDirectory -Path $artifactRoot -Directory $approvedArtifactRoot `
            -Description "Renderer commercial readiness fixture guard cleanup root"
        Remove-Item -LiteralPath $artifactRoot -Recurse -Force
    }
}

Write-Information "renderer-commercial-readiness-evidence-fixture-guard-check: ok" -InformationAction Continue
