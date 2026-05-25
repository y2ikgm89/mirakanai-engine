#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("DryRun", "Apply")]
    [string]$Mode,

    [Parameter(Mandatory = $true)]
    [string]$GameName,

    [Parameter(Mandatory = $true)]
    [string]$DesignSpecPath,

    [string]$DisplayName = "",

    [string]$RepositoryRoot = ""
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "new-game-helpers.ps1")
. (Join-Path $PSScriptRoot "new-game-templates.ps1")

$scriptRepositoryRoot = Get-RepoRoot

function Assert-CreateGameRecipeProperty {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string]$Property,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not $Object.PSObject.Properties.Name.Contains($Property)) {
        Write-Error "$Label missing required property: $Property"
    }
}

function Get-CreateGameRecipeStringArray {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string]$Property,
        [Parameter(Mandatory = $true)][string]$Label
    )

    Assert-CreateGameRecipeProperty -Object $Object -Property $Property -Label $Label
    $values = @($Object.$Property)
    if ($values.Count -eq 0) {
        Write-Error "$Label.$Property must contain at least one entry."
    }
    foreach ($value in $values) {
        if ($value -isnot [string] -or [string]::IsNullOrWhiteSpace($value)) {
            Write-Error "$Label.$Property entries must be non-empty strings."
        }
    }

    return @($values)
}

function Get-CreateGameRecipeObjectArray {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string]$Property,
        [Parameter(Mandatory = $true)][string]$Label
    )

    Assert-CreateGameRecipeProperty -Object $Object -Property $Property -Label $Label
    $values = @($Object.$Property)
    if ($values.Count -eq 0) {
        Write-Error "$Label.$Property must contain at least one entry."
    }
    foreach ($value in $values) {
        if ($value -is [string] -or $null -eq $value) {
            Write-Error "$Label.$Property entries must be JSON objects."
        }
    }

    return @($values)
}

function Assert-CreateGameRecipeStringProperty {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string]$Property,
        [Parameter(Mandatory = $true)][string]$Label
    )

    Assert-CreateGameRecipeProperty -Object $Object -Property $Property -Label $Label
    $value = [string]$Object.$Property
    if ([string]::IsNullOrWhiteSpace($value)) {
        Write-Error "$Label.$Property must be a non-empty string."
    }

    return $value
}

function Get-CreateGameRecipeTemplateForDesign {
    param([Parameter(Mandatory = $true)]$DesignSpec)

    $template = Assert-CreateGameRecipeStringProperty -Object $DesignSpec -Property "template" -Label "gameDesignSpec"
    $gameplayFamily = Assert-CreateGameRecipeStringProperty -Object $DesignSpec -Property "gameplayFamily" -Label "gameDesignSpec"
    if ($template -eq "DesktopRuntime2DPackage" -and $gameplayFamily -eq "2d-desktop-runtime-package") {
        return $template
    }
    if ($template -eq "DesktopRuntime3DPackage" -and $gameplayFamily -eq "3d-playable-desktop-package") {
        return $template
    }

    Write-Error "gameDesignSpec supports only DesktopRuntime2DPackage/2d-desktop-runtime-package and DesktopRuntime3DPackage/3d-playable-desktop-package for create-game-recipe."
}

function Get-CreateGameRecipeGeneratedManifest {
    param(
        [Parameter(Mandatory = $true)][string]$Template,
        [Parameter(Mandatory = $true)][string]$GameName,
        [Parameter(Mandatory = $true)][string]$Title
    )

    if ($Template -eq "DesktopRuntime2DPackage") {
        return New-DesktopRuntime2DManifest -GameName $GameName -DisplayTitle $Title -TargetName $GameName
    }

    return New-DesktopRuntime3DManifest -GameName $GameName -DisplayTitle $Title -TargetName $GameName
}

