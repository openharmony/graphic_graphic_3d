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

#ifndef OHOS_3D_MORPHER_ETS_H
#define OHOS_3D_MORPHER_ETS_H

#include <string>
#include <scene/interface/intf_mesh.h>

namespace OHOS::Render3D {
class MorpherETS {
public:
    MorpherETS(const SCENE_NS::IMorpher::Ptr morpher);
    ~MorpherETS();
    float Get(const std::string &name);
    void Set(const std::string &name, const float weight);

private:
    SCENE_NS::IMorpher::Ptr morpher_;
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_MORPHER_ETS_H
