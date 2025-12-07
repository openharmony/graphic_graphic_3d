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

#ifndef SCENE_SRC_SERIALIZATION_UTIL_H
#define SCENE_SRC_SERIALIZATION_UTIL_H

#include <scene/interface/intf_external_node.h>

#include <meta/api/metadata_util.h>

SCENE_BEGIN_NAMESPACE()
// workaround for now to find forwarded properties for scene objects
// notice that this will instantiate all properties
inline BASE_NS::vector<META_NS::IProperty::ConstPtr> GetAllProperties(const META_NS::IMetadata& m)
{
    BASE_NS::vector<META_NS::IProperty::ConstPtr> res;
    for (auto&& d : m.GetAllMetadatas(META_NS::MetadataType::PROPERTY)) {
        if (auto p = m.GetProperty(d.name)) {
            res.push_back(p);
        }
    }
    return res;
}

bool AddObjectProperties(const META_NS::IObject& obj, const BASE_NS::string& head, META_NS::IMetadata& out);
inline bool AddObjectProperties(const META_NS::IObject& obj, META_NS::IMetadata& out)
{
    return AddObjectProperties(obj, "", out);
}

void AddFlatProperties(const META_NS::IMetadata& in, META_NS::IObject& parent);

inline bool SerCloneAllToDefaultIfSet(const META_NS::IMetadata& in, META_NS::IMetadata& out)
{
    auto m = interface_cast<META_NS::IObject>(&in);
    return m ? AddObjectProperties(*m, out) : false;
}

inline void SerCopy(const META_NS::IMetadata& in, META_NS::IMetadata& out)
{
    if (auto m = interface_cast<META_NS::IObject>(&out)) {
        AddFlatProperties(in, *m);
    }
}

constexpr const BASE_NS::string_view ESCAPED_SER_CHARS = "./![]";
constexpr const char ESCAPE_SER_CHAR = '\\';

inline BASE_NS::string EscapeSerName(BASE_NS::string_view str)
{
    BASE_NS::string res { str };
    for (size_t i = 0; i != res.size(); ++i) {
        if (ESCAPED_SER_CHARS.find(res[i]) != BASE_NS::string_view::npos) {
            res.insert(i, &ESCAPE_SER_CHAR, 1);
            ++i;
        }
    }
    return res;
}

inline BASE_NS::string UnescapeSerName(BASE_NS::string_view str)
{
    BASE_NS::string res { str };
    for (size_t i = 0; i < res.size(); ++i) {
        if (res[i] == ESCAPE_SER_CHAR) {
            res.erase(i, 1);
        }
    }
    return res;
}

template<typename Obj>
bool SerialiseAttachment(const Obj& obj)
{
    return !interface_cast<META_NS::IEvent>(obj) && !interface_cast<META_NS::IFunction>(obj) &&
           !interface_cast<META_NS::IProperty>(obj) && !interface_cast<IComponent>(obj) &&
           !interface_cast<IExternalNode>(obj);
}

SCENE_END_NAMESPACE()

#endif