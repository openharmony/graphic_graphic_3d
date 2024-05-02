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

#ifndef META_SRC_SETTABLE_FUNCTION_H
#define META_SRC_SETTABLE_FUNCTION_H

#include <meta/interface/intf_function.h>
#include <meta/interface/serialization/intf_serializable.h>

#include "base_object.h"

META_BEGIN_NAMESPACE()

class SettableFunction
    : public Internal::BaseObjectFwd<SettableFunction, ClassId::SettableFunction, ISettableFunction, ISerializable> {
public: // IFunction
    BASE_NS::string GetName() const override;
    IObject::ConstPtr GetDestination() const override;
    void Invoke(const ICallContext::Ptr& context) const override;
    ICallContext::Ptr CreateCallContext() const override;

    bool SetTarget(const IObject::Ptr& obj, BASE_NS::string_view name) override;

private:
    bool ResolveFunction(const IObject::Ptr& obj, BASE_NS::string_view name);

    ReturnError Export(IExportContext&) const override;
    ReturnError Import(IImportContext&) override;

private:
    IFunction::ConstWeakPtr func_;
};

class PropertyFunction : public Internal::BaseObjectFwd<PropertyFunction, ClassId::PropertyFunction, IPropertyFunction,
                             ISerializable, IImportFinalize> {
public: // IFunction
    BASE_NS::string GetName() const override;
    IObject::ConstPtr GetDestination() const override;
    void Invoke(const ICallContext::Ptr& context) const override;
    ICallContext::Ptr CreateCallContext() const override;

    bool SetTarget(const IProperty::ConstPtr& prop) override;

    ReturnError Export(IExportContext&) const override;
    ReturnError Import(IImportContext&) override;

    ReturnError Finalize(IImportFunctions&) override;

private:
    RefUri uri_;
    IProperty::ConstWeakPtr prop_;
};

META_END_NAMESPACE()

#endif
