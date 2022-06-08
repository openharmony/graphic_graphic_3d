/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "render_data_store_fullscreen_shader.h"

#include <cinttypes>
#include <cstdint>

#include <base/containers/array_view.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>

#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
RenderDataStoreFullscreenShader::RenderDataStoreFullscreenShader(
    const IRenderContext& renderContext, const string_view name)
    : renderContext_(renderContext), name_(name)
{}

RenderDataStoreFullscreenShader::~RenderDataStoreFullscreenShader() {}

IShaderPipelineBinder::Ptr RenderDataStoreFullscreenShader::Create(
    const BASE_NS::string_view name, const RenderHandleReference& shader)
{
    const auto lock = std::lock_guard(mutex_);

    if (const auto iter = nameToObject_.find(name); iter != nameToObject_.cend()) {
#if (RENDER_VALIDATION_ENABLED)
        PLUGIN_LOG_I("RENDER_VALIDATION: RenderDataStoreFullscreenShader already created with name: (%s)", name.data());
#endif
        return iter->second;
    } else {
        if (shader) {
            IShaderPipelineBinder::Ptr obj =
                renderContext_.GetDevice().GetShaderManager().CreateShaderPipelineBinder(shader);
            nameToObject_[name] = obj;
            return obj;
        }
    }
    return {};
}

IShaderPipelineBinder::Ptr RenderDataStoreFullscreenShader::Create(
    const uint64_t id, const RenderHandleReference& shader)
{
    const auto lock = std::lock_guard(mutex_);

    if (const auto iter = idToObject_.find(id); iter != idToObject_.cend()) {
#if (RENDER_VALIDATION_ENABLED)
        PLUGIN_LOG_I("RENDER_VALIDATION: RenderDataStoreFullscreenShader already created with ID: %" PRIu64 ".", id);
#endif
        return iter->second;
    } else {
        if (shader) {
            IShaderPipelineBinder::Ptr obj =
                renderContext_.GetDevice().GetShaderManager().CreateShaderPipelineBinder(shader);
            idToObject_[id] = obj;
            return obj;
        }
    }
    return {};
}

void RenderDataStoreFullscreenShader::Destroy(const string_view name)
{
    const auto lock = std::lock_guard(mutex_);

    if (auto iter = nameToObject_.find(name); iter != nameToObject_.end()) {
        nameToObject_.erase(iter);
    }
}

void RenderDataStoreFullscreenShader::Destroy(const uint64_t id)
{
    const auto lock = std::lock_guard(mutex_);

    if (auto iter = idToObject_.find(id); iter != idToObject_.end()) {
        idToObject_.erase(iter);
    }
}

IShaderPipelineBinder::Ptr RenderDataStoreFullscreenShader::Get(const string_view name) const
{
    const auto lock = std::lock_guard(mutex_);

    if (const auto iter = nameToObject_.find(name); iter != nameToObject_.cend()) {
        return iter->second;
    } else {
        PLUGIN_LOG_ONCE_W(name_ + "RenderDataStoreFullscreenShaderName",
            "RenderDataStoreFullscreenShader shader material not found (%s)", name.data());
        return {};
    }
}

IShaderPipelineBinder::Ptr RenderDataStoreFullscreenShader::Get(const uint64_t id) const
{
    const auto lock = std::lock_guard(mutex_);

    if (const auto iter = idToObject_.find(id); iter != idToObject_.cend()) {
        return iter->second;
    } else {
        PLUGIN_LOG_ONCE_W(name_ + "RenderDataStoreFullscreenShaderId",
            "RenderDataStoreFullscreenShader shader material not found (%s)", to_string(id).c_str());
        return {};
    }
}

// for plugin / factory interface
IRenderDataStore* RenderDataStoreFullscreenShader::Create(IRenderContext& renderContext, char const* name)
{
    // engine not used
    return new RenderDataStoreFullscreenShader(renderContext, name);
}

void RenderDataStoreFullscreenShader::Destroy(IRenderDataStore* aInstance)
{
    delete static_cast<RenderDataStoreFullscreenShader*>(aInstance);
}
RENDER_END_NAMESPACE()
