/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_LUME_H
#define OHOS_RENDER_3D_LUME_H
#include "lume_common.h"

namespace OHOS::Render3D {
class Lume final : public LumeCommon {
public:
    Lume() = default;
    ~Lume() override;

private:
    CORE_NS::PlatformCreateInfo ToEnginePlatformData(const PlatformData& data) const override;
    void RegisterAssertPath() override;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_LUME_H
