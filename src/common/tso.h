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

#ifndef DINGODB_COMMON_TSO_H_
#define DINGODB_COMMON_TSO_H_

#include <cstdint>

namespace dingodb {

// Keep these values in sync with server-side TSO encoding.
constexpr int64_t kBaseTimestampMs = 1577808000000LL;  // 2020-01-01 00:00:00 UTC
constexpr int kLogicalBits = 18;

}  // namespace dingodb

#endif  // DINGODB_COMMON_TSO_H_
