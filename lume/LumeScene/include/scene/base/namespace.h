/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Namespace macros
 * Author: Markus Penttila
 * Create: 2023-01-10
 */

#ifndef SCENE_BASE_NAMESPACE_H
#define SCENE_BASE_NAMESPACE_H

#include <base/util/uid.h>

#if defined(SCENE_VERBOSE_LOGS)
#define SCENE_VERBOSE_LOG CORE_LOG_D
#else
#define SCENE_VERBOSE_LOG(...)
#endif // !NDEBUG

#define SCENE_NS Scene

#define SCENE_BEGIN_NAMESPACE() namespace SCENE_NS {
#define SCENE_END_NAMESPACE() } // namespace Scene

SCENE_BEGIN_NAMESPACE()
static constexpr BASE_NS::Uid UID_SCENE_PLUGIN { "ad98b964-a219-4b65-8033-209f44ba3b24" };
SCENE_END_NAMESPACE()

#endif
