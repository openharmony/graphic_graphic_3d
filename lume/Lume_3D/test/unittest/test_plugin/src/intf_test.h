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

#ifndef CORE3D_TEST_PLUGIN_INTF_TEST
#define CORE3D_TEST_PLUGIN_INTF_TEST

#include <core/plugin/intf_class_register.h>
#include <core/plugin/intf_interface.h>

namespace TestPlugin3D {
class ITest : public CORE_NS::IInterface {
public:
    static constexpr BASE_NS::Uid UID { "12345678-3d3d-3d3d-3d3d-deadbeef0000" };
    using Ptr = BASE_NS::refcnt_ptr<ITest>;

    enum class Type { LOCAL, GLOBAL };

    virtual Type GetType() const = 0;

protected:
    ITest() = default;
    virtual ~ITest() = default;
};

inline constexpr BASE_NS::string_view GetName(const ITest*)
{
    return "ITest";
}
} // namespace TestPlugin3D
#endif // CORE3D_TEST_PLUGIN_INTF_TEST
