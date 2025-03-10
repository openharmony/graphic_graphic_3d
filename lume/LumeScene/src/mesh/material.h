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

 #ifndef SCENE_SRC_MESH_MATERIAL_H
 #define SCENE_SRC_MESH_MATERIAL_H
 
 #include <scene/ext/intf_create_entity.h>
 #include <scene/ext/named_scene_object.h>
 #include <scene/interface/intf_material.h>
 
 #include <meta/ext/implementation_macros.h>
 #include <meta/ext/object.h>
 
 #include "component/material_component.h"
 
 SCENE_BEGIN_NAMESPACE()
 
 class Material : public META_NS::IntroduceInterfaces<NamedSceneObject, IMaterial, ICreateEntity> {
     META_OBJECT(Material, ClassId::Material, IntroduceInterfaces)
 
 public:
     bool SetEcsObject(const IEcsObject::Ptr&) override;
     bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;
 
 public:
     META_FORWARD_PROPERTY(MaterialType, Type, material_->Type())
     META_FORWARD_PROPERTY(float, AlphaCutoff, material_->AlphaCutoff())
 
     META_BEGIN_STATIC_DATA()
     SCENE_STATIC_DYNINIT_PROPERTY_DATA(IMaterial, IShader::Ptr, MaterialShader, "MaterialComponent.materialShader")
     SCENE_STATIC_DYNINIT_PROPERTY_DATA(IMaterial, IShader::Ptr, DepthShader, "MaterialComponent.depthShader")
     SCENE_STATIC_DYNINIT_PROPERTY_DATA(IMaterial, SCENE_NS::RenderSort, RenderSort, "")
     SCENE_STATIC_DYNINIT_PROPERTY_DATA(IMaterial, SCENE_NS::LightingFlags, LightingFlags, "")
     SCENE_STATIC_DYNINIT_ARRAY_PROPERTY_DATA(IMaterial, ITexture::Ptr, Textures, "")
     META_END_STATIC_DATA()
 
     META_IMPLEMENT_PROPERTY(IShader::Ptr, MaterialShader)
     META_IMPLEMENT_PROPERTY(IShader::Ptr, DepthShader)
     META_IMPLEMENT_READONLY_ARRAY_PROPERTY(ITexture::Ptr, Textures)
     META_IMPLEMENT_PROPERTY(SCENE_NS::RenderSort, RenderSort)
     META_IMPLEMENT_PROPERTY(SCENE_NS::LightingFlags, LightingFlags)
 
     META_NS::IMetadata::Ptr GetCustomProperties() const override;
     META_NS::IProperty::Ptr GetCustomProperty(BASE_NS::string_view name) const override;
 
     CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;
 
 private:
     bool ConstructTextures(const META_NS::IProperty::Ptr& p);
     /**
      * @brief Synchronize custom properties from ECS to engine and construct engine values for them.
      * @param synced_values If not null pointer, store the synced engine values here.
      * @return True if succeeded, false otherwise.
      */
     bool SyncCustomProperties(BASE_NS::vector<META_NS::IEngineValue::Ptr>* synced_values) const;
 
 private:
     IInternalMaterial::Ptr material_;
 };
 
 SCENE_END_NAMESPACE()
 
 #endif