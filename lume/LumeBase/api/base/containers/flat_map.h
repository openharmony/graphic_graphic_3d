/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef API_BASE_CONTAINERS_FLAT_MAP_H
#define API_BASE_CONTAINERS_FLAT_MAP_H

#include <base/containers/iterator.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/util/algorithm.h>

BASE_BEGIN_NAMESPACE()
template<typename KeyType, typename ValueType, typename Compare>
class flat_map;

namespace Detail {
template<typename KeyIter, typename ValueIter, typename ValueConstIter>
class Iterator {
public:
    template<typename T>
    using IterReference = decltype(*BASE_NS::declval<T&>());
    using ConstIterator = Iterator<KeyIter, ValueConstIter, ValueConstIter>;
    using reference = BASE_NS::pair<IterReference<KeyIter>, IterReference<ValueIter>>;
    using difference_type = ptrdiff_t;

private:
    class RefWrapper {
    public:
        explicit RefWrapper(const reference& ref) noexcept : ref_ { ref } {}

        const reference* operator->() const noexcept
        {
            return &ref_;
        }

    private:
        reference ref_;
    };

public:
    using pointer = RefWrapper;

    Iterator() = default;

    Iterator(KeyIter key, ValueIter value) noexcept : keyIter_(BASE_NS::move(key)), valueIter_(BASE_NS::move(value)) {}

    reference operator*() const
    {
        return reference { *keyIter_, *valueIter_ };
    }

    pointer operator->() const
    {
        return pointer { this->operator*() };
    }

    Iterator& operator++()
    {
        ++keyIter_;
        ++valueIter_;
        return *this;
    }

    Iterator operator++(int)
    {
        auto prev = *this;
        ++*this;
        return prev;
    }

    Iterator& operator--()
    {
        --keyIter_;
        --valueIter_;
        return *this;
    }

    Iterator operator--(int)
    {
        auto prev = *this;
        --*this;
        return prev;
    }

    Iterator& operator+=(const difference_type offset)
    {
        keyIter_ += offset;
        valueIter_ += offset;
        return *this;
    }

    Iterator& operator-=(const difference_type offset)
    {
        keyIter_ -= offset;
        valueIter_ -= offset;
        return *this;
    }

    Iterator operator+(const difference_type offset)
    {
        auto prev = *this;
        prev += offset;
        return prev;
    }

    Iterator operator-(const difference_type offset)
    {
        auto prev = *this;
        prev -= offset;
        return prev;
    }

    bool operator==(const Iterator& _Right) const
    {
        return keyIter_ == _Right.keyIter_;
    }

    bool operator!=(const Iterator& _Right) const
    {
        return keyIter_ != _Right.keyIter_;
    }

    template<typename T = ValueConstIter, BASE_NS::enable_if_t<!BASE_NS::is_same_v<ValueIter, T>, bool> = true>
    operator ConstIterator() const
    {
        return ConstIterator(keyIter_, valueIter_);
    }

private:
    template<typename KeyType, typename ValueType, typename Compare>
    friend class BASE_NS::flat_map;
    KeyIter keyIter_;
    ValueIter valueIter_;
};
} // namespace Detail

template<typename KeyType, typename ValueType, typename Compare = Less<KeyType>>
class flat_map {
public:
    using key_type = KeyType;
    using mapped_type = ValueType;
    using value_type = BASE_NS::pair<key_type, mapped_type>;
    using difference_type = ptrdiff_t;
    using reference = BASE_NS::pair<const key_type&, mapped_type&>;
    using const_reference = BASE_NS::pair<const key_type&, const mapped_type&>;

    using iterator = Detail::Iterator<BASE_NS::const_iterator<BASE_NS::vector<key_type>>,
        BASE_NS::iterator<BASE_NS::vector<mapped_type>>, BASE_NS::const_iterator<BASE_NS::vector<mapped_type>>>;
    using const_iterator = Detail::Iterator<BASE_NS::const_iterator<BASE_NS::vector<key_type>>,
        BASE_NS::const_iterator<BASE_NS::vector<mapped_type>>, BASE_NS::const_iterator<BASE_NS::vector<mapped_type>>>;

    flat_map() = default;

