#requires -Version 7.0
#requires -PSEdition Core

param(
    [switch]$RequireAll,
    [string[]]$AdditionalSearchRoot = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

function Join-IfSet($base, $child) {
    if ([string]::IsNullOrWhiteSpace($base)) {
        return $null
    }

    return Join-Path $base $child
}

function Join-IfEnvSet($name, $child) {
    return Join-IfSet (Get-EnvironmentVariableAnyScope $name) $child
}

function Get-ToolRootVariants($base) {
    if ([string]::IsNullOrWhiteSpace($base)) {
        return @()
    }

    return @(
        $base,
        (Join-Path $base "bin"),
        (Join-Path $base "Bin"),
        (Join-Path $base "dxc/bin"),
        (Join-Path $base "vulkan/bin"),
        (Join-Path $base "apple/bin"),
        (Join-Path $base "tools/directx-dxc"),
        (Join-Path $base "tools/spirv-tools"),
        (Join-Path $base "directx-dxc"),
        (Join-Path $base "spirv-tools")
    )
}

function Get-VcpkgToolRoots {
    $roots = @()
    foreach ($name in @("VCPKG_ROOT", "VCPKG_INSTALLATION_ROOT")) {
        $value = Get-EnvironmentVariableAnyScope $name
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $roots += Join-Path $value "installed/x64-windows/tools"
        }
    }

    $roots += Join-Path $root "vcpkg_installed/x64-windows/tools"
    $roots += Join-Path $root "external/vcpkg/installed/x64-windows/tools"
    return @($roots)
}

function Get-ToolCandidates($name) {
    $executableNames = @($name)
    if ($IsWindows -or $env:OS -eq "Windows_NT") {
        $executableNames += "$name.exe"
    }

    $searchRoots = @()
    foreach ($rootCandidate in $AdditionalSearchRoot) {
        $searchRoots += Get-ToolRootVariants $rootCandidate
    }
    $searchRoots += @(
        (Join-Path $root "toolchains/dxc/bin"),
        (Join-Path $root "toolchains/vulkan/bin"),
        (Join-Path $root "toolchains/apple/bin"),
        (Join-Path $root "external/dxc/bin"),
        (Join-Path $root "external/vulkan/bin"),
        (Join-Path $root "external/apple/bin"),
        (Join-IfEnvSet "VULKAN_SDK" "Bin"),
        (Join-IfEnvSet "VK_SDK_PATH" "Bin")
    )
    $searchRoots += Get-ToolRootVariants (Get-EnvironmentVariableAnyScope "MK_SHADER_TOOLCHAIN_ROOT")
    foreach ($toolRoot in Get-VcpkgToolRoots) {
        $searchRoots += Get-ToolRootVariants $toolRoot
    }

    $candidates = @()
    foreach ($rootCandidate in $searchRoots) {
        if ([string]::IsNullOrWhiteSpace($rootCandidate)) {
            continue
        }
        foreach ($executableName in $executableNames) {
            $candidates += Join-Path $rootCandidate $executableName
        }
    }

    return @($candidates | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)
}

function Find-ShaderTool($name) {
    $candidate = Get-FirstExistingFile (Get-ToolCandidates $name)
    if ($null -ne $candidate) {
        return [pscustomobject]@{
            Path = $candidate
            Source = "known-location"
        }
    }

    $command = Find-CommandOnCombinedPath $name
    if ($null -ne $command) {
        return [pscustomobject]@{
            Path = $command
            Source = "PATH"
        }
    }

    if ($name -eq "dxc") {
        $windowsSdkDxc = Get-FirstExistingFile (Get-WindowsSdkDxcCandidates)
        if ($null -ne $windowsSdkDxc) {
            return [pscustomobject]@{
                Path = $windowsSdkDxc
                Source = "Windows SDK known-location"
            }
        }
    }

    return $null
}

function Test-DxcSpirvCodegen($path) {
    if ([string]::IsNullOrWhiteSpace($path) -or -not (Test-Path -LiteralPath $path -PathType Leaf)) {
        return $false
    }

    $probeRoot = Join-Path $root "out\shader-toolchain-probe"
    New-Item -ItemType Directory -Force -Path $probeRoot | Out-Null
    $source = Join-Path $probeRoot "spirv-codegen-probe.hlsl"
    $artifact = Join-Path $probeRoot "spirv-codegen-probe.vs.spv"

    @'
float4 vs_main(uint vertex_id : SV_VertexID) : SV_Position {
    float2 positions[3] = {
        float2(-0.5, -0.5),
        float2( 0.5, -0.5),
        float2( 0.0,  0.5)
    };
    return float4(positions[vertex_id], 0.0, 1.0);
}
'@ | Set-Content -LiteralPath $source -Encoding utf8

    if (Test-Path -LiteralPath $artifact -PathType Leaf) {
        Remove-Item -LiteralPath $artifact -Force
    }

    try {
        $output = & $path -spirv "-fspv-target-env=vulkan1.3" -T vs_6_7 -E vs_main -Fo $artifact $source 2>&1
        if ($LASTEXITCODE -ne 0) {
            return $false
        }
        return (Test-Path -LiteralPath $artifact -PathType Leaf) -and ((Get-Item -LiteralPath $artifact).Length -gt 0)
    } catch {
        return $false
    }
}

