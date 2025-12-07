/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef CORE3D_TEST_PLUGIN_UIDS
#define CORE3D_TEST_PLUGIN_UIDS

#include <atomic>

#include <base/util/uid.h>

#include "intf_test.h"

namespace TestPlugin3D {
constexpr BASE_NS::Uid UID_3D_TEST_PLUGIN { "12345678-3d3d-3d3d-3d3d-deadbeef0020" };

constexpr BASE_NS::Uid UID_3D_GLOBAL_TEST_IMPL { "12345678-3d3d-3d3d-3d3d-deadbeef0021" };
constexpr BASE_NS::Uid UID_3D_CONTEXT_TEST_IMPL { "12345678-3d3d-3d3d-3d3d-deadbeef0022" };

class Test : public ITest {
public:
    Test() = default;

    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override
    {
        if ((uid == ITest::UID) || (uid == IInterface::UID)) {
            return static_cast<const ITest*>(this);
        }
        return nullptr;
    }

    IInterface* GetInterface(const BASE_NS::Uid& uid) override
    {
        if ((uid == ITest::UID) || (uid == IInterface::UID)) {
            return static_cast<ITest*>(this);
        }
        return nullptr;
    }

    void Ref() override
    {
        refcnt_.fetch_add(1, std::memory_order_relaxed);
    }

    void Unref() override
    {
        if (std::atomic_fetch_sub_explicit(&refcnt_, 1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete this;
        }
    }

private:
    std::atomic<int32_t> refcnt_ { 0 };
};

class ContextImpl final : public Test {
public:
    ContextImpl() = default;

    Type GetType() const override
    {
        return Type::LOCAL;
    }
};

class GlobalImpl final : public Test {
public:
    GlobalImpl() = default;

    Type GetType() const override
    {
        return Type::GLOBAL;
    }
};
} // namespace TestPlugin3D
#endif // CORE3D_TEST_PLUGIN_UIDS
