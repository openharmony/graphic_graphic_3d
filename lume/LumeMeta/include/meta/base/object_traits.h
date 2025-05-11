/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Type traits for IObjects.
 * Author: Lauri Jaaskela
 * Create: 2023-01-25
 */

#ifndef META_BASE_OBJECT_TRAITS_H
#define META_BASE_OBJECT_TRAITS_H

#include <base/containers/type_traits.h>
#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>

#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

class IObject;

// NOLINTBEGIN(readability-identifier-naming)
/** Is type IObject::Ptr */
template<class Type>
constexpr bool IsIObjectPtr_v = BASE_NS::is_same_v<BASE_NS::remove_const_t<Type>, BASE_NS::shared_ptr<IObject>>;
// NOLINTEND(readability-identifier-naming)

META_END_NAMESPACE()

#endif // META_BASE_OBJECT_TRAITS_H
