param([string]$Executable = "$PSScriptRoot/../out/Demo/Windows/QiantongCore.exe")
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path $PSScriptRoot -Parent
$Log = Join-Path $ProjectRoot 'Saved/Logs/Demo-PackagedSmoke.log'
if (-not (Test-Path -LiteralPath $Executable)) { throw "Packaged executable missing: $Executable" }
$Started = Get-Date
$Process = Start-Process -FilePath $Executable -ArgumentList '-windowed','-ResX=720','-ResY=1280','-ForceRes','-DemoSmoke','-DemoOption=3','-DemoCaptureTick=121',"-abslog=`"$Log`"" -WindowStyle Hidden -PassThru
if (-not $Process.WaitForExit(180000)) { throw "Smoke timeout; process $($Process.Id) left running for inspection" }
# The root executable may bootstrap a child; require the actual runtime success log.
$Deadline = (Get-Date).AddMinutes(3)
do {
    if ((Test-Path $Log) -and (Get-Item $Log).LastWriteTime -ge $Started) {
        $Text = Get-Content $Log -Raw
        if ($Text -match 'QIANTONG_DEMO_SMOKE_PASS') { break }
        if ($Text -match 'Fatal error:') { throw 'Packaged runtime fatal error; inspect log' }
    }
    Start-Sleep -Seconds 1
} while ((Get-Date) -lt $Deadline)
if ($Text -notmatch 'QIANTONG_DEMO_SMOKE_PASS') { throw 'Packaged run did not finish' }
$ResultRoot=Join-Path (Split-Path $Executable -Parent) 'QiantongCore/Saved/DemoResults'
foreach($Allies in @(2,5)) {
    $ActualPath=Join-Path $ResultRoot "continuous-$Allies-seed1-option3.json"
    if(-not (Test-Path $ActualPath) -or (Get-Item $ActualPath).LastWriteTime -lt $Started) {
        throw 'This smoke test requires a freshly packaged demo-3 build. The retained Windows package may still be demo-1.'
    }
    $Actual=Get-Content -Encoding UTF8 $ActualPath -Raw | ConvertFrom-Json
    $Expected=Get-Content -Encoding UTF8 (Join-Path $ProjectRoot "Tests/Golden/continuous-$Allies-option3.json") -Raw | ConvertFrom-Json
    if(($Actual|ConvertTo-Json -Depth 20 -Compress) -cne ($Expected|ConvertTo-Json -Depth 20 -Compress)) { throw "Golden mismatch: $Allies allies" }
}
Write-Output 'PACKAGED DEMO-3 SMOKE PASS: both 3-sector runs match reviewed Golden results'
