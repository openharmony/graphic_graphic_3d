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

#ifndef META_API_PROPERTY_ARRAY_PROPERTY_CHANGES_RECOGNIZER_H
#define META_API_PROPERTY_ARRAY_PROPERTY_CHANGES_RECOGNIZER_H

#include <base/containers/unordered_map.h>

#include <meta/base/algorithms.h>
#include <meta/interface/property/array_property.h>

META_BEGIN_NAMESPACE()

template<typename ValueType>
struct AddedValue {
    ValueType value;
    int index = 0;

    bool operator==(const AddedValue& v) const
    {
        return value == v.value && index == v.index;
    }
};

struct IndexChange {
    int previousIndex = 0;
    int newIndex = 0;

    bool operator==(const IndexChange& v) const
    {
        return previousIndex == v.previousIndex && newIndex == v.newIndex;
    }
};

template<typename ValueType>
struct ArrayChanges {
    BASE_NS::vector<size_t> indexesRemoved;
    BASE_NS::vector<AddedValue<ValueType>> valuesAdded;
    BASE_NS::vector<IndexChange> positionChanged;
};

template<typename ValueType>
class ArrayPropertyChangesRecognizer {
public:
    ArrayPropertyChangesRecognizer() = default;

    void SetValue(const META_NS::ArrayProperty<ValueType>& arrayProperty)
    {
        previousValues_ = arrayProperty->GetValue();
    }

    void Clear()
    {
        previousValues_.clear();
    }

    ArrayChanges<ValueType> OnArrayPropertyChanged(const META_NS::ArrayProperty<ValueType>& arrayProperty)
    {
        auto newValuesVector = arrayProperty->GetValue();
        auto changes = ArrayChanges<ValueType>();

        { // check for removes
            BASE_NS::unordered_map<ValueType, size_t> prev;
            for (size_t oldIndex = 0; oldIndex != previousValues_.size(); ++oldIndex) {
                auto& v = previousValues_[oldIndex];
                if (auto p = FindFirstOf(newValuesVector, v, prev[v]); p != NPOS) {
                    prev[v] = p + 1;
                } else {
                    changes.indexesRemoved.push_back(static_cast<int>(oldIndex));
                }
            }
        }

        { // check for modifications and additions
            BASE_NS::unordered_map<ValueType, size_t> prev;
            for (size_t newIndex = 0; newIndex < newValuesVector.size(); ++newIndex) {
                auto& v = newValuesVector[newIndex];
                if (auto p = FindFirstOf(previousValues_, v, prev[v]); p != NPOS) {
                    prev[v] = p + 1;
                    if (p != newIndex) {
                        changes.positionChanged.push_back({ static_cast<int>(p), static_cast<int>(newIndex) });
                    }
                } else {
                    changes.valuesAdded.push_back({ v, static_cast<int>(newIndex) });
                }
            }
        }

        previousValues_ = BASE_NS::move(newValuesVector);

        return changes;
    }

private:
    BASE_NS::vector<ValueType> previousValues_;
};

META_END_NAMESPACE()

#endif