$toolRequests = @(
    [ordered]@{
        Name = "dxc"
        Purpose = "D3D12 DXIL and Vulkan SPIR-V shader compilation"
    },
    [ordered]@{
        Name = "spirv-val"
        Purpose = "Vulkan SPIR-V validation"
    },
    [ordered]@{
        Name = "metal"
        Purpose = "Metal IR shader compilation"
    },
    [ordered]@{
        Name = "metallib"
        Purpose = "Metal library packaging"
    }
)

$results = @()
foreach ($tool in $toolRequests) {
    $command = Find-ShaderTool $tool.Name
    $supportsSpirvCodegen = $false
    if ($tool.Name -eq "dxc" -and $null -ne $command) {
        $supportsSpirvCodegen = Test-DxcSpirvCodegen $command.Path
    }
    $results += [pscustomobject]@{
        Name = $tool.Name
        Purpose = $tool.Purpose
        Available = $null -ne $command
        Path = if ($null -ne $command) { $command.Path } else { "" }
        Source = if ($null -ne $command) { $command.Source } else { "" }
        SupportsSpirvCodegen = $supportsSpirvCodegen
    }
}

function Test-ShaderToolAvailable($name) {
    foreach ($result in $results) {
        if ($result.Name -eq $name) {
            return $result.Available
        }
    }

    return $false
}

$dxcResult = $results | Where-Object { $_.Name -eq "dxc" } | Select-Object -First 1
$d3d12Ready = Test-ShaderToolAvailable "dxc"
$dxcSpirvCodegenReady = ($null -ne $dxcResult) -and $dxcResult.Available -and $dxcResult.SupportsSpirvCodegen
$vulkanReady = (Test-ShaderToolAvailable "dxc") -and $dxcSpirvCodegenReady -and (Test-ShaderToolAvailable "spirv-val")
$metalReady = (Test-ShaderToolAvailable "metal") -and (Test-ShaderToolAvailable "metallib")

Write-Host "shader-toolchain: d3d12_dxil=$(if ($d3d12Ready) { "ready" } else { "missing" })"
Write-Host "shader-toolchain: vulkan_spirv=$(if ($vulkanReady) { "ready" } else { "missing" })"
Write-Host "shader-toolchain: metal_library=$(if ($metalReady) { "ready" } else { "missing" })"
Write-Host "shader-toolchain: dxc_spirv_codegen=$(if ($dxcSpirvCodegenReady) { "ready" } else { "missing" })"

foreach ($result in $results) {
    $status = if ($result.Available) { "found" } else { "missing" }
    $suffix = if ($result.Available) { " via $($result.Source) at $($result.Path)" } else { "" }
    Write-Host "shader-toolchain: $($result.Name)=$status$suffix"
}

$missing = @()
if (-not (Test-ShaderToolAvailable "dxc")) {
    $missing += "Missing dxc for D3D12 DXIL and Vulkan SPIR-V shader compilation"
} elseif (-not $dxcSpirvCodegenReady) {
    $missing += "DXC SPIR-V CodeGen is unavailable; install a DXC build compiled with ENABLE_SPIRV_CODEGEN=ON for Vulkan SPIR-V shader compilation"
}
if (-not (Test-ShaderToolAvailable "spirv-val")) {
    $missing += "Missing spirv-val for Vulkan SPIR-V validation"
}
if (-not (Test-ShaderToolAvailable "metal")) {
    $missing += "Missing metal for Metal IR shader compilation"
}
if (-not (Test-ShaderToolAvailable "metallib")) {
    $missing += "Missing metallib for Metal library packaging"
}

foreach ($diagnostic in $missing) {
    Write-Host "shader-toolchain: blocker - $diagnostic"
}

if ($RequireAll -and $missing.Count -gt 0) {
    Write-Error "shader-toolchain-check: required shader tools are missing"
}

if ($missing.Count -gt 0) {
    Write-Host "shader-toolchain-check: diagnostic-only"
} else {
    Write-Host "shader-toolchain-check: ok"
}

