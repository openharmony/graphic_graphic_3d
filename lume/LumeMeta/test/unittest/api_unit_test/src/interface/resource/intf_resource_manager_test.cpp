/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include <test_framework.h>

#include <core/resources/intf_resource_manager.h>

#include <meta/api/animation.h>
#include <meta/api/iteration.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_iterable.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/resource/intf_object_resource.h>
#include <meta/interface/resource/intf_object_template.h>
#include <meta/interface/resource/intf_owned_resource_groups.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/resource/intf_resource_query.h>
#include <meta/interface/serialization/intf_refuri_builder.h>

#include "helpers/serialisation_utils.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

META_BEGIN_NAMESPACE()

namespace UTest {

META_REGISTER_CLASS(TestResource, "cfef223f-a9eb-4f19-a60e-b2a3238f5691", META_NS::ObjectCategoryBits::NO_CATEGORY)

class API_FileResourceManagerTest : public ::testing::Test {
public:
    void SetUp() override
    {
        resources = GetObjectRegistry().Create<CORE_NS::IResourceManager>(META_NS::ClassId::FileResourceManager);
        ASSERT_TRUE(resources);

        auto res = GetObjectRegistry().Create<IObjectResource>(META_NS::ClassId::ObjectResourceType);
        ASSERT_TRUE(res);

        res->SetResourceType(ClassId::TestResource);
        ASSERT_TRUE(resources->AddResourceType(interface_pointer_cast<CORE_NS::IResourceType>(res)));

        resources->SetFileManager(CORE_NS::IFileManager::Ptr(&GetTestEnv()->engine->GetFileManager()));
        context = GetObjectRegistry().Create(META_NS::ClassId::Object);
        context2 = GetObjectRegistry().Create(META_NS::ClassId::Object);
    }
    void TearDown() override
    {}

    CORE_NS::IResource::Ptr CreateTestResource(const CORE_NS::ResourceId& id)
    {
        auto res = GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::ObjectResource);
        if (auto i = interface_cast<IObjectResource>(res)) {
            i->SetResourceType(ClassId::TestResource);
        }
        if (auto i = interface_cast<CORE_NS::ISetResourceId>(res)) {
            i->SetResourceId({id, context});
        }
        return res;
    }

    CORE_NS::ResourceContextPtr context;
    CORE_NS::ResourceContextPtr context2;
    CORE_NS::IResourceManager::Ptr resources;
};

struct Listener : CORE_NS::IResourceManager::IResourceListener {
    void OnResourceEvent(CORE_NS::IResourceManager::IResourceListener::EventType type,
        const BASE_NS::array_view<const CORE_NS::ResourceIdContext>& ids) override
    {
        if (type == CORE_NS::IResourceManager::IResourceListener::EventType::ADDED) {
            for (auto&& v : ids) {
                added.push_back(v.id);
            }
        } else {
            for (auto&& v : ids) {
                removed.push_back(v.id);
            }
        }
    }
    BASE_NS::vector<CORE_NS::ResourceId> added;
    BASE_NS::vector<CORE_NS::ResourceId> removed;
};

