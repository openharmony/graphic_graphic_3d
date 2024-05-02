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

#ifndef META_SRC_SERIALIZATION_BACKEND_DEBUG_OUTPUT_H
#define META_SRC_SERIALIZATION_BACKEND_DEBUG_OUTPUT_H

#include <meta/base/namespace.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/serialization/intf_ser_output.h>

#include "../../base_object.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

class DebugOutput : public Internal::BaseObjectFwd<DebugOutput, ClassId::DebugOutput, ISerOutput, ISerNodeVisitor> {
public:
    BASE_NS::string Process(const ISerNode::Ptr& tree) override;

private:
    void Visit(const IRootNode&) override;
    void Visit(const INilNode&) override;
    void Visit(const IObjectNode&) override;
    void Visit(const IArrayNode&) override;
    void Visit(const IMapNode&) override;
    void Visit(const IBuiltinValueNode<bool>&) override;
    void Visit(const IBuiltinValueNode<double>&) override;
    void Visit(const IBuiltinValueNode<int64_t>&) override;
    void Visit(const IBuiltinValueNode<uint64_t>&) override;
    void Visit(const IBuiltinValueNode<BASE_NS::string>&) override;
    void Visit(const IBuiltinValueNode<RefUri>&) override;
    void Visit(const ISerNode&) override;

    void Output(const BASE_NS::string& v);
    void IncOutputIndent();
    void DecIndentEndLine();
    void NewLine();

private:
    size_t indent_ {};
    BASE_NS::string result_;
};

} // namespace Serialization
META_END_NAMESPACE()

#endif
