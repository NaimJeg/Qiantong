param([Parameter(Mandatory=$true)][string]$EngineRoot)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
& (Join-Path $PSScriptRoot 'Build.ps1') -CoreOnly
if ($LASTEXITCODE -ne 0) { throw 'Core verification failed' }
$Automation = Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat'
& $Automation BuildCookRun "-project=$ProjectRoot/QiantongCore.uproject" -noP4 -platform=Win64 -clientconfig=Development -build -cook -map=/Game/Demo/Maps/L_Demo -stage -pak -archive "-archivedirectory=$ProjectRoot/out/Demo" -utf8output -unattended
if ($LASTEXITCODE -ne 0) { throw "Packaging failed: $LASTEXITCODE" }
