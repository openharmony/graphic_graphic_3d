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
#define JSON_IMPL
#include "json_output.h"

#include <base/containers/vector.h>
#include <base/util/uid_util.h>
#include <core/json/json.h>

#include <meta/base/namespace.h>
#include <meta/base/plugin.h>
#include <meta/base/ref_uri.h>
#include <meta/ext/minimal_object.h>

META_BEGIN_NAMESPACE()

namespace Serialization {

constexpr Version CURRENT_JSON_VERSION(2, 0);

META_REGISTER_CLASS(Visitor, "2838202a-b362-4715-96ed-59f19334b3ac", ObjectCategoryBits::NO_CATEGORY)

struct Visitor : MinimalObject<ClassId::Visitor, ISerNodeVisitor> {
    json_value GetValue()
    {
        return BASE_NS::move(node_);
    }

private:
    void Visit(const IRootNode&) override
    {
        CORE_LOG_E("Second root node, ignoring...");
    }
    void Visit(const INilNode&) override
    {
        node_ = json_value::null {};
    }
    void Visit(const IObjectNode& n) override
    {
        json_value::object object;
        if (n.GetObjectId().IsValid() && n.GetMembers()) {
            Visitor v;
            n.GetMembers()->Apply(v);
            auto value = v.GetValue();
            if (value.is_object()) {
                object.emplace_back("$classId", json_value::string(n.GetObjectId().ToString()));
                if (!n.GetObjectClassName().empty()) {
                    object.emplace_back("$className", json_value::string(n.GetObjectClassName()));
                }
                if (!n.GetObjectName().empty() && n.GetObjectName() != n.GetInstanceId().ToString()) {
                    object.emplace_back("$name", json_value::string(n.GetObjectName()));
                }
                if (n.GetInstanceId().IsValid()) {
                    object.emplace_back("$instanceId", json_value::string(n.GetInstanceId().ToString()));
                }
                json_value::object obj = value.object_;
                object.insert(object.end(), obj.begin(), obj.end());
            }
        }
        node_ = BASE_NS::move(object);
    }
    void Visit(const IArrayNode& n) override
    {
        auto members = n.GetMembers();
        json_value::array array;
        array.reserve(members.size());
        for (auto&& m : members) {
            Visitor v;
            m->Apply(v);
            array.emplace_back(v.GetValue());
        }
        node_ = BASE_NS::move(array);
    }
    void Visit(const IMapNode& n) override
    {
        auto members = n.GetMembers();
        json_value::object object;
        for (auto&& m : members) {
            Visitor v;
            m.node->Apply(v);
            object.emplace_back(BASE_NS::move(m.name), v.GetValue());
        }
        node_ = BASE_NS::move(object);
    }
    void Visit(const IBuiltinValueNode<bool>& n) override
    {
        node_ = json_value { n.GetValue() };
    }
    void Visit(const IBuiltinValueNode<double>& n) override
    {
        node_ = json_value { n.GetValue() };
    }
    void Visit(const IBuiltinValueNode<int64_t>& n) override
    {
        node_ = json_value { n.GetValue() };
    }
    void Visit(const IBuiltinValueNode<uint64_t>& n) override
    {
        node_ = json_value { n.GetValue() };
    }
    void Visit(const IBuiltinValueNode<BASE_NS::string>& n) override
    {
        node_ = json_value { n.GetValue() };
    }
    void Visit(const IBuiltinValueNode<RefUri>& n) override
    {
        json_value::object object;
        object.emplace_back("$ref", n.GetValue().ToString());
        node_ = BASE_NS::move(object);
    }
    void Visit(const ISerNode&) override
    {
        CORE_LOG_E("Unknown node type");
    }

private:
    json_value node_;
};

json_value MetadataObject(const IRootNode& root)
{
    json_value::object object;
    object.emplace_back("meta-version", json_value::string(META_VERSION.ToString()));
    object.emplace_back("version", json_value::string(CURRENT_JSON_VERSION.ToString()));
    object.emplace_back("exporter-version", json_value::string(root.GetSerializerVersion().ToString()));
    return object;
}

BASE_NS::string JsonOutput::Process(const ISerNode::Ptr& tree)
{
    CORE_NS::json::standalone_value res;

    if (auto root = interface_cast<IRootNode>(tree)) {
        if (root->GetObject()) {
            json_value::object object;
            object.emplace_back("$meta", MetadataObject(*root));

            Visitor v;
            root->GetObject()->Apply(v);
            auto node = v.GetValue();
            if (node.is_object()) {
                object.emplace_back("$root", BASE_NS::move(node));
                res = BASE_NS::move(object);
            } else {
                CORE_LOG_E("failed to output root node object");
            }
        } else {
            CORE_LOG_E("root node did not contain object");
        }
    }
    return CORE_NS::json::to_string(res);
}

} // namespace Serialization

META_END_NAMESPACE()
