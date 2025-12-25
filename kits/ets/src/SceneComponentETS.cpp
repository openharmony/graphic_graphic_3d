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
SceneComponentETS::SceneComponentETS(SCENE_NS::IComponent::Ptr comp, const std::string &name) : comp_(comp), name_(name)
{
    AddProperties();
}

SceneComponentETS::~SceneComponentETS()
{
    keys_.clear();
    proxies_.clear();
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

using GenericComponentMapping = BASE_NS::unordered_map<BASE_NS::string_view, BASE_NS::vector<BASE_NS::string_view>>;

const GenericComponentMapping& GetComponentMapping()
{
    static const GenericComponentMapping map = []() {
        GenericComponentMapping m;
        m["RenderConfigurationComponent"] = { "shadowType", "shadowQuality", "shadowSmoothness", "renderingFlags" };
        return m;
    }();
    return map;
}

bool ShouldExpose(BASE_NS::string_view componentName, BASE_NS::string_view propertyName)
{
    // Weather system uses mixed casing, we cannot exclude properties based on first char
    if (componentName == "WeatherEffectComponent") {
        return true;
    }

    if (!propertyName.empty() && std::isupper(propertyName[0])) {
        // Some specific component wrappers in LumeScene expose ECS properties as LumeScene
        // level properties (whose name starts with an upper case letter). Do not expose those
        // to JS.
        return false;
    }

    const auto& map = GetComponentMapping();
    auto it = map.find(componentName);
    if (it == map.end()) {
        return true; // No mapping defined, expose all
    }
    const auto& mapping = it->second;
    return std::find(mapping.begin(), mapping.end(), propertyName) != mapping.end();
}

void SceneComponentETS::AddProperties()
{
    auto compoment = comp_.lock();
    auto meta = interface_cast<META_NS::IMetadata>(compoment);
    if (!compoment || !meta) {
        return;
    }
    compoment->PopulateAllProperties();
    keys_.clear();
    proxies_.clear();
    BASE_NS::string componentName;
    if (auto o = interface_cast<META_NS::IObject>(compoment)) {
        componentName = o->GetName();
    }
    for (auto&& p : meta->GetProperties()) {
        if (!p) {
            continue;
        }
        const auto name = SCENE_NS::PropertyName(p->GetName());
        if (ShouldExpose(componentName, name)) {
            if (auto proxy = PropertyToProxy(p)) {
                proxies_.insert_or_assign(SCENE_NS::PropertyName(p->GetName()).data(), std::move(proxy));
            }
        }
    }
    keys_.reserve(proxies_.size());
    for (auto& pair : proxies_) {
        keys_.push_back(pair.first);
    }
}

std::shared_ptr<IPropertyProxy> SceneComponentETS::GetProperty(const std::string &key)
{
    return proxies_[key];
}

}  // namespace OHOS::Render3D