/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "engine_factory.h"
#include "lume.h"

namespace OHOS::Render3D {
std::unique_ptr<IEngine> EngineFactory::CreateEngine(EngineType type)
{
    switch (type) {
        case EngineType::LUME:
            return std::make_unique<Lume>();
        default:
            return std::make_unique<Lume>();
    }
}
} // namespace OHOS::Render3D
