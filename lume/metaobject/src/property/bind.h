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
#ifndef META_SRC_PROPERTY_BIND_H
#define META_SRC_PROPERTY_BIND_H

#include <meta/interface/property/intf_bind.h>

#include "../base_object.h"
#include "stack_property.h"

META_BEGIN_NAMESPACE()
namespace Internal {

class Bind : public Internal::BaseObjectFwd<Bind, META_NS::ClassId::Bind, IValue, IBind, INotifyOnChange, ISerializable,
                 IImportFinalize> {
    using Super = Internal::BaseObjectFwd<Bind, META_NS::ClassId::Bind, IValue, IBind, INotifyOnChange, ISerializable,
        IImportFinalize>;

public:
    META_NO_COPY_MOVE(Bind)

    Bind() = default;
    ~Bind() override;

    AnyReturnValue SetValue(const IAny& value) override;
    const IAny& GetValue() const override;
    bool IsCompatible(const TypeId& id) const override;

    bool SetTarget(const IProperty::ConstPtr& prop, bool getDeps, const IProperty* owner) override;
    bool SetTarget(const IFunction::ConstPtr& func, bool getDeps, const IProperty* owner) override;
    IFunction::ConstPtr GetTarget() const override;

    bool AddDependency(const INotifyOnChange::ConstPtr& dep) override;
    bool RemoveDependency(const INotifyOnChange::ConstPtr& dep) override;
    BASE_NS::vector<INotifyOnChange::ConstPtr> GetDependencies() const override;

    BASE_NS::shared_ptr<IEvent> EventOnChanged() const override;

private:
    ReturnError Export(IExportContext&) const override;
    ReturnError Import(IImportContext&) override;
    ReturnError Finalize(IImportFunctions&) override;

    bool CreateContext(bool eval, const IProperty* owner);

    void Reset();

private:
    BASE_NS::shared_ptr<EventImpl<IOnChanged>> event_ { new EventImpl<IOnChanged>("OnChanged") };
    ICallContext::Ptr context_;
    IFunction::ConstPtr func_;
    BASE_NS::vector<INotifyOnChange::ConstWeakPtr> dependencies_;
};

} // namespace Internal
META_END_NAMESPACE()

#endif