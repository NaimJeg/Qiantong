param([ValidateSet('LiveCoding','Console','Key','Click')][string]$Mode='Console',[string]$Command,[int]$VirtualKey=0,[double]$X=0,[double]$Y=0)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class EditorInput {
 public delegate bool EnumProc(IntPtr h,IntPtr l);
 [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc f,IntPtr l);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h,out uint p);
 [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
 [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h,StringBuilder s,int n);
 [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
 [DllImport("user32.dll")] public static extern void keybd_event(byte v,byte s,uint f,UIntPtr e);
 [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h,uint msg,IntPtr w,IntPtr l);
 [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
 [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X,Y; }
 [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h,out RECT r);
 [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h,ref POINT p);
 [DllImport("user32.dll")] public static extern bool SetCursorPos(int x,int y);
 [DllImport("user32.dll")] public static extern void mouse_event(uint f,uint x,uint y,uint d,UIntPtr e);
 [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr h,int x,int y,int w,int height,bool r);
}
"@
$process=Get-Process UnrealEditor | Where-Object { $_.Path -eq 'D:\UE\Source\UnrealEngine-5.8.3-release\Engine\Binaries\Win64\UnrealEditor.exe' } | Select-Object -First 1
if(-not $process) { throw 'Project editor not found' }
$script:windows=@()
[EditorInput]::EnumWindows({param($h,$l) $id=0u; [EditorInput]::GetWindowThreadProcessId($h,[ref]$id)|Out-Null; if($id -eq $process.Id -and [EditorInput]::IsWindowVisible($h)) {$s=New-Object Text.StringBuilder 512;[EditorInput]::GetWindowText($h,$s,512)|Out-Null;$script:windows+=@{h=$h;title=$s.ToString()}};return $true},[IntPtr]::Zero)|Out-Null
$target=$windows|Where-Object {$_.title -match 'NetMode'}|Select-Object -First 1
if(-not $target) {$target=$windows|Where-Object {$_.title -match 'QiantongCore'}|Select-Object -First 1}
if(-not $target) {throw 'Project window not found'}
$h=$target.h
[EditorInput]::SetForegroundWindow($h)|Out-Null
Start-Sleep -Milliseconds 300
if([EditorInput]::GetForegroundWindow() -ne $h) {throw 'Target not foreground; no keys sent'}
function Key([byte]$k) { [EditorInput]::keybd_event($k,0,0,[UIntPtr]::Zero);Start-Sleep -Milliseconds 40;[EditorInput]::keybd_event($k,0,2,[UIntPtr]::Zero) }
if($Mode -eq 'LiveCoding') {
 [EditorInput]::keybd_event(0x11,0,0,[UIntPtr]::Zero);[EditorInput]::keybd_event(0x12,0,0,[UIntPtr]::Zero);Key 0x7A
 [EditorInput]::keybd_event(0x12,0,2,[UIntPtr]::Zero);[EditorInput]::keybd_event(0x11,0,2,[UIntPtr]::Zero)
} elseif($Mode -eq 'Click') {
 if($target.title -notmatch 'NetMode') {throw 'Click requires project PIE window'}
 $r=New-Object EditorInput+RECT;[EditorInput]::GetClientRect($h,[ref]$r)|Out-Null
 $scale=[Math]::Min($r.R/720.0,$r.B/1280.0)
 $pt=New-Object EditorInput+POINT;$pt.X=[int](($r.R-720*$scale)/2+$X*$scale);$pt.Y=[int](($r.B-1280*$scale)/2+$Y*$scale)
 [EditorInput]::ClientToScreen($h,[ref]$pt)|Out-Null;[EditorInput]::SetCursorPos($pt.X,$pt.Y)|Out-Null
 [EditorInput]::mouse_event(2,0,0,0,[UIntPtr]::Zero);Start-Sleep -Milliseconds 50;[EditorInput]::mouse_event(4,0,0,0,[UIntPtr]::Zero)
} elseif($Mode -eq 'Key') { Key $VirtualKey } else {
 if($target.title -match 'NetMode') {[EditorInput]::MoveWindow($h,900,70,740,1320,$true)|Out-Null}
 if($target.title -match 'NetMode') { Key 0xC0 } else {
  $r=New-Object EditorInput+RECT; [EditorInput]::GetClientRect($h,[ref]$r)|Out-Null
  $pt=New-Object EditorInput+POINT; $pt.X=[int]($r.R*.28);$pt.Y=$r.B-16
  [EditorInput]::ClientToScreen($h,[ref]$pt)|Out-Null; [EditorInput]::SetCursorPos($pt.X,$pt.Y)|Out-Null
  [EditorInput]::mouse_event(2,0,0,0,[UIntPtr]::Zero);[EditorInput]::mouse_event(4,0,0,0,[UIntPtr]::Zero)
 }
 Start-Sleep -Milliseconds 300
 [EditorInput]::keybd_event(0x11,0,0,[UIntPtr]::Zero);Key 0x41;[EditorInput]::keybd_event(0x11,0,2,[UIntPtr]::Zero)
 foreach($c in $Command.ToCharArray()) {
  if($c -eq ' ') { Key 0x20 } else {
   [EditorInput]::PostMessage($h,0x0102,[IntPtr][int][char]$c,[IntPtr]::Zero)|Out-Null
  }
  Start-Sleep -Milliseconds 12
 }
 Start-Sleep -Milliseconds 300
 Key 0x0D;Start-Sleep -Milliseconds 300
 if($target.title -match 'NetMode') { Key 0xC0 }

}
Write-Output "$Mode sent to project window: $($target.title)"
