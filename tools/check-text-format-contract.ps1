#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

function Assert-TextFormatContract {
    param(
        [Parameter(Mandatory = $true)]
        [bool]$Condition,

        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string]$Message
    )

    if (-not $Condition) {
        Write-Error $Message
    }
}

function New-TextFormatFixture {
    param(
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string]$Root
    )

    $null = New-Item -ItemType Directory -Force -Path (Join-Path $Root "docs")
    $null = New-Item -ItemType Directory -Force -Path (Join-Path $Root "games/sample/runtime")
    Set-Content -LiteralPath (Join-Path $Root ".gitattributes") -Value "* text=auto eol=lf`n" -NoNewline -Encoding utf8NoBOM
    Set-Content -LiteralPath (Join-Path $Root "docs/good.md") -Value "good`n" -NoNewline -Encoding utf8NoBOM
    Set-Content -LiteralPath (Join-Path $Root "docs/trailing-blank.md") -Value "body`n`n" -NoNewline -Encoding utf8NoBOM
    Set-Content -LiteralPath (Join-Path $Root "docs/crlf.md") -Value "one`r`ntwo`r`n" -NoNewline -Encoding utf8NoBOM
    [System.IO.File]::WriteAllBytes((Join-Path $Root "docs/bom.md"), [byte[]](0xEF, 0xBB, 0xBF, 0x62, 0x6F, 0x6D, 0x0A))
    Set-Content -LiteralPath (Join-Path $Root "games/sample/runtime/hashed.txt") -Value "hash-owned`r`n`r`n" -NoNewline -Encoding utf8NoBOM

    git -C $Root init --quiet
    if ($LASTEXITCODE -ne 0) {
        Write-Error "git init failed for text format contract fixture."
    }
    $emptyExcludesFile = Join-Path $Root ".git-empty-excludes"
    Set-Content -LiteralPath $emptyExcludesFile -Value "" -NoNewline -Encoding utf8NoBOM
    git -C $Root config core.excludesFile $emptyExcludesFile
    git -C $Root config core.autocrlf false
    git -C $Root config core.safecrlf false
    git -C $Root add .
    if ($LASTEXITCODE -ne 0) {
        Write-Error "git add failed for text format contract fixture."
    }
}

$corePath = Join-Path $PSScriptRoot "text-format-core.ps1"
if (-not (Test-Path -LiteralPath $corePath -PathType Leaf)) {
    Write-Error "Missing text format core helper: tools/text-format-core.ps1"
}

. $corePath

foreach ($requiredFunction in @("Get-MKTrackedTextFile", "ConvertTo-MKCanonicalText", "Get-MKUtf8TextFile", "Test-MKTextFormat", "Repair-MKTextFormat")) {
    if (-not (Get-Command -Name $requiredFunction -CommandType Function -ErrorAction SilentlyContinue)) {
        Write-Error "Missing text format function: $requiredFunction"
    }
}

$fixtureRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("mirakana-text-format-contract-" + [Guid]::NewGuid().ToString("N"))
try {
    New-TextFormatFixture -Root $fixtureRoot

    $issues = @(Test-MKTextFormat -Root $fixtureRoot)
    $issuePaths = @($issues | ForEach-Object { $_.RepoPath })
    Assert-TextFormatContract ($issuePaths -contains "docs/trailing-blank.md") "text format check must flag trailing EOF blank lines."
    Assert-TextFormatContract ($issuePaths -contains "docs/crlf.md") "text format check must flag CRLF text."
    Assert-TextFormatContract ($issuePaths -contains "docs/bom.md") "text format check must flag UTF-8 BOM text."
    Assert-TextFormatContract ($issuePaths -notcontains "docs/good.md") "text format check must not flag canonical text."
    Assert-TextFormatContract ($issuePaths -notcontains "games/sample/runtime/hashed.txt") "text format check must skip runtime package files."

    $changed = Repair-MKTextFormat -Root $fixtureRoot
    Assert-TextFormatContract ($changed -eq 3) "text format repair must normalize exactly the three non-runtime malformed files."
    $afterRepair = @(Test-MKTextFormat -Root $fixtureRoot)
    Assert-TextFormatContract ($afterRepair.Count -eq 0) "text format repair must leave tracked non-runtime text canonical."

    $runtimeText = Get-Content -LiteralPath (Join-Path $fixtureRoot "games/sample/runtime/hashed.txt") -Raw
    Assert-TextFormatContract ($runtimeText -eq "hash-owned`r`n`r`n") "text format repair must not rewrite runtime package files."
} finally {
    if (Test-Path -LiteralPath $fixtureRoot) {
        Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    }
}

Write-Host "text-format-contract-check: ok"
