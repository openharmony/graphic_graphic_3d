/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_ENGINE_FACTORY_H
#define OHOS_RENDER_3D_ENGINE_FACTORY_H

#include <memory>

namespace OHOS::Render3D {
class IEngine;
class EngineFactory {
public:
    enum class EngineType {
        LUME,
    };

    static std::unique_ptr<IEngine> CreateEngine(EngineType type);
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_ENGINE_FACTORY_H
