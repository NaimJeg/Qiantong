param(
    [ValidateSet(60,120)][int]$Fps=60,
    [ValidateRange(1,30)][int]$Seconds=10
)
# Run only after explicit approval of a local PIE-window recording.
# Captures one verified project window by title, never the whole desktop.
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$ffmpeg=Join-Path $projectRoot 'Saved/CaptureDeps/imageio_ffmpeg/binaries/ffmpeg-win-x86_64-v7.1.exe'
if(-not (Test-Path -LiteralPath $ffmpeg)) { throw 'Install imageio-ffmpeg into Saved/CaptureDeps first.' }
Add-Type @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class SmoothCaptureWindow {
 public delegate bool EnumProc(IntPtr h,IntPtr l);
 [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc callback,IntPtr l);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h,out uint p);
 [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
 [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h,StringBuilder s,int n);
}
'@
$editor=Get-Process UnrealEditor | Where-Object {
    $_.Path -eq 'D:\UE\Source\UnrealEngine-5.8.3-release\Engine\Binaries\Win64\UnrealEditor.exe'
} | Select-Object -First 1
if(-not $editor) { throw 'Expected project editor is not running.' }
$script:titles=@()
[SmoothCaptureWindow]::EnumWindows({param($h,$l)
    $owner=[uint32]0
    [SmoothCaptureWindow]::GetWindowThreadProcessId($h,[ref]$owner)|Out-Null
    if($owner -eq $editor.Id -and [SmoothCaptureWindow]::IsWindowVisible($h)) {
        $title=New-Object Text.StringBuilder 512
        [SmoothCaptureWindow]::GetWindowText($h,$title,512)|Out-Null
        if($title.ToString() -match 'QiantongCore' -and $title.ToString() -match 'NetMode') {
            $script:titles+=$title.ToString()
        }
    }
    return $true
},[IntPtr]::Zero)|Out-Null
if($titles.Count -ne 1) { throw 'Require exactly one visible QiantongCore PIE window.' }
$output=Join-Path $projectRoot "docs/exec/evidence/smooth-${Fps}fps.mp4"
& $ffmpeg -hide_banner -n -f gdigrab -framerate $Fps -i "title=$($titles[0])" -t $Seconds -an -c:v libx264 -preset fast -crf 20 -pix_fmt yuv420p $output
if($LASTEXITCODE -ne 0) { throw 'PIE capture failed.' }
