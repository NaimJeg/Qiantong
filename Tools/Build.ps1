param(
    [string]$EngineRoot,
    [switch]$CoreOnly
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
$CoreBuild = Join-Path $ProjectRoot 'build/core'

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}

# Explicit generator/architecture keeps the library compatible with Win64 UE.
Invoke-Checked 'cmake' @('-S', $ProjectRoot, '-B', $CoreBuild,
    '-G', 'Visual Studio 17 2022', '-A', 'x64')
Invoke-Checked 'cmake' @('--build', $CoreBuild, '--config', 'Release')
Invoke-Checked 'ctest' @('--test-dir', $CoreBuild, '-C', 'Release', '--output-on-failure')
if ($CoreOnly) { return }
if (-not $EngineRoot) { throw 'Supply -EngineRoot pointing to the existing UE 5.8.1 installation.' }
$BuildBatch = Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat'
if (-not (Test-Path -LiteralPath $BuildBatch)) { throw "Missing UE build entry: $BuildBatch" }
$BuildArguments = @('QiantongCoreEditor', 'Win64', 'Development',
    "-Project=$(Join-Path $ProjectRoot 'QiantongCore.uproject')", '-WaitMutex', '-NoHotReloadFromIDE')
Invoke-Checked $BuildBatch $BuildArguments
