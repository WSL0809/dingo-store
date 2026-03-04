// Copyright (c) 2023 dingodb.com, Inc. All Rights Reserved
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "client_v2/pretty.h"
#include "proto/common.pb.h"
#include "proto/coordinator.pb.h"
#include "proto/meta.pb.h"
#include "proto/error.pb.h"

namespace client_v2 {

class PrettyTest : public testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

// ==================== ShowError Tests ====================

TEST_F(PrettyTest, ShowError_OkStatus) {
  butil::Status status;
  EXPECT_FALSE(Pretty::ShowError(status));
}

TEST_F(PrettyTest, ShowError_ErrorStatus) {
  butil::Status status(dingodb::pb::error::Errno::EINTERNAL, "test error");
  EXPECT_TRUE(Pretty::ShowError(status));
}

TEST_F(PrettyTest, ShowError_ErrorProto) {
  dingodb::pb::error::Error error;
  error.set_errcode(dingodb::pb::error::Errno::EINTERNAL);
  error.set_errmsg("proto error");
  EXPECT_TRUE(Pretty::ShowError(error));
}

// ==================== Response Tests ====================

TEST_F(PrettyTest, Show_GetCoordinatorMapResponse) {
  dingodb::pb::coordinator::GetCoordinatorMapResponse response;
  auto* leader = response.mutable_leader_location();
  leader->set_host("127.0.0.1");
  leader->set_port(20001);
  
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  // Should not crash
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_GetCoordinatorMapResponse_WithError) {
  dingodb::pb::coordinator::GetCoordinatorMapResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::EINTERNAL);
  error->set_errmsg("coordinator error");
  
  // Error output should not crash
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_GetStoreMapResponse) {
  dingodb::pb::coordinator::GetStoreMapResponse response;
  auto* store_map = response.mutable_storemap();
  auto* store = store_map->add_stores();
  store->set_id(1001);
  store->set_store_type(dingodb::pb::common::STORE_TYPE_STORE);
  store->mutable_server_location()->set_host("192.168.1.1");
  store->mutable_server_location()->set_port(20001);
  store->set_state(dingodb::pb::common::STORE_STATE_ONLINE);
  store->set_in_state(dingodb::pb::common::STORE_IN_STATE_NORMAL);
  store->set_create_timestamp(1234567890);
  store->set_last_seen_timestamp(1234567890);
  
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_QueryRegionResponse) {
  dingodb::pb::coordinator::QueryRegionResponse response;
  auto* region = response.mutable_region();
  region->set_id(2001);
  region->set_epoch(1);
  region->set_region_type(dingodb::pb::common::REGION_TYPE_STORE);
  region->set_state(dingodb::pb::common::REGION_STATE_NORMAL);
  region->set_leader_store_id(1001);
  region->set_create_timestamp(1234567890);
  region->mutable_definition()->mutable_range()->set_start_key("abc");
  region->mutable_definition()->mutable_range()->set_end_key("def");
  region->mutable_definition()->set_table_id(100);
  region->mutable_definition()->set_schema_id(1);
  region->mutable_definition()->set_tenant_id(1);
  
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_CreateIdsResponse) {
  dingodb::pb::coordinator::CreateIdsResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  for (int i = 0; i < 5; ++i) {
    response.add_ids(1000 + i);
  }
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_TenantInfo) {
  std::vector<Pretty::TenantInfo> tenants;
  Pretty::TenantInfo tenant;
  tenant.id = 1;
  tenant.name = "test_tenant";
  tenant.comment = "test comment";
  tenant.create_time = 1234567890;
  tenant.update_time = 1234567890;
  tenants.push_back(tenant);
  
  Pretty::Show(tenants);
  SUCCEED();
}

TEST_F(PrettyTest, Show_GetTenantsResponse) {
  dingodb::pb::meta::GetTenantsResponse response;
  
  auto* tenant = response.add_tenants();
  tenant->set_id(1);
  tenant->set_name("tenant_1");
  tenant->set_comment("test tenant");
  tenant->set_create_timestamp(1234567890);
  tenant->set_update_timestamp(1234567890);
  
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_TxnScanResponse_WithKvs) {
  dingodb::pb::store::TxnScanResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  auto* kv = response.add_kvs();
  kv->set_key("test_key");
  kv->set_value("test_value");
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_TxnScanResponse_WithVectors) {
  dingodb::pb::store::TxnScanResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  auto* vector = response.add_vectors();
  vector->set_vector_id(1);
  vector->mutable_vector()->set_dimension(10);
  vector->mutable_vector()->set_value_type(dingodb::pb::common::ValueType::FLOAT);
  for (int i = 0; i < 10; ++i) {
    vector->mutable_vector()->add_float_values(0.1f * i);
  }
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_TxnScanLockResponse) {
  dingodb::pb::store::TxnScanLockResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  auto* lock = response.add_locks();
  lock->set_primary_lock("primary");
  lock->set_key("lock_key");
  lock->set_lock_ts(1000);
  lock->set_for_update_ts(1000);
  lock->set_lock_ttl(3000);
  lock->set_txn_size(100);
  lock->set_lock_type(dingodb::pb::store::Op::Lock);
  lock->set_min_commit_ts(1000);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_CreateIndexResponse) {
  dingodb::pb::meta::CreateIndexResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  response.mutable_index_id()->set_entity_type(dingodb::pb::meta::EntityType::ENTITY_TYPE_INDEX);
  response.mutable_index_id()->set_parent_entity_id(1);
  response.mutable_index_id()->set_entity_id(100);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_DocumentSearchResponse) {
  dingodb::pb::document::DocumentSearchResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  auto* doc_score = response.add_document_with_scores();
  doc_score->mutable_document_with_id()->set_id(1);
  doc_score->set_score(0.95f);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_DocumentBatchQueryResponse) {
  dingodb::pb::document::DocumentBatchQueryResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  auto* doc = response.add_doucments();
  doc->set_id(1);
  auto* data = doc->mutable_document()->mutable_document_data();
  (*data)["title"].set_string_value("test document");
  (*data)["title"].set_field_type(dingodb::pb::common::ScalarFieldType::STRING);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_RegionVector) {
  std::vector<dingodb::pb::common::Region> regions;
  dingodb::pb::common::Region region;
  region.set_id(1001);
  region.set_epoch(1);
  region.set_region_type(dingodb::pb::common::REGION_TYPE_STORE);
  region.set_state(dingodb::pb::common::REGION_STATE_NORMAL);
  regions.push_back(region);
  
  Pretty::Show(regions);
  SUCCEED();
}

