param([Parameter(Mandatory=$true)][string]$EngineRoot)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
$Editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$Report = Join-Path $ProjectRoot 'Saved/Automation/Demo'
& $Editor "$ProjectRoot/QiantongCore.uproject" -unattended -NullRHI -nosound '-ExecCmds=Automation RunTests Qiantong.Demo' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$Report" "-abslog=$ProjectRoot/Saved/Logs/Demo-Tests.log"
if ($LASTEXITCODE -ne 0) { throw "UE tests failed: $LASTEXITCODE" }
$Results = Get-Content (Join-Path $Report 'index.json') -Raw | ConvertFrom-Json
if ($Results.failed -ne 0 -or $Results.succeeded -ne 7 -or $Results.succeededWithWarnings -ne 0) { throw 'Incomplete or failed automation report' }
Write-Output "Demo automation passed: $($Results.succeeded)"
