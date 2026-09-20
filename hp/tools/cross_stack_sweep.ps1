# Stack a second cross CM on o2×sentmem (1,17). 256 KB, 64-way, mem 16.
# --cross 1,17 fixed; --cross2 all pairs + triples + cartesian of 2 MiB winners.
# Does not overwrite hp_v83.exe–hp_v93.exe. Does not commit.

param(
  [int]$Throttle = 64,
  [int]$Mem = 16,
  [int]$Nctx = 40,
  [string]$Pin = "1,17",
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
$OutDir = Join-Path $Lab "cross2_out"
$LogDir = Join-Path $Lab "cross2_logs"
$Csv = Join-Path $Lab "cross2_sweep.csv"
$Summary = Join-Path $Lab "cross2_sweep_top.txt"
$Exe = Join-Path $BuildDir "hp_cross2.exe"
$LabExe = Join-Path $Lab "hp_cross2.exe"
$BaseExe = Join-Path $BuildDir "hp_c_base.exe"
$LabBase = Join-Path $Lab "hp_c_base.exe"

$Names = @(
  "o1","o2","o3","o4","o6","word","sp13","sp24","col","tag",
  "wbi","wstr_sp","brk","link","num","sen","sentst","sentmem","sengrp","nest",
  "para","line","state","tpl","infokey","o6b","linkpipe","cat","heading","title",
  "sectitle","wikistack","capmask","uppergap","wordlen","wikibold","sentpos","statetrans","cappara","refgroup"
)

$Winners = @("1,17","0,5,17","5,12,17","5,17","5,15,17","17,18")

function Get-Flags {
  param([string[]]$Extra)
  $flags = New-Object System.Collections.Generic.List[string]
  foreach ($f in $script:V78Flags) {
    if ($f -notmatch '^-DHP_SLOT_MAX=') { [void]$flags.Add([string]$f) }
  }
  foreach ($e in $Extra) { if ($e) { [void]$flags.Add($e) } }
  return , $flags.ToArray()
}

function Compile-Bin {
  param($Name, $OutExe, [string[]]$Extra)
  New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
  foreach ($n in 83..93) {
    $p = Join-Path $BuildDir ("hp_v{0}.exe" -f $n)
    if ([IO.Path]::GetFullPath($OutExe) -eq [IO.Path]::GetFullPath($p)) {
      throw "refusing to overwrite $p"
    }
  }
  $flags = Get-Flags $Extra
  Write-Host "COMPILE $Name"
  & g++ @flags $Src -o $OutExe
  if ($LASTEXITCODE -ne 0) { throw "compile failed $Name" }
}

function Spec-Label([string]$spec) {
  if (-not $spec -or $spec -eq "-1,-1") { return "idle" }
  $parts = $spec.Split(',') | ForEach-Object { [int]$_ }
  return (($parts | ForEach-Object { if ($_ -ge 0 -and $_ -lt $Names.Count) { $Names[$_] } else { "?" } }) -join "*")
}

New-Item -ItemType Directory -Force -Path $Lab, $OutDir, $LogDir | Out-Null

if (-not $SkipCompile) {
  Compile-Bin "hp_cross2" $Exe @("-DHP_CROSS_CM=2")
}
if ($CompileOnly) { return }
if (-not (Test-Path $Exe)) { throw "missing $Exe" }
Copy-Item -Force $Exe $LabExe
Copy-Item -Force $InSrc $InLab
if (Test-Path $BaseExe) { Copy-Item -Force $BaseExe $LabBase }

$done = @{}
if (Test-Path $OutDir) {
  Get-ChildItem $OutDir -Filter *.hp | Where-Object { $_.Length -gt 32 } | ForEach-Object {
    $done[$_.BaseName] = $true
  }
}

$queue = New-Object System.Collections.Generic.List[object]
function Enqueue {
  param($Id, $Kind, $Spec, $Spec2, $UseBase)
  if ($done.ContainsKey($Id)) { return }
  [void]$queue.Add([pscustomobject]@{
    id = $Id; kind = $Kind; spec = $Spec; spec2 = $Spec2
    exe = $(if ($UseBase) { $LabBase } else { $LabExe })
  })
}

Enqueue "base125" "base125" $null $null $true
Enqueue "idle2" "idle2" "-1,-1" "-1,-1" $false
Enqueue "x1_only" "x1_only" $Pin "-1,-1" $false
for ($a = 0; $a -lt $Nctx; $a++) {
  for ($b = $a + 1; $b -lt $Nctx; $b++) {
    Enqueue ("p_{0}_{1}" -f $a, $b) "pair" $Pin ("{0},{1}" -f $a, $b) $false
    for ($c = $b + 1; $c -lt $Nctx; $c++) {
      Enqueue ("t_{0}_{1}_{2}" -f $a, $b, $c) "triple" $Pin ("{0},{1},{2}" -f $a, $b, $c) $false
    }
  }
}
for ($i = 0; $i -lt $Winners.Count; $i++) {
  for ($j = $i + 1; $j -lt $Winners.Count; $j++) {
    $id = "s_{0}__{1}" -f ($Winners[$i] -replace ',', '-'), ($Winners[$j] -replace ',', '-')
    Enqueue $id "stack" $Winners[$i] $Winners[$j] $false
  }
}

Write-Host ("QUEUE {0} jobs (skip {1} done) throttle {2} pin {3}" -f $queue.Count, $done.Count, $Throttle, $Pin)

if (-not (Test-Path $Csv)) {
  "id,kind,spec,spec2,label,bytes,utc" | Set-Content -Encoding utf8 $Csv
}

$running = New-Object System.Collections.Generic.List[object]
function Reap {
  $still = New-Object System.Collections.Generic.List[object]
  foreach ($j in $running) {
    $j.proc.Refresh()
    if (-not $j.proc.HasExited) { [void]$still.Add($j); continue }
    $j.proc.WaitForExit() | Out-Null
    $bytes = if (Test-Path $j.hp) { [int64](Get-Item $j.hp).Length } else { -1 }
    $label = if ($j.kind -eq "stack") {
      (Spec-Label $j.spec) + "+" + (Spec-Label $j.spec2)
    } elseif ($j.kind -eq "x1_only") { Spec-Label $j.spec }
    elseif ($j.kind -eq "base125") { "base125" }
    elseif ($j.kind -eq "idle2") { "idle2" }
    else { Spec-Label $j.spec2 }
    $line = "{0},{1},""{2}"",""{3}"",{4},{5},{6}" -f $j.id, $j.kind, $j.spec, $j.spec2, $label, $bytes, ([DateTime]::UtcNow.ToString("o"))
    Add-Content -Encoding utf8 $Csv $line
    if ($bytes -gt 32 -and $j.kind -match 'pair|triple') {
      Remove-Item -Force $j.hp -ErrorAction SilentlyContinue
    }
    Remove-Item -Force $j.err, $j.outlog -ErrorAction SilentlyContinue
  }
  $running.Clear()
  foreach ($j in $still) { [void]$running.Add($j) }
}

$nStarted = 0
$t0 = Get-Date
foreach ($job in $queue) {
  while ($running.Count -ge $Throttle) { Start-Sleep -Milliseconds 200; Reap }
  $hp = Join-Path $OutDir ($job.id + ".hp")
  $err = Join-Path $LogDir ($job.id + ".err")
  $outlog = Join-Path $LogDir ($job.id + ".out")
  $arg = New-Object System.Collections.Generic.List[string]
  [void]$arg.Add("c"); [void]$arg.Add("--mem"); [void]$arg.Add("$Mem")
  if ($job.spec) { [void]$arg.Add("--cross"); [void]$arg.Add($job.spec) }
  if ($job.spec2) { [void]$arg.Add("--cross2"); [void]$arg.Add($job.spec2) }
  [void]$arg.Add($InLab); [void]$arg.Add($hp)
  $proc = Start-Process -FilePath $job.exe -ArgumentList $arg.ToArray() `
    -WorkingDirectory $Lab -WindowStyle Hidden `
    -RedirectStandardError $err -RedirectStandardOutput $outlog -PassThru
  [void]$running.Add([pscustomobject]@{
    id = $job.id; kind = $job.kind; spec = $job.spec; spec2 = $job.spec2
    hp = $hp; err = $err; outlog = $outlog; proc = $proc
  })
  $nStarted++
  if (($nStarted % 64) -eq 0) {
    $elapsed = [math]::Round(((Get-Date) - $t0).TotalSeconds, 0)
    Write-Host ("started {0}/{1} live {2} elapsed {3}s" -f $nStarted, $queue.Count, $running.Count, $elapsed)
  }
}
while ($running.Count -gt 0) { Start-Sleep -Milliseconds 400; Reap }

$files = @(Get-ChildItem $OutDir -Filter *.hp -ErrorAction SilentlyContinue)
$rows = @()
if (Test-Path $Csv) {
  $rows = @(Import-Csv $Csv | Where-Object { [int64]$_.bytes -gt 32 })
}
$x1 = ($rows | Where-Object { $_.id -eq "x1_only" } | Select-Object -First 1)
$base = ($rows | Where-Object { $_.id -eq "base125" } | Select-Object -First 1)
$x1n = if ($x1) { [int64]$x1.bytes } else { 0 }
$basen = if ($base) { [int64]$base.bytes } else { 0 }
$ranked = $rows | ForEach-Object {
  $b = [int64]$_.bytes
  [pscustomobject]@{
    id = $_.id; kind = $_.kind; label = $_.label; bytes = $b
    dX1 = $b - $x1n; d125 = $b - $basen
  }
} | Sort-Object bytes
$beatX1 = @($ranked | Where-Object { $_.kind -match 'pair|triple|stack' -and $_.dX1 -lt 0 })
$lines = New-Object System.Collections.Generic.List[string]
[void]$lines.Add(("256 KB stack pin={0}  ok={1}  x1_only={2}  base125={3}" -f $Pin, $ranked.Count, $x1n, $basen))
[void]$lines.Add(("beat x1_only: {0}" -f $beatX1.Count))
[void]$lines.Add("TOP 40:")
$ranked | Select-Object -First 40 | ForEach-Object {
  [void]$lines.Add(("{0,8}  {1,-8}  {2,-40}  dX1={3,6}  d125={4,6}" -f $_.bytes, $_.kind, $_.label, $_.dX1, $_.d125))
}
$lines | Set-Content -Encoding utf8 $Summary
$lines | ForEach-Object { Write-Host $_ }
Write-Host "DONE csv=$Csv summary=$Summary files_left=$($files.Count)"
