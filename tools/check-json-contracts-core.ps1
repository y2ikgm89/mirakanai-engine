#requires -Version 7.0
#requires -PSEdition Core

# Shared helpers and section dispatcher for check-json-contracts.ps1.

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

function Assert-DoesNotContainText($text, $needle, $label) {
    if ($text.Contains($needle)) {
        Write-Error "$label must not contain forbidden text: $needle"
    }
}

$script:jsonContractSurfaceTextCache = @{}
$script:jsonGameAgentManifestCache = $null

function Get-JsonContractSurfaceText([Parameter(Mandatory)][string]$relativePath) {
    $normalizedRelativePath = $relativePath -replace '\\', '/'
    if ($script:jsonContractSurfaceTextCache.ContainsKey($normalizedRelativePath)) {
        return $script:jsonContractSurfaceTextCache[$normalizedRelativePath]
    }

    $fullPath = Join-Path $root $normalizedRelativePath
    $parts = [System.Collections.Generic.List[string]]::new()
    $parts.Add((Get-Content -LiteralPath $fullPath -Raw))
    if ($normalizedRelativePath -eq "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md") {
        $splitRoot = Join-Path $root "docs/superpowers/master-plans/production-completion-v1"
        if (Test-Path -LiteralPath $splitRoot) {
            Get-ChildItem -LiteralPath $splitRoot -Filter "*.md" -File |
                Sort-Object Name |
                ForEach-Object { $parts.Add((Get-Content -LiteralPath $_.FullName -Raw)) }
        }
    }
    $surfaceText = $parts -join "`n"
    $script:jsonContractSurfaceTextCache[$normalizedRelativePath] = $surfaceText
    return $surfaceText
}

function Get-GameAgentManifests {
    if ($null -ne $script:jsonGameAgentManifestCache) {
        return @($script:jsonGameAgentManifestCache)
    }

    $gamesRoot = Join-Path $root "games"
    $manifests = @()
    if (Test-Path -LiteralPath $gamesRoot -PathType Container) {
        $manifests = @(Get-ChildItem -LiteralPath $gamesRoot -Recurse -Filter "game.agent.json" -File |
            Sort-Object FullName |
            ForEach-Object {
                $manifestText = Get-Content -LiteralPath $_.FullName -Raw
                [pscustomobject]@{
                    RelativePath = Get-RelativeRepoPath $_.FullName
                    FullPath = $_.FullName
                    Text = $manifestText
                    Game = $manifestText | ConvertFrom-Json
                }
            })
    }

    $script:jsonGameAgentManifestCache = $manifests
    return @($script:jsonGameAgentManifestCache)
}

function Get-GameAgentManifest([Parameter(Mandatory)][string]$RelativePath) {
    $normalizedRelativePath = $RelativePath -replace '\\', '/'
    foreach ($manifest in Get-GameAgentManifests) {
        if ($manifest.RelativePath -eq $normalizedRelativePath) {
            return $manifest
        }
    }
    return $null
}

