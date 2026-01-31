# KeenTrader

## Description

KeenTrader is a lightweight, high-performance quantitative trading and connectivity engine framework written in C++. It provides a unified foundation for market data, order management, and strategy execution. The project uses CMake for cross-platform builds.

## Key Features

- Unified Exchange interface for multiple venue integrations
- HTTP and WebSocket clients with an event loop
- Strategy framework (CTA) with example strategies
- Logging, time utilities, and helper libraries

## Supported Exchanges

- Binance (linear/perpetual contracts) — implementation at `core/exchange/binance_linear_exchange.*`
- OKX — implementation at `core/exchange/okx_exchange.*`

See implementations in the folder [core/exchange](core/exchange).

## Planned Exchanges

Planned future integrations include Bybit, Huobi, Coinbase and other major exchanges; priority depends on contributors and demand.

## Cross-platform Build Instructions

The following are quick build steps from the repository root.

General

```
git clone --recursive https://github.com/harvestsure/KeenTrader.git
cd KeenTrader
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

macOS (Homebrew)

```
brew install cmake openssl
mkdir build && cd build
cmake .. -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
cmake --build .
```

Ubuntu / Debian

```
sudo apt update
sudo apt install -y build-essential cmake libssl-dev git
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Windows (Visual Studio + vcpkg)

1. Install Visual Studio with C++ development tools.
2. Use `vcpkg` to install dependencies (for example OpenSSL) and point CMake to the vcpkg toolchain.

Example (PowerShell):

```
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.bat
./vcpkg/vcpkg install openssl:x64-windows
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

Notes

- If libraries are installed in custom locations, set CMake variables such as `-DOPENSSL_ROOT_DIR` accordingly. See `vcpkg.json` and `CMakeLists.txt` for declared dependencies.

## Run Example

After a successful build the executable (for example `trader`) can be found under `build/bin` or `bin` and run directly:

```
./bin/trader
```

See `app/` for example apps and strategy templates.

## Project Layout (brief)

- `core/`: core libraries (networking, event loop, time, logging, etc.)
- `engine/`: trading engine, objects and settings
- `app/`: example applications and CTA strategy templates
- `core/exchange/`: exchange adapters

## Contributing

- Contributions welcome: add exchanges, improve the strategy framework, or enhance docs.
- Please include clear descriptions and reproducible test steps with PRs.

## Disclaimer

This project is a technical framework for exchange connectivity and strategy execution and is not investment advice. Trading involves risk; users assume all responsibility for any financial losses. Thoroughly test with sandboxes or small amounts before using real funds.

---

## 中文简介

`KeenTrader` 是一个以 C++ 开发的轻量级量化交易与接入引擎，提供交易所接入、行情与委托管理、策略框架等基础功能，支持多平台构建。项目目录中 `core/exchange` 包含已实现的接入适配器，当前支持 Binance 与 OKX，后续计划支持更多主流交易所。

如需我将此变更提交为 Git 提交并推送到远程仓库，或另外生成纯英文 / 中英双语的 README 格式（例如英中并列），请告诉我。
# KeenTrader

## 项目简介

`KeenTrader` 是一个轻量级、高性能的量化交易与接入引擎框架，旨在为交易策略、行情和委托管理提供统一且可扩展的基础设施。代码采用 C++ 编写，使用 CMake 构建，可在多平台编译运行。

## 主要功能

- 多交易所接入层（统一的 Exchange 接口）
- HTTP/WS 客户端与事件循环支持
- 策略框架（CTA）与示例策略
- 日志、时间/序列工具与常用工具库

## 当前已支持的交易所

- Binance（线性合约，实现位于 `core/exchange/binance_linear_exchange.*`）
- OKX（实现位于 `core/exchange/okx_exchange.*`）

（实现文件位于 [core/exchange](core/exchange)）

## 计划支持的交易所

计划在后续版本中扩展更多主流交易所的接入，例如 Bybit、Huobi、Coinbase 等（按优先级与贡献者安排实现）。

## 跨平台编译指南

以下为常用平台的快速编译步骤，假设已从仓库根目录运行命令。

通用步骤

1. 克隆仓库（包含子模块）：

```
git clone --recursive https://github.com/harvestsure/KeenTrader.git
cd KeenTrader
```

2. 创建构建目录并使用 CMake：

```
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

macOS（Homebrew）

```
brew install cmake openssl
# 可选：安装 vcpkg 管理第三方库
# clone vcpkg 并按指引 bootstrap
mkdir build && cd build
cmake .. -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
cmake --build .
```

Linux（Ubuntu/Debian）

```
sudo apt update
sudo apt install -y build-essential cmake libssl-dev git
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Windows（Visual Studio + vcpkg）

1. 安装 Visual Studio（含 C++ 工具）
2. 使用 `vcpkg` 安装依赖（如 OpenSSL），并连接 vcpkg 到 MSVC 工具链

示例：

```
# 在 PowerShell 中
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.bat
./vcpkg/vcpkg install openssl:x64-windows
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

注意

- 如果使用自定义安装路径，请通过 `-DOPENSSL_ROOT_DIR` 或 CMake 的相关变量指明。具体依赖请参考 `vcpkg.json` 与 `CMakeLists.txt`。

## 运行示例

构建成功后，可在 `build/bin`（或 `bin/`）目录找到可执行文件，例如 `trader`，直接运行即可：

```
./bin/trader
```

具体运行参数与配置请参阅 `app/` 下的示例与策略实现。

## 代码结构（简要）

- `core/`：核心库（网络、事件、时间、日志等）
- `engine/`：交易引擎、交易对象与设置
- `app/`：示例应用与 CTA 策略模版
- `core/exchange/`：各交易所接入实现

## 贡献与扩展

- 欢迎提交 PR：新增交易所、改进策略框架或完善文档
- 请在 PR 中附上简要说明与可复现的测试步骤

## 免责声明（风险自负）

本项目仅为交易接入与策略框架工具，不构成任何投资建议或推荐。使用本项目进行真实交易存在资金损失风险，用户应自行承担全部风险。在使用真实资金前，请充分测试并在沙箱或小额资金下验证策略与接入正确性。

---

如需我把本次变更提交为 Git 提交并推送到远端，请告诉我。要我继续补充英文版或更详细的构建说明吗？
# keen-trader

## Install third party software

* Install **Openssl**

## Clone source code and prepare libraries

Open *terminal*, run

    git clone --recursive https://github.com/harvestsure/KeenTrader.git
    mkdir build
    cd build
    cmake ..

## Status

[![windows](https://github.com/harvestsure/KeenTrader/actions/workflows/windows.yml/badge.svg)](https://github.com/harvestsure/KeenTrader/actions/workflows/windows.yml)
[![ubuntu](https://github.com/harvestsure/KeenTrader/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/harvestsure/KeenTrader/actions/workflows/ubuntu.yml)
[![macos](https://github.com/harvestsure/KeenTrader/actions/workflows/macos.yml/badge.svg)](https://github.com/harvestsure/KeenTrader/actions/workflows/macos.yml)
