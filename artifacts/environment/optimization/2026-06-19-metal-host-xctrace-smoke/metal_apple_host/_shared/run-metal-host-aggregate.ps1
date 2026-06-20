#requires -Version 7.0
#requires -PSEdition Core
$ErrorActionPreference = "Stop"
try {
    & "/Users/runner/work/mirakanai-engine/mirakanai-engine/tools/validate-environment-metal-host-aggregate.ps1" -Jobs 3 *>&1 |
        Tee-Object -FilePath "/Users/runner/work/mirakanai-engine/mirakanai-engine/artifacts/environment/optimization/2026-06-19-metal-host-xctrace-smoke/metal_apple_host/_shared/metal-host-aggregate.log"
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
} catch {
    [string]$_ | Tee-Object -FilePath "/Users/runner/work/mirakanai-engine/mirakanai-engine/artifacts/environment/optimization/2026-06-19-metal-host-xctrace-smoke/metal_apple_host/_shared/metal-host-aggregate.log" -Append
    exit 1
}
