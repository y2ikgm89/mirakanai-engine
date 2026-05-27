#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

$forbiddenPatterns = @(
    @{
        Pattern = '#\s*include\s*[<"](?:Windows|windows|windef|winuser|handleapi|unknwn|objidl|combaseapi|guiddef|propidl|propvarutil|audioclient|audiopolicy|mmdeviceapi|endpointvolume|xinput|gameinput|d3d12|dxgi|dxgi1_\d+|wrl/client)\.h[>"]'
        Label = "native Windows, COM, WASAPI, controller, D3D12, or DXGI header include"
    },
    @{
        Pattern = '\b(?:HWND|HINSTANCE|HMODULE|HMONITOR|HCURSOR|HRAWINPUT|HANDLE|LRESULT|WPARAM|LPARAM|WNDPROC|WNDCLASSW|WNDCLASSEXW|RAWINPUT|MSG|POINT|RECT)\b'
        Label = "native Win32 handle, message, or windowing type"
    },
    @{
        Pattern = '\b(?:ID3D12[A-Za-z0-9_]*|IDXGI[A-Za-z0-9_]*|D3D12_[A-Z0-9_]+|DXGI_[A-Z0-9_]+|D3D_ROOT_SIGNATURE_VERSION)\b'
        Label = "native D3D12 or DXGI symbol"
    },
    @{
        Pattern = '\b(?:IAudioClient|IAudioRenderClient|IAudioClock|IMMDevice[A-Za-z0-9_]*|WAVEFORMATEX|REFERENCE_TIME)\b'
        Label = "native WASAPI or Core Audio symbol"
    },
    @{
        Pattern = '\b(?:XINPUT_STATE|XINPUT_GAMEPAD|XINPUT_KEYSTROKE|GameInput[A-Za-z0-9_]*|IGameInput[A-Za-z0-9_]*)\b'
        Label = "native Windows controller symbol"
    },
    @{
        Pattern = '\b(?:IUnknown|HRESULT|GUID|REFGUID|REFIID|IID|CLSID|PROPVARIANT|CoTaskMem[A-Za-z0-9_]*|Microsoft::WRL|ComPtr)\b'
        Label = "native COM type or smart pointer"
    },
    @{
        Pattern = '#\s*include\s*[<"]SDL3/'
        Label = "SDL3 header include"
    },
    @{
        Pattern = '\bSDL_[A-Za-z0-9_]+\b'
        Label = "SDL3 C API symbol"
    }
)

function Get-NativeDesktopPublicHeaderRoot {
    $engineRoot = Join-Path $root "engine"
    if (Test-Path $engineRoot) {
        Get-ChildItem -LiteralPath $engineRoot -Recurse -Directory -Filter include
    }

    foreach ($relativeRoot in @("editor/core/include", "editor/include")) {
        $absoluteRoot = Join-Path $root $relativeRoot
        if (Test-Path $absoluteRoot) {
            Get-Item -LiteralPath $absoluteRoot
        }
    }
}

function Get-NativeDesktopPublicHeaderFile {
    foreach ($headerRoot in Get-NativeDesktopPublicHeaderRoot) {
        Get-ChildItem -LiteralPath $headerRoot.FullName -Recurse -File -Include *.h, *.hpp, *.hh, *.hxx
    }
}

$violations = [System.Collections.Generic.List[string]]::new()

foreach ($file in Get-NativeDesktopPublicHeaderFile) {
    $relativeFile = $file.FullName.Substring($root.Length + 1)
    $lineNumber = 0
    foreach ($line in Get-Content -LiteralPath $file.FullName) {
        $lineNumber += 1
        foreach ($entry in $forbiddenPatterns) {
            if ($line -cmatch $entry.Pattern) {
                $violations.Add("${relativeFile}:${lineNumber}: native desktop contract violation exposes $($entry.Label): $line") | Out-Null
            }
        }
    }
}

if ($violations.Count -gt 0) {
    $violations | ForEach-Object { Write-Error $_ }
}

Write-Host "native-desktop-contract-check: ok"
