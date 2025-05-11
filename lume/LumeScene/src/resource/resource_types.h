
#ifndef SCENE_SRC_RESOURCE_RESOURCE_TYPES_H
#define SCENE_SRC_RESOURCE_RESOURCE_TYPES_H

#include <scene/interface/intf_render_context.h>
#include <scene/interface/scene_options.h>

SCENE_BEGIN_NAMESPACE()

void RegisterResourceTypes(const IRenderContext::Ptr& context, const SceneOptions& opts);
void RegisterResourceTypes(const CORE_NS::IResourceManager::Ptr& res, const META_NS::IMetadata::Ptr& args);

SCENE_END_NAMESPACE()

#endif