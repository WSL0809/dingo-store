# DingoDB CLI Project Guide for AI Agents

## Project Scope

This repository is **CLI-only**. It only contains code required to build and run `dingodb_cli`.

Removed from this repo:

- Server roles and storage engine implementation
- Java SDK and packaging
- Deployment/docker scripts
- Unit test suites for server modules

## Key Binary

- `dingodb_cli` (from `src/client_v2`)

## Source Layout

```text
src/client_v2/      # CLI commands and main()
src/common/         # Shared utilities required by CLI
src/coordinator/    # coordinator_interaction
src/coprocessor/    # coprocessor utils
src/mvcc/           # mvcc codec
src/vector/         # vector codec
src/document/       # document codec
src/serial/         # serialization submodule
```

## Submodules Kept

- `src/serial`
- `dingo-store-proto`
- `contrib/cli11`
- `contrib/FTXUI`
- `contrib/tantivy-search`

## Build Notes

1. Build/prepare `tantivy-search` static library first and copy it to:
   - `build/third-party/install/tantivy-search/lib/libtantivy_search.a`
   - `build/third-party/install/tantivy-search/include/`
2. Configure with CMake and build target `dingodb_cli` only.

## Coding Conventions

- C++17
- Keep changes scoped to CLI behavior
- Avoid reintroducing server-side dependencies into CLI paths
