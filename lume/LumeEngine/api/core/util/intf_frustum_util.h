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

#ifndef API_CORE_UTIL_FRUSTUM_UTIL_H
#define API_CORE_UTIL_FRUSTUM_UTIL_H

#include <base/containers/refcnt_ptr.h>
#include <base/math/vector.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>

BASE_BEGIN_NAMESPACE()
namespace Math {
class Mat4X4;
} // namespace Math
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
/** \addtogroup group_util_frustumutil
 *  @{
 */
/** Frustum */
struct Frustum {
    /** Frustum plane ID's */
    enum FrustumPlaneId {
        /** Left */
        PLANE_LEFT = 0,
        /** Right */
        PLANE_RIGHT = 1,
        /** Bottom */
        PLANE_BOTTOM = 2,
        /** Top */
        PLANE_TOP = 3,
        /** Near */
        PLANE_NEAR = 4,
        /** Far */
        PLANE_FAR = 5,
        /** Count */
        PLANE_COUNT = 6,
    };

    /** Planes vector4 array */
    BASE_NS::Math::Vec4 planes[PLANE_COUNT];
};

class IFrustumUtil : public IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid { "3defee25-af81-4c20-b8d0-e1c3419556b2" };

    using Ptr = BASE_NS::refcnt_ptr<IFrustumUtil>;

    /** Create frustum planes from input matrix.
     * @param matrix Matrix to create planes from. (Usually view-proj matrix)
     * @return Frustum with normalized planes, normals pointing towards the center of the frustum.
     */
    virtual Frustum CreateFrustum(const BASE_NS::Math::Mat4X4& matrix) const = 0;

    /** Test sphere against frustum planes.
     * @param frustum Frustum to test against.
     * @param pos Center position of a bounding sphere.
     * @param radius Radius of a bounding sphere.
     * @return Boolean TRUE if sphere is inside partially or fully. FALSE otherwise.
     */
    virtual bool SphereFrustumCollision(
        const Frustum& frustum, const BASE_NS::Math::Vec3 pos, const float radius) const = 0;

protected:
    IFrustumUtil() = default;
    virtual ~IFrustumUtil() = default;
};

/** @} */
CORE_END_NAMESPACE()

#endif // API_CORE_UTIL_FRUSTUM_UTIL_H
