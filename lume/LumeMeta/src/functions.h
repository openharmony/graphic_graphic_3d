/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: Function (IObject+function name) implementation
 * Author: Mikael Kilpeläinen
 * Create: 2023-08-21
 */

#ifndef META_SRC_SETTABLE_FUNCTION_H
#define META_SRC_SETTABLE_FUNCTION_H

#include <meta/base/namespace.h>
#include <meta/interface/intf_function.h>
#include <meta/interface/serialization/intf_serializable.h>

#include "base_object.h"

META_BEGIN_NAMESPACE()

class SettableFunction : public IntroduceInterfaces<BaseObject, ISettableFunction, ISerializable> {
    META_OBJECT(SettableFunction, ClassId::SettableFunction, IntroduceInterfaces)
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

class PropertyFunction : public IntroduceInterfaces<BaseObject, IPropertyFunction, ISerializable, IImportFinalize> {
    META_OBJECT(PropertyFunction, ClassId::PropertyFunction, IntroduceInterfaces)
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
