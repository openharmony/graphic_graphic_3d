
#ifndef SCENE_BASE_TYPES_H
#define SCENE_BASE_TYPES_H

#include <scene/base/namespace.h>

#include <meta/api/future.h>
#include <meta/base/interface_macros.h>
#include <meta/base/types.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

using META_NS::Future;

enum class RenderMode {
    /// Render a frame if something changed in the scene (When calling internal scene update)
    IF_DIRTY,
    /// Always render new frame even if nothing changed
    ALWAYS,
    /// Only render a frame if explicitly requested
    MANUAL
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::RenderMode);

#endif
