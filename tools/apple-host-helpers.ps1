#requires -Version 7.0
#requires -PSEdition Core
# Dot-sourced by Apple/mobile host diagnostic scripts after tools/common.ps1.

function Test-IsMacOS {
    return [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
        [System.Runtime.InteropServices.OSPlatform]::OSX)
}

function Get-AppleDeveloperDirectory {
    $xcodeSelect = Find-CommandOnCombinedPath "xcode-select"
    if (-not $xcodeSelect) {
        return ""
    }

    $output = @(& $xcodeSelect "-p" 2>$null)
    if ($LASTEXITCODE -ne 0 -or $output.Count -eq 0) {
        return ""
    }

    return ([string]$output[0]).Trim()
}

function Test-FullXcodeDeveloperDirectory {
    param([string]$DeveloperDirectory)

    if ([string]::IsNullOrWhiteSpace($DeveloperDirectory)) {
        return $false
    }

    if ($DeveloperDirectory -notmatch "\.app/Contents/Developer$") {
        return $false
    }

    return Test-Path -LiteralPath (Join-Path $DeveloperDirectory "Platforms/iPhoneSimulator.platform")
}

function Test-XcrunSdkAvailable {
    param(
        [string]$Xcrun = "",
        [Parameter(Mandatory = $true)][string]$SdkName
    )

    if ([string]::IsNullOrWhiteSpace($Xcrun)) {
        $Xcrun = Find-CommandOnCombinedPath "xcrun"
    }
    if ([string]::IsNullOrWhiteSpace($Xcrun)) {
        return $false
    }

    $output = @(& $Xcrun "--sdk" $SdkName "--show-sdk-path" 2>$null)
    if ($LASTEXITCODE -ne 0 -or $output.Count -eq 0) {
        return $false
    }

    $sdkPath = ([string]$output[0]).Trim()
    return -not [string]::IsNullOrWhiteSpace($sdkPath) -and (Test-Path -LiteralPath $sdkPath)
}

function Test-XcrunToolAvailable {
    param(
        [string]$Xcrun = "",
        [Parameter(Mandatory = $true)][string]$SdkName,
        [Parameter(Mandatory = $true)][string]$ToolName
    )

    if ([string]::IsNullOrWhiteSpace($Xcrun)) {
        $Xcrun = Find-CommandOnCombinedPath "xcrun"
    }
    if ([string]::IsNullOrWhiteSpace($Xcrun)) {
        return $false
    }

    $output = @(& $Xcrun "--sdk" $SdkName "--find" $ToolName 2>$null)
    if ($LASTEXITCODE -ne 0 -or $output.Count -eq 0) {
        return $false
    }

    $toolPath = ([string]$output[0]).Trim()
    return -not [string]::IsNullOrWhiteSpace($toolPath) -and (Test-Path -LiteralPath $toolPath -PathType Leaf)
}

function Test-IosSimulatorRuntimeAvailable {
    param([string]$Xcrun = "")

    if ([string]::IsNullOrWhiteSpace($Xcrun)) {
        $Xcrun = Find-CommandOnCombinedPath "xcrun"
    }
    if ([string]::IsNullOrWhiteSpace($Xcrun)) {
        return $false
    }

    $jsonText = @(& $Xcrun "simctl" "list" "--json" "runtimes" "available" 2>$null) -join "`n"
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($jsonText)) {
        return $false
    }

    try {
        $json = $jsonText | ConvertFrom-Json
    }
    catch {
        return $false
    }

    foreach ($runtime in @($json.runtimes)) {
        $identifier = [string]$runtime.identifier
        $platform = [string]$runtime.platform
        if ($identifier -match "iOS" -or $platform -eq "iOS") {
            return $true
        }
    }

    return $false
}
