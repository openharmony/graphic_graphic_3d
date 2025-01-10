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

#ifndef CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_LIGHT_H
#define CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_LIGHT_H

#include <atomic>
#include <cstdint>

#include <3d/render/intf_render_data_store_default_light.h>
#include <base/containers/array_view.h>
#include <base/containers/refcnt_ptr.h>
#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
/**
RenderDataStoreDefaultLight implementation.
*/
class RenderDataStoreDefaultLight final : public IRenderDataStoreDefaultLight {
public:
    explicit RenderDataStoreDefaultLight(const BASE_NS::string_view name);
    ~RenderDataStoreDefaultLight() override = default;

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

    // IRenderDataStoreDefaultLight
    void SetShadowTypes(const ShadowTypes& shadowTypes, const uint32_t flags) override;
    ShadowTypes GetShadowTypes() const override;

    void SetShadowQualityResolutions(const ShadowQualityResolutions& resolutions, const uint32_t flags) override;
    BASE_NS::Math::UVec2 GetShadowQualityResolution() const override;

    void AddLight(const RenderLight& light) override;
    BASE_NS::array_view<const RenderLight> GetLights() const override;
    LightCounts GetLightCounts() const override;
    LightingFlags GetLightingFlags() const override;

    // for plugin / factory interface
    static constexpr const char* const TYPE_NAME = "RenderDataStoreDefaultLight";
    static BASE_NS::refcnt_ptr<IRenderDataStore> Create(RENDER_NS::IRenderContext& renderContext, char const* name);

private:
    const BASE_NS::string name_;

    BASE_NS::vector<RenderLight> lights_;

    IRenderDataStoreDefaultLight::LightCounts lightCounts_;
    IRenderDataStoreDefaultLight::ShadowTypes shadowTypes_;
    IRenderDataStoreDefaultLight::ShadowQualityResolutions resolutions_;

    std::atomic_int32_t refcnt_ { 0 };
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_LIGHT_H
