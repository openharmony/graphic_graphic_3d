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

#include "io/proxy_directory.h"

#include <cstdint>
#include <unordered_set>
#include <utility>

#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <base/util/compile_time_hashes.h>
#include <core/io/intf_directory.h>
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
