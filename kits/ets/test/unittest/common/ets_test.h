/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_3D_ETS_TEST_H
#define OHOS_3D_ETS_TEST_H

#include <gtest/gtest.h>

#include <3d/intf_graphics_context.h>
#include <core/intf_engine.h>
#include <render/intf_render_context.h>
#include <scene/interface/intf_application_context.h>

namespace OHOS::Render3D {

class EtsTest : public ::testing::Test {
public:
    void SetUp() override;
    void TearDown() override;
private:
    void LoadEngineLib();
    void LoadPlugins(const CORE_NS::PlatformCreateInfo &platformCreateInfo);
    void CreateEngine(const CORE_NS::PlatformCreateInfo &platformCreateInfo);
    void CreateRenderContext();
    void CreateGraphicsContext();
    void CreateApplicationContext();

    void *libHandle_;
    CORE_NS::IEngine::Ptr engine_;
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext_;
    CORE3D_NS::IGraphicsContext::Ptr graphicsContext3D_;
    SCENE_NS::IApplicationContext::Ptr applicationContext_;
};

} // namespace OHOS::Render3D

#endif // OHOS_3D_ETS_TEST_H
