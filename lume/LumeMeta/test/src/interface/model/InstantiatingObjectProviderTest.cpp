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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <meta/api/make_callback.h>
#include <meta/api/object.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/intf_recyclable.h>
#include <meta/interface/loaders/intf_file_content_loader.h>
#include <meta/interface/model/intf_object_provider.h>

#include "TestRunner.h"
#include "helpers/serialisation_utils.h"
#include "helpers/testing_objects.h"


using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

static IDataModel::Ptr CreateTestDataModel(size_t size, size_t startNum = 0)
{
    auto model = GetObjectRegistry().Create<IContainer>(META_NS::ClassId::ContainerDataModel);
    for (int i = 0; i != size; ++i) {
        auto p = CreateTestType();
        p->First()->SetValue(startNum + i);
        model->Add(p);
    }
    return interface_pointer_cast<IDataModel>(model);
}

META_REGISTER_CLASS(CustomDataItemView, "4a3bd491-3333-4a9a-bcad-92530493be09", ObjectCategoryBits::NO_CATEGORY)

class CustomDataItemView : public IntroduceInterfaces<ObjectFwd, IRecyclable> {
    META_OBJECT(CustomDataItemView, ClassId::CustomDataItemView, IntroduceInterfaces)
public:
    bool ReBuild(const IMetadata::Ptr& data) override
    {
        if (auto p = data->GetProperty("First")) {
            auto prop = DuplicatePropertyType(META_NS::GetObjectRegistry(), p, "MyFirst");
            PropertyLock l { prop };
            l->SetBind(p);
            AddProperty(prop);
            return true;
        }
        return false;
    }
    void Dispose() override
    {
        if (auto p = GetProperty("MyFirst")) {
            RemoveProperty(p);
        }
    }
};

namespace {
struct TestDefinition {
    ClassInfo obj {};
    BASE_NS::string propName;
};
} // namespace

class InstantiatingObjectProviderTest : public ::testing::TestWithParam<std::tuple<TestDefinition, int>> {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
    void SetUp() override
    {
        RegisterObjectType<CustomDataItemView>();
        auto v = std::get<0>(GetParam());
        instObj_ = v.obj;
        propName_ = v.propName;
        cacheHint_ = std::get<1>(GetParam());

        auto instprov =
            GetObjectRegistry().Create<IInstantiatingObjectProvider>(META_NS::ClassId::InstantiatingObjectProvider);
        ASSERT_TRUE(instprov);

        auto model = CreateTestDataModel(10); // 10: param
        cont_ = interface_pointer_cast<IContainer>(model);
        instprov->SetObjectClassId(instObj_.Id());

        prov_ = interface_pointer_cast<IModelObjectProvider>(instprov);
        ASSERT_TRUE(prov_);
        prov_->SetDataModel(model);
        prov_->CacheHint()->SetValue(cacheHint_);

        EXPECT_EQ(prov_->GetObjectCount(), 10); // 10: param
    }

    void TearDown() override
    {
        UnregisterObjectType<CustomDataItemView>();
    }

protected:
    ClassInfo instObj_;
    BASE_NS::string propName_;
    size_t cacheHint_ {};
    IModelObjectProvider::Ptr prov_;
    IContainer::Ptr cont_;
};

HWTEST_P(InstantiatingObjectProviderTest, CreateAndDispose, TestSize.Level1)
{
    BASE_NS::vector<IObject::Ptr> objs;

    for (std::size_t i = 0; i != 10; ++i) {
        objs.push_back(prov_->CreateObject(DataModelIndex { i }));
        auto d = interface_cast<IMetadata>(objs.back());
        ASSERT_TRUE(d);
        auto p = d->GetProperty<int>(propName_);
        ASSERT_TRUE(p);
        EXPECT_EQ(p->GetValue(), i);
    }

    for (std::size_t i = 0; i != 10; ++i) {
        EXPECT_EQ(prov_->DisposeObject(objs.front()), i < cacheHint_);
        auto d = interface_cast<IMetadata>(objs.front());
        ASSERT_TRUE(d);
        if (interface_cast<IRecyclable>(d)) {
            ASSERT_FALSE(d->GetProperty(propName_));
        } else {
            auto p = d->GetProperty<int>(propName_);
            ASSERT_TRUE(p);
            EXPECT_FALSE(p->GetBind());
        }
    }
}

