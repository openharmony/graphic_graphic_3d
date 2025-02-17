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

#include "util/linear_allocator.h"

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

class UtilLinearAllocatorTest : public testing::Test {
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

HWTEST_F(UtilLinearAllocatorTest, allocate, TestSize.Level1)
{
    {
        constexpr size_t size = 1024u;
        auto allocator = make_unique<LinearAllocator>(size);

        for (size_t i = 0; i < 10; ++i) {
            auto ptr = allocator->Allocate(16);
            ASSERT_TRUE(ptr);
        }

        auto ptr = allocator->Allocate(size);
        ASSERT_FALSE(ptr);
    }
}

HWTEST_F(UtilLinearAllocatorTest, alignedAllocate, TestSize.Level1)
{
    constexpr size_t size = 1024u;
    auto allocator = make_unique<LinearAllocator>(size);

    auto ptr = (ptrdiff_t)allocator->Allocate(1);
    ASSERT_TRUE(ptr);

    auto ptr2 = (ptrdiff_t)allocator->Allocate(1);
    ASSERT_TRUE(ptr2);
    ASSERT_TRUE((ptr + 1) == ptr2);

    constexpr size_t alignment = 256u;
    auto ptr3 = (ptrdiff_t)allocator->Allocate(1, alignment);
    ASSERT_TRUE(ptr3);
    ASSERT_TRUE((ptr3 & (alignment - 1)) == 0);

    auto ptr4 = (ptrdiff_t)allocator->Allocate(1);
    ASSERT_TRUE(ptr4);

    auto ptr5 = (ptrdiff_t)allocator->Allocate(1, alignment);
    ASSERT_TRUE(ptr5);
    ASSERT_TRUE((ptr5 & (alignment - 1)) == 0);
}

HWTEST_F(UtilLinearAllocatorTest, alignedAllocator, TestSize.Level1)
{
    constexpr size_t size = 1024u;
    constexpr size_t alignment = 4096u;

    auto allocator = make_unique<LinearAllocator>(size, alignment);

    auto ptr = (ptrdiff_t)allocator->Allocate(16);
    ASSERT_TRUE(ptr);
    ASSERT_TRUE((ptr & (alignment - 1)) == 0);
}

HWTEST_F(UtilLinearAllocatorTest, reset, TestSize.Level1)
{
    constexpr size_t size = 1024u;
    auto allocator = make_unique<LinearAllocator>(size);

    constexpr size_t bigAllocation = 1000u;
    auto ptr = allocator->Allocate(bigAllocation);
    ASSERT_TRUE(ptr);

    ASSERT_FALSE(allocator->Allocate(bigAllocation));

    allocator->Reset();

    auto ptr2 = allocator->Allocate(bigAllocation);
    ASSERT_TRUE(ptr2);
    ASSERT_TRUE(ptr == ptr2);
}