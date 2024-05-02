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

#ifndef META_EXT_SERIALIZATION_COMMON_VALUE_SERIALIZERS_H
#define META_EXT_SERIALIZATION_COMMON_VALUE_SERIALIZERS_H

#include <meta/ext/serialization/value_serializer.h>

META_BEGIN_NAMESPACE()

template<typename Value>
ISerNode::Ptr EnumExport(IExportFunctions& f, const Value& v)
{
    using Type = BASE_NS::underlying_type_t<BASE_NS::remove_const_t<BASE_NS::remove_reference_t<decltype(v)>>>;
    return f.ExportToNode(Any<Type>(static_cast<Type>(v)));
}

template<typename Value>
bool EnumImport(IImportFunctions& f, const ISerNode::ConstPtr& node, Value& out)
{
    using Plain = BASE_NS::remove_const_t<BASE_NS::remove_reference_t<decltype(out)>>;
    using Type = BASE_NS::underlying_type_t<Plain>;
    Any<Type> any;
    bool res = f.ImportFromNode(node, any);
    if (res) {
        out = static_cast<Plain>(any.InternalGetValue());
    }
    return res;
}

template<typename Type>
bool ExtractInteger(const ISerNode::ConstPtr& node, Type& out)
{
    if (auto n = interface_cast<IIntNode>(node)) {
        out = static_cast<Type>(n->GetValue());
        return true;
    }
    if (auto n = interface_cast<IUIntNode>(node)) {
        out = static_cast<Type>(n->GetValue());
        return true;
    }
    return false;
}

template<typename Type>
bool ExtractNumber(const ISerNode::ConstPtr& node, Type& out)
{
    if (ExtractInteger(node, out)) {
        return true;
    }
    if (auto n = interface_cast<IDoubleNode>(node)) {
        out = static_cast<Type>(n->GetValue());
        return true;
    }
    if (auto n = interface_cast<IBoolNode>(node)) {
        out = static_cast<Type>(n->GetValue());
        return true;
    }
    return false;
}

inline IObjectNode::Ptr CreateObjectNode(ObjectId oid, BASE_NS::string className)
{
    auto obj = GetObjectRegistry().Create<IObjectNode>(META_NS::ClassId::ObjectNode);
    if (obj) {
        obj->SetObjectId(oid);
        obj->SetObjectClassName(BASE_NS::move(className));
    }
    return obj;
}

template<typename Type>
ReturnError ImportFromNode(IImportFunctions& f, const ISerNode::ConstPtr& node, Type& value)
{
    Any<Type> any;
    auto res = f.ImportFromNode(node, any);
    if (res) {
        value = any.InternalGetValue();
    }
    return res;
}

META_END_NAMESPACE()

#endif
