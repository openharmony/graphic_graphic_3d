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

#ifndef SCENE_INTERFACE_ISCENE_H
#define SCENE_INTERFACE_ISCENE_H

#include <scene/base/types.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_render_configuration.h>

#include <render/intf_render_context.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/animation/intf_animation.h>

SCENE_BEGIN_NAMESPACE()

class IInternalScene;

class IScene : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IScene, "b61d5521-bfa9-4465-9e4d-65d4358ede26")
public:
    META_READONLY_PROPERTY(IRenderConfiguration::Ptr, RenderConfiguration)

public:
    virtual Future<INode::Ptr> GetRootNode() const = 0;
    virtual Future<INode::Ptr> CreateNode(BASE_NS::string_view path, META_NS::ObjectId id = {}) = 0;
    virtual Future<INode::Ptr> FindNode(BASE_NS::string_view path, META_NS::ObjectId id = {}) const = 0;
    virtual Future<bool> ReleaseNode(INode::Ptr&& node, bool recursive) = 0;
    virtual Future<bool> RemoveNode(const INode::Ptr& node) = 0;

    template<class T>
    Future<typename T::Ptr> CreateNode(BASE_NS::string_view path, META_NS::ObjectId id = {})
    {
        return CreateNode(path, id).Then([](INode::Ptr d) { return interface_pointer_cast<T>(d); }, nullptr);
    }

    template<class T>
    Future<typename T::Ptr> FindNode(BASE_NS::string_view path, META_NS::ObjectId id = {}) const
    {
        return FindNode(path, id).Then([](INode::Ptr d) { return interface_pointer_cast<T>(d); }, nullptr);
    }

    virtual Future<META_NS::IObject::Ptr> CreateObject(META_NS::ObjectId id) = 0;

    template<class T>
    Future<typename T::Ptr> CreateObject(META_NS::ObjectId id)
    {
        return CreateObject(id).Then([](META_NS::IObject::Ptr d) { return interface_pointer_cast<T>(d); }, nullptr);
    }

    virtual Future<BASE_NS::vector<ICamera::Ptr>> GetCameras() const = 0;
    virtual Future<BASE_NS::vector<META_NS::IAnimation::Ptr>> GetAnimations() const = 0;

    virtual BASE_NS::shared_ptr<IInternalScene> GetInternalScene() const = 0;

    virtual void StartAutoUpdate(META_NS::TimeSpan interval) = 0;
    virtual Future<bool> SetRenderMode(RenderMode) = 0;
    virtual Future<RenderMode> GetRenderMode() const = 0;
};

META_REGISTER_CLASS(Scene, "ef6321d7-071c-414a-bb3d-55ea6f94688e", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IScene)
META_TYPE(BASE_NS::shared_ptr<RENDER_NS::IRenderContext>)

#endif
