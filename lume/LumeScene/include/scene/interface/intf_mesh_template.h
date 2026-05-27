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

#ifndef SCENE_INTERFACE_IMESH_TEMPLATE_H
#define SCENE_INTERFACE_IMESH_TEMPLATE_H

#include <scene/interface/mesh_descriptor.h>

#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Interface for populating a MeshTemplate with descriptor and binary
 *        data. Used by the JSON importer; the live mesh build runs later when
 *        the resource manager applies the template to a Scene::Mesh.
 */
class IMeshTemplate : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMeshTemplate, "9e2b417d-3c08-4f6a-8521-c4d1f70a8b62")
public:
    /// Set the mesh descriptor data (submeshes and vertex attributes).
    virtual void SetDescriptor(
        BASE_NS::vector<SubmeshDesc> submeshes, BASE_NS::vector<VertexAttribute> vertexAttributes) = 0;

    /// Set the raw binary buffer data backing the mesh's vertex/index regions.
    virtual void SetBinaryData(BASE_NS::vector<uint8_t> data) = 0;

    /// Read access to the descriptor and binary data. Used by ApplyOptions to
    /// resolve descriptor data from a base template chained via derivedFrom.
    virtual const BASE_NS::vector<SubmeshDesc>& GetSubmeshes() const = 0;
    virtual const BASE_NS::vector<VertexAttribute>& GetVertexAttributes() const = 0;
    virtual const BASE_NS::vector<uint8_t>& GetBinaryData() const = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IMeshTemplate)

#endif
