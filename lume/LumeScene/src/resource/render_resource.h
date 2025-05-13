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

#ifndef SCENE_SRC_RENDER_RESOURCE_H
#define SCENE_SRC_RENDER_RESOURCE_H

#include <scene/ext/intf_render_resource.h>
#include <scene/interface/intf_render_context.h>
#include <shared_mutex>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/resource/resource.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/resource/intf_dynamic_resource.h>

SCENE_BEGIN_NAMESPACE()

class RenderResource : public META_NS::IntroduceInterfaces<META_NS::MetaObject, META_NS::Resource, IRenderResource,
                           META_NS::IDynamicResource> {
    META_OBJECT_NO_CLASSINFO(RenderResource, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_EVENT_DATA(META_NS::IDynamicResource, META_NS::IOnChanged, OnResourceChanged)
    META_END_STATIC_DATA()

    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnResourceChanged)

    bool Build(const META_NS::IMetadata::Ptr&) override;

    RENDER_NS::RenderHandleReference GetRenderHandle() const override;

protected:
    mutable std::shared_mutex mutex_;
    IRenderContext::WeakPtr context_;
    RENDER_NS::RenderHandleReference handle_;
};

SCENE_END_NAMESPACE()

#endif