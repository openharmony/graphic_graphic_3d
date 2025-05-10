
#ifndef SCENE_INTERFACE_SCENE_OPTIONS_H
#define SCENE_INTERFACE_SCENE_OPTIONS_H

#include <scene/base/types.h>

SCENE_BEGIN_NAMESPACE()

struct SceneOptions {
    /// Set custom system graph, empty one uses system default
    BASE_NS::string systemGraphUri;
    /// If true, scene will control any startables attached to nodes in the scene
    bool enableStartables { true };
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::SceneOptions)

#endif
