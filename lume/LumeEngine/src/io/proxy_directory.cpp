/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "io/proxy_directory.h"

#include <unordered_set>
#include <utility>

#include <base/util/compile_time_hashes.h>
#include <core/namespace.h>

BASE_BEGIN_NAMESPACE()
template<>
uint64_t hash(const CORE_NS::IDirectory::Entry::Type& val)
{
    return static_cast<uint64_t>(val);
}
BASE_END_NAMESPACE()

template<>
struct std::hash<CORE_NS::IDirectory::Entry> {
    using argument_type = CORE_NS::IDirectory::Entry;
    using result_type = uint64_t;
    result_type operator()(argument_type const& s) const noexcept
    {
        return BASE_NS::Hash(s.type, s.name);
    }
};

CORE_BEGIN_NAMESPACE()
using BASE_NS::move;
using BASE_NS::vector;

bool operator==(const IDirectory::Entry& lhs, const IDirectory::Entry& rhs)
{
    return lhs.type == rhs.type && lhs.name == rhs.name;
}

ProxyDirectory::ProxyDirectory(vector<IDirectory::Ptr>&& directories) : directories_(move(directories)) {}

void ProxyDirectory::Close() {}

vector<IDirectory::Entry> ProxyDirectory::GetEntries() const
{
    vector<IDirectory::Entry> result;
    std::unordered_set<IDirectory::Entry> entries;
    for (auto const& dir : directories_) {
        for (auto&& entry : dir->GetEntries()) {
            entries.insert(entry);
        }
    }
    result.reserve(entries.size());
    // std::unordered_set cannot really move unless the hash type and value type are the same. Should playaround with
    // BASE_NS::unordered_map to see if it would suite better.
    std::move(entries.begin(), entries.end(), std::back_inserter(result));
    return result;
}
CORE_END_NAMESPACE()
