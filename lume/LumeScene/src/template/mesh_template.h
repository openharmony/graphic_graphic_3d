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

#ifndef SCENE_SRC_TEMPLATE_MESH_TEMPLATE_H
#define SCENE_SRC_TEMPLATE_MESH_TEMPLATE_H

#include <scene/interface/intf_mesh_template.h>
#include <scene/interface/mesh_descriptor.h>
#include <scene/interface/resource/types.h>

#include "resource_template_base.h"

SCENE_BEGIN_NAMESPACE()

class MeshTemplate : public META_NS::IntroduceInterfaces<ResourceTemplateBase, IMeshTemplate> {
    META_OBJECT(MeshTemplate, ClassId::MeshTemplate, IntroduceInterfaces)

public:
    CORE_NS::ResourceType GetResourceType() const override
    {
        return ClassId::MeshResourceTemplate.Id().ToUid();
    }

    META_NS::ObjectId GetTemplateClassId() const override
    {
        return ClassId::MeshTemplate;
    }

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)

    // IMeshTemplate. Bulk descriptor and binary data live as long as the
    // template does — a caller that wants to free CPU memory drops the template
    // once all derived live meshes have been instantiated.
    void SetDescriptor(
        BASE_NS::vector<SubmeshDesc> submeshes, BASE_NS::vector<VertexAttribute> vertexAttributes) override;
    void SetBinaryData(BASE_NS::vector<uint8_t> data) override;

    const BASE_NS::vector<SubmeshDesc>& GetSubmeshes() const override
    {
        return submeshes_;
    }
    const BASE_NS::vector<VertexAttribute>& GetVertexAttributes() const override
    {
        return vertexAttributes_;
    }
    const BASE_NS::vector<uint8_t>& GetBinaryData() const override
    {
        return binData_;
    }

protected:
    bool ValidateResourceType(const CORE_NS::IResource& res) const override;
    bool ApplyOptions(CORE_NS::IResource& res, const CORE_NS::ResourceContextPtr& context) const override;
    bool ApplyTo(META_NS::IObject& target) const override;

private:
    BASE_NS::vector<SubmeshDesc> submeshes_;
    BASE_NS::vector<VertexAttribute> vertexAttributes_;
    BASE_NS::vector<uint8_t> binData_;
};

SCENE_END_NAMESPACE()

#endif