function Get-CreateGameRecipePlannedFiles {
    param(
        [Parameter(Mandatory = $true)][string]$GameName,
        [Parameter(Mandatory = $true)][string]$Template
    )

    $prefix = "games/$GameName"
    $runtimePrefix = "$prefix/runtime"
    $planned = @(
        "$prefix/main.cpp",
        "$prefix/README.md",
        "$prefix/game.agent.json",
        "$runtimePrefix/$GameName.config"
    )

    if ($Template -eq "DesktopRuntime2DPackage") {
        $planned += @(
            "$runtimePrefix/.gitattributes",
            "$runtimePrefix/$GameName.geindex",
            "$runtimePrefix/assets/2d/player.texture.geasset",
            "$runtimePrefix/assets/2d/player.material",
            "$runtimePrefix/assets/2d/jump.audio.geasset",
            "$runtimePrefix/assets/2d/level.tilemap",
            "$runtimePrefix/assets/2d/player.sprite_animation",
            "$runtimePrefix/assets/2d/playable.scene",
            "$prefix/shaders/runtime_2d_sprite.hlsl"
        )
    } else {
        $planned += @(
            "$runtimePrefix/.gitattributes",
            "$runtimePrefix/$GameName.geindex",
            "$runtimePrefix/assets/3d/base_color.texture.geasset",
            "$runtimePrefix/assets/3d/triangle.mesh",
            "$runtimePrefix/assets/3d/skinned_triangle.skinned_mesh",
            "$runtimePrefix/assets/3d/lit.material",
            "$runtimePrefix/assets/3d/packaged_scene.scene",
            "$prefix/source/assets/package.geassets",
            "$prefix/source/scenes/packaged_scene.scene",
            "$prefix/source/prefabs/static_prop.prefab",
            "$prefix/source/textures/base_color.texture_source",
            "$prefix/source/meshes/triangle.mesh_source",
            "$prefix/source/materials/lit.material",
            "$prefix/shaders/runtime_scene.hlsl",
            "$prefix/shaders/runtime_postprocess.hlsl",
            "$prefix/shaders/runtime_shadow.hlsl",
            "$prefix/shaders/runtime_ui_overlay.hlsl"
        )
    }

    $planned += "games/CMakeLists.txt"
    return @($planned)
}

function Read-CreateGameRecipeDesignSpec {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Path
    )

    $normalizedPath = ConvertTo-DesktopRuntimeMetadataRelativePath -Path $Path -Description "DesignSpecPath"
    if ($normalizedPath -match ";") {
        Write-Error "DesignSpecPath must not contain CMake list separators: $Path"
    }

    $fullPath = Join-Path $Root $normalizedPath.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
    Assert-DesktopRuntimePathUnderRoot -Path $fullPath -Root $Root -Description "DesignSpecPath"
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        Write-Error "DesignSpecPath does not exist: $normalizedPath"
    }

    $document = Get-Content -LiteralPath $fullPath -Raw | ConvertFrom-Json
    if ($document.PSObject.Properties.Name.Contains("aiWorkflow") -and
        $null -ne $document.aiWorkflow -and
        $document.aiWorkflow.PSObject.Properties.Name.Contains("gameDesignSpec")) {
        return $document.aiWorkflow.gameDesignSpec
    }

    return $document
}

