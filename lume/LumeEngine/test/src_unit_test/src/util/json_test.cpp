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

#include <gtest/gtest.h>

#include <base/containers/string.h>
#include <base/math/matrix.h>
#include <core/json/json.h>

#include "loader/json_util.h"
#include "util/frustum_util.h"

#include "TestRunner.h"

using namespace CORE_NS;
using namespace BASE_NS;
using namespace testing;
using namespace testing::ext;

namespace {
struct TestContext {
    std::shared_ptr<ISceneInit> sceneInit_ = nullptr;
    CORE_NS::IEcs::Ptr ecs_;
};
static TestContext g_context;

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
static constexpr BASE_NS::Uid ENGINE_THREAD{"2070e705-d061-40e4-bfb7-90fad2c280af"};
static constexpr BASE_NS::Uid APP_THREAD{"b2e8cef3-453a-4651-b564-5190f8b5190d"};
static constexpr BASE_NS::Uid IO_QUEUE{"be88e9a0-9cd8-45ab-be48-937953dc258f"};
static constexpr BASE_NS::Uid JS_RELEASE_THREAD{"3784fa96-b25b-4e9c-bbf1-e897d36f73af"};

bool SceneDispose(TestContext &context)
{
    context.ecs_ = nullptr;
    context.sceneInit_ = nullptr;
    return true;
}

bool SceneCreate(TestContext &context)
{
    context.sceneInit_ = CreateTestScene();
    context.sceneInit_->LoadPluginsAndInit();
    if (!context.sceneInit_->GetEngineInstance().engine_) {
        WIDGET_LOGE("fail to get engine");
        return false;
    }
    context.ecs_ = context.sceneInit_->GetEngineInstance().engine_->CreateEcs();
    if (!context.ecs_) {
        WIDGET_LOGE("fail to get ecs");
        return false;
    }
    auto factory = GetInstance<ISystemGraphLoaderFactory>(UID_SYSTEM_GRAPH_LOADER);
    auto systemGraphLoader = factory->Create(context.sceneInit_->GetEngineInstance().engine_->GetFileManager());
    systemGraphLoader->Load("rofs3D://systemGraph.json", *(context.ecs_));
    auto& ecs = *(context.ecs_);
    ecs.Initialize();

    using namespace SCENE_NS;
#if SCENE_META_TEST
    auto fun = [&context]() {
        auto &obr = META_NS::GetObjectRegistry();

        context.params_ = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        if (!context.params_) {
            CORE_LOG_E("default obj null");
        }
        context.scene_ =
            interface_pointer_cast<SCENE_NS::IScene>(obr.Create(SCENE_NS::ClassId::Scene, context.params_));
        
        auto onLoaded = META_NS::MakeCallback<META_NS::IOnChanged>([&context]() {
            bool complete = false;
            auto status = context.scene_->Status()->GetValue();
            if (status == SCENE_NS::IScene::SCENE_STATUS_READY) {
                // still in engine thread
                complete = true;
            } else if (status == SCENE_NS::IScene::SCENE_STATUS_LOADING_FAILED) {
                // make sure we don't have anything in result if error
                complete = true;
            }

            if (complete) {
                if (context.scene_) {
                    auto &obr = META_NS::GetObjectRegistry();
                    // make sure we have renderconfig
                    auto rc = context.scene_->RenderConfiguration()->GetValue();
                    if (!rc) {
                        // Create renderconfig
                        rc = obr.Create<SCENE_NS::IRenderConfiguration>(SCENE_NS::ClassId::RenderConfiguration);
                        context.scene_->RenderConfiguration()->SetValue(rc);
                    }

                    interface_cast<IEcsScene>(context.scene_)
                        ->RenderMode()
                        ->SetValue(IEcsScene::RenderMode::RENDER_ALWAYS);
                    auto duh = context.params_->GetArrayPropertyByName<IntfWeakPtr>("Scenes");
                    if (!duh) {
                        return ;
                    }
                    duh->AddValue(interface_pointer_cast<CORE_NS::IInterface>(context.scene_));
                }
            }
        });
        context.scene_->Asynchronous()->SetValue(false);
        context.scene_->Uri()->SetValue("scene://empty");
        return META_NS::IAny::Ptr{};
    };
    // Should it be possible to cancel? (ie. do we need to store the token for something ..)
    META_NS::GetTaskQueueRegistry()
        .GetTaskQueue(ENGINE_THREAD)
        ->AddWaitableTask(META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>(BASE_NS::move(fun)))
        ->Wait();
#endif
    return true;
}
} // namespace

class UtilJsonTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SceneCreate(g_context);
    }
    static void TearDownTestSuite()
    {
        SceneDispose(g_context);
    }
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(UtilJsonTest, JsonUtil, TestSize.Level1)
{
    BASE_NS::string_view str { "{\"number\":1984, \"boolean\": true, \"charstr\": \"abc\"}" };
    const CORE_NS::json::value jsonData = CORE_NS::json::parse(str.data());
    float num = .0f;
    bool boolean = false;
    BASE_NS::basic_string<char> charstr;
    BASE_NS::string_view numEle { "number" };
    BASE_NS::string numError = "numError";
    BASE_NS::string_view boolEle { "boolean" };
    BASE_NS::string boolError = "boolError";
    BASE_NS::string_view strEle { "charstr" };
    BASE_NS::string strError = "strError";
    EXPECT_TRUE(CORE_NS::SafeGetJsonValue(jsonData, numEle, numError, num));
    EXPECT_TRUE(CORE_NS::SafeGetJsonValue(jsonData, boolEle, boolError, boolean));
    EXPECT_TRUE(CORE_NS::SafeGetJsonValue(jsonData, strEle, strError, charstr));

    BASE_NS::string_view arrayStr {
        "[{\"foo\":\"bar\"}, [{\"greeting\":\"Hello World\"}], \"hello\", 123.456, false, true, null]"
    };
    const CORE_NS::json::value arrayJson = CORE_NS::json::parse(arrayStr.data());
    BASE_NS::string arrayCon[2] = { "ele1", "ele2" };
    CORE_NS::from_json(arrayJson, arrayCon);

    constexpr const char* numbers[18] = { "01", "0a", "0.0a", "0.0e", "0.0E-", "-01", "-0a", "-0.0a", "-0.0e", "-0.0E-",
        "1a", "1.0a", "1.0e", "1.0E-", "-1a", "-1.0a", "-1.0e", "-1.0E-" };
    for (const auto str : numbers) {
        auto value = CORE_NS::json::parse(str);
        EXPECT_EQ(sizeof(value.as_number<double>()), 8);
    }
}

HWTEST_F(UtilJsonTest, frustumTest, TestSize.Level1)
{
    CORE_NS::FrustumUtil f;
    const CORE_NS::FrustumUtil conF;
    const float matData[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    BASE_NS::Math::Mat4X4 testMat(matData);
    f.CreateFrustum(testMat);
    conF.CreateFrustum(testMat);

    CORE_NS::Frustum frus;
    EXPECT_TRUE(f.SphereFrustumCollision(frus, { 1, 2, 3 }, 1.0f));

    EXPECT_TRUE(f.GetInterface(CORE_NS::IFrustumUtil::UID));
    EXPECT_TRUE(f.GetInterface(CORE_NS::IInterface::UID));
    EXPECT_FALSE(f.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));
    EXPECT_TRUE(conF.GetInterface(CORE_NS::IFrustumUtil::UID));
    EXPECT_TRUE(conF.GetInterface(CORE_NS::IInterface::UID));
    EXPECT_FALSE(conF.GetInterface(BASE_NS::Uid("12345678-1234-1234-1234-1234567890ab")));

    f.Ref();
    f.Unref();
}
