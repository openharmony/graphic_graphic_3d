
#ifndef SCENE_SRC_RESOURCE_MATERIAL_H
#define SCENE_SRC_RESOURCE_MATERIAL_H

#include <atomic>
#include <scene/ext/ecs_lazy_property.h>
#include <scene/ext/intf_create_entity.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/resource/types.h>

#include <meta/api/property/property_event_handler.h>
#include <meta/api/resource/derived_from_resource.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

#include "../component/material_component.h"
#include "render_resource.h"

SCENE_BEGIN_NAMESPACE()

class Material : public META_NS::IntroduceInterfaces<META_NS::DerivedFromResource, NamedSceneObject, IMaterial,
                     ICreateEntity, META_NS::Resource> {
    META_OBJECT(Material, ClassId::Material, IntroduceInterfaces)

public:
    Material() : Super(ClassId::MaterialResourceTemplate) {}

    bool SetEcsObject(const IEcsObject::Ptr&) override;
    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_FORWARDED_PROPERTY_DATA(IMaterial, MaterialType, Type)
    META_STATIC_FORWARDED_PROPERTY_DATA(IMaterial, float, AlphaCutoff)
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IMaterial, IShader::Ptr, MaterialShader, "MaterialComponent.materialShader")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IMaterial, IShader::Ptr, DepthShader, "MaterialComponent.depthShader")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IMaterial, SCENE_NS::RenderSort, RenderSort, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IMaterial, SCENE_NS::LightingFlags, LightingFlags, "")
    SCENE_STATIC_DYNINIT_ARRAY_PROPERTY_DATA(IMaterial, ITexture::Ptr, Textures, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IMaterial, META_NS::IMetadata::Ptr, CustomProperties, "")
    META_END_STATIC_DATA()

    META_FORWARD_PROPERTY(MaterialType, Type, material_->Type())
    META_FORWARD_PROPERTY(float, AlphaCutoff, material_->AlphaCutoff())
    META_IMPLEMENT_PROPERTY(IShader::Ptr, MaterialShader)
    META_IMPLEMENT_PROPERTY(IShader::Ptr, DepthShader)
    META_IMPLEMENT_PROPERTY(SCENE_NS::RenderSort, RenderSort)
    META_IMPLEMENT_PROPERTY(SCENE_NS::LightingFlags, LightingFlags)
    META_IMPLEMENT_READONLY_ARRAY_PROPERTY(ITexture::Ptr, Textures)
    META_IMPLEMENT_READONLY_PROPERTY(META_NS::IMetadata::Ptr, CustomProperties)

    META_NS::IMetadata::Ptr GetCustomProperties() const override;
    META_NS::IProperty::Ptr GetCustomProperty(BASE_NS::string_view name) const override;

    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;

    CORE_NS::ResourceType GetResourceType() const override
    {
        return ClassId::MaterialResource.Id().ToUid();
    }

    bool SetResource(const CORE_NS::IResource::Ptr& p) override;
    CORE_NS::IResource::Ptr CreateResource() const override;

private:
    Future<bool> UpdateTextures(const META_NS::IProperty::Ptr& p);
    bool ConstructTextures(const META_NS::IProperty::Ptr& p);
    bool UpdateTextureNames(
        const BASE_NS::vector<ITexture::Ptr>& textures, const IInternalMaterial::ActiveTextureSlotInfo& tsi);
    /**
     * @brief Synchronize custom properties from ECS to engine and construct engine values for them.
     * @param synced_values If not null pointer, store the synced engine values here.
     * @return True if succeeded, false otherwise.
     */
    bool SyncCustomProperties(BASE_NS::vector<META_NS::IEngineValue::Ptr>* synced_values) const;
    void CopyTextureData(const META_NS::IProperty::ConstPtr& p);

    Future<bool> UpdateCustoms(const META_NS::IProperty::Ptr& p);
    bool UpdateCustomProperties(const META_NS::IProperty::Ptr&);

private:
    IInternalMaterial::Ptr material_;
    std::atomic<bool> textureSyncScheduled_ {};
    std::atomic<bool> customsSyncScheduled_ {};
    META_NS::PropertyChangedEventHandler shaderChanged_;
};

SCENE_END_NAMESPACE()

#endif
