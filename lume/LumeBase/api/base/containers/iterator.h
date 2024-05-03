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

#ifndef API_BASE_CONTAINERS_ITERATOR_H
#define API_BASE_CONTAINERS_ITERATOR_H

// Allow "std::random_access_iterator_tag" compatibility
#define BASE_STD_COMPATIBILITY

#ifdef BASE_STD_COMPATIBILITY
// include std::iterators so we can have compatibility
// and this spoils a lot.
// we only need the "std::random_access_iterator_tag"...
#include <iterator>
#endif

#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
#ifdef BASE_STD_COMPATIBILITY
using random_access_iterator_tag = std::random_access_iterator_tag; // yeah. we need this for std compatibility.
#else
struct random_access_iterator_tag {};
#endif

// Iterators are somewhat simplified from c++11 with a hint of c++17...
// but they are fully trivial..
template<class T>
class const_iterator {
public:
    using base_container = T;
    using base_iterator = typename T::iterator;
    using const_base_iterator = typename T::const_iterator;
    using iterator_category = random_access_iterator_tag;
    using value_type = typename T::value_type;
    using difference_type = typename T::difference_type;
    using pointer = typename T::const_pointer;
    using reference = typename T::const_reference;
    constexpr const_iterator() noexcept = default;
    ~const_iterator() = default;
    constexpr explicit const_iterator(const pointer ptr) noexcept : it_ { ptr } {}
    constexpr const_iterator(const base_iterator& other) noexcept : it_ { other.ptr() } {}
    constexpr const_iterator& operator++() noexcept
    {
        ++it_;
        return *this;
    }
    constexpr const_iterator operator++(int) noexcept
    {
        const_iterator t(*this);
        ++*this;
        return t;
    }
    constexpr const_iterator& operator--() noexcept
    {
        --it_;
        return *this;
    }
    constexpr const_iterator operator--(int) noexcept
    {
        const_iterator t(*this);
        --*this;
        return t;
    }
    constexpr reference operator*() const noexcept
    {
        return *it_;
    }
    constexpr pointer operator->() const noexcept
    {
        return it_;
    }
    constexpr const_iterator operator+(difference_type n) const noexcept
    {
        return const_iterator(it_ + n);
    }
    constexpr const_iterator& operator+=(difference_type n) noexcept
    {
        it_ += n;
        return *this;
    }
    constexpr const_iterator operator-(difference_type n) const noexcept
    {
        return const_iterator(it_ - n);
    }
    constexpr const_iterator& operator-=(difference_type n) noexcept
    {
        it_ -= n;
        return *this;
    }
    constexpr bool operator==(const const_iterator& other) const noexcept
    {
        return it_ == other.it_;
    }
    constexpr bool operator!=(const const_iterator& other) const noexcept
    {
        return it_ != other.it_;
    }
    constexpr difference_type operator-(const const_iterator& other) const noexcept
    {
        return static_cast<difference_type>(it_ - other.it_);
    }
    constexpr bool operator<(const const_iterator& other) const noexcept
    {
        return it_ < other.it_;
    }
    constexpr bool operator<=(const const_iterator& other) const noexcept
    {
        return it_ <= other.it_;
    }
    constexpr bool operator>(const const_iterator& other) const noexcept
    {
        return it_ > other.it_;
    }
    constexpr bool operator>=(const const_iterator& other) const noexcept
    {
        return it_ >= other.it_;
    }
    pointer ptr() const
    {
        return it_;
    }

protected:
    pointer it_;
};

template<class T>
class iterator {
public:
    using base_container = T;
    using iterator_category = random_access_iterator_tag;
    using base_iterator = typename T::iterator;
    using const_base_iterator = typename T::const_iterator;
    using value_type = typename T::value_type;
    using difference_type = typename T::difference_type;
    using pointer = typename T::pointer;
    using reference = typename T::reference;
    constexpr iterator() noexcept = default;
    ~iterator() = default;
    constexpr explicit iterator(const pointer ptr) noexcept : it_ { ptr } {}
    constexpr iterator& operator++() noexcept
    {
        ++it_;
        return *this;
    }
    constexpr iterator operator++(int) noexcept
    {
        iterator t(*this);
        ++*this;
        return t;
    }
    constexpr iterator& operator--() noexcept
    {
        --it_;
        return *this;
    }
    constexpr iterator operator--(int) noexcept
    {
        iterator t(*this);
        --*this;
        return t;
    }
    constexpr reference operator*() const noexcept
    {
        return *it_;
    }
    constexpr pointer operator->() const noexcept
    {
        return it_;
    }
    constexpr reference operator*() noexcept
    {
        return *it_;
    }
    constexpr pointer operator->() noexcept
    {
        return it_;
    }

