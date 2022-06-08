/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */
#ifndef CORE_UTIL_FRUSTUM_UTIL_H
#define CORE_UTIL_FRUSTUM_UTIL_H
#include <core/util/intf_frustum_util.h>

CORE_BEGIN_NAMESPACE()
class FrustumUtil final : public IFrustumUtil {
public:
    Frustum CreateFrustum(const BASE_NS::Math::Mat4X4& matrix) const override;
    bool SphereFrustumCollision(
        const Frustum& frustum, const BASE_NS::Math::Vec3 pos, const float radius) const override;

    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;

    void Ref() override;
    void Unref() override;
};
CORE_END_NAMESPACE()
#endif // CORE_UTIL_FRUSTUM_UTIL_H