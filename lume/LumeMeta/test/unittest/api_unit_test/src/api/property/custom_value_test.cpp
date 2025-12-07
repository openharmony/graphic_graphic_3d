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

// clang-format off
#include "helpers/testing_objects.h"
// clang-format on

#include <meta/api/property/custom_value.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

#include "test_framework.h"

META_BEGIN_NAMESPACE()

namespace UTest {

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

/**
 * @tc.name: GetSet
 * @tc.desc: Tests for Get Set. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_CustomValueTest, GetSet, testing::ext::TestSize.Level1)
{
    RegisterObjectType<GetSetTestType>();
    auto o = GetObjectRegistry().Create(ClassId::GetSetTestType);
    ASSERT_TRUE(o);
    auto obj = interface_cast<IGetSetTestInterface>(o);
    ASSERT_TRUE(obj);

    EXPECT_EQ(obj->Value()->GetValue(), 0);
    EXPECT_EQ(obj->Value()->GetValue(), 0);
    obj->Value()->SetValue(2);
    EXPECT_EQ(obj->Value()->GetValue(), 2);

    EXPECT_EQ(obj->GetCount(), 2);
    EXPECT_EQ(obj->SetCount(), 1);
    o.reset();

    UnregisterObjectType<GetSetTestType>();
}

} // namespace UTest

META_END_NAMESPACE()
