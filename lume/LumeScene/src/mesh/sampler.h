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

#ifndef SCENE_SRC_MESH_SAMPLER_H
#define SCENE_SRC_MESH_SAMPLER_H

#include <scene/ext/component.h>
#include <scene/interface/intf_texture.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/resource/intf_dynamic_resource.h>

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(Sampler, "29637952-7747-4233-bdbf-16e51405d2e9", META_NS::ObjectCategoryBits::NO_CATEGORY)

class ISamplerInternal : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISamplerInternal, "20f22389-2662-4890-9d40-8e2401748f8e")
public:
    virtual void SetScene(const IInternalScene::Ptr& scene) = 0;
    virtual bool UpdateSamplerFromHandle(
        RENDER_NS::IRenderContext& context, const RENDER_NS::RenderHandleReference& handle) = 0;
    virtual RENDER_NS::RenderHandleReference GetHandleFromSampler(RENDER_NS::IRenderContext& context) const = 0;
};

class Sampler
    : public META_NS::IntroduceInterfaces<META_NS::MetaObject, ISampler, ISamplerInternal, META_NS::IPropertyOwner,
          META_NS::IDynamicResource, META_NS::IMetadataOwner, META_NS::IResetableObject> {
public:
    META_OBJECT(Sampler, ClassId::Sampler, IntroduceInterfaces)
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(ISampler, SamplerFilter, MagFilter)
    META_STATIC_PROPERTY_DATA(ISampler, SamplerFilter, MinFilter)
    META_STATIC_PROPERTY_DATA(ISampler, SamplerFilter, MipMapMode)
    META_STATIC_PROPERTY_DATA(ISampler, SamplerAddressMode, AddressModeU)
    META_STATIC_PROPERTY_DATA(ISampler, SamplerAddressMode, AddressModeV)
    META_STATIC_PROPERTY_DATA(ISampler, SamplerAddressMode, AddressModeW)
    META_STATIC_EVENT_DATA(META_NS::IDynamicResource, META_NS::IOnChanged, OnResourceChanged)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(SamplerFilter, MagFilter)
    META_IMPLEMENT_PROPERTY(SamplerFilter, MinFilter)
    META_IMPLEMENT_PROPERTY(SamplerFilter, MipMapMode)
    META_IMPLEMENT_PROPERTY(SamplerAddressMode, AddressModeU)
    META_IMPLEMENT_PROPERTY(SamplerAddressMode, AddressModeV)
    META_IMPLEMENT_PROPERTY(SamplerAddressMode, AddressModeW)

    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnResourceChanged)

public:
    void OnMetadataConstructed(const META_NS::StaticMetadata& m, CORE_NS::IInterface& i) override;
    void OnPropertyChanged(const META_NS::IProperty&) override;

protected: // ISamplerInternal
    void SetScene(const IInternalScene::Ptr& scene) override;
    bool UpdateSamplerFromHandle(
        RENDER_NS::IRenderContext& context, const RENDER_NS::RenderHandleReference& handle) override;
    RENDER_NS::RenderHandleReference GetHandleFromSampler(RENDER_NS::IRenderContext& context) const override;

protected: // IResetableObject
    void ResetObject() override;

private:
    struct SamplerInfo {
        RENDER_NS::RenderHandleReference handle;
        RENDER_NS::GpuSamplerDesc descriptor;
        bool isDefault {};
    };

    const META_NS::IProperty* GetExistingProperty(BASE_NS::string_view name) const;
    META_NS::IProperty* GetExistingProperty(BASE_NS::string_view name);
    RENDER_NS::RenderHandleReference GetSamplerHandle(const Sampler::SamplerInfo& info) const noexcept;
    SamplerInfo GetDefaultSampler() const;
    mutable SamplerInfo sampler_;
    IInternalScene::WeakPtr scene_;

private: // Transaction related
    bool CanNotifyChanged();
    void NotifyChanged();
    void StartTransaction();
    void EndTransaction(bool changed);
    mutable CORE_NS::Mutex transaction_;
    uint32_t setting_ {};
    bool changedDuringTransaction_ {};
};

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_MESH_SAMPLER_H
