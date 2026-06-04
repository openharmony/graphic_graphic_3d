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

#ifndef SCENE_INTERFACE_SERIALIZATION_ISCENE_FILE_LOADER_H
#define SCENE_INTERFACE_SERIALIZATION_ISCENE_FILE_LOADER_H

#include <scene/interface/intf_render_context.h>
#include <scene/interface/intf_scene.h>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Slim probe for loading a scene file via an externally provided plugin.
 *
 * Implementations live in serialization plugins (e.g. the JSON importer).
 * SceneManager creates an instance via the class registry; if the providing
 * plugin is not loaded, Create() returns null and the caller falls through
 * to other loaders.
 *
 * Failure (file missing, wrong format, parse error) is reported as a null
 * IScene::Ptr. The implementation should not log loudly; the caller treats
 * a null result as "this loader did not handle the URI" and tries the next.
 */
class ISceneFileLoader : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISceneFileLoader, "76ffb578-c677-4ba0-8c6e-20e38dea6a03")
public:
    /**
     * @brief Load a scene from a URI, importing the given resource index files first.
     * @param context          Render context the loaded scene should belong to.
     * @param uri              Location of the scene file (engine file-manager URI).
     * @param resourceIndices  Resource index files (.res) to import, in order, before
     *                         the scene is parsed. Typically includes the project's
     *                         default_resources.res plus any extras the caller needs
     *                         for the scene to resolve its references.
     * @return Scene on success, null if the file is absent or not in this loader's format.
     */
    virtual IScene::Ptr LoadScene(const IRenderContext::Ptr& context, BASE_NS::string_view uri,
        BASE_NS::array_view<const BASE_NS::string> resourceIndices) = 0;
};

META_REGISTER_CLASS(SceneFileLoader, "6324b694-235f-4ee1-8363-b527a4998b71", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

#endif
