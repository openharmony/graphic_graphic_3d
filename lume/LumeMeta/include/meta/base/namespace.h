/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Namespace macros
 * Author: Jani Kattelus
 * Create: 2022-05-27
 */

#ifndef META_BASE_NAMESPACE_H
#define META_BASE_NAMESPACE_H

/**
 * @brief Meta object library namespace
 */
#define META_NS Meta

/**
 * @brief Start Meta object library namespace
 */
#define META_BEGIN_NAMESPACE() namespace META_NS {
/**
 * @brief End Meta object library namespace
 */
#define META_END_NAMESPACE() } // namespace Meta

#define META_BEGIN_INTERNAL_NAMESPACE() \
    namespace META_NS {                 \
    namespace Internal {

#define META_END_INTERNAL_NAMESPACE() \
    } /* namespace Internal */        \
    } // namespace Meta

#endif
