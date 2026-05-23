#requires -Version 7.0
#requires -PSEdition Core

function Assert-ProductionCompletionCorpus {
    $splitRoot = Join-Path $root "docs/superpowers/master-plans/production-completion-v1"
    if (-not (Test-Path -LiteralPath $splitRoot -PathType Container)) {
        Write-Error "Missing production-completion split corpus: docs/superpowers/master-plans/production-completion-v1"
    }

    $expectedNames = @(
        "01-one-dot-zero-readiness-ledger.md",
        "04-developer-owned-engine-capability-backlog.md",
        "05-projections-and-scenarios.md",
        "99-historical-verdict-archive.md"
    )
    $actualNames = @(Get-ChildItem -LiteralPath $splitRoot -Filter "*.md" -File | ForEach-Object { $_.Name } | Sort-Object)
    $expectedSortedNames = @($expectedNames | Sort-Object)
    if (($actualNames -join "|") -ne ($expectedSortedNames -join "|")) {
        Write-Error "docs/superpowers/master-plans/production-completion-v1 must contain exactly: $($expectedSortedNames -join ', ')"
    }
}

function Assert-SpecStatusSection {
    $specRoot = Join-Path $root "docs/specs"
    if (-not (Test-Path -LiteralPath $specRoot -PathType Container)) {
        Write-Error "Missing specs directory: docs/specs"
    }

    foreach ($specFile in Get-ChildItem -LiteralPath $specRoot -Filter "*.md" -File | Sort-Object Name) {
        if ($specFile.Name -eq "README.md") {
            continue
        }
        $specText = Get-Content -LiteralPath $specFile.FullName -Raw
        if (-not [System.Text.RegularExpressions.Regex]::IsMatch($specText, "(?m)^## Status$")) {
            Write-Error "Spec file must contain a ## Status section: docs/specs/$($specFile.Name)"
        }
    }
}
