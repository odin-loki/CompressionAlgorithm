# Third extra CM on baked stack o2×sentmem + word×brk. 256 KB, 64-way, mem 16.
# --cross 1,17 --cross2 5,12 fixed; --cross3 all pairs + triples.
# Does not overwrite hp_v83.exe–hp_v93.exe. Does not commit.

param(
  [int]$Throttle = 64,
  [int]$Mem = 16,
  [int]$Nctx = 40,
  [string]$Pin = "1,17",
  [string]$Pin2 = "5,12",
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
$OutDir = Join-Path $Lab "cross3_out"
$LogDir = Join-Path $Lab "cross3_logs"
$Csv = Join-Path $Lab "cross3_sweep.csv"
$Summary = Join-Path $Lab "cross3_sweep_top.txt"
$Exe = Join-Path $BuildDir "hp_cross3.exe"
$LabExe = Join-Path $Lab "hp_cross3.exe"
$BaseExe = Join-Path $BuildDir "hp_c_base.exe"
$LabBase = Join-Path $Lab "hp_c_base.exe"
$StackExe = Join-Path $BuildDir "hp_stack.exe"
$LabStack = Join-Path $Lab "hp_stack.exe"

$Names = @(
  "o1","o2","o3","o4","o6","word","sp13","sp24","col","tag",
  "wbi","wstr_sp","brk","link","num","sen","sentst","sentmem","sengrp","nest",
  "para","line","state","tpl","infokey","o6b","linkpipe","cat","heading","title",
  "sectitle","wikistack","capmask","uppergap","wordlen","wikibold","sentpos","statetrans","cappara","refgroup"
)

$Winners = @("0,5,17","5,12,17","5,17","5,15,17","17,18")

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

function Same-Spec([string]$a, [string]$b) {
  $sa = (($a.Split(',') | ForEach-Object { [int]$_ } | Where-Object { $_ -ge 0 } | Sort-Object) -join ',')
  $sb = (($b.Split(',') | ForEach-Object { [int]$_ } | Where-Object { $_ -ge 0 } | Sort-Object) -join ',')
  return $sa -eq $sb
}

New-Item -ItemType Directory -Force -Path $Lab, $OutDir, $LogDir | Out-Null

if (-not $SkipCompile) {
  Compile-Bin "hp_cross3" $Exe @("-DHP_CROSS_CM=3")
  if (-not (Test-Path $StackExe)) {
    Compile-Bin "hp_stack" $StackExe @("-DHP_CROSS_STACK=1")
  }
}
if ($CompileOnly) { return }
if (-not (Test-Path $Exe)) { throw "missing $Exe" }
Copy-Item -Force $Exe $LabExe
Copy-Item -Force $InSrc $InLab
if (Test-Path $BaseExe) { Copy-Item -Force $BaseExe $LabBase }
if (Test-Path $StackExe) { Copy-Item -Force $StackExe $LabStack }

$done = @{}
if (Test-Path $OutDir) {
  Get-ChildItem $OutDir -Filter *.hp | Where-Object { $_.Length -gt 32 } | ForEach-Object {
    $done[$_.BaseName] = $true
  }
}

$queue = New-Object System.Collections.Generic.List[object]
function Enqueue {
  param($Id, $Kind, $Spec3, $UseBase, $UseStack)
  if ($done.ContainsKey($Id)) { return }
  $which = $LabExe
  if ($UseBase) { $which = $LabBase }
  elseif ($UseStack) { $which = $LabStack }
  [void]$queue.Add([pscustomobject]@{
    id = $Id; kind = $Kind; spec3 = $Spec3; exe = $which
  })
}

Enqueue "base125" "base125" $null $true $false
Enqueue "stack2" "stack2" $null $false $true
Enqueue "idle3" "idle3" "-1,-1" $false $false
for ($a = 0; $a -lt $Nctx; $a++) {
  for ($b = $a + 1; $b -lt $Nctx; $b++) {
    $p = "{0},{1}" -f $a, $b
    if (-not (Same-Spec $p $Pin) -and -not (Same-Spec $p $Pin2)) {
      Enqueue ("p_{0}_{1}" -f $a, $b) "pair" $p $false $false
    }
    for ($c = $b + 1; $c -lt $Nctx; $c++) {
      $t = "{0},{1},{2}" -f $a, $b, $c
      Enqueue ("t_{0}_{1}_{2}" -f $a, $b, $c) "triple" $t $false $false
    }
  }
}
foreach ($w in $Winners) {
  $id = "w_{0}" -f ($w -replace ',', '-')
  Enqueue $id "winner" $w $false $false
}

Write-Host ("QUEUE {0} jobs (skip {1} done) throttle {2} pin {3}+{4}" -f $queue.Count, $done.Count, $Throttle, $Pin, $Pin2)

if (-not (Test-Path $Csv)) {
  "id,kind,spec3,label,bytes,utc" | Set-Content -Encoding utf8 $Csv
}

$running = New-Object System.Collections.Generic.List[object]
function Reap {
  $still = New-Object System.Collections.Generic.List[object]
  foreach ($j in $running) {
    $j.proc.Refresh()
    if (-not $j.proc.HasExited) { [void]$still.Add($j); continue }
    $j.proc.WaitForExit() | Out-Null
    $bytes = if (Test-Path $j.hp) { [int64](Get-Item $j.hp).Length } else { -1 }
    $label = switch ($j.kind) {
      "base125" { "base125" }
      "stack2" { "o2*sentmem+word*brk" }
      "idle3" { "idle3" }
      default { Spec-Label $j.spec3 }
    }
    $line = "{0},{1},""{2}"",{3},{4},{5}" -f $j.id, $j.kind, $j.spec3, $label, $bytes, ([DateTime]::UtcNow.ToString("o"))
    Add-Content -Encoding utf8 $Csv $line
    if ($bytes -gt 32 -and $j.kind -match 'pair|triple|winner') {
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
  if ($job.kind -ne "base125" -and $job.kind -ne "stack2") {
    [void]$arg.Add("--cross"); [void]$arg.Add($Pin)
    [void]$arg.Add("--cross2"); [void]$arg.Add($Pin2)
    if ($job.spec3) { [void]$arg.Add("--cross3"); [void]$arg.Add($job.spec3) }
  }
  [void]$arg.Add($InLab); [void]$arg.Add($hp)
  $proc = Start-Process -FilePath $job.exe -ArgumentList $arg.ToArray() `
    -WorkingDirectory $Lab -WindowStyle Hidden `
    -RedirectStandardError $err -RedirectStandardOutput $outlog -PassThru
  [void]$running.Add([pscustomobject]@{
    id = $job.id; kind = $job.kind; spec3 = $job.spec3
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
$stack = ($rows | Where-Object { $_.id -eq "stack2" } | Select-Object -First 1)
$base = ($rows | Where-Object { $_.id -eq "base125" } | Select-Object -First 1)
$stackn = if ($stack) { [int64]$stack.bytes } else { 0 }
$basen = if ($base) { [int64]$base.bytes } else { 0 }
$ranked = $rows | ForEach-Object {
  $b = [int64]$_.bytes
  [pscustomobject]@{
    id = $_.id; kind = $_.kind; label = $_.label; bytes = $b
    dStack = $b - $stackn; d125 = $b - $basen
  }
} | Sort-Object bytes
$beat = @($ranked | Where-Object { $_.kind -match 'pair|triple|winner' -and $_.dStack -lt 0 })
$lines = New-Object System.Collections.Generic.List[string]
[void]$lines.Add(("256 KB cross3 pin={0}+{1}  ok={2}  stack2={3}  base125={4}" -f $Pin, $Pin2, $ranked.Count, $stackn, $basen))
[void]$lines.Add(("beat stack2: {0}" -f $beat.Count))
[void]$lines.Add("TOP 40:")
$ranked | Select-Object -First 40 | ForEach-Object {
  [void]$lines.Add(("{0,8}  {1,-8}  {2,-40}  dStack={3,6}  d125={4,6}" -f $_.bytes, $_.kind, $_.label, $_.dStack, $_.d125))
}
$lines | Set-Content -Encoding utf8 $Summary
$lines | ForEach-Object { Write-Host $_ }
Write-Host "DONE csv=$Csv summary=$Summary files_left=$($files.Count)"
