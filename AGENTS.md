# DingoDB CLI Project Guide for AI Agents

## Scope

This repository is intentionally trimmed to build **only** `dingodb_cli` (the v2 CLI client).

Avoid reintroducing:

- server/store/coordinator implementations
- Java SDK and packaging
- deploy/docker scripts
- unit-test trees for server modules

## Key Target

- `dingodb_cli` (output: `build/bin/dingodb_cli`)

## Layout

```text
src/client_v2/            # CLI commands and main()
src/common/               # Shared helpers required by CLI
src/coordinator/          # coordinator_interaction used by CLI
src/coprocessor/          # coprocessor utils used by CLI
src/mvcc/                 # mvcc codec used by CLI
src/vector/               # vector codec used by CLI
src/document/             # document codec used by CLI
src/serial/               # serialization submodule
dingo-store-proto/        # protobuf submodule (compiled into build/proto)
contrib/cli11/            # CLI parser
contrib/FTXUI/            # built via CMake ExternalProject
contrib/tantivy-search/   # Rust static lib consumed by CMake (prebuilt)
```

## Dependencies

- `dingo-eureka` is required for C/C++ third-party deps (gflags/glog/brpc/braft/protobuf/rocksdb/...).

`DINGO_EUREKA_INSTALL_PATH` lookup order:

1. `-DDINGO_EUREKA_INSTALL_PATH=...` (CMake option)
2. environment variable `DINGO_EUREKA_INSTALL_PATH`
3. default `~/.local/dingo-eureka`

## Submodules

Initialize only the kept submodules:

```bash
git submodule sync --recursive
git submodule update --init --recursive
```

## Build

`tantivy-search` must be built and staged at:

- `build/third-party/install/tantivy-search/lib/libtantivy_search.a`
- `build/third-party/install/tantivy-search/include/`

Example:

```bash
mkdir -p build/third-party/install/tantivy-search/lib
mkdir -p build/third-party/install/tantivy-search/include

(cd contrib/tantivy-search && cargo build --release)
cp contrib/tantivy-search/target/release/libtantivy_search.a build/third-party/install/tantivy-search/lib/
cp -r contrib/tantivy-search/include/* build/third-party/install/tantivy-search/include/

cmake -B build -DCMAKE_BUILD_TYPE=Release -DTHIRD_PARTY_BUILD_TYPE=Release
cmake --build build --target dingodb_cli -j

build/bin/dingodb_cli --help
```

## Conventions

- C++17, Google-ish style (see `.clang-format` and `.clang-tidy`)
- Keep changes scoped to CLI behavior
- Prefer `rg` for code search
