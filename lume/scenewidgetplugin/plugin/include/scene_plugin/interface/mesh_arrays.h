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
#ifndef SCENEPLUGIN_MESH_ARRAYS_H
#define SCENEPLUGIN_MESH_ARRAYS_H

SCENE_BEGIN_NAMESPACE()
template<typename IndexType>
struct MeshGeometry {
    using Ptr = BASE_NS::shared_ptr<MeshGeometry<IndexType>>;
    using WeakPtr = BASE_NS::weak_ptr<MeshGeometry<IndexType>>;
    BASE_NS::vector<BASE_NS::Math::Vec3> vertices;
    BASE_NS::vector<BASE_NS::Math::Vec3> normals;  // optional, will be generated if empty
    BASE_NS::vector<BASE_NS::Math::Vec2> uvs;      // optional, will be  generated if empty
    BASE_NS::vector<BASE_NS::Math::Vec2> uv2s;     // optional
    BASE_NS::vector<BASE_NS::Math::Vec4> tangents; // optional, will be created if createTangents is set true
    BASE_NS::vector<BASE_NS::Math::Vec4> colors;   // optional
    BASE_NS::vector<IndexType> indices;
    bool generateTangents { false };

    static Ptr Create()
    {
        return Ptr { new MeshGeometry<IndexType>() };
    }
};

template<typename IndexType>
using MeshGeometryPtr = BASE_NS::shared_ptr<MeshGeometry<IndexType>>;

template<typename IndexType>
using MeshGeometryArray = BASE_NS::vector<MeshGeometryPtr<IndexType>>;

template<typename IndexType>
using MeshGeometryArrayPtr = BASE_NS::shared_ptr<MeshGeometryArray<IndexType>>;

template<typename IndexType>
using MeshGeometryArrayWeakPtr = BASE_NS::shared_ptr<MeshGeometryArray<IndexType>>;

template<typename IndexType>
MeshGeometryArrayPtr<IndexType> CreateMeshContainer(size_t reserve)
{
    auto ret = MeshGeometryArrayPtr<IndexType> { new MeshGeometryArray<IndexType>() };
    ret->reserve(reserve);
    return ret;
}

SCENE_END_NAMESPACE()

#endif // SCENEPLUGIN_MESH_ARRAYS_H