HWTEST_P(InstantiatingObjectProviderTest, AddRemoveMove, TestSize.Level1)
{
    size_t addIndex = -1;
    size_t RemoveIndex = -1;
    size_t moveFromIndex = -1;
    size_t moveToIndex = -1;

    prov_->OnDataAdded()->AddHandler(
        MakeCallback<IOnDataAdded>([&](auto index, auto count) { addIndex = index.Index(); }));
    prov_->OnDataRemoved()->AddHandler(
        MakeCallback<IOnDataRemoved>([&](auto index, auto count) { RemoveIndex = index.Index(); }));
    prov_->OnDataMoved()->AddHandler(MakeCallback<IOnDataMoved>([&](auto from, auto count, auto to) {
        moveFromIndex = from.Index();
        moveToIndex = to.Index();
    }));

    auto o1 = CreateTestType<IObject>();
    cont_->Add(o1);

    EXPECT_EQ(prov_->GetObjectCount(), 11);
    EXPECT_EQ(addIndex, 10);

    EXPECT_TRUE(prov_->CreateObject(DataModelIndex { 10 }));

    cont_->Remove(cont_->GetAt(0));

    EXPECT_EQ(prov_->GetObjectCount(), 10);
    EXPECT_EQ(RemoveIndex, 0);

    cont_->Move(1, 8);
    EXPECT_EQ(prov_->GetObjectCount(), 10);
    EXPECT_EQ(moveFromIndex, 1);
    EXPECT_EQ(moveToIndex, 8);
}

static std::string BuildTestName(const testing::TestParamInfo<InstantiatingObjectProviderTest::ParamType>& info)
{
    auto name = std::get<0>(info.param).obj.Name();
    auto test =
        std::string(name.data(), name.size()) + std::string("_CacheHint_") + std::to_string(std::get<1>(info.param));
    return test;
}

INSTANTIATE_TEST_SUITE_P(InstantiatingObjectProviderTests, InstantiatingObjectProviderTest,
    testing::Combine(testing::Values(TestDefinition { META_NS::ClassId::Object, "Model.First" },
                         TestDefinition { ClassId::CustomDataItemView, "MyFirst" }),
        testing::Range(0, 10, 4)),
    BuildTestName);

class CompositeObjectProviderTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
    IObjectProvider::Ptr CreateProvider(size_t size, size_t startNum)
    {
        auto instprov =
            GetObjectRegistry().Create<IInstantiatingObjectProvider>(META_NS::ClassId::InstantiatingObjectProvider);
        if (!instprov) {
            return nullptr;
        }

        auto p = interface_pointer_cast<IModelObjectProvider>(instprov);

        p->SetDataModel(CreateTestDataModel(size, startNum));
        instprov->SetObjectClassId(META_NS::ClassId::Object.Id());

        return p;
    }

    void SetUp() override
    {
        p1_ = CreateProvider(5, 0); // 5: param
        ASSERT_TRUE(p1_);
        c1_ = interface_cast<IContainer>(interface_cast<IModelObjectProvider>(p1_)->GetDataModel());
        p2_ = CreateProvider(0, 5); // 5: param
        ASSERT_TRUE(p2_);
        c2_ = interface_cast<IContainer>(interface_cast<IModelObjectProvider>(p2_)->GetDataModel());
        p3_ = CreateProvider(1, 5); // 5: param
        ASSERT_TRUE(p3_);
        c3_ = interface_cast<IContainer>(interface_cast<IModelObjectProvider>(p3_)->GetDataModel());
        p4_ = CreateProvider(10, 6); // 10: param  6: param
        ASSERT_TRUE(p4_);
        c4_ = interface_cast<IContainer>(interface_cast<IModelObjectProvider>(p4_)->GetDataModel());

        auto p = GetObjectRegistry().Create<IContainer>(META_NS::ClassId::CompositeObjectProvider);
        p->Add(p1_);
        p->Add(p2_);
        p->Add(p3_);
        p->Add(p4_);

        prov_ = interface_pointer_cast<IObjectProvider>(p);

        size_ = c1_->GetSize() + c2_->GetSize() + c3_->GetSize() + c4_->GetSize();
    }

    void TearDown() override {}

    IObjectProvider::Ptr p1_;
    IContainer* c1_;
    IObjectProvider::Ptr p2_;
    IContainer* c2_;
    IObjectProvider::Ptr p3_;
    IContainer* c3_;
    IObjectProvider::Ptr p4_;
    IContainer* c4_;

    IObjectProvider::Ptr prov_;

    size_t size_;
};

