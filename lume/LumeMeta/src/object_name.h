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
#ifndef META_SRC_OBJECT_NAME_H
#define META_SRC_OBJECT_NAME_H

#include <meta/ext/base_object.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/serialization/intf_serializable.h>

META_BEGIN_NAMESPACE()
namespace Internal {
class ObjectName : public IntroduceInterfaces<BaseObject, IObjectName, ISerializable> {
    META_OBJECT(ObjectName, META_NS::ClassId::ObjectName, IntroduceInterfaces)
public:
    BASE_NS::string GetName() const override
    {
        return name_;
    }
    bool SetName(BASE_NS::string_view name) override
    {
        name_ = name;
        return true;
    }

    ReturnError Export(IExportContext& c) const override
    {
        return Serializer(c) & NamedValue("Name", name_);
    }
    ReturnError Import(IImportContext& c) override
    {
        return Serializer(c) & NamedValue("Name", name_);
    }

private:
    BASE_NS::string name_;
};
} // namespace Internal

META_END_NAMESPACE()

#endif
