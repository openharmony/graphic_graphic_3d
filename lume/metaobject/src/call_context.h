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

#ifndef META_SRC_CALL_CONTEXT_H
#define META_SRC_CALL_CONTEXT_H

#include <base/containers/vector.h>

#include <meta/interface/intf_call_context.h>
#include <meta/interface/intf_cloneable.h>
#include <meta/interface/object_macros.h>

META_BEGIN_NAMESPACE()

class DefaultCallContext : public ICallContext, protected ICloneable {
    META_IMPLEMENT_REF_COUNT()
public:
    ~DefaultCallContext() override;
    DefaultCallContext();
    DefaultCallContext(const DefaultCallContext& other) noexcept;
    DefaultCallContext(DefaultCallContext&& other) noexcept;

    DefaultCallContext& operator=(const DefaultCallContext& other) noexcept;
    DefaultCallContext& operator=(DefaultCallContext&& other) noexcept;

    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override;

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
