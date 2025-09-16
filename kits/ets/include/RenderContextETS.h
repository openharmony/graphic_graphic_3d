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

#ifndef OHOS_3D_RENDER_CONTEXT_ETS_H
#define OHOS_3D_RENDER_CONTEXT_ETS_H

#include "ImageETS.h"
#include "RenderResourcesETS.h"
#include "ShaderETS.h"
#include "Utils.h"

#include <scene/interface/resource/intf_render_resource_manager.h>

namespace OHOS::Render3D {
class RenderContextETS {
public:
    ~RenderContextETS();

    static RenderContextETS& GetInstance()
    {
        static RenderContextETS instance;
        return instance;
    }

    std::shared_ptr<RenderResourcesETS> GetResources();

    bool LoadPlugin(const std::string &name);

    bool RegisterResourcePath(const std::string &protocol, const std::string &uri);

    InvokeReturn<std::shared_ptr<ShaderETS>> CreateShader(const std::string &name, const std::string &uri);
    std::shared_ptr<ImageETS> CreateImage(const std::string &name, const std::string &uri);

private:
    RenderContextETS();

    std::weak_ptr<RenderResourcesETS> resources_;
    SCENE_NS::IRenderResourceManager::Ptr renderResourceManager_;
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_RENDER_CONTEXT_ETS_H
