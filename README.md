# 麦块启动器 MKL (MaiKuai Launcher)

> 版本：0.0.2-alpha（**R2 架构**）
> 全平台 Minecraft Java 版启动器：Windows / macOS / Linux / Android / iOS
> 功能对标 PCL2 / HMCL / FCL，并增加全平台插件系统与移动端完整触控方案。

完整开发规格见 [docs/MKL_开发规格书_R2.md](docs/MKL_开发规格书_R2.md)（当前唯一依据）。

## R2 架构（用户决策）

| 层 | 语言 | 说明 |
| --- | --- | --- |
| 核心引擎（基础/数据/服务层） | **主要 C（C11）** | mkl_* 前缀 C 接口，单一 C ABI |
| 增强模块（下载/认证/自更新/插件沙箱） | **中量 Rust** | extern "C" FFI 导出，不 panic 越界 |
| 平台热路径 | **少量汇编** | SHA 族 SIMD（platform/asm/，含 C fallback，后续里程碑） |
| 胶水层 | **C++（仅绑定）** | cpp/mkl_bridge.h，零业务逻辑 |
| UI | **5 套平台原生** | Win32 / AppKit / GTK3 / Kotlin / Swift（壳里程碑 R2-S4 起） |

## 当前状态（R2-S1：核心骨架）

- ✅ C 基础层 5 模块：logger / platform_detector / file_utils / string_utils / json（含 SHA-1）
- ✅ C 单元测试（CTest）+ CLI 冒烟自检（`mkl_app --smoke-test`）
- ✅ Rust 占位模块（mkl_core_rs：版本解析 + Java 需求映射）+ cargo test
- ✅ CI 双工具链：C（三平台 cmake+ctest）+ Rust（三平台 cargo test）

## 目录结构

```
src/
├── core/        # C 基础层
├── data/        # C 数据层（R2-S2）
├── service/     # C 服务层（R2-S3）
├── rust/        # Rust 模块（下载/认证/自更新）
├── platform/    # C 平台垫片（进程/凭证/系统UI）
│   └── asm/     # 少量汇编（SHA SIMD）
├── cpp/         # C++ 胶水层（mkl_bridge.h）
├── ui/          # 5 套原生 UI 壳
│   ├── windows/ ├── macos/ ├── linux/ ├── android/ └── ios/
└── entry/       # 入口（当前为 C CLI）
tests/           # C 测试（test_*.c）
```

## 本地构建

需要 CMake ≥ 3.21 + C11 编译器（gcc/clang/MSVC）与 Rust（可选）。

```bash
# C 核心
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build --output-on-failure     # 运行全部 C 测试
./build/bin/mkl_app --smoke-test               # CLI 冒烟自检

# Rust 模块
cd src/rust/mkl_core_rs && cargo test
```

> 本地 2GB 内存机器构建负担重：所有构建/测试/发布走 GitHub Actions（云端），产物从 Artifacts/Release 下载。

## GitHub Actions

| 工作流 | 触发 | 作用 |
| --- | --- | --- |
| `ci.yml` | push / PR / 手动 | 三平台 C 构建+测试（含冒烟）+ 三平台 Rust 测试 |
| `release.yml` | 推送 `v*` 标签 | 核心验证 + Release notes（图形产物随 UI 壳里程碑加入） |

## License

保留（待定）。
