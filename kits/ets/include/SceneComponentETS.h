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

#ifndef OHOS_3D_SCENE_COMPONENT_ETS_H
#define OHOS_3D_SCENE_COMPONENT_ETS_H

#include "PropertyProxy.h"
#include "Utils.h"

namespace OHOS::Render3D {
class SceneComponentETS {
public:
    SceneComponentETS(SCENE_NS::IComponent::Ptr comp, const std::string &name);
    ~SceneComponentETS();

    std::string GetName();
    void SetName(const std::string& name);

    int32_t GetPropertySize() const
    {
        return proxies_.size();
    }

    std::vector<std::string> GetPropertyKeys() const
    {
        return keys_;
    }

    std::shared_ptr<IPropertyProxy> GetProperty(const std::string &key);

    template<typename Type>
    void SetProperty(const std::string &key, const Type &value)
    {
        ProxySetProperty(proxies_[key], value, key);
    }

    template<typename Type>
    void SetArrayProperty(const std::string &key, const BASE_NS::vector<Type> &value)
    {
        ProxySetArrayProperty(proxies_[key], value, key);
    }

protected:
    void AddProperties();

private:
    std::string name_;
    SCENE_NS::IComponent::Ptr comp_;
    std::unordered_map<std::string, std::shared_ptr<IPropertyProxy>> proxies_;
    std::vector<std::string> keys_;
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_SCENE_COMPONENT_ETS_H
