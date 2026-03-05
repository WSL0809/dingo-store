# dingodb_cli 输出适配计划

## 一、项目概述

### 目标
将所有 `dingodb_cli` 命令的输出适配为支持终端和重定向两种模式，确保内容在重定向到文件时不被截断。

### 参考实现
`PrintTableAdaptive()` (src/client_v2/pretty.cc:120-225) 已实现 TTY 检测和重定向模式下的纯文本输出。

---

## 二、现状调研

### 2.1 表格输出函数现状

| 函数名 | 定义位置 | 功能 | 当前问题 |
|--------|----------|------|----------|
| `PrintTable(string版本)` | Line 95-118 | 输出字符串表格 | ❌ 无TTY检测，FTXUI直接渲染 |
| `PrintTableAdaptive` | Line 120-225 | 自适应表格输出 | ✅ 已支持TTY检测 |
| `PrintTable(Element版本)` | Line 227-252 | 输出FTXUI Element表格 | ❌ 无TTY检测 |
| `PrintTableInteractive` | Line 260-364 | 交互式表格 | ⚠️ 仅终端模式，无法重定向 |

### 2.2 Pretty::Show 函数分类（35个）

#### 使用 PrintTable(string版本) - 需适配
```
Line 388:  Show(GetCoordinatorMapResponse) - GetCoordinatorMap
Line 998:  Show(vector<TenantInfo>) - GetTenants  
Line 1446: Show(CreateIdsResponse) - CreateIds
Line 1660: Show(GetExecutorMapResponse) - GetExecutorMap
Line 1685: Show(QueryRegionResponse) - QueryRegion (注意：这里调用PrintTableAdaptive)
Line 1699: Show(VectorGetBorderIdResponse) - VectorGetBorderId
Line 1713: Show(VectorCountResponse) - VectorCount
Line 1724: Show(TxnScanResponse, calc_count) - TxnScan (重载)
Line 1744: Show(VectorCalcDistanceResponse) - VectorCalcDistance
Line 1770: Show(GetTenantsResponse) - GetTenants
Line 1796: Show(IndexRegionMetricsResponse) - GetIndexRegionMetrics
Line 1813: Show(TxnScanResponse) - TxnScan
```

#### 使用 PrintTable(Element版本) - 需适配
```
Line 487:  ShowTxnTableData - DumpRegion TxnTable
Line 535:  ShowTxnTableLock - DumpRegion TxnTable
Line 573:  ShowTxnDocumentIndexData - DumpRegion
Line 686:  ShowTxnTableLock - DumpRegion
Line 763:  ShowTxnTableWrite - DumpRegion
Line 820:  ShowTxnVectorIndexWrite - DumpRegion
Line 863:  ShowTxnDocumentIndexWrite - DumpRegion
Line 914:  Show(DumpRegionResponse::Data) - DumpRegion
Line 944:  Show(DumpRegionResponse::Data) - DumpRegion Vector
Line 966:  Show(DumpRegionResponse::Data) - DumpRegion Document
Line 1008: ShowKeyValues - TxnScan
Line 1034: ShowVectorWithIds - TxnScan/VectorSearch
Line 1052: ShowDocumentWithIds - TxnScan
Line 1103: ShowLockInfo - TxnScanLock
Line 1130: Show(CreateIndexResponse) - CreateIndex
Line 1153: Show(DocumentSearchResponse) - DocumentSearch
Line 1176: Show(DocumentSearchAllResponse) - DocumentSearchAll
Line 1205: Show(DocumentBatchQueryResponse) - DocumentBatchQuery
Line 1221: Show(DocumentGetBorderIdResponse) - DocumentGetBorderId
Line 1249: Show(DocumentScanQueryResponse) - DocumentScanQuery
Line 1265: Show(DocumentCountResponse) - DocumentCount
Line 1288: Show(DocumentGetRegionMetricsResponse) - DocumentGetRegionMetrics
Line 1319: Show(vector<Region>) - GetRegionMap/ModifyRegionMeta
Line 1386: Show(Schemas) - GetSchemas
Line 1400: Show(GetSchemaResponse) - GetSchema
Line 1411: Show(GetSchemaByNameResponse) - GetSchemaByName
Line 1473: Show(TsoResponse) - GetTso
Line 1516: Show(GetTablesBySchemaResponse) - GetTablesBySchema
Line 1570: Show(GCSafePointResponse) - GetGCSafePoint
Line 1621: Show(GetJobListResponse) - GetJobList (非交互模式)
```

#### 特殊情况
```
Line 420: PrintTable被注释，使用PrintTableAdaptive - GetStoreMap (已适配)
Line 1619: PrintTableInteractive - GetJobList (交互模式，无法重定向)
```

---

## 三、适配策略

### 3.1 核心设计原则