HWTEST_F(CompositeObjectProviderTest, CreateAndDispose, TestSize.Level1)
{
    BASE_NS::vector<IObject::Ptr> objs;

    EXPECT_EQ(prov_->GetObjectCount(), size_);

    for (std::size_t i = 0; i != size_; ++i) {
        objs.push_back(prov_->CreateObject(DataModelIndex { i }));
        auto d = interface_cast<IMetadata>(objs.back());
        ASSERT_TRUE(d);
        auto p = d->GetProperty<int>("Model.First");
        ASSERT_TRUE(p);
        EXPECT_EQ(p->GetValue(), i);
    }

    for (std::size_t i = 0; i != size_; ++i) {
        prov_->DisposeObject(objs.front());
        auto d = interface_cast<IMetadata>(objs.front());
        ASSERT_TRUE(d);

        auto p = d->GetProperty<int>("Model.First");
        ASSERT_TRUE(p);
        EXPECT_FALSE(p->GetBind());
    }
}

HWTEST_F(CompositeObjectProviderTest, AddRemoveMove, TestSize.Level1)
{
    size_t addIndex = -1;
    size_t RemoveIndex = -1;
    size_t moveFromIndex = -1;
    size_t moveToIndex = -1;

    prov_->OnDataAdded()->AddHandler(
        MakeCallback<IOnDataAdded>([&](auto index, auto count) { addIndex = index.Index(); }));
    prov_->OnDataRemoved()->AddHandler(
        MakeCallback<IOnDataRemoved>([&](auto index, auto count) { RemoveIndex = index.Index(); }));
    prov_->OnDataMoved()->AddHandler(MakeCallback<IOnDataMoved>([&](auto from, auto count, auto to) {
        moveFromIndex = from.Index();
        moveToIndex = to.Index();
    }));

    c3_->Add(CreateTestType<IObject>());

    EXPECT_EQ(prov_->GetObjectCount(), ++size_);
    EXPECT_EQ(addIndex, 6);

    c2_->Add(CreateTestType<IObject>());

    EXPECT_EQ(prov_->GetObjectCount(), ++size_);
    EXPECT_EQ(addIndex, 5);

    c4_->Remove(c4_->GetAt(0));

    EXPECT_EQ(prov_->GetObjectCount(), --size_);
    EXPECT_EQ(RemoveIndex, 8);

    c4_->Move(1, 6);
    EXPECT_EQ(prov_->GetObjectCount(), size_);
    EXPECT_EQ(moveFromIndex, 9);
    EXPECT_EQ(moveToIndex, 14);
}

