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
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/minimal_object.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/property/intf_modifier.h>
#include <meta/interface/property/intf_stack_resetable.h>
#include <meta/interface/property/property.h>

#include "TestRunner.h"
#include "helpers/testing_objects.h"

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

class ModifierTest : public testing::Test {
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

META_REGISTER_CLASS(ValidRangeEvaluator, "bf310567-a76c-41c3-af08-81c1adf682ef", ObjectCategory::NO_CATEGORY)

class ValidRangeEvaluator : public IntroduceInterfaces<MinimalObject, IModifier> {
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(ClassId::ValidRangeEvaluator)
public:
    ValidRangeEvaluator(int min, int max) : min_(min), max_(max) {}

    EvaluationResult ProcessOnGet(IAny& value) override
    {
        int v = GetValue<int>(value);
        if (v < min_) {
            value.CopyFrom(Any<int>(min_));
            return EVAL_VALUE_CHANGED;
        } else if (v > max_) {
            value.CopyFrom(Any<int>(max_));
            return EVAL_VALUE_CHANGED;
        }
        return EVAL_CONTINUE;
    }
    EvaluationResult ProcessOnSet(IAny& value, const IAny& current) override
    {
        return EVAL_CONTINUE;
    }
    bool IsCompatible(const TypeId& id) const override
    {
        return id == TypeId(UidFromType<int>());
    }

private:
    int min_, max_;
};

HWTEST_F(ModifierTest, ValidRange, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto stack = interface_cast<IStackProperty>(p->GetProperty());
    ASSERT_TRUE(stack);

    ASSERT_TRUE(stack->AddModifier(IModifier::Ptr(new ValidRangeEvaluator(1, 10))));
    p->SetValue(13);
    EXPECT_EQ(p->GetValue(), 10);
    p->SetValue(0);
    EXPECT_EQ(p->GetValue(), 1);
    p->SetValue(6);
    EXPECT_EQ(p->GetValue(), 6);

    auto mods = stack->GetModifiers(BASE_NS::vector<TypeId>({ IModifier::UID }), false);
    EXPECT_EQ(mods.size(), 1);
}

HWTEST_F(ModifierTest, ValidRangePropertyBind, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto modifier = IModifier::Ptr(new ValidRangeEvaluator(1, 10));
    auto stack = interface_cast<IStackProperty>(p->GetProperty());
    ASSERT_TRUE(stack);
    stack->AddModifier(modifier);

    auto source = ConstructProperty<int>("source");
    source->SetValue(13);

    EXPECT_TRUE(p->SetBind(source));
    EXPECT_EQ(p->GetValue(), 10);

    source->SetValue(0);
    EXPECT_EQ(p->GetValue(), 1);

    source->SetValue(7);
    EXPECT_EQ(p->GetValue(), 7);

    source->Reset();
    EXPECT_EQ(p->GetValue(), 1);

    auto vals = stack->GetValues(BASE_NS::vector<TypeId>({ IBind::UID }), false);
    EXPECT_EQ(vals.size(), 1);

    auto mods = stack->GetModifiers(BASE_NS::vector<TypeId>({ IModifier::UID }), false);
    ASSERT_EQ(mods.size(), 1);
    EXPECT_EQ(mods[0], modifier);

    p->ResetBind();
    EXPECT_EQ(p->GetValue(), 1);

    stack->RemoveModifier(modifier);

    EXPECT_EQ(p->GetValue(), 0);
}

HWTEST_F(ModifierTest, IncompatibleModifier, TestSize.Level1)
{
    auto modifier = IModifier::Ptr(new ValidRangeEvaluator(1, 10));
    auto p = ConstructProperty<BASE_NS::string>("test");
    EXPECT_EQ(p->AddModifier(modifier).GetError(), GenericError::INCOMPATIBLE_TYPES);
}

META_REGISTER_CLASS(StickyValidator, "11310567-a76c-41c3-af08-81c1adf682ef", ObjectCategory::NO_CATEGORY)

class StickyValidator : public IntroduceInterfaces<MinimalObject, IModifier, IStackResetable> {
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(ClassId::StickyValidator)
public:
    StickyValidator(int value) : allowed_(value) {}

    EvaluationResult ProcessOnGet(IAny& value) override
    {
        return EVAL_CONTINUE;
    }
    EvaluationResult ProcessOnSet(IAny& value, const IAny& current) override
    {
        if (GetValue<int>(value) != allowed_) {
            return EVAL_ERROR;
        }
        return EVAL_CONTINUE;
    }
    bool IsCompatible(const TypeId& id) const override
    {
        return id == TypeId(UidFromType<int>());
    }
    ResetResult ProcessOnReset(const IAny& defaultValue) override
    {
        return RESET_CONTINUE;
    }

private:
    int allowed_;
};

HWTEST_F(ModifierTest, StickyValidator, TestSize.Level1)
{
    auto p = ConstructProperty<int>("test");
    auto stack = p->GetStackProperty();
    ASSERT_TRUE(stack);

    p->SetValue(3);

    ASSERT_TRUE(stack->AddModifier(IModifier::Ptr(new StickyValidator(2))));
    EXPECT_FALSE(p->SetValue(1));
    EXPECT_EQ(p->GetValue(), 3);
    EXPECT_TRUE(p->SetValue(2));
    EXPECT_EQ(p->GetValue(), 2);

    EXPECT_FALSE(p->IsDefaultValue());
    p->ResetValue();
    EXPECT_EQ(p->GetValue(), 0);
    EXPECT_FALSE(p->SetValue(1));
    EXPECT_EQ(p->GetValue(), 0);

    EXPECT_TRUE(p->IsDefaultValue());
    auto mods = stack->GetModifiers(BASE_NS::vector<TypeId>({ IModifier::UID }), false);
    EXPECT_EQ(mods.size(), 1);
}

META_END_NAMESPACE()
