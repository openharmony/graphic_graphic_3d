/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef RENDER_DATA_STORE_RENDER_DATA_STORE_FULLSCREEN_SHADER_H
#define RENDER_DATA_STORE_RENDER_DATA_STORE_FULLSCREEN_SHADER_H

#include <cstdint>
#include <mutex>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store_fullscreen_shader.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
/**
 * RenderDataStoreFullscreenShader implementation.
 */
class RenderDataStoreFullscreenShader final : public IRenderDataStoreFullscreenShader {
public:
    RenderDataStoreFullscreenShader(const IRenderContext& renderContex, const BASE_NS::string_view name);
    ~RenderDataStoreFullscreenShader() override;

    void PreRender() override {};
    void PreRenderBackend() override {};
    void PostRender() override {};
    void Clear() override {};

    IShaderPipelineBinder::Ptr Create(const BASE_NS::string_view name, const RenderHandleReference& shader) override;
    IShaderPipelineBinder::Ptr Create(const uint64_t id, const RenderHandleReference& shader) override;

    void Destroy(const BASE_NS::string_view name) override;
    void Destroy(const uint64_t id) override;

    IShaderPipelineBinder::Ptr Get(const BASE_NS::string_view name) const override;
    IShaderPipelineBinder::Ptr Get(const uint64_t id) const override;

    // for plugin / factory interface
    static constexpr const char* const TYPE_NAME = "RenderDataStoreFullscreenShader";

    static IRenderDataStore* Create(IRenderContext& renderContext, char const* name);
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
    const IRenderContext& renderContext_;
    BASE_NS::string name_;

    BASE_NS::unordered_map<uint64_t, IShaderPipelineBinder::Ptr> idToObject_;
    BASE_NS::unordered_map<BASE_NS::string, IShaderPipelineBinder::Ptr> nameToObject_;

    mutable std::mutex mutex_;
};
RENDER_END_NAMESPACE()

#endif // RENDER_DATA_STORE_RENDER_DATA_STORE_FULLSCREEN_SHADER_H
