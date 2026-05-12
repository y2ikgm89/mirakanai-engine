#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

function Get-CPackSetValue {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ConfigPath,
        [Parameter(Mandatory = $true)]
        [string]$VariableName
    )

    $escapedName = [regex]::Escape($VariableName)
    foreach ($line in Get-Content -LiteralPath $ConfigPath) {
        if ($line -match "^\s*set\s*\(\s*$escapedName\s+(.+?)\s*\)\s*$") {
            $value = $Matches[1].Trim()
            if ($value.Length -ge 2 -and $value.StartsWith('"') -and $value.EndsWith('"')) {
                return $value.Substring(1, $value.Length - 2)
            }
            return $value
        }
    }

    throw "CPack config is missing ${VariableName}: $ConfigPath"
}

function Assert-ReleaseArtifactFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,
        [Parameter(Mandatory = $true)]
        [string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Missing release package $Description`: $Path"
    }

    $item = Get-Item -LiteralPath $Path
    if ($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
        throw "Release package $Description must not be a reparse point: $Path"
    }
    if ($item.Length -le 0) {
        throw "Release package $Description is empty: $Path"
    }
}

function Get-ReleasePackageChecksumSidecar {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ChecksumPath,
        [Parameter(Mandatory = $true)]
        [string]$ExpectedFileName
    )

    $content = (Get-Content -LiteralPath $ChecksumPath -Raw).Trim()
    if ($content -notmatch "^(?<hash>[A-Fa-f0-9]{64})(?:\s+\*?(?<file>.+))?$") {
        throw "Release package checksum sidecar is not a SHA-256 sidecar: $ChecksumPath"
    }

    $fileName = $Matches["file"]
    if (-not [string]::IsNullOrWhiteSpace($fileName)) {
        $normalized = $fileName.Trim()
        if ($normalized -ne $ExpectedFileName) {
            throw "Release package checksum sidecar names '$normalized' but expected '$ExpectedFileName'."
        }
    }

    return $Matches["hash"].ToLowerInvariant()
}

function Assert-ReleasePackageZipEntries {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ZipPath,
        [Parameter(Mandatory = $true)]
        [string]$PackageBaseName
    )

    Add-Type -AssemblyName System.IO.Compression.FileSystem

    $requiredEntries = @(
        "lib/cmake/Mirakanai/MirakanaiConfig.cmake",
        "lib/cmake/Mirakanai/MirakanaiConfigVersion.cmake",
        "share/Mirakanai/manifest.json",
        "share/Mirakanai/schemas/engine-agent.schema.json",
        "share/Mirakanai/schemas/engine-agent/ai-operable-production-loop.schema.json",
        "share/Mirakanai/schemas/game-agent.schema.json",
        "share/Mirakanai/tools/validate.ps1",
        "share/Mirakanai/tools/agent-context.ps1",
        "share/Mirakanai/examples/installed_consumer/CMakeLists.txt",
        "share/doc/Mirakanai/README.md",
        "share/Mirakanai/THIRD_PARTY_NOTICES.md",
        "share/Mirakanai/LICENSES/LicenseRef-Proprietary.txt"
    )

    $archive = [System.IO.Compression.ZipFile]::OpenRead($ZipPath)
    try {
        foreach ($requiredEntry in $requiredEntries) {
            $expectedName = "$PackageBaseName/$requiredEntry"
            $entry = $null
            foreach ($candidate in $archive.Entries) {
                $candidateName = $candidate.FullName.Replace("\", "/").TrimStart("/")
                if ($candidateName -eq $expectedName) {
                    $entry = $candidate
                    break
                }
            }

            if ($null -eq $entry) {
                throw "Package archive is missing required entry: $expectedName"
            }
            if ($entry.Length -le 0) {
                throw "Package archive required entry is empty: $expectedName"
            }
        }
    }
    finally {
        $archive.Dispose()
    }
}

function Assert-ReleasePackageArtifacts {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BuildDir
    )

    $resolvedBuildDir = [System.IO.Path]::GetFullPath($BuildDir)
    if (-not (Test-Path -LiteralPath $resolvedBuildDir -PathType Container)) {
        throw "Release package build directory was not found: $resolvedBuildDir"
    }

    $cpackConfig = Join-Path $resolvedBuildDir "CPackConfig.cmake"
    Assert-ReleaseArtifactFile -Path $cpackConfig -Description "CPack config"

    $generator = Get-CPackSetValue -ConfigPath $cpackConfig -VariableName "CPACK_GENERATOR"
    if ($generator -notmatch "(^|;)ZIP(;|$)") {
        throw "Release package artifact validation only supports ZIP CPack output; CPACK_GENERATOR='$generator'."
    }

    $checksum = Get-CPackSetValue -ConfigPath $cpackConfig -VariableName "CPACK_PACKAGE_CHECKSUM"
    if ($checksum -ne "SHA256") {
        throw "Release package artifact validation requires CPACK_PACKAGE_CHECKSUM=SHA256; got '$checksum'."
    }

    $packageBaseName = Get-CPackSetValue -ConfigPath $cpackConfig -VariableName "CPACK_PACKAGE_FILE_NAME"
    if ([string]::IsNullOrWhiteSpace($packageBaseName)) {
        throw "CPack package file name is empty in $cpackConfig"
    }
    if ($packageBaseName.IndexOfAny([System.IO.Path]::GetInvalidFileNameChars()) -ge 0) {
        throw "CPack package file name contains invalid filename characters: $packageBaseName"
    }

    $zipName = "$packageBaseName.zip"
    $zipPath = Join-Path $resolvedBuildDir $zipName
    $checksumPath = "$zipPath.sha256"

    Assert-ReleaseArtifactFile -Path $zipPath -Description "ZIP artifact"
    Assert-ReleaseArtifactFile -Path $checksumPath -Description "checksum sidecar"

    $expectedHash = Get-ReleasePackageChecksumSidecar -ChecksumPath $checksumPath -ExpectedFileName $zipName
    $actualHash = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actualHash -ne $expectedHash) {
        throw "Release package SHA-256 mismatch for $zipName`: expected $expectedHash but computed $actualHash."
    }

    Assert-ReleasePackageZipEntries -ZipPath $zipPath -PackageBaseName $packageBaseName
}
