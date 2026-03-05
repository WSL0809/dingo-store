# DingoDB CLI

`dingodb-cli` is a standalone command-line client for DingoDB/Dingo-Store clusters.

This repository is intentionally trimmed to only keep code and dependencies required by `dingodb_cli`.

## Features

- Coordinator, Store, Meta, KV, Vector, Document, and Tool commands
- Interactive shell mode and one-shot command mode
- Local RocksDB dump/restore helpers
- FTXUI table rendering and redirected output support

## Repository Layout

```text
.
├── src/client_v2/            # CLI commands and entrypoint
├── src/common/               # Shared helpers used by CLI
├── src/coordinator/          # Coordinator RPC interaction used by CLI
├── src/coprocessor/          # Coprocessor helper utilities used by CLI
├── src/mvcc/                 # MVCC codec used by CLI
├── src/vector/               # Vector codec used by CLI
├── src/document/             # Document codec used by CLI
├── src/serial/               # Serialization submodule
├── dingo-store-proto/        # Protobuf submodule
├── contrib/cli11             # CLI parser
├── contrib/FTXUI             # Terminal table rendering
└── contrib/tantivy-search    # Tantivy bridge (prebuilt library consumed by CMake)
```

## Build Prerequisites

- CMake >= 3.23.1
- C++17 toolchain
- `dingo-eureka` installed (default: `~/.local/dingo-eureka`)
- Rust toolchain (only for building `contrib/tantivy-search`)

## Build

```bash
git submodule sync --recursive
git submodule update --init --recursive

mkdir -p build

# Build tantivy-search static library first
cd contrib/tantivy-search
cargo build --release
cd ../..
mkdir -p build/third-party/install/tantivy-search/lib
mkdir -p build/third-party/install/tantivy-search/include
cp contrib/tantivy-search/target/release/libtantivy_search.a build/third-party/install/tantivy-search/lib/
cp -r contrib/tantivy-search/include/* build/third-party/install/tantivy-search/include/

# Configure + build CLI
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHIRD_PARTY_BUILD_TYPE=Release
cmake --build build --target dingodb_cli -j
```

Binary path:

- `build/bin/dingodb_cli`

## Run

```bash
# One-shot mode
build/bin/dingodb_cli --help

# Interactive mode
build/bin/dingodb_cli
```
