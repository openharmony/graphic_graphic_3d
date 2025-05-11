/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Base interpolator implementations
 * Author: Lauri Jaaskela
 * Create: 2021-11-17
 */

#ifndef META_SRC_INTERPOLATOR_H
#define META_SRC_INTERPOLATOR_H

#include <meta/base/namespace.h>
#include <meta/ext/animation/interpolator.h>
#include <meta/interface/animation/builtin_animations.h>

META_BEGIN_NAMESPACE()

void RegisterDefaultInterpolators(IObjectRegistry& registry);
void UnRegisterDefaultInterpolators(IObjectRegistry& registry);

META_END_NAMESPACE()

#endif // META_SRC_INTERPOLATOR_H