function Get-ActiveChildProductionPlans {
    $masterPlanPath = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
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
    $masterPlanPath = "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
    $planRegistryPath = "docs/superpowers/plans/README.md"
    $planRegistryText = Get-Content -LiteralPath (Join-Path $root $planRegistryPath) -Raw
    $activeSliceRow = [regex]::Match($planRegistryText, '(?m)^\| Active (slice|milestone) \(`currentActivePlan`\) \|.*$')
    if (-not $activeSliceRow.Success) {
        Write-Error "$planRegistryPath must contain an Active slice or milestone currentActivePlan row"
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
        if (-not [string]::IsNullOrWhiteSpace($activePlan.planId) -and
            $productionLoop.recommendedNextPlan.id -ne $activePlan.planId) {
            Write-Error "engine manifest aiOperableProductionLoop.recommendedNextPlan.id must match active child plan id: $($activePlan.planId)"
        }
        if ($productionLoop.recommendedNextPlan.PSObject.Properties.Name.Contains("path") -and
            $productionLoop.recommendedNextPlan.path -ne $activePlan.path) {
            Write-Error "engine manifest aiOperableProductionLoop.recommendedNextPlan.path must match active child plan path: $($activePlan.path)"
        }
        if (-not $activeSliceRow.Value.Contains($activePlan.fileName) -and
            ([string]::IsNullOrWhiteSpace($activePlan.planId) -or -not $activeSliceRow.Value.Contains($activePlan.planId))) {
            Write-Error "$planRegistryPath Active slice or milestone row must mention active child plan id or path: $($activePlan.path)"
        }
        if ($activePlan.planId -eq "physics-1-0-collision-system-closeout-v1") {
            $physicsJointsPlanPath = Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            $physicsBenchmarkPlanPath = Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            $physicsJoltPlanPath = Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            $physicsCloseoutPlanPath = Join-Path $root "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            $physicsJointsPlanText = Get-Content -LiteralPath $physicsJointsPlanPath -Raw
            $physicsBenchmarkPlanText = Get-Content -LiteralPath $physicsBenchmarkPlanPath -Raw
            $physicsJoltPlanText = Get-Content -LiteralPath $physicsJoltPlanPath -Raw
            $physicsCloseoutPlanText = Get-Content -LiteralPath $physicsCloseoutPlanPath -Raw
            Assert-ContainsText $physicsJointsPlanText "**Status:** Completed." "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            Assert-ContainsText $physicsBenchmarkPlanText 'Plan ID:** `physics-benchmark-determinism-gates-v1`' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            Assert-ContainsText $physicsBenchmarkPlanText "**Status:** Completed." "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            Assert-ContainsText $physicsBenchmarkPlanText 'Gap:** `physics-1-0-collision-system` Phase P2' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            Assert-ContainsText $physicsBenchmarkPlanText "count-based" "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            Assert-ContainsText $physicsBenchmarkPlanText "budget gates" "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            Assert-ContainsText $physicsBenchmarkPlanText "PhysicsReplaySignature3D" "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            Assert-ContainsText $physicsBenchmarkPlanText "evaluate_physics_determinism_gate_3d" "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            Assert-ContainsText $physicsJoltPlanText 'Plan ID:** `physics-jolt-adapter-gate-v1`' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            Assert-ContainsText $physicsJoltPlanText "**Status:** Completed." "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            Assert-ContainsText $physicsJoltPlanText 'Gap:** `physics-1-0-collision-system` Phase P3' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            Assert-ContainsText $physicsJoltPlanText "Jolt" "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            Assert-ContainsText $physicsJoltPlanText "explicit 1.0 exclusion" "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            Assert-ContainsText $physicsCloseoutPlanText 'Plan ID:** `physics-1-0-collision-system-closeout-v1`' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            Assert-ContainsText $physicsCloseoutPlanText "**Status:** Completed." "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
            Assert-ContainsText $physicsCloseoutPlanText 'Gap:** `physics-1-0-collision-system` Phase P4' "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
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
    $packageFiles = @(Get-NormalizedRuntimePackageFiles $game $relativePath | ForEach-Object { $_.Substring($gameDirectory.Length + 1) })
    $packageFileSet = @{}
    foreach ($packageFile in $packageFiles) { $packageFileSet[$packageFile] = $true }

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

        $hasGraphAuthoring = $target.PSObject.Properties.Name.Contains("sourceMaterialGraphPath") -or
            $target.PSObject.Properties.Name.Contains("shaderExportPath") -or
            $target.PSObject.Properties.Name.Contains("reviewedHlslSourcePath") -or
            $target.PSObject.Properties.Name.Contains("compileRequestTargets") -or
            $target.PSObject.Properties.Name.Contains("unsupportedBoundaries")
        if ($hasGraphAuthoring) {
            Assert-Properties $target @("sourceMaterialGraphPath", "shaderExportPath", "reviewedHlslSourcePath", "compileRequestTargets", "unsupportedBoundaries") "$relativePath materialShaderAuthoringTargets graph authoring target"
            foreach ($pathRow in @(
                    @{ Field = "sourceMaterialGraphPath"; Path = ConvertTo-RepoPath ([string]$target.sourceMaterialGraphPath); Extension = ".materialgraph" },
                    @{ Field = "shaderExportPath"; Path = ConvertTo-RepoPath ([string]$target.shaderExportPath); Extension = ".shader_export" },
                    @{ Field = "reviewedHlslSourcePath"; Path = ConvertTo-RepoPath ([string]$target.reviewedHlslSourcePath); Extension = ".hlsl" }
                )) {
                if (-not (Test-SafeGameRelativePath $pathRow.Path) -or -not $pathRow.Path.EndsWith($pathRow.Extension)) {
                    Write-Error "$relativePath materialShaderAuthoringTargets $($pathRow.Field) must be a safe game-relative $($pathRow.Extension) path: $($pathRow.Path)"
                }
                if ($packageFileSet.ContainsKey($pathRow.Path)) {
                    Write-Error "$relativePath materialShaderAuthoringTargets $($pathRow.Field) must not be declared in runtimePackageFiles: $($pathRow.Path)"
                }
                if (-not (Test-Path -LiteralPath (Join-Path $root "$gameDirectory/$($pathRow.Path)"))) {
                    Write-Error "$relativePath materialShaderAuthoringTargets $($pathRow.Field) does not exist: $($pathRow.Path)"
                }
            }
            if (@($target.shaderSourcePaths) -notcontains $target.reviewedHlslSourcePath) {
                Write-Error "$relativePath materialShaderAuthoringTargets reviewedHlslSourcePath must also appear in shaderSourcePaths: $($target.reviewedHlslSourcePath)"
            }
            foreach ($compileTarget in @("d3d12-dxil", "vulkan-spirv")) {
                if (@($target.compileRequestTargets) -notcontains $compileTarget) {
                    Write-Error "$relativePath materialShaderAuthoringTargets compileRequestTargets missing $compileTarget"
                }
            }
            foreach ($unsupportedBoundary in @("shader-graph-execution", "live-shader-generation", "runtime-compiler-execution", "native-cache-handle", "renderer-rhi-residency", "metal-library-generation", "package-streaming")) {
                if (@($target.unsupportedBoundaries) -notcontains $unsupportedBoundary) {
                    Write-Error "$relativePath materialShaderAuthoringTargets unsupportedBoundaries missing $unsupportedBoundary"
                }
            }
            foreach ($artifactPath in @(
                    "shaders/material_shader_package_scene.vs.dxil",
                    "shaders/material_shader_package_scene.ps.dxil",
                    "shaders/material_shader_package_postprocess.vs.dxil",
                    "shaders/material_shader_package_postprocess.ps.dxil",
                    "shaders/material_shader_package_scene.vs.spv",
                    "shaders/material_shader_package_scene.ps.spv",
                    "shaders/material_shader_package_postprocess.vs.spv",
                    "shaders/material_shader_package_postprocess.ps.spv"
                )) {
                if (@($target.d3d12ShaderArtifactPaths + $target.vulkanShaderArtifactPaths) -notcontains $artifactPath) {
                    Write-Error "$relativePath materialShaderAuthoringTargets graph authoring artifacts missing short path: $artifactPath"
                }
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

function Assert-SpriteAtlasSourceAuthoringTargets($game, [string]$relativePath, [bool]$required) {
    $gameDirectory = Get-GameDirectoryFromManifestPath $relativePath
    $packageFiles = @(Get-NormalizedRuntimePackageFiles $game $relativePath | ForEach-Object {
            $_.Substring($gameDirectory.Length + 1)
        })
    $packageFileSet = @{}
    foreach ($packageFile in $packageFiles) {
        $packageFileSet[$packageFile] = $true
    }

    if (-not $game.PSObject.Properties.Name.Contains("spriteAtlasSourceAuthoringTargets")) {
        if ($required) {
            Write-Error "$relativePath 2D package manifests must declare spriteAtlasSourceAuthoringTargets"
        }
        return
    }
    if ($null -eq $game.spriteAtlasSourceAuthoringTargets -or $game.spriteAtlasSourceAuthoringTargets -isnot [System.Array]) {
        Write-Error "$relativePath spriteAtlasSourceAuthoringTargets must be an array"
    }
    if ($required -and @($game.spriteAtlasSourceAuthoringTargets).Count -lt 1) {
        Write-Error "$relativePath spriteAtlasSourceAuthoringTargets must contain at least one target"
    }

    $sourceFormats = @($game.importerRequirements.sourceFormats)
    foreach ($sourceFormat in @("GameEngine.TextureSource.v1", "GameEngine.SourceAssetRegistry.v1")) {
        if ($sourceFormats -notcontains $sourceFormat) {
            Write-Error "$relativePath spriteAtlasSourceAuthoringTargets requires importerRequirements.sourceFormats entry: $sourceFormat"
        }
    }

    $validationRecipeIds = @{}
    foreach ($recipe in @($game.validationRecipes)) {
        $validationRecipeIds[[string]$recipe.name] = $true
    }

    $seenIds = @{}
    foreach ($target in @($game.spriteAtlasSourceAuthoringTargets)) {
        Assert-Properties $target @("id", "mode", "planner", "sourceRegistryPath", "atlasSourcePath", "atlasImportedPath", "atlasAssetKey", "packageIndexPath", "maxSide", "pagePolicy", "sourceDecoding", "atlasPacking", "runtimeSourceImageDecoding", "rendererRhiResidency", "packageStreaming", "animationSemantics", "editorProductization", "freeFormEdit", "frameRows", "preflightRecipeIds") "$relativePath spriteAtlasSourceAuthoringTargets"

        $id = [string]$target.id
        $sourceRegistryPath = ConvertTo-RepoPath ([string]$target.sourceRegistryPath)
        $atlasSourcePath = ConvertTo-RepoPath ([string]$target.atlasSourcePath)
        $atlasImportedPath = ConvertTo-RepoPath ([string]$target.atlasImportedPath)
        $packageIndexPath = ConvertTo-RepoPath ([string]$target.packageIndexPath)
        $atlasAssetKey = [string]$target.atlasAssetKey

        if ($id -notmatch "^[a-z][a-z0-9-]*$") {
            Write-Error "$relativePath spriteAtlasSourceAuthoringTargets id must be kebab-case: $id"
        }
        if ($seenIds.ContainsKey($id)) {
            Write-Error "$relativePath spriteAtlasSourceAuthoringTargets id is duplicated: $id"
        }
        $seenIds[$id] = $true

        foreach ($pathRow in @(
                @{ Field = "sourceRegistryPath"; Path = $sourceRegistryPath; Extension = ".geassets"; RuntimeFile = $false },
                @{ Field = "atlasSourcePath"; Path = $atlasSourcePath; Extension = ".texture_source"; RuntimeFile = $false },
                @{ Field = "atlasImportedPath"; Path = $atlasImportedPath; Extension = ".geasset"; RuntimeFile = $true },
                @{ Field = "packageIndexPath"; Path = $packageIndexPath; Extension = ".geindex"; RuntimeFile = $true }
            )) {
            if (-not (Test-SafeGameRelativePath $pathRow.Path) -or -not $pathRow.Path.EndsWith($pathRow.Extension)) {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets $($pathRow.Field) must be a safe game-relative $($pathRow.Extension) path: $($pathRow.Path)"
            }
            if ($pathRow.RuntimeFile -and -not $packageFileSet.ContainsKey($pathRow.Path)) {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets $($pathRow.Field) must also be declared in runtimePackageFiles: $($pathRow.Path)"
            }
            if (-not $pathRow.RuntimeFile -and $packageFileSet.ContainsKey($pathRow.Path)) {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets $($pathRow.Field) must not be declared in runtimePackageFiles: $($pathRow.Path)"
            }
            if (-not (Test-Path -LiteralPath (Join-Path $root "$gameDirectory/$($pathRow.Path)"))) {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets $($pathRow.Field) does not exist: $($pathRow.Path)"
            }
        }

        if (-not (Test-SafeAssetKey $atlasAssetKey)) {
            Write-Error "$relativePath spriteAtlasSourceAuthoringTargets atlasAssetKey must be a safe AssetKeyV2 value: $atlasAssetKey"
        }
        if ([string]$target.mode -ne "reviewed-rgba8-source-frames") {
            Write-Error "$relativePath spriteAtlasSourceAuthoringTargets mode must be reviewed-rgba8-source-frames"
        }
        if ([string]$target.planner -ne "plan_sprite_atlas_source_authoring") {
            Write-Error "$relativePath spriteAtlasSourceAuthoringTargets planner must be plan_sprite_atlas_source_authoring"
        }
        if ([string]$target.sourceDecoding -ne "provided-rgba8-texture-source") {
            Write-Error "$relativePath spriteAtlasSourceAuthoringTargets sourceDecoding must be provided-rgba8-texture-source"
        }
        if ([string]$target.atlasPacking -ne "deterministic-sprite-atlas-rgba8-max-side") {
            Write-Error "$relativePath spriteAtlasSourceAuthoringTargets atlasPacking must be deterministic-sprite-atlas-rgba8-max-side"
        }
        if (-not ($target.maxSide -is [int] -or $target.maxSide -is [long]) -or $target.maxSide -lt 1) {
            Write-Error "$relativePath spriteAtlasSourceAuthoringTargets maxSide must be a positive integer"
        }
        Assert-Properties $target.pagePolicy @("mode", "pageId", "pageCount", "paddingPixels") "$relativePath spriteAtlasSourceAuthoringTargets pagePolicy"
        if ([string]$target.pagePolicy.mode -ne "single-page-tight-rgba8-texture-source") {
            Write-Error "$relativePath spriteAtlasSourceAuthoringTargets pagePolicy mode must be single-page-tight-rgba8-texture-source"
        }
        if ([string]$target.pagePolicy.pageId -notmatch "^[a-z][a-z0-9_/-]*$") {
            Write-Error "$relativePath spriteAtlasSourceAuthoringTargets pagePolicy pageId must be a safe page id: $($target.pagePolicy.pageId)"
        }
        if ($target.pagePolicy.pageCount -ne 1 -or $target.pagePolicy.paddingPixels -ne 0) {
            Write-Error "$relativePath spriteAtlasSourceAuthoringTargets pagePolicy must declare one tight page with zero padding"
        }
        foreach ($field in @("runtimeSourceImageDecoding", "rendererRhiResidency", "packageStreaming", "animationSemantics", "editorProductization", "freeFormEdit")) {
            if ([string]$target.$field -ne "unsupported") {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets $field must remain unsupported"
            }
        }

        $sourceRegistryText = Get-Content -LiteralPath (Join-Path $root "$gameDirectory/$sourceRegistryPath") -Raw
        if (-not (Test-TextStartsWithLine $sourceRegistryText "format=GameEngine.SourceAssetRegistry.v1")) {
            Write-Error "$relativePath spriteAtlasSourceAuthoringTargets sourceRegistryPath must contain GameEngine.SourceAssetRegistry.v1 text: $sourceRegistryPath"
        }
        foreach ($needle in @(
                "asset.0.key=$atlasAssetKey",
                "asset.0.kind=texture",
                "asset.0.source=$atlasSourcePath",
                "asset.0.source_format=GameEngine.TextureSource.v1",
                "asset.0.imported=$atlasImportedPath"
            )) {
            if (-not $sourceRegistryText.Contains($needle)) {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets source registry missing: $needle"
            }
        }

        $atlasSourceText = Get-Content -LiteralPath (Join-Path $root "$gameDirectory/$atlasSourcePath") -Raw
        if (-not (Test-TextStartsWithLine $atlasSourceText "format=GameEngine.TextureSource.v1")) {
            Write-Error "$relativePath spriteAtlasSourceAuthoringTargets atlasSourcePath must contain GameEngine.TextureSource.v1 text: $atlasSourcePath"
        }
        foreach ($needle in @("texture.pixel_format=rgba8_unorm", "texture.data_hex=")) {
            if (-not $atlasSourceText.Contains($needle)) {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets atlasSourcePath missing: $needle"
            }
        }

        if ($target.frameRows -isnot [System.Array] -or @($target.frameRows).Count -lt 1) {
            Write-Error "$relativePath spriteAtlasSourceAuthoringTargets frameRows must be a non-empty array"
        }
        $seenFrameIds = @{}
        foreach ($frameRow in @($target.frameRows)) {
            Assert-Properties $frameRow @("id", "sourcePath", "pageId", "width", "height", "pixelFormat", "pivot", "sliceBorder") "$relativePath spriteAtlasSourceAuthoringTargets frameRows"
            $frameId = [string]$frameRow.id
            if ($frameId -notmatch "^[a-z][a-z0-9_/-]*$") {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets frameRows id must be safe slash-separated text: $frameId"
            }
            if ($seenFrameIds.ContainsKey($frameId)) {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets frameRows id is duplicated: $frameId"
            }
            $seenFrameIds[$frameId] = $true
            $frameSourcePath = ConvertTo-RepoPath ([string]$frameRow.sourcePath)
            if (-not (Test-SafeGameRelativePath $frameSourcePath)) {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets frameRows sourcePath must be safe and game-relative: $frameSourcePath"
            }
            if ([string]$frameRow.pageId -ne [string]$target.pagePolicy.pageId) {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets frameRows pageId must match pagePolicy pageId"
            }
            if ([string]$frameRow.pixelFormat -ne "rgba8_unorm") {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets frameRows pixelFormat must be rgba8_unorm"
            }
            if (-not ($frameRow.width -is [int] -or $frameRow.width -is [long]) -or
                -not ($frameRow.height -is [int] -or $frameRow.height -is [long]) -or
                $frameRow.width -lt 1 -or $frameRow.height -lt 1 -or
                $frameRow.width -gt $target.maxSide -or $frameRow.height -gt $target.maxSide) {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets frameRows dimensions must be positive and within maxSide"
            }
            Assert-Properties $frameRow.pivot @("x", "y") "$relativePath spriteAtlasSourceAuthoringTargets frameRows pivot"
            foreach ($axis in @("x", "y")) {
                $value = [double]$frameRow.pivot.$axis
                if ($value -lt 0.0 -or $value -gt 1.0) {
                    Write-Error "$relativePath spriteAtlasSourceAuthoringTargets frameRows pivot $axis must be normalized"
                }
            }
            Assert-Properties $frameRow.sliceBorder @("left", "bottom", "right", "top") "$relativePath spriteAtlasSourceAuthoringTargets frameRows sliceBorder"
            $left = [double]$frameRow.sliceBorder.left
            $bottom = [double]$frameRow.sliceBorder.bottom
            $right = [double]$frameRow.sliceBorder.right
            $top = [double]$frameRow.sliceBorder.top
            foreach ($borderValue in @($left, $bottom, $right, $top)) {
                if ($borderValue -lt 0.0 -or $borderValue -gt 1.0) {
                    Write-Error "$relativePath spriteAtlasSourceAuthoringTargets frameRows sliceBorder values must be normalized"
                }
            }
            if ($left + $right -ge 0.98 -or $bottom + $top -ge 0.98) {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets frameRows sliceBorder must leave a center region"
            }
        }

        foreach ($recipeId in @($target.preflightRecipeIds)) {
            if (-not $validationRecipeIds.ContainsKey([string]$recipeId)) {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets preflightRecipeIds must reference validationRecipes names: $recipeId"
            }
        }
        foreach ($forbiddenField in @("command", "shell", "argv", "pngDecoder", "sourceImagePath", "rendererBackend", "nativeHandle", "rhiHandle", "metalDevice", "runtimeImageDecodingReady", "productionAtlasPackingReady", "animationSemanticReady", "editorProductReady", "packageStreamingReady")) {
            if ($target.PSObject.Properties.Name.Contains($forbiddenField)) {
                Write-Error "$relativePath spriteAtlasSourceAuthoringTargets must not expose unsupported production, renderer, or native-handle field: $forbiddenField"
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

function Assert-PerformanceBudgets($game, [string]$relativePath, [bool]$required) {
    if (-not $game.PSObject.Properties.Name.Contains("performanceBudgets")) {
        if ($required) { Write-Error "$relativePath selected package manifests must declare performanceBudgets" }
        return
    }
    $budget = $game.performanceBudgets
    Assert-Properties $budget @("schemaVersion", "capabilityId", "budgetSetId", "selectedRecipeId", "targetBackend", "hostGateId", "validationRecipeIds", "budgetRows", "evidenceRows", "unsupportedClaims") "$relativePath performanceBudgets"
    if ($budget.schemaVersion -ne 1) { Write-Error "$relativePath performanceBudgets schemaVersion must be 1" }
    if ([string]$budget.capabilityId -ne "ai-operable-performance-budget-and-evidence-v1") { Write-Error "$relativePath performanceBudgets capabilityId must be ai-operable-performance-budget-and-evidence-v1" }
    foreach ($kebabValue in @("budgetSetId", "hostGateId")) { if ([string]$budget.$kebabValue -notmatch "^[a-z][a-z0-9-]*$") { Write-Error "$relativePath performanceBudgets $kebabValue must be kebab-case: $($budget.$kebabValue)" } }
    if (@("none", "null", "d3d12", "vulkan", "metal", "multi-backend", "host-gated") -notcontains [string]$budget.targetBackend) { Write-Error "$relativePath performanceBudgets targetBackend has unsupported value: $($budget.targetBackend)" }
    $validationRecipeIds = @{}
    foreach ($recipe in @($game.validationRecipes)) { $validationRecipeIds[[string]$recipe.name] = $true }
    foreach ($recipeId in @($budget.validationRecipeIds) + @($budget.selectedRecipeId)) { if (-not $validationRecipeIds.ContainsKey([string]$recipeId)) { Write-Error "$relativePath performanceBudgets validation recipe reference is missing from validationRecipes: $recipeId" } }
    if ($budget.budgetRows -isnot [System.Array] -or @($budget.budgetRows).Count -lt 1) { Write-Error "$relativePath performanceBudgets budgetRows must be a non-empty array" }
    $budgetCategories = @("frame", "cpu", "gpu", "memory", "draw", "dispatch", "upload", "package", "streaming", "shader-pipeline", "trace")
    $budgetUnits = @("ms", "us", "bytes", "count", "percent", "bool")
    $budgetRowIds = @{}
    foreach ($row in @($budget.budgetRows)) {
        Assert-Properties $row @("id", "category", "metric", "limit", "unit", "evidenceRequired", "notes") "$relativePath performanceBudgets budgetRows"
        $rowId = [string]$row.id
        if ($rowId -notmatch "^[a-z][a-z0-9-]*$") { Write-Error "$relativePath performanceBudgets budgetRows id must be kebab-case: $rowId" }
        if ($budgetRowIds.ContainsKey($rowId)) { Write-Error "$relativePath performanceBudgets budgetRows id is duplicated: $rowId" }
        $budgetRowIds[$rowId] = $true
        if ($budgetCategories -notcontains [string]$row.category) { Write-Error "$relativePath performanceBudgets budgetRows '$rowId' has unsupported category: $($row.category)" }
        if ([string]::IsNullOrWhiteSpace([string]$row.metric)) { Write-Error "$relativePath performanceBudgets budgetRows '$rowId' metric must be non-empty" }
        if (-not ($row.limit -is [int] -or $row.limit -is [long] -or $row.limit -is [double] -or $row.limit -is [decimal])) { Write-Error "$relativePath performanceBudgets budgetRows '$rowId' limit must be numeric" }
        if ($budgetUnits -notcontains [string]$row.unit) { Write-Error "$relativePath performanceBudgets budgetRows '$rowId' has unsupported unit: $($row.unit)" }
        if ($row.evidenceRequired -isnot [bool]) { Write-Error "$relativePath performanceBudgets budgetRows '$rowId' evidenceRequired must be boolean" }
    }
    if ($budget.evidenceRows -isnot [System.Array] -or @($budget.evidenceRows).Count -lt 1) { Write-Error "$relativePath performanceBudgets evidenceRows must be a non-empty array" }
    $evidenceKinds = @("package-smoke-counter", "trace-artifact", "profiler-artifact", "manifest-budget", "static-contract", "manual-host-evidence")
    $evidenceRowIds = @{}
    foreach ($row in @($budget.evidenceRows)) {
        Assert-Properties $row @("id", "budgetRowIds", "evidenceKind", "source", "validationRecipeId", "status") "$relativePath performanceBudgets evidenceRows"
        $rowId = [string]$row.id
        if ($rowId -notmatch "^[a-z][a-z0-9-]*$") { Write-Error "$relativePath performanceBudgets evidenceRows id must be kebab-case: $rowId" }
        if ($evidenceRowIds.ContainsKey($rowId)) { Write-Error "$relativePath performanceBudgets evidenceRows id is duplicated: $rowId" }
        $evidenceRowIds[$rowId] = $true
        if ($row.budgetRowIds -isnot [System.Array] -or @($row.budgetRowIds).Count -lt 1) { Write-Error "$relativePath performanceBudgets evidenceRows '$rowId' budgetRowIds must be a non-empty array" }
        foreach ($budgetRowId in @($row.budgetRowIds)) { if (-not $budgetRowIds.ContainsKey([string]$budgetRowId)) { Write-Error "$relativePath performanceBudgets evidenceRows '$rowId' references unknown budgetRows id: $budgetRowId" } }
        if ($evidenceKinds -notcontains [string]$row.evidenceKind) { Write-Error "$relativePath performanceBudgets evidenceRows '$rowId' has unsupported evidenceKind: $($row.evidenceKind)" }
        if (-not $validationRecipeIds.ContainsKey([string]$row.validationRecipeId)) { Write-Error "$relativePath performanceBudgets evidenceRows '$rowId' validationRecipeId must reference validationRecipes: $($row.validationRecipeId)" }
        if (@("ready", "host-gated", "planned", "unsupported") -notcontains [string]$row.status) { Write-Error "$relativePath performanceBudgets evidenceRows '$rowId' has unsupported status: $($row.status)" }
        if ($row.PSObject.Properties.Name.Contains("artifactPath") -and -not (Test-SafeGameRelativePath (ConvertTo-RepoPath ([string]$row.artifactPath)))) { Write-Error "$relativePath performanceBudgets evidenceRows '$rowId' artifactPath must be safe and game-relative: $($row.artifactPath)" }
    }
    foreach ($claim in @("broad-optimized-game", "cross-vendor-performance-parity", "cross-backend-performance-parity", "native-handles")) { if (@($budget.unsupportedClaims) -notcontains $claim) { Write-Error "$relativePath performanceBudgets unsupportedClaims missing required claim: $claim" } }
    $allowedClaims = @("broad-optimized-game", "cross-vendor-performance-parity", "cross-backend-performance-parity", "unbounded-frame-time", "runtime-source-parsing", "renderer-rhi-residency", "native-handles", "allocator-gpu-budget-enforcement", "metal-readiness", "cuda-hip-runtime-path", "gpu-driven-rendering-ready")
    foreach ($claim in @($budget.unsupportedClaims)) { if ($allowedClaims -notcontains [string]$claim) { Write-Error "$relativePath performanceBudgets unsupportedClaims has unsupported claim: $claim" } }
    foreach ($forbiddenField in @("command", "shell", "argv", "nativeHandle", "rhiHandle", "cudaContext", "hipContext", "metalDevice", "allocatorHandle", "gpuBudgetEnforcement", "threadScheduler", "workStealingReady", "optimizedGameReady")) { if ($budget.PSObject.Properties.Name.Contains($forbiddenField)) { Write-Error "$relativePath performanceBudgets must not expose execution, native-handle, scheduler, or broad readiness field: $forbiddenField" } }
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
        if (-not (Test-TextStartsWithLine $sourceSceneText "format=GameEngine.Scene.v2")) {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets sceneAuthoringPath must contain GameEngine.Scene.v2 text: $sceneAuthoringPath"
        }
        if (-not (Test-TextStartsWithLine $sourcePrefabText "format=GameEngine.Prefab.v2")) {
            Write-Error "$relativePath prefabScenePackageAuthoringTargets prefabAuthoringPath must contain GameEngine.Prefab.v2 text: $prefabAuthoringPath"
        }

        $sourceRegistryFullPath = Join-Path $root "$gameDirectory/$sourceRegistryPath"
        $sourceRegistryText = Get-Content -LiteralPath $sourceRegistryFullPath -Raw
        $normalizedSourceRegistryText = ConvertTo-LfText $sourceRegistryText
        if (-not (Test-TextStartsWithLine $sourceRegistryText "format=GameEngine.SourceAssetRegistry.v1")) {
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
            if (-not [System.Text.RegularExpressions.Regex]::IsMatch($normalizedSourceRegistryText, "(?m)^asset\.\d+\.key=$([System.Text.RegularExpressions.Regex]::Escape($key))$")) {
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
        $normalizedSourceRegistryText = ConvertTo-LfText $sourceRegistryText
        if (-not (Test-TextStartsWithLine $sourceRegistryText "format=GameEngine.SourceAssetRegistry.v1")) {
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
            if (-not [System.Text.RegularExpressions.Regex]::IsMatch($normalizedSourceRegistryText, "(?m)^asset\.\d+\.key=$([System.Text.RegularExpressions.Regex]::Escape($key))$")) {
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
function Get-JsonContractSectionFiles {
    $ledger = Get-StaticContractLedgerById -Id "check-json-contracts"
    return @($ledger.SectionFiles)
}

function Invoke-JsonContractSections {
    foreach ($sectionFile in Get-JsonContractSectionFiles) {
        . (Join-Path $PSScriptRoot $sectionFile)
    }
}
