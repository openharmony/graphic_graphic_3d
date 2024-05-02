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

#ifndef META_EXT_MINIMAL_OBJECT_H
#define META_EXT_MINIMAL_OBJECT_H

#include <base/util/uid_util.h>

#include <meta/base/namespace.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_lifecycle.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_object_flags.h>

META_BEGIN_NAMESPACE()

template<const META_NS::ClassInfo& ClassInfo, typename... Interfaces>
class MinimalObject : public IntroduceInterfaces<IObject, IObjectFlags, Interfaces...> {
public:
    MinimalObject() : flags_(ObjectFlagBits::DEFAULT_FLAGS) {}
    MinimalObject(const ObjectFlagBitsValue& value) : flags_(value) {}

    ObjectId GetClassId() const override
    {
        return ClassInfo.Id();
    }

    BASE_NS::string_view GetClassName() const override
    {
        return ClassInfo.Name();
    }

    BASE_NS::string GetName() const override
    {
        return GetClassId().ToString();
    }

    BASE_NS::vector<BASE_NS::Uid> GetInterfaces() const override
    {
        return IntroduceInterfaces<IObject, IObjectFlags, Interfaces...>::GetInterfacesVector();
    }

    ObjectFlagBitsValue GetObjectFlags() const override
    {
        return flags_;
    }

    void SetObjectFlags(const ObjectFlagBitsValue& value) override
    {
        flags_ = value;
    }

    ObjectFlagBitsValue GetObjectDefaultFlags() const override
    {
        return ObjectFlagBits::DEFAULT_FLAGS;
    }

private:
    ObjectFlagBitsValue flags_ {};
};

META_END_NAMESPACE()

#endif
