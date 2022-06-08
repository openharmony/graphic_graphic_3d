/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef RENDER_DATA_STORE_RENDER_DATA_STORE_POD_H
#define RENDER_DATA_STORE_RENDER_DATA_STORE_POD_H

#include <cstdint>
#include <mutex>

#include <base/containers/array_view.h>
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
    explicit RenderDataStorePod(const BASE_NS::string_view name);
    ~RenderDataStorePod() override = default;

    void PreRender() override {};
    void PreRenderBackend() override {};
    void PostRender() override {};
    void Clear() override {};

    void CreatePod(const BASE_NS::string_view typeName, const BASE_NS::string_view name,
        const BASE_NS::array_view<const uint8_t> data) override;
    void DestroyPod(const BASE_NS::string_view typeName, const BASE_NS::string_view name) override;

    void Set(const BASE_NS::string_view name, const BASE_NS::array_view<const uint8_t> data) override;
    BASE_NS::array_view<const uint8_t> Get(const BASE_NS::string_view name) const override;

    BASE_NS::array_view<const BASE_NS::string> GetPodNames(const BASE_NS::string_view typeName) const override;

    // for plugin / factory interface
    static constexpr const char* const TYPE_NAME = "RenderDataStorePod";

    static IRenderDataStore* Create(IRenderContext& renderContext, char const* name);
    static void Destroy(IRenderDataStore* instance);

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

private:
    const BASE_NS::string name_;

    // NOTE: allocate bigger blocks
    BASE_NS::vector<uint8_t> dataStore_;

    struct OffsetToData {
        uint32_t index { 0 };
        uint32_t byteSize { 0 };
        uint32_t typeNameVectorIndex { 0 };
    };
    BASE_NS::unordered_map<BASE_NS::string, OffsetToData> nameToDataOffset_;

    BASE_NS::unordered_map<BASE_NS::string, BASE_NS::vector<BASE_NS::string>> typeNameToPodNames_;

    mutable std::mutex mutex_;
};
RENDER_END_NAMESPACE()

#endif // RENDER_DATA_STORE_RENDER_DATA_STORE_POD_H
