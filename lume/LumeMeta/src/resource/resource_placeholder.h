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

#ifndef META_SRC_RESOURCE_RESOURCE_PLACEHOLDER_H
#define META_SRC_RESOURCE_RESOURCE_PLACEHOLDER_H

#include <meta/ext/implementation_macros.h>
#include <meta/ext/minimal_object.h>
#include <meta/ext/object.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/resource/intf_resource_consumer.h>
#include <meta/interface/serialization/intf_serializable.h>

META_BEGIN_NAMESPACE()

class ResourcePlaceholder : public IntroduceInterfaces<BaseObject, ISerializable, CORE_NS::ISetResourceId> {
    META_OBJECT(ResourcePlaceholder, ClassId::ResourcePlaceholder, IntroduceInterfaces)
public:
    explicit ResourcePlaceholder(CORE_NS::ResourceId id = {}) : rid_(BASE_NS::move(id))
    {}

    ReturnError Export(IExportContext& c) const override
    {
        return Serializer(c) & NamedValue("resourceId.name", rid_.name) & NamedValue("resourceId.group", rid_.group);
    }
    ReturnError Import(IImportContext& c) override
    {
        Serializer s(c);
        if (s & NamedValue("resourceId.name", rid_.name) & NamedValue("resourceId.group", rid_.group)) {
            if (auto ri = interface_cast<IResourceConsumer>(c.Context())) {
                if (auto rman = ri->GetResourceManager()) {
                    if (auto res = rman->GetResource({rid_, c.UserContext()})) {
                        return c.SubstituteThis(interface_pointer_cast<IObject>(res));
                    }
                    // hack; try without context if nothing found with context first
                    if (auto res = rman->GetResource(CORE_NS::ResourceIdContext{rid_})) {
                        return c.SubstituteThis(interface_pointer_cast<IObject>(res));
                    }
                } else {
                    CORE_LOG_W("No resource manager set when importing [%s]", rid_.ToString().c_str());
                }
            }
        }
        CORE_LOG_W("Failed to import resource: %s", rid_.ToString().c_str());
        return GenericError::FAIL;
    }
    void SetResourceId(CORE_NS::ResourceIdContext id) override
    {
        rid_ = BASE_NS::move(id.id);
    }

private:
    CORE_NS::ResourceId rid_;
};

META_END_NAMESPACE()

#endif
