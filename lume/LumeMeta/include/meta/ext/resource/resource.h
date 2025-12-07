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

#ifndef META_EXT_RESOURCE_RESOURCE_H
#define META_EXT_RESOURCE_RESOURCE_H

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/serialization/intf_exporter_state.h>
#include <meta/interface/serialization/intf_serializable.h>

META_BEGIN_NAMESPACE()

class Resource : public IntroduceInterfaces<CORE_NS::IResource, ISerializable, CORE_NS::ISetResourceId> {
public:
    bool SerialiseAsResourceId(IExportContext& c) const
    {
        return c.Context().GetResourceManager() != nullptr;
    }
    ReturnError ExportResourceId(IExportContext& c) const
    {
        ReturnError res = GenericError::FAIL;
        if (resourceId_.IsValid()) {
            if (auto ph = GetObjectRegistry().Create<CORE_NS::ISetResourceId>(ClassId::ResourcePlaceholder)) {
                ph->SetResourceId(resourceId_);
                ISerNode::Ptr node;
                res = c.ExportValueToNode(interface_pointer_cast<IObject>(ph), node);
                if (res) {
                    return c.SubstituteThis(node);
                }
            }
        } else {
            CORE_LOG_W("Empty resource id when trying to serialising resource");
        }
        return res;
    }
    ReturnError Export(IExportContext& c) const override
    {
        if (SerialiseAsResourceId(c)) {
            return ExportResourceId(c);
        }
        return Serializer(c) & AutoSerialize();
    }
    ReturnError Import(IImportContext& c) override
    {
        return Serializer(c) & AutoSerialize();
    }
    CORE_NS::ResourceId GetResourceId() const override
    {
        return resourceId_;
    }

    void SetResourceId(CORE_NS::ResourceId id) override
    {
        resourceId_ = BASE_NS::move(id);
    }

private:
    CORE_NS::ResourceId resourceId_;
};

META_END_NAMESPACE()

#endif
