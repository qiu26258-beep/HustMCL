param(
    [string]$SourcePackage,
    [string]$QtRoot,
    [string]$NoticesSource
)
$ErrorActionPreference = 'Stop'
$workspace = Split-Path $PSScriptRoot -Parent
if (!$SourcePackage) { $SourcePackage = Join-Path $workspace 'out/HustMCL-Offline-26.2' }
if (!$QtRoot) { $QtRoot = Join-Path $workspace '.deps/Qt/6.10.3/mingw_64' }
if (!$NoticesSource) { $NoticesSource = Join-Path $workspace 'out/trial-26.2-icon-staging/HustMCL-Offline-26.2/THIRD-PARTY-NOTICES' }
$source = (Resolve-Path -LiteralPath $SourcePackage).Path
$out = [IO.Path]::GetFullPath((Join-Path $workspace 'out'))
$stage = Join-Path $out 'gui-26.2-staging'
$package = Join-Path $stage 'HustMCL-GUI-26.2'
$archive = Join-Path $out 'HustMCL-GUI-26.2-Windows-x64.zip'
foreach ($target in @($stage,$archive)) {
    if (!( [IO.Path]::GetFullPath($target)).StartsWith($out + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Output escapes workspace out' }
    if (Test-Path -LiteralPath $target) { throw "Output already exists; refusing to overwrite: $target" }
}
$launcher = Join-Path $workspace 'build/desktop-release/src/app/hustmcl_desktop.exe'
foreach ($required in @($launcher,$NoticesSource,(Join-Path $workspace 'tools/create_gui_shortcut.vbs'))) {
    if (!(Test-Path -LiteralPath $required)) { throw "Required input missing: $required" }
}
& (Join-Path $source 'hustmcl_offline.exe') --check-only
if ($LASTEXITCODE -ne 0) { throw 'Source game/runtime verification failed' }
New-Item -ItemType Directory -Path (Join-Path $package 'data') | Out-Null
foreach ($directory in @('assets','libraries','versions')) {
    Copy-Item -LiteralPath (Join-Path $source "data/$directory") -Destination (Join-Path $package 'data') -Recurse
}
Copy-Item -LiteralPath (Join-Path $source 'runtime') -Destination $package -Recurse
Copy-Item -LiteralPath $launcher -Destination $package
Copy-Item -LiteralPath (Join-Path $workspace 'tools/create_gui_shortcut.vbs') -Destination (Join-Path $package '创建桌面快捷方式.vbs')
Copy-Item -LiteralPath (Join-Path $workspace 'resources/brand/hustmcl.ico') -Destination $package
Copy-Item -LiteralPath $NoticesSource -Destination (Join-Path $package 'THIRD-PARTY-NOTICES') -Recurse
$noticesReadme = Join-Path $package 'THIRD-PARTY-NOTICES/README.txt'
(Get-Content -LiteralPath $noticesReadme -Raw).Replace('Qt 6.10.3 QtCore is dynamically linked. QtCore.dll', 'Qt 6.10.3 QtCore, QtGui, QtWidgets and QtNetwork are dynamically linked. Their DLLs') | Set-Content -LiteralPath $noticesReadme -Encoding utf8
& (Join-Path $QtRoot 'bin/windeployqt.exe') --release --compiler-runtime --no-translations --no-opengl-sw (Join-Path $package 'hustmcl_desktop.exe')
if ($LASTEXITCODE -ne 0) { throw 'Qt Widgets deployment failed' }
@'
HustMCL 26.2 图形界面体验包 · Windows 10 / 11 x64

【开始体验】
1. 完整解压 ZIP 到可写目录，不要在压缩软件中直接运行。
2. 双击 hustmcl_desktop.exe，进入 HustMCL 校园主题界面。
3. 点击“启动游戏”，等待本地文件校验和 Java 检测，随后进入 Minecraft。
4. Java 25 和原版 26.2 已随包提供；运行无需再安装 Java。
5. 双击“创建桌面快捷方式.vbs”可创建 HustMCL GUI 快捷方式。
   移动目录后需重新创建；脚本不会覆盖指向其他目录的同名快捷方式。

【界面功能】
首页：真实实例、运行环境、启动与退出状态。
实例与检查：校验/Java/启动进度、取消准备、错误重试。
设置：修改本次打开启动器期间的离线玩家名；固定 2048 MiB 内存上限。
日志与错误：本次状态、错误码、运行日志位置，打开日志/存档文件夹。
默认名 HustPlayer。只支持 1–16 位英文字母、数字或下划线。
改名会改变离线 UUID，影响世界内玩家物品归属；重新打开启动器恢复默认名。
不登录微软账号，不包含正版在线认证/Realms；其他版本下载尚未接入。

【运行与存档】
关闭启动器窗口不会结束正在运行的游戏。游戏退出后隐藏的启动器自动退出。
同一实例运行期间不能再次启动，也不能修改身份。
世界：data\game\saves
游戏日志：data\game\logs\latest.log
启动日志/退出报告：data\launcher-logs
更新或移动时保留自己的 saves；本包不含打包者的存档、账号和运行日志。

【反馈】
请说明 Windows 版本、显卡、内存、解压路径、卡在哪一步、能否进入主菜单/世界。
提供本次 .stdout.log/.stderr.log/.json 有助于定位，发送前检查个人信息。
本机通过实测不代表所有设备已验证。

HustMCL 是独立第三方启动器。Minecraft 文件权利属于 Mojang/Microsoft，
打包不代表取得原版文件再分发授权。第三方许可见 THIRD-PARTY-NOTICES。
'@ | Set-Content -LiteralPath (Join-Path $package '使用说明.txt') -Encoding utf8
& (Join-Path $source 'hustmcl_offline.exe') --data-root (Join-Path $package 'data') --java (Join-Path $package 'runtime/bin/java.exe') --check-only
if ($LASTEXITCODE -ne 0) { throw 'Staged game/runtime verification failed' }
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
