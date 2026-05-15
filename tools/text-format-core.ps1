#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

function Get-MKTrackedTextFile {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string]$Root
    )

    $entries = @(git -C $Root ls-files --eol)
    if ($LASTEXITCODE -ne 0) {
        Write-Error "git ls-files --eol failed while enumerating tracked text files."
    }

    foreach ($entry in $entries) {
        $tabParts = $entry -split "`t", 2
        if ($tabParts.Count -ne 2) {
            continue
        }

        $metadata = $tabParts[0]
        $repoPath = $tabParts[1]
        if ($repoPath -match '^games/[^/]+/runtime/') {
            continue
        }

        $metadataParts = @($metadata -split '\s+' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
        if ($metadataParts.Count -lt 1 -or $metadataParts[0] -notmatch '^i/(lf|crlf|mixed)$') {
            continue
        }

        $localPath = $repoPath.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
        $fullPath = Join-Path $Root $localPath
        if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
            continue
        }

        [PSCustomObject]@{
            RepoPath = $repoPath
            FullPath = $fullPath
        }
    }
}

function ConvertTo-MKCanonicalText {
    [CmdletBinding()]
    param(
        [AllowEmptyString()]
        [string]$Text
    )

    $lfText = ConvertTo-LfText $Text
    if ($lfText.Length -gt 0 -and $lfText[0] -eq [char]0xFEFF) {
        $lfText = $lfText.Substring(1)
    }

    $body = [System.Text.RegularExpressions.Regex]::Replace($lfText, "(?:`n[ `t]*)+\z", "")
    if ([string]::IsNullOrWhiteSpace($body)) {
        return ""
    }

    return "$body`n"
}

function Get-MKUtf8TextFile {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string]$Path
    )

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $decoder = [System.Text.UTF8Encoding]::new($false, $true)
    $text = $decoder.GetString($bytes)

    return [PSCustomObject]@{
        Bytes = $bytes
        Text = $text
    }
}

function Test-MKTextFormat {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string]$Root
    )

    foreach ($file in Get-MKTrackedTextFile -Root $Root) {
        try {
            $fileText = Get-MKUtf8TextFile -Path $file.FullPath
        } catch {
            [PSCustomObject]@{
                RepoPath = $file.RepoPath
                Reason = "invalid UTF-8 text"
            }
            continue
        }

        $canonicalText = ConvertTo-MKCanonicalText -Text $fileText.Text
        $canonicalBytes = [System.Text.UTF8Encoding]::new($false).GetBytes($canonicalText)
        if (-not [System.Linq.Enumerable]::SequenceEqual([byte[]]$fileText.Bytes, [byte[]]$canonicalBytes)) {
            [PSCustomObject]@{
                RepoPath = $file.RepoPath
                Reason = "not LF/UTF-8-no-BOM/final-newline normalized or has trailing EOF blank lines"
            }
        }
    }
}

function Repair-MKTextFormat {
    [CmdletBinding(SupportsShouldProcess = $true)]
    param(
        [Parameter(Mandatory = $true)]
        [ValidateNotNullOrEmpty()]
        [string]$Root
    )

    $changed = 0
    $encoding = [System.Text.UTF8Encoding]::new($false)
    foreach ($file in Get-MKTrackedTextFile -Root $Root) {
        try {
            $fileText = Get-MKUtf8TextFile -Path $file.FullPath
        } catch {
            Write-Error "Tracked text file is not valid UTF-8: $($file.RepoPath)"
        }

        $canonicalText = ConvertTo-MKCanonicalText -Text $fileText.Text
        $canonicalBytes = $encoding.GetBytes($canonicalText)
        if ([System.Linq.Enumerable]::SequenceEqual([byte[]]$fileText.Bytes, [byte[]]$canonicalBytes)) {
            continue
        }

        if ($PSCmdlet.ShouldProcess($file.FullPath, "normalize text file")) {
            [System.IO.File]::WriteAllText($file.FullPath, $canonicalText, $encoding)
            $changed++
        }
    }

    return $changed
}
