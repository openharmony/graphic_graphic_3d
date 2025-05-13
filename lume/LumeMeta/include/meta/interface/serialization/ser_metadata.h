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

#ifndef META_INTERFACE_SERIALIZATION_SER_METADATA_H
#define META_INTERFACE_SERIALIZATION_SER_METADATA_H

#include <meta/base/namespace.h>
#include <meta/base/version.h>

META_BEGIN_NAMESPACE()

struct SerMetadataEntity {
    BASE_NS::string key;
    BASE_NS::string data;
};
using SerMetadata = BASE_NS::vector<SerMetadataEntity>;

struct SerMetadataValues : SerMetadata {
    SerMetadataValues() = default;
    SerMetadataValues(SerMetadata m) : SerMetadata(BASE_NS::move(m)) {}

    SerMetadataValues& Set(BASE_NS::string_view key, BASE_NS::string_view data)
    {
        for (auto&& v : *this) {
            if (v.key == key) {
                v.data = data;
                return *this;
            }
        }
        push_back({ BASE_NS::string(key), BASE_NS::string(data) });
        return *this;
    }
    BASE_NS::string_view Get(BASE_NS::string_view key) const
    {
        for (auto&& v : *this) {
            if (v.key == key) {
                return v.data;
            }
        }
        return {};
    }
    SerMetadataValues& SetVersion(const Version& ver)
    {
        return Set("version", ver.ToString());
    }
    Version GetVersion() const
    {
        auto res = Get("version");
        return !res.empty() ? Version(res) : Version();
    }
    SerMetadataValues& SetType(BASE_NS::string_view type)
    {
        return Set("type", type);
    }
    BASE_NS::string GetType() const
    {
        return BASE_NS::string(Get("type"));
    }
};

META_END_NAMESPACE()

#endif
