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

#ifndef CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_SCENE_H
#define CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_SCENE_H

#include <cstdint>

#include <3d/render/intf_render_data_store_default_scene.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
/**
RenderDataStoreDefaultScene implementation.
*/
class RenderDataStoreDefaultScene final : public IRenderDataStoreDefaultScene {
public:
    explicit RenderDataStoreDefaultScene(const BASE_NS::string_view name);
    ~RenderDataStoreDefaultScene() override = default;

    void CommitFrameData() override {};
    void PreRender() override {};
    // Reset and start indexing from the beginning. i.e. frame boundary reset.
    void PostRender() override;
    void PreRenderBackend() override {};
    void PostRenderBackend() override {};
    void Clear() override;
    uint32_t GetFlags() const override
    {
        return 0;
    };

    void SetScene(const RenderScene& scene) override;
    RenderScene GetScene(const BASE_NS::string_view sceneName) const override;
    RenderScene GetScene() const override;

    // for plugin / factory interface
    static constexpr char const* const TYPE_NAME = "RenderDataStoreDefaultScene";
    static IRenderDataStore* Create(RENDER_NS::IRenderContext& renderContext, char const* name);
    static void Destroy(IRenderDataStore* instance);

    BASE_NS::string_view GetTypeName() const override
    {
        return TYPE_NAME;
    }

    BASE_NS::string_view GetName() const override
    {
        return name_;
    }

    const BASE_NS::Uid& GetUid() const override
    {
        return UID;
    }

private:
    BASE_NS::string name_;

    BASE_NS::vector<RenderScene> scenes_;
    BASE_NS::unordered_map<BASE_NS::string, size_t> nameToScene_;
    uint32_t nextId { 0u };
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_SCENE_H
