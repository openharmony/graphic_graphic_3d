/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef META_SRC_RESOURCE_TYPES_RESOURCE_TYPE_BASE_H
#define META_SRC_RESOURCE_TYPES_RESOURCE_TYPE_BASE_H

#include <scene/base/namespace.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_render_context.h>
#include <scene/interface/resource/types.h>

#include <core/resources/intf_resource_manager.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/resource/resource.h>

SCENE_BEGIN_NAMESPACE()

class ResourceTypeBase : public META_NS::IntroduceInterfaces<META_NS::BaseObject, CORE_NS::IResourceType> {
    using Super = IntroduceInterfaces;

public:
    explicit ResourceTypeBase(const META_NS::ClassInfo& info) : classInfo_(info) {}

    bool Build(const META_NS::IMetadata::Ptr& d) override
    {
        bool res = Super::Build(d);
        if (res) {
            auto context = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
            if (!context) {
                CORE_LOG_E("Invalid arguments to construct %s-type", GetResourceName().c_str());
                return false;
            }
            context_ = context;
        }
        return res;
    }

    BASE_NS::string GetResourceName() const override
    {
        return BASE_NS::string(classInfo_.Name());
    }
    BASE_NS::Uid GetResourceType() const override
    {
        return classInfo_.Id().ToUid();
    }

protected:
    IRenderContext::WeakPtr context_;
    META_NS::ClassInfo classInfo_;
};

SCENE_END_NAMESPACE()

#endif
