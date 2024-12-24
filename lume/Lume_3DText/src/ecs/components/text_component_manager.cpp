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

#include <text_3d/ecs/components/text_component.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

CORE_BEGIN_NAMESPACE()
using TEXT3D_NS::TextComponent;

// Extend propertysystem with the types
DECLARE_PROPERTY_TYPE(TextComponent::FontMethod);

// Declare their metadata
BEGIN_ENUM(TextComponentFontMethodMetaData, TextComponent::FontMethod)
DECL_ENUM(TextComponent::FontMethod, RASTER, "Raster Fonts")
DECL_ENUM(TextComponent::FontMethod, SDF, "SDF Fonts")
DECL_ENUM(TextComponent::FontMethod, TEXT3D, "3D Fonts")
END_ENUM(TextComponentFontMethodMetaData, TextComponent::FontMethod)
CORE_END_NAMESPACE()

TEXT3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class TextComponentManager final : public BaseManager<TextComponent, ITextComponentManager> {
    BEGIN_PROPERTY(TextComponent, ComponentMetadata)
#include <text_3d/ecs/components/text_component.h>
    END_PROPERTY();
    const array_view<const Property> componentMetaData_ { ComponentMetadata, countof(ComponentMetadata) };

public:
    explicit TextComponentManager(IEcs& ecs)
        : BaseManager<TextComponent, ITextComponentManager>(ecs, CORE_NS::GetName<TextComponent>())
    {}

    ~TextComponentManager() = default;

    size_t PropertyCount() const override
    {
        return componentMetaData_.size();
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < componentMetaData_.size()) {
            return &componentMetaData_[index];
        }
        return nullptr;
    }

    array_view<const Property> MetaData() const override
    {
        return componentMetaData_;
    }
};

CORE_NS::IComponentManager* ITextComponentManagerInstance(IEcs& ecs)
{
    return new TextComponentManager(ecs);
}

void ITextComponentManagerDestroy(CORE_NS::IComponentManager* instance)
{
    delete static_cast<TextComponentManager*>(instance);
}
TEXT3D_END_NAMESPACE()
