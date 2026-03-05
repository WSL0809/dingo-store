# Build dingodb_cli

## 1. Prerequisites

- Linux with GCC/Clang toolchain
- CMake >= 3.23.1
- `dingo-eureka` installed
- Rust toolchain (for `contrib/tantivy-search`)

`dingo-eureka` lookup order:

1. `-DDINGO_EUREKA_INSTALL_PATH=...`
2. environment variable `DINGO_EUREKA_INSTALL_PATH`
3. default `~/.local/dingo-eureka`

## 2. Init

```bash
git submodule sync --recursive
git submodule update --init --recursive
```

## 3. Build tantivy-search static library

```bash
mkdir -p build/third-party/install/tantivy-search/lib
mkdir -p build/third-party/install/tantivy-search/include

cd contrib/tantivy-search
cargo build --release
cd ../..

cp contrib/tantivy-search/target/release/libtantivy_search.a build/third-party/install/tantivy-search/lib/
cp -r contrib/tantivy-search/include/* build/third-party/install/tantivy-search/include/
```

## 4. Configure and Build

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHIRD_PARTY_BUILD_TYPE=Release

cmake --build build --target dingodb_cli -j
```

Binary output:

- `build/bin/dingodb_cli`

## 5. Quick Verification

```bash
build/bin/dingodb_cli --help
printf "help\nexit\n" | build/bin/dingodb_cli
```
