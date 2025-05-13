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

#ifndef SCENE_SRC_SCENE_MANAGER_H
#define SCENE_SRC_SCENE_MANAGER_H

#include <scene/interface/intf_render_context.h>
#include <scene/interface/intf_scene_manager.h>

#include <meta/ext/object_container.h>

SCENE_BEGIN_NAMESPACE()

class SceneManager : public META_NS::IntroduceInterfaces<META_NS::CommonObjectContainerFwd, ISceneManager> {
    META_OBJECT(SceneManager, ClassId::SceneManager, IntroduceInterfaces, META_NS::ClassId::ObjectContainer)
public:
    bool Build(const META_NS::IMetadata::Ptr&) override;

    /**
     * @brief Create an empty scene with default options
     * @return Null scene pointer if creation failed
     */
    Future<IScene::Ptr> CreateScene() override;
    /**
     * @brief Create an empty scene
     * @param opts Options for creation
     * @return Null scene pointer if creation failed
     */
    Future<IScene::Ptr> CreateScene(SceneOptions opts) override;
    /**
     * @brief Load scene from a file. Multiple formats are supported
     * @param uri Location of the scene file, for example file://myscene.gltf or file://myscene.scene
     * @return Null scene pointer if creation failed
     * @note Supports 2 types of .scene files. First tries with older loader. If that fails, then newer one with index
     */
    Future<IScene::Ptr> CreateScene(BASE_NS::string_view uri) override;
    /**
     * @brief Load scene from a file. Multiple formats are supported
     * @param uri Location of the scene file, for example file://myscene.gltf or file://myscene.scene
     * @param opts Options for creation
     * @return Null scene pointer if creation failed
     * @note Supports 2 types of .scene files. First tries with older loader. If that fails, then newer one with index
     */
    Future<IScene::Ptr> CreateScene(BASE_NS::string_view uri, SceneOptions opts) override;

    IRenderContext::Ptr GetContext() const override
    {
        return context_;
    }

private:
    META_NS::IMetadata::Ptr CreateContext(SceneOptions opts) const;

    // Load a scene by using an index file. See GuessIndexFilePath
    static IScene::Ptr LoadSceneWithIndex(const IRenderContext::Ptr& context, BASE_NS::string_view uri);

    // Run the given load function with a path temporarily registered in the file manager. See GuessProjectPath
    template<typename LoadFunc>
    static inline IScene::Ptr WithPathRegistered(
        const META_NS::IMetadata::Ptr& context, BASE_NS::string_view uri, LoadFunc loadFunc)
    {
        // Rely on an implementation detail: If the path is already registered, it can be re-registered.
        // Unregistering will remove only the re-registered path and leave the original.
        if (SetProjectPath(context, uri, ProjectPathAction::REGISTER)) {
            const auto renderContext = META_NS::GetValue(context->GetProperty<IRenderContext::Ptr>("RenderContext"));
            const auto result = loadFunc(renderContext, uri);
            SetProjectPath(context, uri, ProjectPathAction::UNREGISTER);
            return result;
        }
        return {};
    }

    enum class ProjectPathAction { REGISTER, UNREGISTER };
    // Register/unregister the given uri with the file manager
    static bool SetProjectPath(
        const META_NS::IMetadata::Ptr& engineContext, BASE_NS::string_view uri, ProjectPathAction action);

    // GuessIndexFilePath("schema://default.scene2") == "schema://default.res"
    static BASE_NS::string GuessIndexFilePath(BASE_NS::string_view uri);

    // GuessProjectPath("schema://path/to/PROJECT/assets/default.scene2") == "schema://path/to/PROJECT"
    static BASE_NS::string GuessProjectPath(BASE_NS::string_view uri);

private:
    IRenderContext::Ptr context_;
    SceneOptions opts_;
};

SCENE_END_NAMESPACE()

#endif