function Assert-CreateGameRecipeDesignSpec {
    param(
        [Parameter(Mandatory = $true)]$DesignSpec,
        [Parameter(Mandatory = $true)][string]$GameName,
        [Parameter(Mandatory = $true)][string]$Template,
        [Parameter(Mandatory = $true)]$GeneratedManifest
    )

    $schemaVersion = Assert-CreateGameRecipeStringProperty -Object $DesignSpec -Property "schemaVersion" -Label "gameDesignSpec"
    if ($schemaVersion -ne "1") {
        Write-Error "gameDesignSpec.schemaVersion must be 1."
    }
    $capabilityId = Assert-CreateGameRecipeStringProperty -Object $DesignSpec -Property "capabilityId" -Label "gameDesignSpec"
    if ($capabilityId -ne "ai-game-design-spec-v1") {
        Write-Error "gameDesignSpec.capabilityId must be ai-game-design-spec-v1."
    }
    $designId = Assert-CreateGameRecipeStringProperty -Object $DesignSpec -Property "designId" -Label "gameDesignSpec"
    $expectedDesignId = $GameName.Replace("_", "-")
    if ($designId -ne $expectedDesignId) {
        Write-Error "gameDesignSpec.designId must match the generated game name '$expectedDesignId'."
    }

    foreach ($property in @("camera", "coreLoop")) {
        Assert-CreateGameRecipeProperty -Object $DesignSpec -Property $property -Label "gameDesignSpec"
    }
    $inputRows = Get-CreateGameRecipeObjectArray -Object $DesignSpec -Property "inputMap" -Label "gameDesignSpec"
    foreach ($row in $inputRows) {
        foreach ($property in @("action", "defaultBinding", "purpose")) {
            $null = Assert-CreateGameRecipeStringProperty -Object $row -Property $property -Label "gameDesignSpec.inputMap entry"
        }
    }
    $sceneRows = Get-CreateGameRecipeObjectArray -Object $DesignSpec -Property "sceneList" -Label "gameDesignSpec"
    foreach ($row in $sceneRows) {
        foreach ($property in @("id", "kind", "path", "purpose")) {
            $null = Assert-CreateGameRecipeStringProperty -Object $row -Property $property -Label "gameDesignSpec.sceneList entry"
        }
    }
    $assetRows = Get-CreateGameRecipeObjectArray -Object $DesignSpec -Property "assetRequests" -Label "gameDesignSpec"
    foreach ($row in $assetRows) {
        foreach ($property in @("id", "kind", "purpose", "delivery")) {
            $null = Assert-CreateGameRecipeStringProperty -Object $row -Property $property -Label "gameDesignSpec.assetRequests entry"
        }
    }
    $null = Get-CreateGameRecipeStringArray -Object $DesignSpec -Property "systems" -Label "gameDesignSpec"

    $packageTargets = Get-CreateGameRecipeStringArray -Object $DesignSpec -Property "packageTargets" -Label "gameDesignSpec"
    foreach ($packageTarget in $packageTargets) {
        if (@($GeneratedManifest.packagingTargets) -notcontains $packageTarget) {
            Write-Error "gameDesignSpec.packageTargets contains unsupported generated target '$packageTarget' for $Template."
        }
    }

    $generatedRecipeIds = @($GeneratedManifest.validationRecipes | ForEach-Object { $_.name })
    $validationRecipeIds = Get-CreateGameRecipeStringArray -Object $DesignSpec -Property "validationRecipeIds" -Label "gameDesignSpec"
    foreach ($recipeId in $validationRecipeIds) {
        if ($generatedRecipeIds -notcontains $recipeId) {
            Write-Error "gameDesignSpec.validationRecipeIds contains unsupported generated recipe '$recipeId' for $Template."
        }
    }

    Assert-CreateGameRecipeProperty -Object $DesignSpec -Property "qualityGates" -Label "gameDesignSpec"
    foreach ($qualityGate in @($DesignSpec.qualityGates)) {
        Assert-CreateGameRecipeProperty -Object $qualityGate -Property "id" -Label "gameDesignSpec.qualityGates entry"
        Assert-CreateGameRecipeProperty -Object $qualityGate -Property "evidence" -Label "gameDesignSpec.qualityGates entry"
        $recipeIds = Get-CreateGameRecipeStringArray -Object $qualityGate -Property "recipeIds" -Label "gameDesignSpec.qualityGates entry"
        foreach ($recipeId in $recipeIds) {
            if ($generatedRecipeIds -notcontains $recipeId) {
                Write-Error "gameDesignSpec.qualityGates recipe '$recipeId' is not generated by $Template."
            }
        }
    }

    $unsupportedClaims = Get-CreateGameRecipeStringArray -Object $DesignSpec -Property "unsupportedClaims" -Label "gameDesignSpec"
    foreach ($claim in @("engine-internal-edits", "native-handles", "middleware-contracts", "unreviewed-external-assets", "arbitrary-shell")) {
        if ($unsupportedClaims -notcontains $claim) {
            Write-Error "gameDesignSpec.unsupportedClaims must explicitly reject '$claim'."
        }
    }
}

function Write-CreateGameRecipeResult {
    param([Parameter(Mandatory = $true)]$Result)

    Write-Output ($Result | ConvertTo-Json -Depth 30)
}

if ([string]::IsNullOrWhiteSpace($RepositoryRoot)) {
    $root = $scriptRepositoryRoot
} else {
    $root = (Resolve-Path -LiteralPath $RepositoryRoot).Path
}
$root = [System.IO.Path]::GetFullPath($root)

if ($GameName -notmatch "^[a-z][a-z0-9_]*$") {
    Write-Error "GameName must match ^[a-z][a-z0-9_]*$"
}

