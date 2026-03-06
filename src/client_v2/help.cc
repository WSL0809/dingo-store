#include "client_v2/help.h"

#include <initializer_list>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace client_v2 {

namespace {

enum class HelpTag {
  kStable,
  kPresetTemplate,
  kExpert,
};

struct ExampleDoc {
  std::string command;
  std::string note;
};

struct CommandDoc {
  HelpTag tag;
  std::string summary;
  std::string use_when;
  std::vector<ExampleDoc> examples;
  std::vector<std::pair<std::string, std::string>> key_parameters;
  std::vector<std::string> prerequisites;
  std::vector<std::string> common_errors;
  std::vector<std::string> output_notes;
  std::vector<std::string> see_also;
};

using SummaryCatalog = std::map<std::string, std::pair<HelpTag, std::string>>;

std::string TagLabel(HelpTag tag) {
  switch (tag) {
    case HelpTag::kStable:
      return "[稳定]";
    case HelpTag::kPresetTemplate:
      return "[预设模板]";
    case HelpTag::kExpert:
      return "[专家/调试]";
  }
  return "[未知]";
}

std::string MakeTaggedSummary(HelpTag tag, const std::string& summary) {
  return TagLabel(tag) + " " + summary;
}

void AppendSimpleSection(std::ostringstream& out, std::string_view title, const std::vector<std::string>& items) {
  if (items.empty()) {
    return;
  }

  out << '\n' << title << '\n';
  for (const auto& item : items) {
    out << "  - " << item << '\n';
  }
}

void AppendExamples(std::ostringstream& out, const std::vector<ExampleDoc>& examples) {
  if (examples.empty()) {
    return;
  }

  out << "\n示例\n";
  for (const auto& example : examples) {
    out << "  " << example.command << '\n';
    if (!example.note.empty()) {
      out << "    " << example.note << '\n';
    }
  }
}

void AppendPairs(std::ostringstream& out, std::string_view title,
                 const std::vector<std::pair<std::string, std::string>>& pairs) {
  if (pairs.empty()) {
    return;
  }

  out << '\n' << title << '\n';
  for (const auto& [name, desc] : pairs) {
    out << "  " << name << ": " << desc << '\n';
  }
}

void SetCommandDoc(CLI::App* cmd, const CommandDoc& doc) {
  if (cmd == nullptr) {
    return;
  }

  cmd->description(MakeTaggedSummary(doc.tag, doc.summary));

  std::ostringstream footer;
  footer << "用途\n";
  footer << "  " << doc.use_when << '\n';
  AppendExamples(footer, doc.examples);
  AppendPairs(footer, "关键参数", doc.key_parameters);
  AppendSimpleSection(footer, "前置条件与默认行为", doc.prerequisites);
  AppendSimpleSection(footer, "常见错误", doc.common_errors);
  AppendSimpleSection(footer, "输出说明", doc.output_notes);
  AppendSimpleSection(footer, "相关命令", doc.see_also);
  cmd->footer(footer.str());
}

void SetOptionDescription(CLI::App* cmd, const std::string& name, const std::string& description) {
  if (cmd == nullptr) {
    return;
  }

  auto* option = cmd->get_option_no_throw(name);
  if (option != nullptr) {
    option->description(description);
  }
}

SummaryCatalog BuildSummaryCatalog() {
  return {
      {"GetCoordinatorMap", {HelpTag::kStable, "查看 coordinator 节点拓扑与角色分布。"}},
      {"GetStoreMap", {HelpTag::kStable, "查看 store 节点清单、状态与基础元信息。"}},
      {"GetRegionMap", {HelpTag::kStable, "查看 region 分布；支持按 tenant 过滤。"}},
      {"GetJobList", {HelpTag::kExpert, "查看后台任务列表与执行状态。"}},
      {"CleanJobList", {HelpTag::kExpert, "清理后台任务记录。"}},
      {"ChangeLogLevel", {HelpTag::kExpert, "远程调整节点日志级别。"}},
      {"TransferLeaderRegion", {HelpTag::kExpert, "触发 region leader 迁移。"}},
      {"CreateExecutor", {HelpTag::kExpert, "注册 executor 节点。"}},
      {"DeleteExecutor", {HelpTag::kExpert, "删除 executor 节点。"}},
      {"DeleteExcutor", {HelpTag::kExpert, "删除 executor 节点。"}},
      {"ExecutorHeartbeat", {HelpTag::kExpert, "上报 executor 心跳。"}},
      {"GetExecutorMap", {HelpTag::kExpert, "查看 executor 节点映射。"}},
      {"UpdateGCSafePoint", {HelpTag::kExpert, "更新全局 GC safe point。"}},
      {"UpdateTenantGCSafePoint", {HelpTag::kExpert, "更新租户级 GC safe point。"}},
      {"UpdateGCFlag", {HelpTag::kExpert, "修改 GC 开关状态。"}},
      {"GetGCSafePoint", {HelpTag::kExpert, "查询当前 GC safe point。"}},
      {"GetBackUpStatus", {HelpTag::kExpert, "查看备份任务状态。"}},
      {"GetRestoreStatus", {HelpTag::kExpert, "查看恢复任务状态。"}},
      {"DisableBalance", {HelpTag::kExpert, "启用或关闭 leader/region balance。"}},
      {"AddPeerRegion", {HelpTag::kExpert, "为 region 增加副本。"}},
      {"RemovePeerRegion", {HelpTag::kExpert, "为 region 移除副本。"}},
      {"SplitRegion", {HelpTag::kExpert, "按 key 或 id 拆分 region。"}},
      {"MergeRegion", {HelpTag::kExpert, "合并两个 region。"}},
      {"QueryRegion", {HelpTag::kStable, "按 region_id 查询 region 元信息与副本状态。"}},
      {"Snapshot", {HelpTag::kExpert, "触发 region 快照。"}},
      {"GetRegionMetrics", {HelpTag::kExpert, "查询 region 指标或实际指标。"}},
      {"TxnDump", {HelpTag::kExpert, "导出 region 内的事务状态。"}},
      {"QueryRegionStatus", {HelpTag::kExpert, "查看 region 当前运行状态。"}},
      {"ModifyRegionMeta", {HelpTag::kExpert, "修改 region 元数据。"}},
      {"QueryMemoryLocks", {HelpTag::kExpert, "查询内存锁信息。"}},
      {"KvHello", {HelpTag::kExpert, "向 coordinator KV/lease 服务做连通性探测。"}},
      {"GetRawKvIndex", {HelpTag::kExpert, "查询原始 KV 索引信息。"}},
      {"GetRawKvRev", {HelpTag::kExpert, "查询原始 KV revision 信息。"}},
      {"CoorKvPut", {HelpTag::kStable, "向 coordinator 内置 KV 写入单条记录。"}},
      {"CoorKvRange", {HelpTag::kStable, "按前缀或区间读取 coordinator 内置 KV。"}},
      {"CoorKvDeleteRange", {HelpTag::kStable, "删除 coordinator 内置 KV 的单 key 或区间。"}},
      {"CoorKvCompaction", {HelpTag::kExpert, "对 coordinator 内置 KV 做 compact。"}},
      {"OneTimeWatch", {HelpTag::kExpert, "一次性 watch 指定 key 的变更事件。"}},
      {"Lock", {HelpTag::kExpert, "基于 coordinator KV 获取分布式锁。"}},
      {"LeaseGrant", {HelpTag::kExpert, "创建 lease。"}},
      {"LeaseRevoke", {HelpTag::kExpert, "撤销 lease。"}},
      {"LeaseRenew", {HelpTag::kExpert, "续租 lease。"}},
      {"LeaseQuery", {HelpTag::kExpert, "查询 lease 明细。"}},
      {"ListLeases", {HelpTag::kExpert, "列出租约。"}},
      {"CreateTable", {HelpTag::kPresetTemplate, "创建一个带固定列模板的测试表。"}},
      {"GetTable", {HelpTag::kStable, "按 table_id 查询表定义与分区信息。"}},
      {"GetTableByName", {HelpTag::kStable, "按 schema_id + 表名查询表定义。"}},
      {"GenTso", {HelpTag::kExpert, "申请新的 TSO。"}},
      {"GetSchemas", {HelpTag::kStable, "列出租户下的 schema。"}},
      {"GetSchema", {HelpTag::kStable, "按 schema_id 查询 schema 定义。"}},
      {"GetSchemaByName", {HelpTag::kStable, "按名称查询 schema。"}},
      {"GetTablesBySchema", {HelpTag::kStable, "列出 schema 下的表。"}},
      {"CreateSchema", {HelpTag::kStable, "在指定 tenant 下创建 schema。"}},
      {"DropSchema", {HelpTag::kExpert, "删除 schema。"}},
      {"GetRegionByTable", {HelpTag::kStable, "查看表或索引对应的 region 分布。"}},
      {"CreateTenant", {HelpTag::kExpert, "创建 tenant。"}},
      {"UpdateTenant", {HelpTag::kExpert, "更新 tenant。"}},
      {"DropTenant", {HelpTag::kExpert, "删除 tenant。"}},
      {"GetTenant", {HelpTag::kExpert, "查询 tenant 列表或详情。"}},
      {"ImportMeta", {HelpTag::kExpert, "从导出文件导入 meta 数据。"}},
      {"ExportMeta", {HelpTag::kExpert, "导出 meta 数据到文件。"}},
      {"KvGet", {HelpTag::kStable, "按 region_id + key 从 store 读取一条 KV。"}},
      {"KvPut", {HelpTag::kStable, "按 region_id + key 向 store 写入一条 KV。"}},
      {"DumpRegion", {HelpTag::kExpert, "导出 region 数据内容。"}},
      {"DumpDb", {HelpTag::kExpert, "导出 RocksDB 内容。"}},
      {"TxnScan", {HelpTag::kExpert, "扫描事务区间数据。"}},
      {"TxnScanLock", {HelpTag::kExpert, "扫描事务锁记录。"}},
      {"TxnPrewrite", {HelpTag::kExpert, "执行事务 prewrite。"}},
      {"TxnCommit", {HelpTag::kExpert, "执行事务 commit。"}},
      {"TxnPessimisticLock", {HelpTag::kExpert, "执行悲观锁加锁。"}},
      {"TxnPessimisticRollback", {HelpTag::kExpert, "执行悲观锁回滚。"}},
      {"TxnBatchGet", {HelpTag::kExpert, "批量读取事务 key。"}},
      {"TxnGC", {HelpTag::kExpert, "执行事务 GC。"}},
      {"TxnCount", {HelpTag::kExpert, "统计事务数据量。"}},
      {"WhichRegion", {HelpTag::kStable, "按 table/index id + key 定位目标 region。"}},
      {"Compact", {HelpTag::kExpert, "触发 store compact。"}},
      {"StoreEnableOrDisableSplitAndMerge", {HelpTag::kExpert, "启用或关闭 store 的 split/merge。"}},
      {"CreateDocumentIndex", {HelpTag::kPresetTemplate, "创建带固定字段模板的 document index。"}},
      {"DocumentAdd", {HelpTag::kStable, "写入单条 document。"}},
      {"DocumentBatchAdd", {HelpTag::kStable, "批量写入 document。"}},
      {"DocumentDelete", {HelpTag::kStable, "按 document id 范围删除 document。"}},
      {"DocumentSearch", {HelpTag::kStable, "按 query_string 搜索 document。"}},
      {"DocumentSearchAll", {HelpTag::kStable, "搜索所有 document。"}},
      {"DocumentBatchQuery", {HelpTag::kStable, "按 id 批量读取 document。"}},
      {"DocumentScanQuery", {HelpTag::kStable, "按 id 范围扫描 document。"}},
      {"DocumentGetMaxId", {HelpTag::kStable, "查询 document 最大 id。"}},
      {"DocumentGetMinId", {HelpTag::kStable, "查询 document 最小 id。"}},
      {"DocumentCount", {HelpTag::kStable, "统计 document 数量。"}},
      {"DocumentGetRegionMetrics", {HelpTag::kExpert, "查询 document region 指标。"}},
      {"StringToHex", {HelpTag::kStable, "把普通字符串编码成 hex。"}},
      {"HexToString", {HelpTag::kStable, "把 hex 解码成普通字符串。"}},
      {"EncodeTablePrefixToHex", {HelpTag::kExpert, "把 table 前缀编码成 hex key。"}},
      {"EncodeVectorPrefixToHex", {HelpTag::kExpert, "把 vector 前缀编码成 hex key。"}},
      {"DecodeTablePrefix", {HelpTag::kExpert, "解析 table 前缀 key。"}},
      {"DecodeVectorPrefix", {HelpTag::kExpert, "解析 vector 前缀 key。"}},
      {"OctalToHex", {HelpTag::kExpert, "把八进制字节串转成 hex。"}},
      {"CoordinatorDebug", {HelpTag::kExpert, "向 coordinator 发送调试请求。"}},
      {"TransformTS", {HelpTag::kExpert, "把时间戳转成人类可读时间。"}},
      {"GenPlainKey", {HelpTag::kExpert, "生成 plain key。"}},
      {"CreateIndex", {HelpTag::kPresetTemplate, "创建带固定模板参数的 vector index。"}},
      {"VectorAdd", {HelpTag::kStable, "写入单条向量。"}},
      {"VectorSearch", {HelpTag::kStable, "执行向量近邻检索。"}},
      {"VectorDelete", {HelpTag::kStable, "删除向量。"}},
      {"VectorGetMaxId", {HelpTag::kStable, "查询向量最大 id。"}},
      {"VectorGetMinId", {HelpTag::kStable, "查询向量最小 id。"}},
      {"VectorAddBatch", {HelpTag::kStable, "批量写入向量。"}},
      {"VectorCount", {HelpTag::kStable, "统计向量数量。"}},
      {"VectorCalcDistance", {HelpTag::kExpert, "计算两组向量之间的距离矩阵。"}},
      {"CountVectorTable", {HelpTag::kExpert, "统计 vector table 条数。"}},
      {"VectorSearchDebug", {HelpTag::kExpert, "输出更详细的向量搜索调试信息。"}},
      {"VectorGetRegionMetrics", {HelpTag::kExpert, "查询 vector region 指标。"}},
      {"VectorImport", {HelpTag::kExpert, "向 diskann 导入向量数据。"}},
      {"VectorBuild", {HelpTag::kExpert, "触发 diskann 建索引。"}},
      {"VectorLoad", {HelpTag::kExpert, "加载 diskann 索引。"}},
      {"VectorStatus", {HelpTag::kExpert, "查看 diskann 状态。"}},
      {"VectorReset", {HelpTag::kExpert, "重置 diskann 状态。"}},
      {"VectorDump", {HelpTag::kExpert, "导出 vector 数据。"}},
      {"VectorCountMemory", {HelpTag::kExpert, "统计内存中的向量数量。"}},
      {"IndexEnableOrDisableSplitAndMerge", {HelpTag::kExpert, "启用或关闭 index split/merge。"}},
      {"RestoreRegionData", {HelpTag::kExpert, "从 SST 恢复 region 数据。"}},
      {"CheckRestoreData", {HelpTag::kExpert, "校验恢复数据。"}},
      {"Shell", {HelpTag::kStable, "进入交互式 CLI shell。"}},
  };
}

void ApplyCatalogDescriptions(CLI::App& app) {
  const auto catalog = BuildSummaryCatalog();
  for (const auto& [name, entry] : catalog) {
    if (auto* cmd = app.get_subcommand_no_throw(name); cmd != nullptr) {
      cmd->description(MakeTaggedSummary(entry.first, entry.second));
    }
  }
}

void ApplyRootHelp(CLI::App& app) {
  app.description(
      "DingoDB v2 CLI。默认 `--help` 同时面向人类与大语言模型：先看分组与标签，再进入目标子命令帮助。");

  std::ostringstream footer;
  footer << "阅读顺序\n";
  footer << "  1. 先在本页选择分组与命令。\n";
  footer << "  2. 再执行 `dingodb_cli <子命令> --help` 查看用途、示例、关键参数与常见错误。\n";
  footer << "  3. 需要机器可读结果时，加 `--format json`；需要更完整 JSON 可加 `--json-pretty`。\n";
  footer << "\n常见工作流\n";
  footer << "  - 集群发现: `dingodb_cli GetCoordinatorMap --help`\n";
  footer << "  - schema / table 查询: `dingodb_cli GetSchemas --help`、`dingodb_cli GetTable --help`\n";
  footer << "  - coordinator KV: `dingodb_cli CoorKvPut --help`、`dingodb_cli CoorKvRange --help`\n";
  footer << "  - store KV 与 region 定位: `dingodb_cli KvPut --help`、`dingodb_cli WhichRegion --help`\n";
  footer << "\n全局约定\n";
  footer << "  - `--coor_url` 是 coordinator 地址发现入口；默认读取 `file://./coor_list`。\n";
  footer << "  - `schema_id` / `table_id` / `region_id` 都是服务端对象 id，不是名字。\n";
  footer << "  - 大多数查询命令可直接配合 `--format json` 供 agent/脚本消费。\n";
  footer << "\n标签图例\n";
  footer << "  - [稳定]: 常规使用入口，帮助文本可直接当操作手册。\n";
  footer << "  - [预设模板]: 命令内部带硬编码模板或示例数据，适合演示/测试，不是通用 DSL。\n";
  footer << "  - [专家/调试]: 面向运维、排障或底层 RPC，默认假设你理解对象模型与风险。\n";
  app.footer(footer.str());
}

void ApplyDiscoveryDocs(CLI::App& app) {
  SetCommandDoc(app.get_subcommand_no_throw("GetCoordinatorMap"),
                {HelpTag::kStable,
                 "查看 coordinator 节点拓扑与角色分布。",
                 "当你需要确认 coordinator 是否可连通、有哪些节点、谁是 leader 时使用。",
                 {
                     {"dingodb_cli GetCoordinatorMap",
                      "最小调用。默认从 `file://./coor_list` 读取 coordinator 地址。"},
                     {"dingodb_cli GetCoordinatorMap --format json",
                      "把结果转成结构化 JSON，适合 agent 或脚本后续解析。"},
                 },
                 {
                     {"--coor_url", "coordinator 地址发现入口。通常保持默认即可。"},
                     {"--format", "建议脚本和 agent 使用 `json`。"},
                 },
                 {
                     "当前命令没有业务对象参数；主要用于确认集群入口是否正确。",
                 },
                 {
                     "如果返回连接错误，优先检查 `coor_list` 文件内容或显式传 `--coor_url`。",
                 },
                 {
                     "默认输出表格；`--format json` 时输出统一 envelope。"},
                 {
                     "GetStoreMap",
                     "GetRegionMap",
                 }});

  SetCommandDoc(app.get_subcommand_no_throw("GetStoreMap"),
                {HelpTag::kStable,
                 "查看 store 节点清单、状态与基础元信息。",
                 "当你需要知道当前有哪些 store、节点状态是否正常时使用。",
                 {
                     {"dingodb_cli GetStoreMap", "列出全部 store。"},
                     {"dingodb_cli GetStoreMap --format json", "输出 JSON，便于后续筛选。"},
                 },
                 {
                     {"--coor_url", "coordinator 地址发现入口。"},
                     {"--format", "建议机器读取时使用 `json`。"},
                 },
                 {"该命令依赖 coordinator 可访问。"},
                 {"如果结果为空，先确认集群中确实注册了 store 节点。"},
                 {"输出包含 store 基本信息与状态。"},
                 {"GetCoordinatorMap", "GetRegionMap"}});

  SetCommandDoc(app.get_subcommand_no_throw("GetRegionMap"),
                {HelpTag::kStable,
                 "查看 region 分布；支持按 tenant 过滤。",
                 "当你需要查看 region 清单、核对 tenant 下的 region 分布时使用。",
                 {
                     {"dingodb_cli GetRegionMap", "查看默认 tenant 的 region。"},
                     {"dingodb_cli GetRegionMap --tenant_id 0 --format json", "查询指定 tenant 并输出 JSON。"},
                 },
                 {
                     {"--coor_url", "coordinator 地址发现入口。"},
                     {"--tenant_id", "租户 id；默认 0。只想看某个 tenant 时再传。"},
                 },
                 {"如果不传 `--tenant_id`，默认按 0 处理。"},
                 {"若 region 明显缺失，先检查 tenant_id 是否传对。"},
                 {"查询结果来自 coordinator 侧 region map。"},
                 {"GetCoordinatorMap", "GetStoreMap", "QueryRegion"}});

  SetOptionDescription(app.get_subcommand_no_throw("GetRegionMap"), "--tenant_id",
                       "要查询的 tenant_id。默认 0；只想看某个租户的 region 时再显式传入。");
}

void ApplyMetaDocs(CLI::App& app) {
  SetCommandDoc(app.get_subcommand_no_throw("GetSchemas"),
                {HelpTag::kStable,
                 "列出租户下的 schema。",
                 "当你只知道 tenant，想先发现有哪些 schema 时使用；这是查询表之前的常见第一步。",
                 {
                     {"dingodb_cli GetSchemas", "列出默认 tenant(0) 下的 schema。"},
                     {"dingodb_cli GetSchemas --tenant_id 0 --format json",
                      "输出 JSON，适合后续把 schema_id 喂给别的命令。"},
                 },
                 {
                     {"--tenant_id", "租户 id。默认 0。"},
                     {"--coor_url", "coordinator 地址发现入口。"},
                 },
                 {"如果你的部署只有 root tenant，通常直接用默认值即可。"},
                 {"如果查不到 schema，先确认 tenant_id 是否正确。"},
                 {"结果中会包含 schema id 和 schema 名称。"},
                 {"GetSchema", "GetTableByName", "CreateSchema"}});

  SetCommandDoc(app.get_subcommand_no_throw("GetSchema"),
                {HelpTag::kStable,
                 "按 schema_id 查询 schema 定义。",
                 "当你已经知道 schema_id，需要确认 schema 详情或核对 tenant 归属时使用。",
                 {
                     {"dingodb_cli GetSchema --schema_id 50001", "查询指定 schema。"},
                     {"dingodb_cli GetSchema --tenant_id 0 --schema_id 50001 --format json",
                      "指定 tenant 并输出 JSON。"},
                 },
                 {
                     {"--schema_id", "必填。要查询的 schema id。"},
                     {"--tenant_id", "租户 id；默认 0。"},
                     {"--coor_url", "coordinator 地址发现入口。"},
                 },
                 {"schema_id 是服务端对象 id，不是名称。"},
                 {"若返回未找到，优先核对 schema_id 和 tenant_id。"},
                 {"输出包含 schema 基本信息以及挂载的表 / 索引 id。"},
                 {"GetSchemas", "GetSchemaByName"}});

  SetCommandDoc(app.get_subcommand_no_throw("CreateSchema"),
                {HelpTag::kStable,
                 "在指定 tenant 下创建 schema。",
                 "当你已经有 tenant_id，准备创建新的逻辑数据库/schema 时使用。",
                 {
                     {"dingodb_cli CreateSchema --tenant_id 0 --name APP",
                      "在 tenant 0 下创建名为 APP 的 schema。"},
                     {"dingodb_cli CreateSchema --tenant_id 0 --name APP --format json",
                      "如果你的自动化流程需要结构化结果，添加 `--format json`。"},
                 },
                 {
                     {"--tenant_id", "必填。schema 将创建到哪个 tenant。"},
                     {"--name", "必填。schema 名称。"},
                     {"--coor_url", "coordinator 地址发现入口。"},
                 },
                 {"命令会直接调用 meta service 创建 schema，不会先做交互确认。"},
                 {"如果名称已存在，服务端会返回重复创建错误。"},
                 {"成功时输出简单成功信息；JSON 模式下输出统一 envelope。"},
                 {"GetSchemas", "GetSchema"}});

  SetCommandDoc(app.get_subcommand_no_throw("GetTable"),
                {HelpTag::kStable,
                 "按 table_id 查询表定义与分区信息。",
                 "当你已经知道 table_id，想确认列定义、分区规则、region 范围或索引挂载情况时使用。",
                 {
                     {"dingodb_cli GetTable --id 12345", "查询 table_id=12345 的表定义。"},
                     {"dingodb_cli GetTable --id 12345 --format json", "输出 JSON，便于脚本提取字段。"},
                 },
                 {
                     {"--id", "必填。要查询的 table_id。"},
                     {"--coor_url", "coordinator 地址发现入口。"},
                 },
                 {"该命令面向 table_id；历史上的索引模式开关已不再推荐，也不会改变查询行为。"},
                 {"如果返回未找到，先确认 id 是否真的是 table_id。"},
                 {"输出包含表定义、分区和关联元信息。"},
                 {"GetTableByName", "GetRegionByTable", "WhichRegion"}});

  SetCommandDoc(app.get_subcommand_no_throw("GetTableByName"),
                {HelpTag::kStable,
                 "按 schema_id + 表名查询表定义。",
                 "当你还不知道 table_id，只有 schema_id 和表名时使用；通常这是最容易让 LLM 直接拼出的查表入口。",
                 {
                     {"dingodb_cli GetTableByName --schema_id 50001 --name users",
                      "按名称查询；命令内部会自动把名称转成大写后再查询。"},
                     {"dingodb_cli GetTableByName --schema_id 50001 --name USERS --format json",
                      "直接输出 JSON。"},
                 },
                 {
                     {"--schema_id", "必填。表所在 schema。"},
                     {"--name", "必填。表名；命令会自动转成大写。"},
                     {"--coor_url", "coordinator 地址发现入口。"},
                 },
                 {"如果你的表名是小写，直接传即可；命令内部会自动转大写。"},
                 {"最常见错误是 schema_id 传错，导致同名表查不到。"},
                 {"输出与 `GetTable` 相同，只是查找方式不同。"},
                 {"GetSchemas", "GetTable", "CreateTable"}});

  SetCommandDoc(app.get_subcommand_no_throw("GetRegionByTable"),
                {HelpTag::kStable,
                 "查看表或索引对应的 region 分布。",
                 "当你已经知道 table_id 或 table_name，需要进一步拿到 region 列表时使用。",
                 {
                     {"dingodb_cli GetRegionByTable --table_id 12345",
                      "按 table_id 查询对应 region。"},
                     {"dingodb_cli GetRegionByTable --table_name USERS --schema_id 50001 --format json",
                      "按名字查询并输出 JSON。"},
                 },
                 {
                     {"--table_id", "已知 table_id 时使用；和 `--table_name` 二选一即可。"},
                     {"--table_name", "已知表名时使用。"},
                     {"--schema_id", "按表名查询时通常需要一起提供。"},
                 },
                 {"table_id 与 table_name 任选一种定位方式即可。"},
                 {"如果按表名查不到，先确认 schema_id 和大小写规则。"},
                 {"结果会列出与该表相关的 region。"},
                 {"GetTable", "WhichRegion", "QueryRegion"}});

  SetCommandDoc(app.get_subcommand_no_throw("CreateTable"),
                {HelpTag::kPresetTemplate,
                 "创建一个带固定列模板的测试表。",
                 "当你需要快速生成演示/测试表时使用。它不是通用建表 DSL，因为内部会硬编码 3 个 BIGINT 列和预设分区列。",
                 {
                     {"dingodb_cli CreateTable --name DEMO --schema_id 50001",
                      "按默认引擎和副本数创建一个测试表。"},
                     {"dingodb_cli CreateTable --name DEMO --schema_id 50001 --part_count 4 --replica 3",
                      "调整分区数和副本数；列定义仍然是固定模板。"},
                 },
                 {
                     {"--name", "必填。表名。命令内部会转成大写保存。"},
                     {"--schema_id", "目标 schema；不传时使用默认 schema。"},
                     {"--part_count", "分区数；0 会被当成 1。"},
                     {"--engine", "存储引擎，只支持 `rocksdb` 或 `bdb`。"},
                     {"--with_increment", "为第 1 列打开自增。"},
                 },
                 {
                     "该命令会创建固定 3 列 `BIGINT` 模板表，不支持自定义列定义。",
                     "这是预设模板命令，不是通用 SQL 建表入口。",
                 },
                 {
                     "`--engine` 只能是 `rocksdb` 或 `bdb`。",
                     "如果创建失败，先确认 schema_id 存在、名称未冲突、coordinator 可达。",
                 },
                 {
                     "成功时会打印新 table_id；JSON 模式下输出统一 envelope。"},
                 {
                     "GetSchemas",
                     "GetTableByName",
                     "GetRegionByTable",
                 }});

  SetOptionDescription(app.get_subcommand_no_throw("GetSchemas"), "--tenant_id",
                       "要列出的 tenant_id。默认 0；不知道 tenant 时通常先用默认值。");
  SetOptionDescription(app.get_subcommand_no_throw("GetSchema"), "--tenant_id",
                       "schema 所属 tenant_id。默认 0。");
  SetOptionDescription(app.get_subcommand_no_throw("GetSchema"), "--schema_id", "必填。要查询的 schema_id。");
  SetOptionDescription(app.get_subcommand_no_throw("CreateSchema"), "--name", "必填。新 schema 名称。");
  SetOptionDescription(app.get_subcommand_no_throw("CreateSchema"), "--tenant_id",
                       "必填。新 schema 所属 tenant_id。");
  SetOptionDescription(app.get_subcommand_no_throw("GetTable"), "--id", "必填。要查询的 table_id。");
  SetOptionDescription(app.get_subcommand_no_throw("GetTableByName"), "--schema_id",
                       "必填。表所在 schema_id。");
  SetOptionDescription(app.get_subcommand_no_throw("GetTableByName"), "--name",
                       "必填。表名；命令内部会自动转成大写。");
  SetOptionDescription(app.get_subcommand_no_throw("CreateTable"), "--name",
                       "必填。表名；命令内部会转成大写。");
  SetOptionDescription(app.get_subcommand_no_throw("CreateTable"), "--schema_id",
                       "目标 schema_id；不传时使用默认 schema。");
  SetOptionDescription(app.get_subcommand_no_throw("CreateTable"), "--part_count",
                       "分区数。传 0 时会按 1 处理。");
  SetOptionDescription(app.get_subcommand_no_throw("CreateTable"), "--enable_rocks_engine",
                       "为 true 时使用 rocks engine 风格的 store engine 设置。");
  SetOptionDescription(app.get_subcommand_no_throw("CreateTable"), "--engine",
                       "存储引擎，只支持 `rocksdb` 或 `bdb`。");
  SetOptionDescription(app.get_subcommand_no_throw("CreateTable"), "--replica", "副本数。");
  SetOptionDescription(app.get_subcommand_no_throw("CreateTable"), "--with_increment",
                       "为模板表第 1 列开启自增。");
  SetOptionDescription(app.get_subcommand_no_throw("GetRegionByTable"), "--table_id",
                       "已知 table_id 时使用；和 `--table_name` 二选一即可。");
  SetOptionDescription(app.get_subcommand_no_throw("GetRegionByTable"), "--table_name",
                       "已知表名时使用。");
  SetOptionDescription(app.get_subcommand_no_throw("GetRegionByTable"), "--tenant_id",
                       "租户 id。默认 0。通常只在跨 tenant 查询时显式传入。");
  SetOptionDescription(app.get_subcommand_no_throw("GetRegionByTable"), "--schema_id",
                       "按表名查询时通常需要一起提供的 schema_id。");
}

void ApplyKvDocs(CLI::App& app) {
  SetCommandDoc(app.get_subcommand_no_throw("CoorKvPut"),
                {HelpTag::kStable,
                 "向 coordinator 内置 KV 写入单条记录。",
                 "当你要写 coordinator 侧的小规模配置或测试 key-value 时使用。",
                 {
                     {"dingodb_cli CoorKvPut --key foo --value bar", "写入单条 KV。"},
                     {"dingodb_cli CoorKvPut --key foo --value bar --need_prev_kv true --format json",
                      "如果你关心旧值，可让服务端返回 previous KV。"},
                 },
                 {
                     {"--key", "必填。要写入的 key。"},
                     {"--value", "要写入的 value。"},
                     {"--lease", "可选 lease id。默认 0 表示不绑定 lease。"},
                     {"--need_prev_kv", "为 true 时，让服务端返回旧值。"},
                 },
                 {"这是 coordinator 内置 KV，不是 store region KV。"},
                 {
                     "如果没有绑定 `--lease`，key 会按普通持久 key 写入。",
                     "key/value 都按普通字符串处理；需要二进制请先自行编码。",
                 },
                 {"默认打印 protobuf DebugString；`--format json` 时输出统一 envelope。"},
                 {"CoorKvRange", "CoorKvDeleteRange", "KvPut"}});

  SetCommandDoc(app.get_subcommand_no_throw("CoorKvRange"),
                {HelpTag::kStable,
                 "按单 key、前缀或区间读取 coordinator 内置 KV。",
                 "当你需要验证某个 key 是否存在，或按前缀扫出一批 coordinator KV 时使用。",
                 {
                     {"dingodb_cli CoorKvRange --key foo", "读取单 key 或从 key 开始的范围。"},
                     {"dingodb_cli CoorKvRange --key foo --range_end foo\\xFF --limit 50",
                      "常见的前缀扫描写法。"},
                 },
                 {
                     {"--key", "必填。起始 key。"},
                     {"--range_end", "结束 key；为空时按单 key/默认行为处理。"},
                     {"--limit", "最多返回多少条。旧名 `--sub_rversion` 仍兼容，但语义就是 limit。"},
                     {"--keys_only", "只返回 key，不带 value。"},
                     {"--count_only", "只返回计数。"},
                 },
                 {
                     "该命令的规范参数名是 `--limit`；旧的 `--sub_rversion` 只是兼容别名。",
                 },
                 {
                     "如果扫前缀没有结果，先检查 range_end 是否真的覆盖了你想要的 key 空间。",
                 },
                 {"默认打印 protobuf DebugString；JSON 模式下输出统一 envelope。"},
                 {"CoorKvPut", "CoorKvDeleteRange"}});

  SetCommandDoc(app.get_subcommand_no_throw("CoorKvDeleteRange"),
                {HelpTag::kStable,
                 "删除 coordinator 内置 KV 的单 key 或区间。",
                 "当你需要清理 coordinator 侧测试 key 或一段前缀下的配置时使用。",
                 {
                     {"dingodb_cli CoorKvDeleteRange --key foo", "删除单 key。"},
                     {"dingodb_cli CoorKvDeleteRange --key foo --range_end foo\\xFF --need_prev_kv true",
                      "按前缀删除，并要求返回被删旧值。"},
                 },
                 {
                     {"--key", "必填。起始 key。"},
                     {"--range_end", "结束 key；为空时按单 key 删除。"},
                     {"--need_prev_kv", "为 true 时返回被删除的旧值。"},
                 },
                 {"这是 coordinator 内置 KV，不会操作 store region KV。"},
                 {"如果误删风险较高，先用 `CoorKvRange` 预览命中的 key。"},
                 {"默认打印 protobuf DebugString；JSON 模式下输出统一 envelope。"},
                 {"CoorKvRange", "CoorKvPut"}});

  SetOptionDescription(app.get_subcommand_no_throw("CoorKvPut"), "--key", "必填。要写入的 coordinator KV key。");
  SetOptionDescription(app.get_subcommand_no_throw("CoorKvPut"), "--value", "要写入的字符串 value。");
  SetOptionDescription(app.get_subcommand_no_throw("CoorKvPut"), "--lease",
                       "可选 lease id。默认 0 表示不绑定 lease。");
  SetOptionDescription(app.get_subcommand_no_throw("CoorKvPut"), "--ignore_lease",
                       "为 true 时忽略旧 lease 条件。");
  SetOptionDescription(app.get_subcommand_no_throw("CoorKvPut"), "--ignore_value",
                       "为 true 时忽略旧 value 条件。");
  SetOptionDescription(app.get_subcommand_no_throw("CoorKvPut"), "--need_prev_kv",
                       "为 true 时要求服务端返回旧值。");

  SetOptionDescription(app.get_subcommand_no_throw("CoorKvRange"), "--key", "必填。范围起始 key。");
  SetOptionDescription(app.get_subcommand_no_throw("CoorKvRange"), "--range_end",
                       "结束 key；为空时按单 key 或默认行为读取。");
  SetOptionDescription(app.get_subcommand_no_throw("CoorKvRange"), "--limit",
                       "最多返回多少条记录。规范参数名是 `--limit`；旧名 `--sub_rversion` 仅作兼容。");
  SetOptionDescription(app.get_subcommand_no_throw("CoorKvRange"), "--keys_only",
                       "为 true 时只返回 key，不返回 value。");
  SetOptionDescription(app.get_subcommand_no_throw("CoorKvRange"), "--count_only",
                       "为 true 时只返回命中数量。");

  SetOptionDescription(app.get_subcommand_no_throw("CoorKvDeleteRange"), "--key", "必填。删除起始 key。");
  SetOptionDescription(app.get_subcommand_no_throw("CoorKvDeleteRange"), "--range_end",
                       "结束 key；为空时按单 key 删除。");
  SetOptionDescription(app.get_subcommand_no_throw("CoorKvDeleteRange"), "--need_prev_kv",
                       "为 true 时要求服务端返回被删旧值。");
}

void ApplyStoreDocs(CLI::App& app) {
  SetCommandDoc(app.get_subcommand_no_throw("KvPut"),
                {HelpTag::kStable,
                 "按 region_id + key 向 store 写入一条 KV。",
                 "当你已经知道目标 region_id，想直接向该 region 写入 KV 时使用。",
                 {
                     {"dingodb_cli KvPut --region_id 80001 --key user:1 --value alice",
                      "向指定 region 写入单条 KV。"},
                     {"dingodb_cli KvPut --region_id 80001 --key user:1",
                      "如果省略 `--value`，命令会自动生成一个随机 256 字节 value。"},
                 },
                 {
                     {"--region_id", "必填。目标 region。"},
                     {"--key", "必填。要写入的业务 key。"},
                     {"--value", "要写入的 value；不传时会自动生成随机 256 字节字符串。"},
                     {"--coor_url", "coordinator 地址发现入口，用于把 region_id 路由到 store。"},
                 },
                 {
                     "该命令写的是 store region KV，不是 coordinator 内置 KV。",
                     "如果你还不知道 region_id，先用 `WhichRegion` 或 `GetRegionByTable`。",
                 },
                 {
                     "最常见误用是把 table_id 当成 region_id 传入。",
                     "如果你想写确定值，请显式传 `--value`；否则会生成随机值。",
                 },
                 {"默认输出普通文本；JSON 模式下输出统一 envelope。"},
                 {"KvGet", "WhichRegion", "CoorKvPut"}});

  SetCommandDoc(app.get_subcommand_no_throw("KvGet"),
                {HelpTag::kStable,
                 "按 region_id + key 从 store 读取一条 KV。",
                 "当你已经知道目标 region_id，想确认某个 key 在 store 上的值时使用。",
                 {
                     {"dingodb_cli KvGet --region_id 80001 --key user:1", "读取指定 key。"},
                     {"dingodb_cli KvGet --region_id 80001 --key user:1 --format json",
                      "输出 JSON，便于自动化处理。"},
                 },
                 {
                     {"--region_id", "必填。目标 region。"},
                     {"--key", "必填。要读取的业务 key。"},
                     {"--coor_url", "coordinator 地址发现入口，用于 region 路由。"},
                 },
                 {"不知道 region_id 时先用 `WhichRegion` 或 `GetRegionByTable`。"},
                 {"如果读取不到，先确认 key 的编码方式与写入时一致。"},
                 {"默认输出普通文本；JSON 模式下输出统一 envelope。"},
                 {"KvPut", "WhichRegion", "QueryRegion"}});

  SetCommandDoc(app.get_subcommand_no_throw("WhichRegion"),
                {HelpTag::kStable,
                 "按 table/index id + key 定位目标 region。",
                 "当你有业务 key，但不知道应该落到哪个 region 时使用；这通常是从元数据查询转到 store 查询的桥梁。",
                 {
                     {"dingodb_cli WhichRegion --id 12345 --key user:1",
                      "根据 table_id/index_id 和业务 key 计算所属 region。"},
                 },
                 {
                     {"--id", "必填。table_id 或 index_id，不是 region_id。"},
                     {"--key", "必填。要定位的业务 key，例如主键。"},
                     {"--coor_url", "coordinator 地址发现入口。"},
                 },
                 {
                     "该命令会先读取表/索引元数据，再根据 key 所在范围计算 region。",
                 },
                 {
                     "最常见错误是把 region_id 传给 `--id`。",
                     "如果 key 编码与业务表真实编码规则不一致，定位结果也会不准。",
                 },
                 {"输出里会给出命中的 region 信息；随后通常接 `KvGet`、`KvPut` 或 `QueryRegion`。"},
                 {"GetTable", "GetRegionByTable", "KvGet", "QueryRegion"}});

  SetCommandDoc(app.get_subcommand_no_throw("QueryRegion"),
                {HelpTag::kStable,
                 "按 region_id 查询 region 元信息与副本状态。",
                 "当你已经拿到 region_id，需要进一步确认 leader、副本、范围和状态时使用。",
                 {
                     {"dingodb_cli QueryRegion --id 80001", "查询指定 region。"},
                     {"dingodb_cli QueryRegion --id 80001 --format json", "输出 JSON。"},
                 },
                 {
                     {"--id", "必填。要查询的 region_id。"},
                     {"--coor_url", "coordinator 地址发现入口。"},
                 },
                 {"这是 region 级元数据查询；不读取业务 KV 内容。"},
                 {"如果 region_id 不存在，服务端会返回未找到。"},
                 {"输出通常包含 range、副本、leader 和状态信息。"},
                 {"WhichRegion", "GetRegionMap", "KvGet"}});

  SetOptionDescription(app.get_subcommand_no_throw("KvPut"), "--region_id",
                       "必填。目标 region_id。");
  SetOptionDescription(app.get_subcommand_no_throw("KvPut"), "--key", "必填。要写入的业务 key。");
  SetOptionDescription(app.get_subcommand_no_throw("KvPut"), "--value",
                       "要写入的 value；不传时会自动生成随机 256 字节字符串。");
  SetOptionDescription(app.get_subcommand_no_throw("KvGet"), "--region_id",
                       "必填。目标 region_id。");
  SetOptionDescription(app.get_subcommand_no_throw("KvGet"), "--key", "必填。要读取的业务 key。");
  SetOptionDescription(app.get_subcommand_no_throw("WhichRegion"), "--id",
                       "必填。table_id 或 index_id，不是 region_id。");
  SetOptionDescription(app.get_subcommand_no_throw("WhichRegion"), "--key",
                       "必填。要定位的业务 key，例如主键。");
  SetOptionDescription(app.get_subcommand_no_throw("QueryRegion"), "--id", "必填。要查询的 region_id。");
}

void ApplyShellDoc(CLI::App& app) {
  SetCommandDoc(app.get_subcommand_no_throw("Shell"),
                {HelpTag::kStable,
                 "进入交互式 CLI shell。",
                 "当你需要在 TTY 里反复试命令、快速查看子命令帮助时使用。",
                 {
                     {"dingodb_cli Shell", "进入交互式 shell。"},
                     {"help", "在 shell 中查看根帮助；`help <子命令>` 查看子命令帮助。"},
                 },
                 {},
                 {
                     "只能在交互式 TTY 中运行；重定向输入输出时会报 usage 错误。",
                 },
                 {
                     "如果提示需要交互式 TTY，请回到终端直接运行，不要通过管道或重定向调用。",
                 },
                 {"shell 内执行命令与普通 CLI 共享同一套参数语义。"},
                 {"GetCoordinatorMap", "GetSchemas", "KvPut"}});
}

}  // namespace

void ConfigureHelp(CLI::App& app) {
  ApplyCatalogDescriptions(app);
  ApplyRootHelp(app);
  ApplyDiscoveryDocs(app);
  ApplyMetaDocs(app);
  ApplyKvDocs(app);
  ApplyStoreDocs(app);
  ApplyShellDoc(app);
}

}  // namespace client_v2