    constexpr iterator operator+(difference_type n) const noexcept
    {
        return iterator(it_ + n);
    }
    constexpr iterator& operator+=(difference_type n) noexcept
    {
        it_ += n;
        return *this;
    }
    constexpr iterator operator-(difference_type n) const noexcept
    {
        return iterator(it_ - n);
    }
    constexpr iterator& operator-=(difference_type n) noexcept
    {
        it_ -= n;
        return *this;
    }
    constexpr bool operator==(const iterator& other) const noexcept
    {
        return it_ == other.it_;
    }
    constexpr bool operator!=(const iterator& other) const noexcept
    {
        return it_ != other.it_;
    }
    constexpr difference_type operator-(const iterator& other) const noexcept
    {
        return static_cast<difference_type>(it_ - other.it_);
    }

    constexpr bool operator<(const iterator& other) const noexcept
    {
        return it_ < other.it_;
    }
    constexpr bool operator<=(const iterator& other) const noexcept
    {
        return it_ <= other.it_;
    }
    constexpr bool operator>(const iterator& other) const noexcept
    {
        return it_ > other.it_;
    }
    constexpr bool operator>=(const iterator& other) const noexcept
    {
        return it_ >= other.it_;
    }

    pointer ptr() const
    {
        return it_;
    }

protected:
    pointer it_;
};

template<typename Iter>
class reverse_iterator {
public:
    using base_container = typename Iter::base_container;
    using iterator = typename base_container::iterator;
    using const_iterator = typename base_container::const_iterator;

    using iterator_type = Iter;
    using iterator_category = typename Iter::iterator_category;
    using value_type = typename Iter::value_type;
    using difference_type = typename Iter::difference_type;
    using pointer = typename Iter::pointer;
    using reference = typename Iter::reference;

    constexpr reverse_iterator() = default;
    ~reverse_iterator() = default;
    constexpr explicit reverse_iterator(iterator_type it) noexcept : it_(it) {}
    constexpr reverse_iterator(const reverse_iterator& other) noexcept : it_(other.base()) {}
    template<class U>
    constexpr reverse_iterator& operator=(const reverse_iterator<U>& other)
    {
        it_ = other.base();
        return *this;
    }
    constexpr reverse_iterator& operator++()
    {
        --it_;
        return *this;
    }
    constexpr reverse_iterator operator++(int)
    {
        reverse_iterator t(*this);
        --it_;
        return t;
    }
    constexpr reverse_iterator& operator--()
    {
        ++it_;
        return *this;
    }
    constexpr reverse_iterator operator--(int)
    {
        reverse_iterator t(*this);
        ++it_;
        return t;
    }

    constexpr reference operator*() const
    {
        iterator_type it = it_;
        return (--it).operator*();
    }
    constexpr pointer operator->() const
    {
        iterator_type it = it_;
        return (--it).operator->();
    }

    constexpr reverse_iterator operator+(difference_type n) const
    {
        return reverse_iterator(it_ - n);
    }
    constexpr reverse_iterator& operator+=(difference_type n)
    {
        it_ -= n;
        return *this;
    }
    constexpr reverse_iterator operator-(difference_type n) const
    {
        return reverse_iterator(it_ + n);
    }
    constexpr reverse_iterator& operator-=(difference_type n)
    {
        it_ += n;
        return *this;
    }
    constexpr difference_type operator-(const reverse_iterator& other) const
    {
        return other.it_ - it_;
    }
    constexpr bool operator==(const reverse_iterator& other) const
    {
        return it_ == other.it_;
    }
    constexpr bool operator!=(const reverse_iterator& other) const
    {
        return it_ != other.it_;
    }
    constexpr iterator_type base() const
    {
        return it_;
    }
    constexpr bool operator<(const reverse_iterator& other) const noexcept
    {
        return it_ > other.it_;
    }
    constexpr bool operator<=(const reverse_iterator& other) const noexcept
    {
        return it_ >= other.it_;
    }
    constexpr bool operator>(const reverse_iterator& other) const noexcept
    {
        return it_ < other.it_;
    }
    constexpr bool operator>=(const reverse_iterator& other) const noexcept
    {
        return it_ <= other.it_;
    }

private:
    Iter it_;
};

