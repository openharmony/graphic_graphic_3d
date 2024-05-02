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
#include "json_input.h"

#include <base/containers/vector.h>
#include <base/util/uid_util.h>

#include <meta/base/namespace.h>
#include <meta/base/plugin.h>
#include <meta/base/ref_uri.h>
#include <meta/ext/minimal_object.h>

#include "../ser_nodes.h"

META_BEGIN_NAMESPACE()

namespace Serialization {

constexpr Version CURRENT_JSON_VERSION(2, 0);
constexpr int UID_SIZE = 36;

ISerNode::Ptr JsonInput::ImportRef(const json::value& ref)
{
    if (ref.is_string()) {
        RefUri uri;
        // check for backward compatibility
        if (ref.string_.substr(0, 4) == "ref:") {
            uri = RefUri(CORE_NS::json::unescape(ref.string_));
        } else {
            uri.SetBaseObjectUid(BASE_NS::StringToUid(CORE_NS::json::unescape(ref.string_)));
        }
        if (uri.IsValid()) {
            return ISerNode::Ptr(new RefNode(BASE_NS::move(uri)));
        }
    }
    return nullptr;
}

BASE_NS::string ReadString(BASE_NS::string_view name, const json::value& value)
{
    BASE_NS::string str;
    if (const auto& v = value.find(name)) {
        if (v->is_string()) {
            str = CORE_NS::json::unescape(v->string_);
        }
    }
    return str;
}

BASE_NS::Uid ReadUid(BASE_NS::string_view name, const json::value& value)
{
    BASE_NS::Uid uid;
    if (const auto& id = value.find(name)) {
        if (id->is_string() && id->string_.size() == UID_SIZE) {
            uid = BASE_NS::StringToUid(id->string_);
        }
    }
    return uid;
}

static bool IsValidName(const BASE_NS::string_view& name)
{
    return !name.empty() && name[0] != '$';
}

ISerNode::Ptr JsonInput::ImportObject(const json::value& value)
{
    BASE_NS::string className = ReadString(ClassNameName, value);
    BASE_NS::string name = ReadString(NameName, value);
    ObjectId oid = ReadUid(ObjectIdName, value);
    InstanceId iid = ReadUid(InstanceIdName, value);

    BASE_NS::vector<NamedNode> members;
    for (auto&& p : value.object_) {
        auto key = RewriteReservedName(p.key);
        if (!IsValidName(key)) {
            // Skip empty keys and keys starting with $
            continue;
        }
        auto n = Import(p.value);
        if (!n) {
            return nullptr;
        }
        members.push_back(NamedNode { key, BASE_NS::move(n) });
    }
    auto map = CreateShared<MapNode>(BASE_NS::move(members));
    if (oid.IsValid()) {
        return ISerNode::Ptr(
            new ObjectNode(BASE_NS::move(className), BASE_NS::move(name), oid, iid, BASE_NS::move(map)));
    }
    return map;
}

ISerNode::Ptr JsonInput::ImportArray(const json::value::array& arr)
{
    BASE_NS::vector<ISerNode::Ptr> nodes;
    nodes.reserve(arr.size());
    for (auto&& value : arr) {
        if (auto n = Import(value)) {
            nodes.emplace_back(BASE_NS::move(n));
        } else {
            return nullptr;
        }
    }
    return ISerNode::Ptr(new ArrayNode(BASE_NS::move(nodes)));
}

ISerNode::Ptr JsonInput::Import(const json::value& value)
{
    switch (value.type) {
        case json::type::boolean:
            return ISerNode::Ptr(new BoolNode(value.boolean_));
        case json::type::floating_point:
            return ISerNode::Ptr(new DoubleNode(value.float_));
        case json::type::signed_int:
            return ISerNode::Ptr(new IntNode(value.signed_));
        case json::type::unsigned_int:
            return ISerNode::Ptr(new UIntNode(value.unsigned_));
        case json::type::string:
            return ISerNode::Ptr(new StringNode(CORE_NS::json::unescape(value.string_)));
        case json::type::object:
            if (auto ref = value.find("$ref")) {
                return ImportRef(*ref);
            }
            return ImportObject(value);
        case json::type::array:
            return ImportArray(value.array_);
        case json::type::null:
            return ISerNode::Ptr(new NilNode);
        default:
            CORE_ASSERT_MSG(false, "Unhandled primitive type in Json input");
            return nullptr;
    }
}

bool JsonInput::ReadMetadata(const json::value& value)
{
    if (auto v = value.find("version")) {
        if (v->is_string()) {
            auto ver = Version(CORE_NS::json::unescape(v->string_));
            if (ver == Version()) {
                CORE_LOG_E(
                    "Invalid file version: %s != %s", ver.ToString().c_str(), CURRENT_JSON_VERSION.ToString().c_str());
                return false;
            }
            jsonVersion_ = ver;
        }
    }
    if (auto v = value.find("exporter-version")) {
        if (v->is_string()) {
            exporterVersion_ = Version(CORE_NS::json::unescape(v->string_));
        }
    }
    return true;
}

ISerNode::Ptr JsonInput::ImportRootObject(const json::value& value)
{
    // see if we have meta data
    if (auto v = value.find("$meta")) {
        if (v->is_object() && !ReadMetadata(*v)) {
            return nullptr;
        }
    }

    json::value root;

    // is it legacy version?
    if (jsonVersion_ == Version {}) {
        jsonVersion_ = Version { 1, 0 };
        root = value;
    } else if (auto v = value.find("$root")) {
        if (v->is_object()) {
            root = *v;
        }
    }
    if (jsonVersion_ < Version(2, 0)) {
        SetMetaV1Compatibility();
    }

    auto obj = Import(root);
    return obj ? ISerNode::Ptr(new RootNode(exporterVersion_, obj)) : nullptr;
}

ISerNode::Ptr JsonInput::Process(BASE_NS::string_view data)
{
    ISerNode::Ptr res;
    auto json = CORE_NS::json::parse(data.data());
    if (json.is_object()) {
        res = ImportRootObject(json);
    }
    return res;
}

void JsonInput::SetMetaV1Compatibility()
{
    CORE_LOG_I("Enabling Meta Object Version 1 compatibility");
    ClassNameName = "$name";
    NameName = "";
    RewriteReservedName = [](auto name) {
        if (name == "$properties") {
            return BASE_NS::string("__properties");
        }
        if (name == "$flags") {
            return BASE_NS::string("__flags");
        }
        return BASE_NS::string(name);
    };
}

} // namespace Serialization

META_END_NAMESPACE()
