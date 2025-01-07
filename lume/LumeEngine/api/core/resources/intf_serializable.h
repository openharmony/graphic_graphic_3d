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

#ifndef API_CORE_RESOURCES_ISERIALIZABLE_H
#define API_CORE_RESOURCES_ISERIALIZABLE_H

#include <base/containers/array_view.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <core/plugin/intf_interface.h>

CORE_BEGIN_NAMESPACE()
class ISerializable : public IInterface {
public:
    static constexpr BASE_NS::Uid UID { "4de1999d-0b57-49d7-b5ed-df4bbfecd17c" };
    using Ptr = BASE_NS::refcnt_ptr<ISerializable>;

    /** Deserialize the object from data. Previous state of the object is replaced from the data. If deserialization
     * fails the object may still have lost its previous state.
     * @param uri Identifier of the data.
     * @param data Data previously returned by Export.
     * @return true if the object could be deserialized from data, otherwise false.
     */
    virtual bool Import(BASE_NS::string_view uri, BASE_NS::array_view<const uint8_t> data) = 0;

    /** Serializes the object's data. The returned bytes can be later used to deserialize the object.
     * @return Serialized object.
     */
    virtual BASE_NS::vector<uint8_t> Export() const = 0;
};

CORE_END_NAMESPACE()
#endif // API_CORE_RESOURCES_ISERIALIZABLE_H
