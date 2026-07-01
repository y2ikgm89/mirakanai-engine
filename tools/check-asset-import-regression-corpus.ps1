#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [string]$CorpusRoot = "",
    [switch]$RequireReady
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$pwshCommand = (Get-Command pwsh -ErrorAction Stop).Source
$repoRoot = Get-RepoRoot

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Join-Path $repoRoot ($RelativePath -replace "/", [System.IO.Path]::DirectorySeparatorChar)
}

function Write-Utf8TextFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Text
    )

    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        $null = New-Item -ItemType Directory -Path $parent -Force
    }
    Set-Content -LiteralPath $Path -Value $Text -Encoding utf8NoBOM -NoNewline
}

function New-Directory {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        $null = New-Item -ItemType Directory -Path $Path -Force
    }
}

function Assert-LinePresent {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$ExpectedLine,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Lines.Contains($ExpectedLine)) {
        Write-Error "$Context missing expected line: $ExpectedLine"
    }
}

function Invoke-AssetImportRegressionGeneratorSmoke {
    $generatorScript = Join-Path $PSScriptRoot "generate-asset-import-regression-corpus-manifest.ps1"
    if (-not (Test-Path -LiteralPath $generatorScript -PathType Leaf)) {
        Write-Error "tools/generate-asset-import-regression-corpus-manifest.ps1 must exist for approved large corpus intake."
    }

    $contractRootRelative = "out/tmp/asset-import-regression-generator-$PID"
    $contractRoot = ConvertTo-LocalPath $contractRootRelative
    if (Test-Path -LiteralPath $contractRoot) {
        Remove-Item -LiteralPath $contractRoot -Recurse -Force
    }

    try {
        $sourcesRoot = Join-Path $contractRoot "sources"
        $sourcePath = Join-Path $sourcesRoot "gltf/smoke_scene.gltf"
        $noticesPath = Join-Path $contractRoot "notices/THIRD_PARTY_ASSET_NOTICES.md"
        $outputManifest = Join-Path $contractRoot "corpus.gecorpus"
        $retainedSummary = Join-Path $contractRoot "retained/corpus-selection-summary.md"
        $retainedLedger = Join-Path $contractRoot "retained/official-source-ledger.md"

        Write-Utf8TextFile -Path $sourcePath -Text '{"asset":"smoke"}'
        foreach ($sourceDirectory in @("textures", "materials", "audio")) {
            New-Directory -Path (Join-Path $sourcesRoot $sourceDirectory)
        }
        Write-Utf8TextFile -Path $retainedSummary -Text "gltf_rows=1`ntexture_rows=0`nmaterial_rows=0`nanimation_rows=0`naudio_rows=0`n"
        Write-Utf8TextFile -Path $retainedLedger -Text "# Smoke official source ledger`n"
        Write-Utf8TextFile -Path $noticesPath -Text @'
asset.count=1
asset.0.asset_id=gltf.smoke.scene
asset.0.kind=gltf_scene
asset.0.asset_key=asset_import_regression/gltf/smoke_scene
asset.0.source_path=gltf/smoke_scene.gltf
asset.0.expected_output_kinds=GameEngine.MeshSource.v2
asset.0.required_features=gltf_scene,mesh_only
asset.0.preset_metadata=mesh.unit_scale=1,mesh.up_axis=y
asset.0.mesh.unit_scale=1
asset.0.mesh.up_axis=y
asset.0.mesh.triangulate=true
asset.0.mesh.generate_normals=true
asset.0.mesh.generate_tangents=true
asset.0.mesh.material_extraction=source_references
asset.0.license_policy=accepted_for_host_corpus_only
asset.0.allow_external_resources=false
asset.0.allow_checked_in_distribution=false
asset.0.provenance.origin=third_party
asset.0.provenance.source_url=https://example.invalid/gameengine/smoke-scene.gltf
asset.0.provenance.retrieved_date=2026-06-30
asset.0.provenance.version_or_commit=fixture-smoke
asset.0.provenance.copyright_holder=GameEngine smoke fixture
asset.0.provenance.license_id=CC0-1.0
asset.0.provenance.modification_status=unmodified
asset.0.provenance.distribution_target=host-only regression corpus
asset.0.provenance.notice_id=THIRD_PARTY_ASSET_NOTICES.md#smoke-scene
asset.0.provenance.notice_complete=true
asset.0.provenance.external_engine_material=false
'@

        $missingNoticeRejected = $false
        try {
            $null = & $generatorScript `
                -CorpusRoot $contractRoot `
                -SourcesRoot $sourcesRoot `
                -NoticesPath $noticesPath `
                -OutputManifest $outputManifest `
                -FailOnMissingNotice 2>&1
        } catch {
            $missingNoticeRejected = [string]$_.Exception.Message -like "*missing_notice_anchor*"
        }
        if (-not $missingNoticeRejected) {
            Write-Error "asset import regression corpus generator must reject rows whose notice id is absent from notices."
        }

        Add-Content -LiteralPath $noticesPath -Value "`n## Smoke scene`nTHIRD_PARTY_ASSET_NOTICES.md#smoke-scene`n" -Encoding utf8NoBOM

        $generatorLines = @(& $generatorScript `
                -CorpusRoot $contractRoot `
                -SourcesRoot $sourcesRoot `
                -NoticesPath $noticesPath `
                -OutputManifest $outputManifest `
                -FailOnMissingNotice)
        Assert-LinePresent $generatorLines "asset_import_regression_manifest_generator_asset_count=1" "asset import regression generator smoke"
        Assert-LinePresent $generatorLines "asset_import_regression_manifest_generator_ready=1" "asset import regression generator smoke"

        $validatorLines = @(& (Join-Path $PSScriptRoot "validate-asset-import-regression-corpus.ps1") -CorpusRoot $contractRoot)
        Assert-LinePresent $validatorLines "asset_import_regression_asset_count=1" "generated asset import regression corpus validation"
    } finally {
        if (Test-Path -LiteralPath $contractRoot) {
            Remove-Item -LiteralPath $contractRoot -Recurse -Force
        }
    }
}

function Invoke-AssetImportRegressionRequireReadyLayoutSmoke {
    $contractRootRelative = "out/tmp/asset-import-regression-ready-layout-$PID"
    $contractRoot = ConvertTo-LocalPath $contractRootRelative
    if (Test-Path -LiteralPath $contractRoot) {
        Remove-Item -LiteralPath $contractRoot -Recurse -Force
    }

    try {
        $sourcesRoot = Join-Path $contractRoot "sources"
        $sourcePath = Join-Path $sourcesRoot "gltf/ready_layout_scene.gltf"
        $noticesPath = Join-Path $contractRoot "notices/THIRD_PARTY_ASSET_NOTICES.md"
        $outputManifest = Join-Path $contractRoot "corpus.gecorpus"
        $generatorScript = Join-Path $PSScriptRoot "generate-asset-import-regression-corpus-manifest.ps1"

        Write-Utf8TextFile -Path $sourcePath -Text '{"asset":"ready-layout"}'
        Write-Utf8TextFile -Path $noticesPath -Text @'
asset.count=1
asset.0.asset_id=gltf.ready_layout.scene
asset.0.kind=gltf_scene
asset.0.asset_key=asset_import_regression/gltf/ready_layout_scene
asset.0.source_path=gltf/ready_layout_scene.gltf
asset.0.expected_output_kinds=GameEngine.MeshSource.v2
asset.0.required_features=gltf_scene,mesh_only
asset.0.preset_metadata=mesh.unit_scale=1,mesh.up_axis=y
asset.0.mesh.unit_scale=1
asset.0.mesh.up_axis=y
asset.0.mesh.triangulate=true
asset.0.mesh.generate_normals=true
asset.0.mesh.generate_tangents=true
asset.0.mesh.material_extraction=source_references
asset.0.license_policy=accepted_for_host_corpus_only
asset.0.allow_external_resources=false
asset.0.allow_checked_in_distribution=false
asset.0.provenance.origin=third_party
asset.0.provenance.source_url=https://example.invalid/gameengine/ready-layout-scene.gltf
asset.0.provenance.retrieved_date=2026-06-30
asset.0.provenance.version_or_commit=fixture-ready-layout
asset.0.provenance.copyright_holder=GameEngine ready-layout fixture
asset.0.provenance.license_id=CC0-1.0
asset.0.provenance.modification_status=unmodified
asset.0.provenance.distribution_target=host-only regression corpus
asset.0.provenance.notice_id=THIRD_PARTY_ASSET_NOTICES.md#ready-layout-scene
asset.0.provenance.notice_complete=true
asset.0.provenance.external_engine_material=false

THIRD_PARTY_ASSET_NOTICES.md#ready-layout-scene
'@

        $null = & $generatorScript `
            -CorpusRoot $contractRoot `
            -SourcesRoot $sourcesRoot `
            -NoticesPath $noticesPath `
            -OutputManifest $outputManifest `
            -FailOnMissingNotice
        $fallbackManifest = Join-Path $contractRoot "first_party_corpus.gecorpus"
        Move-Item -LiteralPath $outputManifest -Destination $fallbackManifest -Force
        (Get-Content -LiteralPath $fallbackManifest -Encoding utf8 -Raw).Replace(
            "asset.0.source_path=sources/gltf/ready_layout_scene.gltf",
            "asset.0.source_path=sources/ready_layout_scene.gltf"
        ) | Set-Content -LiteralPath $fallbackManifest -Encoding utf8NoBOM
        Move-Item -LiteralPath $sourcePath -Destination (Join-Path $sourcesRoot "ready_layout_scene.gltf") -Force
        Remove-Item -LiteralPath $noticesPath -Force

        $validatorLines = @(& $pwshCommand `
                -NoProfile `
                -ExecutionPolicy Bypass `
                -File (Join-Path $PSScriptRoot "validate-asset-import-regression-corpus.ps1") `
                -CorpusRoot $contractRoot `
                -RequireReady 2>&1)
        foreach ($expectedDiagnostic in @(
                "asset_import_regression_diagnostic=require_ready.corpus_manifest_missing",
                "asset_import_regression_diagnostic=require_ready.report_missing",
                "asset_import_regression_diagnostic=require_ready.notices_missing",
                "asset_import_regression_diagnostic=require_ready.sources_textures_missing",
                "asset_import_regression_diagnostic=require_ready.sources_materials_missing",
                "asset_import_regression_diagnostic=require_ready.sources_audio_missing",
                "asset_import_regression_diagnostic=require_ready.official_source_ledger_missing",
                "asset_import_regression_diagnostic=require_ready.selection_summary_missing",
                "asset_import_regression_diagnostic=require_ready.notice_rows_incomplete",
                "asset_import_regression_diagnostic=require_ready.source_paths_noncanonical",
                "asset_import_regression_diagnostic=require_ready.minimum_composition_not_met",
                "asset_import_regression_diagnostic=require_ready.required_feature_categories_missing"
            )) {
            if (-not ($validatorLines -contains $expectedDiagnostic)) {
                Write-Error "asset import regression RequireReady layout smoke missing diagnostic: $expectedDiagnostic"
            }
        }
    } finally {
        if (Test-Path -LiteralPath $contractRoot) {
            Remove-Item -LiteralPath $contractRoot -Recurse -Force
        }
    }
}

function Invoke-RepoPwshScript {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RelativeScript,

        [string[]]$ScriptArguments = @()
    )

    $scriptPath = Join-Path $PSScriptRoot $RelativeScript
    $arguments = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $scriptPath) + $ScriptArguments
    Invoke-CheckedCommand -FilePath $pwshCommand -Arguments $arguments
}

function Invoke-AssetImportRegressionRunnerCliSmoke {
    $runnerScript = Join-Path $PSScriptRoot "run-asset-import-regression-corpus.ps1"
    if (-not (Test-Path -LiteralPath $runnerScript -PathType Leaf)) {
        Write-Error "tools/run-asset-import-regression-corpus.ps1 must exist for retained corpus runner execution."
    }

    $runnerExe = Join-Path $repoRoot "out/build/dev/Debug/MK_asset_import_regression_runner.exe"
    if (-not (Test-Path -LiteralPath $runnerExe -PathType Leaf)) {
        Write-Error "MK_asset_import_regression_runner executable must be built for corpus runner smoke."
    }

    $contractRootRelative = "out/tmp/asset-import-regression-runner-$PID"
    $contractRoot = ConvertTo-LocalPath $contractRootRelative
    if (Test-Path -LiteralPath $contractRoot) {
        Remove-Item -LiteralPath $contractRoot -Recurse -Force
    }

    try {
        $sourcesRoot = Join-Path $contractRoot "sources"
        $sourcePath = Join-Path $sourcesRoot "materials/player.material"
        $noticesPath = Join-Path $contractRoot "notices/THIRD_PARTY_ASSET_NOTICES.md"
        $outputManifest = Join-Path $contractRoot "corpus.gecorpus"
        $reportPath = Join-Path $contractRoot "report.gereport"
        $outputRootRelative = "$contractRootRelative/staging"

        Write-Utf8TextFile -Path $sourcePath -Text @'
format=GameEngine.Material.v1
material.id=7
material.name=Player
material.shading=lit
material.surface=opaque
material.double_sided=false
factor.base_color=1,1,1,1
factor.emissive=0,0,0
factor.metallic=0
factor.roughness=1
texture.count=0
'@
        Write-Utf8TextFile -Path $noticesPath -Text @'
asset.count=1
asset.0.asset_id=material.player
asset.0.kind=material_document
asset.0.asset_key=materials/player
asset.0.source_path=materials/player.material
asset.0.expected_output_kinds=GameEngine.Material.v1
asset.0.required_features=material_document
asset.0.preset_metadata=mesh.unit_scale=1,mesh.up_axis=y
asset.0.mesh.unit_scale=1
asset.0.mesh.up_axis=y
asset.0.mesh.triangulate=true
asset.0.mesh.generate_normals=true
asset.0.mesh.generate_tangents=true
asset.0.mesh.material_extraction=source_references
asset.0.license_policy=accepted_for_host_corpus_only
asset.0.allow_external_resources=false
asset.0.allow_checked_in_distribution=false
asset.0.provenance.origin=third_party
asset.0.provenance.source_url=https://example.invalid/gameengine/player.material
asset.0.provenance.retrieved_date=2026-06-30
asset.0.provenance.version_or_commit=fixture-runner
asset.0.provenance.copyright_holder=GameEngine runner fixture
asset.0.provenance.license_id=CC0-1.0
asset.0.provenance.modification_status=unmodified
asset.0.provenance.distribution_target=host-only regression corpus
asset.0.provenance.notice_id=THIRD_PARTY_ASSET_NOTICES.md#runner-material
asset.0.provenance.notice_complete=true
asset.0.provenance.external_engine_material=false

THIRD_PARTY_ASSET_NOTICES.md#runner-material
'@
        $null = & (Join-Path $PSScriptRoot "generate-asset-import-regression-corpus-manifest.ps1") `
            -CorpusRoot $contractRoot `
            -SourcesRoot $sourcesRoot `
            -NoticesPath $noticesPath `
            -OutputManifest $outputManifest `
            -FailOnMissingNotice

        $runnerLines = @(& $runnerExe `
                --corpus-root $contractRootRelative `
                --output-root $outputRootRelative `
                --write-report "$contractRootRelative/report.gereport" `
                --compare-expected-hashes `
                --collect-preview-rows `
                --row-budget 8)
        Assert-LinePresent $runnerLines "asset_import_regression_asset_count=1" "asset import regression runner smoke"
        Assert-LinePresent $runnerLines "asset_import_regression_succeeded_count=1" "asset import regression runner smoke"
        Assert-LinePresent $runnerLines "asset_import_regression_failed_count=0" "asset import regression runner smoke"
        if (-not (Test-Path -LiteralPath $reportPath -PathType Leaf)) {
            Write-Error "asset import regression runner did not write report.gereport."
        }

        $wrapperLines = @(& $pwshCommand `
                -NoProfile `
                -ExecutionPolicy Bypass `
                -File $runnerScript `
                -CorpusRoot $contractRootRelative `
                -OutputRoot $outputRootRelative `
                -RowBudget 8 `
                -SkipCorpusCheck)
        Assert-LinePresent $wrapperLines "asset_import_regression_fresh_process_match=1" "asset import regression wrapper smoke"

        $badUrlLines = @(& $runnerExe `
                --corpus-root "https://example.invalid/corpus" `
                --output-root $outputRootRelative `
                --write-report "$contractRootRelative/bad.gereport" 2>&1)
        if ($LASTEXITCODE -eq 0) {
            Write-Error "asset import regression runner must reject network URL corpus roots."
        }
        if (-not (($badUrlLines -join "`n") -like "*network_url_rejected*")) {
            Write-Error "asset import regression runner URL rejection must emit network_url_rejected."
        }
    } finally {
        if (Test-Path -LiteralPath $contractRoot) {
            Remove-Item -LiteralPath $contractRoot -Recurse -Force
        }
    }
}

function Write-OperatorLoopRetainedReportFixtures {
    param([Parameter(Mandatory = $true)][string]$CorpusRoot)

    $successReportPath = Join-Path $CorpusRoot "retained/success/report.gereport"
    $failureReportPath = Join-Path $CorpusRoot "retained/failure/report.gereport"

    Write-Utf8TextFile -Path $successReportPath -Text @'
format=GameEngine.AssetImportRegressionReport.v1
corpus_id=GameEngine.AssetImportRegressionCorpus.v1
run_id=operator-loop-success
asset_count=1
succeeded_count=1
failed_count=0
legal_blocked_count=0
nondeterministic_count=0
ready=true
row.count=1
row.0.asset_id=material.ok
row.0.kind=material_document
row.0.asset=201
row.0.source_path=sources/materials/operator_ok.material
row.0.source_sha256=sha256:operator-ok
row.0.preset_sha256=sha256:preset-ok
row.0.importer_id=mirakana.importer.material_document
row.0.importer_version=asset-import-regression-v1
row.0.phase=cook
row.0.code=none
row.0.message=asset import succeeded
row.0.deterministic_output_hash=fnv64:1111111111111111
row.0.succeeded=true
row.0.ready_for_commercial_evidence=true
'@

    Write-Utf8TextFile -Path $failureReportPath -Text @'
format=GameEngine.AssetImportRegressionReport.v1
corpus_id=GameEngine.AssetImportRegressionCorpus.v1
run_id=operator-loop-failure
asset_count=4
succeeded_count=0
failed_count=4
legal_blocked_count=1
nondeterministic_count=1
ready=false
row.count=4
row.0.asset_id=mesh.legal
row.0.kind=gltf_mesh
row.0.asset=301
row.0.source_path=sources/gltf/legal.gltf
row.0.source_sha256=sha256:legal
row.0.preset_sha256=sha256:preset-legal
row.0.importer_id=mirakana.importer.gltf_mesh
row.0.importer_version=asset-import-regression-v1
row.0.phase=legal
row.0.code=rejected_license
row.0.message=license row rejected after source review
row.0.deterministic_output_hash=
row.0.succeeded=false
row.0.ready_for_commercial_evidence=false
row.1.asset_id=mesh.hash
row.1.kind=gltf_mesh
row.1.asset=302
row.1.source_path=sources/gltf/hash.gltf
row.1.source_sha256=sha256:actual
row.1.preset_sha256=sha256:preset-hash
row.1.importer_id=mirakana.importer.gltf_mesh
row.1.importer_version=asset-import-regression-v1
row.1.phase=source
row.1.code=source_hash_mismatch
row.1.message=source hash mismatch
row.1.deterministic_output_hash=
row.1.succeeded=false
row.1.ready_for_commercial_evidence=false
row.2.asset_id=mesh.axis
row.2.kind=gltf_mesh
row.2.asset=303
row.2.source_path=sources/gltf/axis.gltf
row.2.source_sha256=sha256:axis
row.2.preset_sha256=sha256:preset-axis
row.2.importer_id=mirakana.importer.gltf_mesh
row.2.importer_version=asset-import-regression-v1
row.2.phase=normalization
row.2.code=coordinate_normalization_failed
row.2.message=axis or unit preview required
row.2.deterministic_output_hash=
row.2.succeeded=false
row.2.ready_for_commercial_evidence=false
row.3.asset_id=texture.codec
row.3.kind=png_texture
row.3.asset=304
row.3.source_path=sources/textures/codec.png
row.3.source_sha256=sha256:codec
row.3.preset_sha256=sha256:preset-codec
row.3.importer_id=mirakana.importer.png_texture
row.3.importer_version=asset-import-regression-v1
row.3.phase=determinism
row.3.code=nondeterministic_output
row.3.message=fresh process hash mismatch
row.3.deterministic_output_hash=
row.3.succeeded=false
row.3.ready_for_commercial_evidence=false
'@
}

function Invoke-AssetImportRegressionOperatorLoopCorpusGateSmoke {
    $operatorLoopScript = Join-Path $PSScriptRoot "check-asset-import-regression-operator-loop.ps1"
    $contractRootRelative = "out/tmp/asset-import-regression-operator-loop-corpus-gate-$PID"
    $contractRoot = ConvertTo-LocalPath $contractRootRelative
    if (Test-Path -LiteralPath $contractRoot) {
        Remove-Item -LiteralPath $contractRoot -Recurse -Force
    }

    try {
        Write-OperatorLoopRetainedReportFixtures -CorpusRoot $contractRoot

        $operatorLines = @(& $pwshCommand `
                -NoProfile `
                -ExecutionPolicy Bypass `
                -File $operatorLoopScript `
                -CorpusRoot $contractRootRelative `
                -RequireReady `
                -OutputRoot "$contractRootRelative/operator-loop" 2>&1)
        if ($LASTEXITCODE -ne 0) {
            Write-Error "asset import regression operator-loop corpus gate smoke failed: $($operatorLines -join "`n")"
        }
        foreach ($expectedLine in @(
                "asset_import_regression_operator_loop_corpus_retained_reports=2",
                "asset_import_regression_operator_loop_corpus_success_reports=1",
                "asset_import_regression_operator_loop_corpus_failure_reports=1",
                "asset_import_regression_operator_loop_require_ready=1",
                "asset_import_regression_operator_loop_report_rows=5",
                "asset_import_regression_operator_loop_failed_rows=4",
                "asset_import_regression_operator_loop_legal_blocked_rows=1",
                "asset_import_regression_operator_loop_nondeterministic_rows=1",
                "asset_import_regression_operator_loop_reimport_candidates=1",
                "asset_import_regression_operator_loop_blocked_rows=3",
                "asset_import_regression_operator_loop_preset_diff_required=0",
                "asset_import_regression_operator_loop_axis_unit_preview_required=1",
                "asset_import_regression_operator_loop_editor_core_value_only=1",
                "asset_import_regression_operator_loop_external_engine_claim=0",
                "asset_import_regression_operator_loop_corpus_ready=1"
            )) {
            Assert-LinePresent $operatorLines $expectedLine "asset import regression operator-loop corpus gate smoke"
        }

        $missingFailureRoot = Join-Path $contractRoot "missing-failure"
        Copy-Item -LiteralPath (Join-Path $contractRoot "retained/success") -Destination (Join-Path $missingFailureRoot "retained/success") -Recurse -Force
        $missingFailureLines = @(& $pwshCommand `
                -NoProfile `
                -ExecutionPolicy Bypass `
                -File $operatorLoopScript `
                -CorpusRoot $missingFailureRoot `
                -RequireReady `
                -OutputRoot (Join-Path $missingFailureRoot "operator-loop") 2>&1)
        if ($LASTEXITCODE -eq 0) {
            Write-Error "asset import regression operator-loop corpus gate must fail when a retained failure report is missing."
        }
        if (-not (($missingFailureLines -join "`n") -like "*require_ready.retained_failure_report_missing*")) {
            Write-Error "asset import regression operator-loop corpus gate missing failure diagnostic: $($missingFailureLines -join "`n")"
        }
    } finally {
        if (Test-Path -LiteralPath $contractRoot) {
            Remove-Item -LiteralPath $contractRoot -Recurse -Force
        }
    }
}

Invoke-AssetImportRegressionGeneratorSmoke
Invoke-AssetImportRegressionRequireReadyLayoutSmoke
Invoke-RepoPwshScript -RelativeScript "check-json-contracts.ps1"

$validatorArguments = [System.Collections.Generic.List[string]]::new()
if (-not [string]::IsNullOrWhiteSpace($CorpusRoot)) {
    $validatorArguments.Add("-CorpusRoot") | Out-Null
    $validatorArguments.Add($CorpusRoot) | Out-Null
}
if ($RequireReady) {
    $validatorArguments.Add("-RequireReady") | Out-Null
}
Invoke-RepoPwshScript -RelativeScript "validate-asset-import-regression-corpus.ps1" -ScriptArguments ([string[]]$validatorArguments)

Invoke-RepoPwshScript -RelativeScript "cmake.ps1" -ScriptArguments @("--preset", "dev")
Invoke-RepoPwshScript -RelativeScript "cmake.ps1" -ScriptArguments @(
    "--build",
    "--preset",
    "dev",
    "--target",
    "MK_asset_import_regression_tests",
    "MK_asset_import_regression_runner"
)
Invoke-RepoPwshScript -RelativeScript "ctest.ps1" -ScriptArguments @(
    "--preset",
    "dev",
    "--output-on-failure",
    "-R",
    "MK_asset_import_regression_tests"
)
Invoke-AssetImportRegressionRunnerCliSmoke
Invoke-AssetImportRegressionOperatorLoopCorpusGateSmoke
Invoke-RepoPwshScript -RelativeScript "check-asset-import-regression-corpus-handoff.ps1"
Invoke-RepoPwshScript -RelativeScript "check-asset-import-regression-operator-loop.ps1" -ScriptArguments @("-SyntheticSmoke")

Write-Information "asset-import-regression-corpus-check: ok" -InformationAction Continue
