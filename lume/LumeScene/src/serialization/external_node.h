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

#ifndef SCENE_SRC_SERIALIZATION_EXTERNAL_NODE_H
#define SCENE_SRC_SERIALIZATION_EXTERNAL_NODE_H

#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_scene.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/interface/resource/intf_owned_resource_groups.h>

SCENE_BEGIN_NAMESPACE()

struct SceneResourceHelper {
    IScene::Ptr scene;
    IInternalScene::Ptr iScene;
    CORE_NS::IResourceManager::Ptr resources;

    SceneResourceHelper(const INode::WeakPtr& w)
    {
        if (auto p = w.lock()) {
            scene = p->GetScene();
            if (scene) {
                iScene = scene->GetInternalScene();
                if (iScene) {
                    if (auto c = iScene->GetContext()) {
                        resources = c->GetResources();
                    }
                }
            }
        }
    }

    operator bool() const
    {
        return resources != nullptr;
    }
};

class ExternalNode : public META_NS::IntroduceInterfaces<META_NS::BaseObject, IExternalNode, META_NS::IAttachable> {
    META_OBJECT(ExternalNode, ClassId::ExternalNode, IntroduceInterfaces)
public:
    META_NS::ObjectFlagBitsValue GetObjectDefaultFlags() const override
    {
        return META_NS::ObjectFlagBits::NONE;
    }
    CORE_NS::ResourceId GetResourceId() const override
    {
        return id_;
    }
    void SetResourceId(CORE_NS::ResourceId id) override
    {
        id_ = id;
    }
    void SetAddedEntities(BASE_NS::vector<CORE_NS::Entity> ents) override
    {
        entities_ = BASE_NS::move(ents);
    }
    BASE_NS::vector<CORE_NS::Entity> GetAddedEntities() const override
    {
        return entities_;
    }
    void SetSubresourceGroup(BASE_NS::string_view group) override
    {
        group_ = group;
    }
    BASE_NS::string GetSubresourceGroup() const override
    {
        return group_;
    }
    void Activate() override
    {
        if (group_.empty() || resources_.empty()) {
            return;
        }
        if (SceneResourceHelper h { parent_ }) {
            if (auto i = interface_cast<META_NS::IOwnedResourceGroups>(h.resources)) {
                auto bundle = h.iScene->GetResourceGroups();
                if (!bundle.GetHandle(group_)) {
                    auto handle = i->GetGroupHandle(group_);
                    bundle.PushGroupHandleToBack(handle);
                    h.iScene->SetResourceGroups(BASE_NS::move(bundle));
                }
            }
            size_t index = 0;
            for (auto&& v : resources_) {
                if (index < resourcePaths_.size()) {
                    h.resources->AddResource(v, resourcePaths_[index++]);
                }
            }
        }
    }
    void Deactivate() override
    {
        if (group_.empty()) {
            return;
        }
        if (SceneResourceHelper h { parent_ }) {
            resources_ = h.resources->GetResources(group_, h.scene);
            resourcePaths_.clear();
            for (auto&& r : resources_) {
                if (auto info = h.resources->GetResourceInfo(r->GetResourceId()); info.id.IsValid()) {
                    resourcePaths_.push_back(info.path);
                }
            }
            auto bundle = h.iScene->GetResourceGroups();
            bundle.RemoveHandle(group_);
            h.iScene->SetResourceGroups(BASE_NS::move(bundle));
        }
    }

    bool Attaching(const META_NS::IAttach::Ptr& target, const IObject::Ptr& dataContext) override
    {
        auto res = interface_pointer_cast<INode>(target);
        parent_ = res;
        return res != nullptr;
    }
    bool Detaching(const META_NS::IAttach::Ptr& target) override
    {
        parent_ = nullptr;
        return true;
    }

private:
    CORE_NS::ResourceId id_;
    BASE_NS::vector<CORE_NS::Entity> entities_;
    INode::WeakPtr parent_;
    BASE_NS::vector<CORE_NS::IResource::Ptr> resources_;
    BASE_NS::vector<BASE_NS::string> resourcePaths_;
    BASE_NS::string group_;
};

SCENE_END_NAMESPACE()

#endif
