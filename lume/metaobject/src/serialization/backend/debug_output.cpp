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
#include "debug_output.h"

#include <string>

META_BEGIN_NAMESPACE()
namespace Serialization {

BASE_NS::string DebugOutput::Process(const ISerNode::Ptr& tree)
{
    tree->Apply(*this);
    return result_;
}

void DebugOutput::Visit(const IRootNode& n)
{
    n.GetObject()->Apply(*this);
}
void DebugOutput::Visit(const INilNode& n)
{
    Output("<null>");
}
void DebugOutput::Visit(const IObjectNode& n)
{
    IncOutputIndent();
    Output("Object [name=" + n.GetObjectName() + "]\n");
    IncOutputIndent();
    n.GetMembers()->Apply(*this);
    DecIndentEndLine();
    DecIndentEndLine();
}
void DebugOutput::Visit(const IArrayNode& n)
{
    bool first = true;
    for (auto&& v : n.GetMembers()) {
        if (!first) {
            NewLine();
            first = false;
        }
        v->Apply(*this);
    }
}
void DebugOutput::Visit(const IMapNode& n)
{
    bool first = true;
    for (auto&& v : n.GetMembers()) {
        if (!first) {
            NewLine();
            first = false;
        }
        Output(v.name + ": ");
        v.node->Apply(*this);
    }
}
void DebugOutput::Visit(const IBuiltinValueNode<bool>& n)
{
    Output(std::to_string(n.GetValue()).c_str());
}
void DebugOutput::Visit(const IBuiltinValueNode<double>& n)
{
    Output(std::to_string(n.GetValue()).c_str());
}
void DebugOutput::Visit(const IBuiltinValueNode<int64_t>& n)
{
    Output(std::to_string(n.GetValue()).c_str());
}
void DebugOutput::Visit(const IBuiltinValueNode<uint64_t>& n)
{
    Output(std::to_string(n.GetValue()).c_str());
}
void DebugOutput::Visit(const IBuiltinValueNode<BASE_NS::string>& n)
{
    Output(n.GetValue());
}
void DebugOutput::Visit(const IBuiltinValueNode<RefUri>& n)
{
    Output(n.GetValue().ToString());
}
void DebugOutput::Visit(const ISerNode& n)
{
    result_ += "<Unknown node type>\n";
}
void DebugOutput::Output(const BASE_NS::string& v)
{
    result_ += v;
}
void DebugOutput::NewLine()
{
    result_ += "\n" + BASE_NS::string(indent_, '\t');
}
void DebugOutput::IncOutputIndent()
{
    ++indent_;
    result_ += BASE_NS::string(indent_, '\t');
}
void DebugOutput::DecIndentEndLine()
{
    --indent_;
    result_ += "\n";
}

} // namespace Serialization
META_END_NAMESPACE()