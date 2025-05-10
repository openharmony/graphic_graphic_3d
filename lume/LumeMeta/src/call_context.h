/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: Call Context impl
 * Author: Mikael Kilpeläinen
 * Create: 2023-04-17
 */

#ifndef META_SRC_CALL_CONTEXT_H
#define META_SRC_CALL_CONTEXT_H

#include <base/containers/vector.h>

#include <meta/base/namespace.h>
#include <meta/ext/implementation_macros.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_call_context.h>
#include <meta/interface/intf_cloneable.h>
#include <meta/interface/object_macros.h>

META_BEGIN_NAMESPACE()

class DefaultCallContext : public IntroduceInterfaces<ICallContext, ICloneable> {
public:
    ~DefaultCallContext() override;
    DefaultCallContext();
    DefaultCallContext(const DefaultCallContext& other) noexcept;
    DefaultCallContext(DefaultCallContext&& other) noexcept;

    DefaultCallContext& operator=(const DefaultCallContext& other) noexcept;
    DefaultCallContext& operator=(DefaultCallContext&& other) noexcept;

    bool DefineParameter(BASE_NS::string_view name, const IAny::Ptr& value) override;
    bool Set(BASE_NS::string_view name, const IAny& value) override;
    IAny::Ptr Get(BASE_NS::string_view name) const override;

    BASE_NS::array_view<const ArgumentNameValue> GetParameters() const override;

    bool Succeeded() const override;

    bool DefineResult(const IAny::Ptr& value) override;
    bool SetResult(const IAny& value) override;
    bool SetResult() override;
    IAny::Ptr GetResult() const override;
    void Reset() override;

    void ReportError(BASE_NS::string_view error) override;

    BASE_NS::shared_ptr<CORE_NS::IInterface> GetClone() const override;

private:
    BASE_NS::vector<ArgumentNameValue> params_;
    bool succeeded_ {};
    IAny::Ptr result_;
};

META_END_NAMESPACE()

#endif
