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

#include "animation_type.h"

#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/resource/intf_resource_context.h>

#include <meta/api/util.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_metadata.h>

SCENE_IMP_BEGIN_NAMESPACE()

CORE_NS::IResource::Ptr GltfAnimationResourceType::LoadResource(const StorageInfo& s) const
{
    auto rcontext = interface_cast<SCENE_NS::IResourceContext>(s.context);
    auto scene = rcontext ? rcontext->GetScene() : interface_pointer_cast<SCENE_NS::IScene>(s.context);
    if (!scene) {
        CORE_LOG_E("Missing scene/resource context when loading gltf animation resource: %s", s.id.ToString().c_str());
        return nullptr;
    }
    auto is = scene->GetInternalScene();
    CORE_NS::IResource::Ptr res = is->RunDirectlyOrInTask(
        [&is, &s]() { return interface_pointer_cast<CORE_NS::IResource>(is->ConstructAnimation(s.id)); });
    if (res && s.options) {
        s.options->ApplyOptions(*res, s.context);
    }
    return res;
}

static META_NS::IAnimation::Ptr CreateAnimationByType(BASE_NS::string_view animationType)
{
    META_NS::ClassInfo classInfo = META_NS::ClassId::TrackAnimation;
    if (animationType == "keyframeAnimation") {
        classInfo = META_NS::ClassId::KeyframeAnimation;
    } else if (animationType == "sequentialAnimation") {
        classInfo = META_NS::ClassId::SequentialAnimation;
    } else if (animationType == "parallelAnimation") {
        classInfo = META_NS::ClassId::ParallelAnimation;
    }
    return META_NS::GetObjectRegistry().Create<META_NS::IAnimation>(classInfo);
}

static const META_NS::IMetadata* GetTemplateMetadata(const CORE_NS::IResourceOptions::Ptr& options)
{
    return interface_cast<META_NS::IMetadata>(options);
}

CORE_NS::IResource::Ptr MetaAnimationResourceType::LoadResource(const StorageInfo& s) const
{
    META_NS::IObject::Ptr templateObj;
    CORE_NS::IResourceOptions::Ptr templateOptions;

    auto rcontext = interface_cast<SCENE_NS::IResourceContext>(s.context);
    auto scene = rcontext ? rcontext->GetScene() : interface_pointer_cast<SCENE_NS::IScene>(s.context);

    if (s.payload) {
        auto importer = importer_.lock();
        if (!importer) {
            CORE_LOG_E("Importer not available when loading animation resource: %s", s.id.ToString().c_str());
            return nullptr;
        }
        ImportParameters params;
        params.filename = s.path;
        params.scene = scene;
        auto impRes = importer->Import(*s.payload, params);
        templateObj = impRes.object;
        templateOptions = interface_pointer_cast<CORE_NS::IResourceOptions>(templateObj);
    }

    // Determine which options object will apply properties to the animation.
    auto options = templateOptions ? templateOptions : s.options;

    // Read AnimationType from whichever metadata source is available.
    const META_NS::IMetadata* tmeta = options ? GetTemplateMetadata(options) : nullptr;
    BASE_NS::string animationType;
    if (tmeta) {
        animationType = META_NS::GetValue<BASE_NS::string>(tmeta, "AnimationType");
    }

    auto anim = CreateAnimationByType(animationType);
    if (!anim) {
        CORE_LOG_E(
            "Failed to create animation (type: '%s', resource: %s)", animationType.c_str(), s.id.ToString().c_str());
        return nullptr;
    }

    if (auto setId = interface_cast<CORE_NS::ISetResourceId>(anim)) {
        setId->SetResourceId({s.id, s.context});
    }

    auto res = interface_pointer_cast<CORE_NS::IResource>(anim);
    if (res && options) {
        options->ApplyOptions(*res, s.context);
    }

    return res;
}

SCENE_IMP_END_NAMESPACE()