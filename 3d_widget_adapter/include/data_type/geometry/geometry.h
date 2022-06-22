/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_SV_GEOMETRY_H
#define OHOS_RENDER_3D_SV_GEOMETRY_H

#include "../vec3.h"
#include <string>
#include "base/memory/ace_type.h"

namespace OHOS::Render3D {
class SVGeometry : public virtual OHOS::Ace::AceType {
    DECLARE_ACE_TYPE(SVGeometry, OHOS::Ace::AceType)
public:
    SVGeometry(std::string name, Vec3& position, bool castShadows = false, bool receiveShadows = false) :
        name_(name), position_(position), castShadows_(castShadows), receiveShadows_(receiveShadows) {};
    ~SVGeometry() = default;

    virtual int32_t GetType() = 0;

    std::string GetName()
    {
        return name_;
    }

    const Vec3& GetPosition()
    {
        return position_;
    }

    bool CastShadows()
    {
        return castShadows_;
    }

    bool ReceiveShadows()
    {
        return receiveShadows_;
    }

private:
    std::string name_;
    Vec3 position_;
    bool castShadows_;
    bool receiveShadows_;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_SV_GEOMETRY_H
