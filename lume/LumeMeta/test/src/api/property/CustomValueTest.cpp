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

#include <TestRunner.h>
#include <src/testing_objects.h>

#include <gtest/gtest.h>

#include <meta/api/property/custom_value.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

class IGetSetTestInterface : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IGetSetTestInterface, "a952bbc2-c352-4134-bb09-95be0b983c0b")
public:
    META_PROPERTY(int, Value)

    virtual int GetCount() const = 0;
    virtual int SetCount() const = 0;
};

META_REGISTER_CLASS(GetSetTestType, "35ed312a-6195-4f0d-959f-2e21a47584d6", ObjectCategory::NO_CATEGORY)

class GetSetTestType : public IntroduceInterfaces<MetaObject, IGetSetTestInterface> {
    META_OBJECT(GetSetTestType, ClassId::GetSetTestType, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IGetSetTestInterface, int, Value)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(int, Value)

    int GetF() const
    {
        ++getCalled_;
        return value_;
    }
    bool SetF(const int& value)
    {
        ++setCalled_;
        value_ = value;
        return true;
    }

    bool Build(const IMetadata::Ptr& d) override
    {
        bool res = Super::Build(d);
        if (res) {
            AddGetSetCustomValue(Value(), this, &GetSetTestType::GetF, &GetSetTestType::SetF);
        }
        return res;
    }

    int GetCount() const override
    {
        return getCalled_;
    }
    int SetCount() const override
    {
        return setCalled_;
    }

private:
    int value_ {};
    mutable int getCalled_ {};
    mutable int setCalled_ {};
};

class CustomValueTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: GetSet
 * @tc.desc: test Get Set
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(CustomValueTest, GetSet, TestSize.Level1)
{
    auto adapter = std::make_shared<OHOS::Render3D::SceneAdapter>();
    adapter->LoadPluginsAndInit();
    
    RegisterObjectType<GetSetTestType>();
    auto obj = GetObjectRegistry().Create<IGetSetTestInterface>(ClassId::GetSetTestType);

    EXPECT_EQ(obj->Value()->GetValue(), 0);
    EXPECT_EQ(obj->Value()->GetValue(), 0);
    obj->Value()->SetValue(2);
    EXPECT_EQ(obj->Value()->GetValue(), 2);

    EXPECT_EQ(obj->GetCount(), 2);
    EXPECT_EQ(obj->SetCount(), 1);

    UnregisterObjectType<GetSetTestType>();
}

META_END_NAMESPACE()