1. **向后兼容**: 不改发现有接口，只增加内部实现
2. **统一入口**: 所有表格输出通过统一的 TTY 检测逻辑
3. **纯文本降级**: 重定向模式下输出简化但完整的表格格式
4. **可测试性**: 通过依赖注入支持模拟 TTY 状态

### 3.2 技术方案

#### 方案 A: 改造现有 PrintTable 函数（推荐）

```cpp
// 1. 提取纯文本输出逻辑为独立函数
void PrintTableToFile(const std::vector<std::vector<std::string>>& rows, 
                      std::ostream& out = std::cout);
void PrintElementsToFile(const std::vector<std::vector<ftxui::Element>>& rows,
                         std::ostream& out = std::cout);

// 2. 修改 PrintTable(string版本) 添加 TTY 检测
void PrintTable(const std::vector<std::vector<std::string>>& rows) {
  if (!isatty(STDOUT_FILENO)) {
    PrintTableToFile(rows);
    return;
  }
  // 原有 FTXUI 逻辑
}

// 3. 修改 PrintTable(Element版本)
void PrintTable(const std::vector<std::vector<ftxui::Element>>& rows) {
  if (!isatty(STDOUT_FILENO)) {
    PrintElementsToFile(rows);
    return;
  }
  // 原有 FTXUI 逻辑
}
```

#### 方案 B: 为 Element 表格提供字符串转换

由于 `ftxui::Element` 包含富文本属性，需要提取其中的文本内容：

```cpp
// 新增辅助函数
std::string ExtractElementText(const ftxui::Element& element);
std::vector<std::vector<std::string>> ConvertElementsToStrings(
    const std::vector<std::vector<ftxui::Element>>& rows);
```

---

## 四、详细任务分解

### Phase 1: 基础框架改造（3天）

#### Task 1.1: 重构 PrintTable(string版本)
- **文件**: src/client_v2/pretty.cc
- **行号**: 95-118
- **工作项**:
  1. 提取纯文本输出逻辑到 `PrintTableToFile()`
  2. 在 `PrintTable()` 开头添加 TTY 检测
  3. 重定向模式下调用 `PrintTableToFile()`
  4. 终端模式下保持原有 FTXUI 逻辑
- **验证方式**: 编译通过，单元测试通过

#### Task 1.2: 实现 Element 到字符串的转换
- **文件**: src/client_v2/pretty.cc
- **新增**: ~50行代码
- **工作项**:
  1. 实现 `ExtractElementText()` 递归提取 Element 中的文本
  2. 实现 `ConvertElementsToStrings()` 批量转换
  3. 支持常见 Element 类型: text, paragraph, vflow, separator
- **验证方式**: 单元测试覆盖各种 Element 类型

#### Task 1.3: 重构 PrintTable(Element版本)
- **文件**: src/client_v2/pretty.cc
- **行号**: 227-252
- **工作项**:
  1. 添加 TTY 检测
  2. 重定向模式下：Element → String → PrintTableToFile
  3. 终端模式下保持原有 FTXUI 逻辑
- **验证方式**: 编译通过，单元测试通过

### Phase 2: 辅助数据结构适配（2天）

#### Task 2.1: 调研所有数据转换函数
- **工作项**:
  1. 列出所有使用 Element 表格的 Show 函数
  2. 分析每个函数中 Element 的构造方式
  3. 确定哪些可以直接转为字符串，哪些需要特殊处理

#### Task 2.2: 实现格式化辅助函数
- **新增文件**: src/client_v2/pretty_helper.cc (可选)
- **工作项**:
  1. 统一向量数据格式化函数
  2. 统一文档数据格式化函数  
  3. 统一锁信息格式化函数
  4. 确保字符串格式与 Element 视觉输出一致

### Phase 3: 单元测试（3天）

#### Task 3.1: 创建测试框架
- **新建文件**: test/unit_test/client_v2/test_pretty.cc
- **工作项**:
  1. 参考 test/unit_test/common/test_helper.cc 的测试结构
  2. 设计 TTY 模拟机制（mock isatty 返回值）
  3. 设计输出捕获机制（ostringstream）

#### Task 3.2: 编写 PrintTable 测试用例
- **测试覆盖**:
```cpp
// 基础表格测试
TEST_F(PrettyTest, PrintTable_String_Basic);
TEST_F(PrettyTest, PrintTable_String_Empty);
TEST_F(PrettyTest, PrintTable_String_LargeContent);
TEST_F(PrettyTest, PrintTable_String_SpecialChars);

// TTY 模式测试
TEST_F(PrettyTest, PrintTable_String_TtyMode);
TEST_F(PrettyTest, PrintTable_String_NonTtyMode);

// Element 表格测试
TEST_F(PrettyTest, PrintTable_Element_Basic);
TEST_F(PrettyTest, PrintTable_Element_MixedTypes);
TEST_F(PrettyTest, PrintTable_Element_Empty);

// 对比测试：TTY vs 非TTY 输出内容一致
TEST_F(PrettyTest, OutputConsistency_String);
TEST_F(PrettyTest, OutputConsistency_Element);
```

