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

#define JSON2_IMPL
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

namespace json = CORE_NS::json2;

constexpr Version CURRENT_JSON_VERSION(2, 0);
constexpr int UID_SIZE = 36;

ISerNode::Ptr JsonInput::ImportRef(const json::view& ref)
{
    if (ref.is_string()) {
        RefUri uri;
        // as_string() returns the unescaped value
        const auto str = ref.as_string();
        // check for backward compatibility
        if (str.substr(0, 4) == "ref:") {
            uri = RefUri(str);
        } else {
            uri.SetBaseObjectUid(BASE_NS::StringToUid(str));
        }
        if (uri.IsValid()) {
            return ISerNode::Ptr(new RefNode(BASE_NS::move(uri)));
        }
    }
    return nullptr;
}

BASE_NS::string ReadString(BASE_NS::string_view name, const json::view& value)
{
    BASE_NS::string str;
    if (const auto v = value.find(name)) {
        if (v->is_string()) {
            str = v->as_string();
        }
    }
    return str;
}

BASE_NS::Uid ReadUid(BASE_NS::string_view name, const json::view& value)
{
    BASE_NS::Uid uid;
    if (const auto id = value.find(name)) {
        if (id->is_string()) {
            const auto str = id->as_string();
            if (str.size() == UID_SIZE) {
                uid = BASE_NS::StringToUid(str);
            }
        }
    }
    return uid;
}

static bool IsValidName(const BASE_NS::string_view& name)
{
    return !name.empty() && name[0] != '$';
}

ISerNode::Ptr JsonInput::ImportObject(const json::view& value)
{
    BASE_NS::string className = ReadString(ClassNameName, value);
    BASE_NS::string name = ReadString(NameName, value);
    ObjectId oid = ReadUid(ObjectIdName, value);
    InstanceId iid = ReadUid(InstanceIdName, value);

    BASE_NS::vector<NamedNode> members;
    for (auto&& p : value.as_object()) {
        auto key = RewriteReservedName(p.escapedKey.escaped);
        if (!IsValidName(key)) {
            // Skip empty keys and keys starting with $
            continue;
        }
        auto n = Import(p.value);
        if (!n) {
            return nullptr;
        }
        members.push_back(NamedNode{key, BASE_NS::move(n)});
    }
    auto map = CreateShared<MapNode>(BASE_NS::move(members));
    if (oid.IsValid()) {
        return ISerNode::Ptr(
            new ObjectNode(BASE_NS::move(className), BASE_NS::move(name), oid, iid, BASE_NS::move(map)));
    }
    return map;
}

ISerNode::Ptr JsonInput::ImportArray(const json::view::array_t& arr)
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

ISerNode::Ptr JsonInput::Import(const json::view& value)
{
    static constexpr uint32_t MAX_DEPTH = 256u;

    if (++depth_ >= MAX_DEPTH) {
        CORE_LOG_E("Maximum JSON hierarchy depth exceeded");
        return {};
    }
    ISerNode::Ptr result;

    switch (value.type()) {
        case json::type::boolean:
            result = ISerNode::Ptr(new BoolNode(value.as_boolean()));
            break;
        case json::type::floating_point:
            result = ISerNode::Ptr(new DoubleNode(value.as_strict_floating_point()));
            break;
        case json::type::signed_int:
            result = ISerNode::Ptr(new IntNode(value.as_strict_signed_int()));
            break;
        case json::type::unsigned_int:
            result = ISerNode::Ptr(new UIntNode(value.as_strict_unsigned_int()));
            break;
        case json::type::string:
            result = ISerNode::Ptr(new StringNode(value.as_string()));
            break;
        case json::type::object:
            if (auto ref = value.find("$ref")) {
                result = ImportRef(*ref);
            } else {
                result = ImportObject(value);
            }
            break;
        case json::type::array:
            result = ImportArray(value.as_array());
            break;
        case json::type::null:
            result = ISerNode::Ptr(new NilNode);
            break;
        default:
            CORE_ASSERT_MSG(false, "Unhandled primitive type in Json input");
            break;
    }
    depth_--;
    return result;
}

bool JsonInput::ReadMetadata(const json::view& value)
{
    if (auto v = value.find("meta-version")) {
        if (v->is_string()) {
            auto ver = Version(v->as_string());
            if (ver == Version()) {
                CORE_LOG_E(
                    "Invalid file version: %s != %s", ver.ToString().c_str(), CURRENT_JSON_VERSION.ToString().c_str());
                return false;
            }
            metaVersion_ = ver;
        }
    }
    if (metaVersion_ != Version()) {
        for (auto&& v : value.as_object()) {
            if (v.value.is_string()) {
                metadata_.push_back(SerMetadataEntity{BASE_NS::string(v.escapedKey.escaped), v.value.as_string()});
            }
        }
    } else {
        if (auto v = value.find("version")) {
            if (v->is_string() && Version(v->as_string()) == Version(1, 0)) {
                metaVersion_ = Version(1, 0);
            }
        }
    }
    return true;
}

ISerNode::Ptr JsonInput::ImportRootObject(const json::view& value)
{
    // see if we have meta data
    if (auto v = value.find("$meta")) {
        if (v->is_object() && !ReadMetadata(*v)) {
            return nullptr;
        }
    }

    json::view root;

    // is it legacy version?
    if (metaVersion_ == Version{}) {
        metaVersion_ = Version{1, 0};
        root = value;
    } else if (auto v = value.find("$root")) {
        if (v->is_object()) {
            root = *v;
        }
    }
    if (metaVersion_ < Version(2, 0)) {
        SetMetaV1Compatibility();
    }

    auto obj = Import(root);
    return obj ? ISerNode::Ptr(new RootNode(obj, metadata_)) : nullptr;
}

ISerNode::Ptr JsonInput::Process(BASE_NS::string_view data)
{
    depth_ = 0;
    ISerNode::Ptr res;
    auto json = json::parse(data);
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

}  // namespace Serialization

META_END_NAMESPACE()
