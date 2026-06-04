/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "SceneComponentETS.h"

#include <meta/api/make_callback.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/property/property_events.h>

#include <render/intf_render_context.h>

#include <scene/ext/intf_component.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_scene.h>

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include "ImageETS.h"
#include "SceneResourceETS.h"
#include "Vec2Proxy.h"
#include "Vec3Proxy.h"
#include "Vec4Proxy.h"

namespace OHOS::Render3D {
SceneComponentETS::SceneComponentETS(SCENE_NS::IComponent::Ptr comp, const std::string& name) : comp_(comp), name_(name)
{
    AddProperties();
}

SceneComponentETS::~SceneComponentETS()
{
    keys_.clear();
    proxies_.clear();
    propertyPaths_.clear();
    comp_.reset();
}

std::string SceneComponentETS::GetName()
{
    return name_;
}

void SceneComponentETS::SetName(const std::string& name)
{
    name_ = name;
}

void SceneComponentETS::AddProperties()
{
    auto component = comp_.lock();
    if (!component) {
        return;
    }

    keys_.clear();
    proxies_.clear();
    propertyPaths_.clear();

    // Enumerate property names without creating engine value bridges
    auto props = component->EnumerateProperties();
    for (auto&& prop : props) {
        auto sv = SCENE_NS::PropertyName(prop.name);
        std::string name(sv.data(), sv.size());
        propertyPaths_[name] = BASE_NS::move(prop.name);
        keys_.push_back(std::move(name));
    }
}

std::shared_ptr<IPropertyProxy> SceneComponentETS::GetOrCreateProxy(const std::string& key)
{
    auto proxyIt = proxies_.find(key);
    if (proxyIt != proxies_.end()) {
        return proxyIt->second;
    }

    auto pathIt = propertyPaths_.find(key);
    if (pathIt == propertyPaths_.end()) {
        return {};
    }

    auto component = comp_.lock();
    if (!component) {
        return {};
    }

    auto native = interface_cast<SCENE_NS::IEcsObjectAccess>(component);
    if (!native) {
        return {};
    }

    auto ecsObj = native->GetEcsObject();
    if (!ecsObj) {
        return {};
    }

    auto prop = ecsObj->CreateProperty(pathIt->second).GetResult();
    if (!prop) {
        return {};
    }

    // Add the property to the component's metadata
    if (auto meta = interface_cast<META_NS::IMetadata>(component)) {
        meta->AddProperty(prop);
    }

    auto proxy = PropertyToProxy(prop);
    proxies_[key] = proxy;
    return proxy;
}

std::shared_ptr<IPropertyProxy> SceneComponentETS::GetProperty(const std::string& key)
{
    return GetOrCreateProxy(key);
}

void SceneComponentETS::ClearArrayProperty(const std::string& key)
{
    // NOTE: Currently only supports clearing IImage::Ptr array properties.
    // If other SceneResource array types need to be supported in the future,
    // add corresponding type checks and SetArrayProperty calls here.
    std::shared_ptr<IPropertyProxy> proxy = GetProperty(key);
    if (!proxy) {
        CORE_LOG_E("property [%s] proxy not found", key.c_str());
        return;
    }
    META_NS::IProperty::Ptr prop = proxy->GetPropertyPtr();
    if (!prop) {
        CORE_LOG_E("property [%s] prop is invalid", key.c_str());
        return;
    }
    if (META_NS::IsCompatibleWith<BASE_NS::vector<SCENE_NS::IImage::Ptr>>(prop)) {
        SetArrayProperty(key, BASE_NS::vector<SCENE_NS::IImage::Ptr>());
        return;
    }
    auto any = META_NS::GetInternalAny(prop);
    CORE_LOG_E("property [%s] type [%s] is not supported yet",
        key.c_str(),
        any ? any->GetTypeIdString().c_str() : "<Unknown>");
}

}  // namespace OHOS::Render3D
