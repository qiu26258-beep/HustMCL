# HustMCL

华中科技大学主题、面向新手的 Minecraft Java Edition 启动器。

当前处于 M0：已有可编译的 C++20 核心库、Java 运行时选择规则和离线 CTest；另有一个可运行的 Qt Widgets **界面演示**，包含原生首页与安装页。演示使用显式样例数据，尚未接入真实微软登录、下载、安装或游戏启动，不是完整启动器，也没有发布包或性能实测结果。目标平台为 Windows 10 / Windows 11 x64；桌面实现路线为 C++20 + Qt 6 Widgets + CMake，发行工具链仍以 MSVC x64 为目标。

首版目标：具备游戏授权的微软账号用户，在没有 Java 和 Minecraft 的环境下完成原版安装并进入主菜单，再次打开时可直接启动已安装实例。

## 项目资料

| 文件 | 用途 |
| --- | --- |
| [MVP 项目规划](docs/MVP项目规划.md) | 产品范围、技术选型、性能目标、里程碑与待决策事项 |
| [架构设计](docs/架构设计.md) | PCL / HMCL 参考依据、依赖方向、任务与数据约定、M0 实施顺序 |
| [构建与测试](docs/构建与测试.md) | 核心与 Qt 桌面构建命令、已验证工具链、测试边界 |
| [前端界面方案](docs/frontend/界面方案.md) | Campus Desk 页面结构、主题、状态映射与应用接口 |
| [前端验证记录](docs/frontend/验证记录.md) | 设计、原生 UI、截图和测试的实际结果 |
| [验证记录](docs/验证记录.md) | 本仓库较早的核心与规划验证记录 |
| [Agent 硬规定](agent.md) | 每批改动前验证、完成后验证并立即提交的工作流程 |

维护优先级：可维护性 → 安装与启动可靠性 → 功能广度 → 视觉装饰。新增功能先找到所属模块和验收场景，再实现页面。框架参考 PCL 与 HMCL，具体取舍见架构设计；首版继续聚焦原版安装与启动闭环。

## 核心构建

使用环境中的 x64 C++20 编译器、Ninja、CMake ≥3.24 与 Python ≥3.10，可运行：

```powershell
cmake --preset core-debug
cmake --build --preset core-debug
ctest --preset core-debug
```

这是核心库回归，不依赖 Qt，不能代替桌面界面、真实认证、游戏启动与干净系统验收。

## Qt 桌面演示

桌面构建默认关闭；只有显式开启时才查找 Qt 6.10 Widgets。本机使用 Qt 6.10.3 MinGW 组件与配套 GCC 13.1，命令如下：

```powershell
$env:PATH = "D:\HustMCL\.deps\Qt\Tools\mingw1310_64\bin;D:\HustMCL\.deps\Qt\6.10.3\mingw_64\bin;$env:PATH"
cmake -S . -B build/desktop-debug -G Ninja `
  -DHUSTMCL_BUILD_DESKTOP=ON -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_CXX_COMPILER=D:/HustMCL/.deps/Qt/Tools/mingw1310_64/bin/g++.exe `
  -DCMAKE_PREFIX_PATH=D:/HustMCL/.deps/Qt/6.10.3/mingw_64
cmake --build build/desktop-debug
ctest --test-dir build/desktop-debug --output-on-failure
```

可执行文件：

```text
build/desktop-debug/src/app/hustmcl_desktop.exe
```

截图与界面测试。启动器是 Windows GUI 子系统，避免双击时出现控制台；命令行截图请用 `Start-Process -Wait` 或 `cmd /c` 等待进程，这样仍能重定向输出并取得非零退出码：

```powershell
$env:PATH = "D:\HustMCL\.deps\Qt\Tools\mingw1310_64\bin;D:\HustMCL\.deps\Qt\6.10.3\mingw_64\bin;$env:PATH"
$env:QT_QPA_PLATFORM = "offscreen"
$exe = "build/desktop-debug/src/app/hustmcl_desktop.exe"
$p = Start-Process -FilePath $exe -Wait -PassThru `
  -ArgumentList @("--scenario","install-unknown","--page","install","--scale","1",
                  "--window-size","1120x720",
                  "--screenshot","out/native-ui-review/install-unknown-1x.png") `
  -RedirectStandardOutput "out/native-ui-review/capture.stdout.txt" `
  -RedirectStandardError "out/native-ui-review/capture.stderr.txt"
$p.ExitCode
```

可用场景：`home-ready`、`install-unknown`、`install-known`、`install-cancelling`、`install-cancelled`、`install-committing`、`install-failed`、`install-long-path`。`--native-screenshot` 通过 Windows QPA 抓取已显示窗口；`QT_QPA_PLATFORM=offscreen` 可做可重复的离屏截图。`--scale` 是 Qt 缩放提示，不是跨平台绝对 DPI；Windows 原生平台仍按系统缩放（本机 150%），离屏基础 1x / 1.5x / 2x 单独记录。

## 演示边界

- 首页主操作与安装页导航已接入演示；设置与日志页暂未实现，侧栏禁用并给出说明。
- 安装页展示未知 / 已知总量、取消请求、取消确认、提交不可取消、失败重试和长路径；这些状态由 `src/app` 的演示控制器驱动，不是真实下载器。
- 真实微软认证、版本目录、Java 获取、下载校验、游戏启动、发布包和干净系统验收仍未实现。
- MSVC 安装尚未成功；本轮 Qt 构建使用 MinGW 13.1 + Qt 6.10.3。部署与 `windeployqt` 由后续集成批次完成。

全部前端工作交由 DSH 中的 DeepSeek 完成，Codex 负责核心、构建集成与验收。本项目许可证尚待确定；仓库中的参考链接不表示已经引入第三方源码或资源。华小科角色衍生图片的授权边界见[素材与背景设计](docs/frontend/华小科素材与背景设计.md)，项目代码许可不自动覆盖该图片。
