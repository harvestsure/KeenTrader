# KeenTrader（中文说明）

## 状态

[![windows](https://github.com/harvestsure/KeenTrader/actions/workflows/windows.yml/badge.svg)](https://github.com/harvestsure/KeenTrader/actions/workflows/windows.yml)
[![ubuntu](https://github.com/harvestsure/KeenTrader/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/harvestsure/KeenTrader/actions/workflows/ubuntu.yml)
[![macos](https://github.com/harvestsure/KeenTrader/actions/workflows/macos.yml/badge.svg)](https://github.com/harvestsure/KeenTrader/actions/workflows/macos.yml)

## 项目简介

`KeenTrader` 是一个以 C++ 编写的轻量级、高性能量化交易与接入引擎框架。它为行情获取、委托管理和策略执行提供了统一且可扩展的基础设施，便于在多交易所环境下开发与运行自动化交易策略。

## 主要功能

- 统一的 `Exchange` 接口，便于接入不同交易所
- HTTP 与 WebSocket 客户端，基于事件循环的异步处理
- CTA（跟踪止损/开平仓）策略框架与示例策略
- 日志系统、时间/序列工具和常用辅助库

## 当前支持的交易所

- Binance（线性合约） — 实现在 `core/exchange/binance_linear_exchange.*`
- OKX — 实现在 `core/exchange/okx_exchange.*`

更多实现可在目录 [core/exchange](core/exchange) 中查看。

## 计划支持的交易所

后续计划按优先级与贡献者安排扩展支持，例如 Bybit、Huobi、Coinbase 等主流交易所。

## 跨平台构建说明

以下为常见平台的快速构建步骤，假设在仓库根目录运行命令。

通用步骤：

```
git clone --recursive https://github.com/harvestsure/KeenTrader.git
cd KeenTrader
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

macOS（使用 Homebrew）：

```
brew install cmake openssl
mkdir build && cd build
cmake .. -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
cmake --build .
```

Ubuntu / Debian：

```
sudo apt update
sudo apt install -y build-essential cmake libssl-dev git
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Windows（Visual Studio + vcpkg）：

1. 安装带有 C++ 开发工具的 Visual Studio。
2. 使用 `vcpkg` 安装依赖（例如 OpenSSL），并在 CMake 中指定 vcpkg 工具链文件。

示例（PowerShell）：

```
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.bat
./vcpkg/vcpkg install openssl:x64-windows
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

注意：若依赖安装在自定义路径，请使用 `-DOPENSSL_ROOT_DIR` 等 CMake 变量指定路径。依赖声明请参考 `vcpkg.json` 与 `CMakeLists.txt`。

## 运行示例

构建完成后，可在 `build/bin` 或 `bin` 目录下找到可执行文件（例如 `trader`），直接运行：

```
./bin/trader
```

具体运行参数与配置请查看 `app/` 下示例与策略实现。

## 代码结构（简要）

- `core/`：核心库（网络、事件循环、时间、日志等）
- `engine/`：交易引擎、交易对象与配置
- `app/`：示例应用与 CTA 策略模版
- `core/exchange/`：交易所适配器实现

## 贡献指南

- 欢迎通过 PR 贡献代码：新增交易所、优化策略框架或完善文档。
- 请在 PR 中附上简要说明与可复现测试步骤。

## 免责声明（风险自负）

本项目仅为交易接入与策略执行的技术框架，不构成任何投资建议。真实交易存在资金损失风险，使用者需自行承担全部风险。请在真实资金交易前充分在沙箱环境或小额资金下验证策略与接入正确性。

