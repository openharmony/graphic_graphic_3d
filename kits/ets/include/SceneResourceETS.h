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

#ifndef SCENE_RESOURCE_ETS_H
#define SCENE_RESOURCE_ETS_H

#include <string>
#include <base/containers/vector.h>
#include <meta/api/util.h>
#include <scene/interface/intf_scene.h>

namespace OHOS::Render3D {
class SceneResourceETS {
public:
    enum SceneResourceType {
        /**
         * The resource is an Unknow.
         */
        UNKNOWN = 0,
        /**
         * The resource is an Node.
         */
        NODE = 1,
        /**
         * The resource is an Environment.
         */
        ENVIRONMENT = 2,
        /**
         * The resource is a Material.
         */
        MATERIAL = 3,
        /**
         * The resource is a Mesh.
         */
        MESH = 4,
        /**
         * The resource is an Animation.
         */
        ANIMATION = 5,
        /**
         * The resource is a Shader.
         */
        SHADER = 6,
        /**
         * The resource is a Image.
         */
        IMAGE = 7,
        /**
         * The resource is a MeshResource.
         */
        MESH_RESOURCE = 8,
    };

    explicit SceneResourceETS(SceneResourceType type);
    virtual ~SceneResourceETS();

    virtual META_NS::IObject::Ptr GetNativeObj() const = 0;

    SceneResourceType GetResourceType();
    virtual std::string GetName() const;
    void SetName(const std::string &name);
    std::string GetUri() const;

protected:
    void SetUri(const std::string &uri);

private:
    SceneResourceType type_;
    std::string name_; // Cache for name if object does not support naming
    std::string uri_;
};
}  // namespace OHOS::Render3D
#endif  // SCENE_RESOURCE_ETS_H