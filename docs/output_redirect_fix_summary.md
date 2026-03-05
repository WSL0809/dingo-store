# dingodb_cli 输出重定向修复总结

## 修改概述

将 `dingodb_cli` 的所有表格输出命令适配为支持终端和重定向两种模式，确保内容在重定向到文件时不被截断。

## 修改文件

### 1. src/client_v2/pretty.cc

#### 新增函数

| 函数名 | 功能 | 说明 |
|--------|------|------|
| `PrintTableToFile(const vector<vector<string>>&)` | 输出纯文本表格 | 用于重定向模式，确保内容完整 |
| `PrintTableToTerminal(const vector<vector<string>>&)` | FTXUI终端输出 | 用于TTY模式，美化渲染 |
| `PrintTableToFile(const vector<vector<Element>>&)` | Element转纯文本输出 | 用于Element表格的重定向模式 |
| `PrintTableToTerminal(const vector<vector<Element>>&)` | FTXUI Element输出 | 用于Element表格的TTY模式 |
| `ExtractElementText(const Element&)` | 提取Element文本内容 | 将FTXUI Element转为字符串 |
| `ConvertElementsToStrings()` | 批量转换Element | 为Element表格提供降级方案 |

#### 修改的函数

| 函数名 | 变更 |
|--------|------|
| `PrintTable(string版本)` | 添加TTY检测，自动选择输出模式 |
| `PrintTable(Element版本)` | 添加TTY检测，非TTY时转为纯文本输出 |

### 2. test/unit_test/client_v2/test_pretty.cc

新增34个单元测试用例，覆盖：
- ShowError 错误处理测试
- GetCoordinatorMapResponse 测试
- GetStoreMapResponse 测试
- QueryRegionResponse 测试
- TxnScanResponse 测试
- DocumentSearchResponse 测试
- Region列表测试
- TSO响应测试
- 边界情况测试（空列表、大数据量、特殊字符）

### 3. test/unit_test/client_v2/CMakeLists.txt

新增client_v2测试模块的CMake配置。

### 4. test/unit_test/CMakeLists.txt

添加对 client_v2 子目录的引用。

## 适配命令列表

以下命令现在支持重定向输出（内容不会截断）：

| 类别 | 命令 |
|------|------|
| Coordinator | GetCoordinatorMap, GetStoreMap, QueryRegion, CreateIds |
| Executor | GetExecutorMap |
| Region | GetRegionMap, DumpRegion |
| Transaction | TxnScan, TxnScanLock |
| Vector | VectorGetBorderId, VectorCount, VectorCalcDistance |
| Document | DocumentSearch, DocumentSearchAll, DocumentBatchQuery |
| Meta | GetTenants, GetSchemas, GetSchema, GetTablesBySchema |
| TSO | GetTso |
| GC | GetGCSafePoint |

## 工作原理

```
┌─────────────────────────────────────────────────────────────────┐
│                        PrintTable (Unified)                     │
└──────────────────────────┬──────────────────────────────────────┘
                           │
              ┌────────────┴────────────┐
              │ isatty(STDOUT_FILENO)   │
              └────────────┬────────────┘
                           │
           ┌───────────────┴───────────────┐
           │                               │
      TTY == true                      TTY == false
           │                               │
           ▼                               ▼
┌──────────────────────┐          ┌──────────────────────┐
│ PrintTableToTerminal │          │ PrintTableToFile     │
│ (FTXUI渲染)          │          │ (纯文本表格)         │
│                      │          │                      │
│ - 屏幕自适应         │          │ - 内容自适应         │
│ - 颜色边框           │          │ - ASCII边框          │
│ - 交互式表格         │          │ - 完整数据           │
└──────────────────────┘          └──────────────────────┘
```

## 输出格式对比

### 终端模式（TTY）
```
╔═══════════════════════════════════════════╗
║ ID    Type    Address            State    ║
╠═══════════════════════════════════════════╣
║ 1001  STORE   192.168.1.1:20001  ONLINE   ║
╚═══════════════════════════════════════════╝
```

### 重定向模式（非TTY）
```
+------+-------+------------------+--------+
| ID   | Type  | Address          | State  |
+======+=======+==================+========+
| 1001 | STORE | 192.168.1.1:20001| ONLINE |
+------+-------+------------------+--------+
```

## 验证方法

### 1. 编译验证
```bash
cd /root/dingo-store/build
source /opt/rh/gcc-toolset-14/enable
make dingodb_cli -j10
```

### 2. 功能验证

#### 终端输出测试
```bash
./dingodb_cli GetCoordinatorMap
# 应该显示带边框的美化表格
```

#### 重定向输出测试
```bash
./dingodb_cli GetCoordinatorMap > /tmp/test.txt
cat /tmp/test.txt
# 应该显示纯文本表格，内容完整
```

#### 管道输出测试
```bash
./dingodb_cli GetStoreMap | cat | head -20
# 应该完整输出，不截断
```

#### 内容完整性测试
```bash
./dingodb_cli GetStoreMap > /tmp/store_tty.txt
./dingodb_cli GetStoreMap | cat > /tmp/store_pipe.txt
wc -l /tmp/store_tty.txt /tmp/store_pipe.txt
# 两行数应该一致
```

## 兼容性说明

- **向后兼容**: 终端模式下的输出格式完全不变
- **无性能影响**: 仅在初始化时检测一次TTY状态
- **线程安全**: 所有函数保持原有的线程安全性

## 已知限制

1. **PrintTableInteractive**: `GetJobList --interactive=true` 仍需终端，无法重定向（这是设计意图，交互功能必须终端支持）

2. **Element文本提取**: `ExtractElementText()` 通过渲染到屏幕缓冲区提取文本，对于复杂的嵌套Element可能不够完美，但能满足正常表格输出需求。

## 后续建议

1. **JSON输出选项**: 考虑增加 `--format=json` 支持结构化数据导出
2. **CSV输出选项**: 考虑增加 `--format=csv` 便于数据处理
3. **测试增强**: 后续可以增加重定向模式的精确输出内容验证

---

**修改日期**: 2026-03-04  
**状态**: 已完成  
**测试覆盖**: 34个单元测试用例
