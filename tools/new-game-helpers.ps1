#requires -Version 7.0
#requires -PSEdition Core
# Dot-sourced from new-game.ps1 after common.ps1 and `$scriptRepositoryRoot` are initialized.

$ErrorActionPreference = "Stop"

function Test-ContainsControlCharacter {
    param(
        [string]$Text
    )

    foreach ($character in $Text.ToCharArray()) {
        if ([char]::IsControl($character)) {
            return $true
        }
    }
    return $false
}

function ConvertTo-CppStringLiteralContent {
    param(
        [string]$Text
    )

    $builder = [System.Text.StringBuilder]::new()
    foreach ($character in $Text.ToCharArray()) {
        if ($character -eq [char]'\') {
            [void]$builder.Append("\\")
        } elseif ($character -eq [char]'"') {
            [void]$builder.Append('\"')
        } else {
            [void]$builder.Append($character)
        }
    }
    return $builder.ToString()
}

function Get-Fnv1a64Decimal {
    param(
        [string]$Text
    )

    $hash = [System.Numerics.BigInteger]::Parse("14695981039346656037")
    $prime = [System.Numerics.BigInteger]::Parse("1099511628211")
    $modulus = [System.Numerics.BigInteger]::Parse("18446744073709551616")
    foreach ($byte in [System.Text.Encoding]::UTF8.GetBytes($Text)) {
        $hash = (($hash -bxor [System.Numerics.BigInteger]$byte) * $prime) % $modulus
        if ($hash.Sign -lt 0) {
            $hash += $modulus
        }
    }
    return $hash.ToString()
}

function ConvertTo-LfText {
    param(
        [string]$Text
    )

    return $Text.Replace("`r`n", "`n").Replace("`r", "`n")
}

function Format-CppSourceText {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text
    )

    $format = Get-ClangFormatCommand
    if (-not $format) {
        return $Text
    }

    $tempPath = Join-Path ([System.IO.Path]::GetTempPath()) ("gameengine-new-game-" + [guid]::NewGuid().ToString("N") + ".cpp")
    try {
        [System.IO.File]::WriteAllText($tempPath, (ConvertTo-LfText -Text $Text), [System.Text.UTF8Encoding]::new($false))
        $stylePath = Join-Path $scriptRepositoryRoot ".clang-format"
        if (Test-Path -LiteralPath $stylePath) {
            & $format -i "--style=file:$stylePath" $tempPath
        } else {
            & $format -i $tempPath
        }
        if ($LASTEXITCODE -ne 0) {
            Write-Error "clang-format failed while formatting generated main.cpp"
        }
        $formatted = ConvertTo-LfText -Text ([System.IO.File]::ReadAllText($tempPath, [System.Text.Encoding]::UTF8))
        if (-not $formatted.EndsWith("`n")) {
            $formatted += "`n"
        }
        return $formatted
    } finally {
        if (Test-Path -LiteralPath $tempPath) {
            Remove-Item -LiteralPath $tempPath -Force
        }
    }
}
