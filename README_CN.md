# IP Assistant

简体中文 | [English](README.md)

IP Assistant 是一个面向 Windows 10/11 的 Qt 6 网络配置切换工具。它可以为同一网卡保存多组 IPv4、IPv6 和 DNS 配置，并通过主窗口或系统托盘快速切换。

当前版本：**1.0.0**

许可证：[MIT](LICENSE)

隐私与安全：

- [隐私政策](PRIVACY.md)
- [安全政策与漏洞报告](SECURITY.md)
- [代码签名政策](.signpath/README.md)

## 主要功能

- 为不同网卡创建多个网络配置。
- 支持 IPv4 DHCP、静态 IP、子网掩码、默认网关和 DNS。
- 支持 IPv6 自动配置、静态地址、前缀长度、默认网关和 DNS。
- 配置使用网卡 GUID 绑定，网卡改名后仍可正常激活。
- GUID 失效时按网卡名称兜底；两者均不可用时保留配置并显示失效提示。
- 切换前自动保存当前网络状态；任一步骤失败时尝试恢复原配置，并返回真实错误。
- 支持 Windows 开机启动、系统托盘和 18 种界面语言。

## 支持的语言

应用内置以下语言，并会在首次启动时按 Windows 区域设置自动选择：

| 语言 | Locale |
| --- | --- |
| English | `en_US` |
| 简体中文 | `zh_CN` |
| 繁體中文 | `zh_TW` |
| 日本語 | `ja_JP` |
| 한국어 | `ko_KR` |
| Deutsch | `de_DE` |
| Français | `fr_FR` |
| Español | `es_ES` |
| Português (Brasil) | `pt_BR` |
| Русский | `ru_RU` |
| Italiano | `it_IT` |
| Türkçe | `tr_TR` |
| Polski | `pl_PL` |
| Bahasa Indonesia | `id_ID` |
| Tiếng Việt | `vi_VN` |
| ไทย | `th_TH` |
| العربية | `ar_SA` |
| हिन्दी | `hi_IN` |

语言可在“设置”中即时切换；阿拉伯语会自动使用从右到左布局。翻译源文件位于 `src/translations`，构建时由 Qt Linguist 自动编译并嵌入主程序。

## 工作方式

程序由两个进程组成：

- `IPAssistant.exe`：普通用户权限运行的图形界面。
- `IPAssistantService.exe`：以 Windows 服务 `IPAssistantSvc` 运行，负责执行需要管理员权限的网络配置。

首次使用或安装时需要管理员权限安装服务，之后切换 Profile 不需要反复弹出 UAC。

IPv4 地址和 DNS 通过 Windows `netsh interface` 配置；IPv6 手工地址和路由通过 IP Helper API 配置。服务在修改前创建当前网卡快照，并在失败时执行回滚。

## Profile 与网卡绑定

Profile 同时保存：

- `deviceGuid`：稳定的实际绑定标识。
- `deviceName`：当前显示名称和兼容旧数据的兜底标识。

旧版 `profiles.json` 会在启动时自动升级为版本 2。升级前会在同一目录创建一次 `profiles.json.bak`，原有 Profile 不会被删除。

如果卸载网卡驱动后重新安装，Windows 可能生成新的 GUID。软件会尝试按原名称重新绑定；如果名称也发生变化，需要编辑 Profile 并重新选择网卡。

## 数据位置

Profile 保存在程序目录：

```text
data/profiles.json
data/profiles.json.bak
```

界面设置使用 `QSettings` 保存到当前用户注册表。卸载程序默认保留 `data` 目录，便于升级或重新安装后继续使用。

## 构建

依赖：

- Windows 10/11 x64
- Qt 6.5 或更高版本（Core、Gui、Widgets、Network、Test）
- CMake 3.16 或更高版本
- MinGW 13.x 或兼容的 MSVC 工具链

示例：

```powershell
cmake -S . -B build `
  -G "MinGW Makefiles" `
  -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\mingw_64" `
  -DCMAKE_BUILD_TYPE=Release `
  -DBUILD_TESTING=ON

cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

构建会自动调用 Qt Linguist 的 `lrelease`，将 17 个非英文 `.ts` 文件编译为 `.qm` 并嵌入 `IPAssistant.exe`。更新源代码中的可翻译文本后，可先运行：

```powershell
& "C:\Qt\6.9.1\mingw_64\bin\lupdate.exe" `
  src `
  -no-obsolete `
  -locations none `
  -ts src\translations\ipassistant_zh_CN.ts
```

主要输出：

```text
build/IPAssistant.exe
build/IPAssistantService.exe
build/ProfileBindingTests.exe
```

发布前使用 `windeployqt` 将 Qt DLL 和插件部署到 `build`：

```powershell
& "C:\Qt\6.9.1\mingw_64\bin\windeployqt.exe" `
  --release `
  --compiler-runtime `
  build\IPAssistant.exe
```

## 生成安装包

安装器使用 Inno Setup 6：

```powershell
ISCC.exe installer\IPAssistant.iss
```

输出：

```text
installer/Output/IPAssistantSetup-1.0.0.exe
```

安装程序会：

1. 停止旧版 `IPAssistantSvc`。
2. 更新主程序、服务和 Qt 运行库。
3. 删除旧服务注册并安装、启动新服务。
4. 根据用户选择创建桌面快捷方式。

卸载时会先关闭主程序，再停止并删除服务。

安装器提供英文、简体中文、日语、韩语、德语、法语、西班牙语、巴西葡萄牙语、俄语、意大利语、土耳其语、波兰语、泰语和阿拉伯语向导。繁体中文、印尼语、越南语和印地语系统上安装向导回退英文，安装后的应用界面仍使用对应语言。

## 项目结构

```text
.
├── CMakeLists.txt
├── IPAssistant.pro          # 可选的 GUI qmake 工程
├── LICENSE                  # MIT 许可证
├── PRIVACY.md               # 隐私政策
├── SECURITY.md              # 安全报告政策
├── README.md                # English README
├── README_CN.md             # 简体中文 README
├── .github/workflows/       # GitHub Release 自动构建
├── .signpath/               # SignPath 签名策略与制品约束
├── installer/               # Inno Setup 安装脚本
├── src/
│   ├── core/                # Profile、持久化、网卡解析和核心调用
│   ├── service/             # Windows 服务、网络配置与回滚
│   ├── ui/                  # 主窗口、配置编辑、设置和关于窗口
│   ├── translations/        # 17 个非英文 Qt Linguist 翻译源文件
│   └── resources/           # 图标和 Windows 资源
└── tests/                   # GUID绑定和兼容性测试
```

## 注意事项

- 切换网络配置可能短暂中断当前连接。
- 静态 IP、网关和 DNS 必须符合目标网络环境。
- 如果回滚也失败，程序会明确显示回滚失败信息；此时应通过 Windows 网络设置检查网卡状态。

## License

Copyright © 2026 Jiugang He.

IP Assistant is licensed under the [MIT License](LICENSE).
