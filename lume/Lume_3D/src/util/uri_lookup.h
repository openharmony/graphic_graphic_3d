/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_UTIL_URI_LOOKUP_H
#define CORE_UTIL_URI_LOOKUP_H

#include <3d/namespace.h>
#include <base/containers/string_view.h>
#include <core/ecs/entity.h>

CORE3D_BEGIN_NAMESPACE()
class IUriComponentManager;
// Instantiated for AnimationComponent, MaterialComponent, MeshComponent, and SkinIbmComponent. Others can be added to
// the cpp or implementation inlined here.
template<typename ComponentManager>
CORE_NS::Entity LookupResourceByUri(
    BASE_NS::string_view uri, const IUriComponentManager& uriManager, const ComponentManager& componentManager);
CORE3D_END_NAMESPACE()
#endif