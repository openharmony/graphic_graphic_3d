/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Base object implementation
 * Author: Lauri Jaaskela
 * Create: 2023-04-25
 */
#ifndef META_SRC_BASE_OBJECT_H
#define META_SRC_BASE_OBJECT_H

#include <meta/ext/base_object.h>

META_BEGIN_NAMESPACE()

namespace Internal {

class BaseObject : public META_NS::BaseObject {
public:
    META_OBJECT(BaseObject, META_NS::ClassId::BaseObject, META_NS::BaseObject)
};

} // namespace Internal

META_END_NAMESPACE()

#endif
