param(
    [Parameter(Mandatory=$true)][string]$VerifiedGameRoot,
    [Parameter(Mandatory=$true)][string]$JavaRoot,
    [string]$QtRoot = 'D:/HustMCL/.deps/Qt/6.10.3/mingw_64'
)
$ErrorActionPreference = 'Stop'
$workspace = Split-Path $PSScriptRoot -Parent
$package = [IO.Path]::GetFullPath((Join-Path $workspace 'out/HustMCL-Offline-26.2'))
$outputRoot = [IO.Path]::GetFullPath((Join-Path $workspace 'out')) + [IO.Path]::DirectorySeparatorChar
if (!$package.StartsWith($outputRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Package must stay under workspace out' }
if (Test-Path -LiteralPath $package) { throw 'Package already exists; refusing to overwrite files or saves' }
$launcher = Join-Path $workspace 'build/backend-release/src/app/hustmcl_offline.exe'
$sourceRoot = (Resolve-Path -LiteralPath $VerifiedGameRoot).Path
$runtimeRoot = (Resolve-Path -LiteralPath $JavaRoot).Path
& $launcher --data-root $sourceRoot --java (Join-Path $runtimeRoot 'bin/java.exe') --check-only
if ($LASTEXITCODE -ne 0) { throw 'Source validation failed' }
New-Item -ItemType Directory -Path (Join-Path $package 'data') | Out-Null
foreach ($name in @('assets','libraries','versions')) {
    Copy-Item -LiteralPath (Join-Path $sourceRoot $name) -Destination (Join-Path $package 'data') -Recurse
}
Copy-Item -LiteralPath $runtimeRoot -Destination (Join-Path $package 'runtime') -Recurse
Copy-Item -LiteralPath $launcher -Destination $package
& (Join-Path $QtRoot 'bin/windeployqt.exe') --release --compiler-runtime --no-translations --no-opengl-sw (Join-Path $package 'hustmcl_offline.exe')
if ($LASTEXITCODE -ne 0) { throw 'Qt deployment failed' }
@'
@echo off
cd /d "%~dp0"
"%~dp0hustmcl_offline.exe" --username HustPlayer
if errorlevel 1 pause
'@ | Set-Content -LiteralPath (Join-Path $package 'Start-Offline.cmd') -Encoding ascii
@'
HustMCL Offline 26.2

Double-click Start-Offline.cmd to play single-player as HustPlayer.
Java 25 and the required libraries are included. Keep this whole folder together.
Your worlds live in data/game/saves. Do not delete this directory when updating.

To choose a different offline name, edit HustPlayer in Start-Offline.cmd.
Names use 1-16 letters, digits or underscores. A different name has a different
offline UUID and therefore different player inventory ownership in a world.

This local package does not sign in to a Microsoft account. It is not an
authenticated multiplayer/Realms session. The existing PCL files are not used
at launch. Full transactional installation and the Qt frontend are separate work.

Diagnostics: data/launcher-logs and data/game/logs.
'@ | Set-Content -LiteralPath (Join-Path $package 'README.txt') -Encoding utf8
$previousPath = $env:PATH
try {
    $env:PATH = "$env:SystemRoot/System32;$env:SystemRoot"
    & (Join-Path $package 'hustmcl_offline.exe') --check-only
    if ($LASTEXITCODE -ne 0) { throw 'Deployed package validation failed' }
} finally { $env:PATH = $previousPath }
Write-Output $package
