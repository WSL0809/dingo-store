# dingodb_cli 输出重定向截断问题分析报告

## 问题概述

`dingodb_cli` 部分命令在输出重定向到文件时可能会出现内容被截断的情况。这一问题主要由 **FTXUI 库**的终端渲染机制与重定向输出不兼容导致。

---

## 根因分析

### 1. FTXUI 终端渲染库的工作原理

`dingodb_cli` 使用 **FTXUI**（一个 C++ 终端 UI 库）来渲染表格输出。FTXUI 的核心渲染流程：

```cpp
// src/client_v2/pretty.cc:112-115
auto document = table.Render();
auto screen = ftxui::Screen::Create(ftxui::Dimension::Fit(document));
ftxui::Render(screen, document);
screen.Print();  // 输出到终端
```

**问题点**：
- `ftxui::Screen` 是基于**终端尺寸**创建的
- `Dimension::Fit()` 根据终端宽度计算布局
- 当输出被重定向到文件时，`isatty(STDOUT_FILENO)` 返回 false，但 FTXUI 仍尝试按终端方式渲染

### 2. 截断发生的具体场景

| 函数 | 调用命令 | 是否适配重定向 | 截断原因 |
|------|----------|----------------|----------|
| `PrintTable()` (std::string 版本) | GetCoordinatorMap, DumpRegion 等 | ❌ 未适配 | 直接使用 FTXUI，无 TTY 检测 |
| `PrintTable()` (Element 版本) | TxnScan, VectorSearch 等 | ❌ 未适配 | 直接使用 FTXUI，无 TTY 检测 |
| `PrintTableAdaptive()` | GetStoreMap, QueryRegion | ✅ 已适配 | 检测非 TTY 时使用纯文本格式 |
| `PrintTableInteractive()` | GetJobList (交互模式) | ❌ 仅终端 | 必须 TTY，无法重定向 |

---

## 代码分析

### 已适配的命令（PrintTableAdaptive）

```cpp
// src/client_v2/pretty.cc:134-175
static void PrintTableAdaptive(const std::vector<std::vector<std::string>>& rows) {
  // ...
  bool is_tty = isatty(STDOUT_FILENO);  // 检测是否为终端

  if (!is_tty) {
    // 重定向到文件：使用纯文本格式，确保内容完整
    // 手动输出表格边框和内容
    std::cout << "+" << std::string(col_widths[i] + 2, '-') << "+";
    // ...
    return;
  }

  // 终端模式：使用 FTXUI 渲染
  // ...
}
```

### 未适配的命令（PrintTable）

```cpp
// src/client_v2/pretty.cc:95-118
static void PrintTable(const std::vector<std::vector<std::string>>& rows) {
  // 直接使用 FTXUI，无 TTY 检测
  auto table = ftxui::Table(rows);
  auto document = table.Render();
  auto screen = ftxui::Screen::Create(ftxui::Dimension::Fit(document));
  // ...
}
```

### 仅终端的命令（PrintTableInteractive）

```cpp
// src/client_v2/pretty.cc:260-364
void Pretty::PrintTableInteractive(const std::vector<std::vector<ftxui::Element>>& rows) {
  auto screen = ScreenInteractive::Fullscreen();  // 必须全屏终端
  // 等待用户按键交互
  screen.Loop(event_handler);
}
```

---

## 受影响命令列表

### 已确认有截断风险的命令（使用未适配的 PrintTable）

```
GetCoordinatorMap           - GetCoordinatorMapResponse 解析
GetStoreMap                 - 已使用 PrintTableAdaptive，安全
DumpRegion                  - DumpRegionResponse 解析  
TxnScan                     - TxnScanResponse 解析
TxnScanLock                 - TxnScanLockResponse 解析
VectorSearch                - VectorWithIds 表格
DocumentSearch              - DocumentWithScores 表格
CreateIndex                 - CreateIndexResponse 解析
CreateSchema                - Schemas 表格
GetTablesBySchema           - Tables 表格
GetTenants                  - Tenants 表格
DumpVector/Doc/Data         - TxnTable 数据
GetRegionMetrics            - RegionMetrics 解析
GetExecutorMap              - ExecutorMap 解析
GetGCSafePoint              - GCSafePoint 解析
QueryRegion                 - 已使用 PrintTableAdaptive，安全
GetJobList                  - 参数控制，交互模式无法重定向
GetCoordinatorMap           - CoordinatorMap 解析
```

