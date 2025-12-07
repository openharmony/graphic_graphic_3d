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

#ifndef CORE_TEST_PLUGIN_INTF_TEST
#define CORE_TEST_PLUGIN_INTF_TEST

#include <atomic>

#include <core/plugin/intf_class_register.h>
#include <core/plugin/intf_interface.h>

namespace UTest {
class ITest : public CORE_NS::IInterface {
public:
    static constexpr BASE_NS::Uid UID { "12345678-1234-1234-1234-deadbeef0000" };
    using Ptr = BASE_NS::refcnt_ptr<ITest>;

    enum class Type { STATIC, SHARED, STATIC_GLOBAL, SHARED_GLOBAL };

    virtual Type GetType() const = 0;

protected:
    ITest() = default;
    virtual ~ITest() = default;
};

inline constexpr const BASE_NS::string_view GetName(const ITest*)
{
    return "ITest";
}

class Test : public ITest {
public:
    Test() = default;

    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override
    {
        if (uid == ITest::UID) {
            return static_cast<const ITest*>(this);
        }
        return nullptr;
    }

    IInterface* GetInterface(const BASE_NS::Uid& uid) override
    {
        if (uid == ITest::UID) {
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
} // namespace UTest
#endif // CORE_TEST_PLUGIN_INTF_TEST