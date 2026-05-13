#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "manifest-command-surface-legacy-guard.ps1")

$root = Get-RepoRoot

$composeVerify = Join-Path $PSScriptRoot "compose-agent-manifest.ps1"
& $composeVerify -Verify

function Read-Json($relativePath) {
    $path = Join-Path $root $relativePath
    if (-not (Test-Path $path)) {
        Write-Error "Missing JSON file: $relativePath"
    }
    return Get-Content -LiteralPath $path -Encoding utf8 -Raw | ConvertFrom-Json
}

function Assert-Properties($object, [string[]]$properties, $label) {
    foreach ($property in $properties) {
        if (-not $object.PSObject.Properties.Name.Contains($property)) {
            Write-Error "$label missing required property: $property"
        }
    }
}

function Assert-ContainsText($text, $needle, $label) {
    if (-not $text.Contains($needle)) {
        Write-Error "$label did not contain expected text: $needle"
    }
}

function ConvertTo-RepoPath([string]$path) {
    return $path.Replace("\", "/")
}

function Get-RelativeRepoPath([string]$fullPath) {
    return ConvertTo-RepoPath $fullPath.Substring($root.Length + 1)
}

function Get-ActiveChildProductionPlans {
    $masterPlanPath = "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md"
    $plansRoot = Join-Path $root "docs/superpowers/plans"
    if (-not (Test-Path -LiteralPath $plansRoot -PathType Container)) {
        Write-Error "Missing production plan directory: docs/superpowers/plans"
    }

    $activePlans = @()
    Get-ChildItem -LiteralPath $plansRoot -Filter "2026-*.md" -File | ForEach-Object {
        $relativePath = Get-RelativeRepoPath $_.FullName
        if ($relativePath -eq $masterPlanPath) {
            return
        }

        $text = Get-Content -LiteralPath $_.FullName -Raw
        if ($text.Contains("**Status:** Active.")) {
            $planId = ""
            $planIdMatch = [regex]::Match($text, '\*\*Plan ID:\*\*\s*`([^`]+)`')
            if ($planIdMatch.Success) {
                $planId = $planIdMatch.Groups[1].Value
            }
            $activePlans += [pscustomobject]@{
                path = $relativePath
                fileName = $_.Name
                planId = $planId
            }
        }
    }

    return @($activePlans)
}

function Assert-ActiveProductionPlanDrift($productionLoop) {
    $masterPlanPath = "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md"
    $planRegistryPath = "docs/superpowers/plans/README.md"
    $planRegistryText = Get-Content -LiteralPath (Join-Path $root $planRegistryPath) -Raw
    $activeSliceRow = [regex]::Match($planRegistryText, '(?m)^\| Active slice \(`currentActivePlan`\) \|.*$')
    if (-not $activeSliceRow.Success) {
        Write-Error "$planRegistryPath must contain an Active slice currentActivePlan row"
    }

    $activeChildPlans = @(Get-ActiveChildProductionPlans)
    if ($activeChildPlans.Count -gt 1) {
        Write-Error "Only one active child production plan is allowed: $(@($activeChildPlans | ForEach-Object { $_.path }) -join ', ')"
    }

    if ($activeChildPlans.Count -eq 1) {
        $activePlan = $activeChildPlans[0]
        if ($productionLoop.currentActivePlan -ne $activePlan.path) {
            Write-Error "engine manifest aiOperableProductionLoop.currentActivePlan must match active child plan: $($activePlan.path)"
        }
        if (-not $activeSliceRow.Value.Contains($activePlan.fileName) -and
            ([string]::IsNullOrWhiteSpace($activePlan.planId) -or -not $activeSliceRow.Value.Contains($activePlan.planId))) {
            Write-Error "$planRegistryPath Active slice row must mention active child plan id or path: $($activePlan.path)"
        }
        if ($activePlan.planId -eq "physics-1-0-collision-system-closeout-v1") {
            $physicsJointsPlanPath = Join-Path $root "docs/superpowers/plans/2026-05-09-physics-joints-foundation-v1.md"
            $physicsBenchmarkPlanPath = Join-Path $root "docs/superpowers/plans/2026-05-09-physics-benchmark-determinism-gates-v1.md"
            $physicsJoltPlanPath = Join-Path $root "docs/superpowers/plans/2026-05-09-physics-jolt-adapter-gate-v1.md"
            $physicsCloseoutPlanPath = Join-Path $root "docs/superpowers/plans/2026-05-09-physics-1-0-collision-system-closeout-v1.md"
            $physicsJointsPlanText = Get-Content -LiteralPath $physicsJointsPlanPath -Raw
            $physicsBenchmarkPlanText = Get-Content -LiteralPath $physicsBenchmarkPlanPath -Raw
            $physicsJoltPlanText = Get-Content -LiteralPath $physicsJoltPlanPath -Raw
            $physicsCloseoutPlanText = Get-Content -LiteralPath $physicsCloseoutPlanPath -Raw
            Assert-ContainsText $physicsJointsPlanText "**Status:** Completed." "docs/superpowers/plans/2026-05-09-physics-joints-foundation-v1.md"
            Assert-ContainsText $physicsBenchmarkPlanText 'Plan ID:** `physics-benchmark-determinism-gates-v1`' "docs/superpowers/plans/2026-05-09-physics-benchmark-determinism-gates-v1.md"
            Assert-ContainsText $physicsBenchmarkPlanText "**Status:** Completed." "docs/superpowers/plans/2026-05-09-physics-benchmark-determinism-gates-v1.md"
            Assert-ContainsText $physicsBenchmarkPlanText 'Gap:** `physics-1-0-collision-system` Phase P2' "docs/superpowers/plans/2026-05-09-physics-benchmark-determinism-gates-v1.md"
            Assert-ContainsText $physicsBenchmarkPlanText "count-based" "docs/superpowers/plans/2026-05-09-physics-benchmark-determinism-gates-v1.md"
            Assert-ContainsText $physicsBenchmarkPlanText "budget gates" "docs/superpowers/plans/2026-05-09-physics-benchmark-determinism-gates-v1.md"
            Assert-ContainsText $physicsBenchmarkPlanText "PhysicsReplaySignature3D" "docs/superpowers/plans/2026-05-09-physics-benchmark-determinism-gates-v1.md"
            Assert-ContainsText $physicsBenchmarkPlanText "evaluate_physics_determinism_gate_3d" "docs/superpowers/plans/2026-05-09-physics-benchmark-determinism-gates-v1.md"
            Assert-ContainsText $physicsJoltPlanText 'Plan ID:** `physics-jolt-adapter-gate-v1`' "docs/superpowers/plans/2026-05-09-physics-jolt-adapter-gate-v1.md"
            Assert-ContainsText $physicsJoltPlanText "**Status:** Completed." "docs/superpowers/plans/2026-05-09-physics-jolt-adapter-gate-v1.md"
            Assert-ContainsText $physicsJoltPlanText 'Gap:** `physics-1-0-collision-system` Phase P3' "docs/superpowers/plans/2026-05-09-physics-jolt-adapter-gate-v1.md"
            Assert-ContainsText $physicsJoltPlanText "Jolt" "docs/superpowers/plans/2026-05-09-physics-jolt-adapter-gate-v1.md"
            Assert-ContainsText $physicsJoltPlanText "explicit 1.0 exclusion" "docs/superpowers/plans/2026-05-09-physics-jolt-adapter-gate-v1.md"
            Assert-ContainsText $physicsCloseoutPlanText 'Plan ID:** `physics-1-0-collision-system-closeout-v1`' "docs/superpowers/plans/2026-05-09-physics-1-0-collision-system-closeout-v1.md"
            Assert-ContainsText $physicsCloseoutPlanText "**Status:** Completed." "docs/superpowers/plans/2026-05-09-physics-1-0-collision-system-closeout-v1.md"
            Assert-ContainsText $physicsCloseoutPlanText 'Gap:** `physics-1-0-collision-system` Phase P4' "docs/superpowers/plans/2026-05-09-physics-1-0-collision-system-closeout-v1.md"
        }
        return
    }

    if ($productionLoop.currentActivePlan -ne $masterPlanPath) {
        Write-Error "engine manifest aiOperableProductionLoop.currentActivePlan must be the master plan when no child plan is active"
    }
    if ($productionLoop.recommendedNextPlan.id -ne "next-production-gap-selection") {
        Write-Error "engine manifest aiOperableProductionLoop.recommendedNextPlan.id must be next-production-gap-selection when no child plan is active"
    }
}

function Assert-DesktopRuntimeGameManifestPath([string]$manifestPath, $label) {
    if ([string]::IsNullOrWhiteSpace($manifestPath)) {
        Write-Error "$label must declare GAME_MANIFEST"
    }

    $normalizedPath = ConvertTo-RepoPath $manifestPath
    if ($normalizedPath.StartsWith("/") -or $normalizedPath -match "^[A-Za-z]:") {
        Write-Error "$label GAME_MANIFEST must be a repository-relative path: $manifestPath"
    }
    if ($normalizedPath.Split("/") -contains "..") {
        Write-Error "$label GAME_MANIFEST must not contain parent-directory segments: $manifestPath"
    }
    if ($normalizedPath -notmatch "^games/[a-z][a-z0-9_]*/game\.agent\.json$") {
        Write-Error "$label GAME_MANIFEST must point at games/<game_name>/game.agent.json: $manifestPath"
    }

    return $normalizedPath
}

function Assert-GameSourceDirectoryLayout {
    $gamesRoot = Join-Path $root "games"
    if (-not (Test-Path -LiteralPath $gamesRoot -PathType Container)) {
        Write-Error "Missing games directory"
    }

    Get-ChildItem -LiteralPath $gamesRoot -Directory | ForEach-Object {
        $gameDirectoryName = $_.Name
        if ($gameDirectoryName -notmatch "^[a-z][a-z0-9_]*$") {
            Write-Error "Game source directory must be lowercase snake_case: games/$gameDirectoryName"
        }
    }
}

function Get-DesktopRuntimeGameRegistrations {
    $gamesCmake = Join-Path $root "games/CMakeLists.txt"
    if (-not (Test-Path $gamesCmake)) {
        Write-Error "Missing games CMake file: games/CMakeLists.txt"
    }

    $content = Get-Content -LiteralPath $gamesCmake -Raw
    $registrations = @{}
    $registrationMatches = [System.Text.RegularExpressions.Regex]::Matches(
        $content,
        "(?ms)^\s*MK_add_desktop_runtime_game\s*\(\s*(?<target>[A-Za-z_][A-Za-z0-9_]*)\b(?<body>.*?^\s*\))"
    )

    foreach ($registrationMatch in $registrationMatches) {
        $target = [string]$registrationMatch.Groups["target"].Value
        $body = [string]$registrationMatch.Groups["body"].Value
        $tokens = @([System.Text.RegularExpressions.Regex]::Matches($body, "[^\s\)]+") | ForEach-Object { $_.Value })
        $gameManifest = ""
        $packageFiles = @()
        $packageFilesFromManifest = $tokens -contains "PACKAGE_FILES_FROM_MANIFEST"
        for ($index = 0; $index -lt $tokens.Count; ++$index) {
            if ($tokens[$index] -eq "GAME_MANIFEST" -and $index + 1 -lt $tokens.Count) {
                $gameManifest = $tokens[$index + 1]
                break
            }
        }
        for ($index = 0; $index -lt $tokens.Count; ++$index) {
            if ($tokens[$index] -ne "PACKAGE_FILES") {
                continue
            }

            for ($fileIndex = $index + 1; $fileIndex -lt $tokens.Count; ++$fileIndex) {
                if (@("SOURCES", "GAME_MANIFEST", "SMOKE_ARGS", "PACKAGE_SMOKE_ARGS", "PACKAGE_FILES", "PACKAGE_FILES_FROM_MANIFEST", "REQUIRES_D3D12_SHADERS", "REQUIRES_VULKAN_SHADERS") -contains $tokens[$fileIndex]) {
                    break
                }
                $packageFiles += $tokens[$fileIndex]
            }
            break
        }

        if ($registrations.ContainsKey($target)) {
            Write-Error "Duplicate MK_add_desktop_runtime_game registration for target '$target'"
        }

        $registrations[$target] = [pscustomobject]@{
            target = $target
            gameManifest = $gameManifest
            hasSmokeArgs = $tokens -contains "SMOKE_ARGS"
            hasPackageSmokeArgs = $tokens -contains "PACKAGE_SMOKE_ARGS"
            packageFiles = @($packageFiles)
            packageFilesFromManifest = $packageFilesFromManifest
        }
    }

    return $registrations
}

function Get-GameDirectoryFromManifestPath([string]$relativePath) {
    return $relativePath.Substring(0, $relativePath.LastIndexOf("/"))
}

function Assert-SnakeCaseGameRelativePath([string]$path, [string]$label) {
    foreach ($segment in $path.Split("/")) {
        if ($segment -notmatch "^[a-z0-9][a-z0-9_]*(\.[a-z0-9][a-z0-9_]*)*$") {
            Write-Error "$label must use lowercase snake_case path segments: $path"
        }
    }
}

function Get-NormalizedRuntimePackageFiles($game, [string]$relativePath) {
    if (-not $game.PSObject.Properties.Name.Contains("runtimePackageFiles")) {
        return @()
    }
    if ($null -eq $game.runtimePackageFiles -or $game.runtimePackageFiles -isnot [System.Array]) {
        Write-Error "$relativePath runtimePackageFiles must be an array"
    }

    $gameDirectory = Get-GameDirectoryFromManifestPath $relativePath
    $packageFiles = @()
    $seenPackageFiles = @{}
    foreach ($entry in $game.runtimePackageFiles) {
        if ($entry -isnot [string]) {
            Write-Error "$relativePath runtimePackageFiles entries must be strings"
        }
        $packageFile = ConvertTo-RepoPath $entry
        if ([string]::IsNullOrWhiteSpace($packageFile)) {
            Write-Error "$relativePath runtimePackageFiles must not contain empty entries"
        }
        if ($packageFile -match ";") {
            Write-Error "$relativePath runtimePackageFiles entries must not contain CMake list separators: $packageFile"
        }
        if ($packageFile.StartsWith("/") -or $packageFile -match "^[A-Za-z]:") {
            Write-Error "$relativePath runtimePackageFiles entries must be relative to the game directory: $packageFile"
        }
        foreach ($segment in $packageFile.Split("/")) {
            if ([string]::IsNullOrWhiteSpace($segment)) {
                Write-Error "$relativePath runtimePackageFiles entries must not contain empty path segments: $packageFile"
            }
            if ($segment -eq "..") {
                Write-Error "$relativePath runtimePackageFiles entries must not contain parent-directory segments: $packageFile"
            }
        }
        if ($packageFile.StartsWith("games/")) {
            Write-Error "$relativePath runtimePackageFiles entries must be relative to the game directory, not repository-relative: $packageFile"
        }
        Assert-SnakeCaseGameRelativePath $packageFile "$relativePath runtimePackageFiles entry"

        $sourcePath = "$gameDirectory/$packageFile"
        $sourceAbsolutePath = Join-Path $root $sourcePath
        if (-not (Test-Path -LiteralPath $sourceAbsolutePath)) {
            Write-Error "$relativePath runtimePackageFiles entry does not exist: $sourcePath"
        }
        if (Test-Path -LiteralPath $sourceAbsolutePath -PathType Container) {
            Write-Error "$relativePath runtimePackageFiles entries must be files, not directories: $sourcePath"
        }
        $resolvedSourcePath = ConvertTo-RepoPath (Resolve-Path -LiteralPath $sourceAbsolutePath).Path
        if ($seenPackageFiles.ContainsKey($resolvedSourcePath)) {
            Write-Error "$relativePath runtimePackageFiles entry resolves to a duplicate file: $sourcePath"
        }
        $seenPackageFiles[$resolvedSourcePath] = $true
        $packageFiles += $sourcePath
    }

    return @($packageFiles)
}

function Test-SafeGameRelativePath([string]$path) {
    $normalizedPath = ConvertTo-RepoPath $path
    if ([string]::IsNullOrWhiteSpace($normalizedPath)) {
        return $false
    }
    if ($normalizedPath -match ";") {
        return $false
    }
    if ($normalizedPath.StartsWith("/") -or $normalizedPath -match "^[A-Za-z]:") {
        return $false
    }
    if ($normalizedPath.StartsWith("games/")) {
        return $false
    }
    foreach ($segment in $normalizedPath.Split("/")) {
        if ([string]::IsNullOrWhiteSpace($segment) -or $segment -eq "..") {
            return $false
        }
    }
    return $true
}

function Assert-ContentRootDoesNotDuplicateCookedPackageEntryPrefix(
    [string]$relativePath,
    [string]$packageIndexPath,
    [string]$contentRoot,
    [string]$label
) {
    if ([string]::IsNullOrWhiteSpace($contentRoot)) {
        return
    }

    $normalizedRoot = (ConvertTo-RepoPath $contentRoot).TrimEnd("/")
    if ([string]::IsNullOrWhiteSpace($normalizedRoot)) {
        return
    }

    $gameDirectory = Get-GameDirectoryFromManifestPath $relativePath
    $indexAbsolutePath = Join-Path $root "$gameDirectory/$packageIndexPath"
    if (-not (Test-Path -LiteralPath $indexAbsolutePath -PathType Leaf)) {
        return
    }

    $indexText = Get-Content -LiteralPath $indexAbsolutePath -Raw
    $entryPrefixPattern = "(?m)^entry\.\d+\.path=$([regex]::Escape($normalizedRoot))/"
    if ([regex]::IsMatch($indexText, $entryPrefixPattern)) {
        Write-Error "$label contentRoot duplicates the cooked package entry path prefix; omit contentRoot when .geindex entry paths already include '$normalizedRoot/'"
    }
}

function Test-SafeAssetKey([string]$key) {
    if ([string]::IsNullOrWhiteSpace($key)) {
        return $false
    }
    if ($key.StartsWith("/") -or $key -match "\s" -or $key -match "[\x00-\x1F\x7F]") {
        return $false
    }
    foreach ($segment in $key.Split("/")) {
        if ([string]::IsNullOrWhiteSpace($segment) -or $segment -eq "..") {
            return $false
        }
    }
    return $true
}

function Assert-RuntimeSceneValidationTargets($game, [string]$relativePath, [bool]$required) {
    $packageFiles = @(Get-NormalizedRuntimePackageFiles $game $relativePath | ForEach-Object {
            $_.Substring((Get-GameDirectoryFromManifestPath $relativePath).Length + 1)
        })
    $packageFileSet = @{}
    foreach ($packageFile in $packageFiles) {
        $packageFileSet[$packageFile] = $true
    }

    if (-not $game.PSObject.Properties.Name.Contains("runtimeSceneValidationTargets")) {
        if ($required) {
            Write-Error "$relativePath cooked scene package manifests must declare runtimeSceneValidationTargets"
        }
        return
    }
    if ($null -eq $game.runtimeSceneValidationTargets -or $game.runtimeSceneValidationTargets -isnot [System.Array]) {
        Write-Error "$relativePath runtimeSceneValidationTargets must be an array"
    }
    if ($required -and @($game.runtimeSceneValidationTargets).Count -lt 1) {
        Write-Error "$relativePath runtimeSceneValidationTargets must contain at least one target"
    }

    $seenIds = @{}
    foreach ($target in @($game.runtimeSceneValidationTargets)) {
        Assert-Properties $target @("id", "packageIndexPath", "sceneAssetKey") "$relativePath runtimeSceneValidationTargets"
        $id = [string]$target.id
        $packageIndexPath = ConvertTo-RepoPath ([string]$target.packageIndexPath)
        $sceneAssetKey = [string]$target.sceneAssetKey

        if ($id -notmatch "^[a-z][a-z0-9-]*$") {
            Write-Error "$relativePath runtimeSceneValidationTargets id must be kebab-case: $id"
        }
        if ($seenIds.ContainsKey($id)) {
            Write-Error "$relativePath runtimeSceneValidationTargets id is duplicated: $id"
        }
        $seenIds[$id] = $true

        if (-not (Test-SafeGameRelativePath $packageIndexPath) -or -not $packageIndexPath.EndsWith(".geindex")) {
            Write-Error "$relativePath runtimeSceneValidationTargets packageIndexPath must be a safe game-relative .geindex path: $packageIndexPath"
        }
        if (-not $packageFileSet.ContainsKey($packageIndexPath)) {
            Write-Error "$relativePath runtimeSceneValidationTargets packageIndexPath must also be declared in runtimePackageFiles: $packageIndexPath"
        }
        if (-not (Test-SafeAssetKey $sceneAssetKey)) {
            Write-Error "$relativePath runtimeSceneValidationTargets sceneAssetKey must be a non-empty safe AssetKeyV2: $sceneAssetKey"
        }

        if ($target.PSObject.Properties.Name.Contains("contentRoot")) {
            $contentRoot = ConvertTo-RepoPath ([string]$target.contentRoot)
            if (-not (Test-SafeGameRelativePath $contentRoot)) {
                Write-Error "$relativePath runtimeSceneValidationTargets contentRoot must be a safe game-relative path: $contentRoot"
            } else {
                Assert-ContentRootDoesNotDuplicateCookedPackageEntryPrefix `
                    $relativePath `
                    $packageIndexPath `
                    $contentRoot `
                    "$relativePath runtimeSceneValidationTargets '$id'"
            }
        }
        foreach ($flag in @("validateAssetReferences", "requireUniqueNodeNames")) {
            if ($target.PSObject.Properties.Name.Contains($flag) -and $target.$flag -isnot [bool]) {
                Write-Error "$relativePath runtimeSceneValidationTargets $flag must be boolean"
            }
        }
        foreach ($forbiddenField in @("command", "shell", "argv", "sourceFile", "sourceScenePath", "sourceAssetPath", "rendererBackend", "nativeHandle", "rhiHandle", "metalDevice")) {
            if ($target.PSObject.Properties.Name.Contains($forbiddenField)) {
                Write-Error "$relativePath runtimeSceneValidationTargets must not expose free-form, source-file, renderer, or native-handle field: $forbiddenField"
            }
        }
    }
}

function Assert-MaterialShaderAuthoringTargets($game, [string]$relativePath, [bool]$required) {
    $gameDirectory = Get-GameDirectoryFromManifestPath $relativePath
    $packageFiles = @(Get-NormalizedRuntimePackageFiles $game $relativePath | ForEach-Object {
            $_.Substring($gameDirectory.Length + 1)
        })
    $packageFileSet = @{}
    foreach ($packageFile in $packageFiles) {
        $packageFileSet[$packageFile] = $true
    }

    if (-not $game.PSObject.Properties.Name.Contains("materialShaderAuthoringTargets")) {
        if ($required) {
            Write-Error "$relativePath material/shader package manifests must declare materialShaderAuthoringTargets"
        }
        return
    }
    if ($null -eq $game.materialShaderAuthoringTargets -or $game.materialShaderAuthoringTargets -isnot [System.Array]) {
        Write-Error "$relativePath materialShaderAuthoringTargets must be an array"
    }
    if ($required -and @($game.materialShaderAuthoringTargets).Count -lt 1) {
        Write-Error "$relativePath materialShaderAuthoringTargets must contain at least one target"
    }

    $seenIds = @{}
    foreach ($target in @($game.materialShaderAuthoringTargets)) {
        Assert-Properties $target @("id", "sourceMaterialPath", "runtimeMaterialPath", "packageIndexPath", "shaderSourcePaths") "$relativePath materialShaderAuthoringTargets"
        $id = [string]$target.id
        $sourceMaterialPath = ConvertTo-RepoPath ([string]$target.sourceMaterialPath)
        $runtimeMaterialPath = ConvertTo-RepoPath ([string]$target.runtimeMaterialPath)
        $packageIndexPath = ConvertTo-RepoPath ([string]$target.packageIndexPath)

        if ($id -notmatch "^[a-z][a-z0-9-]*$") {
            Write-Error "$relativePath materialShaderAuthoringTargets id must be kebab-case: $id"
        }
        if ($seenIds.ContainsKey($id)) {
            Write-Error "$relativePath materialShaderAuthoringTargets id is duplicated: $id"
        }
        $seenIds[$id] = $true

        foreach ($pathRow in @(
                @{ Field = "sourceMaterialPath"; Path = $sourceMaterialPath; Extension = ".material"; RuntimeFile = $false },
                @{ Field = "runtimeMaterialPath"; Path = $runtimeMaterialPath; Extension = ".material"; RuntimeFile = $true },
                @{ Field = "packageIndexPath"; Path = $packageIndexPath; Extension = ".geindex"; RuntimeFile = $true }
            )) {
            if (-not (Test-SafeGameRelativePath $pathRow.Path) -or -not $pathRow.Path.EndsWith($pathRow.Extension)) {
                Write-Error "$relativePath materialShaderAuthoringTargets $($pathRow.Field) must be a safe game-relative $($pathRow.Extension) path: $($pathRow.Path)"
            }
            if ($pathRow.RuntimeFile -and -not $packageFileSet.ContainsKey($pathRow.Path)) {
                Write-Error "$relativePath materialShaderAuthoringTargets $($pathRow.Field) must also be declared in runtimePackageFiles: $($pathRow.Path)"
            }
            if (-not $pathRow.RuntimeFile -and $packageFileSet.ContainsKey($pathRow.Path)) {
                Write-Error "$relativePath materialShaderAuthoringTargets $($pathRow.Field) must not be declared in runtimePackageFiles: $($pathRow.Path)"
            }
            if (-not (Test-Path -LiteralPath (Join-Path $root "$gameDirectory/$($pathRow.Path)"))) {
                Write-Error "$relativePath materialShaderAuthoringTargets $($pathRow.Field) does not exist: $($pathRow.Path)"
            }
        }

        if ($target.shaderSourcePaths -isnot [System.Array] -or @($target.shaderSourcePaths).Count -lt 1) {
            Write-Error "$relativePath materialShaderAuthoringTargets shaderSourcePaths must be a non-empty array"
        }
        foreach ($shaderSourcePath in @($target.shaderSourcePaths)) {
            $shaderPath = ConvertTo-RepoPath ([string]$shaderSourcePath)
            if (-not (Test-SafeGameRelativePath $shaderPath) -or -not $shaderPath.EndsWith(".hlsl")) {
                Write-Error "$relativePath materialShaderAuthoringTargets shaderSourcePaths entries must be safe game-relative .hlsl paths: $shaderPath"
            }
            if ($packageFileSet.ContainsKey($shaderPath)) {
                Write-Error "$relativePath materialShaderAuthoringTargets shaderSourcePaths must not be declared in runtimePackageFiles: $shaderPath"
            }
            if (-not (Test-Path -LiteralPath (Join-Path $root "$gameDirectory/$shaderPath"))) {
                Write-Error "$relativePath materialShaderAuthoringTargets shaderSourcePaths entry does not exist: $shaderPath"
            }
        }

        foreach ($artifactListName in @("d3d12ShaderArtifactPaths", "vulkanShaderArtifactPaths")) {
            if (-not $target.PSObject.Properties.Name.Contains($artifactListName)) {
                continue
            }
            if ($target.$artifactListName -isnot [System.Array]) {
                Write-Error "$relativePath materialShaderAuthoringTargets $artifactListName must be an array"
            }
            foreach ($artifactPathEntry in @($target.$artifactListName)) {
                $artifactPath = ConvertTo-RepoPath ([string]$artifactPathEntry)
                if (-not (Test-SafeGameRelativePath $artifactPath) -or
                    ($artifactListName -eq "d3d12ShaderArtifactPaths" -and -not $artifactPath.EndsWith(".dxil")) -or
                    ($artifactListName -eq "vulkanShaderArtifactPaths" -and -not $artifactPath.EndsWith(".spv"))) {
                    Write-Error "$relativePath materialShaderAuthoringTargets $artifactListName entries must be safe game-relative shader artifact paths: $artifactPath"
                }
                if ($packageFileSet.ContainsKey($artifactPath)) {
                    Write-Error "$relativePath materialShaderAuthoringTargets $artifactListName entries must not be declared in runtimePackageFiles: $artifactPath"
                }
            }
        }

        foreach ($flag in @("validateMaterialTextures", "validateShaderArtifacts")) {
            if ($target.PSObject.Properties.Name.Contains($flag) -and $target.$flag -isnot [bool]) {
                Write-Error "$relativePath materialShaderAuthoringTargets $flag must be boolean"
            }
        }
        foreach ($forbiddenField in @("command", "shell", "argv", "shaderGraph", "materialGraph", "liveShaderGeneration", "rendererBackend", "nativeHandle", "rhiHandle", "metalDevice")) {
            if ($target.PSObject.Properties.Name.Contains($forbiddenField)) {
                Write-Error "$relativePath materialShaderAuthoringTargets must not expose free-form, graph, renderer, or native-handle field: $forbiddenField"
            }
        }
    }
}

function Assert-AtlasTilemapAuthoringTargets($game, [string]$relativePath, [bool]$required) {
    $gameDirectory = Get-GameDirectoryFromManifestPath $relativePath
    $packageFiles = @(Get-NormalizedRuntimePackageFiles $game $relativePath | ForEach-Object {
            $_.Substring($gameDirectory.Length + 1)
        })
    $packageFileSet = @{}
    foreach ($packageFile in $packageFiles) {
        $packageFileSet[$packageFile] = $true
    }

    if (-not $game.PSObject.Properties.Name.Contains("atlasTilemapAuthoringTargets")) {
        if ($required) {
            Write-Error "$relativePath 2D package manifests must declare atlasTilemapAuthoringTargets"
        }
        return
    }
    if ($null -eq $game.atlasTilemapAuthoringTargets -or $game.atlasTilemapAuthoringTargets -isnot [System.Array]) {
        Write-Error "$relativePath atlasTilemapAuthoringTargets must be an array"
    }
    if ($required -and @($game.atlasTilemapAuthoringTargets).Count -lt 1) {
        Write-Error "$relativePath atlasTilemapAuthoringTargets must contain at least one target"
    }

    $validationRecipeIds = @{}
    foreach ($recipe in @($game.validationRecipes)) {
        $validationRecipeIds[[string]$recipe.name] = $true
    }

    $seenIds = @{}
    foreach ($target in @($game.atlasTilemapAuthoringTargets)) {
        Assert-Properties $target @("id", "packageIndexPath", "tilemapPath", "atlasTexturePath", "tilemapAssetKey", "atlasTextureAssetKey", "mode", "sourceDecoding", "atlasPacking", "nativeGpuSpriteBatching") "$relativePath atlasTilemapAuthoringTargets"
        $id = [string]$target.id
        $packageIndexPath = ConvertTo-RepoPath ([string]$target.packageIndexPath)
        $tilemapPath = ConvertTo-RepoPath ([string]$target.tilemapPath)
        $atlasTexturePath = ConvertTo-RepoPath ([string]$target.atlasTexturePath)
        $tilemapAssetKey = [string]$target.tilemapAssetKey
        $atlasTextureAssetKey = [string]$target.atlasTextureAssetKey

        if ($id -notmatch "^[a-z][a-z0-9-]*$") {
            Write-Error "$relativePath atlasTilemapAuthoringTargets id must be kebab-case: $id"
        }
        if ($seenIds.ContainsKey($id)) {
            Write-Error "$relativePath atlasTilemapAuthoringTargets id is duplicated: $id"
        }
        $seenIds[$id] = $true

        foreach ($pathRow in @(
                @{ Field = "packageIndexPath"; Path = $packageIndexPath; Extension = ".geindex" },
                @{ Field = "tilemapPath"; Path = $tilemapPath; Extension = ".tilemap" },
                @{ Field = "atlasTexturePath"; Path = $atlasTexturePath; Extension = ".geasset" }
            )) {
            if (-not (Test-SafeGameRelativePath $pathRow.Path) -or -not $pathRow.Path.EndsWith($pathRow.Extension)) {
                Write-Error "$relativePath atlasTilemapAuthoringTargets $($pathRow.Field) must be a safe game-relative $($pathRow.Extension) path: $($pathRow.Path)"
            }
            if (-not $packageFileSet.ContainsKey($pathRow.Path)) {
                Write-Error "$relativePath atlasTilemapAuthoringTargets $($pathRow.Field) must also be declared in runtimePackageFiles: $($pathRow.Path)"
            }
            if (-not (Test-Path -LiteralPath (Join-Path $root "$gameDirectory/$($pathRow.Path)"))) {
                Write-Error "$relativePath atlasTilemapAuthoringTargets $($pathRow.Field) does not exist: $($pathRow.Path)"
            }
        }

        foreach ($assetKey in @($tilemapAssetKey, $atlasTextureAssetKey)) {
            if (-not (Test-SafeAssetKey $assetKey)) {
                Write-Error "$relativePath atlasTilemapAuthoringTargets asset keys must be safe AssetKeyV2 values: $assetKey"
            }
        }
        if ([string]$target.mode -ne "deterministic-package-data") {
            Write-Error "$relativePath atlasTilemapAuthoringTargets mode must be deterministic-package-data"
        }
        if ([string]$target.sourceDecoding -ne "unsupported") {
            Write-Error "$relativePath atlasTilemapAuthoringTargets sourceDecoding must remain unsupported"
        }
        if ([string]$target.atlasPacking -ne "unsupported") {
            Write-Error "$relativePath atlasTilemapAuthoringTargets atlasPacking must remain unsupported"
        }
        if ([string]$target.nativeGpuSpriteBatching -ne "unsupported") {
            Write-Error "$relativePath atlasTilemapAuthoringTargets nativeGpuSpriteBatching must remain unsupported"
        }
        if ($target.PSObject.Properties.Name.Contains("preflightRecipeIds")) {
            if ($target.preflightRecipeIds -isnot [System.Array] -or @($target.preflightRecipeIds).Count -lt 1) {
                Write-Error "$relativePath atlasTilemapAuthoringTargets preflightRecipeIds must be a non-empty array when present"
            }
            foreach ($recipeId in @($target.preflightRecipeIds)) {
                if (-not $validationRecipeIds.ContainsKey([string]$recipeId)) {
                    Write-Error "$relativePath atlasTilemapAuthoringTargets preflightRecipeIds must reference validationRecipes names: $recipeId"
                }
            }
        }
        foreach ($forbiddenField in @("command", "shell", "argv", "sourceFile", "sourceImagePath", "sourceTilemapPath", "rendererBackend", "nativeHandle", "rhiHandle", "metalDevice", "runtimeImageDecoding", "productionAtlasPacking", "tilemapEditorUX", "productionSpriteBatching", "nativeGpuOutput", "packageStreamingReady", "rendererQualityClaim", "metalReady")) {
            if ($target.PSObject.Properties.Name.Contains($forbiddenField)) {
                Write-Error "$relativePath atlasTilemapAuthoringTargets must not expose unsupported production, renderer, or native-handle field: $forbiddenField"
            }
        }
    }
}

function Assert-PackageStreamingResidencyTargets($game, [string]$relativePath, [bool]$required) {
    $gameDirectory = Get-GameDirectoryFromManifestPath $relativePath
    $packageFiles = @(Get-NormalizedRuntimePackageFiles $game $relativePath | ForEach-Object {
            $_.Substring($gameDirectory.Length + 1)
        })
    $packageFileSet = @{}
    foreach ($packageFile in $packageFiles) {
        $packageFileSet[$packageFile] = $true
    }

    $sceneTargetIds = @{}
    foreach ($sceneTarget in @($game.runtimeSceneValidationTargets)) {
        $sceneTargetId = [string]$sceneTarget.id
        if (-not [string]::IsNullOrWhiteSpace($sceneTargetId)) {
            $sceneTargetIds[$sceneTargetId] = $true
        }
    }

    if (-not $game.PSObject.Properties.Name.Contains("packageStreamingResidencyTargets")) {
        if ($required) {
            Write-Error "$relativePath cooked scene package manifests must declare packageStreamingResidencyTargets"
        }
        return
    }
    if ($null -eq $game.packageStreamingResidencyTargets -or $game.packageStreamingResidencyTargets -isnot [System.Array]) {
        Write-Error "$relativePath packageStreamingResidencyTargets must be an array"
    }
    if ($required -and @($game.packageStreamingResidencyTargets).Count -lt 1) {
        Write-Error "$relativePath packageStreamingResidencyTargets must contain at least one target"
    }

    $allowedResourceKinds = @("texture", "mesh", "morph_mesh_cpu", "animation_float_clip", "animation_quaternion_clip", "sprite_animation", "skinned_mesh", "material", "scene", "physics_collision_scene", "audio", "ui_atlas", "tilemap")
    $seenIds = @{}
    foreach ($target in @($game.packageStreamingResidencyTargets)) {
        Assert-Properties $target @("id", "packageIndexPath", "runtimeSceneValidationTargetId", "mode", "residentBudgetBytes", "safePointRequired") "$relativePath packageStreamingResidencyTargets"
        $id = [string]$target.id
        $packageIndexPath = ConvertTo-RepoPath ([string]$target.packageIndexPath)
        $sceneTargetId = [string]$target.runtimeSceneValidationTargetId

        if ($id -notmatch "^[a-z][a-z0-9-]*$") {
            Write-Error "$relativePath packageStreamingResidencyTargets id must be kebab-case: $id"
        }
        if ($seenIds.ContainsKey($id)) {
            Write-Error "$relativePath packageStreamingResidencyTargets id is duplicated: $id"
        }
        $seenIds[$id] = $true

        if (-not (Test-SafeGameRelativePath $packageIndexPath) -or -not $packageIndexPath.EndsWith(".geindex")) {
            Write-Error "$relativePath packageStreamingResidencyTargets packageIndexPath must be a safe game-relative .geindex path: $packageIndexPath"
        }
        if (-not $packageFileSet.ContainsKey($packageIndexPath)) {
            Write-Error "$relativePath packageStreamingResidencyTargets packageIndexPath must also be declared in runtimePackageFiles: $packageIndexPath"
        }
        if ($sceneTargetId -notmatch "^[a-z][a-z0-9-]*$" -or -not $sceneTargetIds.ContainsKey($sceneTargetId)) {
            Write-Error "$relativePath packageStreamingResidencyTargets runtimeSceneValidationTargetId must reference a manifest runtimeSceneValidationTargets id: $sceneTargetId"
        }
        if ([string]$target.mode -ne "host-gated-safe-point") {
            Write-Error "$relativePath packageStreamingResidencyTargets mode must be host-gated-safe-point for selected safe-point streaming execution"
        }
        if (-not ($target.residentBudgetBytes -is [int] -or $target.residentBudgetBytes -is [long]) -or
            [int64]$target.residentBudgetBytes -lt 1) {
            Write-Error "$relativePath packageStreamingResidencyTargets residentBudgetBytes must be a positive integer"
        }
        if ($target.safePointRequired -ne $true) {
            Write-Error "$relativePath packageStreamingResidencyTargets safePointRequired must be true until safe-point execution is implemented"
        }

        if ($target.PSObject.Properties.Name.Contains("contentRoot")) {
            $contentRoot = ConvertTo-RepoPath ([string]$target.contentRoot)
            if (-not (Test-SafeGameRelativePath $contentRoot)) {
                Write-Error "$relativePath packageStreamingResidencyTargets contentRoot must be a safe game-relative path: $contentRoot"
            } else {
                Assert-ContentRootDoesNotDuplicateCookedPackageEntryPrefix `
                    $relativePath `
                    $packageIndexPath `
                    $contentRoot `
                    "$relativePath packageStreamingResidencyTargets '$id'"
            }
        }
        if ($target.PSObject.Properties.Name.Contains("maxResidentPackages") -and
            (-not ($target.maxResidentPackages -is [int] -or $target.maxResidentPackages -is [long]) -or
                [int64]$target.maxResidentPackages -lt 1)) {
            Write-Error "$relativePath packageStreamingResidencyTargets maxResidentPackages must be a positive integer"
        }
        if ($target.PSObject.Properties.Name.Contains("preloadAssetKeys")) {
            if ($target.preloadAssetKeys -isnot [System.Array] -or @($target.preloadAssetKeys).Count -lt 1) {
                Write-Error "$relativePath packageStreamingResidencyTargets preloadAssetKeys must be a non-empty array when present"
            }
            foreach ($assetKey in @($target.preloadAssetKeys)) {
                if (-not (Test-SafeAssetKey ([string]$assetKey))) {
                    Write-Error "$relativePath packageStreamingResidencyTargets preloadAssetKeys entries must be safe AssetKeyV2 values: $assetKey"
                }
            }
        }
        if ($target.PSObject.Properties.Name.Contains("residentResourceKinds")) {
            if ($target.residentResourceKinds -isnot [System.Array] -or @($target.residentResourceKinds).Count -lt 1) {
                Write-Error "$relativePath packageStreamingResidencyTargets residentResourceKinds must be a non-empty array when present"
            }
            foreach ($resourceKind in @($target.residentResourceKinds)) {
                if (@($allowedResourceKinds) -notcontains ([string]$resourceKind)) {
                    Write-Error "$relativePath packageStreamingResidencyTargets residentResourceKinds has unsupported kind: $resourceKind"
                }
            }
        }
        if ($target.PSObject.Properties.Name.Contains("preflightRecipeIds")) {
            if ($target.preflightRecipeIds -isnot [System.Array] -or @($target.preflightRecipeIds).Count -lt 1) {
                Write-Error "$relativePath packageStreamingResidencyTargets preflightRecipeIds must be a non-empty array when present"
            }
            $validationRecipeIds = @{}
            foreach ($recipe in @($game.validationRecipes)) {
                $validationRecipeIds[[string]$recipe.name] = $true
            }
            foreach ($recipeId in @($target.preflightRecipeIds)) {
                if (-not $validationRecipeIds.ContainsKey([string]$recipeId)) {
                    Write-Error "$relativePath packageStreamingResidencyTargets preflightRecipeIds must reference validationRecipes names: $recipeId"
                }
            }
        }
        foreach ($forbiddenField in @("command", "shell", "argv", "sourceFile", "sourcePackagePath", "sourceAssetPath", "rendererBackend", "nativeHandle", "rhiHandle", "metalDevice", "asyncEviction", "evictionCommand", "streamingThread", "backgroundStreaming", "executionCommand", "allocatorHandle", "allocatorBudgetEnforcement", "gpuBudgetEnforcement", "rendererQualityClaim", "metalReady")) {
            if ($target.PSObject.Properties.Name.Contains($forbiddenField)) {
                Write-Error "$relativePath packageStreamingResidencyTargets must not expose free-form, source-file, async execution, renderer, allocator, or native-handle field: $forbiddenField"
            }
        }
    }
}

function Assert-PrefabScenePackageAuthoringTargets($game, [string]$relativePath, [bool]$required) {
    $gameDirectory = Get-GameDirectoryFromManifestPath $relativePath
    $packageFiles = @(Get-NormalizedRuntimePackageFiles $game $relativePath | ForEach-Object {
            $_.Substring($gameDirectory.Length + 1)
        })
    $packageFileSet = @{}
    foreach ($packageFile in $packageFiles) {
        $packageFileSet[$packageFile] = $true
    }

    $sceneTargetRows = @{}
    foreach ($sceneTarget in @($game.runtimeSceneValidationTargets)) {
        $sceneTargetId = [string]$sceneTarget.id
        if (-not [string]::IsNullOrWhiteSpace($sceneTargetId)) {
            $sceneTargetRows[$sceneTargetId] = $sceneTarget
        }
    }

    $validationRecipeIds = @{}
    foreach ($recipe in @($game.validationRecipes)) {
        $validationRecipeIds[[string]$recipe.name] = $true
    }

    if (-not $game.PSObject.Properties.Name.Contains("prefabScenePackageAuthoringTargets")) {
        if ($required) {
            Write-Error "$relativePath 3D package manifests must declare prefabScenePackageAuthoringTargets"
        }
        return
    }
    if ($null -eq $game.prefabScenePackageAuthoringTargets -or $game.prefabScenePackageAuthoringTargets -isnot [System.Array]) {
        Write-Error "$relativePath prefabScenePackageAuthoringTargets must be an array"
    }
    if ($required -and @($game.prefabScenePackageAuthoringTargets).Count -lt 1) {
        Write-Error "$relativePath prefabScenePackageAuthoringTargets must contain at least one target"
    }

    $expectedAuthoringOperations = @(
        "create-scene",
        "add-scene-node",
        "add-or-update-component",
        "create-prefab",
        "instantiate-prefab"
    )
    $allowedSurfaces = @("GameEngine.Scene.v2", "GameEngine.Prefab.v2")
    $unsupportedSentinelFields = @(
        "broadImporterExecution",
        "broadDependencyCooking",
        "runtimeSourceParsing",
        "materialGraph",
        "shaderGraph",
        "liveShaderGeneration",
        "skeletalAnimation",
        "publicNativeRhiHandles",
        "rendererQuality"
    )

    $seenIds = @{}
    foreach ($target in @($game.prefabScenePackageAuthoringTargets)) {
        Assert-Properties $target @(
            "id",
            "mode",
            "sceneAuthoringPath",
            "prefabAuthoringPath",
            "sourceRegistryPath",
            "packageIndexPath",
            "outputScenePath",
            "sceneAssetKey",
            "runtimeSceneValidationTargetId",
            "authoringCommandRows",
            "selectedSourceAssetKeys",
            "sourceCookMode",
            "sceneMigration",
            "runtimeSceneValidation",
            "hostGatedSmokeRecipeIds",
            "broadImporterExecution",
            "broadDependencyCooking",
            "runtimeSourceParsing",
            "materialGraph",
            "shaderGraph",
            "liveShaderGeneration",
            "skeletalAnimation",
            "gpuSkinning",
            "publicNativeRhiHandles",
            "metalReadiness",
            "rendererQuality"
        ) "$relativePath prefabScenePackageAuthoringTargets"

        $id = [string]$target.id
        $sceneAuthoringPath = ConvertTo-RepoPath ([string]$target.sceneAuthoringPath)
        $prefabAuthoringPath = ConvertTo-RepoPath ([string]$target.prefabAuthoringPath)
        $sourceRegistryPath = ConvertTo-RepoPath ([string]$target.sourceRegistryPath)
        $packageIndexPath = ConvertTo-RepoPath ([string]$target.packageIndexPath)
        $outputScenePath = ConvertTo-RepoPath ([string]$target.outputScenePath)
        $runtimeSceneValidationTargetId = [string]$target.runtimeSceneValidationTargetId
        $sceneAssetKey = [string]$target.sceneAssetKey

        if ($id -notmatch "^[a-z][a-z0-9-]*$") {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets id must be kebab-case: $id"
        }
        if ($seenIds.ContainsKey($id)) {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets id is duplicated: $id"
        }
        $seenIds[$id] = $true

        foreach ($pathRow in @(
                @{ Field = "sceneAuthoringPath"; Path = $sceneAuthoringPath; Extension = ".scene"; RuntimeFile = $false },
                @{ Field = "prefabAuthoringPath"; Path = $prefabAuthoringPath; Extension = ".prefab"; RuntimeFile = $false },
                @{ Field = "sourceRegistryPath"; Path = $sourceRegistryPath; Extension = ".geassets"; RuntimeFile = $false },
                @{ Field = "packageIndexPath"; Path = $packageIndexPath; Extension = ".geindex"; RuntimeFile = $true },
                @{ Field = "outputScenePath"; Path = $outputScenePath; Extension = ".scene"; RuntimeFile = $true }
            )) {
            if (-not (Test-SafeGameRelativePath $pathRow.Path) -or -not $pathRow.Path.EndsWith($pathRow.Extension)) {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets $($pathRow.Field) must be a safe game-relative $($pathRow.Extension) path: $($pathRow.Path)"
            }
            if ($pathRow.RuntimeFile -and -not $packageFileSet.ContainsKey($pathRow.Path)) {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets $($pathRow.Field) must also be declared in runtimePackageFiles: $($pathRow.Path)"
            }
            if (-not $pathRow.RuntimeFile -and $packageFileSet.ContainsKey($pathRow.Path)) {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets $($pathRow.Field) must not be declared in runtimePackageFiles: $($pathRow.Path)"
            }
            if (-not (Test-Path -LiteralPath (Join-Path $root "$gameDirectory/$($pathRow.Path)"))) {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets $($pathRow.Field) does not exist: $($pathRow.Path)"
            }
        }

        if (-not (Test-SafeAssetKey $sceneAssetKey)) {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets sceneAssetKey must be a safe AssetKeyV2 value: $sceneAssetKey"
        }
        if ($runtimeSceneValidationTargetId -notmatch "^[a-z][a-z0-9-]*$" -or -not $sceneTargetRows.ContainsKey($runtimeSceneValidationTargetId)) {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets runtimeSceneValidationTargetId must reference runtimeSceneValidationTargets: $runtimeSceneValidationTargetId"
        } else {
            $sceneTarget = $sceneTargetRows[$runtimeSceneValidationTargetId]
            if ((ConvertTo-RepoPath ([string]$sceneTarget.packageIndexPath)) -ne $packageIndexPath) {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets packageIndexPath must match referenced runtimeSceneValidationTargets row: $packageIndexPath"
            }
            if ([string]$sceneTarget.sceneAssetKey -ne $sceneAssetKey) {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets sceneAssetKey must match referenced runtimeSceneValidationTargets row: $sceneAssetKey"
            }
        }
        if ([string]$target.mode -ne "deterministic-3d-prefab-scene-package-data") {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets mode must be deterministic-3d-prefab-scene-package-data"
        }
        if ([string]$target.sourceCookMode -ne "selected-source-registry-rows") {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets sourceCookMode must be selected-source-registry-rows"
        }
        if ([string]$target.sceneMigration -ne "migrate-scene-v2-runtime-package") {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets sceneMigration must be migrate-scene-v2-runtime-package"
        }
        if ([string]$target.runtimeSceneValidation -ne "validate-runtime-scene-package") {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets runtimeSceneValidation must be validate-runtime-scene-package"
        }

        if ($target.authoringCommandRows -isnot [System.Array] -or @($target.authoringCommandRows).Count -lt 1) {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets authoringCommandRows must be a non-empty array"
        }
        $actualAuthoringOperations = @()
        $authoringRows = @($target.authoringCommandRows)
        for ($rowIndex = 0; $rowIndex -lt $authoringRows.Count; ++$rowIndex) {
            $row = $authoringRows[$rowIndex]
            Assert-Properties $row @("id", "surface", "operation") "$relativePath prefabScenePackageAuthoringTargets authoringCommandRows"
            if ([string]$row.id -notmatch "^[a-z][a-z0-9-]*$") {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets authoringCommandRows id must be kebab-case: $($row.id)"
            }
            if (@($allowedSurfaces) -notcontains ([string]$row.surface)) {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets authoringCommandRows surface must be a reviewed Scene/Prefab v2 surface: $($row.surface)"
            }
            if (@($expectedAuthoringOperations) -notcontains ([string]$row.operation)) {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets authoringCommandRows operation is unsupported: $($row.operation)"
            }
            $expectedSurface = if ([string]$row.operation -eq "create-prefab") { "GameEngine.Prefab.v2" } else { "GameEngine.Scene.v2" }
            if ([string]$row.surface -ne $expectedSurface) {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets authoringCommandRows operation '$($row.operation)' must use $expectedSurface"
            }
            $actualAuthoringOperations += [string]$row.operation
        }
        if (($actualAuthoringOperations -join "|") -ne ($expectedAuthoringOperations -join "|")) {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets authoringCommandRows must be exactly: $($expectedAuthoringOperations -join ', ')"
        }

        $sourceSceneText = Get-Content -LiteralPath (Join-Path $root "$gameDirectory/$sceneAuthoringPath") -Raw
        $sourcePrefabText = Get-Content -LiteralPath (Join-Path $root "$gameDirectory/$prefabAuthoringPath") -Raw
        if ([string]::IsNullOrWhiteSpace($sourceSceneText) -or -not $sourceSceneText.StartsWith("format=GameEngine.Scene.v2`n")) {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets sceneAuthoringPath must contain GameEngine.Scene.v2 text: $sceneAuthoringPath"
        }
        if ([string]::IsNullOrWhiteSpace($sourcePrefabText) -or -not $sourcePrefabText.StartsWith("format=GameEngine.Prefab.v2`n")) {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets prefabAuthoringPath must contain GameEngine.Prefab.v2 text: $prefabAuthoringPath"
        }

        $sourceRegistryFullPath = Join-Path $root "$gameDirectory/$sourceRegistryPath"
        $sourceRegistryText = Get-Content -LiteralPath $sourceRegistryFullPath -Raw
        if ([string]::IsNullOrWhiteSpace($sourceRegistryText) -or -not $sourceRegistryText.StartsWith("format=GameEngine.SourceAssetRegistry.v1`n")) {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets sourceRegistryPath must contain GameEngine.SourceAssetRegistry.v1 text: $sourceRegistryPath"
        }
        $sourceRegistryRows = @{}
        foreach ($line in ($sourceRegistryText -split "`r?`n")) {
            if ($line -match "^asset\.(\d+)\.(kind|source_format)=(.+)$") {
                $rowIndex = $Matches[1]
                if (-not $sourceRegistryRows.ContainsKey($rowIndex)) {
                    $sourceRegistryRows[$rowIndex] = @{}
                }
                $sourceRegistryRows[$rowIndex][$Matches[2]] = $Matches[3]
            }
        }
        foreach ($sourceRow in @(
                @{ SourceFormat = "GameEngine.TextureSource.v1"; Kind = "texture" },
                @{ SourceFormat = "GameEngine.MeshSource.v2"; Kind = "mesh" },
                @{ SourceFormat = "GameEngine.Material.v1"; Kind = "material" }
            )) {
            $hasMatchingSourceRow = $false
            foreach ($registryRow in $sourceRegistryRows.Values) {
                if ($registryRow["kind"] -eq $sourceRow.Kind -and
                    $registryRow["source_format"] -eq $sourceRow.SourceFormat) {
                    $hasMatchingSourceRow = $true
                    break
                }
            }
            if (-not $hasMatchingSourceRow) {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets sourceRegistryPath must include selected $($sourceRow.Kind) row format: $($sourceRow.SourceFormat)"
            }
        }

        if ($target.selectedSourceAssetKeys -isnot [System.Array] -or @($target.selectedSourceAssetKeys).Count -lt 1) {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets selectedSourceAssetKeys must be a non-empty array"
        }
        foreach ($assetKey in @($target.selectedSourceAssetKeys)) {
            $key = [string]$assetKey
            if (-not (Test-SafeAssetKey $key)) {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets selectedSourceAssetKeys entries must be safe AssetKeyV2 values: $key"
            }
            if (-not [System.Text.RegularExpressions.Regex]::IsMatch($sourceRegistryText, "(?m)^asset\.\d+\.key=$([System.Text.RegularExpressions.Regex]::Escape($key))$")) {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets selectedSourceAssetKeys must exist in sourceRegistryPath: $key"
            }
        }

        if ($target.hostGatedSmokeRecipeIds -isnot [System.Array] -or @($target.hostGatedSmokeRecipeIds).Count -lt 1) {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets hostGatedSmokeRecipeIds must be a non-empty array"
        }
        foreach ($recipeId in @($target.hostGatedSmokeRecipeIds)) {
            if (-not $validationRecipeIds.ContainsKey([string]$recipeId)) {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets hostGatedSmokeRecipeIds must reference validationRecipes names: $recipeId"
            }
        }

        foreach ($field in $unsupportedSentinelFields) {
            if ([string]$target.$field -ne "unsupported") {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets $field must remain unsupported"
            }
        }
        $gpuSkinningValue = [string]$target.gpuSkinning
        if ($gpuSkinningValue -eq "unsupported") {
            # Generated 3D package manifests stay descriptor-only unless a committed sample
            # has a host-gated smoke that proves a narrower GPU skinning path.
        } elseif ($relativePath -eq "games/sample_desktop_runtime_game/game.agent.json") {
            $hasSkinnedMeshRuntimeFile = @($packageFiles | Where-Object { ([string]$_).EndsWith(".skinned_mesh") }).Count -gt 0
            $graphicsReadiness = [string]$game.backendReadiness.graphics
            if (-not $hasSkinnedMeshRuntimeFile -or
                -not $gpuSkinningValue.Contains("host-gated D3D12 package smoke") -or
                -not $gpuSkinningValue.Contains("GameEngine.CookedSkinnedMesh.v1") -or
                -not $gpuSkinningValue.Contains("gpu_skinning_draws") -or
                -not $graphicsReadiness.Contains("--require-gpu-skinning") -or
                -not $graphicsReadiness.Contains("renderer_gpu_skinning_draws")) {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets gpuSkinning may only be host-gated after declaring a cooked skinned mesh package file and D3D12 --require-gpu-skinning smoke counters"
            }
        } else {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets gpuSkinning must remain unsupported outside the committed host-gated sample proof"
        }
        if ([string]$target.metalReadiness -ne "host-gated") {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets metalReadiness must remain host-gated"
        }
        foreach ($forbiddenField in @("command", "shell", "argv", "sourceFile", "sourcePackagePath", "runtimeSourceFile", "rendererBackend", "nativeHandle", "rhiHandle", "metalDevice", "broadImporterReady", "broadCookingReady", "runtimeSourceParsingReady", "materialGraphReady", "shaderGraphReady", "liveShaderGenerationReady", "skeletalAnimationReady", "gpuSkinningReady", "rendererQualityClaim", "metalReady")) {
            if ($target.PSObject.Properties.Name.Contains($forbiddenField)) {
                Write-Error "$relativePath prefabScenePackageAuthoringTargets must not expose unsupported production, renderer, or native-handle field: $forbiddenField"
            }
        }
    }
}

function Assert-RegisteredSourceAssetCookTargets($game, [string]$relativePath, [bool]$required) {
    $gameDirectory = Get-GameDirectoryFromManifestPath $relativePath
    $packageFiles = @(Get-NormalizedRuntimePackageFiles $game $relativePath | ForEach-Object {
            $_.Substring($gameDirectory.Length + 1)
        })
    $packageFileSet = @{}
    foreach ($packageFile in $packageFiles) {
        $packageFileSet[$packageFile] = $true
    }

    if (-not $game.PSObject.Properties.Name.Contains("registeredSourceAssetCookTargets")) {
        if ($required) {
            Write-Error "$relativePath 3D package manifests must declare registeredSourceAssetCookTargets"
        }
        return
    }
    if ($null -eq $game.registeredSourceAssetCookTargets -or $game.registeredSourceAssetCookTargets -isnot [System.Array]) {
        Write-Error "$relativePath registeredSourceAssetCookTargets must be an array"
    }
    if ($required -and @($game.registeredSourceAssetCookTargets).Count -lt 1) {
        Write-Error "$relativePath registeredSourceAssetCookTargets must contain at least one target"
    }

    if (-not $game.PSObject.Properties.Name.Contains("prefabScenePackageAuthoringTargets")) {
        Write-Error "$relativePath registeredSourceAssetCookTargets requires prefabScenePackageAuthoringTargets for cross-validation"
    }

    $prefabById = @{}
    foreach ($prefabTarget in @($game.prefabScenePackageAuthoringTargets)) {
        $prefabById[[string]$prefabTarget.id] = $prefabTarget
    }

    $requiredCookFields = @(
        "id",
        "mode",
        "cookCommandId",
        "prefabScenePackageAuthoringTargetId",
        "sourceRegistryPath",
        "packageIndexPath",
        "selectedAssetKeys",
        "dependencyExpansion",
        "dependencyCooking",
        "externalImporterExecution",
        "rendererRhiResidency",
        "packageStreaming",
        "materialGraph",
        "shaderGraph",
        "liveShaderGeneration",
        "editorProductization",
        "metalReadiness",
        "publicNativeRhiHandles",
        "generalProductionRendererQuality",
        "arbitraryShell",
        "freeFormEdit"
    )

    $seenIds = @{}
    foreach ($target in @($game.registeredSourceAssetCookTargets)) {
        Assert-Properties $target $requiredCookFields "$relativePath registeredSourceAssetCookTargets"

        $id = [string]$target.id
        if ($id -notmatch "^[a-z][a-z0-9-]*$") {
            Write-Error "$relativePath registeredSourceAssetCookTargets id must be kebab-case: $id"
        }
        if ($seenIds.ContainsKey($id)) {
            Write-Error "$relativePath registeredSourceAssetCookTargets id is duplicated: $id"
        }
        $seenIds[$id] = $true

        if ([string]$target.mode -ne "descriptor-only-cook-registered-source-assets") {
            Write-Error "$relativePath registeredSourceAssetCookTargets '$id' mode must be descriptor-only-cook-registered-source-assets"
        }
        if ([string]$target.cookCommandId -ne "cook-registered-source-assets") {
            Write-Error "$relativePath registeredSourceAssetCookTargets '$id' cookCommandId must be cook-registered-source-assets"
        }

        $prefabId = [string]$target.prefabScenePackageAuthoringTargetId
        if (-not $prefabById.ContainsKey($prefabId)) {
            Write-Error "$relativePath registeredSourceAssetCookTargets '$id' prefabScenePackageAuthoringTargetId must reference prefabScenePackageAuthoringTargets: $prefabId"
        }
        $prefabTarget = $prefabById[$prefabId]
        if ([string]$target.sourceRegistryPath -ne [string]$prefabTarget.sourceRegistryPath) {
            Write-Error "$relativePath registeredSourceAssetCookTargets '$id' sourceRegistryPath must match prefabScenePackageAuthoringTargets '$prefabId'"
        }
        if ([string]$target.packageIndexPath -ne [string]$prefabTarget.packageIndexPath) {
            Write-Error "$relativePath registeredSourceAssetCookTargets '$id' packageIndexPath must match prefabScenePackageAuthoringTargets '$prefabId'"
        }

        $expansion = [string]$target.dependencyExpansion
        $cooking = [string]$target.dependencyCooking
        if ($expansion -eq "explicit_dependency_selection" -and $cooking -ne "unsupported") {
            Write-Error "$relativePath registeredSourceAssetCookTargets '$id' dependencyCooking must be unsupported when dependencyExpansion is explicit_dependency_selection"
        }
        if ($expansion -eq "registered_source_registry_closure" -and $cooking -ne "registry_closure") {
            Write-Error "$relativePath registeredSourceAssetCookTargets '$id' dependencyCooking must be registry_closure when dependencyExpansion is registered_source_registry_closure"
        }
        if ($expansion -ne "explicit_dependency_selection" -and $expansion -ne "registered_source_registry_closure") {
            Write-Error "$relativePath registeredSourceAssetCookTargets '$id' dependencyExpansion is unsupported: $expansion"
        }

        foreach ($pathRow in @(
                @{ Field = "sourceRegistryPath"; Path = [string]$target.sourceRegistryPath; Extension = ".geassets" },
                @{ Field = "packageIndexPath"; Path = [string]$target.packageIndexPath; Extension = ".geindex" }
            )) {
            if (-not (Test-SafeGameRelativePath $pathRow.Path)) {
                Write-Error "$relativePath registeredSourceAssetCookTargets $($pathRow.Field) must be a safe game-relative $($pathRow.Extension) path: $($pathRow.Path)"
            }
            if ($pathRow.Field -eq "packageIndexPath") {
                if (-not $packageFileSet.ContainsKey($pathRow.Path)) {
                    Write-Error "$relativePath registeredSourceAssetCookTargets $($pathRow.Field) must also be declared in runtimePackageFiles: $($pathRow.Path)"
                }
            } else {
                if ($packageFileSet.ContainsKey($pathRow.Path)) {
                    Write-Error "$relativePath registeredSourceAssetCookTargets $($pathRow.Field) must not be declared in runtimePackageFiles: $($pathRow.Path)"
                }
            }
            $fullPath = Join-Path $root "$gameDirectory/$($pathRow.Path)"
            if (-not (Test-Path -LiteralPath $fullPath)) {
                Write-Error "$relativePath registeredSourceAssetCookTargets $($pathRow.Field) does not exist: $($pathRow.Path)"
            }
        }

        $sourceRegistryFullPath = Join-Path $root "$gameDirectory/$($target.sourceRegistryPath)"
        $sourceRegistryText = Get-Content -LiteralPath $sourceRegistryFullPath -Raw
        if ([string]::IsNullOrWhiteSpace($sourceRegistryText) -or -not $sourceRegistryText.StartsWith("format=GameEngine.SourceAssetRegistry.v1`n")) {
            Write-Error "$relativePath registeredSourceAssetCookTargets sourceRegistryPath must contain GameEngine.SourceAssetRegistry.v1 text: $($target.sourceRegistryPath)"
        }

        if ($target.selectedAssetKeys -isnot [System.Array] -or @($target.selectedAssetKeys).Count -lt 1) {
            Write-Error "$relativePath registeredSourceAssetCookTargets '$id' selectedAssetKeys must be a non-empty array"
        }
        foreach ($assetKey in @($target.selectedAssetKeys)) {
            $key = [string]$assetKey
            if (-not (Test-SafeAssetKey $key)) {
                Write-Error "$relativePath registeredSourceAssetCookTargets '$id' selectedAssetKeys entries must be safe AssetKeyV2 values: $key"
            }
            if (-not [System.Text.RegularExpressions.Regex]::IsMatch($sourceRegistryText, "(?m)^asset\.\d+\.key=$([System.Text.RegularExpressions.Regex]::Escape($key))$")) {
                Write-Error "$relativePath registeredSourceAssetCookTargets '$id' selectedAssetKeys must exist in sourceRegistryPath: $key"
            }
        }

        foreach ($field in @("externalImporterExecution", "rendererRhiResidency", "packageStreaming", "materialGraph", "shaderGraph", "liveShaderGeneration", "editorProductization", "publicNativeRhiHandles", "generalProductionRendererQuality", "arbitraryShell", "freeFormEdit")) {
            if ([string]$target.$field -ne "unsupported") {
                Write-Error "$relativePath registeredSourceAssetCookTargets '$id' $field must remain unsupported"
            }
        }
        if ([string]$target.metalReadiness -ne "host-gated") {
            Write-Error "$relativePath registeredSourceAssetCookTargets '$id' metalReadiness must remain host-gated"
        }
        foreach ($forbiddenField in @("command", "shell", "argv", "sourceFile", "sourcePackagePath", "runtimeSourceFile", "rendererBackend", "nativeHandle", "rhiHandle", "metalDevice", "broadImporterReady", "broadCookingReady", "runtimeSourceParsingReady", "materialGraphReady", "shaderGraphReady", "liveShaderGenerationReady", "rendererQualityClaim", "metalReady")) {
            if ($target.PSObject.Properties.Name.Contains($forbiddenField)) {
                Write-Error "$relativePath registeredSourceAssetCookTargets must not expose unsupported production, renderer, or native-handle field: $forbiddenField"
            }
        }
    }
}

function Assert-DesktopRuntimePackageFileRegistration($game, [string]$relativePath, $registration) {
    $manifestPackageFiles = @(Get-NormalizedRuntimePackageFiles $game $relativePath)
    $registeredPackageFiles = @($registration.packageFiles | ForEach-Object { ConvertTo-RepoPath ([string]$_) })

    if ($registration.packageFilesFromManifest) {
        if ($registeredPackageFiles.Count -gt 0) {
            Write-Error "$relativePath MK_add_desktop_runtime_game must not mix PACKAGE_FILES_FROM_MANIFEST with literal PACKAGE_FILES"
        }
        if ($manifestPackageFiles.Count -eq 0) {
            Write-Error "$relativePath MK_add_desktop_runtime_game uses PACKAGE_FILES_FROM_MANIFEST but runtimePackageFiles is empty or missing"
        }
        return
    }

    foreach ($packageFile in $manifestPackageFiles) {
        if (-not ($registeredPackageFiles -contains $packageFile)) {
            Write-Error "$relativePath runtimePackageFiles entry '$packageFile' is not declared in MK_add_desktop_runtime_game PACKAGE_FILES"
        }
    }
    foreach ($packageFile in $registeredPackageFiles) {
        if (-not ($manifestPackageFiles -contains $packageFile)) {
            Write-Error "$relativePath MK_add_desktop_runtime_game PACKAGE_FILES entry '$packageFile' is not declared in runtimePackageFiles"
        }
    }
}

function Assert-DesktopRuntimePackageRecipe($game, [string]$relativePath) {
    $commands = @($game.validationRecipes | ForEach-Object { [string]$_.command })
    $target = [string]$game.target
    $targetPattern = "(?:^|\s)-GameTarget\s+$([System.Text.RegularExpressions.Regex]::Escape($target))(?:\s|$)"
    $hasRecipe = $false

    foreach ($command in $commands) {
        $trimmed = $command.Trim()
        if ($target -eq "sample_desktop_runtime_shell" -and $trimmed -eq "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1") {
            $hasRecipe = $true
        }
        if ($trimmed -match "tools[\\/]+package-desktop-runtime\.ps1(?:\s|$)" -and $trimmed -match $targetPattern) {
            $hasRecipe = $true
        }
    }

    if (-not $hasRecipe) {
        Write-Error "$relativePath declares desktop-runtime-release but does not include a package validation recipe for target '$target'"
    }
}

$engine = Read-Json "engine/agent/manifest.json"
$gameAgentSchema = Read-Json "schemas/game-agent.schema.json"
Assert-GameSourceDirectoryLayout
if (-not $gameAgentSchema.properties.PSObject.Properties.Name.Contains("runtimeSceneValidationTargets")) {
    Write-Error "schemas/game-agent.schema.json must define runtimeSceneValidationTargets"
}
if (-not $gameAgentSchema.properties.PSObject.Properties.Name.Contains("materialShaderAuthoringTargets")) {
    Write-Error "schemas/game-agent.schema.json must define materialShaderAuthoringTargets"
}
if (-not $gameAgentSchema.properties.PSObject.Properties.Name.Contains("packageStreamingResidencyTargets")) {
    Write-Error "schemas/game-agent.schema.json must define packageStreamingResidencyTargets"
}
if (-not $gameAgentSchema.properties.PSObject.Properties.Name.Contains("atlasTilemapAuthoringTargets")) {
    Write-Error "schemas/game-agent.schema.json must define atlasTilemapAuthoringTargets"
}
if (-not $gameAgentSchema.properties.PSObject.Properties.Name.Contains("prefabScenePackageAuthoringTargets")) {
    Write-Error "schemas/game-agent.schema.json must define prefabScenePackageAuthoringTargets"
}
if (-not $gameAgentSchema.properties.PSObject.Properties.Name.Contains("registeredSourceAssetCookTargets")) {
    Write-Error "schemas/game-agent.schema.json must define registeredSourceAssetCookTargets"
}
Assert-Properties $engine @("schemaVersion", "engine", "commands", "modules", "runtimeBackendReadiness", "importerCapabilities", "packagingTargets", "validationRecipes", "aiOperableProductionLoop", "aiSurfaces", "documentationPolicy", "sourcePolicy", "gameCodeGuidance", "aiDrivenGameWorkflow") "engine manifest"
$geNavigationModule = @($engine.modules | Where-Object { $_.name -eq "MK_navigation" })
if ($geNavigationModule.Count -ne 1) {
    Write-Error "engine manifest must declare exactly one MK_navigation module"
}
if ($geNavigationModule[0].status -ne "implemented-production-path-planner") {
    Write-Error "engine manifest MK_navigation status must be implemented-production-path-planner"
}
foreach ($header in @(
    "engine/navigation/include/mirakana/navigation/navigation_path_planner.hpp",
    "engine/navigation/include/mirakana/navigation/navigation_replan.hpp",
    "engine/navigation/include/mirakana/navigation/path_smoothing.hpp",
    "engine/navigation/include/mirakana/navigation/local_avoidance.hpp"
)) {
    if (@($geNavigationModule[0].publicHeaders) -notcontains $header) {
        Write-Error "engine manifest MK_navigation publicHeaders missing $header"
    }
}
foreach ($needle in @(
    "NavigationGridAgentPathRequest",
    "NavigationGridAgentPathPlan",
    "NavigationGridAgentPathStatus",
    "NavigationGridAgentPathDiagnostic",
    "plan_navigation_grid_agent_path",
    "navmesh",
    "crowd",
    "scene/physics integration",
    "editor visualization"
)) {
    if (-not ([string]$geNavigationModule[0].purpose).Contains($needle)) {
        Write-Error "engine manifest MK_navigation purpose missing $needle"
    }
}
if (-not $engine.gameCodeGuidance.PSObject.Properties.Name.Contains("currentNavigation")) {
    Write-Error "engine manifest gameCodeGuidance must declare currentNavigation"
}
foreach ($needle in @("NavigationGridAgentPathRequest", "NavigationGridAgentPathPlan", "plan_navigation_grid_agent_path", "navmesh", "crowd", "scene/physics integration", "editor visualization")) {
    if (-not ([string]$engine.gameCodeGuidance.currentNavigation).Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentNavigation missing $needle"
    }
}
Assert-Properties $engine.gameCodeGuidance @("currentEditorContentBrowserImportDiagnostics") "engine manifest gameCodeGuidance"
Assert-Properties $engine.gameCodeGuidance @("currentEditorPrefabInstanceSourceLinks") "engine manifest gameCodeGuidance"
Assert-Properties $engine.gameCodeGuidance @("currentEditorInProcessRuntimeHost") "engine manifest gameCodeGuidance"
Assert-Properties $engine.gameCodeGuidance @("currentEditorGameModuleDriverLoad") "engine manifest gameCodeGuidance"
Assert-Properties $engine.gameCodeGuidance @("currentEditorRuntimeScenePackageValidationExecution") "engine manifest gameCodeGuidance"
foreach ($needle in @(
    "EditorInProcessRuntimeHostDesc",
    "EditorInProcessRuntimeHostModel",
    "EditorInProcessRuntimeHostBeginResult",
    "make_editor_in_process_runtime_host_model",
    "begin_editor_in_process_runtime_host_session",
    "make_editor_in_process_runtime_host_ui_model",
    "play_in_editor.in_process_runtime_host",
    "Begin In-Process Runtime Host",
    "already-linked caller-supplied IEditorPlaySessionDriver",
    "Dynamic game-module driver loading",
    "currentEditorGameModuleDriverLoad",
    "DesktopGameRunner embedding",
    "renderer/RHI uploads or handles",
    "package streaming"
)) {
    if (-not ([string]$engine.gameCodeGuidance.currentEditorInProcessRuntimeHost).Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEditorInProcessRuntimeHost missing: $needle"
    }
}
foreach ($needle in @(
    "DynamicLibrary",
    "load_dynamic_library",
    "resolve_dynamic_library_symbol",
    "LoadLibraryExW",
    "LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR",
    "LOAD_LIBRARY_SEARCH_SYSTEM32",
    "EditorGameModuleDriverApi",
    "EditorGameModuleDriverContractMetadataModel",
    "EditorGameModuleDriverContractMetadataRow",
    "GameEngine.EditorGameModuleDriver.v1",
    "mirakana_create_editor_game_module_driver_v1",
    "EditorGameModuleDriverLoadDesc",
    "EditorGameModuleDriverReloadDesc",
    "EditorGameModuleDriverReloadModel",
    "EditorGameModuleDriverUnloadDesc",
    "EditorGameModuleDriverUnloadModel",
    "EditorGameModuleDriverCreateResult",
    "make_editor_game_module_driver_contract_metadata_model",
    "make_editor_game_module_driver_contract_metadata_ui_model",
    "make_editor_game_module_driver_ctest_probe_evidence_model",
    "make_editor_game_module_driver_ctest_probe_evidence_ui_model",
    "MK_editor_game_module_driver_probe",
    "MK_editor_game_module_driver_load_tests",
    "make_editor_game_module_driver_load_model",
    "make_editor_game_module_driver_reload_model",
    "make_editor_game_module_driver_from_symbol",
    "make_editor_game_module_driver_load_ui_model",
    "make_editor_game_module_driver_reload_ui_model",
    "make_editor_game_module_driver_unload_model",
    "make_editor_game_module_driver_unload_ui_model",
    "EditorGameModuleDriverHostSessionPhase",
    "EditorGameModuleDriverHostSessionSnapshot",
    "make_editor_game_module_driver_host_session_snapshot",
    "make_editor_game_module_driver_host_session_ui_model",
    "ge.editor.editor_game_module_driver_host_session.v1",
    "play_in_editor.game_module_driver",
    "play_in_editor.game_module_driver.reload",
    "play_in_editor.game_module_driver.contract",
    "play_in_editor.game_module_driver.unload",
    "play_in_editor.game_module_driver.session",
    "policy_dll_mutation_order_guidance",
    "phase_idle_no_driver_order_review_load_then_load_library",
    "play_in_editor.game_module_driver.ctest_probe_evidence",
    "Game Module Driver",
    "Load Game Module Driver",
    "Reload Game Module Driver",
    "Unload Game Module Driver",
    "stopped-state reload",
    "Active-session hot reload",
    "stable third-party ABI",
    "package streaming"
)) {
    if (-not ([string]$engine.gameCodeGuidance.currentEditorGameModuleDriverLoad).Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEditorGameModuleDriverLoad missing: $needle"
    }
}
foreach ($needle in @(
    "EditorRuntimeScenePackageValidationExecutionDesc",
    "EditorRuntimeScenePackageValidationExecutionModel",
    "EditorRuntimeScenePackageValidationExecutionResult",
    "make_editor_runtime_scene_package_validation_execution_model",
    "execute_editor_runtime_scene_package_validation",
    "make_editor_runtime_scene_package_validation_execution_ui_model",
    "playtest_package_review.runtime_scene_validation",
    "Validate Runtime Scene Package",
    "RootedFileSystem",
    "Package cooking",
    "validation recipe execution",
    "package streaming"
)) {
    if (-not ([string]$engine.gameCodeGuidance.currentEditorRuntimeScenePackageValidationExecution).Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEditorRuntimeScenePackageValidationExecution missing: $needle"
    }
}
foreach ($needle in @(
    "Editor Scene Prefab Instance Refresh Review v1",
    "Editor Prefab Instance Local Child Refresh Resolution v1",
    "Editor Prefab Instance Stale Node Refresh Resolution v1",
    "Editor Nested Prefab Refresh Resolution v1",
    "ScenePrefabInstanceRefreshPlan",
    "ScenePrefabInstanceRefreshRow",
    "ScenePrefabInstanceRefreshResult",
    "ScenePrefabInstanceRefreshPolicy",
    "plan_scene_prefab_instance_refresh",
    "apply_scene_prefab_instance_refresh",
    "make_scene_prefab_instance_refresh_action",
    "make_scene_prefab_instance_refresh_ui_model",
    "scene_prefab_instance_refresh",
    "keep_local_child",
    "keep_local_children",
    "keep_stale_source_nodes_as_local",
    "keep_stale_source_node_as_local",
    "keep_nested_prefab_instances",
    "keep_nested_prefab_instance",
    "unsupported_nested_prefab_instance",
    "Refresh Prefab Instance",
    "Keep Local Children",
    "Keep Stale Source Nodes",
    "Keep Nested Prefab Instances",
    "preserves existing scene node state",
    "unsupported local children",
    "automatic nested prefab refresh"
)) {
    if (-not ([string]$engine.gameCodeGuidance.currentEditorPrefabInstanceSourceLinks).Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEditorPrefabInstanceSourceLinks missing: $needle"
    }
}
foreach ($needle in @(
    "Editor Content Browser Import Codec Adapter Review v1",
    "EditorContentBrowserImportExternalSourceCopyModel",
    "make_content_browser_import_external_source_copy_model",
    "make_content_browser_import_external_source_copy_ui_model",
    "content_browser_import.external_copy",
    "Copy External Sources",
    ".png",
    ".gltf",
    ".glb",
    ".wav",
    ".mp3",
    ".flac",
    "ExternalAssetImportAdapters",
    "arbitrary importer adapters",
    "arbitrary file import",
    "broad editor/importer readiness"
)) {
    if (-not ([string]$engine.gameCodeGuidance.currentEditorContentBrowserImportDiagnostics).Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEditorContentBrowserImportDiagnostics missing: $needle"
    }
}
Assert-Properties $engine.commands @("validate", "toolchainCheck", "directToolchainCheck", "bootstrapDeps", "build", "buildGui", "buildAssetImporters", "test", "dependencyCheck", "cppStandardCheck", "evaluateCpp23", "shaderToolchainCheck", "agentContext", "agentCheck", "newGame", "ciMatrixCheck") "engine manifest commands"
if ($engine.commands.ciMatrixCheck -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1") {
    Write-Error "engine manifest commands.ciMatrixCheck must expose pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1"
}
if (-not $engine.commands.newGame.Contains("DesktopRuntime2DPackage")) {
    Write-Error "engine manifest commands.newGame must expose DesktopRuntime2DPackage"
}
if (-not $engine.commands.newGame.Contains("DesktopRuntime3DPackage")) {
    Write-Error "engine manifest commands.newGame must expose DesktopRuntime3DPackage"
}
Assert-Properties $engine.documentationPolicy @("preferredMcp", "useFor", "secretStorage", "doNotStoreApiKeysInRepo") "engine manifest documentationPolicy"
Assert-Properties $engine.documentationPolicy @("entrypoints") "engine manifest documentationPolicy"
Assert-Properties $engine.documentationPolicy.entrypoints @("entrypoint", "currentStatus", "currentCapabilities", "workflows", "planRegistry", "specRegistry", "superpowersSpecRegistry", "activeRoadmap") "engine manifest documentationPolicy.entrypoints"
foreach ($docEntrypoint in @(
    $engine.documentationPolicy.entrypoints.entrypoint,
    $engine.documentationPolicy.entrypoints.currentStatus,
    $engine.documentationPolicy.entrypoints.currentCapabilities,
    $engine.documentationPolicy.entrypoints.workflows,
    $engine.documentationPolicy.entrypoints.planRegistry,
    $engine.documentationPolicy.entrypoints.specRegistry,
    $engine.documentationPolicy.entrypoints.superpowersSpecRegistry,
    $engine.documentationPolicy.entrypoints.activeRoadmap
)) {
    if (-not (Test-Path (Join-Path $root $docEntrypoint))) {
        Write-Error "engine manifest documentationPolicy.entrypoints references missing document: $docEntrypoint"
    }
}
Assert-Properties $engine.sourcePolicy @("vcpkgManifest", "vcpkgBaseline", "vcpkgBootstrapCommand", "vcpkgConfigurePolicy", "dependencyPolicy") "engine manifest sourcePolicy"
Assert-Properties $engine.runtimeBackendReadiness @("graphics", "audio", "ui", "physics", "platform") "engine manifest runtimeBackendReadiness"
Assert-Properties $engine.importerCapabilities @("runtimePolicy", "defaultSourceDocuments", "cookedArtifacts", "optionalDependencyFeature", "plannedExternalImporters") "engine manifest importerCapabilities"
if ($engine.commands.bootstrapDeps -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1" -or $engine.sourcePolicy.vcpkgBootstrapCommand -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1") {
    Write-Error "engine manifest must expose pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1 as the vcpkg bootstrap command"
}
if (-not ([string]$engine.sourcePolicy.vcpkgConfigurePolicy).Contains("VCPKG_MANIFEST_INSTALL=OFF")) {
    Write-Error "engine manifest sourcePolicy.vcpkgConfigurePolicy must keep vcpkg install disabled during CMake configure"
}
if ($engine.engine.language -ne "C++23") {
    Write-Error "engine manifest must declare C++23"
}
Assert-Properties $engine.engine @("stageStatus", "stageCompletion", "stageClosurePlan", "stageClosureNotes") "engine manifest engine"
if ($engine.engine.stage -ne "core-first-mvp") {
    Write-Error "engine manifest stage must remain core-first-mvp for the closed MVP foundation"
}
if ($engine.engine.stageStatus -ne "mvp-closed-not-commercial-complete") {
    Write-Error "engine manifest stageStatus must be mvp-closed-not-commercial-complete"
}
if ([string]::IsNullOrWhiteSpace($engine.engine.stageCompletion) -or
    -not ([string]$engine.engine.stageCompletion).Contains("not a commercial-engine completion claim")) {
    Write-Error "engine manifest stageCompletion must distinguish MVP closure from commercial engine completion"
}
if (-not (Test-Path (Join-Path $root $engine.engine.stageClosurePlan))) {
    Write-Error "engine manifest stageClosurePlan references missing document: $($engine.engine.stageClosurePlan)"
}
if ($null -eq $engine.engine.stageClosureNotes -or $engine.engine.stageClosureNotes.Count -lt 1) {
    Write-Error "engine manifest stageClosureNotes must list closure caveats"
}
if (-not ((@($engine.engine.stageClosureNotes) -join " ").Contains("Apple/iOS/Metal"))) {
    Write-Error "engine manifest stageClosureNotes must keep Apple/iOS/Metal host gating explicit"
}
if ($engine.documentationPolicy.preferredMcp -ne "context7") {
    Write-Error "engine manifest documentationPolicy.preferredMcp must be context7"
}
if (-not $engine.documentationPolicy.doNotStoreApiKeysInRepo) {
    Write-Error "engine manifest documentationPolicy must forbid storing API keys in the repository"
}

$vulkanBackendSource = Get-Content -LiteralPath (Join-Path $root "engine/rhi/vulkan/src/vulkan_backend.cpp") -Raw
if (-not $vulkanBackendSource.Contains("refresh_surface_probe_queue_family_snapshots") -or
    -not $vulkanBackendSource.Contains("same native instance handles")) {
    Write-Error "Vulkan surface support probe must refresh queue-family snapshots from the same native instance handles before vkGetPhysicalDeviceSurfaceSupportKHR"
}
$rhiPostprocessSource = Get-Content -LiteralPath (Join-Path $root "engine/renderer/src/rhi_postprocess_frame_renderer.cpp") -Raw
$rhiDirectionalShadowSource = Get-Content -LiteralPath (Join-Path $root "engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp") -Raw
if ($rhiPostprocessSource.Contains("void RhiPostprocessFrameRenderer::draw_sprite(const SpriteCommand&) {`r`n    require_active_frame();`r`n    commands_->draw(3, 1);") -or
    $rhiPostprocessSource.Contains("void RhiPostprocessFrameRenderer::draw_sprite(const SpriteCommand&) {`n    require_active_frame();`n    commands_->draw(3, 1);") -or
    $rhiDirectionalShadowSource.Contains("pending_sprites_")) {
    Write-Error "3D scene/postprocess renderers must not draw HUD sprites through the scene material pipeline"
}

$moduleNames = @{}
foreach ($module in $engine.modules) {
    Assert-Properties $module @("name", "path", "status", "dependencies", "publicHeaders", "purpose") "engine module"
    $moduleNames[$module.name] = $true
    foreach ($header in $module.publicHeaders) {
        if (-not (Test-Path (Join-Path $root $header))) {
            Write-Error "module '$($module.name)' references missing public header: $header"
        }
    }
}

$geSceneModule = @($engine.modules | Where-Object { $_.name -eq "MK_scene" })
if ($geSceneModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_scene module"
}
$geAssetsModule = @($engine.modules | Where-Object { $_.name -eq "MK_assets" })
if ($geAssetsModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_assets module"
}
$geRuntimeModule = @($engine.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($geRuntimeModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_runtime module"
}
$geAudioModule = @($engine.modules | Where-Object { $_.name -eq "MK_audio" })
if ($geAudioModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_audio module"
}
$gePhysicsModule = @($engine.modules | Where-Object { $_.name -eq "MK_physics" })
if ($gePhysicsModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_physics module"
}
$geToolsModule = @($engine.modules | Where-Object { $_.name -eq "MK_tools" })
if ($geToolsModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_tools module"
}
if ($geSceneModule[0].status -ne "implemented-scene-schema-v2-contract") {
    Write-Error "engine manifest MK_scene status must advertise the Scene/Component/Prefab Schema v2 contract slice honestly"
}
if (@($geSceneModule[0].publicHeaders) -notcontains "engine/scene/include/mirakana/scene/schema_v2.hpp") {
    Write-Error "engine manifest MK_scene publicHeaders must include schema_v2.hpp"
}
if (-not ([string]$geSceneModule[0].purpose).Contains("contract-only") -or
    -not ([string]$geSceneModule[0].purpose).Contains("GameEngine.Scene.v2") -or
    -not ([string]$geSceneModule[0].purpose).Contains("nested prefab propagation/merge resolution UX")) {
    Write-Error "engine manifest MK_scene purpose must describe Schema v2 as contract-only and keep follow-up limits explicit"
}
if ($geAssetsModule[0].status -ne "implemented-asset-identity-v2-foundation") {
    Write-Error "engine manifest MK_assets status must advertise the Asset Identity v2 foundation slice honestly"
}
if (@($geAssetsModule[0].publicHeaders) -notcontains "engine/assets/include/mirakana/assets/asset_identity.hpp") {
    Write-Error "engine manifest MK_assets publicHeaders must include asset_identity.hpp"
}
if (-not ([string]$geAssetsModule[0].purpose).Contains("Asset Identity v2") -or
    -not ([string]$geAssetsModule[0].purpose).Contains("GameEngine.AssetIdentity.v2") -or
    -not ([string]$geAssetsModule[0].purpose).Contains("foundation-only") -or
    -not ([string]$geAssetsModule[0].purpose).Contains("renderer/RHI residency")) {
    Write-Error "engine manifest MK_assets purpose must describe Asset Identity v2 as foundation-only and keep follow-up limits explicit"
}
if ($geRuntimeModule[0].status -ne "implemented-runtime-resource-v2-foundation") {
    Write-Error "engine manifest MK_runtime status must advertise the Runtime Resource v2 foundation slice honestly"
}
if (@($geRuntimeModule[0].publicHeaders) -notcontains "engine/runtime/include/mirakana/runtime/resource_runtime.hpp") {
    Write-Error "engine manifest MK_runtime publicHeaders must include resource_runtime.hpp"
}
if ($geAudioModule[0].status -ne "implemented-device-streaming-baseline") {
    Write-Error "engine manifest MK_audio status must advertise the audio device streaming baseline honestly"
}
if (-not ([string]$geAudioModule[0].purpose).Contains("AudioDeviceStreamRequest") -or
    -not ([string]$geAudioModule[0].purpose).Contains("AudioDeviceStreamPlan") -or
    -not ([string]$geAudioModule[0].purpose).Contains("plan_audio_device_stream") -or
    -not ([string]$geAudioModule[0].purpose).Contains("render_audio_device_stream_interleaved_float") -or
    -not ([string]$geAudioModule[0].purpose).Contains("does not open OS audio devices")) {
    Write-Error "engine manifest MK_audio purpose must describe the audio device stream planning APIs and OS-device boundary"
}
if ($gePhysicsModule[0].status -ne "implemented-physics-1-0-ready-surface") {
    Write-Error "engine manifest MK_physics status must advertise the Physics 1.0 ready surface honestly"
}
if (-not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsCharacterController3DDesc") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("move_physics_character_controller_3d") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsAuthoredCollisionScene3DDesc") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("build_physics_world_3d_from_authored_collision_scene") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsShape3DDesc::aabb") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsShape3DDesc::sphere") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsShape3DDesc::capsule") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsQueryFilter3D") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsWorld3D::exact_shape_sweep") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsExactSphereCast3DDesc") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsWorld3D::exact_sphere_cast") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsContactPoint3D") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsContactManifold3D") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsWorld3D::contact_manifolds") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("warm-start-safe") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsContinuousStep3DConfig") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsContinuousStep3DRow") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsContinuousStep3DResult") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsWorld3D::step_continuous") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsCharacterDynamicPolicy3DDesc") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsCharacterDynamicPolicy3DRowKind") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsCharacterDynamicPolicy3DRow") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsCharacterDynamicPolicy3DResult") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("evaluate_physics_character_dynamic_policy_3d") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsJoint3DStatus") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsDistanceJoint3DDesc") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsJointSolve3DResult") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("solve_physics_joints_3d") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsDeterminismGate3DStatus") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsDeterminismGate3DDiagnostic") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsDeterminismGate3DConfig") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsDeterminismGate3DCounts") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsReplaySignature3D") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsDeterminismGate3DResult") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("make_physics_replay_signature_3d") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("evaluate_physics_determinism_gate_3d") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("validated Physics 1.0 ready surface") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("explicit Jolt/native middleware exclusion") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsWorld3D::step remains discrete")) {
    Write-Error "engine manifest MK_physics purpose must describe exact shape sweeps, contact manifold stability, CCD foundation, character/dynamic policy, joints foundation, and benchmark determinism gates honestly"
}
if (-not ([string]$geRuntimeModule[0].purpose).Contains("Runtime Resource v2") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("generation-checked") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("commit_runtime_resident_package_replace_v2") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("commit_runtime_resident_package_unmount_v2") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("foundation-only") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("package streaming") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("renderer/RHI resource ownership")) {
    Write-Error "engine manifest MK_runtime purpose must describe Runtime Resource v2 as foundation-only and keep follow-up limits explicit"
}
if (-not ([string]$geRuntimeModule[0].purpose).Contains("RuntimeInputRebindingPresentationModel") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("make_runtime_input_rebinding_presentation") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("symbolic glyph lookup keys") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("platform input glyph generation")) {
    Write-Error "engine manifest MK_runtime purpose must describe runtime input rebinding presentation rows and platform glyph follow-up boundary"
}
if (-not ([string]$geToolsModule[0].purpose).Contains("PngImageDecodingAdapter") -or
    -not ([string]$geToolsModule[0].purpose).Contains("IImageDecodingAdapter") -or
    -not ([string]$geToolsModule[0].purpose).Contains("decode_audited_png_rgba8")) {
    Write-Error "engine manifest MK_tools purpose must describe Runtime UI PNG image decoding adapter boundary explicitly"
}
if (-not ([string]$geToolsModule[0].purpose).Contains("PackedUiAtlasAuthoringDesc") -or
    -not ([string]$geToolsModule[0].purpose).Contains("author_packed_ui_atlas_from_decoded_images") -or
    -not ([string]$geToolsModule[0].purpose).Contains("plan_packed_ui_atlas_package_update") -or
    -not ([string]$geToolsModule[0].purpose).Contains("GameEngine.CookedTexture.v1") -or
    -not ([string]$geToolsModule[0].purpose).Contains("GameEngine.UiAtlas.v1")) {
    Write-Error "engine manifest MK_tools purpose must describe Runtime UI decoded image atlas package bridge explicitly"
}
if (-not ([string]$geToolsModule[0].purpose).Contains("PackedUiGlyphAtlasAuthoringDesc") -or
    -not ([string]$geToolsModule[0].purpose).Contains("author_packed_ui_glyph_atlas_from_rasterized_glyphs") -or
    -not ([string]$geToolsModule[0].purpose).Contains("plan_packed_ui_glyph_atlas_package_update") -or
    -not ([string]$geToolsModule[0].purpose).Contains("UiAtlasMetadataGlyph") -or
    -not ([string]$geToolsModule[0].purpose).Contains("GameEngine.CookedTexture.v1") -or
    -not ([string]$geToolsModule[0].purpose).Contains("GameEngine.UiAtlas.v1")) {
    Write-Error "engine manifest MK_tools purpose must describe Runtime UI glyph atlas package bridge explicitly"
}

$runtimeCategories = @("graphics", "audio", "ui", "physics", "platform")
foreach ($category in $runtimeCategories) {
    foreach ($backend in $engine.runtimeBackendReadiness.$category) {
        Assert-Properties $backend @("name", "module", "status", "hosts", "validation", "notes") "runtime backend readiness '$category'"
        if (-not $moduleNames.ContainsKey($backend.module)) {
            Write-Error "runtime backend readiness '$category/$($backend.name)' references unknown module: $($backend.module)"
        }
        if ($backend.hosts.Count -lt 1) {
            Write-Error "runtime backend readiness '$category/$($backend.name)' must declare at least one host"
        }
    }
}

if ($engine.importerCapabilities.optionalDependencyFeature -ne "asset-importers") {
    Write-Error "engine manifest importerCapabilities.optionalDependencyFeature must be asset-importers"
}
foreach ($importer in $engine.importerCapabilities.plannedExternalImporters) {
    Assert-Properties $importer @("format", "dependency", "status", "ownerModule") "engine manifest plannedExternalImporters"
    if (-not $moduleNames.ContainsKey($importer.ownerModule)) {
        Write-Error "planned external importer '$($importer.format)' references unknown owner module: $($importer.ownerModule)"
    }
}

$packagingTargetNames = @{}
foreach ($target in $engine.packagingTargets) {
    Assert-Properties $target @("name", "platform", "status", "command", "notes") "engine manifest packagingTargets"
    $packagingTargetNames[$target.name] = $true
}

foreach ($recipe in $engine.validationRecipes) {
    Assert-Properties $recipe @("name", "command", "purpose") "engine manifest validationRecipes"
    if ([string]::IsNullOrWhiteSpace($recipe.command)) {
        Write-Error "engine manifest validation recipe '$($recipe.name)' must declare a command"
    }
}
$validationRecipeNames = @{}
foreach ($recipe in $engine.validationRecipes) {
    if ($validationRecipeNames.ContainsKey($recipe.name)) {
        Write-Error "engine manifest validationRecipes has duplicate recipe name: $($recipe.name)"
    }
    $validationRecipeNames[$recipe.name] = $true
}

$productionLoop = $engine.aiOperableProductionLoop
Assert-Properties $productionLoop @("schemaVersion", "design", "foundationPlan", "currentActivePlan", "recommendedNextPlan", "recipeStatusEnum", "recipes", "commandSurfaces", "authoringSurfaces", "packageSurfaces", "physicsBackendAdapterDecisions", "unsupportedProductionGaps", "hostGates", "validationRecipeMap", "reviewLoops", "resourceExecutionLoops", "materialShaderAuthoringLoops", "atlasTilemapAuthoringLoops", "packageStreamingResidencyLoops", "safePointPackageReplacementLoops") "engine manifest aiOperableProductionLoop"
if ($productionLoop.schemaVersion -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop.schemaVersion must be 1"
}
foreach ($productionLoopDoc in @($productionLoop.design, $productionLoop.foundationPlan, $productionLoop.currentActivePlan)) {
    if (-not (Test-Path (Join-Path $root $productionLoopDoc))) {
        Write-Error "engine manifest aiOperableProductionLoop references missing document: $productionLoopDoc"
    }
}
if ($productionLoop.recommendedNextPlan.PSObject.Properties.Name.Contains("path") -and
    -not (Test-Path (Join-Path $root $productionLoop.recommendedNextPlan.path))) {
    Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan references missing document: $($productionLoop.recommendedNextPlan.path)"
}
Assert-ActiveProductionPlanDrift $productionLoop
$physicsBackendAdapterDecision = @($productionLoop.physicsBackendAdapterDecisions | Where-Object { $_.id -eq "physics-1-0-jolt-native-adapter" })
if ($physicsBackendAdapterDecision.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must record one physics-1-0-jolt-native-adapter decision"
}
Assert-Properties $physicsBackendAdapterDecision[0] @("id", "status", "decision", "futureGate", "unsupportedClaims") "engine manifest aiOperableProductionLoop physicsBackendAdapterDecisions"
if ($physicsBackendAdapterDecision[0].status -ne "excluded-from-1-0-ready-surface") {
    Write-Error "engine manifest aiOperableProductionLoop physicsBackendAdapterDecisions status must be excluded-from-1-0-ready-surface"
}
foreach ($needle in @("first-party MK_physics only", "vcpkg manifest feature", "fail-closed capability negotiation")) {
    if (-not ((([string]$physicsBackendAdapterDecision[0].decision), ([string]$physicsBackendAdapterDecision[0].futureGate)) -join " ").Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop physicsBackendAdapterDecisions missing decision/futureGate text: $needle"
    }
}
foreach ($claim in @("Jolt runtime integration", "native physics handles in public gameplay APIs", "middleware type exposure")) {
    if (@($physicsBackendAdapterDecision[0].unsupportedClaims) -notcontains $claim) {
        Write-Error "engine manifest aiOperableProductionLoop physicsBackendAdapterDecisions unsupportedClaims missing: $claim"
    }
}
$allowedProductionStatuses = @("ready", "host-gated", "planned", "blocked")
foreach ($status in @("ready", "host-gated", "planned", "blocked")) {
    if (@($productionLoop.recipeStatusEnum) -notcontains $status) {
        Write-Error "engine manifest aiOperableProductionLoop.recipeStatusEnum missing status: $status"
    }
}
$editorPlaytestReviewLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-playtest-package-review-loop" })
if ($editorPlaytestReviewLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-playtest-package-review-loop review loop"
}
if ($editorPlaytestReviewLoop.Count -eq 1) {
    Assert-Properties $editorPlaytestReviewLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "preSmokeGate", "hostGatedSmokeRecipes", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-playtest-package-review-loop"
    if ($editorPlaytestReviewLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-playtest-package-review-loop must be ready inside its narrow review scope"
    }
    $expectedEditorReviewSteps = @(
        "review-editor-package-candidates",
        "apply-reviewed-runtime-package-files",
        "select-runtime-scene-validation-target",
        "validate-runtime-scene-package",
        "run-host-gated-desktop-smoke"
    )
    $actualEditorReviewSteps = @($editorPlaytestReviewLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest editor-playtest-package-review-loop ordered step"
        $_.id
    })
    if (($actualEditorReviewSteps -join "|") -ne ($expectedEditorReviewSteps -join "|")) {
        Write-Error "engine manifest editor-playtest-package-review-loop orderedSteps must be exactly: $($expectedEditorReviewSteps -join ', ')"
    }
    foreach ($step in @($editorPlaytestReviewLoop[0].orderedSteps)) {
        if ($step.status -ne "ready" -and $step.status -ne "host-gated") {
            Write-Error "engine manifest editor-playtest-package-review-loop step '$($step.id)' has invalid status: $($step.status)"
        }
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "validationRecipes")) {
        if (@($editorPlaytestReviewLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-playtest-package-review-loop requiredManifestFields missing: $field"
        }
    }
    if ($editorPlaytestReviewLoop[0].preSmokeGate -ne "validate-runtime-scene-package") {
        Write-Error "engine manifest editor-playtest-package-review-loop preSmokeGate must be validate-runtime-scene-package"
    }
    foreach ($recipe in @("desktop-game-runtime", "desktop-runtime-release-target", "installed-d3d12-3d-package-smoke", "installed-d3d12-3d-directional-shadow-smoke", "installed-d3d12-3d-shadow-morph-composition-smoke", "desktop-runtime-release-target-vulkan-toolchain-gated", "desktop-runtime-release-target-vulkan-directional-shadow-toolchain-gated")) {
        if (@($editorPlaytestReviewLoop[0].hostGatedSmokeRecipes) -notcontains $recipe) {
            Write-Error "engine manifest editor-playtest-package-review-loop hostGatedSmokeRecipes missing: $recipe"
        }
    }
    foreach ($claim in @("broad package cooking", "runtime source parsing", "renderer/RHI residency", "package streaming", "editor productization", "native handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorPlaytestReviewLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-playtest-package-review-loop unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPackageDiagnosticsLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-package-authoring-diagnostics" })
if ($editorAiPackageDiagnosticsLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-package-authoring-diagnostics review loop"
}
if ($editorAiPackageDiagnosticsLoop.Count -eq 1) {
    Assert-Properties $editorAiPackageDiagnosticsLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "diagnosticInputs", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-package-authoring-diagnostics"
    if ($editorAiPackageDiagnosticsLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-package-authoring-diagnostics must be ready as diagnostics-only editor-core model"
    }
    $expectedEditorAiDiagnosticsSteps = @(
        "collect-editor-package-candidate-diagnostics",
        "summarize-manifest-descriptor-rows",
        "inspect-runtime-package-payload-diagnostics",
        "summarize-validation-recipe-status",
        "report-host-gated-desktop-smoke-preflight"
    )
    $actualEditorAiDiagnosticsSteps = @($editorAiPackageDiagnosticsLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-package-authoring-diagnostics ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-package-authoring-diagnostics step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-package-authoring-diagnostics step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiDiagnosticsSteps -join "|") -ne ($expectedEditorAiDiagnosticsSteps -join "|")) {
        Write-Error "engine manifest editor-ai-package-authoring-diagnostics orderedSteps must be exactly: $($expectedEditorAiDiagnosticsSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPackageDiagnosticsLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-package-authoring-diagnostics requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("package candidate rows", "descriptor summary rows", "runtime package payload diagnostics", "validation recipe status")) {
        if (-not ((@($editorAiPackageDiagnosticsLoop[0].diagnosticInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-package-authoring-diagnostics diagnosticInputs missing: $requiredInput"
        }
    }
    foreach ($claim in @("arbitrary shell", "free-form manifest edits", "broad package cooking", "runtime source parsing", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPackageDiagnosticsLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-package-authoring-diagnostics blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPackageDiagnosticsLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-package-authoring-diagnostics unsupportedClaims missing: $claim"
        }
    }
}
$editorAiValidationRecipePreflightLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-validation-recipe-preflight" })
if ($editorAiValidationRecipePreflightLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-validation-recipe-preflight review loop"
}
if ($editorAiValidationRecipePreflightLoop.Count -eq 1) {
    Assert-Properties $editorAiValidationRecipePreflightLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "preflightInputs", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-validation-recipe-preflight"
    if ($editorAiValidationRecipePreflightLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-validation-recipe-preflight must be ready as preflight-only editor-core model"
    }
    $expectedEditorAiValidationPreflightSteps = @(
        "collect-manifest-validation-recipes",
        "collect-reviewed-dry-run-plans",
        "map-selected-recipes-to-host-gates",
        "report-blocked-or-unsupported-validation-claims"
    )
    $actualEditorAiValidationPreflightSteps = @($editorAiValidationRecipePreflightLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-validation-recipe-preflight ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-validation-recipe-preflight step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-validation-recipe-preflight step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiValidationPreflightSteps -join "|") -ne ($expectedEditorAiValidationPreflightSteps -join "|")) {
        Write-Error "engine manifest editor-ai-validation-recipe-preflight orderedSteps must be exactly: $($expectedEditorAiValidationPreflightSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiValidationRecipePreflightLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-validation-recipe-preflight requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("manifest validationRecipes", "run-validation-recipe dry-run plans", "host gates", "blocked reasons")) {
        if (-not ((@($editorAiValidationRecipePreflightLoop[0].preflightInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-validation-recipe-preflight preflightInputs missing: $requiredInput"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "package script execution", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiValidationRecipePreflightLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-validation-recipe-preflight blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiValidationRecipePreflightLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-validation-recipe-preflight unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPlaytestReadinessReportLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-readiness-report" })
if ($editorAiPlaytestReadinessReportLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-readiness-report review loop"
}
if ($editorAiPlaytestReadinessReportLoop.Count -eq 1) {
    Assert-Properties $editorAiPlaytestReadinessReportLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "reportInputs", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-playtest-readiness-report"
    if ($editorAiPlaytestReadinessReportLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-playtest-readiness-report must be ready as read-only editor-core model"
    }
    $expectedEditorAiReadinessSteps = @(
        "collect-package-authoring-diagnostics",
        "collect-validation-recipe-preflight",
        "aggregate-readiness-status",
        "report-readiness-blockers"
    )
    $actualEditorAiReadinessSteps = @($editorAiPlaytestReadinessReportLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-playtest-readiness-report ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-readiness-report step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-readiness-report step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiReadinessSteps -join "|") -ne ($expectedEditorAiReadinessSteps -join "|")) {
        Write-Error "engine manifest editor-ai-playtest-readiness-report orderedSteps must be exactly: $($expectedEditorAiReadinessSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestReadinessReportLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-playtest-readiness-report requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("EditorAiPackageAuthoringDiagnosticsModel", "EditorAiValidationRecipePreflightModel", "host gates", "blocking diagnostics")) {
        if (-not ((@($editorAiPlaytestReadinessReportLoop[0].reportInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-playtest-readiness-report reportInputs missing: $requiredInput"
        }
    }
    foreach ($claim in @("arbitrary shell", "free-form manifest edits", "validation execution", "package script execution", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestReadinessReportLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-readiness-report blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestReadinessReportLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-readiness-report unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPlaytestOperatorHandoffLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-operator-handoff" })
if ($editorAiPlaytestOperatorHandoffLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-operator-handoff review loop"
}
if ($editorAiPlaytestOperatorHandoffLoop.Count -eq 1) {
    Assert-Properties $editorAiPlaytestOperatorHandoffLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "handoffInputs", "commandFields", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-playtest-operator-handoff"
    if ($editorAiPlaytestOperatorHandoffLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-playtest-operator-handoff must be ready as read-only editor-core handoff model"
    }
    $expectedEditorAiOperatorHandoffSteps = @(
        "collect-readiness-report",
        "collect-validation-preflight-commands",
        "assemble-reviewed-operator-command-rows",
        "report-host-gates-blockers-and-unsupported-claims"
    )
    $actualEditorAiOperatorHandoffSteps = @($editorAiPlaytestOperatorHandoffLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-playtest-operator-handoff ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-operator-handoff step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-operator-handoff step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiOperatorHandoffSteps -join "|") -ne ($expectedEditorAiOperatorHandoffSteps -join "|")) {
        Write-Error "engine manifest editor-ai-playtest-operator-handoff orderedSteps must be exactly: $($expectedEditorAiOperatorHandoffSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestOperatorHandoffLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-playtest-operator-handoff requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("EditorAiPlaytestReadinessReportModel", "EditorAiValidationRecipePreflightModel", "run-validation-recipe dry-run command plan data", "host gates", "blocked reasons")) {
        if (-not ((@($editorAiPlaytestOperatorHandoffLoop[0].handoffInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-playtest-operator-handoff handoffInputs missing: $requiredInput"
        }
    }
    foreach ($field in @("recipe id", "reviewed command display", "argv plan data", "host gates", "blocked reasons", "readiness dependency")) {
        if (-not ((@($editorAiPlaytestOperatorHandoffLoop[0].commandFields) -join " ").Contains($field))) {
            Write-Error "engine manifest editor-ai-playtest-operator-handoff commandFields missing: $field"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "validation execution", "package script execution", "free-form manifest edits", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestOperatorHandoffLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-operator-handoff blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestOperatorHandoffLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-operator-handoff unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPlaytestEvidenceSummaryLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-evidence-summary" })
if ($editorAiPlaytestEvidenceSummaryLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-evidence-summary review loop"
}
if ($editorAiPlaytestEvidenceSummaryLoop.Count -eq 1) {
    Assert-Properties $editorAiPlaytestEvidenceSummaryLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "evidenceInputs", "evidenceFields", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-playtest-evidence-summary"
    if ($editorAiPlaytestEvidenceSummaryLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-playtest-evidence-summary must be ready as read-only editor-core evidence summary model"
    }
    $expectedEditorAiEvidenceSummarySteps = @(
        "collect-operator-handoff",
        "collect-external-validation-evidence",
        "summarize-evidence-status",
        "report-evidence-blockers-and-unsupported-claims"
    )
    $actualEditorAiEvidenceSummarySteps = @($editorAiPlaytestEvidenceSummaryLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-playtest-evidence-summary ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-evidence-summary step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-evidence-summary step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiEvidenceSummarySteps -join "|") -ne ($expectedEditorAiEvidenceSummarySteps -join "|")) {
        Write-Error "engine manifest editor-ai-playtest-evidence-summary orderedSteps must be exactly: $($expectedEditorAiEvidenceSummarySteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestEvidenceSummaryLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-playtest-evidence-summary requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("EditorAiPlaytestOperatorHandoffModel", "externally supplied run-validation-recipe execute results", "recipe status", "exit code or summary text", "host gates", "blocked reasons", "readiness dependency")) {
        if (-not ((@($editorAiPlaytestEvidenceSummaryLoop[0].evidenceInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-playtest-evidence-summary evidenceInputs missing: $requiredInput"
        }
    }
    foreach ($field in @("recipe id", "handoff row", "status passed failed blocked host-gated missing", "exit code", "summary text", "host gates", "blockers", "readiness dependency")) {
        if (-not ((@($editorAiPlaytestEvidenceSummaryLoop[0].evidenceFields) -join " ").Contains($field))) {
            Write-Error "engine manifest editor-ai-playtest-evidence-summary evidenceFields missing: $field"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "raw/free-form command evaluation", "validation execution", "package script execution", "free-form manifest edits", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestEvidenceSummaryLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-evidence-summary blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestEvidenceSummaryLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-evidence-summary unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPlaytestRemediationQueueLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-remediation-queue" })
if ($editorAiPlaytestRemediationQueueLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-remediation-queue review loop"
}
if ($editorAiPlaytestRemediationQueueLoop.Count -eq 1) {
    Assert-Properties $editorAiPlaytestRemediationQueueLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "queueInputs", "queueFields", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-playtest-remediation-queue"
    if ($editorAiPlaytestRemediationQueueLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-playtest-remediation-queue must be ready as read-only editor-core remediation queue model"
    }
    $expectedEditorAiRemediationQueueSteps = @(
        "collect-evidence-summary",
        "classify-remediation-rows",
        "prioritize-remediation-categories",
        "report-remediation-blockers-and-unsupported-claims"
    )
    $actualEditorAiRemediationQueueSteps = @($editorAiPlaytestRemediationQueueLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-playtest-remediation-queue ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-remediation-queue step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-remediation-queue step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiRemediationQueueSteps -join "|") -ne ($expectedEditorAiRemediationQueueSteps -join "|")) {
        Write-Error "engine manifest editor-ai-playtest-remediation-queue orderedSteps must be exactly: $($expectedEditorAiRemediationQueueSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestRemediationQueueLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-playtest-remediation-queue requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("EditorAiPlaytestEvidenceSummaryModel", "failed evidence", "blocked evidence", "missing evidence", "host-gated evidence", "host gates", "blocked reasons", "readiness dependency")) {
        if (-not ((@($editorAiPlaytestRemediationQueueLoop[0].queueInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-queue queueInputs missing: $requiredInput"
        }
    }
    foreach ($field in @("recipe id", "evidence status", "remediation category", "next-action text", "host gates", "blockers", "readiness dependency", "unsupported claims")) {
        if (-not ((@($editorAiPlaytestRemediationQueueLoop[0].queueFields) -join " ").Contains($field))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-queue queueFields missing: $field"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "raw/free-form command evaluation", "validation execution", "package script execution", "free-form manifest edits", "evidence mutation", "fix execution", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestRemediationQueueLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-queue blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestRemediationQueueLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-queue unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPlaytestRemediationHandoffLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-remediation-handoff" })
if ($editorAiPlaytestRemediationHandoffLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-remediation-handoff review loop"
}
if ($editorAiPlaytestRemediationHandoffLoop.Count -eq 1) {
    Assert-Properties $editorAiPlaytestRemediationHandoffLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "handoffInputs", "handoffFields", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-playtest-remediation-handoff"
    if ($editorAiPlaytestRemediationHandoffLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-playtest-remediation-handoff must be ready as read-only editor-core remediation handoff model"
    }
    $expectedEditorAiRemediationHandoffSteps = @(
        "collect-remediation-queue",
        "map-remediation-rows-to-external-actions",
        "assemble-external-operator-handoff-rows",
        "report-remediation-handoff-blockers-and-unsupported-claims"
    )
    $actualEditorAiRemediationHandoffSteps = @($editorAiPlaytestRemediationHandoffLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-playtest-remediation-handoff ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-remediation-handoff step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-remediation-handoff step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiRemediationHandoffSteps -join "|") -ne ($expectedEditorAiRemediationHandoffSteps -join "|")) {
        Write-Error "engine manifest editor-ai-playtest-remediation-handoff orderedSteps must be exactly: $($expectedEditorAiRemediationHandoffSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestRemediationHandoffLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-playtest-remediation-handoff requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("EditorAiPlaytestRemediationQueueModel", "recipe id", "evidence status", "remediation category", "host gates", "blocked reasons", "readiness dependency", "unsupported claims")) {
        if (-not ((@($editorAiPlaytestRemediationHandoffLoop[0].handoffInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-handoff handoffInputs missing: $requiredInput"
        }
    }
    foreach ($field in @("recipe id", "evidence status", "remediation category", "external-owner", "action kind", "handoff text", "host gates", "blockers", "readiness dependency", "unsupported claims")) {
        if (-not ((@($editorAiPlaytestRemediationHandoffLoop[0].handoffFields) -join " ").Contains($field))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-handoff handoffFields missing: $field"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "raw/free-form command evaluation", "validation execution", "package script execution", "free-form manifest edits", "evidence mutation", "remediation mutation", "fix execution", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestRemediationHandoffLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-handoff blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestRemediationHandoffLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-handoff unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPlaytestOperatorWorkflowLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-operator-workflow" })
if ($editorAiPlaytestOperatorWorkflowLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-operator-workflow review loop"
}
if ($editorAiPlaytestOperatorWorkflowLoop.Count -eq 1) {
    Assert-Properties $editorAiPlaytestOperatorWorkflowLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "workflowInputs", "workflowFields", "structuredReportSurface", "closeoutPolicy", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-playtest-operator-workflow"
    if ($editorAiPlaytestOperatorWorkflowLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-playtest-operator-workflow must be ready as a read-only consolidated operator workflow"
    }
    $expectedEditorAiOperatorWorkflowSteps = @(
        "inspect-package-authoring-diagnostics",
        "preflight-validation-recipes",
        "report-playtest-readiness",
        "handoff-reviewed-operator-commands",
        "summarize-external-validation-evidence",
        "queue-nonpassing-remediation",
        "handoff-remediation-actions",
        "closeout-by-rerunning-evidence-summary"
    )
    $actualEditorAiOperatorWorkflowSteps = @($editorAiPlaytestOperatorWorkflowLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-playtest-operator-workflow ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiOperatorWorkflowSteps -join "|") -ne ($expectedEditorAiOperatorWorkflowSteps -join "|")) {
        Write-Error "engine manifest editor-ai-playtest-operator-workflow orderedSteps must be exactly: $($expectedEditorAiOperatorWorkflowSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestOperatorWorkflowLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("EditorAiPackageAuthoringDiagnosticsModel", "EditorAiValidationRecipePreflightModel", "EditorAiPlaytestReadinessReportModel", "EditorAiPlaytestOperatorHandoffModel", "externally supplied validation evidence", "EditorAiPlaytestEvidenceSummaryModel", "EditorAiPlaytestRemediationQueueModel", "EditorAiPlaytestRemediationHandoffModel")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].workflowInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow workflowInputs missing: $requiredInput"
        }
    }
    foreach ($field in @("package diagnostics", "validation preflight", "readiness dependency", "operator command rows", "evidence status", "remediation category", "external-owner", "closeout through existing evidence summary")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].workflowFields) -join " ").Contains($field))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow workflowFields missing: $field"
        }
    }
    Assert-Properties $editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface @("model", "stageFields", "closeoutFields", "blockedExecution", "unsupportedClaims", "notes") "engine manifest editor-ai-playtest-operator-workflow structuredReportSurface"
    if ($editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface.model -ne "EditorAiPlaytestOperatorWorkflowReportModel") {
        Write-Error "engine manifest editor-ai-playtest-operator-workflow structuredReportSurface.model must be EditorAiPlaytestOperatorWorkflowReportModel"
    }
    foreach ($field in @("operator workflow stage", "source model", "source row count", "source row ids", "status", "host gates", "blockers", "diagnostic")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface.stageFields) -join " ").Contains($field))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow structuredReportSurface.stageFields missing: $field"
        }
    }
    foreach ($field in @("all_required_evidence_passed", "remediation_required=false", "handoff_required=false", "closeout_complete", "external_action_required")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface.closeoutFields) -join " ").Contains($field))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow structuredReportSurface.closeoutFields missing: $field"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "raw/free-form command evaluation", "validation execution", "package script execution", "free-form manifest edits", "manifest mutation", "evidence mutation", "remediation mutation", "fix execution", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface.blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow structuredReportSurface.blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].structuredReportSurface.unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow structuredReportSurface.unsupportedClaims missing: $claim"
        }
    }
    foreach ($policy in @("no separate remediation evidence-review row model", "no separate remediation closeout-report row model", "all_required_evidence_passed", "remediation_required=false", "handoff_required=false")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].closeoutPolicy) -join " ").Contains($policy))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow closeoutPolicy missing: $policy"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "raw/free-form command evaluation", "validation execution", "package script execution", "free-form manifest edits", "evidence mutation", "remediation mutation", "fix execution", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestOperatorWorkflowLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-operator-workflow unsupportedClaims missing: $claim"
        }
    }
}
$rendererResourceExecutionLoop = @($productionLoop.resourceExecutionLoops | Where-Object { $_.id -eq "renderer-resource-residency-upload-execution" })
if ($rendererResourceExecutionLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one renderer-resource-residency-upload-execution loop"
}

$materialShaderAuthoringLoop = @($productionLoop.materialShaderAuthoringLoops | Where-Object { $_.id -eq "material-shader-authoring-review-loop" })
if ($materialShaderAuthoringLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one material-shader-authoring-review-loop"
} else {
    Assert-Properties $materialShaderAuthoringLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "preSmokeGates", "descriptorFields", "hostGatedSmokeRecipes", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop material-shader-authoring-review-loop"
    if ($materialShaderAuthoringLoop[0].status -ne "host-gated") {
        Write-Error "engine manifest material-shader-authoring-review-loop must remain host-gated"
    }
    $expectedMaterialShaderSteps = @(
        "review-source-material-authoring-inputs",
        "validate-source-material-and-texture-dependencies",
        "review-fixed-shader-artifact-requests",
        "validate-shader-artifacts",
        "run-host-gated-material-shader-package-smoke"
    )
    $actualMaterialShaderSteps = @($materialShaderAuthoringLoop[0].orderedSteps | ForEach-Object {
            Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest material-shader-authoring-review-loop ordered step"
            $_.id
        })
    if (($actualMaterialShaderSteps -join "|") -ne ($expectedMaterialShaderSteps -join "|")) {
        Write-Error "engine manifest material-shader-authoring-review-loop orderedSteps must be exactly: $($expectedMaterialShaderSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "materialShaderAuthoringTargets", "validationRecipes")) {
        if (@($materialShaderAuthoringLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest material-shader-authoring-review-loop requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("id", "sourceMaterialPath", "runtimeMaterialPath", "packageIndexPath", "shaderSourcePaths", "d3d12ShaderArtifactPaths", "vulkanShaderArtifactPaths")) {
        if (@($materialShaderAuthoringLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine manifest material-shader-authoring-review-loop descriptorFields missing: $field"
        }
    }
    foreach ($claim in @("shader graph", "material graph", "live shader generation", "package streaming", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($materialShaderAuthoringLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest material-shader-authoring-review-loop unsupportedClaims missing: $claim"
        }
    }
}
$atlasTilemapAuthoringLoop = @($productionLoop.atlasTilemapAuthoringLoops | Where-Object { $_.id -eq "2d-atlas-tilemap-package-authoring" })
if ($atlasTilemapAuthoringLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one 2d-atlas-tilemap-package-authoring loop"
} else {
    Assert-Properties $atlasTilemapAuthoringLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "descriptorFields", "preflightGates", "hostGatedSmokeRecipes", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop 2d-atlas-tilemap-package-authoring"
    if ($atlasTilemapAuthoringLoop[0].status -ne "host-gated") {
        Write-Error "engine manifest 2d-atlas-tilemap-package-authoring must remain host-gated because desktop package recipes are optional host lanes"
    }
    $expectedAtlasTilemapSteps = @(
        "select-atlas-tilemap-authoring-target",
        "author-deterministic-tilemap-metadata",
        "update-tilemap-package-index",
        "validate-runtime-tilemap-payload",
        "run-host-gated-2d-package-preflight"
    )
    $actualAtlasTilemapSteps = @($atlasTilemapAuthoringLoop[0].orderedSteps | ForEach-Object {
            Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest 2d-atlas-tilemap-package-authoring ordered step"
            $_.id
        })
    if (($actualAtlasTilemapSteps -join "|") -ne ($expectedAtlasTilemapSteps -join "|")) {
        Write-Error "engine manifest 2d-atlas-tilemap-package-authoring orderedSteps must be exactly: $($expectedAtlasTilemapSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "atlasTilemapAuthoringTargets", "validationRecipes")) {
        if (@($atlasTilemapAuthoringLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest 2d-atlas-tilemap-package-authoring requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("id", "packageIndexPath", "tilemapPath", "atlasTexturePath", "tilemapAssetKey", "atlasTextureAssetKey", "mode", "sourceDecoding", "atlasPacking", "nativeGpuSpriteBatching", "preflightRecipeIds")) {
        if (@($atlasTilemapAuthoringLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine manifest 2d-atlas-tilemap-package-authoring descriptorFields missing: $field"
        }
    }
    foreach ($gate in @("validate-runtime-scene-package", "desktop-game-runtime", "desktop-runtime-release-target")) {
        if (@($atlasTilemapAuthoringLoop[0].preflightGates) -notcontains $gate) {
            Write-Error "engine manifest 2d-atlas-tilemap-package-authoring preflightGates missing: $gate"
        }
    }
    foreach ($claim in @("source image decoding", "production atlas packing", "tilemap editor UX", "native GPU sprite batching", "package streaming execution", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($atlasTilemapAuthoringLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest 2d-atlas-tilemap-package-authoring unsupportedClaims missing: $claim"
        }
    }
}
if (-not $productionLoop.PSObject.Properties.Name.Contains("prefabScenePackageAuthoringLoops")) {
    Write-Error "engine manifest aiOperableProductionLoop missing prefabScenePackageAuthoringLoops"
}
$prefabScene3dAuthoringLoop = @($productionLoop.prefabScenePackageAuthoringLoops | Where-Object { $_.id -eq "3d-prefab-scene-package-authoring" })
if ($prefabScene3dAuthoringLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one 3d-prefab-scene-package-authoring loop"
} else {
    Assert-Properties $prefabScene3dAuthoringLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "descriptorFields", "preflightGates", "hostGatedSmokeRecipes", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop 3d-prefab-scene-package-authoring"
    if ($prefabScene3dAuthoringLoop[0].status -ne "host-gated") {
        Write-Error "engine manifest 3d-prefab-scene-package-authoring must remain host-gated because desktop package recipes are optional host lanes"
    }
    $expectedPrefabScene3dSteps = @(
        "select-prefab-scene-package-authoring-target",
        "apply-scene-prefab-v2-authoring-commands",
        "cook-selected-source-registry-rows",
        "migrate-scene-v2-runtime-package",
        "validate-runtime-scene-package",
        "run-host-gated-3d-package-smoke"
    )
    $actualPrefabScene3dSteps = @($prefabScene3dAuthoringLoop[0].orderedSteps | ForEach-Object {
            Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest 3d-prefab-scene-package-authoring ordered step"
            $_.id
        })
    if (($actualPrefabScene3dSteps -join "|") -ne ($expectedPrefabScene3dSteps -join "|")) {
        Write-Error "engine manifest 3d-prefab-scene-package-authoring orderedSteps must be exactly: $($expectedPrefabScene3dSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($prefabScene3dAuthoringLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest 3d-prefab-scene-package-authoring requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("id", "mode", "sceneAuthoringPath", "prefabAuthoringPath", "sourceRegistryPath", "packageIndexPath", "outputScenePath", "sceneAssetKey", "runtimeSceneValidationTargetId", "authoringCommandRows", "selectedSourceAssetKeys", "sourceCookMode", "sceneMigration", "runtimeSceneValidation", "hostGatedSmokeRecipeIds", "broadImporterExecution", "broadDependencyCooking", "runtimeSourceParsing", "materialGraph", "shaderGraph", "liveShaderGeneration", "skeletalAnimation", "gpuSkinning", "publicNativeRhiHandles", "metalReadiness", "rendererQuality", "cookCommandId", "prefabScenePackageAuthoringTargetId", "selectedAssetKeys", "dependencyExpansion", "dependencyCooking", "externalImporterExecution", "rendererRhiResidency", "packageStreaming", "editorProductization", "generalProductionRendererQuality", "arbitraryShell", "freeFormEdit")) {
        if (@($prefabScene3dAuthoringLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine manifest 3d-prefab-scene-package-authoring descriptorFields missing: $field"
        }
    }
    foreach ($gate in @("scene-prefab-authoring", "cook-registered-source-assets", "migrate-scene-v2-runtime-package", "validate-runtime-scene-package", "desktop-game-runtime", "desktop-runtime-release-target")) {
        if (@($prefabScene3dAuthoringLoop[0].preflightGates) -notcontains $gate) {
            Write-Error "engine manifest 3d-prefab-scene-package-authoring preflightGates missing: $gate"
        }
    }
    foreach ($claim in @("broad importer execution", "broad/dependent package cooking", "runtime source parsing", "material graph", "shader graph", "live shader generation", "skeletal animation", "GPU skinning", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($prefabScene3dAuthoringLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest 3d-prefab-scene-package-authoring unsupportedClaims missing: $claim"
        }
    }
}
$packageStreamingResidencyLoop = @($productionLoop.packageStreamingResidencyLoops | Where-Object { $_.id -eq "package-streaming-residency-budget-contract" })
if ($packageStreamingResidencyLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one package-streaming-residency-budget-contract loop"
} else {
    Assert-Properties $packageStreamingResidencyLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "descriptorFields", "preflightGates", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop package-streaming-residency-budget-contract"
    if ($packageStreamingResidencyLoop[0].status -ne "ready") {
        Write-Error "engine manifest package-streaming-residency-budget-contract must be ready after the descriptor contract is implemented"
    }
    $expectedPackageStreamingSteps = @(
        "select-package-streaming-residency-target",
        "validate-runtime-scene-package",
        "review-residency-budget-intent",
        "confirm-host-owned-resource-upload-gate",
        "select-host-gated-safe-point-execution"
    )
    $actualPackageStreamingSteps = @($packageStreamingResidencyLoop[0].orderedSteps | ForEach-Object {
            Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest package-streaming-residency-budget-contract ordered step"
            $_.id
        })
    if (($actualPackageStreamingSteps -join "|") -ne ($expectedPackageStreamingSteps -join "|")) {
        Write-Error "engine manifest package-streaming-residency-budget-contract orderedSteps must be exactly: $($expectedPackageStreamingSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "packageStreamingResidencyTargets", "validationRecipes")) {
        if (@($packageStreamingResidencyLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest package-streaming-residency-budget-contract requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("id", "packageIndexPath", "runtimeSceneValidationTargetId", "mode", "residentBudgetBytes", "safePointRequired", "preloadAssetKeys", "residentResourceKinds")) {
        if (@($packageStreamingResidencyLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine manifest package-streaming-residency-budget-contract descriptorFields missing: $field"
        }
    }
    foreach ($gate in @("validate-runtime-scene-package", "renderer-resource-residency-upload-execution")) {
        if (@($packageStreamingResidencyLoop[0].preflightGates) -notcontains $gate) {
            Write-Error "engine manifest package-streaming-residency-budget-contract preflightGates missing: $gate"
        }
    }
    foreach ($blocked in @("background package streaming", "arbitrary eviction", "public native/RHI handles", "Metal readiness", "production renderer quality")) {
        if (-not ((@($packageStreamingResidencyLoop[0].blockedExecution) -join " ").Contains($blocked))) {
            Write-Error "engine manifest package-streaming-residency-budget-contract blockedExecution missing: $blocked"
        }
    }
    foreach ($claim in @("broad async/background package streaming", "arbitrary eviction", "texture streaming", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($packageStreamingResidencyLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest package-streaming-residency-budget-contract unsupportedClaims missing: $claim"
        }
    }
}
$hostGatedPackageStreamingLoop = @($productionLoop.packageStreamingResidencyLoops | Where-Object { $_.id -eq "host-gated-package-streaming-execution" })
if ($hostGatedPackageStreamingLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one host-gated-package-streaming-execution loop"
} else {
    Assert-Properties $hostGatedPackageStreamingLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "descriptorFields", "preflightGates", "resultFields", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop host-gated-package-streaming-execution"
    if ($hostGatedPackageStreamingLoop[0].status -ne "host-gated") {
        Write-Error "engine manifest host-gated-package-streaming-execution must be host-gated"
    }
    $expectedHostGatedStreamingSteps = @(
        "select-package-streaming-residency-target",
        "validate-runtime-scene-package",
        "load-selected-runtime-package",
        "commit-safe-point-package-streaming-replacement",
        "report-streaming-execution-diagnostics",
        "keep-renderer-rhi-teardown-host-owned"
    )
    $actualHostGatedStreamingSteps = @($hostGatedPackageStreamingLoop[0].orderedSteps | ForEach-Object {
            Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest host-gated-package-streaming-execution ordered step"
            $_.id
        })
    if (($actualHostGatedStreamingSteps -join "|") -ne ($expectedHostGatedStreamingSteps -join "|")) {
        Write-Error "engine manifest host-gated-package-streaming-execution orderedSteps must be exactly: $($expectedHostGatedStreamingSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "packageStreamingResidencyTargets", "validationRecipes")) {
        if (@($hostGatedPackageStreamingLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest host-gated-package-streaming-execution requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("id", "packageIndexPath", "runtimeSceneValidationTargetId", "mode", "residentBudgetBytes", "safePointRequired", "preloadAssetKeys", "residentResourceKinds", "maxResidentPackages", "preflightRecipeIds")) {
        if (@($hostGatedPackageStreamingLoop[0].descriptorFields) -notcontains $field) {
            Write-Error "engine manifest host-gated-package-streaming-execution descriptorFields missing: $field"
        }
    }
    foreach ($gate in @("validate-runtime-scene-package", "safe-point-package-unload-replacement-execution")) {
        if (@($hostGatedPackageStreamingLoop[0].preflightGates) -notcontains $gate) {
            Write-Error "engine manifest host-gated-package-streaming-execution preflightGates missing: $gate"
        }
    }
    foreach ($field in @("status", "target_id", "package_index_path", "runtime_scene_validation_target_id", "estimated_resident_bytes", "resident_budget_bytes", "replacement_status", "stale_handle_count", "diagnostics")) {
        if (@($hostGatedPackageStreamingLoop[0].resultFields) -notcontains $field) {
            Write-Error "engine manifest host-gated-package-streaming-execution resultFields missing: $field"
        }
    }
    foreach ($blocked in @("background package streaming", "arbitrary eviction", "dependency-driven streaming middleware", "renderer/RHI teardown execution", "public native/RHI handles", "allocator/GPU budget enforcement", "Metal readiness", "production renderer quality")) {
        if (-not ((@($hostGatedPackageStreamingLoop[0].blockedExecution) -join " ").Contains($blocked))) {
            Write-Error "engine manifest host-gated-package-streaming-execution blockedExecution missing: $blocked"
        }
    }
    foreach ($claim in @("broad async/background package streaming", "arbitrary eviction", "texture streaming", "allocator/GPU budget enforcement", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($hostGatedPackageStreamingLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest host-gated-package-streaming-execution unsupportedClaims missing: $claim"
        }
    }
    foreach ($helperPath in @(
        (Join-Path $root "engine/runtime/include/mirakana/runtime/package_streaming.hpp"),
        (Join-Path $root "engine/runtime/src/package_streaming.cpp")
    )) {
        if (Test-Path -LiteralPath $helperPath -PathType Leaf) {
            $helperText = Get-Content -LiteralPath $helperPath -Raw
            foreach ($forbiddenText in @(
                "mirakana/rhi/",
                "mirakana/renderer/",
                "mirakana/runtime_scene_rhi/",
                "IRhiDevice",
                "SceneGpuBindingPalette",
                "MeshGpuBinding",
                "MaterialGpuBinding",
                "BufferHandle",
                "TextureHandle",
                "DescriptorSetHandle",
                "PipelineLayoutHandle",
                "SwapchainFrame",
                "nativeHandle",
                "allocatorHandle"
            )) {
                if ($helperText.Contains($forbiddenText)) {
                    Write-Error "$helperPath must keep package streaming execution free of renderer/RHI/native surfaces: $forbiddenText"
                }
            }
        }
    }
}
$safePointPackageReplacementLoop = @($productionLoop.safePointPackageReplacementLoops | Where-Object { $_.id -eq "safe-point-package-unload-replacement-execution" })
if ($safePointPackageReplacementLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one safe-point-package-unload-replacement-execution loop"
} else {
    Assert-Properties $safePointPackageReplacementLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredModules", "resultFields", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop safe-point-package-unload-replacement-execution"
    if ($safePointPackageReplacementLoop[0].status -ne "ready") {
        Write-Error "engine manifest safe-point-package-unload-replacement-execution must be ready after runtime package/catalog safe-point replacement tests pass"
    }
    $expectedSafePointSteps = @(
        "stage-loaded-runtime-package",
        "build-pending-resource-catalog",
        "commit-package-and-resource-catalog-at-safe-point",
        "reject-invalid-package-before-active-swap",
        "commit-unload-and-empty-catalog-at-safe-point",
        "keep-renderer-rhi-teardown-host-owned"
    )
    $actualSafePointSteps = @($safePointPackageReplacementLoop[0].orderedSteps | ForEach-Object {
            Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest safe-point-package-unload-replacement-execution ordered step"
            $_.id
        })
    if (($actualSafePointSteps -join "|") -ne ($expectedSafePointSteps -join "|")) {
        Write-Error "engine manifest safe-point-package-unload-replacement-execution orderedSteps must be exactly: $($expectedSafePointSteps -join ', ')"
    }
    foreach ($module in @("MK_runtime")) {
        if (@($safePointPackageReplacementLoop[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest safe-point-package-unload-replacement-execution requiredModules missing: $module"
        }
    }
    foreach ($field in @("status", "previous_record_count", "committed_record_count", "previous_generation", "committed_generation", "stale_handle_count", "discarded_pending_package", "diagnostics")) {
        if (@($safePointPackageReplacementLoop[0].resultFields) -notcontains $field) {
            Write-Error "engine manifest safe-point-package-unload-replacement-execution resultFields missing: $field"
        }
    }
    foreach ($blocked in @("async package streaming", "background eviction", "renderer/RHI teardown execution", "public native/RHI handles", "Metal readiness", "production renderer quality")) {
        if (-not ((@($safePointPackageReplacementLoop[0].blockedExecution) -join " ").Contains($blocked))) {
            Write-Error "engine manifest safe-point-package-unload-replacement-execution blockedExecution missing: $blocked"
        }
    }
    foreach ($claim in @("broad package streaming", "async eviction", "texture streaming", "allocator/GPU budget enforcement", "public native/RHI handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($safePointPackageReplacementLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest safe-point-package-unload-replacement-execution unsupportedClaims missing: $claim"
        }
    }
}
if ($rendererResourceExecutionLoop.Count -eq 1) {
    Assert-Properties $rendererResourceExecutionLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "preUploadGate", "hostGatedSmokeRecipes", "reportFields", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop renderer-resource-residency-upload-execution"
    if ($rendererResourceExecutionLoop[0].status -ne "host-gated") {
        Write-Error "engine manifest renderer-resource-residency-upload-execution must remain host-gated"
    }
    $expectedResourceExecutionSteps = @(
        "validate-runtime-scene-package",
        "instantiate-runtime-scene",
        "build-scene-render-packet",
        "execute-host-owned-runtime-scene-gpu-upload",
        "report-backend-neutral-upload-residency-counters",
        "run-host-gated-scene-gpu-smoke"
    )
    $actualResourceExecutionSteps = @($rendererResourceExecutionLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest renderer-resource-residency-upload-execution ordered step"
        $_.id
    })
    if (($actualResourceExecutionSteps -join "|") -ne ($expectedResourceExecutionSteps -join "|")) {
        Write-Error "engine manifest renderer-resource-residency-upload-execution orderedSteps must be exactly: $($expectedResourceExecutionSteps -join ', ')"
    }
    if ($rendererResourceExecutionLoop[0].preUploadGate -ne "validate-runtime-scene-package") {
        Write-Error "engine manifest renderer-resource-residency-upload-execution preUploadGate must be validate-runtime-scene-package"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "validationRecipes")) {
        if (@($rendererResourceExecutionLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest renderer-resource-residency-upload-execution requiredManifestFields missing: $field"
        }
    }
    foreach ($field in @("scene_gpu_status", "scene_gpu_mesh_uploads", "scene_gpu_texture_uploads", "scene_gpu_material_bindings", "scene_gpu_uploaded_texture_bytes", "scene_gpu_uploaded_mesh_bytes", "scene_gpu_uploaded_material_factor_bytes", "scene_gpu_morph_mesh_bindings", "scene_gpu_morph_mesh_uploads", "scene_gpu_uploaded_morph_bytes", "scene_gpu_morph_mesh_resolved", "renderer_gpu_morph_draws", "renderer_morph_descriptor_binds")) {
        if (@($rendererResourceExecutionLoop[0].reportFields) -notcontains $field) {
            Write-Error "engine manifest renderer-resource-residency-upload-execution reportFields missing: $field"
        }
    }
    foreach ($claim in @("public native handles", "IRhiDevice exposure to gameplay", "package streaming", "broad residency budgets", "material/shader graph", "live shader generation", "Metal readiness", "general renderer quality")) {
        if (-not ((@($rendererResourceExecutionLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest renderer-resource-residency-upload-execution unsupportedClaims missing: $claim"
        }
    }
}
$expectedProductionRecipeIds = @(
    "headless-gameplay",
    "desktop-runtime-config-package",
    "desktop-runtime-cooked-scene-package",
    "desktop-runtime-material-shader-package",
    "ai-navigation-headless",
    "runtime-ui-headless",
    "2d-playable-source-tree",
    "2d-desktop-runtime-package",
    "3d-playable-desktop-package",
    "native-gpu-runtime-ui-overlay",
    "native-ui-textured-sprite-atlas",
    "native-ui-atlas-package-metadata",
    "future-3d-playable-vertical-slice"
)
$productionRecipeIds = @{}
foreach ($recipe in $productionLoop.recipes) {
    Assert-Properties $recipe @("id", "status", "summary", "requiredModules", "allowedTemplates", "allowedPackagingTargets", "importerAssumptions", "cookedRuntimeAssumptions", "rendererBackendAssumptions", "validationRecipes", "unsupportedClaims", "followUpCapability") "engine manifest aiOperableProductionLoop recipe"
    if ($productionRecipeIds.ContainsKey($recipe.id)) {
        Write-Error "engine manifest aiOperableProductionLoop recipe id is duplicated: $($recipe.id)"
    }
    $productionRecipeIds[$recipe.id] = $true
    if ($allowedProductionStatuses -notcontains $recipe.status) {
        Write-Error "engine manifest aiOperableProductionLoop recipe '$($recipe.id)' has invalid status: $($recipe.status)"
    }
    foreach ($module in @($recipe.requiredModules)) {
        if (-not $moduleNames.ContainsKey($module)) {
            Write-Error "engine manifest aiOperableProductionLoop recipe '$($recipe.id)' references unknown module: $module"
        }
    }
    foreach ($target in @($recipe.allowedPackagingTargets)) {
        if (-not $packagingTargetNames.ContainsKey($target)) {
            Write-Error "engine manifest aiOperableProductionLoop recipe '$($recipe.id)' references unknown packaging target: $target"
        }
    }
    foreach ($validationRecipe in @($recipe.validationRecipes)) {
        if (-not $validationRecipeNames.ContainsKey($validationRecipe)) {
            Write-Error "engine manifest aiOperableProductionLoop recipe '$($recipe.id)' references unknown validation recipe: $validationRecipe"
        }
    }
    if (@("ready", "host-gated") -contains $recipe.status -and @($recipe.validationRecipes).Count -lt 1) {
        Write-Error "engine manifest aiOperableProductionLoop recipe '$($recipe.id)' must declare validation recipes when status is $($recipe.status)"
    }
    if (@($recipe.unsupportedClaims).Count -lt 1) {
        Write-Error "engine manifest aiOperableProductionLoop recipe '$($recipe.id)' must list unsupportedClaims"
    }
}
foreach ($recipeId in $expectedProductionRecipeIds) {
    if (-not $productionRecipeIds.ContainsKey($recipeId)) {
        Write-Error "engine manifest aiOperableProductionLoop missing recipe id: $recipeId"
    }
}
foreach ($recipeId in @("future-3d-playable-vertical-slice")) {
    $futureRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq $recipeId })
    if ($futureRecipe.Count -ne 1 -or $futureRecipe[0].status -ne "planned") {
        Write-Error "engine manifest aiOperableProductionLoop recipe '$recipeId' must remain planned"
    }
}
foreach ($recipeId in @("desktop-runtime-config-package", "desktop-runtime-cooked-scene-package", "desktop-runtime-material-shader-package", "2d-desktop-runtime-package", "3d-playable-desktop-package", "native-gpu-runtime-ui-overlay", "native-ui-textured-sprite-atlas", "native-ui-atlas-package-metadata")) {
    $desktopRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq $recipeId })
    if ($desktopRecipe.Count -ne 1 -or $desktopRecipe[0].status -ne "host-gated") {
        Write-Error "engine manifest aiOperableProductionLoop recipe '$recipeId' must remain host-gated"
    }
}
foreach ($recipeId in @("headless-gameplay", "ai-navigation-headless", "runtime-ui-headless", "2d-playable-source-tree")) {
    $readyRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq $recipeId })
    if ($readyRecipe.Count -ne 1 -or $readyRecipe[0].status -ne "ready") {
        Write-Error "engine manifest aiOperableProductionLoop recipe '$recipeId' must remain ready"
    }
}
$sourceTree2dRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "2d-playable-source-tree" })
if ($sourceTree2dRecipe.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose exactly one 2d-playable-source-tree recipe"
} else {
    foreach ($module in @("MK_core", "MK_runtime", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_renderer")) {
        if (@($sourceTree2dRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest 2d-playable-source-tree recipe missing required module: $module"
        }
    }
    if (@($sourceTree2dRecipe[0].allowedTemplates) -notcontains "Headless") {
        Write-Error "engine manifest 2d-playable-source-tree recipe must allow the Headless template for source-tree proof"
    }
    if (@($sourceTree2dRecipe[0].allowedPackagingTargets) -notcontains "source-tree-default") {
        Write-Error "engine manifest 2d-playable-source-tree recipe must be source-tree-default validated"
    }
    if (@($sourceTree2dRecipe[0].validationRecipes) -notcontains "default") {
        Write-Error "engine manifest 2d-playable-source-tree recipe must include the default validation recipe"
    }
    foreach ($claim in @("visible desktop package proof", "texture atlas cook", "tilemap editor UX", "runtime image decoding", "production sprite batching", "native GPU output", "public native or RHI handle access")) {
        if (-not ((@($sourceTree2dRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest 2d-playable-source-tree recipe must keep unsupported claim explicit: $claim"
        }
    }
}
if (@($productionLoop.recipes | Where-Object { $_.id -eq "future-2d-playable-vertical-slice" }).Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop must not keep stale future-2d-playable-vertical-slice after the source-tree 2D recipe is implemented"
}
$desktop2dRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "2d-desktop-runtime-package" })
if ($desktop2dRecipe.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose exactly one 2d-desktop-runtime-package recipe"
} else {
    foreach ($module in @("MK_core", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_scene", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_renderer")) {
        if (@($desktop2dRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest 2d-desktop-runtime-package recipe missing required module: $module"
        }
    }
    if (@($desktop2dRecipe[0].allowedTemplates) -notcontains "DesktopRuntime2DPackage") {
        Write-Error "engine manifest 2d-desktop-runtime-package recipe must allow DesktopRuntime2DPackage"
    }
    foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
        if (@($desktop2dRecipe[0].allowedPackagingTargets) -notcontains $target) {
            Write-Error "engine manifest 2d-desktop-runtime-package recipe must allow $target"
        }
    }
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-2d-package-proof", "desktop-runtime-2d-vulkan-window-package", "shader-toolchain")) {
        if (@($desktop2dRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine manifest 2d-desktop-runtime-package recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("texture atlas cook", "tilemap editor UX", "runtime image decoding", "production sprite batching", "package streaming", "3D playable vertical slice", "editor productization", "public native or RHI handle access", "Metal readiness", "general production renderer quality")) {
        if (-not ((@($desktop2dRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest 2d-desktop-runtime-package recipe must keep unsupported claim explicit: $claim"
        }
    }
}
$desktop3dRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "3d-playable-desktop-package" })
if ($desktop3dRecipe.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose exactly one 3d-playable-desktop-package recipe"
} else {
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($desktop3dRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest 3d-playable-desktop-package recipe missing required module: $module"
        }
    }
    if (@($desktop3dRecipe[0].allowedTemplates) -notcontains "DesktopRuntime3DPackage") {
        Write-Error "engine manifest 3d-playable-desktop-package recipe must allow DesktopRuntime3DPackage"
    }
    foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
        if (@($desktop3dRecipe[0].allowedPackagingTargets) -notcontains $target) {
            Write-Error "engine manifest 3d-playable-desktop-package recipe must allow $target"
        }
    }
    Assert-ContainsText ([string]$desktop3dRecipe[0].summary) "--require-shadow-morph-composition" "engine manifest 3d-playable-desktop-package summary"
    Assert-ContainsText ([string]$desktop3dRecipe[0].cookedRuntimeAssumptions) "--require-shadow-morph-composition" "engine manifest 3d-playable-desktop-package cooked runtime assumptions"
    Assert-ContainsText ([string]$desktop3dRecipe[0].cookedRuntimeAssumptions) "renderer_morph_descriptor_binds" "engine manifest 3d-playable-desktop-package cooked runtime assumptions"
    Assert-ContainsText ([string]$desktop3dRecipe[0].rendererBackendAssumptions.d3d12) "selected generated graphics morph + directional shadow receiver" "engine manifest 3d-playable-desktop-package d3d12 backend assumptions"
    Assert-ContainsText ([string]$desktop3dRecipe[0].rendererBackendAssumptions.vulkan) "no Vulkan shadow-morph validation recipe is ready" "engine manifest 3d-playable-desktop-package vulkan backend assumptions"
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-sample-game-scene-gpu-package", "desktop-runtime-sample-game-vulkan-scene-gpu-package", "installed-d3d12-3d-shadow-morph-composition-smoke", "shader-toolchain")) {
        if (@($desktop3dRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine manifest 3d-playable-desktop-package recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("runtime source asset parsing", "material graph", "shader graph", "skeletal animation production path", "GPU skinning", "package streaming", "broad shadow+morph composition beyond the selected receiver smoke", "compute morph + shadow composition", "morph-deformed shadow-caster silhouettes", "Metal ready", "public native or RHI handle access", "general production renderer quality")) {
        if (-not ((@($desktop3dRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest 3d-playable-desktop-package recipe must keep unsupported claim explicit: $claim"
        }
    }
    if (((@($desktop3dRecipe[0].unsupportedClaims) -join " ").Contains("native GPU HUD or sprite overlay output"))) {
        Write-Error "engine manifest 3d-playable-desktop-package recipe must not keep stale native GPU HUD or sprite overlay unsupported claim after the focused overlay recipe is added"
    }
}
$nativeOverlayRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "native-gpu-runtime-ui-overlay" })
if ($nativeOverlayRecipe.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose exactly one native-gpu-runtime-ui-overlay recipe"
} else {
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($nativeOverlayRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest native-gpu-runtime-ui-overlay recipe missing required module: $module"
        }
    }
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-sample-game-native-ui-overlay-package", "desktop-runtime-sample-game-vulkan-native-ui-overlay-package", "shader-toolchain")) {
        if (@($nativeOverlayRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine manifest native-gpu-runtime-ui-overlay recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("production text shaping", "font rasterization", "glyph atlas", "image decoding", "real texture atlas", "IME", "OS accessibility bridge", "Metal ready", "public native or RHI handle access", "general production renderer quality")) {
        if (-not ((@($nativeOverlayRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest native-gpu-runtime-ui-overlay recipe must keep unsupported claim explicit: $claim"
        }
    }
}
$texturedUiRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "native-ui-textured-sprite-atlas" })
if ($texturedUiRecipe.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose exactly one native-ui-textured-sprite-atlas recipe"
} else {
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($texturedUiRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest native-ui-textured-sprite-atlas recipe missing required module: $module"
        }
    }
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-sample-game-textured-ui-atlas-package", "desktop-runtime-sample-game-vulkan-textured-ui-atlas-package", "shader-toolchain")) {
        if (@($texturedUiRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine manifest native-ui-textured-sprite-atlas recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("source image decoding", "production atlas packing", "production text shaping", "font rasterization", "glyph atlas", "IME", "OS accessibility bridge", "Metal ready", "public native or RHI handle access", "general production UI renderer quality")) {
        if (-not ((@($texturedUiRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest native-ui-textured-sprite-atlas recipe must keep unsupported claim explicit: $claim"
        }
    }
}
$uiAtlasMetadataRecipe = @($productionLoop.recipes | Where-Object { $_.id -eq "native-ui-atlas-package-metadata" })
if ($uiAtlasMetadataRecipe.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose exactly one native-ui-atlas-package-metadata recipe"
} else {
    foreach ($module in @("MK_core", "MK_math", "MK_platform", "MK_platform_sdl3", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($uiAtlasMetadataRecipe[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest native-ui-atlas-package-metadata recipe missing required module: $module"
        }
    }
    foreach ($validationRecipe in @("desktop-game-runtime", "desktop-runtime-sample-game-ui-atlas-metadata-package", "desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package", "shader-toolchain")) {
        if (@($uiAtlasMetadataRecipe[0].validationRecipes) -notcontains $validationRecipe) {
            Write-Error "engine manifest native-ui-atlas-package-metadata recipe must include $validationRecipe validation"
        }
    }
    foreach ($claim in @("runtime source PNG/JPEG image decoding", "production atlas packing", "production text shaping", "font rasterization", "glyph atlas", "IME", "OS accessibility bridge", "Metal ready", "public native or RHI handle access", "general production UI renderer quality")) {
        if (-not ((@($uiAtlasMetadataRecipe[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest native-ui-atlas-package-metadata recipe must keep unsupported claim explicit: $claim"
        }
    }
}

$expectedCommandSurfaceIds = @(
    "create-game-recipe",
    "create-scene",
    "update-scene-package",
    "migrate-scene-v2-runtime-package",
    "validate-runtime-scene-package",
    "add-scene-node",
    "add-or-update-component",
    "create-prefab",
    "instantiate-prefab",
    "create-material-instance",
    "create-material-from-graph",
    "register-source-asset",
    "cook-registered-source-assets",
    "cook-runtime-package",
    "register-runtime-package-files",
    "update-ui-atlas-metadata-package",
    "update-game-agent-manifest",
    "run-validation-recipe"
)
$knownAuthoringSurfaceIds = @{}
foreach ($authoringSurface in $productionLoop.authoringSurfaces) {
    $knownAuthoringSurfaceIds[$authoringSurface.id] = $true
}
$knownPackageSurfaceIds = @{}
foreach ($packageSurface in $productionLoop.packageSurfaces) {
    $knownPackageSurfaceIds[$packageSurface.id] = $true
}
$knownUnsupportedGapIds = @{}
foreach ($gap in $productionLoop.unsupportedProductionGaps) {
    $knownUnsupportedGapIds[$gap.id] = $true
}
$knownHostGateIds = @{}
foreach ($hostGate in $productionLoop.hostGates) {
    $knownHostGateIds[$hostGate.id] = $true
}
$commandSurfaceIds = @{}
foreach ($commandSurface in $productionLoop.commandSurfaces) {
    Assert-Properties $commandSurface @("id", "schemaVersion", "status", "owner", "summary", "requestModes", "requestShape", "resultShape", "requiredModules", "capabilityGates", "hostGates", "validationRecipes", "unsupportedGapIds", "undoToken", "notes") "engine manifest aiOperableProductionLoop commandSurfaces"
    Assert-ManifestCommandSurfaceHasNoLegacyTopLevelFields -CommandSurface $commandSurface -MessagePrefix "engine manifest aiOperableProductionLoop command surface"
    if ($commandSurface.schemaVersion -ne 1) {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' schemaVersion must be 1"
    }
    if ($commandSurfaceIds.ContainsKey($commandSurface.id)) {
        Write-Error "engine manifest aiOperableProductionLoop command surface id is duplicated: $($commandSurface.id)"
    }
    $commandSurfaceIds[$commandSurface.id] = $true
    if ($allowedProductionStatuses -notcontains $commandSurface.status) {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' has invalid status: $($commandSurface.status)"
    }
    $modeIds = @{}
    foreach ($mode in @($commandSurface.requestModes)) {
        Assert-Properties $mode @("id", "status", "mutates", "requiresDryRun", "notes") "engine manifest aiOperableProductionLoop command surface requestModes"
        $modeIds[$mode.id] = $mode
        if (@("dry-run", "apply", "execute") -notcontains $mode.id) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' has unknown request mode: $($mode.id)"
        }
        if ($allowedProductionStatuses -notcontains $mode.status) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' mode '$($mode.id)' has invalid status: $($mode.status)"
        }
    }
    if (-not $modeIds.ContainsKey("dry-run")) {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' must expose a dry-run request mode"
    }
    if ($modeIds.ContainsKey("apply") -and $modeIds["apply"].status -eq "ready" -and $modeIds["dry-run"].status -ne "ready") {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' cannot make apply ready before dry-run is ready"
    }
    if ($modeIds.ContainsKey("execute") -and $modeIds["execute"].status -eq "ready" -and $modeIds["dry-run"].status -ne "ready") {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' cannot make execute ready before dry-run is ready"
    }
    if ($modeIds.ContainsKey("apply") -and $modeIds["apply"].status -eq "ready" -and
        @("register-runtime-package-files", "update-ui-atlas-metadata-package", "create-material-instance", "create-material-from-graph", "update-scene-package", "migrate-scene-v2-runtime-package", "create-scene", "add-scene-node", "add-or-update-component", "create-prefab", "instantiate-prefab", "register-source-asset", "cook-registered-source-assets") -notcontains $commandSurface.id) {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' cannot make apply ready without a focused apply tooling slice"
    }
    if ($modeIds.ContainsKey("execute") -and $modeIds["execute"].status -eq "ready" -and
        @("run-validation-recipe", "validate-runtime-scene-package") -notcontains $commandSurface.id) {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' cannot make execute ready without a focused execution tooling slice"
    }
    Assert-Properties $commandSurface.requestShape @("schema", "requiredFields", "optionalFields", "pathPolicy", "nativeHandlePolicy") "engine manifest aiOperableProductionLoop command surface requestShape"
    if ($commandSurface.requestShape.nativeHandlePolicy -ne "forbidden") {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' requestShape must forbid native handles"
    }
    Assert-Properties $commandSurface.resultShape @("schema", "requiredFields", "diagnosticFields", "dryRunFields") "engine manifest aiOperableProductionLoop command surface resultShape"
    if (@("run-validation-recipe", "validate-runtime-scene-package") -contains $commandSurface.id) {
        Assert-Properties $commandSurface.resultShape @("executeFields") "engine manifest aiOperableProductionLoop run-validation-recipe resultShape"
    } else {
        Assert-Properties $commandSurface.resultShape @("applyFields") "engine manifest aiOperableProductionLoop command surface resultShape"
    }
    foreach ($requiredResultField in @("commandId", "mode", "status", "diagnostics", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($commandSurface.resultShape.requiredFields) -notcontains $requiredResultField) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' resultShape missing required result field: $requiredResultField"
        }
    }
    foreach ($diagnosticField in @("severity", "code", "message", "unsupportedGapId", "validationRecipe")) {
        if (@($commandSurface.resultShape.diagnosticFields) -notcontains $diagnosticField) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' resultShape missing diagnostic field: $diagnosticField"
        }
    }
    foreach ($module in @($commandSurface.requiredModules)) {
        if (-not $moduleNames.ContainsKey($module)) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown module: $module"
        }
    }
    foreach ($gate in @($commandSurface.capabilityGates)) {
        Assert-Properties $gate @("id", "source", "requiredStatus", "notes") "engine manifest aiOperableProductionLoop command surface capabilityGates"
        switch ($gate.source) {
            "module" { if (-not $moduleNames.ContainsKey($gate.id)) { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown module capability gate: $($gate.id)" } }
            "recipe" { if (-not $productionRecipeIds.ContainsKey($gate.id)) { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown recipe capability gate: $($gate.id)" } }
            "authoring-surface" { if (-not $knownAuthoringSurfaceIds.ContainsKey($gate.id)) { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown authoring surface capability gate: $($gate.id)" } }
            "package-surface" { if (-not $knownPackageSurfaceIds.ContainsKey($gate.id)) { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown package surface capability gate: $($gate.id)" } }
            "unsupported-gap" { if (-not $knownUnsupportedGapIds.ContainsKey($gate.id)) { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown unsupported gap capability gate: $($gate.id)" } }
            "host-gate" { if (-not $knownHostGateIds.ContainsKey($gate.id)) { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown host gate capability gate: $($gate.id)" } }
            "validation-recipe" { if (-not $validationRecipeNames.ContainsKey($gate.id)) { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown validation recipe capability gate: $($gate.id)" } }
            default { Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' has unknown capability gate source: $($gate.source)" }
        }
    }
    foreach ($hostGate in @($commandSurface.hostGates)) {
        if (-not $knownHostGateIds.ContainsKey($hostGate)) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown host gate: $hostGate"
        }
    }
    foreach ($validationRecipe in @($commandSurface.validationRecipes)) {
        if (-not $validationRecipeNames.ContainsKey($validationRecipe)) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown validation recipe: $validationRecipe"
        }
    }
    foreach ($gapId in @($commandSurface.unsupportedGapIds)) {
        if (-not $knownUnsupportedGapIds.ContainsKey($gapId)) {
            Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' references unknown unsupported gap: $gapId"
        }
    }
    if (@($commandSurface.unsupportedGapIds).Count -lt 1) {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' must list unsupportedGapIds for diagnostics"
    }
    Assert-Properties $commandSurface.undoToken @("status", "notes") "engine manifest aiOperableProductionLoop command surface undoToken"
    if ($commandSurface.undoToken.status -ne "placeholder-only") {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' undoToken must remain placeholder-only in this slice"
    }
    if ($modeIds.ContainsKey("apply") -and $modeIds["apply"].status -eq "ready" -and @($commandSurface.validationRecipes).Count -lt 1) {
        Write-Error "engine manifest aiOperableProductionLoop command surface '$($commandSurface.id)' must list validation recipes when apply=true"
    }
}
$runtimePackageCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "register-runtime-package-files" })
if ($runtimePackageCommand.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose exactly one register-runtime-package-files command surface"
} else {
    $runtimeModes = @{}
    foreach ($mode in @($runtimePackageCommand[0].requestModes)) {
        $runtimeModes[$mode.id] = $mode
    }
    if (-not $runtimeModes.ContainsKey("dry-run") -or $runtimeModes["dry-run"].status -ne "ready" -or
        -not $runtimeModes.ContainsKey("apply") -or $runtimeModes["apply"].status -ne "ready") {
        Write-Error "engine manifest aiOperableProductionLoop register-runtime-package-files must keep dry-run and apply ready"
    }
    if (-not ([string]$runtimeModes["dry-run"].notes).Contains("-DryRun")) {
        Write-Error "engine manifest aiOperableProductionLoop register-runtime-package-files dry-run notes must reference the actual -DryRun switch"
    }
}
$uiAtlasPackageCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "update-ui-atlas-metadata-package" })
if ($uiAtlasPackageCommand.Count -ne 1 -or $uiAtlasPackageCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready update-ui-atlas-metadata-package command surface"
} else {
    $uiAtlasModes = @{}
    foreach ($mode in @($uiAtlasPackageCommand[0].requestModes)) {
        $uiAtlasModes[$mode.id] = $mode
    }
    if (-not $uiAtlasModes.ContainsKey("dry-run") -or $uiAtlasModes["dry-run"].status -ne "ready" -or
        -not $uiAtlasModes.ContainsKey("apply") -or $uiAtlasModes["apply"].status -ne "ready") {
        Write-Error "engine manifest update-ui-atlas-metadata-package must keep dry-run and apply ready"
    }
    $uiAtlasNotes = [string]$uiAtlasPackageCommand[0].notes
    if (-not $uiAtlasNotes.Contains("plan_cooked_ui_atlas_package_update") -or
        -not $uiAtlasNotes.Contains("apply_cooked_ui_atlas_package_update") -or
        -not $uiAtlasNotes.Contains("author_packed_ui_atlas_from_decoded_images") -or
        -not $uiAtlasNotes.Contains("author_packed_ui_glyph_atlas_from_rasterized_glyphs") -or
        -not $uiAtlasNotes.Contains("plan_packed_ui_glyph_atlas_package_update") -or
        -not $uiAtlasNotes.Contains("GameEngine.CookedTexture.v1") -or
        -not $uiAtlasNotes.Contains("renderer texture upload")) {
        Write-Error "engine manifest update-ui-atlas-metadata-package notes must keep cooked helper names, decoded/glyph atlas bridges, and renderer-upload limits explicit"
    }
}
$materialInstanceCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "create-material-instance" })
if ($materialInstanceCommand.Count -ne 1 -or $materialInstanceCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready create-material-instance command surface"
} else {
    $materialModes = @{}
    foreach ($mode in @($materialInstanceCommand[0].requestModes)) {
        $materialModes[$mode.id] = $mode
    }
    if (-not $materialModes.ContainsKey("dry-run") -or $materialModes["dry-run"].status -ne "ready" -or
        -not $materialModes.ContainsKey("apply") -or $materialModes["apply"].status -ne "ready") {
        Write-Error "engine manifest create-material-instance must keep dry-run and apply ready"
    }
    $materialNotes = [string]$materialInstanceCommand[0].notes
    if (-not $materialNotes.Contains("plan_material_instance_package_update") -or
        -not $materialNotes.Contains("apply_material_instance_package_update") -or
        -not $materialNotes.Contains("material graph") -or
        -not $materialNotes.Contains("shader graph") -or
        -not $materialNotes.Contains("live shader generation")) {
        Write-Error "engine manifest create-material-instance notes must keep helper names and unsupported graph/shader limits explicit"
    }
}
$materialGraphCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "create-material-from-graph" })
if ($materialGraphCommand.Count -ne 1 -or $materialGraphCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready create-material-from-graph command surface"
} else {
    $materialGraphModes = @{}
    foreach ($mode in @($materialGraphCommand[0].requestModes)) {
        $materialGraphModes[$mode.id] = $mode
    }
    if (-not $materialGraphModes.ContainsKey("dry-run") -or $materialGraphModes["dry-run"].status -ne "ready" -or
        -not $materialGraphModes.ContainsKey("apply") -or $materialGraphModes["apply"].status -ne "ready") {
        Write-Error "engine manifest create-material-from-graph must keep dry-run and apply ready"
    }
    $materialGraphNotes = [string]$materialGraphCommand[0].notes
    foreach ($needle in @(
            "plan_material_graph_package_update",
            "apply_material_graph_package_update",
            "GameEngine.MaterialGraph.v1",
            "GameEngine.Material.v1",
            "material_texture",
            "shader graph",
            "shader compiler execution",
            "live shader generation",
            "renderer/RHI residency",
            "package streaming"
        )) {
        if (-not $materialGraphNotes.Contains($needle)) {
            Write-Error "engine manifest create-material-from-graph notes must keep helper names and unsupported graph/package limits explicit: $needle"
        }
    }
}
$scenePackageCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "update-scene-package" })
if ($scenePackageCommand.Count -ne 1 -or $scenePackageCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready update-scene-package command surface"
} else {
    $sceneModes = @{}
    foreach ($mode in @($scenePackageCommand[0].requestModes)) {
        $sceneModes[$mode.id] = $mode
    }
    if (-not $sceneModes.ContainsKey("dry-run") -or $sceneModes["dry-run"].status -ne "ready" -or
        -not $sceneModes.ContainsKey("apply") -or $sceneModes["apply"].status -ne "ready") {
        Write-Error "engine manifest update-scene-package must keep dry-run and apply ready"
    }
    $sceneNotes = [string]$scenePackageCommand[0].notes
    foreach ($needle in @(
            "plan_scene_package_update",
            "apply_scene_package_update",
            "scene_mesh",
            "scene_material",
            "scene_sprite",
            "editor productization",
            "prefab mutation",
            "runtime source import",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation"
        )) {
        if (-not $sceneNotes.Contains($needle)) {
            Write-Error "engine manifest update-scene-package notes must keep helper names and unsupported scene/package limits explicit: $needle"
        }
    }
}
$scenePrefabAuthoringCommandIds = @(
    "create-scene",
    "add-scene-node",
    "add-or-update-component",
    "create-prefab",
    "instantiate-prefab"
)
foreach ($commandId in $scenePrefabAuthoringCommandIds) {
    $scenePrefabCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq $commandId })
    if ($scenePrefabCommand.Count -ne 1 -or $scenePrefabCommand[0].status -ne "ready") {
        Write-Error "engine manifest aiOperableProductionLoop must expose one ready Scene/Prefab v2 authoring command surface: $commandId"
    } else {
        $scenePrefabModes = @{}
        foreach ($mode in @($scenePrefabCommand[0].requestModes)) {
            $scenePrefabModes[$mode.id] = $mode
        }
        if (-not $scenePrefabModes.ContainsKey("dry-run") -or $scenePrefabModes["dry-run"].status -ne "ready" -or
            -not $scenePrefabModes.ContainsKey("apply") -or $scenePrefabModes["apply"].status -ne "ready") {
            Write-Error "engine manifest Scene/Prefab v2 authoring command '$commandId' must keep dry-run and apply ready"
        }
        foreach ($module in @("MK_scene", "MK_tools")) {
            if (@($scenePrefabCommand[0].requiredModules) -notcontains $module) {
                Write-Error "engine manifest Scene/Prefab v2 authoring command '$commandId' missing required module: $module"
            }
        }
        foreach ($field in @("changedFiles", "modelMutations", "validationRecipes", "unsupportedGapIds", "undoToken")) {
            if (@($scenePrefabCommand[0].resultShape.dryRunFields) -notcontains $field) {
                Write-Error "engine manifest Scene/Prefab v2 authoring command '$commandId' dryRunFields missing: $field"
            }
        }
        foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
            if (@($scenePrefabCommand[0].resultShape.applyFields) -notcontains $field) {
                Write-Error "engine manifest Scene/Prefab v2 authoring command '$commandId' applyFields missing: $field"
            }
        }
        $scenePrefabPolicyText = "$($scenePrefabCommand[0].summary) $($scenePrefabCommand[0].requestShape.pathPolicy) $($scenePrefabCommand[0].notes)"
        foreach ($needle in @(
                "GameEngine.Scene.v2",
                "GameEngine.Prefab.v2",
                "plan_scene_prefab_authoring",
                "apply_scene_prefab_authoring",
                "safe repository-relative",
                "does not evaluate arbitrary shell",
                "free-form edits are not supported",
                "Scene v2 runtime package migration",
                "editor productization",
                "nested prefab merge/resolution UX"
            )) {
            if (-not $scenePrefabPolicyText.Contains($needle)) {
                Write-Error "engine manifest Scene/Prefab v2 authoring command '$commandId' must document reviewed helper/policy text: $needle"
            }
        }
    }
}
$sourceAssetCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "register-source-asset" })
if ($sourceAssetCommand.Count -ne 1 -or $sourceAssetCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready register-source-asset command surface"
} else {
    $sourceAssetModes = @{}
    foreach ($mode in @($sourceAssetCommand[0].requestModes)) {
        $sourceAssetModes[$mode.id] = $mode
    }
    if (-not $sourceAssetModes.ContainsKey("dry-run") -or $sourceAssetModes["dry-run"].status -ne "ready" -or
        -not $sourceAssetModes.ContainsKey("apply") -or $sourceAssetModes["apply"].status -ne "ready") {
        Write-Error "engine manifest register-source-asset must keep dry-run and apply ready"
    }
    foreach ($module in @("MK_assets", "MK_tools")) {
        if (@($sourceAssetCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest register-source-asset missing required module: $module"
        }
    }
    foreach ($field in @("changedFiles", "modelMutations", "importMetadata", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($sourceAssetCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine manifest register-source-asset dryRunFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("cookedOutputHint", "packageIndexPath", "backend", "shaderArtifactRequirements", "renderer", "rhi", "metal", "descriptor", "pipeline", "nativeHandle")) {
        if (@($sourceAssetCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($sourceAssetCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine manifest register-source-asset must not expose package/renderer/native field: $forbiddenField"
        }
    }
    foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
        if (@($sourceAssetCommand[0].resultShape.applyFields) -notcontains $field) {
            Write-Error "engine manifest register-source-asset applyFields missing: $field"
        }
    }
    $sourceAssetPolicyText = "$($sourceAssetCommand[0].summary) $($sourceAssetCommand[0].requestShape.pathPolicy) $($sourceAssetCommand[0].notes)"
    foreach ($needle in @(
            "GameEngine.AssetIdentity.v2",
            "GameEngine.SourceAssetRegistry.v1",
            "plan_source_asset_registration",
            "apply_source_asset_registration",
            "safe repository-relative",
            "does not evaluate arbitrary shell",
            "free-form edits are not supported",
            "external importer execution is not supported",
            "does not write cooked artifacts",
            "does not update .geindex",
            "package cooking",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles"
        )) {
        if (-not $sourceAssetPolicyText.Contains($needle)) {
            Write-Error "engine manifest register-source-asset must document reviewed helper/policy text: $needle"
        }
    }
    $sourceAssetHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/source_asset_registration_tool.hpp"
    $sourceAssetSourcePath = Join-Path $root "engine/tools/asset/source_asset_registration_tool.cpp"
    foreach ($requiredPath in @($sourceAssetHeaderPath, $sourceAssetSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "register-source-asset reviewed helper file is missing: $requiredPath"
        }
    }
    foreach ($helperPath in @($sourceAssetHeaderPath, $sourceAssetSourcePath)) {
        if (Test-Path -LiteralPath $helperPath -PathType Leaf) {
            $helperText = Get-Content -LiteralPath $helperPath -Raw
            foreach ($forbiddenText in @(
                "mirakana/tools/asset_import_tool.hpp",
                "mirakana/tools/asset_package_tool.hpp",
                "mirakana/assets/asset_package.hpp",
                "mirakana/renderer/",
                "mirakana/rhi/",
                "execute_asset_import_plan",
                "assemble_asset_cooked_package",
                "write_asset_cooked_package_index"
            )) {
                if ($helperText.Contains($forbiddenText)) {
                    Write-Error "$helperPath must not use package/import execution or renderer/RHI surface: $forbiddenText"
                }
            }
        }
    }
}
$registeredCookCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "cook-registered-source-assets" })
if ($registeredCookCommand.Count -ne 1 -or $registeredCookCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready cook-registered-source-assets command surface"
} else {
    $registeredCookModes = @{}
    foreach ($mode in @($registeredCookCommand[0].requestModes)) {
        $registeredCookModes[$mode.id] = $mode
    }
    if (-not $registeredCookModes.ContainsKey("dry-run") -or $registeredCookModes["dry-run"].status -ne "ready" -or
        -not $registeredCookModes.ContainsKey("apply") -or $registeredCookModes["apply"].status -ne "ready") {
        Write-Error "engine manifest cook-registered-source-assets must keep dry-run and apply ready"
    }
    foreach ($module in @("MK_assets", "MK_tools")) {
        if (@($registeredCookCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest cook-registered-source-assets missing required module: $module"
        }
    }
    foreach ($field in @("changedFiles", "modelMutations", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($registeredCookCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine manifest cook-registered-source-assets dryRunFields missing: $field"
        }
    }
    foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
        if (@($registeredCookCommand[0].resultShape.applyFields) -notcontains $field) {
            Write-Error "engine manifest cook-registered-source-assets applyFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("backend", "nativeHandle", "rhiHandle", "rendererBackend", "metalDevice")) {
        if (@($registeredCookCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($registeredCookCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine manifest cook-registered-source-assets must not expose renderer/native handle field: $forbiddenField"
        }
    }
    $registeredCookPolicyText = "$($registeredCookCommand[0].summary) $($registeredCookCommand[0].requestShape.pathPolicy) $($registeredCookCommand[0].notes)"
    foreach ($needle in @(
            "GameEngine.SourceAssetRegistry.v1",
            "explicitly selected",
            "plan_registered_source_asset_cook_package",
            "apply_registered_source_asset_cook_package",
            "build_asset_import_plan",
            "execute_asset_import_plan",
            "assemble_asset_cooked_package",
            "safe repository-relative",
            "package-relative",
            "external importer execution is not supported",
            "broad dependency cooking",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles",
            "general production renderer quality",
            "free-form edits are not supported"
        )) {
        if (-not $registeredCookPolicyText.Contains($needle)) {
            Write-Error "engine manifest cook-registered-source-assets must document reviewed helper/policy text: $needle"
        }
    }
    $registeredCookHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/registered_source_asset_cook_package_tool.hpp"
    $registeredCookSourcePath = Join-Path $root "engine/tools/asset/registered_source_asset_cook_package_tool.cpp"
    foreach ($requiredPath in @($registeredCookHeaderPath, $registeredCookSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "cook-registered-source-assets reviewed helper file is missing: $requiredPath"
        }
    }
    if (Test-Path -LiteralPath $registeredCookSourcePath -PathType Leaf) {
        $helperText = Get-Content -LiteralPath $registeredCookSourcePath -Raw
        foreach ($requiredText in @(
            "mirakana/tools/asset_import_tool.hpp",
            "mirakana/tools/asset_package_tool.hpp",
            "build_asset_import_plan",
            "execute_asset_import_plan",
            "assemble_asset_cooked_package"
        )) {
            if (-not $helperText.Contains($requiredText)) {
                Write-Error "$registeredCookSourcePath must reuse selected source asset import/package helper: $requiredText"
            }
        }
    }
    foreach ($helperPath in @($registeredCookHeaderPath, $registeredCookSourcePath)) {
        if (Test-Path -LiteralPath $helperPath -PathType Leaf) {
            $helperText = Get-Content -LiteralPath $helperPath -Raw
            foreach ($forbiddenText in @(
                "mirakana/renderer/",
                "mirakana/rhi/",
                "IRhiDevice",
                "ID3D12",
                "VkDevice",
                "MTLDevice"
            )) {
                if ($helperText.Contains($forbiddenText)) {
                    Write-Error "$helperPath must not use renderer/RHI/native surfaces: $forbiddenText"
                }
            }
        }
    }
}
$sceneMigrationCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "migrate-scene-v2-runtime-package" })
if ($sceneMigrationCommand.Count -ne 1 -or $sceneMigrationCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready migrate-scene-v2-runtime-package command surface"
} else {
    $sceneMigrationModes = @{}
    foreach ($mode in @($sceneMigrationCommand[0].requestModes)) {
        $sceneMigrationModes[$mode.id] = $mode
    }
    if (-not $sceneMigrationModes.ContainsKey("dry-run") -or $sceneMigrationModes["dry-run"].status -ne "ready" -or
        -not $sceneMigrationModes.ContainsKey("apply") -or $sceneMigrationModes["apply"].status -ne "ready") {
        Write-Error "engine manifest migrate-scene-v2-runtime-package must keep dry-run and apply ready"
    }
    foreach ($module in @("MK_scene", "MK_assets", "MK_tools")) {
        if (@($sceneMigrationCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest migrate-scene-v2-runtime-package missing required module: $module"
        }
    }
    foreach ($field in @("changedFiles", "modelMutations", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($sceneMigrationCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine manifest migrate-scene-v2-runtime-package dryRunFields missing: $field"
        }
    }
    foreach ($field in @("changedFiles", "validationRecipes", "undoToken")) {
        if (@($sceneMigrationCommand[0].resultShape.applyFields) -notcontains $field) {
            Write-Error "engine manifest migrate-scene-v2-runtime-package applyFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("backend", "nativeHandle", "rhiHandle", "rendererBackend", "metalDevice")) {
        if (@($sceneMigrationCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($sceneMigrationCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine manifest migrate-scene-v2-runtime-package must not expose renderer/native handle field: $forbiddenField"
        }
    }
    $sceneMigrationPolicyText = "$($sceneMigrationCommand[0].summary) $($sceneMigrationCommand[0].requestShape.pathPolicy) $($sceneMigrationCommand[0].notes)"
    foreach ($needle in @(
            "GameEngine.Scene.v2",
            "GameEngine.SourceAssetRegistry.v1",
            "GameEngine.Scene.v1",
            "plan_scene_v2_runtime_package_migration",
            "apply_scene_v2_runtime_package_migration",
            "plan_scene_package_update",
            "apply_scene_package_update",
            "safe repository-relative",
            "package-relative",
            "external importer execution is not supported",
            "package cooking",
            "dependent asset cooking",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles",
            "general production renderer quality",
            "free-form edits are not supported"
        )) {
        if (-not $sceneMigrationPolicyText.Contains($needle)) {
            Write-Error "engine manifest migrate-scene-v2-runtime-package must document reviewed helper/policy text: $needle"
        }
    }
    $sceneMigrationHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/scene_v2_runtime_package_migration_tool.hpp"
    $sceneMigrationSourcePath = Join-Path $root "engine/tools/scene/scene_v2_runtime_package_migration_tool.cpp"
    foreach ($requiredPath in @($sceneMigrationHeaderPath, $sceneMigrationSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "migrate-scene-v2-runtime-package reviewed helper file is missing: $requiredPath"
        }
    }
    foreach ($helperPath in @($sceneMigrationHeaderPath, $sceneMigrationSourcePath)) {
        if (Test-Path -LiteralPath $helperPath -PathType Leaf) {
            $helperText = Get-Content -LiteralPath $helperPath -Raw
            foreach ($forbiddenText in @(
                "mirakana/tools/asset_import_tool.hpp",
                "mirakana/tools/asset_package_tool.hpp",
                "mirakana/renderer/",
                "mirakana/rhi/",
                "execute_asset_import_plan",
                "assemble_asset_cooked_package",
                "write_asset_cooked_package_index",
                "IRhiDevice",
                "ID3D12",
                "VkDevice",
                "MTLDevice"
            )) {
                if ($helperText.Contains($forbiddenText)) {
                    Write-Error "$helperPath must not use importer/package execution or renderer/RHI/native surfaces: $forbiddenText"
                }
            }
        }
    }
    if (Test-Path -LiteralPath $sceneMigrationSourcePath -PathType Leaf) {
        $sceneMigrationSourceText = Get-Content -LiteralPath $sceneMigrationSourcePath -Raw
        foreach ($requiredCall in @(
                "plan_scene_package_update(",
                "apply_scene_package_update("
            )) {
            if (-not $sceneMigrationSourceText.Contains($requiredCall)) {
                Write-Error "migrate-scene-v2-runtime-package source must reuse existing scene package helper call: $requiredCall"
            }
        }
    }
}
$runtimeSceneValidationCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "validate-runtime-scene-package" })
if ($runtimeSceneValidationCommand.Count -ne 1 -or $runtimeSceneValidationCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready validate-runtime-scene-package command surface"
} else {
    $runtimeSceneValidationModes = @{}
    foreach ($mode in @($runtimeSceneValidationCommand[0].requestModes)) {
        $runtimeSceneValidationModes[$mode.id] = $mode
    }
    if (-not $runtimeSceneValidationModes.ContainsKey("dry-run") -or
        $runtimeSceneValidationModes["dry-run"].status -ne "ready" -or
        -not $runtimeSceneValidationModes.ContainsKey("execute") -or
        $runtimeSceneValidationModes["execute"].status -ne "ready") {
        Write-Error "engine manifest validate-runtime-scene-package must keep dry-run and execute ready"
    }
    if ($runtimeSceneValidationModes.ContainsKey("apply") -and $runtimeSceneValidationModes["apply"].status -eq "ready") {
        Write-Error "engine manifest validate-runtime-scene-package must remain non-mutating and must not expose ready apply"
    }
    foreach ($module in @("MK_runtime", "MK_runtime_scene", "MK_tools")) {
        if (@($runtimeSceneValidationCommand[0].requiredModules) -notcontains $module) {
            Write-Error "engine manifest validate-runtime-scene-package missing required module: $module"
        }
    }
    foreach ($field in @("packageIndexPath", "sceneAssetKey")) {
        if (@($runtimeSceneValidationCommand[0].requestShape.requiredFields) -notcontains $field) {
            Write-Error "engine manifest validate-runtime-scene-package requestShape requiredFields missing: $field"
        }
    }
    foreach ($field in @("contentRoot", "validateAssetReferences", "requireUniqueNodeNames")) {
        if (@($runtimeSceneValidationCommand[0].requestShape.optionalFields) -notcontains $field) {
            Write-Error "engine manifest validate-runtime-scene-package requestShape optionalFields missing: $field"
        }
    }
    foreach ($field in @("packageSummary", "sceneAsset", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($runtimeSceneValidationCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine manifest validate-runtime-scene-package dryRunFields missing: $field"
        }
    }
    foreach ($field in @("packageSummary", "sceneSummary", "references", "packageRecordCount", "validationRecipes", "unsupportedGapIds", "undoToken")) {
        if (@($runtimeSceneValidationCommand[0].resultShape.executeFields) -notcontains $field) {
            Write-Error "engine manifest validate-runtime-scene-package executeFields missing: $field"
        }
    }
    foreach ($forbiddenField in @("backend", "nativeHandle", "rhiHandle", "rendererBackend", "metalDevice", "shaderArtifactRequirements")) {
        if (@($runtimeSceneValidationCommand[0].requestShape.optionalFields) -contains $forbiddenField -or
            @($runtimeSceneValidationCommand[0].requestShape.requiredFields) -contains $forbiddenField) {
            Write-Error "engine manifest validate-runtime-scene-package must not expose renderer/native handle field: $forbiddenField"
        }
    }
    $runtimeSceneValidationPolicyText = "$($runtimeSceneValidationCommand[0].summary) $($runtimeSceneValidationCommand[0].requestShape.pathPolicy) $($runtimeSceneValidationCommand[0].notes)"
    foreach ($needle in @(
            "plan_runtime_scene_package_validation",
            "execute_runtime_scene_package_validation",
            "mirakana::runtime::load_runtime_asset_package",
            "mirakana::runtime_scene::instantiate_runtime_scene",
            "safe package-relative",
            "does not mutate",
            "runtime source parsing is not supported",
            "package cooking is not supported",
            "renderer/RHI residency",
            "package streaming",
            "material graph",
            "shader graph",
            "live shader generation",
            "editor productization",
            "Metal readiness",
            "public native/RHI handles",
            "general production renderer quality",
            "free-form edits are not supported"
        )) {
        if (-not $runtimeSceneValidationPolicyText.Contains($needle)) {
            Write-Error "engine manifest validate-runtime-scene-package must document reviewed helper/policy text: $needle"
        }
    }
    $runtimeSceneValidationHeaderPath = Join-Path $root "engine/tools/include/mirakana/tools/runtime_scene_package_validation_tool.hpp"
    $runtimeSceneValidationSourcePath = Join-Path $root "engine/tools/scene/runtime_scene_package_validation_tool.cpp"
    foreach ($requiredPath in @($runtimeSceneValidationHeaderPath, $runtimeSceneValidationSourcePath)) {
        if (-not (Test-Path -LiteralPath $requiredPath -PathType Leaf)) {
            Write-Error "validate-runtime-scene-package reviewed helper file is missing: $requiredPath"
        }
    }
    if (Test-Path -LiteralPath $runtimeSceneValidationSourcePath -PathType Leaf) {
        $helperText = Get-Content -LiteralPath $runtimeSceneValidationSourcePath -Raw
        foreach ($requiredText in @(
            "mirakana/runtime/asset_runtime.hpp",
            "mirakana/runtime_scene/runtime_scene.hpp",
            "load_runtime_asset_package",
            "instantiate_runtime_scene"
        )) {
            if (-not $helperText.Contains($requiredText)) {
                Write-Error "$runtimeSceneValidationSourcePath must reuse runtime package/scene validation helper: $requiredText"
            }
        }
        foreach ($forbiddenText in @(
            "mirakana/tools/asset_import_tool.hpp",
            "mirakana/tools/asset_package_tool.hpp",
            "mirakana/renderer/",
            "mirakana/rhi/",
            "execute_asset_import_plan",
            "assemble_asset_cooked_package",
            "write_text(",
            "IRhiDevice",
            "ID3D12",
            "VkDevice",
            "MTLDevice"
        )) {
            if ($helperText.Contains($forbiddenText)) {
                Write-Error "$runtimeSceneValidationSourcePath must not mutate, import, package, or use renderer/RHI/native surfaces: $forbiddenText"
            }
        }
    }
}
$validationRunnerCommand = @($productionLoop.commandSurfaces | Where-Object { $_.id -eq "run-validation-recipe" })
if ($validationRunnerCommand.Count -ne 1 -or $validationRunnerCommand[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop must expose one ready run-validation-recipe command surface"
} else {
    $runnerModes = @{}
    foreach ($mode in @($validationRunnerCommand[0].requestModes)) {
        $runnerModes[$mode.id] = $mode
    }
    if (-not $runnerModes.ContainsKey("dry-run") -or $runnerModes["dry-run"].status -ne "ready" -or
        -not $runnerModes.ContainsKey("execute") -or $runnerModes["execute"].status -ne "ready") {
        Write-Error "engine manifest run-validation-recipe must keep dry-run and execute ready"
    }
    foreach ($requiredField in @("mode", "validationRecipe")) {
        if (@($validationRunnerCommand[0].requestShape.requiredFields) -notcontains $requiredField) {
            Write-Error "engine manifest run-validation-recipe requestShape missing required field: $requiredField"
        }
    }
    foreach ($optionalField in @("gameTarget", "strictBackend", "hostGateAcknowledgements", "timeoutSeconds")) {
        if (@($validationRunnerCommand[0].requestShape.optionalFields) -notcontains $optionalField) {
            Write-Error "engine manifest run-validation-recipe requestShape missing optional field: $optionalField"
        }
    }
    $runnerPolicyText = "$($validationRunnerCommand[0].requestShape.pathPolicy) $($validationRunnerCommand[0].notes)"
    foreach ($needle in @(
            "tools/run-validation-recipe.ps1",
            "Get-ValidationRecipeCommandPlan",
            "Invoke-ValidationRecipeCommandPlan",
            "does not evaluate raw manifest command strings",
            "free-form arguments are rejected",
            "not arbitrary shell"
        )) {
        if (-not $runnerPolicyText.Contains($needle)) {
            Write-Error "engine manifest run-validation-recipe must document runner path/helper/policy text: $needle"
        }
    }
    foreach ($field in @("recipe", "status", "command", "argv", "hostGates", "diagnostics", "blockedBy")) {
        if (@($validationRunnerCommand[0].resultShape.dryRunFields) -notcontains $field) {
            Write-Error "engine manifest run-validation-recipe dryRunFields missing: $field"
        }
    }
    foreach ($field in @("recipe", "status", "exitCode", "durationSeconds", "stdoutSummary", "stderrSummary", "hostGates", "diagnostics")) {
        if (@($validationRunnerCommand[0].resultShape.executeFields) -notcontains $field) {
            Write-Error "engine manifest run-validation-recipe executeFields missing: $field"
        }
    }
    foreach ($recipe in @(
            "agent-contract",
            "default",
            "public-api-boundary",
            "shader-toolchain",
            "desktop-game-runtime",
            "desktop-runtime-sample-game-scene-gpu-package",
            "desktop-runtime-generated-material-shader-scaffold-package",
            "desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict",
            "desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package",
            "dev-windows-editor-game-module-driver-load-tests"
        )) {
        if (@($validationRunnerCommand[0].validationRecipes) -notcontains $recipe) {
            Write-Error "engine manifest run-validation-recipe validationRecipes missing allowlisted recipe: $recipe"
        }
    }
    if (@($validationRunnerCommand[0].validationRecipes).Count -ne 10) {
        Write-Error "engine manifest run-validation-recipe validationRecipes must be exactly the reviewed allowlist"
    }
    if (@($validationRunnerCommand[0].requestModes | Where-Object { $_.id -eq "apply" -and $_.status -eq "ready" }).Count -gt 0) {
        Write-Error "engine manifest run-validation-recipe must not expose a ready apply mode"
    }
    $runnerScriptPath = Join-Path $root "tools/run-validation-recipe.ps1"
    if (-not (Test-Path -LiteralPath $runnerScriptPath -PathType Leaf)) {
        Write-Error "engine manifest run-validation-recipe references missing tools/run-validation-recipe.ps1"
    }
    $runnerText = Get-Content -LiteralPath $runnerScriptPath -Raw
    foreach ($needle in @("Get-ValidationRecipeCommandPlan", "Invoke-ValidationRecipeCommandPlan")) {
        if (-not $runnerText.Contains($needle)) {
            Write-Error "tools/run-validation-recipe.ps1 missing reviewed helper name: $needle"
        }
    }
    foreach ($forbiddenRunnerPattern in @(
        "\bInvoke-Expression\b",
        "\biex\b",
        "\[scriptblock\]::Create",
        "\bcmd\s*/c\b",
        "\bbash\s+-c\b",
        "\bpwsh\b[^\r\n]*-Command\b",
        "\bpowershell\b[^\r\n]*-Command\b"
    )) {
        if ([System.Text.RegularExpressions.Regex]::IsMatch($runnerText, $forbiddenRunnerPattern, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)) {
            Write-Error "tools/run-validation-recipe.ps1 must not contain shell-eval pattern: $forbiddenRunnerPattern"
        }
    }
}
foreach ($commandId in $expectedCommandSurfaceIds) {
    if (-not $commandSurfaceIds.ContainsKey($commandId)) {
        Write-Error "engine manifest aiOperableProductionLoop missing command surface id: $commandId"
    }
}

$aiGameDevelopmentText = Get-Content -LiteralPath (Join-Path $root "docs/ai-game-development.md") -Raw
$aiIntegrationText = Get-Content -LiteralPath (Join-Path $root "docs/ai-integration.md") -Raw
$generatedScenariosText = Get-Content -LiteralPath (Join-Path $root "docs/specs/generated-game-validation-scenarios.md") -Raw
$promptPackText = Get-Content -LiteralPath (Join-Path $root "docs/specs/game-prompt-pack.md") -Raw
$handoffPromptText = Get-Content -LiteralPath (Join-Path $root "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md") -Raw
$roadmapText = Get-Content -LiteralPath (Join-Path $root "docs/roadmap.md") -Raw
$authoredRuntimeWorkflowRequiredText = @(
    "validated authored-to-runtime workflow",
    "register-source-asset -> cook-registered-source-assets -> migrate-scene-v2-runtime-package -> mirakana::runtime::load_runtime_asset_package -> mirakana::runtime_scene::instantiate_runtime_scene"
)
foreach ($workflowDoc in @(
    @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
    @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
    @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
    @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" }
)) {
    foreach ($requiredText in $authoredRuntimeWorkflowRequiredText) {
        if (-not $workflowDoc.Text.Contains($requiredText)) {
            Write-Error "$($workflowDoc.Label) did not contain expected text: $requiredText"
        }
    }
}
$runtimeScenePackageValidationRequiredText = @(
    "validate-runtime-scene-package",
    "plan_runtime_scene_package_validation",
    "execute_runtime_scene_package_validation",
    "non-mutating runtime scene package validation"
)
foreach ($runtimeSceneValidationDoc in @(
    @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
    @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
    @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
    @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
    @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
    @{ Text = $roadmapText; Label = "docs/roadmap.md" }
)) {
    foreach ($requiredText in $runtimeScenePackageValidationRequiredText) {
        if (-not $runtimeSceneValidationDoc.Text.Contains($requiredText)) {
            Write-Error "$($runtimeSceneValidationDoc.Label) did not contain expected text: $requiredText"
        }
    }
}
foreach ($forbiddenScenePrefabAuthoringClaim in @(
    "Scene/Prefab v2 authoring makes Scene v2 runtime package migration ready",
    "Scene/Prefab v2 authoring alone makes Scene v2 package migration ready",
    "editor productization is ready",
    "nested prefab merge/resolution UX is ready",
    "arbitrary free-form scene edits are supported",
    "arbitrary free-form prefab edits are supported",
    "Scene/Prefab v2 authoring runs arbitrary shell"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenScenePrefabAuthoringClaim)) {
            Write-Error "$($doc.Label) contains forbidden Scene/Prefab v2 authoring claim: $forbiddenScenePrefabAuthoringClaim"
        }
    }
}
foreach ($forbiddenSceneMigrationClaim in @(
    "migrate-scene-v2-runtime-package executes external importers",
    "Scene v2 runtime package migration executes external importers",
    "migrate-scene-v2-runtime-package cooks dependent assets",
    "Scene v2 runtime package migration cooks dependent assets",
    "migrate-scene-v2-runtime-package performs broad package cooking",
    "Scene v2 runtime package migration performs broad package cooking",
    "migrate-scene-v2-runtime-package makes renderer/RHI residency ready",
    "Scene v2 runtime package migration makes renderer/RHI residency ready",
    "migrate-scene-v2-runtime-package makes package streaming ready",
    "Scene v2 runtime package migration makes package streaming ready",
    "migrate-scene-v2-runtime-package supports material graphs",
    "Scene v2 runtime package migration supports material graphs",
    "migrate-scene-v2-runtime-package supports shader graphs",
    "Scene v2 runtime package migration supports shader graphs",
    "migrate-scene-v2-runtime-package supports live shader generation",
    "Scene v2 runtime package migration supports live shader generation",
    "migrate-scene-v2-runtime-package makes editor productization ready",
    "Scene v2 runtime package migration makes editor productization ready",
    "migrate-scene-v2-runtime-package makes Metal ready",
    "Scene v2 runtime package migration makes Metal ready",
    "migrate-scene-v2-runtime-package exposes public native/RHI handles",
    "Scene v2 runtime package migration exposes public native/RHI handles",
    "migrate-scene-v2-runtime-package makes general production renderer quality ready",
    "Scene v2 runtime package migration makes general production renderer quality ready"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenSceneMigrationClaim)) {
            Write-Error "$($doc.Label) contains forbidden Scene v2 runtime package migration claim: $forbiddenSceneMigrationClaim"
        }
    }
}
foreach ($forbiddenSourceAssetRegistrationClaim in @(
    "source asset registration executes external importers",
    "register-source-asset executes external importers",
    "source asset registration cooks runtime packages",
    "register-source-asset cooks runtime packages",
    "source asset registration makes renderer/RHI residency ready",
    "source asset registration makes package streaming ready",
    "source asset registration supports material graphs",
    "source asset registration supports shader graphs",
    "source asset registration supports live shader generation",
    "source asset registration makes editor productization ready"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenSourceAssetRegistrationClaim)) {
            Write-Error "$($doc.Label) contains forbidden source asset registration claim: $forbiddenSourceAssetRegistrationClaim"
        }
    }
}
foreach ($forbiddenRegisteredCookClaim in @(
    "cook-registered-source-assets performs broad dependency cooking",
    "cook-registered-source-assets cooks unselected dependencies",
    "cook-registered-source-assets makes renderer/RHI residency ready",
    "cook-registered-source-assets makes package streaming ready",
    "cook-registered-source-assets supports material graphs",
    "cook-registered-source-assets supports shader graphs",
    "cook-registered-source-assets supports live shader generation",
    "cook-registered-source-assets makes editor productization ready",
    "cook-registered-source-assets makes Metal ready",
    "cook-registered-source-assets exposes public native/RHI handles",
    "cook-registered-source-assets makes general production renderer quality ready",
    "cook-registered-source-assets executes arbitrary shell",
    "cook-registered-source-assets supports free-form edits"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenRegisteredCookClaim)) {
            Write-Error "$($doc.Label) contains forbidden registered source asset cook/package claim: $forbiddenRegisteredCookClaim"
        }
    }
}
foreach ($forbiddenAuthoredRuntimeWorkflowClaim in @(
    "authored-to-runtime workflow performs broad package cooking",
    "authored-to-runtime workflow cooks unselected dependencies",
    "authored-to-runtime workflow parses source assets at runtime",
    "authored-to-runtime workflow makes renderer/RHI residency ready",
    "authored-to-runtime workflow makes package streaming ready",
    "authored-to-runtime workflow supports material graphs",
    "authored-to-runtime workflow supports shader graphs",
    "authored-to-runtime workflow supports live shader generation",
    "authored-to-runtime workflow makes editor productization ready",
    "authored-to-runtime workflow makes Metal ready",
    "authored-to-runtime workflow exposes public native/RHI handles",
    "authored-to-runtime workflow makes general production renderer quality ready"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenAuthoredRuntimeWorkflowClaim)) {
            Write-Error "$($doc.Label) contains forbidden authored-to-runtime workflow claim: $forbiddenAuthoredRuntimeWorkflowClaim"
        }
    }
}
foreach ($forbiddenRuntimeSceneValidationClaim in @(
    "validate-runtime-scene-package performs package cooking",
    "validate-runtime-scene-package parses source assets at runtime",
    "validate-runtime-scene-package executes external importers",
    "validate-runtime-scene-package makes renderer/RHI residency ready",
    "validate-runtime-scene-package makes package streaming ready",
    "validate-runtime-scene-package supports material graphs",
    "validate-runtime-scene-package supports shader graphs",
    "validate-runtime-scene-package supports live shader generation",
    "validate-runtime-scene-package makes editor productization ready",
    "validate-runtime-scene-package makes Metal ready",
    "validate-runtime-scene-package exposes public native/RHI handles",
    "validate-runtime-scene-package makes general production renderer quality ready",
    "validate-runtime-scene-package executes arbitrary shell",
    "validate-runtime-scene-package supports free-form edits"
)) {
    foreach ($doc in @(
        @{ Text = $aiIntegrationText; Label = "docs/ai-integration.md" },
        @{ Text = $generatedScenariosText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $promptPackText; Label = "docs/specs/game-prompt-pack.md" },
        @{ Text = $handoffPromptText; Label = "docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" }
    )) {
        if ($doc.Text.Contains($forbiddenRuntimeSceneValidationClaim)) {
            Write-Error "$($doc.Label) contains forbidden runtime scene package validation claim: $forbiddenRuntimeSceneValidationClaim"
        }
    }
}

$authoringSurfaceIds = @{}
foreach ($authoringSurface in $productionLoop.authoringSurfaces) {
    Assert-Properties $authoringSurface @("id", "status", "owner", "notes") "engine manifest aiOperableProductionLoop authoringSurfaces"
    if ($authoringSurfaceIds.ContainsKey($authoringSurface.id)) {
        Write-Error "engine manifest aiOperableProductionLoop authoring surface id is duplicated: $($authoringSurface.id)"
    }
    $authoringSurfaceIds[$authoringSurface.id] = $true
    if ($allowedProductionStatuses -notcontains $authoringSurface.status) {
        Write-Error "engine manifest aiOperableProductionLoop authoring surface '$($authoringSurface.id)' has invalid status: $($authoringSurface.status)"
    }
}
$sceneAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "scene-component-prefab-schema-v2" })
if ($sceneAuthoringSurface.Count -ne 1 -or $sceneAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop authoring surface scene-component-prefab-schema-v2 must be ready as a contract-only MK_scene surface"
}
if (-not ([string]$sceneAuthoringSurface[0].notes).Contains("Contract-only") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("nested prefab propagation/merge resolution UX") -or
    -not ([string]$sceneAuthoringSurface[0].notes).Contains("2D/3D vertical slices")) {
    Write-Error "engine manifest scene-component-prefab-schema-v2 authoring surface must keep contract-only follow-up limits explicit"
}
$assetIdentityAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "asset-identity-v2" })
if ($assetIdentityAuthoringSurface.Count -ne 1 -or $assetIdentityAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop authoring surface asset-identity-v2 must be ready as a foundation-only MK_assets surface"
}
if (-not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("Foundation-only") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("GameEngine.AssetIdentity.v2") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("plan_asset_identity_placements_v2") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("Reviewed command-owned apply surfaces") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("placement_rows") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("ContentBrowserState") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("SourceAssetRegistryDocumentV1") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("ContentBrowserState::refresh_from") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("content_browser_import.assets") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("GameEngine.Project.v4 project.source_registry") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("refresh_content_browser_from_project_source_registry") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("Reload Source Registry") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("audit_runtime_scene_asset_identity") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("AssetKeyV2 key-first") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("tools/new-game.ps1") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("runtime source registry parsing") -or
    -not ([string]$assetIdentityAuthoringSurface[0].notes).Contains("2D/3D vertical slices")) {
    Write-Error "engine manifest asset-identity-v2 authoring surface must keep foundation-only follow-up limits explicit"
}
$uiAtlasAuthoringSurface = @($productionLoop.authoringSurfaces | Where-Object { $_.id -eq "ui-atlas-metadata-authoring-tooling-v1" })
if ($uiAtlasAuthoringSurface.Count -ne 1 -or $uiAtlasAuthoringSurface[0].status -ne "ready") {
    Write-Error "engine manifest aiOperableProductionLoop authoring surface ui-atlas-metadata-authoring-tooling-v1 must be ready as a cooked-metadata-only MK_assets/MK_tools surface"
}
if (-not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("GameEngine.UiAtlas.v1") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("author_cooked_ui_atlas_metadata") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("verify_cooked_ui_atlas_package_metadata") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("author_packed_ui_atlas_from_decoded_images") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("author_packed_ui_glyph_atlas_from_rasterized_glyphs") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("RuntimeUiAtlasGlyph") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("GameEngine.CookedTexture.v1") -or
    -not ([string]$uiAtlasAuthoringSurface[0].notes).Contains("renderer texture upload")) {
    Write-Error "engine manifest ui-atlas-metadata-authoring-tooling-v1 authoring surface must keep cooked metadata tooling, decoded/glyph atlas bridges, and renderer-upload limits explicit"
}

foreach ($packageSurface in $productionLoop.packageSurfaces) {
    Assert-Properties $packageSurface @("id", "status", "targets", "validationRecipes", "notes") "engine manifest aiOperableProductionLoop packageSurfaces"
    if ($allowedProductionStatuses -notcontains $packageSurface.status) {
        Write-Error "engine manifest aiOperableProductionLoop package surface '$($packageSurface.id)' has invalid status: $($packageSurface.status)"
    }
    foreach ($target in @($packageSurface.targets)) {
        if (-not $packagingTargetNames.ContainsKey($target)) {
            Write-Error "engine manifest aiOperableProductionLoop package surface '$($packageSurface.id)' references unknown packaging target: $target"
        }
    }
    foreach ($validationRecipe in @($packageSurface.validationRecipes)) {
        if (-not $validationRecipeNames.ContainsKey($validationRecipe)) {
            Write-Error "engine manifest aiOperableProductionLoop package surface '$($packageSurface.id)' references unknown validation recipe: $validationRecipe"
        }
    }
}

$requiredGapIds = @(
    "scene-component-prefab-schema-v2",
    "runtime-resource-v2",
    "renderer-rhi-resource-foundation",
    "frame-graph-v1",
    "upload-staging-v1",
    "2d-playable-vertical-slice",
    "3d-playable-vertical-slice",
    "editor-productization",
    "production-ui-importer-platform-adapters",
    "full-repository-quality-gate"
)
$gapIds = @{}
foreach ($gap in $productionLoop.unsupportedProductionGaps) {
    Assert-Properties $gap @("id", "oneDotZeroCloseoutTier", "status", "requiredBeforeReadyClaim", "notes") "engine manifest aiOperableProductionLoop unsupportedProductionGaps"
    $tier = [string]$gap.oneDotZeroCloseoutTier
    if (@("foundation-follow-up", "package-evidence", "closeout-wedge") -notcontains $tier) {
        Write-Error "engine manifest aiOperableProductionLoop unsupported gap '$($gap.id)' has invalid oneDotZeroCloseoutTier '$tier'"
    }
    $gapIds[$gap.id] = $true
    if ($gap.status -eq "ready") {
        Write-Error "engine manifest aiOperableProductionLoop unsupported gap '$($gap.id)' must not be ready"
    }
}
foreach ($gapId in $requiredGapIds) {
    if (-not $gapIds.ContainsKey($gapId)) {
        Write-Error "engine manifest aiOperableProductionLoop missing unsupported gap id: $gapId"
    }
}
$sceneSchemaGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "scene-component-prefab-schema-v2" })
if ($sceneSchemaGap.Count -ne 1 -or $sceneSchemaGap[0].status -ne "implemented-contract-only") {
    Write-Error "engine manifest aiOperableProductionLoop scene-component-prefab-schema-v2 gap must be implemented-contract-only"
}
if (-not ([string]$sceneSchemaGap[0].notes).Contains("contract-only") -or
    -not ([string]$sceneSchemaGap[0].notes).Contains("broad/dependent package cooking") -or
    -not ([string]$sceneSchemaGap[0].notes).Contains("nested prefab propagation/merge resolution UX")) {
    Write-Error "engine manifest aiOperableProductionLoop scene-component-prefab-schema-v2 gap must keep remaining unsupported claims explicit"
}
$assetIdentityGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "asset-identity-v2" })
if ($assetIdentityGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop asset-identity-v2 gap must leave unsupportedProductionGaps after reference cleanup closeout"
}
foreach ($needle in @(
    "Asset Identity v2 Reference Cleanup Milestone v1 completes",
    "audit_runtime_scene_asset_identity",
    "runtime-resource-v2 next"
)) {
    if (-not ((([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " ").Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe asset identity closeout and next gap: $needle"
    }
}
$runtimeResourceGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "runtime-resource-v2" })
if ($runtimeResourceGap.Count -ne 1 -or $runtimeResourceGap[0].status -ne "implemented-foundation-only") {
    Write-Error "engine manifest aiOperableProductionLoop runtime-resource-v2 gap must be implemented-foundation-only"
}
if (-not ([string]$runtimeResourceGap[0].notes).Contains("foundation-only") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("generation-checked handles") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("RuntimeResidentPackageMountSetV2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("RuntimeResidentCatalogCacheV2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("RuntimeResidentPackageMountIdV2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("commit_runtime_resident_package_replace_v2") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("slot-preserving") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("projected resident budget") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("raw loaded-package catalog") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("mount-set generations") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("package streaming") -or
    -not ([string]$runtimeResourceGap[0].notes).Contains("renderer/RHI resource ownership")) {
    Write-Error "engine manifest aiOperableProductionLoop runtime-resource-v2 gap must keep remaining unsupported claims explicit"
}
$runtimeResourceRequiredClaims = @($runtimeResourceGap[0].requiredBeforeReadyClaim)
if ($runtimeResourceRequiredClaims -contains "production package mounts") {
    Write-Error "engine manifest aiOperableProductionLoop runtime-resource-v2 production package mounts claim must be closed by resident package mount set"
}
foreach ($requiredClaim in @("resource residency", "hot reload", "renderer/RHI resource ownership", "package streaming")) {
    if ($runtimeResourceRequiredClaims -notcontains $requiredClaim) {
        Write-Error "engine manifest aiOperableProductionLoop runtime-resource-v2 remaining claim missing: $requiredClaim"
    }
}
foreach ($check in @(
    @{
        Path = "engine/runtime/include/mirakana/runtime/resource_runtime.hpp"
        Needles = @(
            "RuntimeResidentPackageMountSetV2",
            "RuntimeResidentPackageMountIdV2",
            "RuntimeResidentPackageMountStatusV2",
            "RuntimeResidentPackageMountCatalogBuildResultV2",
            "build_runtime_resource_catalog_v2_from_resident_mount_set",
            "RuntimeResidentCatalogCacheV2",
            "RuntimeResidentCatalogCacheStatusV2",
            "RuntimeResidentPackageReplaceCommitStatusV2",
            "RuntimeResidentPackageReplaceCommitResultV2",
            "commit_runtime_resident_package_replace_v2",
            "RuntimeResidentPackageUnmountCommitStatusV2",
            "RuntimeResidentPackageUnmountCommitResultV2",
            "commit_runtime_resident_package_unmount_v2"
        )
    },
    @{
        Path = "engine/runtime/src/resource_runtime.cpp"
        Needles = @(
            "RuntimeResidentPackageReplaceCommitResultV2::succeeded",
            "RuntimeResidentPackageMountSetReplaceAccessV2",
            "invoked_candidate_catalog_build",
            "RuntimeResidentPackageMountSetV2 projected_mount_set = mount_set",
            "RuntimeResidentCatalogCacheV2 projected_catalog_cache = catalog_cache",
            "mount_set = std::move(projected_mount_set)",
            "catalog_cache = std::move(projected_catalog_cache)"
        )
    },
    @{
        Path = "docs/current-capabilities.md"
        Needles = @(
            "Resident package mount set",
            "RuntimeResidentPackageMountSetV2",
            "Resident catalog cache",
            "RuntimeResidentCatalogCacheV2",
            "Resident package streaming mount commit",
            "Resident package replacement commit",
            "commit_runtime_resident_package_replace_v2",
            "disk/VFS mount discovery"
        )
    },
    @{
        Path = "docs/testing.md"
        Needles = @(
            "Runtime Resource Resident Package Replacement Commit v1 coverage",
            "MK_runtime_resource_resident_replace_tests",
            "commit_runtime_resident_package_replace_v2"
        )
    },
    @{
        Path = "tests/unit/runtime_resource_resident_replace_tests.cpp"
        Needles = @(
            "runtime resident package replacement commit preserves mount slot and refreshes catalog cache",
            "runtime resident package replacement commit rejects invalid and missing ids before mutation",
            "runtime resident package replacement commit rejects duplicate candidate records before mutation",
            "runtime resident package replacement commit preserves state on projected budget failure"
        )
    },
    @{
        Path = "tests/unit/runtime_resource_resident_cache_tests.cpp"
        Needles = @(
            "runtime resident catalog cache reuses catalog for unchanged mount generation and budget",
            "runtime resident catalog cache rebuilds when mount set generation changes",
            "runtime resident catalog cache rejects budget changes without replacing cached catalog"
        )
    },
    @{
        Path = "engine/runtime/include/mirakana/runtime/package_streaming.hpp"
        Needles = @(
            "resident_mount_failed",
            "resident_catalog_refresh_failed",
            "RuntimeResidentCatalogCacheV2& catalog_cache",
            "RuntimeResidentPackageMountIdV2 mount_id",
            "resident_catalog_refresh"
        )
    },
    @{
        Path = "engine/runtime/src/package_streaming.cpp"
        Needles = @(
            "project_resident_packages",
            "evaluate_projected_resident_budget",
            "validate_loaded_package_catalog_before_mount",
            "resident_catalog_refresh_failed",
            "mount_set.unmount(mount_id)"
        )
    },
    @{
        Path = "tests/unit/runtime_package_streaming_resident_mount_tests.cpp"
        Needles = @(
            "runtime package streaming resident mount commit mounts package and refreshes resident catalog",
            "runtime package streaming resident mount commit rejects duplicate mount id before mutation",
            "runtime package streaming resident mount commit rejects duplicate records before mutation",
            "runtime package streaming resident mount commit preserves catalog on projected budget failure"
        )
    }
)) {
    $checkPath = Join-Path $root $check.Path
    if (-not (Test-Path -LiteralPath $checkPath)) {
        Write-Error "Missing runtime resource resident package mount set evidence file: $($check.Path)"
    }
    $fileText = Get-Content -LiteralPath $checkPath -Raw
    foreach ($needle in $check.Needles) {
        Assert-ContainsText $fileText $needle "$($check.Path) runtime resource resident package mount set evidence"
    }
}
$rendererRhiGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "renderer-rhi-resource-foundation" })
if ($rendererRhiGap.Count -ne 1 -or $rendererRhiGap[0].status -ne "implemented-foundation-only") {
    Write-Error "engine manifest aiOperableProductionLoop renderer-rhi-resource-foundation gap must be implemented-foundation-only"
}
if (-not ([string]$rendererRhiGap[0].notes).Contains("foundation-only") -or
    -not ([string]$rendererRhiGap[0].notes).Contains("RhiResourceLifetimeRegistry") -or
    -not ([string]$rendererRhiGap[0].notes).Contains("GPU allocator") -or
    -not ([string]$rendererRhiGap[0].notes).Contains("upload/staging") -or
    -not ([string]$rendererRhiGap[0].notes).Contains("2D/3D playable vertical slices")) {
    Write-Error "engine manifest aiOperableProductionLoop renderer-rhi-resource-foundation gap must keep foundation-only follow-up limits explicit"
}
$frameGraphGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "frame-graph-v1" })
if ($frameGraphGap.Count -ne 1 -or $frameGraphGap[0].status -ne "implemented-foundation-only") {
    Write-Error "engine manifest aiOperableProductionLoop frame-graph-v1 gap must be implemented-foundation-only"
}
if (-not ([string]$frameGraphGap[0].notes).Contains("foundation-only") -or
    -not ([string]$frameGraphGap[0].notes).Contains("FrameGraphV1Desc") -or
    -not ([string]$frameGraphGap[0].notes).Contains("barrier intent") -or
    -not ([string]$frameGraphGap[0].notes).Contains("execute_frame_graph_v1_schedule") -or
    -not ([string]$frameGraphGap[0].notes).Contains("execute_frame_graph_rhi_texture_schedule") -or
    -not ([string]$frameGraphGap[0].notes).Contains("production render graph") -or
    -not ([string]$frameGraphGap[0].notes).Contains("2D/3D playable vertical slices")) {
    Write-Error "engine manifest aiOperableProductionLoop frame-graph-v1 gap must keep foundation-only follow-up limits explicit"
}
$uploadStagingGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "upload-staging-v1" })
if ($uploadStagingGap.Count -ne 1 -or $uploadStagingGap[0].status -ne "implemented-foundation-only") {
    Write-Error "engine manifest aiOperableProductionLoop upload-staging-v1 gap must be implemented-foundation-only"
}
if (-not ([string]$uploadStagingGap[0].notes).Contains("foundation-only") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("RhiUploadStagingPlan") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("FenceValue") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("Runtime RHI Upload Submission Fence Rows v1") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("submitted_upload_fences") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("submitted_upload_fence_count") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("RHI Upload Stale Generation Diagnostics v1") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("stale_generation") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("native GPU upload") -or
    -not ([string]$uploadStagingGap[0].notes).Contains("2D/3D playable vertical slices")) {
    Write-Error "engine manifest aiOperableProductionLoop upload-staging-v1 gap must keep foundation-only follow-up limits explicit"
}
$playable3dGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "3d-playable-vertical-slice" })
if ($playable3dGap.Count -ne 1 -or $playable3dGap[0].status -ne "implemented-generated-desktop-3d-package-proof") {
    Write-Error "engine manifest aiOperableProductionLoop 3d-playable-vertical-slice gap must be implemented-generated-desktop-3d-package-proof"
}
if (@($playable3dGap[0].requiredBeforeReadyClaim) -contains "AI-created visible 3D game") {
    Write-Error "engine manifest aiOperableProductionLoop 3d-playable-vertical-slice gap must not keep AI-created visible 3D game as an unmet claim after the committed generated package proof"
}
foreach ($claim in @(
    "broader generated 3D production readiness",
    "broad dependency cooking and package streaming",
    "production material/shader graph and live shader generation",
    "Metal readiness and broad backend parity",
    "editor productization and general renderer quality"
)) {
    if (@($playable3dGap[0].requiredBeforeReadyClaim) -notcontains $claim) {
        Write-Error "engine manifest aiOperableProductionLoop 3d-playable-vertical-slice gap requiredBeforeReadyClaim missing: $claim"
    }
}
if (-not ([string]$playable3dGap[0].notes).Contains("sample_generated_desktop_runtime_3d_package") -or
    -not ([string]$playable3dGap[0].notes).Contains("committed generated") -or
    -not ([string]$playable3dGap[0].notes).Contains("camera/controller") -or
    -not ([string]$playable3dGap[0].notes).Contains("compute morph") -or
    -not ([string]$playable3dGap[0].notes).Contains("selected generated directional shadow package smoke") -or
    -not ([string]$playable3dGap[0].notes).Contains("directional_shadow_status=ready") -or
    -not ([string]$playable3dGap[0].notes).Contains("selected generated gameplay systems package smoke") -or
    -not ([string]$playable3dGap[0].notes).Contains("gameplay_systems_status=ready") -or
    -not ([string]$playable3dGap[0].notes).Contains("production directional shadow quality") -or
    -not ([string]$playable3dGap[0].notes).Contains("broad shadow+morph composition beyond the selected receiver smoke") -or
    -not ([string]$playable3dGap[0].notes).Contains("broad generated 3D production readiness") -or
    -not ([string]$playable3dGap[0].notes).Contains("Metal readiness")) {
    Write-Error "engine manifest aiOperableProductionLoop 3d-playable-vertical-slice gap must describe the committed generated 3D package proof and remaining unsupported limits"
}
if (([string]$playable3dGap[0].notes).Contains("directional shadows and shadow filtering for generated packages")) {
    Write-Error "engine manifest aiOperableProductionLoop 3d-playable-vertical-slice gap must not keep stale generated directional shadow unsupported text"
}
$physicsCollisionGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "physics-1-0-collision-system" })
if ($physicsCollisionGap.Count -ne 0) {
    Write-Error "engine manifest aiOperableProductionLoop physics-1-0-collision-system gap must leave unsupportedProductionGaps after Physics 1.0 closeout"
}
foreach ($needle in @(
    "Physics 1.0 Collision System Closeout v1 is complete",
    "first-party MK_physics 1.0 ready surface",
    "dynamic-vs-dynamic TOI, rotational CCD, 2D CCD, oriented boxes, mesh/convex casts",
    "runtime-resource-v2 next"
)) {
    if (-not ((([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " ").Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan must describe Physics 1.0 closeout and next gap: $needle"
    }
}
$editorProductizationGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "editor-productization" })
if ($editorProductizationGap.Count -ne 1 -or $editorProductizationGap[0].status -ne "partly-ready") {
    Write-Error "engine manifest aiOperableProductionLoop editor-productization gap must be partly-ready until full editor productization closes"
}
foreach ($claim in @(
    "Vulkan/Metal material-preview display parity beyond D3D12 host-owned execution evidence"
)) {
    if (@($editorProductizationGap[0].requiredBeforeReadyClaim) -notcontains $claim) {
        Write-Error "engine manifest aiOperableProductionLoop editor-productization gap requiredBeforeReadyClaim missing: $claim"
    }
}
foreach ($excludedClaim in @(
    "Unity/UE-like editor UX",
    "active-session hot reload and broader in-process runtime-host embedding beyond reviewed external runtime-host launch, linked-driver handoff, and explicit editor game-module driver load evidence",
    "nested prefab propagation/merge resolution UX",
    "resource management/capture execution beyond host-owned evidence rows",
    "stable third-party ABI",
    "unacknowledged/automatic host-gated AI command execution workflows"
)) {
    if (@($editorProductizationGap[0].requiredBeforeReadyClaim) -contains $excludedClaim) {
        Write-Error "engine manifest aiOperableProductionLoop editor-productization gap requiredBeforeReadyClaim must not require Engine 1.0 exclusions: $excludedClaim"
    }
}
if (-not ([string]$editorProductizationGap[0].notes).Contains("EditorRuntimeHostPlaytestLaunchDesc") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorRuntimeHostPlaytestLaunchModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_editor_runtime_host_playtest_launch_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("play_in_editor.runtime_host") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Win32ProcessRunner") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor In-Process Runtime Host Review v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorInProcessRuntimeHostDesc") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorInProcessRuntimeHostModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("begin_editor_in_process_runtime_host_session") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("play_in_editor.in_process_runtime_host") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("IEditorPlaySessionDriver handoff") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Dynamic Game Module Driver Load v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Game Module Driver Safe Reload Review v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Game Module Driver Contract Metadata Review v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Game Module Driver Dynamic Probe v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("DynamicLibrary") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("LoadLibraryExW") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorGameModuleDriverApi") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorGameModuleDriverContractMetadataModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_editor_game_module_driver_contract_metadata_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("MK_editor_game_module_driver_probe") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("MK_editor_game_module_driver_load_tests") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("GameEngine.EditorGameModuleDriver.v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("mirakana_create_editor_game_module_driver_v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("EditorGameModuleDriverReloadModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_editor_game_module_driver_reload_model") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("play_in_editor.game_module_driver") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("play_in_editor.game_module_driver.reload") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("play_in_editor.game_module_driver.contract") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Load Game Module Driver") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Reload Game Module Driver") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Productization 1.0 Scope Closeout v1 reclassifies") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("vendor-stable third-party editor DLL ABI and unacknowledged or automatic host-gated AI command execution are explicit Engine 1.0 exclusions rather than required-before-ready claims")) {
    Write-Error "engine manifest aiOperableProductionLoop editor-productization gap must describe reviewed external runtime-host launch, in-process linked-driver handoff, dynamic game-module driver load evidence, and remaining unsupported limits"
}
if (-not ([string]$editorProductizationGap[0].notes).Contains("Editor Prefab Variant Base Refresh Merge Review v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("PrefabVariantBaseRefreshPlan") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("plan_prefab_variant_base_refresh") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("apply_prefab_variant_base_refresh") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("prefab_variant_base_refresh") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Prefab Instance Source-Link Review v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ScenePrefabSourceLink") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("PrefabInstantiateDesc") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ScenePrefabInstanceSourceLinkModel") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("scene_prefab_source_links") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Scene Prefab Instance Refresh Review v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Prefab Instance Local Child Refresh Resolution v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ScenePrefabInstanceRefreshPlan") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("ScenePrefabInstanceRefreshPolicy") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("plan_scene_prefab_instance_refresh") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("make_scene_prefab_instance_refresh_action") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("scene_prefab_instance_refresh") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("keep_local_child") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Keep Local Children") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Prefab Instance Stale Node Refresh Resolution v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("keep_stale_source_node_as_local") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Keep Stale Source Nodes") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Editor Nested Prefab Refresh Resolution v1") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("keep_nested_prefab_instance") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("unsupported_nested_prefab_instance") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("Keep Nested Prefab Instances") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("unsupported local children") -or
    -not ([string]$editorProductizationGap[0].notes).Contains("explicit base-refresh apply")) {
    Write-Error "engine manifest aiOperableProductionLoop editor-productization gap must describe reviewed prefab variant base-refresh/source-link/scene-refresh evidence and remaining unsupported limits"
}
if (([string]$editorProductizationGap[0].notes).Contains("Full editor productization, dynamic game-module/runtime-host Play-In-Editor execution")) {
    Write-Error "engine manifest aiOperableProductionLoop editor-productization gap must not keep stale no-runtime-host-launch unsupported wording"
}
$productionUiImporterPlatformGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "production-ui-importer-platform-adapters" })
if ($productionUiImporterPlatformGap.Count -ne 1 -or $productionUiImporterPlatformGap[0].status -ne "planned") {
    Write-Error "engine manifest aiOperableProductionLoop production-ui-importer-platform-adapters gap must remain planned until OS/platform adapter work is complete"
}
foreach ($needle in @(
    "Runtime UI Accessibility Publish Plan v1",
    "AccessibilityPublishPlan",
    "AccessibilityPublishResult",
    "plan_accessibility_publish",
    "publish_accessibility_payload",
    "AccessibilityPayload",
    "IAccessibilityAdapter",
    "OS accessibility bridge publication",
    "native accessibility objects",
    "platform SDK calls",
    "dependency, legal, vcpkg, and notice records"
)) {
    if (-not ([string]$productionUiImporterPlatformGap[0].notes).Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop production-ui-importer-platform-adapters gap missing accessibility publish boundary text: $needle"
    }
}
foreach ($needle in @(
    "Runtime UI IME Composition Publish Plan v1",
    "ImeCompositionPublishPlan",
    "ImeCompositionPublishResult",
    "plan_ime_composition_update",
    "publish_ime_composition",
    "ImeComposition",
    "IImeAdapter",
    "native IME/text-input sessions",
    "platform SDK calls",
    "dependency, legal, vcpkg, and notice records"
)) {
    if (-not ([string]$productionUiImporterPlatformGap[0].notes).Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop production-ui-importer-platform-adapters gap missing IME composition publish boundary text: $needle"
    }
}
foreach ($needle in @(
    "Runtime UI Platform Text Input Session Plan v1",
    "PlatformTextInputSessionPlan",
    "PlatformTextInputSessionResult",
    "PlatformTextInputEndPlan",
    "PlatformTextInputEndResult",
    "plan_platform_text_input_session",
    "begin_platform_text_input",
    "plan_platform_text_input_end",
    "end_platform_text_input",
    "PlatformTextInputRequest",
    "IPlatformIntegrationAdapter",
    "native text-input object/session ownership",
    "virtual keyboard behavior",
    "platform SDK calls",
    "dependency, legal, vcpkg, and notice records"
)) {
    if (-not ([string]$productionUiImporterPlatformGap[0].notes).Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop production-ui-importer-platform-adapters gap missing platform text input boundary text: $needle"
    }
}
foreach ($needle in @(
    "Runtime UI Text Shaping Request Plan v1",
    "TextShapingRequestPlan",
    "TextShapingResult",
    "plan_text_shaping_request",
    "shape_text_run",
    "TextLayoutRequest",
    "TextLayoutRun",
    "ITextShapingAdapter",
    "production text shaping implementation",
    "bidirectional reordering",
    "production line breaking",
    "dependency, legal, vcpkg, and notice records"
)) {
    if (-not ([string]$productionUiImporterPlatformGap[0].notes).Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop production-ui-importer-platform-adapters gap missing text shaping request boundary text: $needle"
    }
}
foreach ($needle in @(
    "Runtime UI Font Rasterization Request Plan v1",
    "FontRasterizationRequestPlan",
    "FontRasterizationResult",
    "plan_font_rasterization_request",
    "rasterize_font_glyph",
    "FontRasterizationRequest",
    "GlyphAtlasAllocation",
    "IFontRasterizerAdapter",
    "real font loading/rasterization",
    "renderer texture upload",
    "dependency, legal, vcpkg, and notice records"
)) {
    if (-not ([string]$productionUiImporterPlatformGap[0].notes).Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop production-ui-importer-platform-adapters gap missing font rasterization request boundary text: $needle"
    }
}
foreach ($needle in @(
    "Runtime UI Image Decode Request Plan v1",
    "ImageDecodeRequestPlan",
    "ImageDecodeDispatchResult",
    "ImageDecodePixelFormat",
    "plan_image_decode_request",
    "decode_image_request",
    "ImageDecodeRequest",
    "ImageDecodeResult",
    "IImageDecodingAdapter",
    "runtime image decoding",
    "source image codecs",
    "SVG/vector decoding",
    "renderer texture upload",
    "dependency, legal, vcpkg, and notice records"
)) {
    if (-not ([string]$productionUiImporterPlatformGap[0].notes).Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop production-ui-importer-platform-adapters gap missing image decode request boundary text: $needle"
    }
}
foreach ($needle in @(
    "Runtime UI PNG Image Decoding Adapter v1",
    "PngImageDecodingAdapter",
    "IImageDecodingAdapter",
    "decode_audited_png_rgba8",
    "libspng",
    "asset-importers",
    "without new dependency, legal, vcpkg, or notice records",
    "runtime image decoding beyond the reviewed PNG adapter",
    "broader source image codecs"
)) {
    if (-not ([string]$productionUiImporterPlatformGap[0].notes).Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop production-ui-importer-platform-adapters gap missing PNG image decoding adapter boundary text: $needle"
    }
}
foreach ($needle in @(
    "Runtime UI Decoded Image Atlas Package Bridge v1",
    "PackedUiAtlasAuthoringDesc",
    "author_packed_ui_atlas_from_decoded_images",
    "plan_packed_ui_atlas_package_update",
    "apply_packed_ui_atlas_package_update",
    "pack_sprite_atlas_rgba8_max_side",
    "GameEngine.CookedTexture.v1",
    "GameEngine.UiAtlas.v1",
    "without new dependency, legal, vcpkg, or notice records",
    "renderer texture upload"
)) {
    if (-not ([string]$productionUiImporterPlatformGap[0].notes).Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop production-ui-importer-platform-adapters gap missing decoded image atlas package bridge text: $needle"
    }
}
foreach ($needle in @(
    "Runtime UI Glyph Atlas Package Bridge v1",
    "UiAtlasMetadataGlyph",
    "RuntimeUiAtlasGlyph",
    "PackedUiGlyphAtlasAuthoringDesc",
    "author_packed_ui_glyph_atlas_from_rasterized_glyphs",
    "plan_packed_ui_glyph_atlas_package_update",
    "apply_packed_ui_glyph_atlas_package_update",
    "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas",
    "source.decoding=rasterized-glyph-adapter",
    "atlas.packing=deterministic-glyph-atlas-rgba8-max-side",
    "GameEngine.CookedTexture.v1",
    "GameEngine.UiAtlas.v1",
    "without new dependency, legal, vcpkg, or notice records",
    "real font loading/rasterization",
    "renderer texture upload"
)) {
    if (-not ([string]$productionUiImporterPlatformGap[0].notes).Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop production-ui-importer-platform-adapters gap missing glyph atlas package bridge text: $needle"
    }
}
$fullRepoQualityGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "full-repository-quality-gate" })
if ($fullRepoQualityGap.Count -ne 1 -or $fullRepoQualityGap[0].status -ne "partly-ready") {
    Write-Error "engine manifest aiOperableProductionLoop full-repository-quality-gate gap must be partly-ready until Phase 1 quality gates complete"
}
if (-not ([string]$fullRepoQualityGap[0].notes).Contains("clang-tidy") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("coverage") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("sanitizer") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("Phase 1") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("CI Matrix Contract Check v1") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("ci-matrix-check") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("Windows/Linux/sanitizer/macOS/iOS") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("targeted changed-file clang-tidy") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("Full Repository Static Analysis CI Contract v1") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("static-analysis") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("tools/check-tidy.ps1 -Strict") -or
    -not ([string]$fullRepoQualityGap[0].notes).Contains("broader static analyzer profile")) {
    Write-Error "engine manifest aiOperableProductionLoop full-repository-quality-gate gap must name tidy, coverage, sanitizer, CI matrix contract, and remaining analyzer limits explicitly"
}

$tidyWrapperContent = Get-Content -LiteralPath (Join-Path $root "tools/check-tidy.ps1") -Raw
if (-not $tidyWrapperContent.Contains('[string[]]$Files')) {
    Write-Error "tools/check-tidy.ps1 must expose a targeted -Files lane for changed-file clang-tidy validation"
}
$testingContent = Get-Content -LiteralPath (Join-Path $root "docs/testing.md") -Raw
if (-not $testingContent.Contains("pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/src/physics3d.cpp")) {
    Write-Error "docs/testing.md must document the targeted tidy -Files lane"
}

$ciMatrixCheckText = Get-Content -LiteralPath (Join-Path $root "tools/check-ci-matrix.ps1") -Raw
$validateWorkflowText = Get-Content -LiteralPath (Join-Path $root ".github/workflows/validate.yml") -Raw
$validateScriptText = Get-Content -LiteralPath (Join-Path $root "tools/validate.ps1") -Raw
if (-not $validateScriptText.Contains("check-ci-matrix.ps1")) {
    Write-Error "tools/validate.ps1 must run check-ci-matrix.ps1"
}
foreach ($needle in @(
    ".github/workflows/validate.yml",
    ".github/workflows/ios-validate.yml",
    "windows-packages",
    "linux-coverage",
    "linux-sanitizer-test-logs",
    "static-analysis-tidy-logs",
    "macos-test-logs",
    "ios-simulator-build"
)) {
    if (-not $ciMatrixCheckText.Contains($needle)) {
        Write-Error "tools/check-ci-matrix.ps1 missing required CI matrix contract text: $needle"
    }
}
foreach ($needle in @(
    "static-analysis:",
    "name: Full Repository Static Analysis",
    "runs-on: ubuntu-latest",
    "sudo apt-get update && sudo apt-get install -y clang clang-tidy ninja-build",
    "./tools/check-tidy.ps1 -Strict -Preset ci-linux-clang",
    "static-analysis-tidy-logs"
)) {
    if (-not $validateWorkflowText.Contains($needle)) {
        Write-Error ".github/workflows/validate.yml missing required static-analysis lane text: $needle"
    }
}
$fullRepoStaticAnalysisPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-full-repository-static-analysis-ci-contract-v1.md") -Raw
foreach ($needle in @(
    "Full Repository Static Analysis CI Contract v1 Implementation Plan",
    "**Status:** Completed.",
    "static-analysis",
    "tools/check-tidy.ps1 -Strict",
    "static-analysis-tidy-logs",
    "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1",
    "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1"
)) {
    if (-not $fullRepoStaticAnalysisPlanText.Contains($needle)) {
        Write-Error "Full Repository Static Analysis CI Contract v1 plan missing completion evidence: $needle"
    }
}

$runtimeUiAccessibilityPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-07-runtime-ui-accessibility-publish-plan-v1.md") -Raw
foreach ($needle in @(
    "Runtime UI Accessibility Publish Plan v1",
    "**Status:** Completed",
    "AccessibilityPublishPlan",
    "AccessibilityPublishResult",
    "plan_accessibility_publish",
    "publish_accessibility_payload",
    "IAccessibilityAdapter",
    "OS accessibility adapter",
    "MK_ui_renderer_tests"
)) {
    if (-not $runtimeUiAccessibilityPlanText.Contains($needle)) {
        Write-Error "Runtime UI accessibility publish plan missing required evidence: $needle"
    }
}
$runtimeUiImePlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-07-runtime-ui-ime-composition-publish-plan-v1.md") -Raw
foreach ($needle in @(
    "Runtime UI IME Composition Publish Plan v1",
    "**Status:** Completed",
    "ImeCompositionPublishPlan",
    "ImeCompositionPublishResult",
    "plan_ime_composition_update",
    "publish_ime_composition",
    "IImeAdapter",
    "Win32/TSF",
    "MK_ui_renderer_tests"
)) {
    if (-not $runtimeUiImePlanText.Contains($needle)) {
        Write-Error "Runtime UI IME composition publish plan missing required evidence: $needle"
    }
}
$runtimeUiPlatformTextInputSessionPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-runtime-ui-platform-text-input-session-plan-v1.md") -Raw
foreach ($needle in @(
    "Runtime UI Platform Text Input Session Plan v1",
    "**Status:** Completed",
    "PlatformTextInputSessionPlan",
    "PlatformTextInputSessionResult",
    "PlatformTextInputEndPlan",
    "PlatformTextInputEndResult",
    "begin_platform_text_input",
    "end_platform_text_input",
    "IPlatformIntegrationAdapter",
    "native text-input object/session ownership",
    "MK_ui_renderer_tests"
)) {
    if (-not $runtimeUiPlatformTextInputSessionPlanText.Contains($needle)) {
        Write-Error "Runtime UI platform text input session plan missing required evidence: $needle"
    }
}
$runtimeUiFontRasterizationRequestPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-07-runtime-ui-font-rasterization-request-plan-v1.md") -Raw
foreach ($needle in @(
    "Runtime UI Font Rasterization Request Plan v1",
    "**Status:** Completed",
    "FontRasterizationRequestPlan",
    "FontRasterizationResult",
    "plan_font_rasterization_request",
    "rasterize_font_glyph",
    "IFontRasterizerAdapter",
    "invalid_font_allocation",
    "MK_ui_renderer_tests"
)) {
    if (-not $runtimeUiFontRasterizationRequestPlanText.Contains($needle)) {
        Write-Error "Runtime UI font rasterization request plan missing required evidence: $needle"
    }
}
$runtimeUiTextShapingRequestPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-runtime-ui-text-shaping-request-plan-v1.md") -Raw
foreach ($needle in @(
    "Runtime UI Text Shaping Request Plan v1",
    "**Status:** Completed",
    "TextShapingRequestPlan",
    "TextShapingResult",
    "plan_text_shaping_request",
    "shape_text_run",
    "ITextShapingAdapter",
    "invalid_text_shaping_result",
    "MK_ui_renderer_tests"
)) {
    if (-not $runtimeUiTextShapingRequestPlanText.Contains($needle)) {
        Write-Error "Runtime UI text shaping request plan missing required evidence: $needle"
    }
}
$runtimeUiImageDecodeRequestPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-runtime-ui-image-decode-request-plan-v1.md") -Raw
foreach ($needle in @(
    "Runtime UI Image Decode Request Plan v1",
    "**Status:** Completed",
    "ImageDecodeRequestPlan",
    "ImageDecodeDispatchResult",
    "ImageDecodePixelFormat",
    "plan_image_decode_request",
    "decode_image_request",
    "IImageDecodingAdapter",
    "invalid_image_decode_result",
    "MK_ui_renderer_tests"
)) {
    if (-not $runtimeUiImageDecodeRequestPlanText.Contains($needle)) {
        Write-Error "Runtime UI image decode request plan missing required evidence: $needle"
    }
}
$runtimeUiPngImageDecodingAdapterPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-runtime-ui-png-image-decoding-adapter-v1.md") -Raw
foreach ($needle in @(
    "Runtime UI PNG Image Decoding Adapter v1",
    "**Status:** Completed",
    "PngImageDecodingAdapter",
    "IImageDecodingAdapter",
    "decode_audited_png_rgba8",
    "ImageDecodePixelFormat::rgba8_unorm",
    "MK_tools_tests",
    "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1"
)) {
    if (-not $runtimeUiPngImageDecodingAdapterPlanText.Contains($needle)) {
        Write-Error "Runtime UI PNG image decoding adapter plan missing required evidence: $needle"
    }
}
$runtimeUiDecodedImageAtlasPackageBridgePlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-runtime-ui-decoded-image-atlas-package-bridge-v1.md") -Raw
foreach ($needle in @(
    "Runtime UI Decoded Image Atlas Package Bridge v1",
    "**Status:** Completed",
    "PackedUiAtlasAuthoringDesc",
    "author_packed_ui_atlas_from_decoded_images",
    "plan_packed_ui_atlas_package_update",
    "apply_packed_ui_atlas_package_update",
    "pack_sprite_atlas_rgba8_max_side",
    "GameEngine.CookedTexture.v1",
    "GameEngine.UiAtlas.v1",
    "MK_tools_tests"
)) {
    if (-not $runtimeUiDecodedImageAtlasPackageBridgePlanText.Contains($needle)) {
        Write-Error "Runtime UI decoded image atlas package bridge plan missing required evidence: $needle"
    }
}
$runtimeUiGlyphAtlasPackageBridgePlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-runtime-ui-glyph-atlas-package-bridge-v1.md") -Raw
foreach ($needle in @(
    "Runtime UI Glyph Atlas Package Bridge v1",
    "**Status:** Completed",
    "UiAtlasMetadataGlyph",
    "RuntimeUiAtlasGlyph",
    "PackedUiGlyphAtlasAuthoringDesc",
    "author_packed_ui_glyph_atlas_from_rasterized_glyphs",
    "plan_packed_ui_glyph_atlas_package_update",
    "apply_packed_ui_glyph_atlas_package_update",
    "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas",
    "GameEngine.CookedTexture.v1",
    "GameEngine.UiAtlas.v1",
    "MK_tools_tests"
)) {
    if (-not $runtimeUiGlyphAtlasPackageBridgePlanText.Contains($needle)) {
        Write-Error "Runtime UI glyph atlas package bridge plan missing required evidence: $needle"
    }
}
$runtimeInputRebindingCapturePlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-runtime-input-rebinding-capture-contract-v1.md") -Raw
foreach ($needle in @(
    "Runtime Input Rebinding Capture Contract v1",
    "**Status:** Completed",
    "RuntimeInputRebindingCaptureRequest",
    "RuntimeInputRebindingCaptureResult",
    "capture_runtime_input_rebinding_action",
    "MK_runtime_tests"
)) {
    if (-not $runtimeInputRebindingCapturePlanText.Contains($needle)) {
        Write-Error "Runtime input rebinding capture plan missing required evidence: $needle"
    }
}
$runtimeInputRebindingFocusConsumptionPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-runtime-input-rebinding-focus-consumption-v1.md") -Raw
foreach ($needle in @(
    "Runtime Input Rebinding Focus Consumption v1",
    "**Status:** Completed",
    "RuntimeInputRebindingFocusCaptureRequest",
    "RuntimeInputRebindingFocusCaptureResult",
    "capture_runtime_input_rebinding_action_with_focus",
    "focus_retained",
    "gameplay_input_consumed",
    "MK_runtime_tests"
)) {
    if (-not $runtimeInputRebindingFocusConsumptionPlanText.Contains($needle)) {
        Write-Error "Runtime input rebinding focus consumption plan missing required evidence: $needle"
    }
}
$runtimeInputRebindingPresentationRowsPlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-runtime-input-rebinding-presentation-rows-v1.md") -Raw
foreach ($needle in @(
    "Runtime Input Rebinding Presentation Rows v1",
    "**Status:** Completed",
    "RuntimeInputRebindingPresentationToken",
    "RuntimeInputRebindingPresentationRow",
    "RuntimeInputRebindingPresentationModel",
    "present_runtime_input_action_trigger",
    "present_runtime_input_axis_source",
    "make_runtime_input_rebinding_presentation",
    "glyph_lookup_key",
    "model.diagnostics[0].path == row->id",
    "MK_runtime_tests"
)) {
    if (-not $runtimeInputRebindingPresentationRowsPlanText.Contains($needle)) {
        Write-Error "Runtime input rebinding presentation rows plan missing required evidence: $needle"
    }
}
$editorInputRebindingActionCapturePlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-editor-input-rebinding-action-capture-panel-v1.md") -Raw
foreach ($needle in @(
    "Editor Input Rebinding Action Capture Panel v1",
    "**Status:** Completed",
    "EditorInputRebindingCaptureModel",
    "make_editor_input_rebinding_capture_action_model",
    "make_input_rebinding_capture_action_ui_model",
    "input_rebinding.capture",
    "MK_editor_core_tests",
    "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1"
)) {
    if (-not $editorInputRebindingActionCapturePlanText.Contains($needle)) {
        Write-Error "Editor input rebinding action capture panel plan missing required evidence: $needle"
    }
}
$runtimeUiDocs = @{
    "docs/current-capabilities.md" = @("AccessibilityPublishPlan", "publish_accessibility_payload", "OS accessibility bridge publication", "ImeCompositionPublishPlan", "publish_ime_composition", "native IME/text-input adapter integration", "PlatformTextInputSessionPlan", "begin_platform_text_input", "IPlatformIntegrationAdapter", "virtual keyboard/session ownership", "TextShapingRequestPlan", "shape_text_run", "ITextShapingAdapter", "FontRasterizationRequestPlan", "rasterize_font_glyph", "adapter allocation diagnostics", "ImageDecodeRequestPlan", "decode_image_request", "ImageDecodePixelFormat::rgba8_unorm")
    "docs/ui.md" = @("AccessibilityPublishPlan", "IAccessibilityAdapter", "UI Automation", "NSAccessibility", "ImeCompositionPublishPlan", "IImeAdapter", "Win32/TSF", "PlatformTextInputSessionPlan", "IPlatformIntegrationAdapter", "virtual keyboard behavior", "TextShapingRequestPlan", "ITextShapingAdapter", "TextLayoutRun", "FontRasterizationRequestPlan", "IFontRasterizerAdapter", "GlyphAtlasAllocation", "ImageDecodeRequestPlan", "IImageDecodingAdapter", "ImageDecodePixelFormat::rgba8_unorm")
    "docs/roadmap.md" = @("Runtime UI Accessibility Publish Plan v1", "plan_accessibility_publish", "Runtime UI IME Composition Publish Plan v1", "plan_ime_composition_update", "Runtime UI Platform Text Input Session Plan v1", "begin_platform_text_input", "Runtime UI Text Shaping Request Plan v1", "plan_text_shaping_request", "Runtime UI Font Rasterization Request Plan v1", "plan_font_rasterization_request", "Runtime UI Image Decode Request Plan v1", "plan_image_decode_request", "source codecs", "platform SDK calls")
    "docs/superpowers/plans/README.md" = @("2026-05-08-runtime-ui-text-shaping-request-plan-v1.md", "TextShapingResult", "2026-05-07-runtime-ui-accessibility-publish-plan-v1.md", "AccessibilityPublishResult", "2026-05-07-runtime-ui-ime-composition-publish-plan-v1.md", "ImeCompositionPublishResult", "2026-05-08-runtime-ui-platform-text-input-session-plan-v1.md", "PlatformTextInputSessionResult", "2026-05-07-runtime-ui-font-rasterization-request-plan-v1.md", "FontRasterizationResult", "2026-05-08-runtime-ui-image-decode-request-plan-v1.md", "ImageDecodeDispatchResult", "source codecs", "third-party adapters unsupported")
    "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md" = @("Runtime UI Accessibility Publish Plan v1", "IAccessibilityAdapter", "native accessibility objects", "Runtime UI IME Composition Publish Plan v1", "IImeAdapter", "native IME/text-input sessions", "Runtime UI Platform Text Input Session Plan v1", "IPlatformIntegrationAdapter", "virtual keyboard behavior", "Runtime UI Text Shaping Request Plan v1", "ITextShapingAdapter", "Runtime UI Font Rasterization Request Plan v1", "IFontRasterizerAdapter", "Runtime UI Image Decode Request Plan v1", "IImageDecodingAdapter", "source image codecs", "renderer texture upload")
}
foreach ($docPath in $runtimeUiDocs.Keys) {
    $docText = Get-Content -LiteralPath (Join-Path $root $docPath) -Raw
    foreach ($needle in $runtimeUiDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error "$docPath missing Runtime UI accessibility publish evidence: $needle"
        }
    }
}
$runtimeUiPngDocs = @{
    "docs/current-capabilities.md" = @("Runtime UI PNG Image Decoding Adapter v1", "PngImageDecodingAdapter", "decode_audited_png_rgba8")
    "docs/dependencies.md" = @("PngImageDecodingAdapter", "decode_audited_png_rgba8", "IImageDecodingAdapter")
    "docs/ui.md" = @("Runtime UI PNG Image Decoding Adapter v1", "PngImageDecodingAdapter", "decode_audited_png_rgba8")
    "docs/architecture.md" = @("PngImageDecodingAdapter", "decode_audited_png_rgba8", "IImageDecodingAdapter")
    "docs/roadmap.md" = @("Runtime UI PNG Image Decoding Adapter v1", "PngImageDecodingAdapter", "decode_audited_png_rgba8")
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md" = @("PngImageDecodingAdapter", "decode_audited_png_rgba8", "asset-importers")
    "docs/ai-game-development.md" = @("PngImageDecodingAdapter", "decode_audited_png_rgba8", "source PNGs")
    "docs/superpowers/plans/README.md" = @("2026-05-08-runtime-ui-png-image-decoding-adapter-v1.md", "PngImageDecodingAdapter", "decode_audited_png_rgba8")
    "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md" = @("Runtime UI PNG Image Decoding Adapter v1", "PngImageDecodingAdapter", "decode_audited_png_rgba8")
}
foreach ($docPath in $runtimeUiPngDocs.Keys) {
    $docText = Get-Content -LiteralPath (Join-Path $root $docPath) -Raw
    foreach ($needle in $runtimeUiPngDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error "$docPath missing Runtime UI PNG image decoding adapter evidence: $needle"
        }
    }
}
$runtimeUiDecodedAtlasDocs = @{
    "docs/current-capabilities.md" = @("Runtime UI Decoded Image Atlas Package Bridge v1", "author_packed_ui_atlas_from_decoded_images", "GameEngine.CookedTexture.v1")
    "docs/dependencies.md" = @("author_packed_ui_atlas_from_decoded_images", "plan_packed_ui_atlas_package_update", "GameEngine.CookedTexture.v1")
    "docs/ui.md" = @("Runtime UI Decoded Image Atlas Package Bridge v1", "author_packed_ui_atlas_from_decoded_images", "GameEngine.CookedTexture.v1")
    "docs/architecture.md" = @("author_packed_ui_atlas_from_decoded_images", "plan_packed_ui_atlas_package_update", "GameEngine.CookedTexture.v1")
    "docs/roadmap.md" = @("Runtime UI Decoded Image Atlas Package Bridge v1", "author_packed_ui_atlas_from_decoded_images", "GameEngine.CookedTexture.v1")
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md" = @("author_packed_ui_atlas_from_decoded_images", "plan_packed_ui_atlas_package_update", "GameEngine.CookedTexture.v1")
    "docs/ai-game-development.md" = @("author_packed_ui_atlas_from_decoded_images", "plan_packed_ui_atlas_package_update", "GameEngine.CookedTexture.v1")
    "docs/superpowers/plans/README.md" = @("2026-05-08-runtime-ui-decoded-image-atlas-package-bridge-v1.md", "PackedUiAtlasAuthoringDesc", "GameEngine.CookedTexture.v1")
    "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md" = @("Runtime UI Decoded Image Atlas Package Bridge v1", "author_packed_ui_atlas_from_decoded_images", "GameEngine.CookedTexture.v1")
}
foreach ($docPath in $runtimeUiDecodedAtlasDocs.Keys) {
    $docText = Get-Content -LiteralPath (Join-Path $root $docPath) -Raw
    foreach ($needle in $runtimeUiDecodedAtlasDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error "$docPath missing Runtime UI decoded image atlas package bridge evidence: $needle"
        }
    }
}
$runtimeUiGlyphAtlasDocs = @{
    "docs/current-capabilities.md" = @("Runtime UI Glyph Atlas Package Bridge v1", "UiAtlasMetadataGlyph", "RuntimeUiAtlasGlyph", "author_packed_ui_glyph_atlas_from_rasterized_glyphs", "GameEngine.CookedTexture.v1", "GameEngine.UiAtlas.v1")
    "docs/dependencies.md" = @("author_packed_ui_glyph_atlas_from_rasterized_glyphs", "plan_packed_ui_glyph_atlas_package_update", "GameEngine.CookedTexture.v1", "GameEngine.UiAtlas.v1")
    "docs/testing.md" = @("UiAtlasMetadataGlyph", "RuntimeUiAtlasGlyph", "author_packed_ui_glyph_atlas_from_rasterized_glyphs", "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas")
    "docs/ui.md" = @("Runtime UI Glyph Atlas Package Bridge v1", "UiAtlasMetadataGlyph", "author_packed_ui_glyph_atlas_from_rasterized_glyphs", "source.decoding=rasterized-glyph-adapter", "atlas.packing=deterministic-glyph-atlas-rgba8-max-side")
    "docs/architecture.md" = @("author_packed_ui_glyph_atlas_from_rasterized_glyphs", "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas", "GameEngine.CookedTexture.v1")
    "docs/roadmap.md" = @("Runtime UI Glyph Atlas Package Bridge v1", "UiAtlasMetadataGlyph", "RuntimeUiAtlasGlyph", "author_packed_ui_glyph_atlas_from_rasterized_glyphs")
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md" = @("Runtime UI Glyph Atlas Package Bridge v1", "UiAtlasMetadataGlyph", "RuntimeUiAtlasGlyph", "author_packed_ui_glyph_atlas_from_rasterized_glyphs")
    "docs/ai-game-development.md" = @("author_packed_ui_glyph_atlas_from_rasterized_glyphs", "plan_packed_ui_glyph_atlas_package_update", "source.decoding=rasterized-glyph-adapter", "atlas.packing=deterministic-glyph-atlas-rgba8-max-side")
    "docs/superpowers/plans/README.md" = @("2026-05-08-runtime-ui-glyph-atlas-package-bridge-v1.md", "Runtime UI Glyph Atlas Package Bridge v1", "PackedUiGlyphAtlasAuthoringDesc")
    "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md" = @("Runtime UI Glyph Atlas Package Bridge v1", "author_packed_ui_glyph_atlas_from_rasterized_glyphs", "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas")
}
foreach ($docPath in $runtimeUiGlyphAtlasDocs.Keys) {
    $docText = Get-Content -LiteralPath (Join-Path $root $docPath) -Raw
    foreach ($needle in $runtimeUiGlyphAtlasDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error "$docPath missing Runtime UI glyph atlas package bridge evidence: $needle"
        }
    }
}
$runtimeInputRebindingCaptureDocs = @{
    "docs/current-capabilities.md" = @("Runtime Input Rebinding Capture Contract v1", "capture_runtime_input_rebinding_action", "RuntimeInputRebindingCaptureRequest")
    "docs/architecture.md" = @("capture_runtime_input_rebinding_action", "RuntimeInputRebindingProfile", "axis capture")
    "docs/roadmap.md" = @("Runtime Input Rebinding Capture Contract v1", "RuntimeInputRebindingCaptureResult", "capture_runtime_input_rebinding_action")
    "docs/ai-game-development.md" = @("RuntimeInputRebindingCaptureRequest", "RuntimeInputRebindingCaptureResult", "capture_runtime_input_rebinding_action")
    "docs/superpowers/plans/README.md" = @("2026-05-08-runtime-input-rebinding-capture-contract-v1.md", "RuntimeInputRebindingCaptureRequest", "capture_runtime_input_rebinding_action")
    "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md" = @("Runtime Input Rebinding Capture Contract v1", "RuntimeInputRebindingCaptureRequest", "capture_runtime_input_rebinding_action")
}
foreach ($docPath in $runtimeInputRebindingCaptureDocs.Keys) {
    $docText = Get-Content -LiteralPath (Join-Path $root $docPath) -Raw
    foreach ($needle in $runtimeInputRebindingCaptureDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
        Write-Error "$docPath missing Runtime input rebinding capture evidence: $needle"
        }
    }
}
$runtimeInputRebindingFocusConsumptionDocs = @{
    "docs/current-capabilities.md" = @("Runtime Input Rebinding Focus Consumption v1", "RuntimeInputRebindingFocusCaptureRequest", "capture_runtime_input_rebinding_action_with_focus", "gameplay_input_consumed")
    "docs/architecture.md" = @("RuntimeInputRebindingFocusCaptureRequest", "RuntimeInputRebindingFocusCaptureResult", "capture_runtime_input_rebinding_action_with_focus", "pressed-but-rejected candidates")
    "docs/roadmap.md" = @("Runtime Input Rebinding Focus Consumption v1", "RuntimeInputRebindingFocusCaptureResult", "capture_runtime_input_rebinding_action_with_focus", "gameplay_input_consumed")
    "docs/testing.md" = @("RuntimeInputRebindingFocusCaptureRequest", "capture_runtime_input_rebinding_action_with_focus", "pressed rejected-trigger consumption")
    "docs/ai-game-development.md" = @("Runtime Input Rebinding Focus Consumption v1", "RuntimeInputRebindingFocusCaptureRequest", "capture_runtime_input_rebinding_action_with_focus", "gameplay_input_consumed")
    "docs/superpowers/plans/README.md" = @("2026-05-08-runtime-input-rebinding-focus-consumption-v1.md", "RuntimeInputRebindingFocusCaptureRequest", "capture_runtime_input_rebinding_action_with_focus")
    "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md" = @("Runtime Input Rebinding Focus Consumption v1", "RuntimeInputRebindingFocusCaptureRequest", "capture_runtime_input_rebinding_action_with_focus", "gameplay_input_consumed")
}
foreach ($docPath in $runtimeInputRebindingFocusConsumptionDocs.Keys) {
    $docText = Get-Content -LiteralPath (Join-Path $root $docPath) -Raw
    foreach ($needle in $runtimeInputRebindingFocusConsumptionDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error "$docPath missing Runtime input rebinding focus consumption evidence: $needle"
        }
    }
}
$runtimeInputRebindingPresentationRowsDocs = @{
    "docs/current-capabilities.md" = @("Runtime Input Rebinding Presentation Rows v1", "RuntimeInputRebindingPresentationModel", "make_runtime_input_rebinding_presentation", "symbolic glyph lookup keys", "platform input glyph generation")
    "docs/architecture.md" = @("RuntimeInputRebindingPresentationToken", "make_runtime_input_rebinding_presentation", "symbolic glyph lookup keys", "platform input glyph generation")
    "docs/roadmap.md" = @("Runtime Input Rebinding Presentation Rows v1", "RuntimeInputRebindingPresentationRow", "present_runtime_input_axis_source", "symbolic glyph lookup keys")
    "docs/testing.md" = @("Runtime Input Rebinding Presentation Rows v1", "RuntimeInputRebindingPresentationToken", "glyph_lookup_key", "invalid-profile diagnostics")
    "docs/ai-game-development.md" = @("Runtime Input Rebinding Presentation Rows v1", "RuntimeInputRebindingPresentationModel", "make_runtime_input_rebinding_presentation", "symbolic glyph lookup keys")
    "docs/superpowers/plans/README.md" = @("2026-05-08-runtime-input-rebinding-presentation-rows-v1.md", "Runtime Input Rebinding Presentation Rows v1", "RuntimeInputRebindingPresentationModel")
    "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md" = @("Runtime Input Rebinding Presentation Rows v1", "RuntimeInputRebindingPresentationToken", "make_runtime_input_rebinding_presentation", "platform input glyph generation")
}
foreach ($docPath in $runtimeInputRebindingPresentationRowsDocs.Keys) {
    $docText = Get-Content -LiteralPath (Join-Path $root $docPath) -Raw
    foreach ($needle in $runtimeInputRebindingPresentationRowsDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error "$docPath missing Runtime input rebinding presentation rows evidence: $needle"
        }
    }
}
$editorInputRebindingActionCaptureDocs = @{
    "docs/current-capabilities.md" = @("Editor Input Rebinding Action Capture Panel v1", "EditorInputRebindingCaptureModel", "in-memory profile")
    "docs/editor.md" = @("Editor Input Rebinding Action Capture Panel v1", "make_editor_input_rebinding_capture_action_model", "input_rebinding.capture")
    "docs/testing.md" = @("Editor Input Rebinding Action Capture Panel v1", "make_input_rebinding_capture_action_ui_model", "pressed-key candidate capture")
    "docs/roadmap.md" = @("EditorInputRebindingCaptureModel", "action-row capture controls", "in-memory profile")
    "docs/architecture.md" = @("reviewed editor action capture rows", "RuntimeInputStateView", "axis capture")
    "docs/ai-game-development.md" = @("EditorInputRebindingCaptureModel", "make_editor_input_rebinding_capture_action_model", "in-memory profile")
    "docs/superpowers/plans/README.md" = @("2026-05-08-editor-input-rebinding-action-capture-panel-v1.md", "Editor Input Rebinding Action Capture Panel v1", "in-memory profile")
    "docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md" = @("Editor Input Rebinding Action Capture Panel v1", "EditorInputRebindingCaptureModel", "reviewed editor action capture lane")
}
foreach ($docPath in $editorInputRebindingActionCaptureDocs.Keys) {
    $docText = Get-Content -LiteralPath (Join-Path $root $docPath) -Raw
    foreach ($needle in $editorInputRebindingActionCaptureDocs[$docPath]) {
        if (-not $docText.Contains($needle)) {
            Write-Error "$docPath missing Editor input rebinding action capture evidence: $needle"
        }
    }
}
$geUiHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/ui/include/mirakana/ui/ui.hpp") -Raw
$geUiSourceText = Get-Content -LiteralPath (Join-Path $root "engine/ui/src/ui.cpp") -Raw
$sourceImageDecodeHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/tools/include/mirakana/tools/source_image_decode.hpp") -Raw
$sourceImageDecodeSourceText = Get-Content -LiteralPath (Join-Path $root "engine/tools/asset/source_image_decode.cpp") -Raw
$uiAtlasToolHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") -Raw
$uiAtlasToolSourceText = Get-Content -LiteralPath (Join-Path $root "engine/tools/asset/ui_atlas_tool.cpp") -Raw
$toolsTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/tools_tests.cpp") -Raw
$uiRendererTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/ui_renderer_tests.cpp") -Raw
foreach ($needle in @(
    "AccessibilityPublishPlan",
    "AccessibilityPublishResult",
    "plan_accessibility_publish",
    "publish_accessibility_payload",
    "IAccessibilityAdapter",
    "ImeCompositionPublishPlan",
    "ImeCompositionPublishResult",
    "plan_ime_composition_update",
    "publish_ime_composition",
    "IImeAdapter",
    "PlatformTextInputSessionPlan",
    "PlatformTextInputSessionResult",
    "PlatformTextInputEndPlan",
    "PlatformTextInputEndResult",
    "begin_platform_text_input",
    "end_platform_text_input",
    "IPlatformIntegrationAdapter",
    "TextShapingRequestPlan",
    "TextShapingResult",
    "plan_text_shaping_request",
    "shape_text_run",
    "ITextShapingAdapter",
    "invalid_text_shaping_result",
    "FontRasterizationRequestPlan",
    "FontRasterizationResult",
    "plan_font_rasterization_request",
    "rasterize_font_glyph",
    "IFontRasterizerAdapter",
    "invalid_font_allocation",
    "ImageDecodeRequestPlan",
    "ImageDecodeDispatchResult",
    "ImageDecodePixelFormat",
    "plan_image_decode_request",
    "decode_image_request",
    "IImageDecodingAdapter",
    "invalid_image_decode_result"
)) {
    if (-not $geUiHeaderText.Contains($needle)) {
        Write-Error "engine/ui/include/mirakana/ui/ui.hpp missing runtime UI publish API: $needle"
    }
}
foreach ($needle in @(
    "AccessibilityPublishPlan::ready",
    "AccessibilityPublishResult::succeeded",
    "adapter.publish_nodes",
    "invalid_accessibility_bounds",
    "ImeCompositionPublishPlan::ready",
    "ImeCompositionPublishResult::succeeded",
    "adapter.update_composition",
    "invalid_ime_target",
    "invalid_ime_cursor",
    "PlatformTextInputSessionPlan::ready",
    "PlatformTextInputSessionResult::succeeded",
    "PlatformTextInputEndPlan::ready",
    "PlatformTextInputEndResult::succeeded",
    "adapter.begin_text_input",
    "adapter.end_text_input",
    "invalid_platform_text_input_target",
    "invalid_platform_text_input_bounds",
    "TextShapingRequestPlan::ready",
    "TextShapingResult::succeeded",
    "adapter.shape_text",
    "invalid_text_shaping_text",
    "invalid_text_shaping_font_family",
    "invalid_text_shaping_max_width",
    "invalid_text_shaping_result",
    "FontRasterizationRequestPlan::ready",
    "FontRasterizationResult::succeeded",
    "adapter.rasterize_glyph",
    "invalid_font_family",
    "invalid_font_glyph",
    "invalid_font_pixel_size",
    "invalid_font_allocation",
    "ImageDecodeRequestPlan::ready",
    "ImageDecodeDispatchResult::succeeded",
    "adapter.decode_image",
    "invalid_image_decode_uri",
    "empty_image_decode_bytes",
    "invalid_image_decode_result"
)) {
    if (-not $geUiSourceText.Contains($needle)) {
        Write-Error "engine/ui/src/ui.cpp missing runtime UI publish implementation evidence: $needle"
    }
}
foreach ($needle in @(
    "PngImageDecodingAdapter",
    "ui::IImageDecodingAdapter",
    "decode_image"
)) {
    if (-not $sourceImageDecodeHeaderText.Contains($needle)) {
        Write-Error "engine/tools/include/mirakana/tools/source_image_decode.hpp missing Runtime UI PNG adapter API: $needle"
    }
}
foreach ($needle in @(
    "PngImageDecodingAdapter::decode_image",
    "decode_audited_png_rgba8",
    "ImageDecodePixelFormat::rgba8_unorm",
    "catch (...)"
)) {
    if (-not $sourceImageDecodeSourceText.Contains($needle)) {
        Write-Error "engine/tools/asset/source_image_decode.cpp missing Runtime UI PNG adapter implementation evidence: $needle"
    }
}
foreach ($needle in @(
    "runtime UI PNG image decoding adapter fails closed when importers are disabled",
    "runtime UI PNG image decoding adapter returns rgba8 image when importers are enabled",
    "invalid_image_decode_result"
)) {
    if (-not $toolsTestsText.Contains($needle)) {
        Write-Error "tests/unit/tools_tests.cpp missing Runtime UI PNG adapter test evidence: $needle"
    }
}
foreach ($needle in @(
    "PackedUiAtlasAuthoringDesc",
    "PackedUiAtlasPackageUpdateDesc",
    "author_packed_ui_atlas_from_decoded_images",
    "plan_packed_ui_atlas_package_update",
    "apply_packed_ui_atlas_package_update"
)) {
    if (-not $uiAtlasToolHeaderText.Contains($needle)) {
        Write-Error "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp missing Runtime UI decoded atlas API: $needle"
    }
}
foreach ($needle in @(
    "PackedUiGlyphAtlasAuthoringDesc",
    "PackedUiGlyphAtlasPackageUpdateDesc",
    "author_packed_ui_glyph_atlas_from_rasterized_glyphs",
    "plan_packed_ui_glyph_atlas_package_update",
    "apply_packed_ui_glyph_atlas_package_update",
    "rasterized-glyph-adapter",
    "deterministic-glyph-atlas-rgba8-max-side"
)) {
    if (-not $uiAtlasToolHeaderText.Contains($needle)) {
        Write-Error "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp missing Runtime UI glyph atlas API: $needle"
    }
}
foreach ($needle in @(
    "pack_sprite_atlas_rgba8_max_side",
    "GameEngine.CookedTexture.v1",
    "decoded image must be RGBA8",
    "ui atlas page output path collides"
)) {
    if (-not $uiAtlasToolSourceText.Contains($needle)) {
        Write-Error "engine/tools/asset/ui_atlas_tool.cpp missing Runtime UI decoded atlas implementation evidence: $needle"
    }
}
foreach ($needle in @(
    "rasterized glyph must be RGBA8",
    "ui atlas page output path collides"
)) {
    if (-not $uiAtlasToolSourceText.Contains($needle)) {
        Write-Error "engine/tools/asset/ui_atlas_tool.cpp missing Runtime UI glyph atlas implementation evidence: $needle"
    }
}
foreach ($needle in @(
    "packed runtime UI atlas authoring maps decoded images into texture page and metadata",
    "packed runtime UI atlas package update writes texture page metadata and package index",
    "packed runtime UI atlas rejects invalid decoded images and package path collisions",
    "packed runtime UI atlas apply leaves existing files unchanged when validation fails"
)) {
    if (-not $toolsTestsText.Contains($needle)) {
        Write-Error "tests/unit/tools_tests.cpp missing Runtime UI decoded atlas test evidence: $needle"
    }
}
foreach ($needle in @(
    "packed runtime UI glyph atlas authoring maps rasterized glyphs into texture page and metadata",
    "packed runtime UI glyph atlas package update writes texture page metadata and package index",
    "packed runtime UI glyph atlas rejects invalid glyph pixels and package path collisions",
    "packed runtime UI glyph atlas apply leaves existing files unchanged when validation fails"
)) {
    if (-not $toolsTestsText.Contains($needle)) {
        Write-Error "tests/unit/tools_tests.cpp missing Runtime UI glyph atlas test evidence: $needle"
    }
}
foreach ($needle in @(
    "CapturingAccessibilityAdapter",
    "ui accessibility publish plan dispatches validated nodes to adapter",
    "ui accessibility publish plan blocks invalid nodes before adapter",
    "CapturingImeAdapter",
    "ui ime composition publish plan dispatches valid composition to adapter",
    "ui ime composition publish plan blocks invalid composition before adapter",
    "CapturingFontRasterizerAdapter",
    "InvalidFontRasterizerAdapter",
    "ui font rasterization request plan dispatches valid request to adapter",
    "ui font rasterization request plan blocks invalid request before adapter",
    "ui font rasterization result reports invalid adapter allocation",
    "CapturingTextShapingAdapter",
    "ui text shaping request plan dispatches valid request to adapter",
    "ui text shaping request plan blocks invalid request before adapter",
    "ui text shaping result reports invalid adapter runs",
    "CapturingImageDecodingAdapter",
    "ui image decode request plan dispatches valid request to adapter",
    "ui image decode request plan blocks invalid request before adapter",
    "ui image decode result reports missing or invalid adapter output"
)) {
    if (-not $uiRendererTestsText.Contains($needle)) {
        Write-Error "tests/unit/ui_renderer_tests.cpp missing runtime UI publish test evidence: $needle"
    }
}

$geRhiModule = @($engine.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($geRhiModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_rhi module"
}
if (@($geRhiModule[0].publicHeaders) -notcontains "engine/rhi/include/mirakana/rhi/resource_lifetime.hpp") {
    Write-Error "engine manifest MK_rhi publicHeaders must include resource_lifetime.hpp"
}
if (@($geRhiModule[0].publicHeaders) -notcontains "engine/rhi/include/mirakana/rhi/upload_staging.hpp") {
    Write-Error "engine manifest MK_rhi publicHeaders must include upload_staging.hpp"
}
if (-not ([string]$geRhiModule[0].purpose).Contains("RhiResourceLifetimeRegistry") -or
    -not ([string]$geRhiModule[0].purpose).Contains("RhiUploadStagingPlan") -or
    -not ([string]$geRhiModule[0].purpose).Contains("FenceValue") -or
    -not ([string]$geRhiModule[0].purpose).Contains("foundation-only") -or
    -not ([string]$geRhiModule[0].purpose).Contains("GPU allocator")) {
    Write-Error "engine manifest MK_rhi purpose must describe foundation-only resource lifetime/upload staging and remaining GPU allocator limits"
}

foreach ($hostGate in $productionLoop.hostGates) {
    Assert-Properties $hostGate @("id", "status", "hosts", "validationRecipes", "notes") "engine manifest aiOperableProductionLoop hostGates"
    foreach ($validationRecipe in @($hostGate.validationRecipes)) {
        if (-not $validationRecipeNames.ContainsKey($validationRecipe)) {
            Write-Error "engine manifest aiOperableProductionLoop host gate '$($hostGate.id)' references unknown validation recipe: $validationRecipe"
        }
    }
}
$vulkanGate = @($productionLoop.hostGates | Where-Object { $_.id -eq "vulkan-strict" })
if ($vulkanGate.Count -ne 1 -or $vulkanGate[0].status -ne "host-gated") {
    Write-Error "engine manifest aiOperableProductionLoop must keep vulkan-strict host-gated"
}
$metalGate = @($productionLoop.hostGates | Where-Object { $_.id -eq "metal-apple" })
if ($metalGate.Count -ne 1 -or $metalGate[0].status -ne "host-gated") {
    Write-Error "engine manifest aiOperableProductionLoop must keep metal-apple host-gated"
}

$recipeMapIds = @{}
foreach ($map in $productionLoop.validationRecipeMap) {
    Assert-Properties $map @("recipeId", "validationRecipes") "engine manifest aiOperableProductionLoop validationRecipeMap"
    if (-not $productionRecipeIds.ContainsKey($map.recipeId)) {
        Write-Error "engine manifest aiOperableProductionLoop validationRecipeMap references unknown recipe: $($map.recipeId)"
    }
    $recipeMapIds[$map.recipeId] = $true
    foreach ($validationRecipe in @($map.validationRecipes)) {
        if (-not $validationRecipeNames.ContainsKey($validationRecipe)) {
            Write-Error "engine manifest aiOperableProductionLoop validationRecipeMap '$($map.recipeId)' references unknown validation recipe: $validationRecipe"
        }
    }
}
foreach ($recipeId in $expectedProductionRecipeIds) {
    if (-not $recipeMapIds.ContainsKey($recipeId)) {
        Write-Error "engine manifest aiOperableProductionLoop validationRecipeMap missing recipe id: $recipeId"
    }
}
Assert-Properties $engine.aiDrivenGameWorkflow @("steps", "templateSpec", "validationScenarios", "promptPack", "sampleGame") "engine manifest aiDrivenGameWorkflow"
foreach ($workflowDoc in @($engine.aiDrivenGameWorkflow.templateSpec, $engine.aiDrivenGameWorkflow.validationScenarios, $engine.aiDrivenGameWorkflow.promptPack)) {
    if (-not (Test-Path (Join-Path $root $workflowDoc))) {
        Write-Error "engine manifest aiDrivenGameWorkflow references missing document: $workflowDoc"
    }
}

$desktopRuntimeGameRegistrations = Get-DesktopRuntimeGameRegistrations

Get-ChildItem -Path (Join-Path $root "games") -Recurse -Filter "game.agent.json" | ForEach-Object {
    $relative = Get-RelativeRepoPath $_.FullName
    $game = Get-Content -LiteralPath $_.FullName -Raw | ConvertFrom-Json
    Assert-Properties $game @("schemaVersion", "name", "displayName", "language", "entryPoint", "target", "engineModules", "aiWorkflow", "gameplayContract", "backendReadiness", "importerRequirements", "packagingTargets", "validationRecipes") $relative
    Assert-Properties $game.backendReadiness @("platform", "graphics", "audio", "ui") "$relative backendReadiness"
    Assert-Properties $game.importerRequirements @("sourceFormats", "cookedOnlyRuntime", "externalImportersRequired") "$relative importerRequirements"
    if ($game.language -ne "C++23") {
        Write-Error "$relative must declare language C++23"
    }
    if ($game.name -notmatch "^[a-z][a-z0-9-]*$") {
        Write-Error "$relative has invalid game name: $($game.name)"
    }
    if ($game.target -notmatch "^[a-z][a-z0-9_]*$") {
        Write-Error "$relative has invalid target: $($game.target)"
    }
    if (-not (Test-Path (Join-Path $root $game.entryPoint))) {
        Write-Error "$relative references missing entryPoint: $($game.entryPoint)"
    }
    foreach ($module in $game.engineModules) {
        if (-not $moduleNames.ContainsKey($module)) {
            Write-Error "$relative references unknown engine module: $module"
        }
    }
    if (-not $game.importerRequirements.cookedOnlyRuntime) {
        Write-Error "$relative must keep cookedOnlyRuntime enabled"
    }
    foreach ($target in $game.packagingTargets) {
        if (-not $packagingTargetNames.ContainsKey($target)) {
            Write-Error "$relative references unknown packaging target: $target"
        }
    }
    $selectsDesktopGameRuntime = @($game.packagingTargets) -contains "desktop-game-runtime"
    $selectsDesktopRuntimeRelease = @($game.packagingTargets) -contains "desktop-runtime-release"
    if ($selectsDesktopRuntimeRelease -and -not $selectsDesktopGameRuntime) {
        Write-Error "$relative declares desktop-runtime-release but does not declare desktop-game-runtime"
    }
    if ($selectsDesktopGameRuntime) {
        if (-not $desktopRuntimeGameRegistrations.ContainsKey($game.target)) {
            Write-Error "$relative declares desktop-game-runtime but target '$($game.target)' is not registered with MK_add_desktop_runtime_game in games/CMakeLists.txt"
        }
        $registration = $desktopRuntimeGameRegistrations[$game.target]
        if (-not $registration.hasSmokeArgs) {
            Write-Error "$relative desktop runtime target '$($game.target)' must declare SMOKE_ARGS in MK_add_desktop_runtime_game"
        }
        $registeredManifest = Assert-DesktopRuntimeGameManifestPath $registration.gameManifest "desktop runtime target '$($game.target)'"
        if ($registeredManifest -ne $relative) {
            Write-Error "$relative desktop runtime target '$($game.target)' is registered with GAME_MANIFEST '$registeredManifest'"
        }
        Assert-DesktopRuntimePackageFileRegistration $game $relative $registration
    }
    if ($selectsDesktopRuntimeRelease) {
        Assert-DesktopRuntimePackageRecipe $game $relative
    }
    $declaredRuntimeFiles = @($game.runtimePackageFiles)
    $hasRuntimeScenePackage = @($declaredRuntimeFiles | Where-Object { ([string]$_).EndsWith(".geindex") }).Count -gt 0 -and
        @($declaredRuntimeFiles | Where-Object { ([string]$_).EndsWith(".scene") }).Count -gt 0
    Assert-RuntimeSceneValidationTargets $game $relative $hasRuntimeScenePackage
    $sourceFormats = @($game.importerRequirements.sourceFormats)
    $requiresMaterialShaderTargets = $sourceFormats -contains "first-party-material-source" -and $sourceFormats -contains "hlsl-source"
    Assert-MaterialShaderAuthoringTargets $game $relative $requiresMaterialShaderTargets
    $requiresAtlasTilemapTargets = $game.gameplayContract.productionRecipe -eq "2d-desktop-runtime-package"
    Assert-AtlasTilemapAuthoringTargets $game $relative $requiresAtlasTilemapTargets
    $requiresPrefabScene3dTargets = $game.gameplayContract.productionRecipe -eq "3d-playable-desktop-package"
    Assert-PrefabScenePackageAuthoringTargets $game $relative $requiresPrefabScene3dTargets
    Assert-RegisteredSourceAssetCookTargets $game $relative $requiresPrefabScene3dTargets
    Assert-PackageStreamingResidencyTargets $game $relative $hasRuntimeScenePackage
    foreach ($recipe in $game.validationRecipes) {
        Assert-Properties $recipe @("name", "command") "$relative validationRecipes"
        if ([string]::IsNullOrWhiteSpace($recipe.command)) {
            Write-Error "$relative validation recipe '$($recipe.name)' must declare a command"
        }
    }
}

$sample2dManifestPath = "games/sample_2d_playable_foundation/game.agent.json"
$sample2dManifestFullPath = Join-Path $root $sample2dManifestPath
if (-not (Test-Path $sample2dManifestFullPath)) {
    Write-Error "2d-playable-source-tree recipe must have a sample game manifest: $sample2dManifestPath"
} else {
    $sample2dManifest = Get-Content -LiteralPath $sample2dManifestFullPath -Raw | ConvertFrom-Json
    if ($sample2dManifest.target -ne "sample_2d_playable_foundation") {
        Write-Error "$sample2dManifestPath target must be sample_2d_playable_foundation"
    }
    if ($sample2dManifest.gameplayContract.productionRecipe -ne "2d-playable-source-tree") {
        Write-Error "$sample2dManifestPath gameplayContract.productionRecipe must be 2d-playable-source-tree"
    }
    foreach ($module in @("MK_runtime", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_renderer")) {
        if (@($sample2dManifest.engineModules) -notcontains $module) {
            Write-Error "$sample2dManifestPath engineModules missing $module"
        }
    }
    if (@($sample2dManifest.packagingTargets) -notcontains "source-tree-default") {
        Write-Error "$sample2dManifestPath must use source-tree-default packaging target"
    }
    if (@($sample2dManifest.packagingTargets) -contains "desktop-game-runtime") {
        Write-Error "$sample2dManifestPath must not claim desktop-game-runtime readiness in the source-tree 2D slice"
    }
}

$sample2dDesktopManifestPath = "games/sample_2d_desktop_runtime_package/game.agent.json"
$sample2dDesktopManifestFullPath = Join-Path $root $sample2dDesktopManifestPath
if (-not (Test-Path $sample2dDesktopManifestFullPath)) {
    Write-Error "2d-desktop-runtime-package recipe must have a sample game manifest: $sample2dDesktopManifestPath"
} else {
    $sample2dDesktopManifest = Get-Content -LiteralPath $sample2dDesktopManifestFullPath -Raw | ConvertFrom-Json
    if ($sample2dDesktopManifest.target -ne "sample_2d_desktop_runtime_package") {
        Write-Error "$sample2dDesktopManifestPath target must be sample_2d_desktop_runtime_package"
    }
    if ($sample2dDesktopManifest.gameplayContract.productionRecipe -ne "2d-desktop-runtime-package") {
        Write-Error "$sample2dDesktopManifestPath gameplayContract.productionRecipe must be 2d-desktop-runtime-package"
    }
    foreach ($module in @("MK_runtime", "MK_runtime_scene", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_renderer")) {
        if (@($sample2dDesktopManifest.engineModules) -notcontains $module) {
            Write-Error "$sample2dDesktopManifestPath engineModules missing $module"
        }
    }
    foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
        if (@($sample2dDesktopManifest.packagingTargets) -notcontains $target) {
            Write-Error "$sample2dDesktopManifestPath packagingTargets missing $target"
        }
    }
    foreach ($packageFile in @(
        "runtime/sample_2d_desktop_runtime_package.config",
        "runtime/sample_2d_desktop_runtime_package.geindex",
        "runtime/assets/2d/player.texture.geasset",
        "runtime/assets/2d/player.material",
        "runtime/assets/2d/jump.audio.geasset",
        "runtime/assets/2d/level.tilemap",
        "runtime/assets/2d/playable.scene"
    )) {
        if (@($sample2dDesktopManifest.runtimePackageFiles) -notcontains $packageFile) {
            Write-Error "$sample2dDesktopManifestPath runtimePackageFiles missing $packageFile"
        }
    }
    if (-not $desktopRuntimeGameRegistrations.ContainsKey($sample2dDesktopManifest.target)) {
        Write-Error "$sample2dDesktopManifestPath target must be registered with MK_add_desktop_runtime_game"
    }
    $sample2dManifestText = Get-Content -LiteralPath $sample2dDesktopManifestFullPath -Raw
    foreach ($needle in @(
        "D3D12 package window smoke",
        "Vulkan package window smoke",
        "native 2D sprite package proof",
        "installed-d3d12-window-smoke",
        "installed-vulkan-window-smoke",
        "installed-native-2d-sprite-smoke",
        "--require-d3d12-shaders",
        "--require-d3d12-renderer",
        "--require-vulkan-shaders",
        "--require-vulkan-renderer",
        "--require-native-2d-sprites",
        "public native or RHI handle access remains unsupported",
        "broad production sprite batching readiness remains unsupported",
        "general production renderer quality remains unsupported"
    )) {
        if (-not $sample2dManifestText.Contains($needle)) {
            Write-Error "$sample2dDesktopManifestPath missing 2D native window presentation contract text: $needle"
        }
    }
    $sample2dMainPath = Join-Path $root "games/sample_2d_desktop_runtime_package/main.cpp"
    $sample2dMainText = Get-Content -LiteralPath $sample2dMainPath -Raw
    foreach ($needle in @(
        "mirakana/runtime_host/shader_bytecode.hpp",
        "mirakana/renderer/sprite_batch.hpp",
        "--require-d3d12-shaders",
        "--require-d3d12-renderer",
        "--require-vulkan-shaders",
        "--require-vulkan-renderer",
        "--require-native-2d-sprites",
        "sample_2d_desktop_runtime_package_sprite.vs.dxil",
        "sample_2d_desktop_runtime_package_sprite.ps.dxil",
        "sample_2d_desktop_runtime_package_sprite.vs.spv",
        "sample_2d_desktop_runtime_package_sprite.ps.spv",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.dxil",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.dxil",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.spv",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.spv",
        "native_2d_sprites_status",
        "native_2d_textured_sprites_submitted",
        "native_2d_texture_binds",
        "plan_scene_sprite_batches",
        "sprite_batch_plan_draws",
        "sprite_batch_plan_texture_binds",
        "sprite_batch_plan_diagnostics",
        "required_native_2d_sprites_unavailable",
        "required_d3d12_renderer_unavailable",
        "required_vulkan_renderer_unavailable"
    )) {
        if (-not $sample2dMainText.Contains($needle)) {
            Write-Error "games/sample_2d_desktop_runtime_package/main.cpp missing native presentation smoke field: $needle"
        }
    }
    $gamesCMakeText = Get-Content -LiteralPath (Join-Path $root "games/CMakeLists.txt") -Raw
    foreach ($needle in @(
        "sample_2d_desktop_runtime_package_sprite.vs.dxil",
        "sample_2d_desktop_runtime_package_sprite.ps.dxil",
        "sample_2d_desktop_runtime_package_sprite.vs.spv",
        "sample_2d_desktop_runtime_package_sprite.ps.spv",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.dxil",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.dxil",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.spv",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.spv",
        "--require-native-2d-sprites",
        "sample_2d_desktop_runtime_package_shaders",
        "sample_2d_desktop_runtime_package_vulkan_shaders",
        "REQUIRES_D3D12_SHADERS"
    )) {
        if (-not $gamesCMakeText.Contains($needle)) {
            Write-Error "games/CMakeLists.txt missing 2D native presentation package metadata: $needle"
        }
    }
    $installedDesktopRuntimeValidationText = Get-Content -LiteralPath (Join-Path $root "tools/validate-installed-desktop-runtime.ps1") -Raw
    foreach ($needle in @(
        "--require-native-2d-sprites",
        "native_2d_sprites_status",
        "native_2d_textured_sprites_submitted",
        "native_2d_texture_binds",
        "sprite_batch_plan_draws",
        "sprite_batch_plan_texture_binds",
        "sprite_batch_plan_diagnostics"
    )) {
        if (-not $installedDesktopRuntimeValidationText.Contains($needle)) {
            Write-Error "tools/validate-installed-desktop-runtime.ps1 missing 2D native sprite package validation field: $needle"
        }
    }
    $newGameText = Get-Content -LiteralPath (Join-Path $root "tools/new-game.ps1") -Raw
    foreach ($needle in @(
        "shaders/runtime_2d_sprite.hlsl",
        "installed-native-2d-sprite-smoke",
        "--require-native-2d-sprites",
        "MK_configure_desktop_runtime_2d_sprite_shader_artifacts"
    )) {
        if (-not $newGameText.Contains($needle)) {
            Write-Error "tools/new-game.ps1 missing generated 2D native sprite scaffold contract: $needle"
        }
    }
    if (-not $gamesCMakeText.Contains("function(MK_configure_desktop_runtime_2d_sprite_shader_artifacts)")) {
        Write-Error "games/CMakeLists.txt missing generated 2D sprite shader artifact helper"
    }
}

$spriteBatchHeaderPath = Join-Path $root "engine/renderer/include/mirakana/renderer/sprite_batch.hpp"
$spriteBatchSourcePath = Join-Path $root "engine/renderer/src/sprite_batch.cpp"
if (-not (Test-Path -LiteralPath $spriteBatchHeaderPath)) {
    Write-Error "Missing 2D sprite batch planning header"
}
if (-not (Test-Path -LiteralPath $spriteBatchSourcePath)) {
    Write-Error "Missing 2D sprite batch planning source"
}
$spriteBatchHeaderText = Get-Content -LiteralPath $spriteBatchHeaderPath -Raw
$spriteBatchSourceText = Get-Content -LiteralPath $spriteBatchSourcePath -Raw
$sceneRendererHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp") -Raw
$sceneRendererSourceText = Get-Content -LiteralPath (Join-Path $root "engine/scene_renderer/src/scene_renderer.cpp") -Raw
$sceneRendererTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/scene_renderer_tests.cpp") -Raw
$rendererTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/renderer_rhi_tests.cpp") -Raw
$frameGraphHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/include/mirakana/renderer/frame_graph.hpp") -Raw
$frameGraphSourceText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/src/frame_graph.cpp") -Raw
$frameGraphRhiHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp") -Raw
$frameGraphRhiSourceText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/src/frame_graph_rhi.cpp") -Raw
$rhiUploadStagingHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/rhi/include/mirakana/rhi/upload_staging.hpp") -Raw
$rhiUploadStagingSourceText = Get-Content -LiteralPath (Join-Path $root "engine/rhi/src/upload_staging.cpp") -Raw
$rhiUploadStagingTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/rhi_upload_staging_tests.cpp") -Raw
$runtimeRhiUploadHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp") -Raw
$runtimeRhiUploadSourceText = Get-Content -LiteralPath (Join-Path $root "engine/runtime_rhi/src/runtime_upload.cpp") -Raw
$runtimeSceneRhiHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp") -Raw
$runtimeSceneRhiSourceText = Get-Content -LiteralPath (Join-Path $root "engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp") -Raw
$runtimeRhiTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/runtime_rhi_tests.cpp") -Raw
$runtimeSceneRhiTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/runtime_scene_rhi_tests.cpp") -Raw
$runtimeUploadFencePlanText = Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-runtime-rhi-upload-submission-fence-rows-v1.md") -Raw
$frameGraphRhiTextureSchedulePlanText =
    Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-frame-graph-rhi-texture-schedule-execution-v1.md") -Raw
$rhiUploadStaleGenerationPlanText =
    Get-Content -LiteralPath (Join-Path $root "docs/superpowers/plans/2026-05-08-rhi-upload-stale-generation-diagnostics-v1.md") -Raw
$rendererCmakeText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/CMakeLists.txt") -Raw
$engineManifestText = Get-Content -LiteralPath (Join-Path $root "engine/agent/manifest.json") -Raw
foreach ($needle in @("FrameGraphPassExecutionBinding", "FrameGraphExecutionCallbacks", "FrameGraphExecutionResult", "execute_frame_graph_v1_schedule")) {
    if (-not $frameGraphHeaderText.Contains($needle)) {
        Write-Error "Frame Graph callback execution header missing contract text: $needle"
    }
}
foreach ($needle in @(
    "frame graph pass callback is missing",
    "frame graph barrier callback is missing",
    "frame graph pass callback threw an exception",
    "frame graph barrier callback threw an exception",
    "frame graph pass callback failed",
    "frame graph barrier callback failed"
)) {
    if (-not $frameGraphSourceText.Contains($needle)) {
        Write-Error "Frame Graph callback execution source missing diagnostic text: $needle"
    }
}
foreach ($needle in @(
    "frame graph v1 dispatches barrier and pass callbacks in schedule order",
    "frame graph v1 callback execution diagnoses missing callbacks before later passes",
    "frame graph v1 callback execution converts thrown callbacks to diagnostics",
    "frame graph v1 callback execution copies pass bindings before dispatch",
    "frame graph v1 callback execution reports returned callback failures",
    "frame graph v1 callback execution converts thrown barrier callbacks to diagnostics"
)) {
    if (-not $rendererTestsText.Contains($needle)) {
        Write-Error "MK_renderer_tests missing Frame Graph callback execution coverage: $needle"
    }
}
foreach ($needle in @("FrameGraphRhiTextureExecutionDesc", "FrameGraphRhiTextureExecutionResult", "execute_frame_graph_rhi_texture_schedule")) {
    if (-not $frameGraphRhiHeaderText.Contains($needle)) {
        Write-Error "Frame Graph RHI texture schedule execution header missing contract text: $needle"
    }
}
foreach ($needle in @(
    "frame graph rhi texture schedule execution requires a command list",
    "frame graph rhi texture schedule execution cannot use a closed command list",
    "frame graph pass callback is empty",
    "frame graph pass callback is declared more than once",
    "frame graph texture barrier recording failed"
)) {
    if (-not $frameGraphRhiSourceText.Contains($needle)) {
        Write-Error "Frame Graph RHI texture schedule execution source missing diagnostic text: $needle"
    }
}
foreach ($needle in @(
    "frame graph rhi texture schedule execution interleaves barriers and pass callbacks",
    "frame graph rhi texture schedule execution validates barriers before pass callbacks",
    "frame graph rhi texture schedule execution validates pass callbacks before barriers"
)) {
    if (-not $rendererTestsText.Contains($needle)) {
        Write-Error "MK_renderer_tests missing Frame Graph RHI texture schedule execution coverage: $needle"
    }
}
foreach ($needle in @("**Status:** Completed.", "FrameGraphRhiTextureExecutionDesc", "FrameGraphRhiTextureExecutionResult", "execute_frame_graph_rhi_texture_schedule")) {
    if (-not $frameGraphRhiTextureSchedulePlanText.Contains($needle)) {
        Write-Error "Frame Graph RHI texture schedule execution plan missing contract text: $needle"
    }
}
foreach ($needle in @(
    "submitted_fence",
    "RuntimeTextureUploadResult",
    "RuntimeMeshUploadResult",
    "RuntimeSkinnedMeshUploadResult",
    "RuntimeMorphMeshUploadResult",
    "RuntimeMaterialGpuBinding"
)) {
    if (-not $runtimeRhiUploadHeaderText.Contains($needle)) {
        Write-Error "Runtime RHI upload submission fence header missing contract text: $needle"
    }
}
foreach ($needle in @("submitted_upload_fences", "submitted_upload_fence_count", "last_submitted_upload_fence")) {
    if (-not $runtimeSceneRhiHeaderText.Contains($needle)) {
        Write-Error "Runtime scene RHI upload execution report header missing contract text: $needle"
    }
}
foreach ($needle in @(
    "result.submitted_fence = fence",
    "return RuntimeTextureUploadResult",
    "return RuntimeMeshUploadResult",
    "return RuntimeSkinnedMeshUploadResult",
    "return RuntimeMorphMeshUploadResult"
)) {
    if (-not $runtimeRhiUploadSourceText.Contains($needle)) {
        Write-Error "Runtime RHI upload submission fence source missing contract text: $needle"
    }
}
foreach ($needle in @(
    "record_submitted_upload_fence",
    "result.submitted_upload_fences.push_back",
    "upload.submitted_fence",
    "base_upload.submitted_fence",
    "binding.submitted_fence",
    "bindings.submitted_upload_fences"
)) {
    if (-not $runtimeSceneRhiSourceText.Contains($needle)) {
        Write-Error "Runtime scene RHI upload fence aggregation source missing contract text: $needle"
    }
}
foreach ($needle in @(
    "runtime rhi upload reports submitted fence without forcing wait",
    "result.submitted_fence.value != 0",
    "binding.submitted_fence.value != 0",
    "upload.submitted_fence.value != 0"
)) {
    if (-not $runtimeRhiTestsText.Contains($needle)) {
        Write-Error "MK_runtime_rhi_tests missing upload submission fence coverage: $needle"
    }
}
foreach ($needle in @(
    "runtime scene rhi upload execution preserves submitted fences in submit order across queues",
    "submitted_upload_fences[0].queue == mirakana::rhi::QueueKind::compute",
    "submitted_upload_fences[1].value == compute_resource.base_position_upload.submitted_fence.value",
    "submitted_upload_fence_count == 3",
    "submitted_upload_fence_count == 4",
    "last_submitted_upload_fence.value != 0"
)) {
    if (-not $runtimeSceneRhiTestsText.Contains($needle)) {
        Write-Error "MK_runtime_scene_rhi_tests missing upload submission fence coverage: $needle"
    }
}
foreach ($needle in @(
    "**Status:** Completed",
    "Runtime RHI Upload Submission Fence Rows v1",
    "submitted_upload_fences",
    "submitted_upload_fence_count",
    "native async upload execution"
)) {
    if (-not $runtimeUploadFencePlanText.Contains($needle)) {
        Write-Error "Runtime RHI Upload Submission Fence Rows plan missing text: $needle"
    }
}
foreach ($needle in @("RhiUploadDiagnosticCode", "stale_generation", "RhiUploadRing")) {
    if (-not $rhiUploadStagingHeaderText.Contains($needle)) {
        Write-Error "RHI upload staging header missing stale-generation contract text: $needle"
    }
}
foreach ($needle in @(
    "inactive_allocation_code",
    "RHI upload staging allocation generation is stale.",
    "span.allocation_generation",
    "RhiUploadDiagnosticCode::stale_generation"
)) {
    if (-not $rhiUploadStagingSourceText.Contains($needle)) {
        Write-Error "RHI upload staging source missing stale-generation implementation text: $needle"
    }
}
foreach ($needle in @(
    "rhi upload staging diagnoses stale allocation generations",
    "rhi upload ring ownership requires matching allocation generation",
    "RhiUploadDiagnosticCode::stale_generation",
    "!ring.owns_allocation(stale)"
)) {
    if (-not $rhiUploadStagingTestsText.Contains($needle)) {
        Write-Error "MK_rhi_upload_staging_tests missing stale-generation coverage: $needle"
    }
}
foreach ($needle in @("**Status:** Completed.", "RHI Upload Stale Generation Diagnostics v1", "stale_generation", "native async upload execution")) {
    if (-not $rhiUploadStaleGenerationPlanText.Contains($needle)) {
        Write-Error "RHI Upload Stale Generation Diagnostics plan missing text: $needle"
    }
}
foreach ($needle in @("SpriteBatchPlan", "SpriteBatchRange", "SpriteBatchDiagnosticCode", "plan_sprite_batches")) {
    if (-not $spriteBatchHeaderText.Contains($needle)) {
        Write-Error "2D sprite batch planning header missing contract text: $needle"
    }
}
foreach ($needle in @("append_or_extend_batch", "missing_texture_atlas", "invalid_uv_rect", "texture_bind_count")) {
    if (-not $spriteBatchSourceText.Contains($needle)) {
        Write-Error "2D sprite batch planning source missing contract text: $needle"
    }
}
foreach ($needle in @(
    "sprite batch planner preserves order",
    "sprite batch planner diagnoses invalid texture metadata",
    "SpriteBatchDiagnosticCode::missing_texture_atlas",
    "SpriteBatchDiagnosticCode::invalid_uv_rect"
)) {
    if (-not $rendererTestsText.Contains($needle)) {
        Write-Error "MK_renderer_tests missing 2D sprite batch planning coverage: $needle"
    }
}
foreach ($needle in @("scene sprite batch telemetry", "plan_scene_sprite_batches")) {
    if (-not $sceneRendererTestsText.Contains($needle)) {
        Write-Error "MK_scene_renderer_tests missing 2D sprite batch telemetry coverage: $needle"
    }
}
if (-not $rendererCmakeText.Contains("src/sprite_batch.cpp")) {
    Write-Error "MK_renderer CMake missing sprite_batch.cpp"
}
foreach ($needle in @("mirakana/renderer/sprite_batch.hpp", "plan_scene_sprite_batches")) {
    if (-not $sceneRendererHeaderText.Contains($needle)) {
        Write-Error "MK_scene_renderer header missing 2D sprite batch telemetry contract: $needle"
    }
    if (-not $sceneRendererSourceText.Contains($needle)) {
        Write-Error "MK_scene_renderer source missing 2D sprite batch telemetry contract: $needle"
    }
}
foreach ($needle in @(
    "2d-sprite-batch-planning-contract",
    "2d-sprite-batch-package-telemetry",
    "plan_sprite_batches",
    "plan_scene_sprite_batches",
    "production sprite batching readiness",
    "native_sprite_batches_executed"
)) {
    if (-not $engineManifestText.Contains($needle)) {
        Write-Error "engine manifest missing sprite batch planning contract text: $needle"
    }
}
if ($engineManifestText.Contains("production sprite batching ready")) {
    Write-Error "engine manifest must not claim production sprite batching ready"
}

$sample3dManifestPath = "games/sample_desktop_runtime_game/game.agent.json"
$sample3dManifestFullPath = Join-Path $root $sample3dManifestPath
if (-not (Test-Path $sample3dManifestFullPath)) {
    Write-Error "3d-playable-desktop-package recipe must have a sample game manifest: $sample3dManifestPath"
} else {
    $sample3dManifest = Get-Content -LiteralPath $sample3dManifestFullPath -Raw | ConvertFrom-Json
    if ($sample3dManifest.target -ne "sample_desktop_runtime_game") {
        Write-Error "$sample3dManifestPath target must be sample_desktop_runtime_game"
    }
    if ($sample3dManifest.gameplayContract.productionRecipe -ne "3d-playable-desktop-package") {
        Write-Error "$sample3dManifestPath gameplayContract.productionRecipe must be 3d-playable-desktop-package"
    }
    foreach ($module in @("MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_sdl3", "MK_runtime_host_sdl3_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($sample3dManifest.engineModules) -notcontains $module) {
            Write-Error "$sample3dManifestPath engineModules missing $module"
        }
    }
    foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
        if (@($sample3dManifest.packagingTargets) -notcontains $target) {
            Write-Error "$sample3dManifestPath packagingTargets missing $target"
        }
    }
    foreach ($packageFile in @(
        "runtime/sample_desktop_runtime_game.config",
        "runtime/sample_desktop_runtime_game.geindex",
        "runtime/assets/desktop_runtime/base_color.texture.geasset",
        "runtime/assets/desktop_runtime/hud.uiatlas",
        "runtime/assets/desktop_runtime/skinned_triangle.skinned_mesh",
        "runtime/assets/desktop_runtime/unlit.material",
        "runtime/assets/desktop_runtime/packaged_scene.scene"
    )) {
        if (@($sample3dManifest.runtimePackageFiles) -notcontains $packageFile) {
            Write-Error "$sample3dManifestPath runtimePackageFiles missing $packageFile"
        }
    }
    if (-not $desktopRuntimeGameRegistrations.ContainsKey($sample3dManifest.target)) {
        Write-Error "$sample3dManifestPath target must be registered with MK_add_desktop_runtime_game"
    }
    $sample3dManifestText = Get-Content -LiteralPath $sample3dManifestFullPath -Raw
    foreach ($needle in @(
        "material instance intent",
        "camera/controller movement",
        "HUD diagnostics",
        "runtime source asset parsing remains unsupported",
        "material graph remains unsupported",
        "skeletal animation production path remains unsupported",
        "GPU skinning is host-proven on the D3D12 package smoke lane",
        "sample_desktop_runtime_game --require-gpu-skinning",
        "package streaming remains unsupported",
        "native GPU runtime UI overlay",
        "textured UI sprite atlas",
        "production text/font/image/atlas/accessibility remains unsupported",
        "public native or RHI handle access remains unsupported",
        "general production renderer quality remains unsupported"
    )) {
        if (-not $sample3dManifestText.Contains($needle)) {
            Write-Error "$sample3dManifestPath missing 3D boundary text: $needle"
        }
    }
    if ($sample3dManifestText.Contains("native GPU HUD or sprite overlay output remains unsupported")) {
        Write-Error "$sample3dManifestPath keeps a stale native GPU HUD or sprite overlay unsupported claim"
    }
    $sample3dUiAtlasPath = Join-Path $root "games/sample_desktop_runtime_game/runtime/assets/desktop_runtime/hud.uiatlas"
    $sample3dUiAtlasText = Get-Content -LiteralPath $sample3dUiAtlasPath -Raw
    foreach ($needle in @("format=GameEngine.UiAtlas.v1", "source.decoding=unsupported", "atlas.packing=unsupported", "page.count=1", "image.count=1")) {
        if (-not $sample3dUiAtlasText.Contains($needle)) {
            Write-Error "games/sample_desktop_runtime_game/runtime/assets/desktop_runtime/hud.uiatlas missing cooked UI atlas metadata text: $needle"
        }
    }
    $sample3dIndexPath = Join-Path $root "games/sample_desktop_runtime_game/runtime/sample_desktop_runtime_game.geindex"
    $sample3dIndexText = Get-Content -LiteralPath $sample3dIndexPath -Raw
    foreach ($needle in @("kind=ui_atlas", "kind=ui_atlas_texture")) {
        if (-not $sample3dIndexText.Contains($needle)) {
            Write-Error "games/sample_desktop_runtime_game/runtime/sample_desktop_runtime_game.geindex missing UI atlas package row: $needle"
        }
    }
    $sample3dMainPath = Join-Path $root "games/sample_desktop_runtime_game/main.cpp"
    $sample3dMainText = Get-Content -LiteralPath $sample3dMainPath -Raw
    foreach ($needle in @(
        "mirakana/ui/ui.hpp",
        "mirakana/ui_renderer/ui_renderer.hpp",
        "--require-native-ui-overlay",
        "--require-native-ui-textured-sprite-atlas",
        "plan_scene_mesh_draws",
        "camera_primary=",
        "camera_controller_ticks=",
        "scene_mesh_plan_draws=",
        "scene_mesh_plan_unique_materials=",
        "scene_mesh_plan_diagnostics=",
        "hud_boxes=",
        "ui_overlay_requested=",
        "ui_overlay_status=",
        "ui_overlay_ready=",
        "ui_overlay_sprites_submitted=",
        "ui_overlay_draws=",
        "ui_texture_overlay_requested=",
        "ui_texture_overlay_status=",
        "ui_texture_overlay_atlas_ready=",
        "ui_texture_overlay_sprites_submitted=",
        "ui_texture_overlay_texture_binds=",
        "ui_texture_overlay_draws=",
        "ui_atlas_metadata_status=",
        "ui_atlas_metadata_pages=",
        "ui_atlas_metadata_bindings=",
        "--require-renderer-quality-gates",
        "renderer_quality_status=",
        "renderer_quality_ready=",
        "renderer_quality_diagnostics=",
        "renderer_quality_expected_framegraph_passes=",
        "renderer_quality_framegraph_passes_ok=",
        "renderer_quality_framegraph_execution_budget_ok=",
        "renderer_quality_scene_gpu_ready=",
        "renderer_quality_postprocess_ready=",
        "renderer_quality_postprocess_depth_input_ready=",
        "renderer_quality_directional_shadow_ready=",
        "renderer_quality_directional_shadow_filter_ready=",
        "primary_camera_seen_",
        "hud_boxes_submitted_"
    )) {
        if (-not $sample3dMainText.Contains($needle)) {
            Write-Error "games/sample_desktop_runtime_game/main.cpp missing 3D smoke field or HUD contract: $needle"
        }
    }
    $sceneRendererHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp") -Raw
    foreach ($needle in @(
        "SceneMeshDrawPlan",
        "SceneMeshDrawPlanDiagnosticCode",
        "plan_scene_mesh_draws"
    )) {
        if (-not $sceneRendererHeaderText.Contains($needle)) {
            Write-Error "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp missing 3D scene mesh package telemetry API: $needle"
        }
    }
    $installedDesktopRuntimeValidationText = Get-Content -LiteralPath (Join-Path $root "tools/validate-installed-desktop-runtime.ps1") -Raw
    foreach ($field in @(
        "scene_mesh_plan_meshes",
        "scene_mesh_plan_draws",
        "scene_mesh_plan_unique_meshes",
        "scene_mesh_plan_unique_materials",
        "scene_mesh_plan_diagnostics",
        "--require-renderer-quality-gates",
        "renderer_quality_status",
        "renderer_quality_ready",
        "renderer_quality_diagnostics",
        "renderer_quality_expected_framegraph_passes",
        "renderer_quality_framegraph_passes_ok",
        "renderer_quality_framegraph_execution_budget_ok",
        "renderer_quality_scene_gpu_ready",
        "renderer_quality_postprocess_ready",
        "renderer_quality_postprocess_depth_input_ready",
        "renderer_quality_directional_shadow_ready",
        "renderer_quality_directional_shadow_filter_ready",
        "--require-native-ui-overlay",
        "hud_boxes",
        "ui_overlay_requested",
        "ui_overlay_status",
        "ui_overlay_ready",
        "ui_overlay_sprites_submitted",
        "ui_overlay_draws"
    )) {
        if (-not $installedDesktopRuntimeValidationText.Contains($field)) {
            Write-Error "tools/validate-installed-desktop-runtime.ps1 missing 3D scene mesh package telemetry validation field: $field"
        }
    }
}

$generated3dPackageManifestPath = "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
$generated3dPackageManifestFullPath = Join-Path $root $generated3dPackageManifestPath
if (-not (Test-Path -LiteralPath $generated3dPackageManifestFullPath)) {
    Write-Error "Generated 3D package gameplay systems smoke must have a sample game manifest: $generated3dPackageManifestPath"
} else {
    $generated3dPackageManifest = Get-Content -LiteralPath $generated3dPackageManifestFullPath -Raw | ConvertFrom-Json
    $generated3dPackageManifestText = Get-Content -LiteralPath $generated3dPackageManifestFullPath -Raw
    $generated3dPackageReadmeText = Get-Content -LiteralPath (Join-Path $root "games/sample_generated_desktop_runtime_3d_package/README.md") -Raw
    $generated3dPackageMainText = Get-Content -LiteralPath (Join-Path $root "games/sample_generated_desktop_runtime_3d_package/main.cpp") -Raw
    $gamesCMakeText = Get-Content -LiteralPath (Join-Path $root "games/CMakeLists.txt") -Raw

    if ($generated3dPackageManifest.target -ne "sample_generated_desktop_runtime_3d_package") {
        Write-Error "$generated3dPackageManifestPath target must be sample_generated_desktop_runtime_3d_package"
    }
    foreach ($recipe in @($generated3dPackageManifest.validationRecipes)) {
        $isGenerated3dPackageRecipe = [string]$recipe.name -match "installed-d3d12|vulkan"
        $isDirectionalShadowRecipe = [string]$recipe.command -match "--require-directional-shadow"
        $isShadowMorphRecipe = [string]$recipe.command -match "--require-shadow-morph-composition"
        $isSceneCollisionPackageRecipe = [string]$recipe.command -match "--require-scene-collision-package"
        if ($isGenerated3dPackageRecipe -and -not $isDirectionalShadowRecipe -and -not $isShadowMorphRecipe -and -not $isSceneCollisionPackageRecipe -and [string]$recipe.command -notmatch "--require-gameplay-systems") {
            Write-Error "$generated3dPackageManifestPath package validation recipe missing --require-gameplay-systems: $($recipe.name)"
        }
        if ($recipe.name -match "scene-collision-package" -and [string]$recipe.command -notmatch "--require-scene-collision-package") {
            Write-Error "$generated3dPackageManifestPath scene collision package recipe missing --require-scene-collision-package: $($recipe.name)"
        }
        if ($recipe.name -match "shadow-morph" -and [string]$recipe.command -notmatch "--require-shadow-morph-composition") {
            Write-Error "$generated3dPackageManifestPath shadow-morph recipe missing --require-shadow-morph-composition: $($recipe.name)"
        }
        if ($recipe.name -match "native-ui-overlay" -and [string]$recipe.command -notmatch "--require-native-ui-overlay") {
            Write-Error "$generated3dPackageManifestPath native UI overlay recipe missing --require-native-ui-overlay: $($recipe.name)"
        }
        if ($recipe.name -match "visible-production-proof" -and [string]$recipe.command -notmatch "--require-visible-3d-production-proof") {
            Write-Error "$generated3dPackageManifestPath visible production proof recipe missing --require-visible-3d-production-proof: $($recipe.name)"
        }
    }
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-shadow-morph-composition-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-native-ui-overlay-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-visible-production-proof-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-native-ui-textured-sprite-atlas-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-native-ui-text-glyph-atlas-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "runtime/assets/3d/hud.uiatlas" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "runtime/assets/3d/hud_text.uiatlas" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "shadow_receiver_shifted.ps.dxil" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "shadow_receiver_shifted.ps.spv" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "runtime_ui_overlay.hlsl" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "ui_overlay.vs.dxil" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "ui_overlay.ps.dxil" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "ui_overlay.vs.spv" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "ui_overlay.ps.spv" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-shadow-morph-composition" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-native-ui-overlay" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-visible-3d-production-proof" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-native-ui-textured-sprite-atlas" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-native-ui-text-glyph-atlas" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-gameplay-systems" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageReadmeText "--require-shadow-morph-composition" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $generated3dPackageReadmeText "--require-native-ui-overlay" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $generated3dPackageReadmeText "--require-visible-3d-production-proof" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $generated3dPackageReadmeText "--require-native-ui-textured-sprite-atlas" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $generated3dPackageReadmeText "--require-native-ui-text-glyph-atlas" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $generated3dPackageReadmeText "--require-gameplay-systems" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $gamesCMakeText "shadow_receiver_shifted.ps.dxil" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "GE_SAMPLE_SHIFTED_SCENE_SHADOW_RECEIVER_PS" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "--require-gameplay-systems" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "UI_OVERLAY_SHADER_SOURCE" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "--require-native-ui-overlay" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "--require-visible-3d-production-proof" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "--require-native-ui-textured-sprite-atlas" "games/CMakeLists.txt"
    $generated3dPackageUiAtlasText = Get-Content -LiteralPath (Join-Path $root "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud.uiatlas") -Raw
    Assert-ContainsText $generated3dPackageUiAtlasText "format=GameEngine.UiAtlas.v1" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud.uiatlas"
    Assert-ContainsText $generated3dPackageUiAtlasText "page.0.asset_uri=runtime/assets/3d/base_color.texture.geasset" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud.uiatlas"
    $generated3dPackageUiTextGlyphAtlasText = Get-Content -LiteralPath (Join-Path $root "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas") -Raw
    Assert-ContainsText $generated3dPackageUiTextGlyphAtlasText "format=GameEngine.UiAtlas.v1" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas"
    Assert-ContainsText $generated3dPackageUiTextGlyphAtlasText "glyph.0.font_family=engine-default" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas"
    Assert-ContainsText $generated3dPackageUiTextGlyphAtlasText "glyph.0.glyph=65" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas"
    Assert-ContainsText (Get-Content -LiteralPath (Join-Path $root "games/sample_generated_desktop_runtime_3d_package/runtime/sample_generated_desktop_runtime_3d_package.geindex") -Raw) "kind=ui_atlas_texture" "games/sample_generated_desktop_runtime_3d_package/runtime/sample_generated_desktop_runtime_3d_package.geindex"
    Assert-ContainsText (Get-Content -LiteralPath (Join-Path $root "tools/new-game.ps1") -Raw) "runtime/assets/3d/hud.uiatlas" "tools/new-game.ps1"
    Assert-ContainsText (Get-Content -LiteralPath (Join-Path $root "tools/new-game.ps1") -Raw) "runtime/assets/3d/hud_text.uiatlas" "tools/new-game.ps1"
    foreach ($needle in @(
        "--require-shadow-morph-composition",
        "require_shadow_morph_composition",
        "require_graphics_morph_scene",
        "load_packaged_d3d12_shifted_shadow_receiver_scene_shaders",
        "load_packaged_vulkan_shifted_shadow_receiver_scene_shaders",
        "skinned_scene_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode",
        "--require-gameplay-systems",
        "require_gameplay_systems",
        "gameplay_systems_status=",
        "gameplay_systems_ready=",
        "gameplay_systems_navigation_plan_status=",
        "gameplay_systems_blackboard_status=",
        "gameplay_systems_behavior_status=",
        "gameplay_systems_audio_status=",
        "gameplay_systems_audio_first_sample=",
        "--require-native-ui-overlay",
        "--require-visible-3d-production-proof",
        "--require-native-ui-textured-sprite-atlas",
        "--require-native-ui-text-glyph-atlas",
        "require_native_ui_overlay",
        "require_visible_3d_production_proof",
        "require_native_ui_textured_sprite_atlas",
        "require_native_ui_text_glyph_atlas",
        "build_ui_renderer_image_palette_from_runtime_ui_atlas",
        "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas",
        "hud_boxes=",
        "hud_images=",
        "hud_text_glyphs=",
        "text_glyphs_resolved=",
        "ui_atlas_metadata_status=",
        "ui_atlas_metadata_glyphs=",
        "ui_texture_overlay_status=",
        "ui_overlay_requested=",
        "ui_overlay_status=",
        "ui_overlay_ready=",
        "ui_overlay_sprites_submitted=",
        "ui_overlay_draws=",
        "visible_3d_status=",
        "visible_3d_presented_frames=",
        "visible_3d_d3d12_selected=",
        "load_packaged_d3d12_native_ui_overlay_shaders",
        "load_packaged_vulkan_native_ui_overlay_shaders",
        "native_ui_overlay_vertex_shader",
        "enable_native_ui_overlay",
        "enable_native_ui_overlay_textures"
    )) {
        Assert-ContainsText $generated3dPackageMainText $needle "games/sample_generated_desktop_runtime_3d_package/main.cpp"
    }
}

foreach ($registration in $desktopRuntimeGameRegistrations.Values) {
    $manifestRelativePath = Assert-DesktopRuntimeGameManifestPath $registration.gameManifest "desktop runtime target '$($registration.target)'"
    $manifestPath = Join-Path $root $manifestRelativePath
    if (-not (Test-Path $manifestPath)) {
        Write-Error "desktop runtime target '$($registration.target)' references missing GAME_MANIFEST: $manifestRelativePath"
    }
    $game = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ($game.target -ne $registration.target) {
        Write-Error "$manifestRelativePath target '$($game.target)' does not match desktop runtime registration '$($registration.target)'"
    }
    if (-not (@($game.packagingTargets) -contains "desktop-game-runtime")) {
        Write-Error "$manifestRelativePath is registered with MK_add_desktop_runtime_game but does not declare desktop-game-runtime"
    }
    if (-not (@($game.packagingTargets) -contains "desktop-runtime-release")) {
        Write-Error "$manifestRelativePath is registered with MK_add_desktop_runtime_game but does not declare desktop-runtime-release"
    }
    Assert-DesktopRuntimePackageFileRegistration $game $manifestRelativePath $registration
    Assert-DesktopRuntimePackageRecipe $game $manifestRelativePath
}

Write-Host "json-contract-check: ok"