    template<class InputIter>
    flat_map(InputIter first, InputIter last)
    {
        insert(first, last);
    }

    flat_map(std::initializer_list<value_type> init) : flat_map(init.begin(), init.end()) {}

    // Element access
    mapped_type& operator[](const key_type& key)
    {
        return try_emplace(key).first->second;
    }

    mapped_type& operator[](key_type&& key)
    {
        return try_emplace(BASE_NS::move(key)).first->second;
    }

    template<typename Key>
    mapped_type& operator[](Key&& key)
    {
        return try_emplace(BASE_NS::forward<Key>(key)).first->second;
    }

    // Iterators
    iterator begin() noexcept
    {
        return iterator { keys_.cbegin(), values_.begin() };
    }

    const_iterator begin() const noexcept
    {
        return const_iterator { keys_.cbegin(), values_.begin() };
    }

    const_iterator cbegin() const noexcept
    {
        return const_iterator { keys_.cbegin(), values_.begin() };
    }

    iterator end() noexcept
    {
        return iterator { keys_.cend(), values_.end() };
    }

    const_iterator end() const noexcept
    {
        return const_iterator { keys_.cend(), values_.end() };
    }

    const_iterator cend() const noexcept
    {
        return const_iterator { keys_.cend(), values_.end() };
    }

    // Capacity
    bool empty() const noexcept
    {
        return keys_.empty();
    }

    size_t size() const noexcept
    {
        return keys_.size();
    }

    // Modifiers
    template<class... Args>
    BASE_NS::pair<iterator, bool> emplace(Args&&... args)
    {
        auto predicate = Predicate {};
        auto t = value_type(BASE_NS::forward<Args>(args)...);
        const auto pos = LowerBound(keys_.cbegin(), keys_.cend(), t.first, predicate);
        if ((pos == keys_.cend()) || predicate(t.first, *pos)) {
            const auto index = pos - keys_.cbegin();
            keys_.insert(pos, (BASE_NS::move(t.first)));
            values_.insert(values_.cbegin() + index, BASE_NS::move(t.second));
            return { begin() + index, true };
        }
        return { begin() + (pos - keys_.begin()), false };
    }

    template<typename Key, typename... Args>
    BASE_NS::pair<iterator, bool> try_emplace(Key&& key, Args&&... args)
    {
        auto predicate = Predicate {};
        const auto pos = LowerBound(keys_.cbegin(), keys_.cend(), key, predicate);
        if ((pos == keys_.cend()) || predicate(key, *pos)) {
            const auto index = pos - keys_.cbegin();
            keys_.insert(pos, key_type(BASE_NS::forward<Key>(key)));
            values_.insert(values_.cbegin() + index, mapped_type(BASE_NS::forward<Args>(args)...));
            return { begin() + index, true };
        }
        return { begin() + (pos - keys_.begin()), false };
    }

    BASE_NS::pair<iterator, bool> insert(const value_type& value)
    {
        return emplace(value);
    }

    BASE_NS::pair<iterator, bool> insert(value_type&& value)
    {
        return emplace(BASE_NS::move(value));
    }

    template<class InputIt>
    void insert(InputIt first, InputIt last)
    {
        for (; first != last; ++first) {
            insert(*first);
        }
    }

    template<class P>
    BASE_NS::pair<iterator, bool> insert(P&& x)
    {
        return emplace(BASE_NS::forward<P>(x));
    }

    template<class M>
    BASE_NS::pair<iterator, bool> insert_or_assign(const key_type& key, M&& obj)
    {
        auto predicate = Predicate {};
        const auto pos = LowerBound(keys_.begin(), keys_.end(), key, predicate);
        const auto index = pos - keys_.begin();
        if ((pos == keys_.end()) || predicate(key, *pos)) {
            keys_.insert(pos, key);
            values_.insert(values_.cbegin() + index, mapped_type(BASE_NS::forward<M>(obj)));
            return { begin() + index, true };
        }
        *(values_.begin() + index) = BASE_NS::forward<M>(obj);
        return { begin() + index, false };
    }