---

## 复现步骤

### 1. 直接输出（完整）
```bash
./dingodb_cli GetCoordinatorMap
# 输出完整，但受终端宽度限制
```

### 2. 重定向到文件（可能截断）
```bash
./dingodb_cli GetCoordinatorMap > /tmp/output.txt
# 部分字段可能被截断或格式错乱
```

### 3. 管道传输（可能截断）
```bash
./dingodb_cli GetCoordinatorMap | cat
# 输出可能不完整
```

---

## 修复方案

### 方案一：扩展 PrintTableAdaptive 适配所有字符串表格（推荐）

将 `PrintTableAdaptive` 的逻辑扩展到所有使用 `std::vector<std::vector<std::string>>` 的 `PrintTable` 调用：

```cpp
// 修改 src/client_v2/pretty.cc:95-118
static void PrintTable(const std::vector<std::vector<std::string>>& rows) {
  if (rows.empty()) return;
  
  bool is_tty = isatty(STDOUT_FILENO);
  
  if (!is_tty) {
    // 重定向模式：使用 PrintTableAdaptive 的逻辑
    PrintTableToFile(rows);  // 新增函数
    return;
  }
  
  // 终端模式：使用 FTXUI
  // ... 原有代码
}
```

### 方案二：为 Element 表格添加重定向支持

对于使用 `std::vector<std::vector<ftxui::Element>>` 的表格，提供降级方案：

```cpp
// 新增辅助函数
static void PrintElementsToFile(const std::vector<std::vector<ftxui::Element>>& rows) {
  // 将 ftxui::Element 转为纯文本
  // 简化输出格式但保证内容完整
}
```

### 方案三：增加 --plain/--json 输出选项

在命令行参数中增加输出格式选项：

```bash
# 增加 --format 参数
./dingodb_cli GetCoordinatorMap --format=plain  # 纯文本表格
./dingodb_cli GetCoordinatorMap --format=json   # JSON 格式
./dingodb_cli GetCoordinatorMap --format=table  # FTXUI 表格（默认）
```

---

## 临时解决方案

在修复工程实现之前，用户可以采用以下方法避免截断：

### 方法 1：使用 `script` 命令捕获终端输出
```bash
script -q /tmp/output.txt
./dingodb_cli GetCoordinatorMap
exit
cat /tmp/output.txt
```

### 方法 2：使用 `expect` 伪终端
```bash
unbuffer ./dingodb_cli GetCoordinatorMap > /tmp/output.txt
```

### 方法 3：指定较大的终端宽度（如果命令支持）
```bash
# 设置足够宽的环境变量（需代码支持）
COLUMNS=500 ./dingodb_cli GetCoordinatorMap > /tmp/output.txt
```

### 方法 4：使用交互模式查看（适合 GetJobList）
```bash
./dingodb_cli GetJobList --interactive=true
# 使用 q 退出，无法重定向
```

---

## 实现建议

### 优先级

| 优先级 | 任务 | 影响范围 |
|--------|------|----------|
| P0 | 修改 `PrintTable(std::string 版本)` 支持 TTY 检测 | 大部分命令 |
| P1 | 为 Element 表格添加纯文本降级 | TxnScan, VectorSearch 等 |
| P2 | 增加 `--format` 全局选项 | 所有命令 |
| P3 | 修复 `PrintTableInteractive` 文档说明 | GetJobList |

### 代码修改位置

```
src/client_v2/pretty.cc:
  - Line 95-118:  PrintTable (string 版本) - 需要修改
  - Line 227-252: PrintTable (Element 版本) - 需要修改
  - Line 120-225: PrintTableAdaptive - 参照实现
```

---

## 结论

`dingodb_cli` 的输出截断问题源于 **FTXUI 库在重定向场景下的设计限制**。现有代码已部分解决（`PrintTableAdaptive` 用于 GetStoreMap、QueryRegion），但大部分命令仍使用未适配的 `PrintTable`。

**建议立即采取措施**：
1. 统一表格输出函数，所有表格输出都进行 TTY 检测
2. 提供 `--format=plain` 选项允许用户强制使用纯文本输出
3. 更新文档说明哪些命令支持重定向

---

**分析日期**: 2026-03-04  
**分析版本**: dingo-store v3.0.0  
**相关文件**: `src/client_v2/pretty.cc`, `src/client_v2/pretty.h`