template<class Iter>
class move_iterator {
public:
    using iterator_type = Iter;
    using iterator_category = typename Iter::iterator_category;
    using value_type = typename Iter::value_type;
    using difference_type = typename Iter::difference_type;
    using pointer = Iter;
    using reference = value_type&&;

    /** constructs a new iterator adaptor */
    constexpr move_iterator() = default;

    constexpr explicit move_iterator(iterator_type x) : current_(x) {}

    template<class U>
    constexpr move_iterator(const move_iterator<U>& other) : current_(other.current)
    {}

    /** assigns another iterator */
    template<class U>
    constexpr move_iterator& operator=(const move_iterator<U>& other)
    {
        current_ = other.current;
        return *this;
    }

    /** accesses the underlying iterator */
    constexpr iterator_type base() const
    {
        return current_;
    }

    /** accesses the pointed-to element */
    constexpr reference operator*() const
    {
        return static_cast<reference>(*current_);
    }

    constexpr pointer operator->() const
    {
        return current_;
    }

    /** accesses an element by index */
    constexpr reference operator[](difference_type n) const
    {
        return static_cast<reference>(current_[n]);
    }

    /** advances or decrements the iterator */
    constexpr move_iterator& operator++()
    {
        ++current_;
        return *this;
    }
    constexpr move_iterator& operator--()
    {
        --current_;
        return *this;
    }
    constexpr move_iterator operator++(int)
    {
        move_iterator before(*this);
        ++current_;
        return before;
    }

    constexpr move_iterator operator--(int)
    {
        move_iterator before(*this);
        --current_;
        return before;
    }

    constexpr move_iterator operator+(difference_type n) const
    {
        return move_iterator(current_ + n);
    }

    constexpr move_iterator operator-(difference_type n) const
    {
        return move_iterator(current_ - n);
    }

    constexpr move_iterator& operator+=(difference_type n)
    {
        current_ += n;
        return *this;
    }

    constexpr move_iterator& operator-=(difference_type n)
    {
        current_ -= n;
        return *this;
    }

private:
    Iter current_;
};

template<class Iterator1, class Iterator2>
constexpr bool operator==(const move_iterator<Iterator1>& lhs, const move_iterator<Iterator2>& rhs)
{
    return lhs.base() == rhs.base();
}

template<class Iterator1, class Iterator2>
constexpr bool operator!=(const move_iterator<Iterator1>& lhs, const move_iterator<Iterator2>& rhs)
{
    return lhs.base() != rhs.base();
}

template<class Iterator1, class Iterator2>
constexpr bool operator<(const move_iterator<Iterator1>& lhs, const move_iterator<Iterator2>& rhs)
{
    return lhs.base() < rhs.base();
}

template<class Iterator1, class Iterator2>
constexpr bool operator<=(const move_iterator<Iterator1>& lhs, const move_iterator<Iterator2>& rhs)
{
    return lhs.base() <= rhs.base();
}

template<class Iterator1, class Iterator2>
constexpr bool operator>(const move_iterator<Iterator1>& lhs, const move_iterator<Iterator2>& rhs)
{
    return lhs.base() > rhs.base();
}

template<class Iterator1, class Iterator2>
constexpr bool operator>=(const move_iterator<Iterator1>& lhs, const move_iterator<Iterator2>& rhs)
{
    return lhs.base() >= rhs.base();
}

template<class Iter>
constexpr move_iterator<Iter> operator+(typename move_iterator<Iter>::difference_type n, const move_iterator<Iter>& it)
{
    return move_iterator(it.base() + n);
}

template<class Iterator1, class Iterator2>
constexpr auto operator-(const move_iterator<Iterator1>& lhs, const move_iterator<Iterator2>& rhs)
    -> decltype(lhs.base() - rhs.base())
{
    return lhs.base() - rhs.base();
}
template<class Iter>
constexpr move_iterator<Iter> make_move_iterator(Iter i)
{
    return move_iterator<Iter> { i };
}
BASE_END_NAMESPACE()

#endif // API_BASE_CONTAINERS_ITERATOR_H
