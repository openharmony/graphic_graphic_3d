/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "frustum_util.h"

#include <base/math/matrix.h>
#include <base/math/vector.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_plugin_register.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::string_view;
using BASE_NS::Uid;
using BASE_NS::Math::Mat4X4;
using BASE_NS::Math::Vec3;

Frustum FrustumUtil::CreateFrustum(const Mat4X4& matrix) const
{
    Frustum frustum;

    auto& planes = frustum.planes;
    planes[Frustum::PLANE_LEFT].x = matrix[0].w + matrix[0].x;
    planes[Frustum::PLANE_LEFT].y = matrix[1].w + matrix[1].x;
    planes[Frustum::PLANE_LEFT].z = matrix[2].w + matrix[2].x;
    planes[Frustum::PLANE_LEFT].w = matrix[3].w + matrix[3].x;

    planes[Frustum::PLANE_RIGHT].x = matrix[0].w - matrix[0].x;
    planes[Frustum::PLANE_RIGHT].y = matrix[1].w - matrix[1].x;
    planes[Frustum::PLANE_RIGHT].z = matrix[2].w - matrix[2].x;
    planes[Frustum::PLANE_RIGHT].w = matrix[3].w - matrix[3].x;

    planes[Frustum::PLANE_BOTTOM].x = matrix[0].w - matrix[0].y;
    planes[Frustum::PLANE_BOTTOM].y = matrix[1].w - matrix[1].y;
    planes[Frustum::PLANE_BOTTOM].z = matrix[2].w - matrix[2].y;
    planes[Frustum::PLANE_BOTTOM].w = matrix[3].w - matrix[3].y;

    planes[Frustum::PLANE_TOP].x = matrix[0].w + matrix[0].y;
    planes[Frustum::PLANE_TOP].y = matrix[1].w + matrix[1].y;
    planes[Frustum::PLANE_TOP].z = matrix[2].w + matrix[2].y;
    planes[Frustum::PLANE_TOP].w = matrix[3].w + matrix[3].y;

    planes[Frustum::PLANE_NEAR].x = matrix[0].w + matrix[0].z;
    planes[Frustum::PLANE_NEAR].y = matrix[1].w + matrix[1].z;
    planes[Frustum::PLANE_NEAR].z = matrix[2].w + matrix[2].z;
    planes[Frustum::PLANE_NEAR].w = matrix[3].w + matrix[3].z;

    planes[Frustum::PLANE_FAR].x = matrix[0].w - matrix[0].z;
    planes[Frustum::PLANE_FAR].y = matrix[1].w - matrix[1].z;
    planes[Frustum::PLANE_FAR].z = matrix[2].w - matrix[2].z;
    planes[Frustum::PLANE_FAR].w = matrix[3].w - matrix[3].z;

    for (uint32_t idx = 0; idx < Frustum::PLANE_COUNT; ++idx) {
        const float rcpLength = 1.0f / Magnitude(Vec3(planes[idx]));
        planes[idx] *= rcpLength;
    }
    return frustum;
}

bool FrustumUtil::SphereFrustumCollision(const Frustum& frustum, const Vec3 pos, const float radius) const
{
    for (auto const& plane : frustum.planes) {
        const float d = (plane.x * pos.x) + (plane.y * pos.y) + (plane.z * pos.z) + plane.w;
        if (d <= -radius) {
            return false;
        }
    }
    return true;
}

const IInterface* FrustumUtil::GetInterface(const Uid& uid) const
{
    if (uid == IFrustumUtil::UID) {
        return this;
    }
    return nullptr;
}

IInterface* FrustumUtil::GetInterface(const Uid& uid)
{
    if (uid == IFrustumUtil::UID) {
        return this;
    }
    return nullptr;
}

void FrustumUtil::Ref() {}

void FrustumUtil::Unref() {}
CORE_END_NAMESPACE()