#### Task 3.3: 编写 Show 函数集成测试
- **测试覆盖**:
```cpp
// 各 Show 函数在非 TTY 模式下输出完整性测试
TEST_F(PrettyTest, Show_GetCoordinatorMap_Redirect);
TEST_F(PrettyTest, Show_GetStoreMap_Redirect);
TEST_F(PrettyTest, Show_TxnScan_Redirect);
TEST_F(PrettyTest, Show_DumpRegion_Redirect);
// ... 其他 Show 函数
```

### Phase 4: 集成与回归测试（2天）

#### Task 4.1: 全量编译验证
- **工作项**:
  1. 确保所有修改的文件编译通过
  2. 运行现有 unit_test 确保无回归
  3. 检查是否有其他模块依赖被修改的接口

#### Task 4.2: 手动验证关键命令
- **验证命令列表**:
```bash
# 终端模式
./dingodb_cli GetCoordinatorMap
./dingodb_cli GetStoreMap
./dingodb_cli GetRegionMap
./dingodb_cli TxnDump

# 重定向模式
./dingodb_cli GetCoordinatorMap > /tmp/test1.txt && wc -l /tmp/test1.txt
./dingodb_cli GetStoreMap > /tmp/test2.txt && wc -l /tmp/test2.txt
./dingodb_cli TxnDump --region_id=xxx > /tmp/test3.txt && wc -l /tmp/test3.txt

# 管道模式
./dingodb_cli GetStoreMap | cat | wc -l
./dingodb_cli TxnDump --region_id=xxx | grep "Key"
```

---

## 五、代码结构图

### 修改前
```
Pretty::Show(Response)
    └── PrintTable(rows)  // 直接 FTXUI，无 TTY 检测
```

### 修改后
```
Pretty::Show(Response)
    └── PrintTable(rows)  // 统一入口
        ├── TTY? ──Yes──> PrintTableFtxui(rows)  // 终端美化
        └── TTY? ──No──-> PrintTableToFile(rows) // 纯文本完整
        
PrintTableToFile(rows)
    ├── 计算列宽
    ├── 输出边框
    ├── 输出内容（左对齐）
    └── 输出边框
```

---

## 六、新增/修改文件清单

### 修改文件
| 文件路径 | 修改类型 | 行数变化 | 说明 |
|----------|----------|----------|------|
| src/client_v2/pretty.cc | 修改 | +150/-50 | 重构 PrintTable，新增纯文本输出 |
| src/client_v2/pretty.h | 修改 | +10 | 新增辅助函数声明 |

### 新增文件
| 文件路径 | 类型 | 行数 | 说明 |
|----------|------|------|------|
| test/unit_test/client_v2/test_pretty.cc | 测试 | ~500 | PrintTable 单元测试 |

---

## 七、风险评估

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| Element 提取不完整 | 中 | 高 | 全面单元测试覆盖各种 Element 类型 |
| 列宽计算不一致 | 低 | 中 | TTY/非TTY 模式使用相同列宽算法 |
| 特殊字符处理 | 中 | 中 | 测试包含 Unicode、控制字符等边界情况 |
| 性能退化 | 低 | 低 | 纯文本输出比 FTXUI 更快，无性能问题 |

---

## 八、验收标准

### 功能验收
- [ ] 所有使用 `PrintTable` 的命令在 TTY 模式下正常显示
- [ ] 所有使用 `PrintTable` 的命令在重定向模式下输出完整内容
- [ ] 管道传输（`| cat`）时输出不截断
- [ ] 输出格式与原有 FTXUI 格式一致（列对齐）

### 测试验收
- [ ] 单元测试覆盖率 > 80%
- [ ] 所有现有 unit_test 通过
- [ ] 新增测试用例 > 20 个

### 回归验收
- [ ] `./dingodb_cli --help` 正常输出
- [ ] 手动验证 10+ 个命令输出正确

---

## 九、时间估算

| 阶段 | 工作天数 | 备注 |
|------|----------|------|
| Phase 1: 基础框架改造 | 3天 | 核心逻辑修改 |
| Phase 2: 辅助数据结构 | 2天 | 格式化函数优化 |
| Phase 3: 单元测试 | 3天 | 全面测试覆盖 |
| Phase 4: 集成与回归 | 2天 | 验证与修复 |
| **总计** | **10天** | 含缓冲时间 |

---

## 十、后续优化（可选）

1. **JSON 输出支持**: 增加 `--format=json` 选项
2. **CSV 输出支持**: 增加 `--format=csv` 选项
3. **颜色控制**: 增加 `--color=auto|always|never` 选项
4. **列过滤**: 增加 `--columns=col1,col2` 选项

---

**计划制定**: 2026-03-04  
**文档版本**: 1.0  
**状态**: 待审核/实施