    template<class M>
    BASE_NS::pair<iterator, bool> insert_or_assign(key_type&& key, M&& obj)
    {
        auto predicate = Predicate {};
        const auto pos = LowerBound(keys_.begin(), keys_.end(), key, predicate);
        const auto index = pos - keys_.begin();
        if ((pos == keys_.end()) || predicate(key, *pos)) {
            keys_.insert(pos, BASE_NS::move(key));
            values_.insert(values_.cbegin() + index, mapped_type(BASE_NS::forward<M>(obj)));
            return { begin() + index, true };
        }
        *(values_.begin() + index) = BASE_NS::forward<M>(obj);
        return { begin() + index, false };
    }

    template<class K, class M>
    BASE_NS::pair<iterator, bool> insert_or_assign(K&& k, M&& obj)
    {
        auto predicate = Predicate {};
        const auto pos = LowerBound(keys_.begin(), keys_.end(), BASE_NS::forward<K>(k), predicate);
        const auto index = pos - keys_.begin();
        if ((pos == keys_.end()) || predicate(BASE_NS::forward<K>(k), *pos)) {
            keys_.insert(pos, BASE_NS::forward<K>(k));
            values_.insert(values_.cbegin() + index, mapped_type(BASE_NS::forward<M>(obj)));
            return { begin() + index, true };
        }
        *(values_.begin() + index) = BASE_NS::forward<M>(obj);
        return { begin() + index, false };
    }

    iterator erase(iterator pos)
    {
        keys_.erase(pos.keyIter_);
        values_.erase(pos.valueIter_);
        return pos;
    }

    iterator erase(const_iterator pos)
    {
        keys_.erase(pos.keyIter_);
        values_.erase(pos.valueIter_);
        return pos;
    }

    template<typename Key>
    size_t erase(const Key& key)
    {
        auto predicate = Predicate {};
        auto pos = LowerBound(keys_.cbegin(), keys_.cend(), key, predicate);
        if ((pos == keys_.cend()) || predicate(key, *pos)) {
            return 0u;
        }
        auto index = pos - keys_.cbegin();
        keys_.erase(pos);
        values_.erase(values_.cbegin() + index);
        return 1u;
    }

    void clear()
    {
        keys_.clear();
        values_.clear();
    }

    // Lookup
    template<typename Key>
    iterator find(const Key& key) noexcept
    {
        auto predicate = Predicate {};
        auto pos = LowerBound(keys_.cbegin(), keys_.cend(), key, predicate);
        if ((pos == keys_.cend()) || predicate(key, *pos)) {
            return end();
        }
        auto index = pos - keys_.cbegin();
        return iterator { pos, values_.begin() + index };
    }

    template<typename Key>
    const_iterator find(const Key& key) const noexcept
    {
        auto predicate = Predicate {};
        auto pos = LowerBound(keys_.cbegin(), keys_.cend(), key, predicate);
        if ((pos == keys_.cend()) || predicate(key, *pos)) {
            return end();
        }
        auto index = pos - keys_.cbegin();
        return const_iterator { pos, values_.begin() + index };
    }

private:
    struct Predicate : Compare {
        auto operator()(const key_type& lhs, const key_type& rhs)
        {
            return Compare::operator()(lhs, rhs);
        }
        auto operator()(const key_type& lhs, const value_type& rhs)
        {
            return Compare::operator()(lhs, rhs.first);
        }
        auto operator()(const value_type& lhs, const key_type& rhs)
        {
            return Compare::operator()(lhs.first, rhs);
        }
        auto operator()(const value_type& lhs, const value_type& rhs)
        {
            return Compare::operator()(lhs.first, rhs.first);
        }
    };
    BASE_NS::vector<key_type> keys_;
    BASE_NS::vector<mapped_type> values_;
};

template<typename Container>
bool operator!=(
    const typename BASE_NS::iterator<Container>& lhs, const typename BASE_NS::const_iterator<Container>& rhs) noexcept
{
    return lhs.ptr() != rhs.ptr();
}

template<typename Container>
bool operator!=(
    const typename BASE_NS::const_iterator<Container>& lhs, const typename BASE_NS::iterator<Container>& rhs) noexcept
{
    return lhs.ptr() != rhs.ptr();
}
BASE_END_NAMESPACE()
#endif // API_BASE_CONTAINERS_FLAT_MAP_H
