# HustMCL

面向 Windows 10 / 11 x64 的 Minecraft Java 启动器预览版。

[下载最新版安装器](https://github.com/qiu26258-beep/HustMCL/releases/tag/v0.1.0-preview.2)

安装后打开 HustMCL Accounts，使用微软账号登录并验证 Minecraft Java 版所有权，在首页选择原版版本安装，或导入支持的 Modrinth 格式 ZIP/MRPACK 整合包。游戏与匹配的 Java 按需下载，安装后需手动启动。支持现代原版和 Fabric；Forge、NeoForge、Quilt 与其他整合包格式暂不支持。

安装包不包含 Minecraft、Java、账号数据或私人素材。预览版尚未完成干净 Windows 10/11 的全面兼容性验证。HustMCL 不是 Minecraft 官方产品。

## 构建

使用 Qt 6.10.3、MinGW 13.1 x64、CMake、Ninja 和 Python 3.10+：

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DHUSTMCL_BUILD_DESKTOP=ON -DHUSTMCL_PUBLIC_RELEASE=ON -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH=C:/Qt/6.10.3/mingw_64
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

必须启用公开版构建选项。项目源码尚未选定通用许可证；第三方许可随安装包提供。
