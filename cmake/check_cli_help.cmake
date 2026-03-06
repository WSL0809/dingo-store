if(NOT DEFINED CLI)
  message(FATAL_ERROR "CLI is required")
endif()

if(NOT DEFINED TESTCASE)
  message(FATAL_ERROR "TESTCASE is required")
endif()

function(run_cli out_var)
  execute_process(
    COMMAND ${ARGN}
    OUTPUT_VARIABLE _output
    ERROR_VARIABLE _error
    RESULT_VARIABLE _rv)
  if(NOT _rv EQUAL 0)
    message(FATAL_ERROR "Command failed (${_rv}): ${ARGN}\nstdout:\n${_output}\nstderr:\n${_error}")
  endif()
  set(${out_var} "${_output}${_error}" PARENT_SCOPE)
endfunction()

function(assert_contains text needle)
  string(FIND "${text}" "${needle}" _index)
  if(_index EQUAL -1)
    message(FATAL_ERROR "Expected help output to contain: ${needle}\nActual output:\n${text}")
  endif()
endfunction()

function(assert_not_contains text needle)
  string(FIND "${text}" "${needle}" _index)
  if(NOT _index EQUAL -1)
    message(FATAL_ERROR "Expected help output to NOT contain: ${needle}\nActual output:\n${text}")
  endif()
endfunction()

if(TESTCASE STREQUAL "root")
  run_cli(output ${CLI} --help)
  assert_contains("${output}" "阅读顺序")
  assert_contains("${output}" "常见工作流")
  assert_contains("${output}" "[稳定] 查看 coordinator 节点拓扑与角色分布。")
  assert_contains("${output}" "DeleteExecutor")
  assert_not_contains("${output}" "DeleteExcutor")
elseif(TESTCASE STREQUAL "create_table")
  run_cli(output ${CLI} CreateTable --help)
  assert_contains("${output}" "[预设模板] 创建一个带固定列模板的测试表。")
  assert_contains("${output}" "固定 3 列 `BIGINT` 模板表")
  assert_contains("${output}" "不是通用 SQL 建表入口")
elseif(TESTCASE STREQUAL "kv_put")
  run_cli(output ${CLI} KvPut --help)
  assert_contains("${output}" "随机 256 字节")
  assert_contains("${output}" "该命令写的是 store region KV")
elseif(TESTCASE STREQUAL "get_table_by_name")
  run_cli(output ${CLI} GetTableByName --help)
  assert_contains("${output}" "自动把名称转成大写")
  assert_contains("${output}" "schema_id")
elseif(TESTCASE STREQUAL "coor_kv_range")
  run_cli(output ${CLI} CoorKvRange --help)
  assert_contains("${output}" "--limit,--sub_rversion")
  assert_contains("${output}" "语义就是 limit")
elseif(TESTCASE STREQUAL "get_table")
  run_cli(output ${CLI} GetTable --help)
  assert_not_contains("${output}" "--is_index")
  assert_contains("${output}" "历史上的索引模式开关已不再推荐")
elseif(TESTCASE STREQUAL "delete_executor_alias")
  run_cli(output_new ${CLI} DeleteExecutor --help)
  run_cli(output_old ${CLI} DeleteExcutor --help)
  assert_contains("${output_new}" "删除 executor 节点")
  assert_contains("${output_old}" "删除 executor 节点")
elseif(TESTCASE STREQUAL "no_request_parameter")
  foreach(command_name
          GetCoordinatorMap
          GetStoreMap
          GetRegionMap
          GetSchemas
          GetSchema
          CreateSchema
          GetTable
          GetTableByName
          GetRegionByTable
          CreateTable
          CoorKvPut
          CoorKvRange
          CoorKvDeleteRange
          KvPut
          KvGet
          WhichRegion
          QueryRegion)
    run_cli(command_output ${CLI} ${command_name} --help)
    assert_not_contains("${command_output}" "Request parameter")
  endforeach()
else()
  message(FATAL_ERROR "Unknown TESTCASE=${TESTCASE}")
endif()
