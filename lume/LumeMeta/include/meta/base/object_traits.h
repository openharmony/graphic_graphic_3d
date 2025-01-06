/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef META_BASE_OBJECT_TRAITS_H
#define META_BASE_OBJECT_TRAITS_H

#include <base/containers/type_traits.h>
#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>

#include <meta/base/namespace.h>
#include <meta/base/shared_ptr.h>

META_BEGIN_NAMESPACE()

class IObject;

// NOLINTBEGIN(readability-identifier-naming)
/** Is type IObject::Ptr */
template<class Type>
constexpr bool IsIObjectPtr_v = BASE_NS::is_same_v<BASE_NS::remove_const_t<Type>, BASE_NS::shared_ptr<IObject>>;
// NOLINTEND(readability-identifier-naming)

META_END_NAMESPACE()

#endif // META_BASE_OBJECT_TRAITS_H
