/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Object base implementation interface
 * Author: Jani Kattelus
 * Create: 2022-03-31
 */
#ifndef META_SRC_OBJECT_H
#define META_SRC_OBJECT_H

#include <meta/ext/object.h>

META_BEGIN_NAMESPACE()
namespace Internal {
class Object : public MetaObject {
    META_OBJECT(Object, ClassId::Object, MetaObject)
};
} // namespace Internal

META_END_NAMESPACE()

#endif