/**
 * @tc.name: AddWithObject
 * @tc.desc: Tests for Add With Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, AddWithObject, testing::ext::TestSize.Level1)
{
    BASE_NS::shared_ptr<Listener> listener(new Listener);
    resources->AddListener(listener);
    resources->AddListener(listener);  // again, should be a no-op
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    EXPECT_THAT(listener->added, testing::ElementsAre(CORE_NS::ResourceId("app://test.scene")));
    auto queried = resources->GetResource(CORE_NS::ResourceIdContext("app://test.scene", context));
    ASSERT_TRUE(queried);
    EXPECT_EQ(queried, res);
    resources->RemoveListener(listener);
}

/**
 * @tc.name: ResourceTypes
 * @tc.desc: Test resource type APIs
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ResourceTypes, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(resources->GetResourceTypes().empty());
    EXPECT_TRUE(resources->GetResourceGroups(context).empty());
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    EXPECT_FALSE(resources->GetResourceGroups(context).empty());
    EXPECT_TRUE(resources->GetResourceGroups(nullptr).empty());
    EXPECT_TRUE(resources->GetResourceGroups(context2).empty());
}

/**
 * @tc.name: ReloadResource
 * @tc.desc: Test ReloadResource API.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ReloadResource, testing::ext::TestSize.Level1)
{
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    auto groupsBefore = resources->GetResourceGroups(context);
    EXPECT_FALSE(resources->ReloadResource(res));  // ObjectResource fails to reload
    auto groupsAfter = resources->GetResourceGroups(context);
    EXPECT_THAT(groupsBefore, ::testing::ElementsAreArray(groupsAfter));
}

/**
 * @tc.name: PurgeGroup
 * @tc.desc: Test PurgeGroup API.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, PurgeGroup, testing::ext::TestSize.Level1)
{
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    auto groups = resources->GetResourceGroups(context);
    EXPECT_EQ(resources->PurgeGroup("invalid_name", context), 0);
    for (auto&& group : groups) {
        EXPECT_GT(resources->PurgeGroup(group, context), 0);
    }
}

/**
 * @tc.name: ResourceQuery
 * @tc.desc: Test IResourceQuery API.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ResourceQuery, testing::ext::TestSize.Level1)
{
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    auto query = interface_cast<IResourceQuery>(resources);
    ASSERT_TRUE(query);
    auto groups = resources->GetResourceGroups(context);
    for (auto&& group : groups) {
        const auto match = CORE_NS::MatchingResourceId(group);
        auto alive = query->FindAliveResources({match}, context);
        EXPECT_FALSE(alive.empty());
        EXPECT_EQ(query->GetAliveCount({match}, context), alive.size());
    }
}

/**
 * @tc.name: GetResourceHandle
 * @tc.desc: Test GetResourceHandle API.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, GetResourceHandle, testing::ext::TestSize.Level1)
{
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    auto owned = interface_cast<IOwnedResourceGroups>(resources);
    ASSERT_TRUE(owned);
    auto groups = resources->GetResourceGroups(context);
    for (auto&& group : groups) {
        EXPECT_TRUE(owned->GetGroupHandle(group, context));
    }
}

static void MakeEmptyJsonFile()
{
    // make sure we have json file to load
    TestSerialiser ser;
    auto type = GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::ObjectResource);
    if (auto i = interface_cast<IObjectResource>(type)) {
        i->SetResourceType(ClassId::TestResource);
    }
    ASSERT_TRUE(ser.Export(type));
    ser.Dump("app://test.json");
}

/**
 * @tc.name: AddWithoutObject
 * @tc.desc: Tests for Add Without Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, AddWithoutObject, testing::ext::TestSize.Level1)
{
    MakeEmptyJsonFile();
    BASE_NS::shared_ptr<Listener> listener(new Listener);
    resources->AddListener(listener);
    CORE_NS::ResourceIdContext ctx{"test", context};
    ASSERT_TRUE(resources->AddResource(ctx, ClassId::TestResource.Id().ToUid(), "app://test.json"));
    EXPECT_THAT(listener->added, testing::ElementsAre(CORE_NS::ResourceId("test")));
    auto queried = resources->GetResource(ctx);
    ASSERT_TRUE(queried);
}

/**
 * @tc.name: OptionsData
 * @tc.desc: Tests for Options Data. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, OptionsData, testing::ext::TestSize.Level1)
{
    MakeEmptyJsonFile();
    auto opts = GetObjectRegistry().Create<IObjectResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
    ASSERT_TRUE(opts);
    opts->SetProperty("test", uint32_t(2));
    ASSERT_TRUE(resources->AddResource({"test", context},
        ClassId::TestResource.Id().ToUid(),
        "app://test.json",
        interface_pointer_cast<CORE_NS::IResourceOptions>(opts)));
    auto queried = resources->GetResource(CORE_NS::ResourceIdContext("test", context));
    ASSERT_TRUE(queried);
    auto info = resources->GetResourceInfo({"test", context});
    EXPECT_EQ(info.id, "test");
    EXPECT_EQ(info.type, ClassId::TestResource.Id().ToUid());
    EXPECT_EQ(info.path, "app://test.json");
    auto nopts = interface_cast<IObjectResourceOptions>(info.options);
    ASSERT_TRUE(nopts);
    auto p = nopts->GetProperty<uint32_t>("test");
    ASSERT_TRUE(p);
    EXPECT_EQ(p->GetValue(), 2);
}

/**
 * @tc.name: ExportImportAll
 * @tc.desc: Tests for Export Import All. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ExportImportAll, testing::ext::TestSize.Level1)
{
    BASE_NS::vector<CORE_NS::ResourceId> exp;
    for (int i = 0; i != 10; ++i) {
        BASE_NS::string name("test_" + BASE_NS::to_string(i));
        exp.push_back(CORE_NS::ResourceId(name));
        auto res = CreateTestResource(name);
        ASSERT_TRUE(resources->AddResource(res, "app://" + name + ".json"));
    }
    ASSERT_EQ(resources->Export("app://test.resources", context), CORE_NS::IResourceManager::Result::OK);
    resources->RemoveAllResources();
    BASE_NS::shared_ptr<Listener> listener(new Listener);
    resources->AddListener(listener);
    ASSERT_EQ(resources->Import("app://test.resources", context), CORE_NS::IResourceManager::Result::OK);
    auto res = resources->GetResources(context);
    EXPECT_EQ(res.size(), 10);
    EXPECT_THAT(listener->added, testing::UnorderedElementsAreArray(exp));
}

/**
 * @tc.name: ExportImportGroups
 * @tc.desc: Tests for Export Import Groups. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ExportImportGroups, testing::ext::TestSize.Level1)
{
    BASE_NS::vector<BASE_NS::string> groups{"hips", "hops", "some", ""};
    for (auto&& g : groups) {
        for (int i = 0; i != 10; ++i) {
            BASE_NS::string name("test_" + BASE_NS::to_string(i));
            auto res = CreateTestResource(CORE_NS::ResourceId(name, g));
            ASSERT_TRUE(resources->AddResource(res, "app://" + name + ".json"));
        }
    }
    for (auto&& g : groups) {
        ASSERT_EQ(resources->Export("app://test_" + g + ".resources", {CORE_NS::MatchingResourceId(g)}, context),
            CORE_NS::IResourceManager::Result::OK);
    }
    resources->RemoveAllResources();
    int i = 0;
    for (auto&& g : groups) {
        ASSERT_EQ(resources->Import("app://test_" + g + ".resources", context), CORE_NS::IResourceManager::Result::OK);
        auto res = resources->GetResources(context);
        EXPECT_EQ(res.size(), ++i * 10);
        auto gres = resources->GetResources(g, context);
        EXPECT_EQ(gres.size(), 10);
    }

    EXPECT_TRUE(resources->GetResource(CORE_NS::ResourceIdContext("test_1", context)));
    EXPECT_TRUE(resources->GetResource(CORE_NS::ResourceIdContext({"test_1", "hops"}, context)));
    EXPECT_TRUE(resources->GetResource(CORE_NS::ResourceIdContext({"test_1", "some"}, context)));

    resources->RemoveResource(CORE_NS::ResourceIdContext({"test_1", "hops"}, context));
    EXPECT_TRUE(resources->GetResource(CORE_NS::ResourceIdContext("test_1", context)));
    EXPECT_FALSE(resources->GetResource(CORE_NS::ResourceIdContext({"test_1", "hops"}, context)));
    EXPECT_TRUE(resources->GetResource(CORE_NS::ResourceIdContext({"test_1", "some"}, context)));

    resources->RemoveGroup("some", context);
    EXPECT_TRUE(resources->GetResource(CORE_NS::ResourceIdContext("test_1", context)));
    EXPECT_FALSE(resources->GetResource(CORE_NS::ResourceIdContext({"test_1", "hops"}, context)));
    EXPECT_FALSE(resources->GetResource(CORE_NS::ResourceIdContext({"test_1", "some"}, context)));
    EXPECT_EQ(resources->GetResources("some", context).size(), 0);

    EXPECT_EQ(resources->GetResourceInfos("hips", context).size(), 10);
}

/**
 * @tc.name: Rename
 * @tc.desc: Tests for Rename. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, Rename, testing::ext::TestSize.Level1)
{
    BASE_NS::shared_ptr<Listener> listener(new Listener);
    resources->AddListener(listener);
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    EXPECT_THAT(listener->added, testing::ElementsAre(CORE_NS::ResourceId("app://test.scene")));
    {
        auto queried = resources->GetResource(CORE_NS::ResourceIdContext("app://test.scene", context));
        ASSERT_TRUE(queried);
        EXPECT_EQ(queried, res);
    }
    listener->added.clear();
    resources->RenameResource({"app://test.scene", context}, {"ggg", context});
    EXPECT_THAT(listener->removed, testing::ElementsAre(CORE_NS::ResourceId("app://test.scene")));
    EXPECT_THAT(listener->added, testing::ElementsAre(CORE_NS::ResourceId("ggg")));
    {
        EXPECT_FALSE(resources->GetResource({CORE_NS::ResourceId("app://test.scene"), context}));
        auto queried = resources->GetResource(CORE_NS::ResourceIdContext("ggg", context));
        ASSERT_TRUE(queried);
        EXPECT_EQ(queried, res);
    }
}

/**
 * @tc.name: Purge
 * @tc.desc: Tests for Purge. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, Purge, testing::ext::TestSize.Level1)
{
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    {
        auto queried = resources->GetResource(CORE_NS::ResourceIdContext("app://test.scene", context));
        ASSERT_TRUE(queried);
        EXPECT_EQ(queried, res);
    }
    CORE_NS::IResource::WeakPtr w = res;
    res.reset();
    resources->PurgeResource({"app://test.scene", context});
    EXPECT_TRUE(w.expired());
    {
        auto queried = resources->GetResource(CORE_NS::ResourceIdContext("app://test.scene", context));
        ASSERT_TRUE(queried);
    }
}

/**
 * @tc.name: ObjectResourceTypeName
 * @tc.desc: Tests that ObjectResourceType returns the correct resource name.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ObjectResourceTypeName, testing::ext::TestSize.Level1)
{
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::ObjectResourceType);
    ASSERT_TRUE(resType);
    EXPECT_EQ(resType->GetResourceName(), "ObjectResource");
}

/**
 * @tc.name: SaveResourceNullPtr
 * @tc.desc: Tests that SaveResource returns false when resource pointer is null.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, SaveResourceNullPtr, testing::ext::TestSize.Level1)
{
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::ObjectResourceType);
    ASSERT_TRUE(resType);
    if (auto objRes = interface_cast<IObjectResource>(resType)) {
        objRes->SetResourceType(ClassId::TestResource);
    }

    MemFile file;
    CORE_NS::ResourceId rid("test_save_null");
    CORE_NS::IResourceType::StorageInfo info{nullptr, &file, rid};
    EXPECT_FALSE(resType->SaveResource(nullptr, info));
}

/**
 * @tc.name: LoadResourceTypeMismatch
 * @tc.desc: Tests that LoadResource returns null when loaded resource type does not match.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, LoadResourceTypeMismatch, testing::ext::TestSize.Level1)
{
    // Serialize an ObjectResource with TestResource type
    TestSerialiser ser;
    auto obj = GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::ObjectResource);
    ASSERT_TRUE(obj);
    if (auto i = interface_cast<IObjectResource>(obj)) {
        i->SetResourceType(ClassId::TestResource);
    }
    ASSERT_TRUE(ser.Export(obj));
    auto data = ser.GetData();

    // Create an ObjectResourceType configured for a different type
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::ObjectResourceType);
    ASSERT_TRUE(resType);
    if (auto i = interface_cast<IObjectResource>(resType)) {
        i->SetResourceType(::META_NS::ClassId::Object);
    }

    MemFile payload(data);
    CORE_NS::ResourceId rid("test_mismatch");
    CORE_NS::IResourceType::StorageInfo info{nullptr, &payload, rid, "", resources, context};
    auto result = resType->LoadResource(info);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SetPropertyOverridesExisting
 * @tc.desc: Tests that SetProperty replaces an existing property with the same name.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, SetPropertyOverridesExisting, testing::ext::TestSize.Level1)
{
    auto opts = GetObjectRegistry().Create<IObjectResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
    ASSERT_TRUE(opts);

    opts->SetProperty("test", uint32_t(1));
    auto p1 = opts->GetProperty<uint32_t>("test");
    ASSERT_TRUE(p1);
    EXPECT_EQ(p1->GetValue(), 1);

    // Override with same name - should remove old and add new
    opts->SetProperty("test", uint32_t(2));
    auto p2 = opts->GetProperty<uint32_t>("test");
    ASSERT_TRUE(p2);
    EXPECT_EQ(p2->GetValue(), 2);
}

/**
 * @tc.name: ObjectResourceReloadResource
 * @tc.desc: Tests that ObjectResourceType ReloadResource returns false.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ObjectResourceReloadResource, testing::ext::TestSize.Level1)
{
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::ObjectResourceType);
    ASSERT_TRUE(resType);

    CORE_NS::ResourceId rid;
    CORE_NS::IResourceType::StorageInfo info{nullptr, nullptr, rid};
    CORE_NS::IResource::Ptr res;
    EXPECT_FALSE(resType->ReloadResource(info, res));
}

/**
 * @tc.name: RemoveResourceType
 * @tc.desc: Tests that RemoveResourceType removes a registered type.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, RemoveResourceType, testing::ext::TestSize.Level1)
{
    auto types = resources->GetResourceTypes();
    ASSERT_FALSE(types.empty());
    auto type = types[0]->GetResourceType();
    EXPECT_TRUE(resources->RemoveResourceType(type, types[0]->GetVersion()));
    auto typesAfter = resources->GetResourceTypes();
    EXPECT_EQ(typesAfter.size(), types.size() - 1);
}

/**
 * @tc.name: AddResourceTypeNull
 * @tc.desc: Tests that AddResourceType(nullptr) returns false.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, AddResourceTypeNull, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(resources->AddResourceType(nullptr));
}

/**
 * @tc.name: ImportFileNotFound
 * @tc.desc: Tests that Import with non-existent file returns FILE_NOT_FOUND.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ImportFileNotFound, testing::ext::TestSize.Level1)
{
    EXPECT_EQ(resources->Import("app://nonexistent_file.resources", context),
        CORE_NS::IResourceManager::Result::FILE_NOT_FOUND);
}

/**
 * @tc.name: ImportInvalidFile
 * @tc.desc: Tests that Import with invalid content returns INVALID_FILE.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ImportInvalidFile, testing::ext::TestSize.Level1)
{
    // Write garbage data to a file
    BASE_NS::vector<uint8_t> garbage = {0xFF, 0xFE, 0x00, 0x01, 0x02};
    WriteToFile(garbage, "app://garbage.resources");
    EXPECT_EQ(resources->Import("app://garbage.resources", context), CORE_NS::IResourceManager::Result::INVALID_FILE);
}

/**
 * @tc.name: GetResourceInfoNotFound
 * @tc.desc: Tests that GetResourceInfo with non-existent id returns empty ResourceInfo.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, GetResourceInfoNotFound, testing::ext::TestSize.Level1)
{
    auto info = resources->GetResourceInfo(CORE_NS::ResourceIdContext("nonexistent_resource", context));
    EXPECT_TRUE(info.id.name.empty());
}

/**
 * @tc.name: RenameToExistingTarget
 * @tc.desc: Tests that renaming to an existing target id returns false.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, RenameToExistingTarget, testing::ext::TestSize.Level1)
{
    auto res1 = CreateTestResource("app://res1");
    auto res2 = CreateTestResource("app://res2");
    ASSERT_TRUE(resources->AddResource(res1, "app://res1.json"));
    ASSERT_TRUE(resources->AddResource(res2, "app://res2.json"));
    EXPECT_FALSE(resources->RenameResource({"app://res1", context}, {"app://res2", context}));
}

/**
 * @tc.name: PurgeNonExistentResource
 * @tc.desc: Tests that PurgeResource with unknown id returns false.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, PurgeNonExistentResource, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(resources->PurgeResource(CORE_NS::ResourceIdContext("nonexistent", context)));
}

/**
 * @tc.name: RemoveNonExistentResource
 * @tc.desc: Tests that RemoveResource with unknown id returns false.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, RemoveNonExistentResource, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(resources->RemoveResource(CORE_NS::ResourceIdContext("nonexistent", context)));
}

/**
 * @tc.name: RemoveAllResourcesWithContext
 * @tc.desc: Tests that RemoveAllResources(context) only removes resources of that context.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, RemoveAllResourcesWithContext, testing::ext::TestSize.Level1)
{
    // Add resources under context
    auto res1 = CreateTestResource("app://ctx1_res");
    ASSERT_TRUE(resources->AddResource(res1, "app://ctx1_res.json"));

    // Add resources under context2 via the overload that takes a ResourceIdContext
    MakeEmptyJsonFile();
    ASSERT_TRUE(resources->AddResource({"ctx2_res", context2}, ClassId::TestResource.Id().ToUid(), "app://test.json"));

    // Remove only context's resources
    BASE_NS::shared_ptr<Listener> listener(new Listener);
    resources->AddListener(listener);
    resources->RemoveAllResources(context);

    // context resources should be gone
    EXPECT_TRUE(resources->GetResources(context).empty());
    EXPECT_FALSE(listener->removed.empty());

    // context2 resources should remain
    EXPECT_FALSE(resources->GetResources(context2).empty());
    resources->RemoveListener(listener);
}

/**
 * @tc.name: GroupHandleReuse
 * @tc.desc: Tests that GetGroupHandle returns the same cached handle on repeated calls.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, GroupHandleReuse, testing::ext::TestSize.Level1)
{
    auto res = CreateTestResource("app://test.scene");
    ASSERT_TRUE(resources->AddResource(res, "app://test.json"));
    auto owned = interface_cast<IOwnedResourceGroups>(resources);
    ASSERT_TRUE(owned);
    auto groups = resources->GetResourceGroups(context);
    ASSERT_FALSE(groups.empty());
    auto handle1 = owned->GetGroupHandle(groups[0], context);
    auto handle2 = owned->GetGroupHandle(groups[0], context);
    EXPECT_EQ(handle1, handle2);
}

/**
 * @tc.name: GetResourceCount
 * @tc.desc: Tests that GetResourceCount returns the correct number.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, GetResourceCount, testing::ext::TestSize.Level1)
{
    auto query = interface_cast<IResourceQuery>(resources);
    ASSERT_TRUE(query);
    EXPECT_EQ(query->GetResourceCount(), 0);
    auto res1 = CreateTestResource("app://res1");
    ASSERT_TRUE(resources->AddResource(res1, "app://res1.json"));
    EXPECT_EQ(query->GetResourceCount(), 1);
    auto res2 = CreateTestResource("app://res2");
    ASSERT_TRUE(resources->AddResource(res2, "app://res2.json"));
    EXPECT_EQ(query->GetResourceCount(), 2);
}

/**
 * @tc.name: ExportResourcePayloadNotFound
 * @tc.desc: Tests that ExportResourcePayload returns NO_RESOURCE_DATA for an unregistered resource.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ExportResourcePayloadNotFound, testing::ext::TestSize.Level1)
{
    // Create a resource but don't add it to the manager
    auto res = CreateTestResource("app://orphan_resource");
    EXPECT_EQ(resources->ExportResourcePayload(res), CORE_NS::IResourceManager::Result::NO_RESOURCE_DATA);
}

/**
 * @tc.name: AddResourceReplaceExisting
 * @tc.desc: Tests that adding a resource with the same id replaces the old one.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, AddResourceReplaceExisting, testing::ext::TestSize.Level1)
{
    auto res1 = CreateTestResource("app://same_id");
    ASSERT_TRUE(resources->AddResource(res1, "app://same_id.json"));
    auto queried1 = resources->GetResource(CORE_NS::ResourceIdContext("app://same_id", context));
    EXPECT_EQ(queried1, res1);

    auto res2 = CreateTestResource("app://same_id");
    ASSERT_TRUE(resources->AddResource(res2, "app://same_id_v2.json"));
    auto queried2 = resources->GetResource(CORE_NS::ResourceIdContext("app://same_id", context));
    EXPECT_EQ(queried2, res2);
    EXPECT_NE(queried2, res1);
}

/**
 * @tc.name: ObjectTemplateTypeName
 * @tc.desc: Tests that ObjectTemplateType returns "ObjectTemplate" as resource name.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ObjectTemplateTypeName, testing::ext::TestSize.Level1)
{
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::ObjectTemplateType);
    ASSERT_TRUE(resType);
    EXPECT_EQ(resType->GetResourceName(), "ObjectTemplate");
}

/**
 * @tc.name: ObjectTemplateReloadResource
 * @tc.desc: Tests that ObjectTemplateType ReloadResource returns false.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ObjectTemplateReloadResource, testing::ext::TestSize.Level1)
{
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::ObjectTemplateType);
    ASSERT_TRUE(resType);
    CORE_NS::ResourceId rid;
    CORE_NS::IResourceType::StorageInfo info{nullptr, nullptr, rid};
    CORE_NS::IResource::Ptr res;
    EXPECT_FALSE(resType->ReloadResource(info, res));
}

/**
 * @tc.name: ObjectTemplateIterate
 * @tc.desc: Tests ObjectTemplate iteration with content searchable and not searchable.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, ObjectTemplateIterate, testing::ext::TestSize.Level1)
{
    auto templ = GetObjectRegistry().Create<IObjectTemplate>(META_NS::ClassId::ObjectTemplate);
    ASSERT_TRUE(templ);
    auto iterable = interface_cast<IIterable>(templ);
    ASSERT_TRUE(iterable);

    // Set content and make searchable
    auto content = GetObjectRegistry().Create<IObject>(META_NS::ClassId::Object);
    ASSERT_TRUE(content);
    templ->SetContent(content);

    // ContentSearchable defaults to false, so iterate should return CONTINUE without calling
    {
        bool called = false;
        auto func = MakeIterationConstCallable([&](const IObject::Ptr&) {
            called = true;
            return true;
        });
        IterationParameters params{*func, {}};
        auto result = static_cast<const IIterable*>(iterable)->Iterate(params);
        EXPECT_TRUE(result);
        EXPECT_FALSE(called);
    }

    // Make searchable
    if (auto md = interface_cast<IMetadata>(templ)) {
        if (auto prop = md->GetProperty<bool>("ContentSearchable")) {
            prop->SetValue(true);
        }
    }

    // Now iterate should invoke the callback
    {
        bool called = false;
        auto func = MakeIterationConstCallable([&](const IObject::Ptr& obj) {
            called = true;
            EXPECT_TRUE(obj);
            return true;
        });
        IterationParameters params{*func, {}};
        auto result = static_cast<const IIterable*>(iterable)->Iterate(params);
        EXPECT_TRUE(called);
    }
}

/**
 * @tc.name: SaveResourceTypeMismatch
 * @tc.desc: Tests that SaveResource returns false when resource type doesn't match.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, SaveResourceTypeMismatch, testing::ext::TestSize.Level1)
{
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::ObjectResourceType);
    ASSERT_TRUE(resType);
    // Configure for Object type
    if (auto objRes = interface_cast<IObjectResource>(resType)) {
        objRes->SetResourceType(META_NS::ClassId::Object);
    }

    // Create a resource of TestResource type
    auto res = GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::ObjectResource);
    ASSERT_TRUE(res);
    if (auto i = interface_cast<IObjectResource>(res)) {
        i->SetResourceType(ClassId::TestResource);
    }

    MemFile file;
    CORE_NS::ResourceId rid("test_type_mismatch");
    CORE_NS::IResourceType::StorageInfo info{nullptr, &file, rid};
    EXPECT_FALSE(resType->SaveResource(res, info));
}

/**
 * @tc.name: AnimationResourceTypeName
 * @tc.desc: Tests that AnimationResourceType returns "AnimationResource" as resource name.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, AnimationResourceTypeName, testing::ext::TestSize.Level1)
{
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::AnimationResourceType);
    ASSERT_TRUE(resType);
    EXPECT_EQ(resType->GetResourceName(), "AnimationResource");
}

/**
 * @tc.name: AnimationResourceReloadResource
 * @tc.desc: Tests that AnimationResourceType ReloadResource returns false.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, AnimationResourceReloadResource, testing::ext::TestSize.Level1)
{
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::AnimationResourceType);
    ASSERT_TRUE(resType);
    CORE_NS::ResourceId rid;
    CORE_NS::IResourceType::StorageInfo info{nullptr, nullptr, rid};
    CORE_NS::IResource::Ptr res;
    EXPECT_FALSE(resType->ReloadResource(info, res));
}

/**
 * @tc.name: AnimationSaveResourceNullPtr
 * @tc.desc: Tests that AnimationResourceType SaveResource returns false when resource is null.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, AnimationSaveResourceNullPtr, testing::ext::TestSize.Level1)
{
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::AnimationResourceType);
    ASSERT_TRUE(resType);

    MemFile file;
    CORE_NS::ResourceId rid("test_anim_null");
    CORE_NS::IResourceType::StorageInfo info{nullptr, &file, rid};
    EXPECT_FALSE(resType->SaveResource(nullptr, info));
}

/**
 * @tc.name: AnimationLoadResourceInvalidPayload
 * @tc.desc: Tests that AnimationResourceType LoadResource returns null for invalid payload data.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, AnimationLoadResourceInvalidPayload, testing::ext::TestSize.Level1)
{
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::AnimationResourceType);
    ASSERT_TRUE(resType);

    BASE_NS::vector<uint8_t> garbage = {0xFF, 0xFE, 0x00, 0x01, 0x02};
    MemFile payload(garbage);
    CORE_NS::ResourceId rid("test_anim_invalid");
    CORE_NS::IResourceType::StorageInfo info{nullptr, &payload, rid};
    auto result = resType->LoadResource(info);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: AnimationSaveLoadRoundTrip
 * @tc.desc: Tests AnimationResourceType save and load round trip via payload.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, AnimationSaveLoadRoundTrip, testing::ext::TestSize.Level1)
{
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::AnimationResourceType);
    ASSERT_TRUE(resType);

    // Create a PropertyAnimation
    auto anim = GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::PropertyAnimation);
    ASSERT_TRUE(anim);

    // Save
    MemFile saveFile;
    CORE_NS::ResourceId rid("test_anim_roundtrip");
    CORE_NS::IResourceType::StorageInfo saveInfo{nullptr, &saveFile, rid, "", resources, context};
    EXPECT_TRUE(resType->SaveResource(anim, saveInfo));

    // Load from saved data
    MemFile loadFile(saveFile.Data());
    CORE_NS::IResourceType::StorageInfo loadInfo{nullptr, &loadFile, rid, "", resources, context};
    auto loaded = resType->LoadResource(loadInfo);
    EXPECT_TRUE(loaded);
}

/**
 * @tc.name: AnimationSaveLoadWithOptions
 * @tc.desc: Tests AnimationResourceType save and load via options (template path, empty payload path).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, AnimationSaveLoadWithOptions, testing::ext::TestSize.Level1)
{
    auto resType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::AnimationResourceType);
    ASSERT_TRUE(resType);

    // Create a PropertyAnimation
    auto anim = GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::PropertyAnimation);
    ASSERT_TRUE(anim);

    // Save with options (no payload, empty path triggers SaveTemplate embedding in "__anim")
    auto saveOpts = GetObjectRegistry().Create<CORE_NS::IResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
    ASSERT_TRUE(saveOpts);
    CORE_NS::ResourceId rid("test_anim_options");
    CORE_NS::IResourceType::StorageInfo saveInfo{saveOpts, nullptr, rid, "", resources, context};
    EXPECT_TRUE(resType->SaveResource(anim, saveInfo));

    // Load with options
    auto loadOpts = GetObjectRegistry().Create<CORE_NS::IResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
    ASSERT_TRUE(loadOpts);
    if (auto so = interface_cast<IObjectResourceOptions>(saveOpts)) {
        if (auto lo = interface_cast<IObjectResourceOptions>(loadOpts)) {
            lo->SetOptionData(so->GetOptionData());
        }
    }
    CORE_NS::IResourceType::StorageInfo loadInfo{loadOpts, nullptr, rid, "", resources, context};
    auto loaded = resType->LoadResource(loadInfo);
    EXPECT_TRUE(loaded);
}

/**
 * @tc.name: AnimationLoadWithBaseTemplate
 * @tc.desc: Tests AnimationResourceType load with base template to exercise SetValuesFromTemplate.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FileResourceManagerTest, AnimationLoadWithBaseTemplate, testing::ext::TestSize.Level1)
{
    // Register AnimationResourceType with the fixture's resource manager
    auto animResType = GetObjectRegistry().Create<CORE_NS::IResourceType>(META_NS::ClassId::AnimationResourceType);
    ASSERT_TRUE(animResType);
    ASSERT_TRUE(resources->AddResourceType(animResType));

    // Also register AnimationResourceTemplate as an ObjectResource type
    {
        auto res = GetObjectRegistry().Create<IObjectResource>(META_NS::ClassId::ObjectResourceType);
        ASSERT_TRUE(res);
        res->SetResourceType(META_NS::ClassId::AnimationResourceTemplate);
        ASSERT_TRUE(resources->AddResourceType(interface_pointer_cast<CORE_NS::IResourceType>(res)));
    }

    // Create a base ObjectResource template with a property matching one in PropertyAnimation ("Enabled")
    auto baseRes = GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::ObjectResource);
    ASSERT_TRUE(baseRes);
    if (auto i = interface_cast<IObjectResource>(baseRes)) {
        i->SetResourceType(META_NS::ClassId::AnimationResourceTemplate);
    }
    if (auto m = interface_cast<IMetadata>(baseRes)) {
        m->AddProperty(ConstructProperty<bool>("Enabled", true));
    }
    if (auto i = interface_cast<CORE_NS::ISetResourceId>(baseRes)) {
        i->SetResourceId({"base_anim_template", context});
    }
    ASSERT_TRUE(resources->AddResource(baseRes, "app://base_anim_template.json"));

    // Create a PropertyAnimation (simpler than TrackAnimation, no special attachments)
    auto anim = GetObjectRegistry().Create<CORE_NS::IResource>(META_NS::ClassId::PropertyAnimation);
    ASSERT_TRUE(anim);
    if (auto dt = interface_cast<IDerivedFromTemplate>(anim)) {
        dt->SetTemplateId({"base_anim_template"});
    }

    // Save the animation via AnimationResourceType with empty path (triggers SaveTemplate)
    auto saveOpts = GetObjectRegistry().Create<CORE_NS::IResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
    ASSERT_TRUE(saveOpts);
    CORE_NS::ResourceId rid("derived_anim");
    CORE_NS::IResourceType::StorageInfo saveInfo{saveOpts, nullptr, rid, "", resources, context};
    EXPECT_TRUE(animResType->SaveResource(anim, saveInfo));

    // Load it back — LoadTemplate finds the base, calls SetValuesFromTemplate
    auto loadOpts = GetObjectRegistry().Create<CORE_NS::IResourceOptions>(META_NS::ClassId::ObjectResourceOptions);
    ASSERT_TRUE(loadOpts);
    if (auto so = interface_cast<IObjectResourceOptions>(saveOpts)) {
        if (auto lo = interface_cast<IObjectResourceOptions>(loadOpts)) {
            lo->SetOptionData(so->GetOptionData());
        }
    }
    CORE_NS::IResourceType::StorageInfo loadInfo{loadOpts, nullptr, rid, "", resources, context};
    auto loaded = animResType->LoadResource(loadInfo);
    EXPECT_TRUE(loaded);
}

}  // namespace UTest

META_END_NAMESPACE()
