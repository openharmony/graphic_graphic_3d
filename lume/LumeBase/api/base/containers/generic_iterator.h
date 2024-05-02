/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef API_BASE_CONTAINERS_GENERIC_ITERATOR_H
#define API_BASE_CONTAINERS_GENERIC_ITERATOR_H

#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()

template<typename container>
class IGenericIterator {
public:
    using Val = typename container::IteratorValue;
    using Ptr = unique_ptr<IGenericIterator, void (*)(IGenericIterator*)>;
    // Returns the container where this iterator points to.
    virtual const container* GetOwner() const = 0;
    // Compares two IGenericIterators
    // Returns true if they point to same.
    virtual bool Compare(const Ptr&) const = 0;
    // Advances the iterator to next element
    // returns false if end reached.
    virtual bool Next() = 0;
    // Returns current value. (default if at end)
    virtual Val Get() const = 0;
    // Creates a clone of current iterator.
    virtual Ptr Clone() const = 0;

protected:
    IGenericIterator() = default;
    IGenericIterator(const IGenericIterator&) = delete;
    IGenericIterator(IGenericIterator&&) = delete;
    IGenericIterator& operator=(const IGenericIterator&) = delete;
    virtual ~IGenericIterator() = default;
};

template<typename Type>
class IIterator {
public:
    IIterator() = default;
    ~IIterator() = default;

    IIterator(const IIterator& it)
    {
        if (it.it_) {
            it_ = it.it_->Clone();
        }
    }
    IIterator(const typename Type::Ptr& it)
    {
        if (it) {
            it_ = it->Clone();
        }
    }

    IIterator(IIterator&& it) noexcept : it_(move(it.it_)) {}
    IIterator(typename Type::Ptr&& it) noexcept : it_(move(it)) {}

    auto& operator=(const IIterator& it)
    {
        if (it.it_) {
            it_ = it.it_->Clone();
        } else {
            it_.reset();
        }
        return *this;
    }
    auto& operator=(IIterator&& it) noexcept
    {
        it_ = move(it.it_);
        return *this;
    }
    bool operator==(const IIterator& other) const
    {
        if ((it_ == nullptr) && (other.it_ == nullptr)) {
            return true;
        }
        if (it_) {
            return it_->Compare(other.it_);
        }
        return false;
    }
    bool operator!=(const IIterator& other)
    {
        return !(*this == other);
    }
    IIterator& operator++()
    {
        if (it_) {
            it_->Next();
        }
        return *this;
    }
    typename Type::Val operator*() const
    {
        if (it_) {
            return it_->Get();
        }
        return {};
    }

protected:
    typename Type::Ptr it_;
};

BASE_END_NAMESPACE()
#endif
