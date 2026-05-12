#requires -Version 7.0
#requires -PSEdition Core

<#
.SYNOPSIS
  Composes engine/agent/manifest.json from engine/agent/manifest.fragments/*.json.

.DESCRIPTION
  Fragments are merged in lexicographic filename order. Each fragment must be a JSON object;
  top-level keys must not collide across fragments.

  -Write refreshes the canonical manifest.json from fragments.
  Default (no -Write): prints composed JSON to stdout.
  -Verify exits 0 when canonical manifest.json is semantically equal to the composed result.

  One-time migration: -SplitFromCanonical reads manifest.json and writes manifest.fragments/*.json
  (destructive to fragment directory — use only for bootstrap or repair).
#>

param(
    [switch] $Write,
    [switch] $Verify,
    [switch] $SplitFromCanonical
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$fragDir = Join-Path $root "engine/agent/manifest.fragments"
$manifestPath = Join-Path $root "engine/agent/manifest.json"

function Get-ComposedJsonNode {
    if (-not (Test-Path -LiteralPath $fragDir)) {
        Write-Error "Missing fragment directory: $fragDir"
    }

    $files = @(Get-ChildItem -LiteralPath $fragDir -File -Filter "*.json" | Sort-Object Name)
    if ($files.Count -eq 0) {
        Write-Error "No JSON fragments under $fragDir"
    }

    $merged = [System.Text.Json.Nodes.JsonObject]::new()
    foreach ($file in $files) {
        $text = Get-Content -LiteralPath $file.FullName -Raw -Encoding Utf8
        $node = [System.Text.Json.Nodes.JsonNode]::Parse($text)
        if ($null -eq $node -or $node.GetType().Name -ne "JsonObject") {
            Write-Error "Fragment must be a JSON object: $($file.FullName)"
        }

        foreach ($prop in $node.AsObject()) {
            $key = $prop.Key
            if ($merged.ContainsKey($key)) {
                Write-Error "Duplicate top-level key '$key' while merging $($file.FullName)"
            }

            $merged[$key] = [System.Text.Json.Nodes.JsonNode]::Parse($prop.Value.ToJsonString())
        }
    }

    return ,$merged
}

function Get-CanonicalJsonNode {
    if (-not (Test-Path -LiteralPath $manifestPath)) {
        Write-Error "Missing canonical manifest: $manifestPath"
    }

    $text = Get-Content -LiteralPath $manifestPath -Raw -Encoding Utf8
    $node = [System.Text.Json.Nodes.JsonNode]::Parse($text)
    return ,($node.AsObject())
}

$writeOptions = [System.Text.Json.JsonSerializerOptions]::new()
$writeOptions.WriteIndented = $true

if ($SplitFromCanonical) {
    if (-not (Test-Path -LiteralPath $manifestPath)) {
        Write-Error "Missing $manifestPath"
    }

    New-Item -ItemType Directory -Path $fragDir -Force | Out-Null
    Get-ChildItem -LiteralPath $fragDir -File -Filter "*.json" | Remove-Item -Force

    $obj = Get-CanonicalJsonNode
    $order = @(
        "schemaVersion",
        "engine",
        "commands",
        "windowsDiagnosticsToolchain",
        "modules",
        "applications",
        "runtimeBackendReadiness",
        "importerCapabilities",
        "packagingTargets",
        "validationRecipes",
        "aiOperableProductionLoop",
        "aiSurfaces",
        "documentationPolicy",
        "sourcePolicy",
        "gameCodeGuidance",
        "aiDrivenGameWorkflow"
    )

    $index = 0
    foreach ($key in $order) {
        if (-not $obj.ContainsKey($key)) {
            continue
        }

        $fragment = [System.Text.Json.Nodes.JsonObject]::new()
        $fragment[$key] = [System.Text.Json.Nodes.JsonNode]::Parse($obj[$key].ToJsonString())
        $prefix = "{0:D3}" -f $index
        $safeKey = $key -replace '[^a-zA-Z0-9_-]', '_'
        $outFile = Join-Path $fragDir "$prefix-$safeKey.json"
        $json = $fragment.ToJsonString($writeOptions)
        Set-Content -LiteralPath $outFile -Value $json -Encoding utf8NoBOM
        $index++
    }

    Write-Host "Wrote $($index) fragment(s) to $fragDir"
    exit 0
}

$composed = Get-ComposedJsonNode

if ($Verify) {
    $canonical = Get-CanonicalJsonNode
    if (-not [System.Text.Json.Nodes.JsonNode]::DeepEquals($composed, $canonical)) {
        Write-Error "engine/agent/manifest.json does not match composed manifest.fragments output. Run: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write"
    }

    Write-Host "agent-manifest-compose: ok"
    exit 0
}

$outText = $composed.ToJsonString($writeOptions)
if ($Write) {
    Set-Content -LiteralPath $manifestPath -Value $outText -Encoding utf8NoBOM
    Write-Host "Wrote $manifestPath"
    exit 0
}

Write-Output $outText
