/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef META_BENCHMARK_UTILS_HEADER
#define META_BENCHMARK_UTILS_HEADER

#include <meta/base/namespace.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/object_macros.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IBenchmarkType, "570fda7f-3481-48c8-8839-67738e85f8c9")
META_REGISTER_CLASS(BenchmarkType, "839ddde1-2790-4493-a3c4-7fa49edcbfe4", ObjectCategoryBits::NO_CATEGORY)

namespace benchmarks {

class IBenchmarkType : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IBenchmarkType)
};

enum BenchmarkEnum : uint32_t { BenchmarkEnumA = 1, BenchmarkEnumB = 2 };

void RegisterBenchmarkTypes();
void UnregisterBenchmarkTypes();

IBenchmarkType::Ptr CreateBenchmarkType();

} // namespace benchmarks
META_END_NAMESPACE()

META_TYPE(META_NS::benchmarks::IBenchmarkType::Ptr)
META_TYPE(META_NS::benchmarks::IBenchmarkType::WeakPtr)
META_TYPE(META_NS::benchmarks::BenchmarkEnum)

#endif
