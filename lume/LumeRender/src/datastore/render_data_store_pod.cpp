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

#include "render_data_store_pod.h"

#include <algorithm>
#include <cstdint>

#include <base/containers/array_view.h>
#include <render/namespace.h>

#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
RenderDataStorePod::RenderDataStorePod(const string_view name) : name_(name) {}

void RenderDataStorePod::CreatePod(
    const string_view tpName, const string_view name, const array_view<const uint8_t> srcData)
{
    if (srcData.empty()) {
        PLUGIN_LOG_W("Zero size pod is not created (name: %s)", tpName.data());
        return;
    }
    bool contains = false;
    {
        const auto lock = std::lock_guard(mutex_);

        contains = nameToDataOffset_.contains(name);
        if (!contains) {
            const size_t prevByteSize = dataStore_.size();
            const size_t newByteSize = prevByteSize + srcData.size();
            dataStore_.resize(newByteSize);
            uint8_t* dst = &dataStore_[prevByteSize];
            if (!CloneData(dst, dataStore_.size() - prevByteSize, srcData.data(), srcData.size())) {
                PLUGIN_LOG_E("Copying of RenderDataStorePod Data failed.");
            }

            auto& typeNameRef = typeNameToPodNames_[tpName];
            nameToDataOffset_[name] = { static_cast<uint32_t>(prevByteSize), static_cast<uint32_t>(srcData.size()) };
            typeNameRef.emplace_back(name);
        }
    } // end of lock

    // already created, set data
    if (contains) {
        PLUGIN_LOG_I("updating already created pod: %s", name.data());
        Set(name, srcData);
    }
}

void RenderDataStorePod::DestroyPod(const string_view typeName, const string_view name)
{
    const auto lock = std::lock_guard(mutex_);

    if (const auto iter = nameToDataOffset_.find(name); iter != nameToDataOffset_.end()) {
        const auto offsetToData = iter->second;
        PLUGIN_ASSERT(offsetToData.index + offsetToData.byteSize <= (uint32_t)dataStore_.size());
        const auto first = dataStore_.cbegin() + static_cast<int32_t>(offsetToData.index);
        const auto last = first + static_cast<int32_t>(offsetToData.byteSize);
        dataStore_.erase(first, last);
        nameToDataOffset_.erase(iter);
        // move the index of pods after the destroyed pod by the size of the pod
        for (auto& nameToOffset : nameToDataOffset_) {
            if (nameToOffset.second.index > offsetToData.index) {
                nameToOffset.second.index -= offsetToData.byteSize;
            }
        }
        if (auto tpIter = typeNameToPodNames_.find(typeName); tpIter != typeNameToPodNames_.end()) {
            for (auto nameIter = tpIter->second.cbegin(); nameIter != tpIter->second.cend(); ++nameIter) {
                if (*nameIter == name) {
                    tpIter->second.erase(nameIter);
                    break;
                }
            }
        }
    } else {
        PLUGIN_LOG_I("pod not found: %s", name.data());
    }
}

void RenderDataStorePod::Set(const string_view name, const array_view<const uint8_t> srcData)
{
    const auto lock = std::lock_guard(mutex_);

    const auto iter = nameToDataOffset_.find(name);
    if (iter != nameToDataOffset_.cend()) {
        const uint32_t index = iter->second.index;
        const uint32_t byteSize = iter->second.byteSize;
        PLUGIN_ASSERT(index + byteSize <= (uint32_t)dataStore_.size());
        PLUGIN_ASSERT(srcData.size() <= byteSize);

        const uint32_t maxByteSize = std::min(byteSize, static_cast<uint32_t>(srcData.size()));

        uint8_t* dst = &dataStore_[index];
        if (!CloneData(dst, dataStore_.size() - index, srcData.data(), maxByteSize)) {
            PLUGIN_LOG_E("Copying of RenderDataStorePod Data failed.");
        }
    } else {
        PLUGIN_LOG_E("render data store pod: %s not created", name.data());
    }
}

array_view<const uint8_t> RenderDataStorePod::Get(const string_view name) const
{
    const auto lock = std::lock_guard(mutex_);

    array_view<const uint8_t> view;
    const auto iter = nameToDataOffset_.find(name);
    if (iter != nameToDataOffset_.cend()) {
        const uint32_t index = iter->second.index;
        const uint32_t byteSize = iter->second.byteSize;
        PLUGIN_ASSERT(index + byteSize <= (uint32_t)dataStore_.size());

        const uint8_t* data = &dataStore_[index];
        view = array_view<const uint8_t>(data, byteSize);
    }
    return view;
}

array_view<const string> RenderDataStorePod::GetPodNames(const string_view tpName) const
{
    const auto lock = std::lock_guard(mutex_);

    const auto iter = typeNameToPodNames_.find(tpName);
    if (iter != typeNameToPodNames_.cend()) {
        return array_view<const string>(iter->second.data(), iter->second.size());
    } else {
        PLUGIN_LOG_I("render data store pod type (%s), not found", tpName.data());
        return {};
    }
}

// for plugin / factory interface
IRenderDataStore* RenderDataStorePod::Create(IRenderContext&, char const* name)
{
    // engine not used
    return new RenderDataStorePod(name);
}

void RenderDataStorePod::Destroy(IRenderDataStore* aInstance)
{
    delete static_cast<RenderDataStorePod*>(aInstance);
}
RENDER_END_NAMESPACE()
