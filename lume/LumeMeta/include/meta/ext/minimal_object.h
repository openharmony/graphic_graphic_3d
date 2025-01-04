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
#include <meta/interface/intf_static_metadata.h>

META_BEGIN_NAMESPACE()

class MinimalObject : public IntroduceInterfaces<IObject, IObjectFlags, IStaticMetadata> {
public:
    MinimalObject() : flags_(ObjectFlagBits::DEFAULT_FLAGS) {}
    MinimalObject(const ObjectFlagBitsValue& value) : flags_(value) {}

    BASE_NS::string GetName() const override
    {
        return GetClassId().ToString();
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

    // this allows to use the same macros for minimal objects too
    inline static const nullptr_t STATIC_METADATA {};
    static const StaticObjectMetadata* StaticMetadata()
    {
        return nullptr;
    }
    const StaticObjectMetadata* GetStaticMetadata() const override
    {
        return StaticMetadata();
    }

private:
    ObjectFlagBitsValue flags_ {};
};

META_END_NAMESPACE()

#endif
