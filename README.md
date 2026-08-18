# 麦块启动器 MKL (MaiKuai Launcher)

> 版本：0.0.1-alpha（首个预览版）
> 全平台 Minecraft Java 版启动器：Windows / macOS / Linux / Android / iOS
> 功能对标 PCL2 / HMCL / FCL，并增加全平台插件系统与移动端完整触控方案。

完整开发规格见 [docs/MKL_开发规格书_修正版.md](docs/MKL_开发规格书_修正版.md)。

## 当前里程碑（M1 骨架）

- ✅ 项目骨架：CMake + 分层目录（core / platform / data / service / plugin / ui / entry）
- ✅ 基础层 5 个模块（Qt 无关，可独立编译测试）：Logger / PlatformDetector / FileUtils / StringUtils / JsonUtils
- ✅ 单元测试框架（零依赖迷你断言）与 CTest 接入
- ✅ Qt Quick 空白窗口应用（`mkl_app`），支持 `--smoke-test` 无头冒烟测试
- ✅ GitHub Actions：三平台（Ubuntu / Windows / macOS）构建 + 自动化测试 + Release 打包

## 目录结构

```
src/
├── core/        # 基础层（日志、平台检测、文件、字符串、JSON）
├── platform/    # 平台抽象适配器（进程/凭证/系统UI，M2 起实现）
├── data/        # 数据层（账户、配置，M2 起实现）
├── service/     # 服务层（下载、启动、版本等，M2 起实现）
├── plugin/      # 插件系统（M3 起实现）
├── ui/          # UI 层 QML 组件与控制器
└── entry/       # 入口层（main.cpp）
tests/           # 单元测试
.github/workflows/  # CI / Release
```

## 本地构建

需要 Qt 6.8 + CMake ≥ 3.21 + 支持 C++17 的编译器。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMKL_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build --output-on-failure     # 运行全部测试
./build/mkl_app                                # 启动空白窗口
QT_QPA_PLATFORM=offscreen ./build/mkl_app --smoke-test   # 无头冒烟测试
```

> 本地机器内存不足（2GB）时，请在 GitHub Actions 上构建（见下），产物可从 Release/Artifacts 下载。

## GitHub Actions

| 工作流 | 触发 | 作用 |
| --- | --- | --- |
| `ci.yml` | push / PR / 手动 | Ubuntu + Windows + macOS 三平台构建、单元测试 + GUI 冒烟测试、上传构建产物 |
| `release.yml` | 推送 `v*` 标签 | 三平台打包（Windows zip / macOS dmg / Linux tar.gz）并发布 Release |

## License

保留（待定）。
