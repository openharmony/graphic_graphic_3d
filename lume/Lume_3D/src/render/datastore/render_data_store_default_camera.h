/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_CAMERA_H
#define CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_CAMERA_H

#include <cstdint>

#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/render_data_defines_3d.h>
#include <base/containers/array_view.h>
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

    void PreRender() override {};
    void PreRenderBackend() override {};
    // Reset and start indexing from the beginning. i.e. frame boundary reset.
    void PostRender() override;
    void Clear() override;

    void AddCamera(const RenderCamera& camera) override;

    BASE_NS::array_view<const RenderCamera> GetCameras() const override;
    RenderCamera GetCamera(const BASE_NS::string_view name) const override;
    RenderCamera GetCamera(const uint64_t id) const override;
    uint32_t GetCameraIndex(const BASE_NS::string_view name) const override;
    uint32_t GetCameraIndex(const uint64_t id) const override;
    uint32_t GetCameraCount() const override;

    // for plugin / factory interface
    static constexpr char const* const TYPE_NAME = "RenderDataStoreDefaultCamera";
    static RENDER_NS::IRenderDataStore* Create(RENDER_NS::IRenderContext& renderContext, char const* name);
    static void Destroy(RENDER_NS::IRenderDataStore* instance);

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
    const BASE_NS::string name_;

    BASE_NS::vector<RenderCamera> cameras_;
    uint32_t nextId { 0u };
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_CAMERA_H