TEST_F(PrettyTest, Show_TsoResponse) {
  dingodb::pb::meta::TsoResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  response.mutable_timestamp_px()->set_physical(1234567890);
  response.mutable_timestamp_px()->set_logical(100);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_GetTablesBySchemaResponse) {
  dingodb::pb::meta::GetTablesBySchemaResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  auto* definition = response.add_table_definition_with_ids();
  definition->set_table_id(100);
  definition->mutable_table_definition()->set_name("test_table");
  definition->mutable_table_definition()->set_table_type(dingodb::pb::meta::TableType::TABLE_TYPE_TRANSACTION);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_GetGCSafePointResponse) {
  dingodb::pb::coordinator::GetGCSafePointResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  response.set_safe_point(1234567890);
  response.set_gc_flag(dingodb::pb::coordinator::GCSafePointMode::MANUAL);
  
  // Add a tenant safe point
  auto* tenant_sp = response.add_tenant_safe_points();
  tenant_sp->set_tenant_id(1);
  tenant_sp->set_safe_point(1234567890);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_GetExecutorMapResponse) {
  dingodb::pb::coordinator::GetExecutorMapResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  auto* executor = response.mutable_executormap()->add_executors();
  executor->set_id(1);
  executor->set_epoch(1);
  executor->set_state(dingodb::pb::common::ExecutorState::EXECUTOR_STATE_NORMAL);
  executor->mutable_server_location()->set_host("127.0.0.1");
  executor->mutable_server_location()->set_port(30001);
  executor->mutable_executor_user()->set_user("test_user");
  executor->set_create_timestamp(1234567890);
  executor->set_last_seen_timestamp(1234567890);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_VectorGetBorderIdResponse) {
  dingodb::pb::index::VectorGetBorderIdResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  response.set_id(100);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_VectorCountResponse) {
  dingodb::pb::index::VectorCountResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  response.set_count(1000);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_VectorCalcDistanceResponse) {
  dingodb::pb::index::VectorCalcDistanceResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  auto* distance = response.add_distances();
  distance->add_distance_values(0.5f);
  distance->add_distance_values(0.3f);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_DumpRegionResponse) {
  dingodb::pb::debug::DumpRegionResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  auto* data = response.mutable_data();
  auto* kv = data->add_kvs();
  kv->set_key("test_key");
  kv->set_value("test_value");
  kv->set_ts(1000);
  kv->set_flag(dingodb::pb::debug::DumpRegionResponse_ValueFlag::DumpRegionResponse_ValueFlag_PUT);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, Show_TotalCount) {
  Pretty::ShowTotalCount(999);
  SUCCEED();
}

// ==================== Edge Case Tests ====================

TEST_F(PrettyTest, EmptyResponse) {
  dingodb::pb::coordinator::GetStoreMapResponse response;
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  // No stores added
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, EmptyTenantList) {
  std::vector<Pretty::TenantInfo> tenants;
  Pretty::Show(tenants);
  SUCCEED();
}

TEST_F(PrettyTest, EmptyRegionVector) {
  std::vector<dingodb::pb::common::Region> regions;
  Pretty::Show(regions);
  SUCCEED();
}

TEST_F(PrettyTest, LargeContent) {
  dingodb::pb::meta::GetTenantsResponse response;
  
  // Add many tenants
  for (int i = 0; i < 100; ++i) {
    auto* tenant = response.add_tenants();
    tenant->set_id(i);
    tenant->set_name("tenant_with_very_long_name_" + std::to_string(i));
    tenant->set_comment("This is a very long comment that tests the table width handling " + std::to_string(i));
    tenant->set_create_timestamp(1234567890);
    tenant->set_update_timestamp(1234567890);
  }
  
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  Pretty::Show(response);
  SUCCEED();
}

TEST_F(PrettyTest, SpecialCharactersInContent) {
  dingodb::pb::meta::GetTenantsResponse response;
  
  auto* tenant = response.add_tenants();
  tenant->set_id(1);
  tenant->set_name("tenant|with|pipes");
  tenant->set_comment("comment\nwith\nnewlines");
  tenant->set_create_timestamp(1234567890);
  tenant->set_update_timestamp(1234567890);
  
  auto* error = response.mutable_error();
  error->set_errcode(dingodb::pb::error::Errno::OK);
  
  Pretty::Show(response);
  SUCCEED();
}

}  // namespace client_v2
