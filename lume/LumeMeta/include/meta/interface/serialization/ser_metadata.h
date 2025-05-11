/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: Serialisation metadata
 * Author: Mikael Kilpeläinen
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
