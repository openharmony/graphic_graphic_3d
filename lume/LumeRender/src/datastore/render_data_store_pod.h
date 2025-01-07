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

#ifndef RENDER_DATA_STORE_RENDER_DATA_STORE_POD_H
#define RENDER_DATA_STORE_RENDER_DATA_STORE_POD_H

#include <atomic>
#include <cstdint>
#include <mutex>

#include <base/containers/array_view.h>
#include <base/containers/refcnt_ptr.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
/**
RenderDataStorePod implementation.
*/
class RenderDataStorePod final : public IRenderDataStorePod {
public:
    explicit RenderDataStorePod(BASE_NS::string_view name);
    ~RenderDataStorePod() override = default;

    // IRenderDataStore
    void PreRender() override {}
    void PostRender() override {}
    void PreRenderBackend() override {}
    void PostRenderBackend() override {}
    void Clear() override {}
    uint32_t GetFlags() const override
    {
        return 0;
    }
    BASE_NS::string_view GetTypeName() const override
    {
        return TYPE_NAME;
    }

    BASE_NS::string_view GetName() const override
    {
        return name_;
    }

    const BASE_NS::Uid& GetUid() const override
    {
        return UID;
    }

    void Ref() override;
    void Unref() override;
    int32_t GetRefCount() override;

    // IRenderDataStorePod
    void CreatePod(
        BASE_NS::string_view typeName, BASE_NS::string_view name, BASE_NS::array_view<const uint8_t> data) override;
    void DestroyPod(BASE_NS::string_view typeName, BASE_NS::string_view name) override;

    void Set(BASE_NS::string_view name, BASE_NS::array_view<const uint8_t> data) override;
    BASE_NS::array_view<const uint8_t> Get(BASE_NS::string_view name) const override;

    BASE_NS::array_view<const BASE_NS::string> GetPodNames(BASE_NS::string_view typeName) const override;

    // for plugin / factory interface
    static constexpr const char* const TYPE_NAME = "RenderDataStorePod";

    static BASE_NS::refcnt_ptr<IRenderDataStore> Create(IRenderContext& renderContext, const char* name);

private:
    const BASE_NS::string name_;

    // NOTE: allocate bigger blocks
    BASE_NS::vector<uint8_t> dataStore_;

    struct OffsetToData {
        uint32_t index { 0 };
        uint32_t byteSize { 0 };
    };
    BASE_NS::unordered_map<BASE_NS::string, OffsetToData> nameToDataOffset_;

    BASE_NS::unordered_map<BASE_NS::string, BASE_NS::vector<BASE_NS::string>> typeNameToPodNames_;

    mutable std::mutex mutex_;

    std::atomic_int32_t refcnt_ { 0 };
};
RENDER_END_NAMESPACE()

#endif // RENDER_DATA_STORE_RENDER_DATA_STORE_POD_H
