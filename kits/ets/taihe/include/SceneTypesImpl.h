/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_3D_SCENE_TYPES_IMPL_H
#define OHOS_3D_SCENE_TYPES_IMPL_H

#include "SceneTypes.proj.hpp"
#include "SceneTypes.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

#include "Vec2Impl.h"
#include "Vec3Impl.h"

namespace OHOS::Render3D::KITETS {

class GeometryDefinitionImpl {
public:
    GeometryDefinitionImpl()
    {}

    virtual ::SceneTypes::GeometryType getGeometryType() = 0;
};

class CustomGeometryImpl : public GeometryDefinitionImpl {
public:
    CustomGeometryImpl()
    {}

    ~CustomGeometryImpl()
    {}

    ::taihe::optional<::SceneTypes::PrimitiveTopology> getTopology()
    {
        return ::taihe::optional<::SceneTypes::PrimitiveTopology>(std::in_place, topology_);
    }

    void setTopology(::taihe::optional_view<::SceneTypes::PrimitiveTopology> topology)
    {
        if (topology) {
            topology_ = topology.value();
        } else {
            topology_ = SceneTypes::PrimitiveTopology::key_t::TRIANGLE_LIST;
        }
    }

    ::taihe::array<::SceneTypes::Vec3> getVertices()
    {
        return vertices_;
    }

    void setVertices(::taihe::array_view<::SceneTypes::Vec3> vertices)
    {
        vertices_ = vertices;
    }

    ::taihe::optional<::taihe::array<int32_t>> getIndices()
    {
        return ::taihe::optional<::taihe::array<int32_t>>(std::in_place, indices_);
    }

    void setIndices(::taihe::optional_view<::taihe::array<int32_t>> indices)
    {
        if (indices) {
            indices_ = indices.value();
        } else {
            indices_ = {};
        }
    }

    ::taihe::optional<::taihe::array<::SceneTypes::Vec3>> getNormals()
    {
        return ::taihe::optional<::taihe::array<::SceneTypes::Vec3>>(std::in_place, normals_);
    }

    void setNormals(::taihe::optional_view<::taihe::array<::SceneTypes::Vec3>> normals)
    {
        if (normals) {
            normals_ = normals.value();
        } else {
            normals_ = {};
        }
    }

    ::taihe::optional<::taihe::array<::SceneTypes::Vec2>> getUvs()
    {
        return ::taihe::optional<::taihe::array<::SceneTypes::Vec2>>(std::in_place, uvs_);
    }

    void setUvs(::taihe::optional_view<::taihe::array<::SceneTypes::Vec2>> uvs)
    {
        if (uvs) {
            uvs_ = uvs.value();
        } else {
            uvs_ = {};
        }
    }

    ::taihe::optional<::taihe::array<::SceneTypes::Color>> getColors()
    {
        return ::taihe::optional<::taihe::array<::SceneTypes::Color>>(std::in_place, colors_);
    }

    void setColors(::taihe::optional_view<::taihe::array<::SceneTypes::Color>> colors)
    {
        if (colors) {
            colors_ = colors.value();
        } else {
            colors_ = {};
        }
    }

    ::SceneTypes::GeometryType getGeometryType() override
    {
        return ::SceneTypes::GeometryType::key_t::CUSTOM;
    }

private:
    // std::shared_ptr<CustomETS> custom_;
    ::SceneTypes::PrimitiveTopology topology_{::SceneTypes::PrimitiveTopology::key_t::TRIANGLE_LIST};
    ::taihe::array<::SceneTypes::Vec3> vertices_{};
    ::taihe::array<int32_t> indices_{};
    ::taihe::array<::SceneTypes::Vec3> normals_{};
    ::taihe::array<::SceneTypes::Vec2> uvs_{};
    ::taihe::array<::SceneTypes::Color> colors_{};
};

class CubeGeometryImpl : public GeometryDefinitionImpl {
public:
    CubeGeometryImpl()
    {}

    ~CubeGeometryImpl()
    {}

    ::SceneTypes::Vec3 getSize()
    {
        if (size_.is_error()) {
            size_ = taihe::make_holder<Vec3Impl, SceneTypes::Vec3>(BASE_NS::Math::ZERO_VEC3);
        }
        return size_;
    }

    void setSize(::SceneTypes::weak::Vec3 size)
    {
        this->size_ = size;
    }

    ::SceneTypes::GeometryType getGeometryType() override
    {
        return ::SceneTypes::GeometryType::key_t::CUBE;
    }

private:
    ::SceneTypes::Vec3 size_{{nullptr, nullptr}};
};

class PlaneGeometryImpl : public GeometryDefinitionImpl {
public:
    PlaneGeometryImpl()
    {}

    ::SceneTypes::Vec2 getSize()
    {
        if (size_.is_error()) {
            size_ = taihe::make_holder<Vec2Impl, SceneTypes::Vec2>(BASE_NS::Math::ZERO_VEC2);
        }
        return size_;
    }

    void setSize(::SceneTypes::weak::Vec2 size)
    {
        size_ = size;
    }

    ::SceneTypes::GeometryType getGeometryType() override
    {
        return ::SceneTypes::GeometryType::key_t::PLANE;
    }

private:
    ::SceneTypes::Vec2 size_{{nullptr, nullptr}};
};

class SphereGeometryImpl : public GeometryDefinitionImpl {
public:
    SphereGeometryImpl()
    {}

    double getRadius()
    {
        return radius_;
    }

    void setRadius(double radius)
    {
        radius_ = radius;
    }

    int32_t getSegmentCount()
    {
        return segmentCount_;
    }

    void setSegmentCount(int32_t count)
    {
        segmentCount_ = count;
    }

    ::SceneTypes::GeometryType getGeometryType() override
    {
        return ::SceneTypes::GeometryType::key_t::SPHERE;
    }

private:
    float radius_;
    int32_t segmentCount_;
};
} // namespace OHOS::Render3D::KITETS

#endif  // OHOS_3D_SCENE_TYPES_IMPL_H