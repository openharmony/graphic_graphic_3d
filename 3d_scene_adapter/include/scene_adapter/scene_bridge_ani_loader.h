/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_RENDER_3D_SCENE_BRIDGE_ANI_LOADER_H
#define OHOS_RENDER_3D_SCENE_BRIDGE_ANI_LOADER_H

#include <cstdint>

namespace OHOS::Render3D {

using UnwrapSceneFromAniFunc = void* (*)(void*, void*);

class SceneBridgeAniLoader {
public:
    static SceneBridgeAniLoader& GetInstance()
    {
        static SceneBridgeAniLoader instance;
        return instance;
    }

    void* UnwrapSceneFromAni(void* env, void* value);

    void CloseLibrary();

private:
    SceneBridgeAniLoader()
    {
        DynamicLoadLibrary();
    }

    ~SceneBridgeAniLoader()
    {
        CloseLibrary();
    }

    void* LoadSymbol(const char* name);
    bool DynamicLoadLibrary();

    void* handle_ = nullptr;
    UnwrapSceneFromAniFunc unwrapFunc_ = nullptr;
};

} // namespace OHOS::Render3D

#endif // OHOS_RENDER_3D_SCENE_BRIDGE_ANI_LOADER_H
