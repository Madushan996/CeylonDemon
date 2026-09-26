# SPDX-License-Identifier: GPL-3.0-or-later
# CeylonDemon — Copyright (C) 2026 Madushan Dissanayake.
#
# Release build. Links the Resonance network into the executable, so the
# result is a single self-contained file.
#
#   .\build.ps1                     x86-64-avx2 (portable release default)
#   .\build.ps1 -Arch x86-64-bmi2   Skylake and later, fast PEXT
#   .\build.ps1 -Arch native        tuned to this machine

param(
    [ValidateSet('x86-64-avx2', 'x86-64-bmi2', 'native')]
    [string]$Arch = 'x86-64-avx2',

    [switch]$Tune,

    [string]$EvalFile = '',

    [string]$Output = ''
)

$ErrorActionPreference = 'Stop'

$workspace = Split-Path -Parent $MyInvocation.MyCommand.Path
$toolchain = 'C:\msys64\ucrt64\bin'
$compiler  = Join-Path $toolchain 'g++.exe'
$windres   = Join-Path $toolchain 'windres.exe'

if (-not (Test-Path -LiteralPath $compiler)) { throw "Compiler not found: $compiler" }

$env:PATH = "$toolchain;$env:PATH"

$net = if ($EvalFile) {
    [IO.Path]::GetFullPath((Join-Path $workspace $EvalFile))
} else {
    Join-Path $workspace 'networks\ceylondemon-3.1-resonance-v12-400m-full-scratch-e8.aa'
}
if (-not (Test-Path -LiteralPath $net)) { throw "Network not found: $net" }

$distDir = Join-Path $workspace 'dist'
New-Item -ItemType Directory -Force -Path $distDir | Out-Null
$outputPath = if ($Output) {
    [IO.Path]::GetFullPath((Join-Path $workspace $Output))
} else {
    Join-Path $distDir "CeylonDemon-3.3-$Arch.exe"
}
if ($Tune -and -not $Output) {
    $outputPath = Join-Path $distDir "CeylonDemon-3.3-$Arch-tune.exe"
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $outputPath) | Out-Null

switch ($Arch) {
    'native'       { $archFlags = @('-march=native') }
    'x86-64-bmi2'  { $archFlags = @('-m64','-mpopcnt','-msse','-msse2','-mssse3','-msse4.1','-mavx2','-mbmi','-mbmi2') }
    default        { $archFlags = @('-m64','-mpopcnt','-msse','-msse2','-mssse3','-msse4.1','-mavx2') }
}

# Optional Windows version resource; skipped if windres is unavailable.
$resourceObject = $null
if (Test-Path -LiteralPath $windres) {
    $resourceObject = Join-Path $workspace 'resources.o'
    & $windres (Join-Path $workspace 'src\resources.rc') -O coff -o $resourceObject
    if ($LASTEXITCODE -ne 0) { throw 'Windows resource compilation failed.' }
}

try {
    $args = @(
        '-std=c++20','-O3','-flto','-static','-pthread','-DNDEBUG',
        '-Wall','-Wextra','-Wshadow'
    ) + $archFlags
    if ($Tune) { $args += '-DCEYLON_TUNE' }
    $args += @(
        "-DCEYLON_EMBEDDED_NET=$([IO.Path]::GetFileName($net))",
        "-Wa,-I$([IO.Path]::GetDirectoryName($net))",
        '-o', $outputPath,
        (Join-Path $workspace 'src\main.cpp')
    )
    if ($resourceObject) { $args += $resourceObject }

    & $compiler @args
    if ($LASTEXITCODE -ne 0) { throw 'C++ compilation failed.' }
}
finally {
    if ($resourceObject) {
        Remove-Item -LiteralPath $resourceObject -Force -ErrorAction SilentlyContinue
    }
}

$size = [math]::Round((Get-Item $outputPath).Length / 1MB, 1)
Write-Host "Build complete: $outputPath ($size MB, network embedded)"
