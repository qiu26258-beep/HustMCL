param(
    [string]$SourcePackage,
    [string]$QtRoot,
    [string]$MinGWRoot,
    [switch]$Classic
)
$ErrorActionPreference = 'Stop'
$workspace = Split-Path $PSScriptRoot -Parent
if (!$SourcePackage) { $SourcePackage = Join-Path $workspace 'out/HustMCL-Offline-26.2' }
if (!$QtRoot) { $QtRoot = Join-Path $workspace '.deps/Qt/6.10.3/mingw_64' }
if (!$MinGWRoot) { $MinGWRoot = Join-Path $workspace '.deps/Qt/Tools/mingw1310_64' }
$source = (Resolve-Path -LiteralPath $SourcePackage).Path
$out = [IO.Path]::GetFullPath((Join-Path $workspace 'out'))
$stageName = if ($Classic) { 'trial-26.2-staging' } else { 'trial-26.2-icon-staging' }
$archiveName = if ($Classic) { 'HustMCL-Offline-26.2-Windows-x64.zip' } else { 'HustMCL-Offline-26.2-Icon-Windows-x64.zip' }
$stage = [IO.Path]::GetFullPath((Join-Path $out $stageName))
$archive = [IO.Path]::GetFullPath((Join-Path $out $archiveName))
foreach ($target in @($stage,$archive)) {
    if (!$target.StartsWith($out + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Output escapes workspace out' }
    if (Test-Path -LiteralPath $target) { throw "Output already exists; refusing to overwrite: $target" }
}
$package = Join-Path $stage 'HustMCL-Offline-26.2'
New-Item -ItemType Directory -Path (Join-Path $package 'data') | Out-Null
foreach ($file in @('hustmcl_offline.exe','Qt6Core.dll','libgcc_s_seh-1.dll','libstdc++-6.dll','libwinpthread-1.dll','Start-Offline.cmd')) {
    Copy-Item -LiteralPath (Join-Path $source $file) -Destination $package
}
if (!$Classic) {
    Copy-Item -LiteralPath (Join-Path $workspace 'resources/brand/hustmcl.ico') -Destination $package
    Copy-Item -LiteralPath (Join-Path $workspace 'tools/create_offline_shortcut.vbs') -Destination (Join-Path $package '创建桌面快捷方式.vbs')
}
Copy-Item -LiteralPath (Join-Path $source 'runtime') -Destination $package -Recurse
foreach ($directory in @('assets','libraries','versions')) {
    Copy-Item -LiteralPath (Join-Path $source "data/$directory") -Destination (Join-Path $package 'data') -Recurse
}
$notices = Join-Path $package 'THIRD-PARTY-NOTICES'
New-Item -ItemType Directory -Path $notices | Out-Null
foreach ($component in @('gcc','winpthreads','mingw-w64')) {
    Copy-Item -LiteralPath (Join-Path $MinGWRoot "licenses/$component") -Destination $notices -Recurse
}
Copy-Item -LiteralPath (Join-Path $QtRoot 'sbom/qtbase-6.10.3.spdx.json') -Destination (Join-Path $notices 'qtbase-6.10.3.spdx.json')
foreach ($license in @('LGPL-3.0-only','GPL-3.0-only')) {
    Invoke-WebRequest -Uri "https://raw.githubusercontent.com/qt/qtbase/v6.10.3/LICENSES/$license.txt" -OutFile (Join-Path $notices "$license.txt") -TimeoutSec 60
}
@'
Third-party components

Qt 6.10.3 QtCore is dynamically linked. QtCore.dll can be replaced by a compatible
build. License texts and the installed Qt source/binary component inventory are
included here. Corresponding upstream Qt sources:
https://download.qt.io/archive/qt/6.10/6.10.3/submodules/qtbase-everywhere-src-6.10.3.tar.xz
https://github.com/qt/qtbase/tree/v6.10.3

GCC runtime, MinGW-w64 and winpthreads notices are included in their subfolders.
The Microsoft OpenJDK 25 runtime's original licenses are in runtime/legal/.

Minecraft game software and assets remain Mojang/Microsoft content. Packaging
does not grant a right to redistribute them. Minecraft EULA:
https://www.minecraft.net/en-us/eula

HustMCL is an independent third-party launcher, not an official Minecraft product.
'@ | Set-Content -LiteralPath (Join-Path $notices 'README.txt') -Encoding utf8
@'
HustMCL 原版 26.2 本地体验包 · Windows x64

【开始体验】
1. 将整个 ZIP 解压到可写目录，例如 D:\Games\HustMCL。
2. 不要在压缩软件中直接运行，也不要只拿走 EXE。
3. 双击 Start-Offline.cmd，首次会校验文件，再打开 Minecraft。
4. Java 25 已随包提供，无需另装 Java。

【桌面快捷方式】
带图标版本可双击“创建桌面快捷方式.vbs”，在当前用户桌面创建 HustMCL 图标。
快捷方式会使用本次解压的实际位置；移动整个目录后，请重新创建快捷方式。

默认玩家名：HustPlayer。可用记事本编辑 Start-Offline.cmd 中的 --username。
名字只能使用 1–16 位英文字母、数字或下划线；改名字会改变离线 UUID。
本包为离线单人体验，不登录微软账号，不包含正版在线认证或 Realms 会话。
当前入口为命令行启动器，主题窗口尚未接入；控制台运行期间请保持打开。

【存档与日志】
自己的世界：data\game\saves
游戏日志：data\game\logs\latest.log
启动结果：data\launcher-logs
更新/移动时保留整个目录，尤其不要覆盖或删除自己的 saves。
本包未带打包者的测试世界、账号或运行日志。

【反馈时请说明】
Windows 版本、CPU/内存/显卡、解压路径、卡在哪一步、是否出现游戏窗口、
能否进入单人世界、启动与退出是否正常。若附日志，请先检查其中的个人信息。

仅完成本机 Windows x64 验证，其他机器可能需要单独排查。
Minecraft 原版文件的再分发受官方 EULA 限制，打包不代表取得分发授权。
第三方组件许可与来源见 THIRD-PARTY-NOTICES；包内文件摘要见 FILES.sha256.json。
'@ | Set-Content -LiteralPath (Join-Path $package '使用说明.txt') -Encoding utf8
$previousPath = $env:PATH
try {
    $env:PATH = "$env:SystemRoot/System32;$env:SystemRoot"
    & (Join-Path $package 'hustmcl_offline.exe') --check-only
    if ($LASTEXITCODE -ne 0) { throw 'Staged package failed verification' }
} finally { $env:PATH = $previousPath }
$prefix = $package + [IO.Path]::DirectorySeparatorChar
$manifest = @(Get-ChildItem -LiteralPath $package -Recurse -File | Sort-Object FullName | ForEach-Object {
    [ordered]@{ path=$_.FullName.Substring($prefix.Length).Replace('\','/'); size=$_.Length; sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
})
$manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $package 'FILES.sha256.json') -Encoding utf8
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($package, $archive, [IO.Compression.CompressionLevel]::Optimal, $true)
$sha = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
"$sha  $([IO.Path]::GetFileName($archive))" | Set-Content -LiteralPath ($archive + '.sha256') -Encoding ascii
Get-Item -LiteralPath $archive | Select-Object FullName,Length
Write-Output "SHA256: $sha"
