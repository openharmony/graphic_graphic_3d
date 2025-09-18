/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_3D_RENDER_RESOURCES_H
#define OHOS_3D_RENDER_RESOURCES_H

#include <string>
#include <mutex>

#include <base/containers/unordered_map.h>
#include <scene/interface/intf_image.h>

namespace OHOS::Render3D {
class RenderResourcesETS {
public:
    RenderResourcesETS();
    ~RenderResourcesETS();

    void StoreBitmap(BASE_NS::string_view uri, SCENE_NS::IBitmap::Ptr bitmap);
    SCENE_NS::IBitmap::Ptr FetchBitmap(BASE_NS::string_view uri);

private:
    std::mutex mutex_;
    BASE_NS::unordered_map<BASE_NS::string, SCENE_NS::IBitmap::Ptr> bitmaps_;
};
}  // namespace OHOS::Render3D
#endif // OHOS_3D_RENDER_RESOURCES_H
