#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

$publicRoots = @(
    "engine/core/include",
    "engine/math/include",
    "engine/platform/include",
    "engine/platform/sdl3/include",
    "engine/runtime/include",
    "engine/runtime_host/include",
    "engine/runtime_host/sdl3/include",
    "engine/runtime_rhi/include",
    "engine/runtime_scene/include",
    "engine/rhi/include",
    "engine/rhi/d3d12/include",
    "engine/rhi/vulkan/include",
    "engine/rhi/metal/include",
    "engine/renderer/include",
    "engine/scene/include",
    "engine/scene_renderer/include",
    "engine/assets/include",
    "engine/audio/include",
    "engine/physics/include",
    "engine/navigation/include",
    "engine/animation/include",
    "engine/tools/include",
    "editor/core/include"
)

$forbiddenPatterns = @(
    @{
        Pattern = '#\s*include\s*[<"](?:windows|d3d12|dxgi|dxgi1_\d+|wrl/client)\.h[>"]'
        Label = "native Windows/D3D12/DXGI header include"
    },
    @{
        Pattern = '\b(?:ID3D12\w*|IDXGI\w*|D3D12_[A-Z0-9_]+|DXGI_[A-Z0-9_]+|D3D_ROOT_SIGNATURE_VERSION)\b'
        Label = "native D3D12/DXGI symbol"
    },
    @{
        Pattern = '\b(?:HWND|HINSTANCE|HMODULE|HMONITOR|HANDLE|LRESULT|WPARAM|LPARAM)\b'
        Label = "native Win32 handle or message type"
    },
    @{
        Pattern = '\b(?:Microsoft::WRL|ComPtr)\b'
        Label = "COM smart pointer type"
    },
    @{
        Pattern = '#\s*include\s*[<"]SDL3/'
        Label = "SDL3 header include"
    },
    @{
        Pattern = '\bSDL_[A-Za-z0-9_]+\b'
        Label = "SDL3 public symbol"
    },
    @{
        Pattern = '#\s*include\s*[<"]imgui'
        Label = "Dear ImGui header include"
    },
    @{
        Pattern = '\bImGui\b'
        Label = "Dear ImGui public symbol"
    },
    @{
        Pattern = '\bVk[A-Z][A-Za-z0-9_]*\b'
        Label = "native Vulkan symbol"
    },
    @{
        Pattern = '\b(?:MTL[A-Za-z0-9_]*|CAMetalLayer|ObjectiveC|objc_object)\b'
        Label = "native Metal/Objective-C symbol"
    },
    @{
        Pattern = '\b(?:ANativeWindow|JNIEnv|jobject|AAudio[A-Za-z0-9_]*)\b'
        Label = "native Android symbol"
    }
)

$moduleBoundaryRules = @(
    @{
        Root = "engine/scene/include"
        Pattern = '#\s*include\s*[<"]mirakana/(?:renderer|rhi|platform|runtime|runtime_host|scene_renderer|ui|ui_renderer|tools|editor)/'
        Label = "mirakana_scene public API depending on renderer/RHI/platform/runtime/UI/tools/editor modules"
    },
    @{
        Root = "engine/assets/include"
        Pattern = '#\s*include\s*[<"]mirakana/(?:renderer|rhi|platform|runtime|runtime_host|scene_renderer|ui_renderer|tools|editor)/'
        Label = "mirakana_assets public API depending on renderer/RHI/platform/runtime/tools/editor modules"
    },
    @{
        Root = "engine/ui/include"
        Pattern = '#\s*include\s*[<"]mirakana/(?:renderer|rhi|platform|runtime|runtime_host|scene_renderer|ui_renderer|tools|editor)/'
        Label = "mirakana_ui public API depending on renderer/RHI/platform/runtime/tools/editor modules"
    }
)

$violations = New-Object System.Collections.Generic.List[string]

foreach ($relativeRoot in $publicRoots) {
    $absoluteRoot = Join-Path $root $relativeRoot
    if (-not (Test-Path $absoluteRoot)) {
        continue
    }

    Get-ChildItem -LiteralPath $absoluteRoot -Recurse -File -Include *.h, *.hpp, *.hh, *.hxx | ForEach-Object {
        $relativeFile = $_.FullName.Substring($root.Length + 1)
        $lineNumber = 0
        foreach ($line in Get-Content -LiteralPath $_.FullName) {
            $lineNumber += 1
            foreach ($entry in $forbiddenPatterns) {
                if ($line -cmatch $entry.Pattern) {
                    $violations.Add("${relativeFile}:${lineNumber}: public API exposes $($entry.Label): $line") | Out-Null
                }
            }
        }
    }
}

foreach ($rule in $moduleBoundaryRules) {
    $absoluteRoot = Join-Path $root $rule.Root
    if (-not (Test-Path $absoluteRoot)) {
        continue
    }

    Get-ChildItem -LiteralPath $absoluteRoot -Recurse -File -Include *.h, *.hpp, *.hh, *.hxx | ForEach-Object {
        $relativeFile = $_.FullName.Substring($root.Length + 1)
        $lineNumber = 0
        foreach ($line in Get-Content -LiteralPath $_.FullName) {
            $lineNumber += 1
            if ($line -cmatch $rule.Pattern) {
                $violations.Add("${relativeFile}:${lineNumber}: public API boundary violation: $($rule.Label): $line") | Out-Null
            }
        }
    }
}

if ($violations.Count -gt 0) {
    $violations | ForEach-Object { Write-Error $_ }
}

Write-Host "public-api-boundary-check: ok"


