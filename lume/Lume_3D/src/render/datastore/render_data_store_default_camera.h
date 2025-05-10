/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_CAMERA_H
#define CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_CAMERA_H

#include <atomic>
#include <cstdint>

#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/render_data_defines_3d.h>
#include <base/containers/array_view.h>
#include <base/containers/refcnt_ptr.h>
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
RenderDataStoreDefaultCamera implementation.
*/
class RenderDataStoreDefaultCamera final : public IRenderDataStoreDefaultCamera {
public:
    explicit RenderDataStoreDefaultCamera(const BASE_NS::string_view name);
    ~RenderDataStoreDefaultCamera() override = default;

    // IRenderDataStore
    void PreRender() override {}
    // clear in post render
    void PostRender() override;
    void PreRenderBackend() override {}
    void PostRenderBackend() override {}
    void Clear() override;
    uint32_t GetFlags() const override
    {
        return 0;
    }

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

    void Ref() override;
    void Unref() override;
    int32_t GetRefCount() override;

    // IRenderDataStoreDefaultCamera
    void AddCamera(const RenderCamera& camera) override;
    void AddEnvironment(const RenderCamera::Environment& environment) override;

    BASE_NS::array_view<const RenderCamera> GetCameras() const override;
    RenderCamera GetCamera(const BASE_NS::string_view name) const override;
    RenderCamera GetCamera(const uint64_t id) const override;
    uint32_t GetCameraIndex(const BASE_NS::string_view name) const override;
    uint32_t GetCameraIndex(const uint64_t id) const override;
    uint32_t GetCameraCount() const override;

    BASE_NS::array_view<const RenderCamera::Environment> GetEnvironments() const override;
    RenderCamera::Environment GetEnvironment(const uint64_t id) const override;
    uint32_t GetEnvironmentCount() const override;
    bool HasBlendEnvironments() const override;
    uint32_t GetEnvironmentIndex(const uint64_t id) const override;

    // for plugin / factory interface
    static constexpr const char* const TYPE_NAME = "RenderDataStoreDefaultCamera";
    static BASE_NS::refcnt_ptr<IRenderDataStore> Create(RENDER_NS::IRenderContext& renderContext, char const* name);

private:
    const BASE_NS::string name_;

    BASE_NS::vector<RenderCamera> cameras_;
    BASE_NS::vector<RenderCamera::Environment> environments_;
    bool hasBlendEnvironments_ { false };

    std::atomic_int32_t refcnt_ { 0 };
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_CAMERA_H