HWTEST_F(CompositeObjectProviderTest, AddProviders, TestSize.Level1)
{
    size_t index = -1;
    size_t count = 0;

    prov_->OnDataAdded()->AddHandler(MakeCallback<IOnDataAdded>([&](auto i, auto c) {
        index = i.Index();
        count = c;
    }));

    auto cont = interface_cast<IContainer>(prov_);

    cont->Add(CreateProvider(2, 0));
    EXPECT_EQ(prov_->GetObjectCount(), size_ + 2);
    EXPECT_EQ(index, size_);
    EXPECT_EQ(count, 2);

    cont->Insert(1, CreateProvider(4, 0));
    EXPECT_EQ(prov_->GetObjectCount(), size_ + 2 + 4);
    EXPECT_EQ(index, 5);
    EXPECT_EQ(count, 4);
}

HWTEST_F(CompositeObjectProviderTest, RemoveProviders, TestSize.Level1)
{
    size_t index = -1;
    size_t count = -1;

    prov_->OnDataRemoved()->AddHandler(MakeCallback<IOnDataRemoved>([&](auto i, auto c) {
        index = i.Index();
        count = c;
    }));

    auto cont = interface_cast<IContainer>(prov_);

    cont->Remove(cont->GetAt(1));
    EXPECT_EQ(prov_->GetObjectCount(), size_);
    EXPECT_EQ(index, -1);
    EXPECT_EQ(count, -1);

    cont->Remove(cont->GetAt(1));
    EXPECT_EQ(prov_->GetObjectCount(), size_ - 1);
    EXPECT_EQ(index, 5);
    EXPECT_EQ(count, 1);

    cont->Remove(cont->GetAt(cont->GetSize() - 1));
    EXPECT_EQ(prov_->GetObjectCount(), size_ - 1 - 10);
    EXPECT_EQ(index, 5);
    EXPECT_EQ(count, 10);
}

HWTEST_F(CompositeObjectProviderTest, MoveProviders, TestSize.Level1)
{
    size_t from = -1;
    size_t to = -1;
    size_t count = -1;

    prov_->OnDataMoved()->AddHandler(MakeCallback<IOnDataMoved>([&](auto f, auto c, auto t) {
        from = f.Index();
        to = t.Index();
        count = c;
    }));

    auto cont = interface_cast<IContainer>(prov_);

    cont->Move(1, 4);
    EXPECT_EQ(prov_->GetObjectCount(), size_);
    EXPECT_EQ(from, -1);
    EXPECT_EQ(to, -1);
    EXPECT_EQ(count, -1);
    // move it back
    cont->Move(3, 1);

    cont->Move(0, 4);
    EXPECT_EQ(prov_->GetObjectCount(), size_);
    EXPECT_EQ(from, 0);
    EXPECT_EQ(to, 16);
    EXPECT_EQ(count, 5);

    cont->Move(3, 0);
    EXPECT_EQ(prov_->GetObjectCount(), size_);
    EXPECT_EQ(from, 11);
    EXPECT_EQ(to, 0);
    EXPECT_EQ(count, 5);

    cont->Move(2, 3);
    EXPECT_EQ(prov_->GetObjectCount(), size_);
    EXPECT_EQ(from, 5);
    EXPECT_EQ(to, 16);
    EXPECT_EQ(count, 1);
}

class ContentLoaderObjectProviderTest : public ::testing::TestWithParam<int> {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
    void SetUp() override
    {
        cacheHint_ = GetParam();

        prov_ = GetObjectRegistry().Create<IModelObjectProvider>(META_NS::ClassId::ContentLoaderObjectProvider);
        ASSERT_TRUE(prov_);

        auto model = CreateTestDataModel(20, 10); // 20: param  10: param
        cont_ = interface_pointer_cast<IContainer>(model);

        prov_->SetDataModel(model);
        prov_->CacheHint()->SetValue(cacheHint_);

        EXPECT_EQ(prov_->GetObjectCount(), 20); // 20: param
    }

