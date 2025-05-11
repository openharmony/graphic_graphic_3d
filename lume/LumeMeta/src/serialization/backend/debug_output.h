/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: Debug output serialisation tree
 * Author: Mikael Kilpeläinen
 * Create: 2024-01-05
 */

#ifndef META_SRC_SERIALIZATION_BACKEND_DEBUG_OUTPUT_H
#define META_SRC_SERIALIZATION_BACKEND_DEBUG_OUTPUT_H

#include <meta/base/namespace.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/serialization/intf_ser_output.h>

#include "../../base_object.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

class DebugOutput : public IntroduceInterfaces<BaseObject, ISerOutput, ISerNodeVisitor> {
    META_OBJECT(DebugOutput, ClassId::DebugOutput, IntroduceInterfaces)
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
