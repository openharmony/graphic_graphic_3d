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


#include "utils.h"

#include <meta/ext/object_fwd.h>
#include <meta/interface/interface_macros.h>

META_BEGIN_NAMESPACE()
namespace benchmarks {
namespace {

class BenchmarkType : public IntroduceInterfaces<ObjectFwd, IBenchmarkType> {
    META_OBJECT(BenchmarkType, ClassId::BenchmarkType, IntroduceInterfaces)
};

} // namespace

void RegisterBenchmarkTypes()
{
    META_NS::RegisterObjectType<BenchmarkType>();

    //    RegisterPropertyType<IBenchmarkType>();
    //    RegisterPropertyType<BenchmarkEnum>();
}

void UnregisterBenchmarkTypes()
{
    //    UnRegisterPropertyType<BenchmarkEnum>();
    //    UnRegisterPropertyType<IBenchmarkType>();

    META_NS::UnregisterObjectType<BenchmarkType>();
}

IBenchmarkType::Ptr CreateBenchmarkType()
{
    return META_NS::GetObjectRegistry().Create<IBenchmarkType>(ClassId::BenchmarkType);
}

} // namespace benchmarks
META_END_NAMESPACE()
