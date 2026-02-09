# KeenTrader

## Status

[![windows](https://github.com/harvestsure/KeenTrader/actions/workflows/windows.yml/badge.svg)](https://github.com/harvestsure/KeenTrader/actions/workflows/windows.yml)
[![ubuntu](https://github.com/harvestsure/KeenTrader/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/harvestsure/KeenTrader/actions/workflows/ubuntu.yml)
[![macos](https://github.com/harvestsure/KeenTrader/actions/workflows/macos.yml/badge.svg)](https://github.com/harvestsure/KeenTrader/actions/workflows/macos.yml)
[![clang-tidy](https://github.com/harvestsure/KeenTrader/actions/workflows/clang-tidy.yml/badge.svg)](https://github.com/harvestsure/KeenTrader/actions/workflows/clang-tidy.yml)

## Description

KeenTrader is a lightweight, high-performance quantitative trading and connectivity engine framework written in C++. It provides a unified foundation for market data, order management, and strategy execution. The project uses CMake for cross-platform builds.

## Key Features

- Unified Exchange interface for multiple venue integrations
# KeenTrader

## Description

KeenTrader is a lightweight, high-performance quantitative trading and connectivity engine framework written in C++. It provides a unified foundation for market data, order management, and strategy execution. The project uses CMake for cross-platform builds.

## Key Features

- Unified Exchange interface for multiple venue integrations
- HTTP and WebSocket clients with an event loop
- Strategy framework (CTA) with example strategies
- Logging, time utilities, and helper libraries
- Cross-platform support (Windows, macOS, Linux)
- Strict compile-time checks and static analysis (clang-tidy)

## Dependencies

### External Libraries (managed by vcpkg)
- **OpenSSL** (≥3.3.1) - TLS/SSL support
- **fmt** (≥11.0.0) - Modern string formatting
- **nlohmann-json** - JSON library
- **ASIO** - Network library
- **WebSocket++** - WebSocket protocol support

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

### Code Quality & Build Verification

The project maintains high code quality through:
- **Strict compilation flags**: `-Wall -Wextra -Wpedantic -Werror`
- **Additional checks**: `-Wshadow -Wconversion` to catch common errors
- **Static analysis**: Integrated clang-tidy for automated code inspection

Run clang-tidy manually:
```bash
cd build
make                           # Generate compile_commands.json
clang-tidy -p . ../core/*.cpp  # Check against standard checks
```

### Build Notes

- If libraries are installed in custom locations, set CMake variables such as `-DOPENSSL_ROOT_DIR` accordingly. See `vcpkg.json` and `CMakeLists.txt` for declared dependencies.
- On macOS with Apple Silicon, ensure Homebrew OpenSSL is properly linked.
- Windows builds require Visual Studio 2022 and vcpkg for dependency management.

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

