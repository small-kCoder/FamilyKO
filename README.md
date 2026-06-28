# 电脑回溯器（家长管控版）— C 语言版本

合法、透明、对孩子电脑使用进行监督管理的桌面软件，纯 C 语言实现，无 Python 运行时依赖，
可打包为 MSI 企业级安装包。

## 1. 项目结构

```
pc-retro/
├── CMakeLists.txt              # 构建系统
├── build.bat                   # Windows 一键构建脚本
├── README.md                   # 本文件
├── DISCLAIMER.txt              # 免责声明（位于 installer/）
├── installer/
│   ├── DISCLAIMER.txt          # MSI 内嵌的免责声明
│   └── wix/
│       ├── Product.wxs         # WiX MSI 描述
│       └── UI.wxs              # 自定义 UI（强制接受声明）
├── src/
│   ├── common.c/.h             # 公共：路径、时间、日志
│   ├── main.c                  # 程序入口
│   ├── storage/                # SQLite 数据层
│   ├── collectors/             # 行为采集（应用/网页/文件）
│   ├── query/                  # 查询与时间范围
│   ├── ai/                     # Ollama AI 总结
│   ├── control/                # 黑名单、限时执行
│   ├── security/               # 密码、Hotkey、锁定
│   ├── guard/                  # ACL、开机自启、服务
│   ├── ui/                     # Win32 界面
│   └── installer/              # 安装器、卸载器
└── tests/                      # 单元测试
```

## 2. 功能清单

| 模块 | 功能 |
| --- | --- |
| 采集 | 前台应用使用时长、Chrome/Edge/Firefox 浏览记录、桌面/文档/下载/图片 文件变动 |
| 查询 | 事件流、关键字搜索、按应用聚合、范围选择（今天/昨天/昨晚/本周/近 7 天） |
| AI | 调用本地 Ollama 生成中文家长视角总结；Ollama 不可用时降级为纯统计 |
| 控制 | 应用黑名单（强制结束）、每日时长限制、星期+时间窗口限制 |
| 保护 | 进程 ACL 禁止普通用户结束、Watcher 服务自动重启 UI、开机自启 |
| 安全 | PBKDF2-SHA256 密码（10 万次迭代）、登录失败锁定、全局热键 Ctrl+Alt+P 唤出 |
| 透明 | 每次启动弹"会话记录通知"对话框 |
| MSI | WiX 3.x 打包，企业部署；强制接受 DISCLAIMER 才能下一步 |

## 3. 构建要求

- Windows 10/11 或 Windows Server 2016+
- Visual Studio 2019/2022（含 C 编译器 + Windows SDK）
- CMake 3.20+：<https://cmake.org/download/>
- SQLite3：CMake 会自动从 vcpkg / 系统查找
- （可选）WiX 3.14+：<https://wixtoolset.org/releases/> 用于 MSI 打包
- （可选）Ollama：<https://ollama.com/> 用于 AI 总结

## 4. 快速构建

打开「x64 Native Tools Command Prompt for VS 2022」（或 PowerShell）执行：

```bat
cd pc-retro
build.bat
```

成功执行后：

- 可执行文件位于 `dist/`：
  - `PCRetroUI.exe` — 主程序（家长控制台）
  - `PCRetroWatcher.exe` — 后台守护服务
  - `PCRetroInstaller.exe` — 控制台安装器
  - `PCRetroUninstaller.exe` — 控制台卸载器
  - `DISCLAIMER.txt` — 免责声明
- MSI 安装包位于 `dist/PCRetro-1.0.0.msi`（需先安装 WiX 3.x）

## 5. 手动构建

```bat
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release --parallel
ctest -C Release
```

## 6. 安装

双击 `dist/PCRetro-1.0.0.msi`，按向导操作：

1. 阅读并勾选"我已阅读并同意上述声明"（不勾选无法继续）
2. 选择安装目录（默认 `C:\Program Files\PCRetro`）
3. 点击「安装」开始
4. 安装完成后自动启动 UI 首次运行向导

也可手动安装（无需 MSI）：

```bat
cd dist
PCRetroInstaller.exe
```

## 7. 卸载

- 控制面板 → 程序和功能 → 电脑回溯器 → 卸载
- 或在安装目录运行 `PCRetroUninstaller.exe /keep-data`（保留数据库）

## 8. 默认账户

- 默认家长密码：`admin1234`（首次启动后请通过「设置 → 修改密码」修改为强密码）
- 全局热键：`Ctrl+Alt+P` 唤出主窗口

## 9. 数据存储

- 数据库：`%ProgramData%\PCRetro\db.sqlite`（WAL 模式）
- 日志：`%ProgramData%\PCRetro\pcretro.log`

## 10. 法律与合规

- 本软件仅供家长（或法定监护人）对未成年被监护人进行合法监督使用
- 安装、使用前请阅读 `installer/DISCLAIMER.txt`
- 每次启动会自动弹出"会话记录通知"告知当前会话会被记录

## 11. 已知限制

- AI 总结需要本地 Ollama 运行；未运行时自动降级为统计输出
- Windows ACL 保护需要管理员安装；非管理员只能降级为热键+密码
- 应用黑名单通过进程名匹配，绕过方式较多（重命名/便携版），适合低龄儿童场景
