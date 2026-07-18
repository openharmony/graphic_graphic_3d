/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_SRC_TEMPLATE_ENVIRONMENT_TEMPLATE_H
#define SCENE_SRC_TEMPLATE_ENVIRONMENT_TEMPLATE_H

#include <scene/interface/intf_environment.h>
#include <scene/interface/resource/types.h>

#include "resource_template_base.h"

SCENE_BEGIN_NAMESPACE()

class EnvironmentTemplate : public META_NS::IntroduceInterfaces<ResourceTemplateBase> {
    META_OBJECT(EnvironmentTemplate, ClassId::EnvironmentTemplate, IntroduceInterfaces)

public:
    CORE_NS::ResourceType GetResourceType() const override
    {
        return ClassId::EnvironmentResourceTemplate.Id().ToUid();
    }

    META_NS::ObjectId GetTemplateClassId() const override
    {
        return ClassId::EnvironmentTemplate;
    }

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)

protected:
    bool ValidateResourceType(const CORE_NS::IResource& res) const override;
};

SCENE_END_NAMESPACE()

#endif
