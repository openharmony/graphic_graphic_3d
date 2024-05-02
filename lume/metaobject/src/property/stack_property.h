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
#ifndef META_SRC_STACK_PROPERTY_H
#define META_SRC_STACK_PROPERTY_H

#include <atomic>

#include <meta/api/make_callback.h>
#include <meta/interface/property/intf_stack_property.h>
#include <meta/interface/serialization/intf_serializable.h>

#include "property.h"

META_BEGIN_NAMESPACE()
namespace Internal {

class StackProperty : public IntroduceInterfaces<GenericProperty, IStackProperty, ISerializable> {
    using Super = IntroduceInterfaces<GenericProperty, IStackProperty, ISerializable>;

public:
    META_NO_COPY_MOVE(StackProperty)

    explicit StackProperty(BASE_NS::string name);
    ~StackProperty() override;

    AnyReturnValue SetValue(const IAny& value) override;
    const IAny& GetValue() const override;

    ReturnError PushValue(const IValue::Ptr& value) override;
    ReturnError PopValue() override;
    IValue::Ptr TopValue() const override;
    ReturnError RemoveValue(const IValue::Ptr& value) override;
    BASE_NS::vector<IValue::Ptr> GetValues(const BASE_NS::array_view<const TypeId>& ids, bool strict) const override;

    ReturnError InsertModifier(IndexType pos, const IModifier::Ptr& mod) override;
    IModifier::Ptr RemoveModifier(IndexType pos) override;
    ReturnError RemoveModifier(const IModifier::Ptr& mod) override;
    BASE_NS::vector<IModifier::Ptr> GetModifiers(
        const BASE_NS::array_view<const TypeId>& ids, bool strict) const override;

    AnyReturnValue SetDefaultValue(const IAny& value) override;
    const IAny& GetDefaultValue() const override;

    AnyReturnValue SetInternalAny(IAny::Ptr any) override;
    void NotifyChange() const override;
    bool IsDefaultValue() const override;
    void ResetValue() override;
    void RemoveAll() override;

    ReturnError Export(IExportContext&) const override;
    ReturnError Import(IImportContext&) override;

protected:
    AnyReturnValue SetValueInValueStack(const IAny& value);
    AnyReturnValue SetValueToStack(const IAny::Ptr& internal);
    const IAny& GetValueFromStack() const;
    const IAny& RawGetValue() const;
    void CleanUp();

    template<typename Vec>
    bool ProcessResetables(Vec& vec);

    ObjectId GetClassId() const override
    {
        return META_NS::ClassId::StackProperty.Id();
    }

    void InternalOnChanged()
    {
        requiresEvaluation_ = true;
        if (auto p = onChangedAtomic_.load()) {
            p->Invoke();
        }
    }

private:
    mutable std::atomic<bool> requiresEvaluation_ {};
    IAny::Ptr currentValue_;
    IAny::Ptr defaultValue_;
    BASE_NS::vector<IValue::Ptr> values_;
    BASE_NS::vector<IModifier::Ptr> modifiers_;
    ICallable::Ptr onChangedCallback_;
    mutable bool evaluating_ {};
};

} // namespace Internal
META_END_NAMESPACE()

#endif
