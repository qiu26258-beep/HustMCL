# Mojang 版本元数据样本

`1.21.1.json` 是 2026-10-22（Asia/Taipei）下载的官方原始 UTF-8 字节，未删改、格式化或添加换行；不包含游戏二进制、账号或令牌。

- 发现入口：`https://piston-meta.mojang.com/mc/game/version_manifest_v2.json`
- 固定元数据地址：`https://piston-meta.mojang.com/v1/packages/1757c8b1fbfc6b638504e1f6cceb05893f7389d5/1.21.1.json`
- 清单公布的 SHA-1 与下载实测均为 `1757c8b1fbfc6b638504e1f6cceb05893f7389d5`。
- 大小：38,408 bytes。
- 官方发布说明：[Minecraft Java Edition 1.21.1](https://www.minecraft.net/en-us/article/minecraft-java-edition-1-21-1)。本批以此为解析验证目标，不表示最新版或主菜单兼容性已经验收。

`services.version_metadata` 先验证原始大小与 SHA-1，再解析和生成 Windows x64 启动计划。反例在测试中从该样本复制并修改，不改变此文件。默认测试完全离线；使用 SHA-1 是为了匹配 Mojang 的元数据 / 游戏文件格式，不代表它替代 HTTPS 或来源验证。

`expected-windows-libraries.txt` 是逐项复核的 Windows 依赖顺序：97 项原始依赖按规则选出 70 项，classpath 最后追加客户端 JAR。原清单中 Windows 原生库同时包含 x64 / x86 / ARM64 变体，本批遵循其 OS 规则保留，交由运行库按架构加载，不按文件名自行删减。该期望文件不会在测试运行中生成。

本样本是 Mojang 发布的描述数据，并非本项目原创；本记录提供来源，不把项目未来的代码许可证扩展到该数据或其引用的文件。本批未引入 PCL / HMCL 实现。

## 固定资源索引 17

`17.json` 是上述固定版本 `assetIndex` 引用的原始 UTF-8 字节，2026-10-22 下载后未格式化。固定 URL：`https://piston-meta.mojang.com/v1/packages/e61ea955de95de1f99d36616a8b46147e9f6f5d7/17.json`。大小 449,557 bytes，SHA-1 `e61ea955de95de1f99d36616a8b46147e9f6f5d7`，均与已固定版本文件中的引用一致；`.gitattributes` 同样禁止换行转换。

索引只有 `objects` 字段，3,911 个逻辑名称引用 3,888 个不同的 SHA-1 内容；逻辑总大小 826,115,023 bytes 与版本 `totalSize` 一致，去重后 825,883,021 bytes。按摘要前两位分目录的地址 / 目标约定通过只读查看 Prism 项目的 [AssetsUtils](https://github.com/PrismLauncher/PrismLauncher/blob/develop/launcher/minecraft/AssetsUtils.cpp) 与 [BuildConfig](https://github.com/PrismLauncher/PrismLauncher/blob/develop/buildconfig/BuildConfig.h) 交叉核对；访问日期 2026-10-22，未固定该开发分支提交。只记录互操作事实，没有导入其实现或使用其应用身份。程序仅构造 `https://resources.download.minecraft.net/<前两位>/<SHA-1>` 目标，本批没有下载这些资源对象，不能据此声称其全部可用。

`expected-install-plan.json` 的计数和 SHA-256 在 C++ 实现前，用独立 Python 读取两个原始 JSON 并按已核对的 Windows 库规则计算，测试不会重新生成期望。基础文件按 70 库 / 客户端 / 索引 / 日志顺序，后追加按 SHA-1 字典序排列的 3,888 个资源对象，共 3,961 项、919,909,872 bytes。规范摘要对每项的 `role<TAB>相对路径<TAB>SHA-1<TAB>十进制大小<TAB>URL<LF>` UTF-8 字节计算 SHA-256；角色 0–4 分别为库、客户端、资源索引、日志、资源对象。

固定期望摘要为 `8796c16d9b327dcd276ec30d7faa1a850b3bbe189c9854f58a68f501e9f7be66`。它核对每个地址 / 目标 / 大小及顺序，不只是总数。描述数据仍属 Mojang 来源；没有包含声音、图片、游戏 JAR 或第三方实现代码。
