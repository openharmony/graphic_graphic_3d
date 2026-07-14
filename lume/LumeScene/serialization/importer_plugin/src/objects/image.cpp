/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "image.h"

#include <scene/interface/intf_image.h>
#include <scene/interface/resource/image_info.h>
#include <scene/interface/resource/types.h>

#include <meta/api/metadata_util.h>
#include <meta/api/util.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/resource/intf_object_resource.h>

#include "../import_helpers.h"
#include "flag_tables.h"

SCENE_IMP_BEGIN_NAMESPACE()

static IDiagnostics::Ptr ImportImageInfo(ImportContext& context, SCENE_NS::ImageInfo& out)
{
    auto obj = context.GetOptObject("info");
    if (obj.empty()) {
        return nullptr;
    }
    auto infoContext = context.CreateContext(BASE_NS::move(obj));
    ErrorHandler h(context);
    auto usageArr = infoContext.GetOptArray("usageFlags");
    if (!usageArr.empty()) {
        out.usageFlags = {};
        if (auto err =
                ImportFlagsFromArray(infoContext, usageArr, IMAGE_USAGE_FLAGS_TABLE, "usageFlags", out.usageFlags);
            h.Handle(err)) {
            return err;
        }
    }
    auto memoryArr = infoContext.GetOptArray("memoryFlags");
    if (!memoryArr.empty()) {
        out.memoryFlags = {};
        if (auto err = ImportFlagsFromArray(infoContext, memoryArr, MEMORY_FLAGS_TABLE, "memoryFlags", out.memoryFlags);
            h.Handle(err)) {
            return err;
        }
    }
    auto creationArr = infoContext.GetOptArray("creationFlags");
    if (!creationArr.empty()) {
        out.creationFlags = {};
        if (auto err = ImportFlagsFromArray(
                infoContext, creationArr, CREATION_FLAGS_TABLE, "creationFlags", out.creationFlags);
            h.Handle(err)) {
            return err;
        }
    }
    return h;
}

ImportResult ImportImage::Import(ImportContext& context)
{
    auto trace = context.Trace("Importing image");
    auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObject>(SCENE_NS::ClassId::ImageTemplate);
    auto meta = interface_cast<META_NS::IMetadata>(obj);
    if (!obj || !meta) {
        return ImportResult{context.CreateDiagnostics("Failed to create ImageTemplate")};
    }

    ErrorHandler h(context);
    if (auto err = ImportResourceName(context, *meta); h.Handle(err)) {
        return ImportResult{err};
    }
    SCENE_NS::ImageLoadInfo loadInfo = SCENE_NS::DEFAULT_IMAGE_LOAD_INFO;
    auto loadFlagsArr = context.GetOptArray("loadFlags");
    if (!loadFlagsArr.empty()) {
        loadInfo.loadFlags = {};
        if (auto err =
                ImportFlagsFromArray(context, loadFlagsArr, IMAGE_LOAD_FLAGS_TABLE, "loadFlags", loadInfo.loadFlags);
            h.Handle(err)) {
            return ImportResult{err};
        }
    }
    if (auto err = ImportImageInfo(context, loadInfo.info); h.Handle(err)) {
        return ImportResult{err};
    }

    META_NS::SetValue(meta->GetProperty<SCENE_NS::ImageLoadInfo>("ImageLoadInfo"), loadInfo);

    ImportResult result{obj};
    result.error = h;
    return result;
}

SCENE_IMP_END_NAMESPACE()
