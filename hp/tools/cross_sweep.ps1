# 256 KB cross-context sweep: idle + C(40,2) pairs + C(40,3) triples.
# One extra CM (hp_cross.exe). 64-way. mem 16. Encode-only.
# Does not overwrite hp_v83.exe–hp_v93.exe. Does not commit.

param(
  [int]$Throttle = 64,
  [int]$Mem = 16,
  [int]$Nctx = 40,
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
$OutDir = Join-Path $Lab "cross_out"
$LogDir = Join-Path $Lab "cross_logs"
$Csv = Join-Path $Lab "cross_sweep.csv"
$Summary = Join-Path $Lab "cross_sweep_top.txt"
$CrossExe = Join-Path $BuildDir "hp_cross.exe"
$BaseExe = Join-Path $BuildDir "hp_c_base.exe"
$LabCross = Join-Path $Lab "hp_cross.exe"
$LabBase = Join-Path $Lab "hp_c_base.exe"

$Names = @(
  "o1","o2","o3","o4","o6","word","sp13","sp24","col","tag",
  "wbi","wstr_sp","brk","link","num","sen","sentst","sentmem","sengrp","nest",
  "para","line","state","tpl","infokey","o6b","linkpipe","cat","heading","title",
  "sectitle","wikistack","capmask","uppergap","wordlen","wikibold","sentpos","statetrans","cappara","refgroup"
)

function Get-Flags {
  param([string[]]$Extra)
  $flags = New-Object System.Collections.Generic.List[string]
  foreach ($f in $script:V78Flags) {
    if ($f -notmatch '^-DHP_SLOT_MAX=') { [void]$flags.Add([string]$f) }
  }
  foreach ($e in $Extra) {
    if (-not $e) { continue }
    if ($e -match '^-D([A-Za-z0-9_]+)=') {
      $name = $Matches[1]
      $replaced = $false
      for ($i = 0; $i -lt $flags.Count; $i++) {
        if ($flags[$i] -match "^-D${name}=") {
          $flags[$i] = $e
          $replaced = $true
        }
      }
      if (-not $replaced) { [void]$flags.Add($e) }
    } else {
      [void]$flags.Add($e)
    }
  }
  return , $flags.ToArray()
}

function Compile-Bin {
  param($Name, $OutExe, [string[]]$Extra)
  New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
  $protected = 83..93 | ForEach-Object { Join-Path $BuildDir ("hp_v{0}.exe" -f $_) }
  foreach ($p in $protected) {
    if ([IO.Path]::GetFullPath($OutExe) -eq [IO.Path]::GetFullPath($p)) {
      throw "refusing to overwrite $p"
    }
  }
  $flags = Get-Flags $Extra
  Write-Host "COMPILE $Name"
  & g++ @flags $Src -o $OutExe
  if ($LASTEXITCODE -ne 0) { throw "compile failed $Name" }
}

function Name-Of([int]$i) {
  if ($i -ge 0 -and $i -lt $Names.Count) { return $Names[$i] }
  return "?"
}

function Spec-Label([string]$spec) {
  if ($spec -eq "-1,-1") { return "idle" }
  $parts = $spec.Split(',') | ForEach-Object { [int]$_ }
  $ns = $parts | ForEach-Object { Name-Of $_ }
  return ($ns -join "*")
}

New-Item -ItemType Directory -Force -Path $Lab, $OutDir, $LogDir | Out-Null

if (-not $SkipCompile) {
  Compile-Bin "hp_c_base" $BaseExe @()
  Compile-Bin "hp_cross" $CrossExe @("-DHP_CROSS_CM=1")
}

if ($CompileOnly) { return }

if (-not (Test-Path $CrossExe)) { throw "missing $CrossExe" }
if (-not (Test-Path $BaseExe)) { throw "missing $BaseExe" }
if (-not (Test-Path $InSrc)) { throw "missing $InSrc" }
Copy-Item -Force $CrossExe $LabCross
Copy-Item -Force $BaseExe $LabBase
Copy-Item -Force $InSrc $InLab

$done = @{}
if (Test-Path $Csv) {
  Import-Csv $Csv | ForEach-Object {
    if ($_.exit -eq "0") { $done[$_.id] = $true }
  }
} else {
  "id,kind,spec,label,bytes,delta_idle,delta_125,exit,wall_s,utc" | Set-Content -Encoding utf8 $Csv
}

$queue = New-Object System.Collections.Generic.List[object]
function Enqueue {
  param($Id, $Kind, $Spec, $Exe)
  if ($done.ContainsKey($Id)) { return }
  [void]$queue.Add([pscustomobject]@{ id = $Id; kind = $Kind; spec = $Spec; exe = $Exe })
}

Enqueue "base125" "base125" "" $LabBase
Enqueue "idle" "idle" "-1,-1" $LabCross
for ($a = 0; $a -lt $Nctx; $a++) {
  for ($b = $a + 1; $b -lt $Nctx; $b++) {
    Enqueue ("p_{0}_{1}" -f $a, $b) "pair" ("{0},{1}" -f $a, $b) $LabCross
    for ($c = $b + 1; $c -lt $Nctx; $c++) {
      Enqueue ("t_{0}_{1}_{2}" -f $a, $b, $c) "triple" ("{0},{1},{2}" -f $a, $b, $c) $LabCross
    }
  }
}

Write-Host ("QUEUE {0} jobs (skip {1} done) throttle {2} mem {3}" -f $queue.Count, $done.Count, $Throttle, $Mem)

$running = New-Object System.Collections.Generic.List[object]
$idleBytes = $null
$baseBytes = $null
if ($done.ContainsKey("idle") -or (Test-Path $Csv)) {
  Import-Csv $Csv | ForEach-Object {
    if ($_.id -eq "idle" -and $_.exit -eq "0") { $idleBytes = [int64]$_.bytes }
    if ($_.id -eq "base125" -and $_.exit -eq "0") { $baseBytes = [int64]$_.bytes }
  }
}

function Record-Job {
  param($j)
  $exit = $j.proc.ExitCode
  $hp = $j.hp
  $bytes = -1
  if ((Test-Path $hp) -and $exit -eq 0) { $bytes = [int64](Get-Item $hp).Length }
  if ($j.id -eq "idle" -and $bytes -gt 0) { $script:idleBytes = $bytes }
  if ($j.id -eq "base125" -and $bytes -gt 0) { $script:baseBytes = $bytes }
  $dIdle = if ($null -ne $script:idleBytes -and $bytes -gt 0) { $bytes - $script:idleBytes } else { "" }
  $d125 = if ($null -ne $script:baseBytes -and $bytes -gt 0) { $bytes - $script:baseBytes } else { "" }
  $wall = [math]::Round(((Get-Date) - $j.start).TotalSeconds, 1)
  $label = if ($j.kind -eq "base125") { "base125" } else { Spec-Label $j.spec }
  $line = "{0},{1},""{2}"",{3},{4},{5},{6},{7},{8},{9}" -f `
    $j.id, $j.kind, $j.spec, $label, $bytes, $dIdle, $d125, $exit, $wall, ([DateTime]::UtcNow.ToString("o"))
  Add-Content -Encoding utf8 $Csv $line
  if ($exit -eq 0 -and (Test-Path $hp) -and $j.kind -ne "idle" -and $j.kind -ne "base125") {
    Remove-Item -Force $hp -ErrorAction SilentlyContinue
  }
  Remove-Item -Force $j.err, $j.outlog -ErrorAction SilentlyContinue
}

function Reap {
  $still = New-Object System.Collections.Generic.List[object]
  foreach ($j in $running) {
    if ($j.proc.HasExited) { Record-Job $j }
    else { [void]$still.Add($j) }
  }
  $running.Clear()
  foreach ($j in $still) { [void]$running.Add($j) }
}

$nStarted = 0
$t0 = Get-Date
foreach ($job in $queue) {
  while ($running.Count -ge $Throttle) {
    Start-Sleep -Milliseconds 200
    Reap
  }
  $hp = Join-Path $OutDir ($job.id + ".hp")
  $err = Join-Path $LogDir ($job.id + ".err")
  $outlog = Join-Path $LogDir ($job.id + ".out")
  $arg = New-Object System.Collections.Generic.List[string]
  [void]$arg.Add("c")
  [void]$arg.Add("--mem"); [void]$arg.Add("$Mem")
  if ($job.spec) {
    [void]$arg.Add("--cross"); [void]$arg.Add($job.spec)
  }
  [void]$arg.Add($InLab)
  [void]$arg.Add($hp)
  $proc = Start-Process -FilePath $job.exe -ArgumentList $arg.ToArray() `
    -WorkingDirectory $Lab -WindowStyle Hidden `
    -RedirectStandardError $err -RedirectStandardOutput $outlog -PassThru
  [void]$running.Add([pscustomobject]@{
    id = $job.id; kind = $job.kind; spec = $job.spec; exe = $job.exe
    hp = $hp; err = $err; outlog = $outlog; proc = $proc; start = Get-Date
  })
  $nStarted++
  if (($nStarted % 64) -eq 0) {
    $elapsed = [math]::Round(((Get-Date) - $t0).TotalSeconds, 0)
    Write-Host ("started {0}/{1} live {2} elapsed {3}s" -f $nStarted, $queue.Count, $running.Count, $elapsed)
  }
}

while ($running.Count -gt 0) {
  Start-Sleep -Milliseconds 400
  Reap
}

# Rank vs idle (negative = smaller archive).
$rows = @(Import-Csv $Csv | Where-Object { $_.exit -eq "0" -and $_.bytes -ne "-1" })
$idleRow = $rows | Where-Object { $_.id -eq "idle" } | Select-Object -First 1
$baseRow = $rows | Where-Object { $_.id -eq "base125" } | Select-Object -First 1
$idleN = if ($idleRow) { [int64]$idleRow.bytes } else { 0 }
$baseN = if ($baseRow) { [int64]$baseRow.bytes } else { 0 }

$ranked = $rows | ForEach-Object {
  $b = [int64]$_.bytes
  [pscustomobject]@{
    id = $_.id; kind = $_.kind; spec = $_.spec; label = $_.label; bytes = $b
    dIdle = $b - $idleN; d125 = $b - $baseN
  }
} | Sort-Object bytes

$winsIdle = @($ranked | Where-Object { $_.kind -ne "idle" -and $_.kind -ne "base125" -and $_.dIdle -lt 0 })
$wins125 = @($ranked | Where-Object { $_.kind -ne "idle" -and $_.kind -ne "base125" -and $_.d125 -lt 0 })

$lines = New-Object System.Collections.Generic.List[string]
[void]$lines.Add(("256 KB mem {0} SLOT_MAX=22(hard)  jobs_ok={1}  idle={2}  base125={3}" -f $Mem, $rows.Count, $idleN, $baseN))
[void]$lines.Add(("beat idle: {0}   beat 125: {1}" -f $winsIdle.Count, $wins125.Count))
[void]$lines.Add("TOP 40 by archive bytes:")
$ranked | Select-Object -First 40 | ForEach-Object {
  [void]$lines.Add(("{0,8}  {1,-10}  {2,-28}  dIdle={3,6}  d125={4,6}" -f $_.bytes, $_.kind, $_.label, $_.dIdle, $_.d125))
}
$lines | Set-Content -Encoding utf8 $Summary
$lines | ForEach-Object { Write-Host $_ }

# Condense ablations: drop correlated twins / extra match, same 256 KB mem 16.
$condense = @(
  @{ n = "nohash2"; extra = @("-DHP_HASH2_O6=0") },
  @{ n = "nomatchx"; extra = @("-DHP_MATCH_18=0","-DHP_MATCH_01=0","-DHP_MATCH_02=0","-DHP_MATCH_05=0","-DHP_SKIPK_MOD=0","-DHP_SKIP3_MOD=0","-DHP_SKIP4_MOD=0") },
  @{ n = "notwin"; extra = @(
      "-DHP_HASH2_O6=0",
      "-DHP_SLOT_O34B=0","-DHP_SLOT_O34C=0","-DHP_SLOT_O34D=0","-DHP_SLOT_O34E=0","-DHP_SLOT_O34F=0",
      "-DHP_SLOT_O6B=0","-DHP_SLOT_O6C=0","-DHP_SLOT_O6D=0","-DHP_SLOT_O6E=0","-DHP_SLOT_O6F=0",
      "-DHP_SLOT_O6G=0","-DHP_SLOT_O6H=0","-DHP_SLOT_O6I=0"
    )
  }
)
foreach ($c in $condense) {
  $exe = Join-Path $BuildDir ("hp_c_{0}.exe" -f $c.n)
  Compile-Bin ("hp_c_" + $c.n) $exe $c.extra
  $labExe = Join-Path $Lab ("hp_c_{0}.exe" -f $c.n)
  Copy-Item -Force $exe $labExe
  $hp = Join-Path $OutDir ($c.n + ".hp")
  $err = Join-Path $LogDir ($c.n + ".err")
  $outlog = Join-Path $LogDir ($c.n + ".out")
  $t1 = Get-Date
  $p = Start-Process -FilePath $labExe -ArgumentList @("c","--mem","$Mem",$InLab,$hp) `
    -WorkingDirectory $Lab -WindowStyle Hidden `
    -RedirectStandardError $err -RedirectStandardOutput $outlog -PassThru -Wait
  $bytes = if (Test-Path $hp) { [int64](Get-Item $hp).Length } else { -1 }
  $wall = [math]::Round(((Get-Date) - $t1).TotalSeconds, 1)
  $dIdle = if ($idleN -gt 0 -and $bytes -gt 0) { $bytes - $idleN } else { "" }
  $d125 = if ($baseN -gt 0 -and $bytes -gt 0) { $bytes - $baseN } else { "" }
  $line = "{0},condense,,{0},{1},{2},{3},{4},{5},{6}" -f $c.n, $bytes, $dIdle, $d125, $p.ExitCode, $wall, ([DateTime]::UtcNow.ToString("o"))
  Add-Content -Encoding utf8 $Csv $line
  Write-Host ("CONDENSE {0} bytes={1} d125={2} wall={3}s" -f $c.n, $bytes, $d125, $wall)
}

Write-Host "DONE csv=$Csv summary=$Summary"
