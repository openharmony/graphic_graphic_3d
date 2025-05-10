/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Object registry implementation
 * Author: Lauri Jaaskela
 * Create: 2021-09-07
 */

#ifndef META_SRC_RANDOM_H
#define META_SRC_RANDOM_H

#include "object_registry.h"

META_BEGIN_NAMESPACE()

BASE_NS::unique_ptr<IRandom> CreateXoroshiro128(uint64_t seed);

META_END_NAMESPACE()

#endif