    void SetJsonLoader(CORE_NS::IFile::Ptr file)
    {
        if (auto p = interface_cast<IContentLoaderObjectProvider>(prov_)) {
            auto loader = GetObjectRegistry().Create<IFileContentLoader>(META_NS::ClassId::JsonContentLoader);
            ASSERT_TRUE(loader);
            loader->SetFile(BASE_NS::move(file));
            p->SetContentLoader(loader);
        }
    }

    void TearDown() override
    {
        UnregisterObjectType<CustomDataItemView>();
    }

protected:
    size_t cacheHint_ {};
    IModelObjectProvider::Ptr prov_;
    IContainer::Ptr cont_;
};

static CORE_NS::IFile::Ptr CreateTestFileWithoutProperty()
{
    TestSerialiser ser;
    ser.Export(CreateInstance(META_NS::ClassId::Object));
    return CORE_NS::IFile::Ptr(new MemFile(ser.GetData()));
}

HWTEST_P(ContentLoaderObjectProviderTest, JsonWithoutProperty, TestSize.Level1)
{
    SetJsonLoader(CreateTestFileWithoutProperty());

    BASE_NS::vector<IObject::Ptr> objs;
    for (size_t i = 0; i != 10; ++i) {
        auto obj = prov_->CreateObject(DataModelIndex(i));
        ASSERT_TRUE(obj);
        auto p = interface_cast<IMetadata>(obj)->GetProperty<int>("Model.First");
        ASSERT_TRUE(p);
        EXPECT_EQ(p->GetValue(), i + 10);
        objs.push_back(obj);
    }
    for (size_t i = 0; i != 10; ++i) {
        auto o = objs.back();
        objs.pop_back();
        prov_->DisposeObject(o);
    }
    for (size_t i = 10; i != 20; ++i) {
        auto obj = prov_->CreateObject(DataModelIndex(i));
        ASSERT_TRUE(obj);
        auto p = interface_cast<IMetadata>(obj)->GetProperty<int>("Model.First");
        ASSERT_TRUE(p);
        EXPECT_EQ(p->GetValue(), i + 10);
    }
}

static CORE_NS::IFile::Ptr CreateTestFileWithProperty()
{
    TestSerialiser ser;
    Metadata object(CreateInstance(META_NS::ClassId::Object));
    object.AddProperty<int>("Model.First", 0);
    ser.Export(object.GetPtr<META_NS::IObject>());
    return CORE_NS::IFile::Ptr(new MemFile(ser.GetData()));
}

HWTEST_P(ContentLoaderObjectProviderTest, JsonWithProperty, TestSize.Level1)
{
    SetJsonLoader(CreateTestFileWithProperty());

    BASE_NS::vector<IObject::Ptr> objs;
    for (size_t i = 0; i != 10; ++i) {
        auto obj = prov_->CreateObject(DataModelIndex(i));
        ASSERT_TRUE(obj);
        auto p = interface_cast<IMetadata>(obj)->GetProperty<int>("Model.First");
        ASSERT_TRUE(p);
        EXPECT_EQ(p->GetValue(), i + 10);
        objs.push_back(obj);
    }
    for (size_t i = 0; i != 10; ++i) {
        auto o = objs.back();
        objs.pop_back();
        prov_->DisposeObject(o);
    }
    for (size_t i = 10; i != 20; ++i) {
        auto obj = prov_->CreateObject(DataModelIndex(i));
        ASSERT_TRUE(obj);
        auto p = interface_cast<IMetadata>(obj)->GetProperty<int>("Model.First");
        ASSERT_TRUE(p);
        EXPECT_EQ(p->GetValue(), i + 10);
    }
}

static std::string BuildContentLoaderTestName(
    const testing::TestParamInfo<ContentLoaderObjectProviderTest::ParamType>& info)
{
    return std::string("CacheHint_") + std::to_string(info.param);
}

INSTANTIATE_TEST_SUITE_P(ContentLoaderObjectProviderTests, ContentLoaderObjectProviderTest,
    testing::Range(0, 10, 4), BuildContentLoaderTestName);


META_END_NAMESPACE()
