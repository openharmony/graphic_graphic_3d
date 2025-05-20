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

void AddObjectProperties(const META_NS::IObject& obj, META_NS::IMetadata& out);
void AddFlatProperties(const META_NS::IMetadata& in, META_NS::IObject& parent);

inline void SerCloneAllToDefaultIfSet(const META_NS::IMetadata& in, META_NS::IMetadata& out)
{
    if (auto m = interface_cast<META_NS::IObject>(&in)) {
        AddObjectProperties(*m, out);
    }
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

SCENE_END_NAMESPACE()

#endif