if ([string]::IsNullOrWhiteSpace($DisplayName)) {
    $DisplayName = $GameName.Replace("_", "-")
}
if (Test-ContainsControlCharacter -Text $DisplayName) {
    Write-Error "DisplayName must not contain control characters."
}

$gamesCmake = Join-Path $root "games/CMakeLists.txt"
if (-not (Test-Path -LiteralPath $gamesCmake -PathType Leaf)) {
    Write-Error "games/CMakeLists.txt does not exist under repository root: $root"
}

$designSpec = Read-CreateGameRecipeDesignSpec -Root $root -Path $DesignSpecPath
$template = Get-CreateGameRecipeTemplateForDesign -DesignSpec $designSpec
$generatedManifest = Get-CreateGameRecipeGeneratedManifest -Template $template -GameName $GameName -Title $DisplayName
Assert-CreateGameRecipeDesignSpec -DesignSpec $designSpec -GameName $GameName -Template $template -GeneratedManifest $generatedManifest

$plannedFiles = @(Get-CreateGameRecipePlannedFiles -GameName $GameName -Template $template)
$newGameTool = Join-Path $PSScriptRoot "new-game.ps1"
$newGameArguments = @{
    Name = $GameName
    DisplayName = $DisplayName
    RepositoryRoot = $root
    Template = $template
}

$null = & $newGameTool @newGameArguments -DryRun 6>&1

$validationRecipes = @($designSpec.validationRecipeIds)
$baseResult = [ordered]@{
    schemaVersion = 1
    commandId = "create-game-recipe"
    resultType = "GameEngine.AiCommand.CreateGameRecipe.Result"
    mode = $Mode
    recipeId = $designSpec.designId
    gameName = $GameName
    template = $template
    designSpecPath = (ConvertTo-DesktopRuntimeMetadataRelativePath -Path $DesignSpecPath -Description "DesignSpecPath")
    plannedFiles = @($plannedFiles)
    validationRecipes = @($validationRecipes)
    unsupportedGapIds = @()
    diagnostics = @("validated-reviewed-aiWorkflow.gameDesignSpec", "tools/new-game.ps1 dry-run preflight passed", "arbitrary-shell execution is unsupported")
}

if ($Mode -eq "DryRun") {
    $baseResult["status"] = "planned"
    $baseResult["plannedChanges"] = @($plannedFiles)
    $baseResult["changedFiles"] = @()
    $baseResult["undoToken"] = [ordered]@{
        status = "not-created"
        notes = "Dry-run mode does not write repository files."
    }
    Write-CreateGameRecipeResult -Result $baseResult
    return
}

if (-not $PSCmdlet.ShouldProcess("games/$GameName", "Create reviewed generated game recipe from aiWorkflow.gameDesignSpec")) {
    $baseResult["status"] = "blocked"
    $baseResult["blockedBy"] = @("ShouldProcess declined create-game-recipe apply.")
    $baseResult["changedFiles"] = @()
    Write-CreateGameRecipeResult -Result $baseResult
    return
}

$null = & $newGameTool @newGameArguments 6>&1

$manifestPath = Join-Path $root "games/$GameName/game.agent.json"
Assert-DesktopRuntimePathUnderRoot -Path $manifestPath -Root $root -Description "Generated game manifest"
if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    Write-Error "new-game.ps1 did not create the generated game manifest: games/$GameName/game.agent.json"
}

$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
Assert-CreateGameRecipeProperty -Object $manifest -Property "aiWorkflow" -Label "generated game manifest"
if (-not $manifest.aiWorkflow.PSObject.Properties.Name.Contains("gameDesignSpec")) {
    $manifest.aiWorkflow | Add-Member -MemberType NoteProperty -Name "gameDesignSpec" -Value $designSpec
} else {
    $manifest.aiWorkflow.gameDesignSpec = $designSpec
}
Set-Content -LiteralPath $manifestPath -Value (($manifest | ConvertTo-Json -Depth 30) + "`n") -NoNewline

$baseResult["status"] = "applied"
$baseResult["changedFiles"] = @($plannedFiles)
$baseResult["undoToken"] = [ordered]@{
    status = "manual-revert"
    notes = "Remove games/$GameName and revert games/CMakeLists.txt registration if this reviewed scaffold must be discarded."
}
Write-CreateGameRecipeResult -Result $baseResult
