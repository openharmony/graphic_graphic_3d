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

#ifndef OHOS_3D_SHADER_ETS_H
#define OHOS_3D_SHADER_ETS_H

#include <unordered_map>

#include <base/containers/unordered_map.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_shader.h>

#include "PropertyProxy.h"
#include "SceneResourceETS.h"

namespace OHOS::Render3D {
class ShaderETS : public SceneResourceETS {
public:
    ShaderETS(const SCENE_NS::IShader::Ptr &shader, const SCENE_NS::IMaterial::Ptr &material);
    ShaderETS(const SCENE_NS::IShader::Ptr &shader, const std::string &name, const std::string &uri);
    ~ShaderETS();

    void BindToMaterial(const SCENE_NS::IMaterial::Ptr &material);

    int32_t GetInputsSize() const
    {
        return proxies_.size();
    }

    std::vector<std::string> GetInputsKeys() const
    {
        return keys_;
    }

    std::shared_ptr<IPropertyProxy> GetInput(const std::string &key);
    void SetInput(const std::string &key, const std::any &value);

    META_NS::IObject::Ptr GetNativeObj() const override;
    SCENE_NS::IShader::Ptr GetNativeShader() const
    {
        return shader_;
    }

private:
    SCENE_NS::IShader::Ptr shader_{nullptr};
    std::unordered_map<std::string, std::shared_ptr<IPropertyProxy>> proxies_;
    std::vector<std::string> keys_;
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_SHADER_ETS_H