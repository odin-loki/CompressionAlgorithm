# Integer LSTM insertion screen: one extra -D per leftover binary.
# 256 KB mem 16. HP_SLOT_MAX hard-capped at 22. Does not overwrite hp_v83–v93. No commit.

param(
  [int]$Throttle = 8,
  [int]$Mem = 16,
  [switch]$CompileOnly,
  [switch]$SkipCompile
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if (-not (Test-Path (Join-Path $root "hp\src\main.cpp"))) {
  $root = "C:\Users\odinl\OneDrive\Desktop\Compression Algorithm"
}
Set-Location $root
. (Join-Path $PSScriptRoot "v78_flags.ps1")

$BuildDir = Join-Path $root "hp\build"
$Src = Join-Path $root "hp\src\main.cpp"
$Lab = Join-Path $env:LOCALAPPDATA "hp_lab"
$InSrc = Join-Path $root "hp\data\proxy256k.xml"
$InLab = Join-Path $Lab "proxy256k.xml"
$OutDir = Join-Path $Lab "lstm_out"
$LogDir = Join-Path $Lab "lstm_logs"
$Csv = Join-Path $Lab "lstm_screen2.csv"
$OutDir = Join-Path $Lab "lstm_out2"
$LogDir = Join-Path $Lab "lstm_logs2"

$Jobs = @(
  @{ n = "base"; extra = @() },
  @{ n = "exp64"; extra = @("-DHP_LSTM=1","-DHP_LSTM_H=64") },
  @{ n = "exp128"; extra = @("-DHP_LSTM=1","-DHP_LSTM_H=128") },
  @{ n = "mixin"; extra = @("-DHP_LSTM=1","-DHP_LSTM_MIXIN=1") },
  @{ n = "lr3"; extra = @("-DHP_LSTM=1","-DHP_LSTM_LR=3") },
  @{ n = "lr8"; extra = @("-DHP_LSTM=1","-DHP_LSTM_LR=8") },
  @{ n = "layer2"; extra = @("-DHP_LSTM=1","-DHP_LSTM_LAYERS=2") },
  @{ n = "h64mix"; extra = @("-DHP_LSTM=1","-DHP_LSTM_H=64","-DHP_LSTM_MIXIN=1") }
)

function Get-Flags([string[]]$Extra) {
  $flags = New-Object System.Collections.Generic.List[string]
  foreach ($f in $script:V78Flags) {
    if ($f -notmatch '^-DHP_SLOT_MAX=') { [void]$flags.Add([string]$f) }
  }
  foreach ($e in $Extra) {
    if ($e -match '^-D([A-Za-z0-9_]+)=') {
      $name = $Matches[1]
      $hit = $false
      for ($i = 0; $i -lt $flags.Count; $i++) {
        if ($flags[$i] -match "^-D${name}=") { $flags[$i] = $e; $hit = $true }
      }
      if (-not $hit) { [void]$flags.Add($e) }
    }
  }
  return , $flags.ToArray()
}

function Compile-Bin($Name, [string[]]$Extra) {
  $exe = Join-Path $BuildDir ("hp_lstm_{0}.exe" -f $Name)
  if ($Name -eq "base") { $exe = Join-Path $BuildDir "hp_c_base.exe" }
  foreach ($n in 83..93) {
    $p = Join-Path $BuildDir ("hp_v{0}.exe" -f $n)
    if ([IO.Path]::GetFullPath($exe) -eq [IO.Path]::GetFullPath($p)) { throw "champ overwrite" }
  }
  if ($Name -eq "base" -and (Test-Path $exe)) {
    Write-Host "KEEP $exe"
    return $exe
  }
  $flags = Get-Flags $Extra
  Write-Host "COMPILE hp_lstm_$Name"
  & g++ @flags $Src -o $exe
  if ($LASTEXITCODE -ne 0) { throw "compile failed $Name" }
  return $exe
}

New-Item -ItemType Directory -Force -Path $BuildDir, $Lab, $OutDir, $LogDir | Out-Null
$exes = @{}
if (-not $SkipCompile) {
  foreach ($j in $Jobs) { $exes[$j.n] = Compile-Bin $j.n $j.extra }
}
if ($CompileOnly) { return }

Copy-Item -Force $InSrc $InLab
"name,bytes,delta_base,wall_s,exit,utc" | Set-Content -Encoding utf8 $Csv

$script:done = @()
$running = New-Object System.Collections.Generic.List[object]
foreach ($j in $Jobs) {
  while ($running.Count -ge $Throttle) {
    Start-Sleep -Milliseconds 300
    $still = New-Object System.Collections.Generic.List[object]
    foreach ($r in $running) {
      $r.proc.Refresh()
      if (-not $r.proc.HasExited) { [void]$still.Add($r); continue }
      $r.proc.WaitForExit() | Out-Null
      $bytes = if (Test-Path $r.hp) { (Get-Item $r.hp).Length } else { -1 }
      $wall = [math]::Round(((Get-Date) - $r.start).TotalSeconds, 1)
      Write-Host ("DONE {0} bytes={1} wall={2}s" -f $r.n, $bytes, $wall)
      $script:done += [pscustomobject]@{ n = $r.n; bytes = $bytes; wall = $wall; exit = $r.proc.ExitCode }
    }
    $running.Clear(); foreach ($x in $still) { [void]$running.Add($x) }
  }
  $exe = if ($exes.ContainsKey($j.n)) { $exes[$j.n] } else {
    if ($j.n -eq "base") { Join-Path $BuildDir "hp_c_base.exe" } else { Join-Path $BuildDir ("hp_lstm_{0}.exe" -f $j.n) }
  }
  $labExe = Join-Path $Lab ("run_lstm_{0}.exe" -f $j.n)
  Copy-Item -Force $exe $labExe
  $hp = Join-Path $OutDir ($j.n + ".hp")
  $err = Join-Path $LogDir ($j.n + ".err")
  $outlog = Join-Path $LogDir ($j.n + ".out")
  $p = Start-Process -FilePath $labExe -ArgumentList @("c","--mem","$Mem",$InLab,$hp) `
    -WorkingDirectory $Lab -WindowStyle Hidden -RedirectStandardError $err -RedirectStandardOutput $outlog -PassThru
  [void]$running.Add([pscustomobject]@{ n = $j.n; proc = $p; hp = $hp; start = Get-Date })
  Write-Host ("START {0}" -f $j.n)
}
while ($running.Count -gt 0) {
  Start-Sleep -Milliseconds 400
  $still = New-Object System.Collections.Generic.List[object]
  foreach ($r in $running) {
    $r.proc.Refresh()
    if (-not $r.proc.HasExited) { [void]$still.Add($r); continue }
    $r.proc.WaitForExit() | Out-Null
    $bytes = if (Test-Path $r.hp) { (Get-Item $r.hp).Length } else { -1 }
    $wall = [math]::Round(((Get-Date) - $r.start).TotalSeconds, 1)
    Write-Host ("DONE {0} bytes={1} wall={2}s" -f $r.n, $bytes, $wall)
    $script:done += [pscustomobject]@{ n = $r.n; bytes = [int64]$bytes; wall = $wall; exit = $r.proc.ExitCode }
  }
  $running.Clear(); foreach ($x in $still) { [void]$running.Add($x) }
}

$base = ($script:done | Where-Object { $_.n -eq "base" } | Select-Object -First 1).bytes
foreach ($d in ($script:done | Sort-Object bytes)) {
  $delta = $d.bytes - $base
  $line = "{0},{1},{2},{3},{4},{5}" -f $d.n, $d.bytes, $delta, $d.wall, $d.exit, ([DateTime]::UtcNow.ToString("o"))
  Add-Content -Encoding utf8 $Csv $line
  Write-Host ("{0,10} {1,-10} d={2,6}" -f $d.bytes, $d.n, $delta)
}
Write-Host "LSTM_SCREEN_DONE csv=$Csv"
