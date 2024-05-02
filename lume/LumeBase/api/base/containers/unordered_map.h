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

#ifndef API_BASE_CONTAINERS_UNORDERED_MAP_H
#define API_BASE_CONTAINERS_UNORDERED_MAP_H

#include <cstddef>
#include <cstdint>

#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <base/util/log.h>

BASE_BEGIN_NAMESPACE()
template<class T>
class unordered_map_iterator;
template<class T>
class const_unordered_map_iterator;

template<class T1, class T2>
struct pair {
    using first_type = T1;
    using second_type = T2;
    first_type first;
    second_type second;
};

#ifdef BASE_STD_COMPATIBILITY
using forward_iterator_tag = std::forward_iterator_tag; // yeah. we need this for std compatibility.
#else
struct forward_iterator_tag {};
#endif

template<class T>
class const_unordered_map_iterator {
public:
    using base_container = T;

    using iterator_category = forward_iterator_tag;
    using value_type = typename base_container::value_type;
    using difference_type = ptrdiff_t;
    using pointer = typename base_container::const_pointer;
    using reference = typename base_container::const_reference;

    using node_type = typename base_container::list_node;
    using iterator = typename base_container::const_iterator;

    constexpr const_unordered_map_iterator() noexcept = default;
    constexpr const_unordered_map_iterator(const typename base_container::iterator& other) noexcept
        : owner_ { other.owner_ }, it_ { other.it_ }
    {}

    constexpr reference operator*() const noexcept
    {
        BASE_ASSERT(owner_ && it_);
        return it_->data;
    }
    constexpr pointer operator->() const noexcept
    {
        BASE_ASSERT(owner_ && it_);
        return &it_->data;
    }

    constexpr bool operator==(const iterator& other) const noexcept
    {
        return ((owner_ == other.owner_) && (it_ == other.it_));
    }
    constexpr bool operator!=(const iterator& other) const noexcept
    {
        return ((owner_ != other.owner_) || (it_ != other.it_));
    }

    constexpr bool operator==(const typename base_container::iterator& other) const noexcept
    {
        return ((owner_ == other.owner_) && (it_ == other.it_));
    }
    constexpr bool operator!=(const typename base_container::iterator& other) const noexcept
    {
        return ((owner_ != other.owner_) || (it_ != other.it_));
    }
    constexpr iterator& operator++() noexcept
    {
        it_ = owner_->advance(*this);
        return *this;
    }

protected:
    constexpr explicit const_unordered_map_iterator(const T& owner, const node_type* ptr) noexcept
        : owner_(&owner), it_ { ptr }
    {}
    friend T;
    friend class unordered_map_iterator<T>;
    const T* owner_ { nullptr };
    const node_type* it_ { nullptr };
};

template<class T>
class unordered_map_iterator {
public:
    using base_container = T;

    using iterator_category = forward_iterator_tag;
    using value_type = typename base_container::value_type;
    using difference_type = ptrdiff_t;
    using pointer = typename base_container::pointer;
    using reference = typename base_container::reference;

    using node_type = typename base_container::list_node;
    using iterator = typename base_container::iterator;
    using const_iterator = typename base_container::const_iterator;

    constexpr unordered_map_iterator() noexcept = default;

    constexpr reference operator*() const noexcept
    {
        BASE_ASSERT(owner_ && it_);
        return it_->data;
    }
    constexpr pointer operator->() const noexcept
    {
        BASE_ASSERT(owner_ && it_);
        return &it_->data;
    }
    constexpr reference operator*() noexcept
    {
        BASE_ASSERT(owner_ && it_);
        return it_->data;
    }
    constexpr pointer operator->() noexcept
    {
        BASE_ASSERT(owner_ && it_);
        return &it_->data;
    }

    constexpr bool operator==(const iterator& other) const noexcept
    {
        return ((owner_ == other.owner_) && (it_ == other.it_));
    }
    constexpr bool operator!=(const iterator& other) const noexcept
    {
        return ((owner_ != other.owner_) || (it_ != other.it_));
    }

    constexpr bool operator==(const typename base_container::const_iterator& other) const noexcept
    {
        return ((owner_ == other.owner_) && (it_ == other.it_));
    }
    constexpr bool operator!=(const typename base_container::const_iterator& other) const noexcept
    {
        return ((owner_ != other.owner_) || (it_ != other.it_));
    }

    constexpr iterator& operator++() noexcept
    {
        it_ = owner_->advance(*this);
        return *this;
    }

protected:
    constexpr explicit unordered_map_iterator(T& owner, node_type* ptr) noexcept : owner_ { &owner }, it_ { ptr } {}
    friend T;
    friend const_unordered_map_iterator<T>;
    T* owner_ { nullptr };
    node_type* it_ { nullptr };
};

template<class data_type>
struct node_handle {
    using key_type = typename data_type::value_type::first_type;
    using mapped_type = typename data_type::value_type::second_type;
    using value_type = data_type;
    [[nodiscard]] bool empty() const noexcept
    {
        return pointer == nullptr;
    }
    explicit operator bool() const noexcept
    {
        return !empty();
    }
    key_type& key() const
    {
        return pointer->data.first;
    }
    mapped_type& mapped() const
    {
        return pointer->data.second;
    }
    data_type* pointer { nullptr };
};

template<class Key, class T>
class unordered_map_base {
    constexpr static uint32_t DEFAULT_SHIFT_AMOUNT = 4; // 1<<4 (16) initial buckets, seems to match ms stl.
    constexpr uint32_t get_sa(size_t const count, uint32_t const sa)
    {
        uint32_t ret = sa;
        while ((size_t(1) << ret) < count) {
            ret++;
        }
        return ret;
    }

public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = pair<const Key, T>;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = BASE_NS::unordered_map_iterator<unordered_map_base<Key, T>>;
    using const_iterator = BASE_NS::const_unordered_map_iterator<unordered_map_base<Key, T>>;
    struct list_node {
        using value_type = pair<const Key, T>;
        using first_type = typename value_type::first_type;
        using second_type = typename value_type::second_type;
        value_type data {};
        struct list_node* prev { nullptr };
        struct list_node* next { nullptr };

        list_node(const Key& x, const T& y) : data { x, y } {}
        list_node(Key&& x, const T& y) : data { BASE_NS::forward<Key>(x), y } {}
        list_node(Key&& x, T&& y) : data { BASE_NS::forward<Key>(x), BASE_NS::forward<T>(y) } {}
        list_node(const Key& x, T&& y) : data { x, BASE_NS::forward<T>(y) } {}
        explicit list_node(value_type&& val) : data { BASE_NS::forward<value_type>(val) } {}
        explicit list_node(const value_type& val) : data { val } {}
    };
    using node_type = node_handle<list_node>;

    unordered_map_base() = default;
    explicit unordered_map_base(size_t bucket_count)
        : shift_amount_ { get_sa(bucket_count, DEFAULT_SHIFT_AMOUNT) }, buckets_ { 1ull << shift_amount_ }
    {}
    ~unordered_map_base()
    {
        clear();
    }
    void reserve(size_t count)
    {
        const uint32_t new_shift_amount = get_sa(count, DEFAULT_SHIFT_AMOUNT);
        if (new_shift_amount > shift_amount_) {
            shift_amount_ = new_shift_amount - 1;
            rehash();
        }
    }
    void clear()
    {
        if (!empty()) {
            for (auto t : buckets_) {
                auto next = t;
                for (; next != nullptr;) {
                    auto b = next;
                    next = b->next;
                    release(b);
                }
            }
            buckets_.clear();
            buckets_.resize(1ull << shift_amount_);
            size_ = 0;
        }
    }
    unordered_map_base(unordered_map_base&& other) noexcept
        : size_ { other.size_ }, shift_amount_ { other.shift_amount_ }, buckets_ { BASE_NS::move(other.buckets_) }
    {
        // move..
        other.size_ = 0;
        other.shift_amount_ = 0;
    }
    unordered_map_base(const unordered_map_base& other)
        : shift_amount_ { other.shift_amount_ }, buckets_ { 1ull << shift_amount_ }
    {
        // copy.
        for (auto b : other.buckets_) {
            list_node* last = nullptr;
            if (b) {
                auto ind = index(b->data.first);
                for (; b != nullptr; b = b->next) {
                    auto nb = allocate(b->data);
                    if (last == nullptr) {
                        last = buckets_[ind] = nb;
                    } else {
                        nb->prev = last;
                        last = last->next = nb;
                    }
                    size_++;
                }
            }
        }
    }
    bool empty() const
    {
        return size_ == 0;
    }
    size_t size() const
    {
        return size_;
    }
    iterator erase(const_iterator pos)
    {
        if (pos.owner_ != this || !pos.it_) {
            return end();
        }
        list_node* node = nullptr;
        const auto ind = index(pos.it_->data.first);

        auto next = pos.it_->next;
        // link prev to next or set bucket start to next if this was the first node.
        // if this was the last prev-next will be null or bucket will be empty.
        if (pos.it_->prev) {
            BASE_ASSERT(pos.it_ == pos.it_->prev->next);
            node = pos.it_->prev->next;
            pos.it_->prev->next = next;
        } else {
            BASE_ASSERT(pos.it_ == buckets_[ind]);
            node = buckets_[ind];
            buckets_[ind] = next;
        }

        // link next to prev or if this was the last node look or the next bucket with items.
        // if this was the first next-prev will be null
        if (next) {
            next->prev = node->prev;
        } else {
            // okay, advance to next bucket..
            for (auto i = ind + 1; i < buckets_.size(); ++i) {
                next = buckets_[i];
                if (next) {
                    break;
                }
            }
        }
        --size_;
        release(node);

        return iterator { *this, next };
    }
    size_t erase(const key_type& key)
    {
        if (auto entry = detach_entry(key)) {
            release(entry);
            return 1u;
        }
        return 0u;
    }
    node_type extract(const key_type& key)
    {
        return node_type { detach_entry(key) };
    }
    pair<iterator, bool> insert(value_type&& v)
    {
        const auto ind = index(v.first);
        auto entry = get_entry(ind, v.first);
        if (entry) {
            return { iterator { *this, entry }, false };
        }
        return { iterator { *this, create_entry(ind, BASE_NS::forward<value_type>(v)) }, true };
    }
    pair<iterator, bool> insert(const value_type& v)
    {
        const auto ind = index(v.first);
        auto entry = get_entry(ind, v.first);
        if (entry) {
            return { iterator { *this, entry }, false };
        }
        return { iterator { *this, create_entry(ind, v.first, v.second) }, true };
    }
    auto insert(node_type&& nh)
    {
        struct insert_return_type {
            iterator position;
            bool inserted;
            node_type node;
        };
        if (nh.empty()) {
            return insert_return_type { end(), false };
        }
        const auto& key = nh.pointer->data.first;
        const auto ind = index(key);
        auto res = get_entry(ind, key);
        if (res) {
            return insert_return_type { iterator { *this, res }, false, nh };
        }
        auto nl = nh.pointer;
        nh.pointer = nullptr;
        return insert_return_type { iterator { *this, create_entry(ind, nl) }, true };
    }
    template<class M>
    pair<iterator, bool> insert_or_assign(const key_type& key, M&& value)
    {
        const auto ind = index(key);
        auto res = get_entry(ind, key);
        if (res) {
            res->data.second = BASE_NS::forward<M>(value);
            return { iterator { *this, res }, false };
        }
        auto nl = allocate(key, BASE_NS::forward<M>(value));
        return { iterator { *this, create_entry(ind, nl) }, true };
    }
    iterator begin() noexcept
    {
        for (auto t : buckets_) {
            if (t) {
                return iterator { *this, t };
            }
        }
        return end();
    }
    const_iterator begin() const noexcept
    {
        for (auto t : buckets_) {
            if (t) {
                return const_iterator { *this, t };
            }
        }
        return end();
    }
    const_iterator cbegin() const noexcept
    {
        return begin();
    }

    iterator end() noexcept
    {
        return iterator { *this, nullptr };
    }
    const_iterator end() const noexcept
    {
        return const_iterator { *this, nullptr };
    }
    const_iterator cend() const noexcept
    {
        return end();
    }
    iterator find(const key_type& key)
    {
        return iterator { *this, get_entry(index(key), key) };
    }
    const_iterator find(const key_type& key) const
    {
        return const_iterator { *this, get_entry(index(key), key) };
    }
    bool contains(const key_type& key) const
    {
        return (get_entry(index(key), key) != nullptr);
    }
    size_t count(const key_type& key) const
    {
        return contains(key) ? 1U : 0U;
    }
    mapped_type& operator[](const key_type& key)
    {
        const auto ind = index(key);
        auto b = get_entry(ind, key);
        if (b) {
            return b->data.second;
        }
        return create_entry(ind, key)->data.second;
    }
    mapped_type& operator[](key_type&& key)
    {
        const auto ind = index(key);
        auto b = get_entry(ind, key);
        if (b) {
            return b->data.second;
        }
        return create_entry(ind, BASE_NS::forward<key_type>(key))->data.second;
    }

    unordered_map_base& operator=(unordered_map_base&& other) noexcept
    {
        if (&other != this) {
            // move..
            clear();
            size_ = other.size_;
            other.size_ = 0;
            shift_amount_ = other.shift_amount_;
            buckets_ = BASE_NS::move(other.buckets_);
        }
        return *this;
    }
    unordered_map_base& operator=(const unordered_map_base& other)
    {
        if (&other != this) {
            // copy.
            clear();
            shift_amount_ = other.shift_amount_;
            buckets_.resize(1ull << shift_amount_);
            for (auto b : other.buckets_) {
                list_node* last = nullptr;
                if (b) {
                    uint32_t ind = index(b->data.first);
                    for (; b != nullptr; b = b->next) {
                        auto nb = allocate(b->data);
                        if (last == nullptr) {
                            last = buckets_[ind] = nb;
                        } else {
                            nb->prev = last;
                            last = last->next = nb;
                        }
                        size_++;
                    }
                }
            }
        }
        return *this;
    }

protected:
    friend iterator;
    friend const_iterator;

    // helpers for the iterators. (perhaps link the nodes across buckets?)
    list_node* advance(const const_iterator& it, uint32_t count = 1) const
    {
        list_node* next = nullptr;
        while (count--) {
            if (it.it_->next) {
                next = it.it_->next;
            } else {
                // okay, advance to next bucket..
                uint32_t ind = index(it.it_->data.first);
                for (;;) {
                    ind++;
                    if (ind == buckets_.size()) {
                        next = nullptr;
                        break;
                    }
                    next = buckets_[ind];
                    if (next) {
                        break;
                    }
                }
            }
        }
        return next;
    }

    list_node* create_entry(uint32_t ind, const key_type& key)
    {
        return create_entry(ind, allocate(key, mapped_type {}));
    }
    list_node* create_entry(uint32_t ind, key_type&& key)
    {
        return create_entry(ind, allocate(BASE_NS::forward<key_type>(key), mapped_type {}));
    }
    list_node* create_entry(uint32_t ind, const key_type& key, const mapped_type& value)
    {
        return create_entry(ind, allocate(key, value));
    }
    list_node* create_entry(uint32_t ind, key_type&& key, mapped_type&& value)
    {
        return create_entry(ind, allocate(BASE_NS::forward<key_type>(key), BASE_NS::forward<mapped_type>(value)));
    }
    list_node* create_entry(uint32_t ind, value_type&& v)
    {
        return create_entry(ind, allocate(BASE_NS::forward<value_type>(v)));
    }

    template<class... Args>
    list_node* allocate(Args&&... args)
    {
        auto alloc = buckets_.getAllocator();
        auto node = static_cast<list_node*>(alloc.alloc(alloc.instance, sizeof(list_node)));
        ::new (node) list_node { BASE_NS::forward<Args>(args)... };
        return node;
    }

    void release(list_node* node)
    {
        if constexpr (!__is_trivially_destructible(list_node)) {
            node->~list_node();
        }
        auto alloc = buckets_.getAllocator();
        alloc.free(alloc.instance, node);
    }

    list_node* create_entry(uint32_t ind, list_node* nh)
    {
        // check load level.. and resize/rehash if needed
        bool resize = false;
        if (buckets_.empty()) {
            resize = true;
        } else {
            const float load = size_ / static_cast<float>(buckets_.size());
            resize = (load > 0.7f);
        }
        if (resize) {
            rehash();
            // need to recalculate/rehash the index
            ind = index(nh->data.first);
        }
        // okay. so add it as first..
        auto old = buckets_[ind];
        buckets_[ind] = nh;
        buckets_[ind]->next = old;
        if (old) {
            old->prev = buckets_[ind];
        }
        size_++;
        return buckets_[ind];
    }
    template<class k>
    list_node* get_entry(uint32_t index, const k& key) const
    {
        if (index >= buckets_.size()) {
            // invalid!
            return nullptr;
        }
        auto b = buckets_[index];
        while (b) {
            if (b->data.first == key) {
                return b;
            }
            b = b->next;
        }
        // invalid!
        return nullptr;
    }
    // maps key to bucket index.
    // first uses the hash function to convert key to 64 bit hash.
    // and then uses "fibonacci hashing"/"Knuth’s multiplicative method"
    // with additional magic to  fix high bits
    // and to map the hash to 0 - buckets_.size() range.
    // see:
    // https://probablydance.com/2018/06/16/fibonacci-hashing-the-optimization-that-the-world-forgot-or-a-better-alternative-to-integer-modulo/
    template<class k>
    uint32_t index(const k& key) const
    {
        if (shift_amount_) {
            const uint64_t GOLDEN_RATIO_64 = 0x9E3779B97F4A7C15ull;
            const uint64_t shift = 64u - shift_amount_;
            uint64_t h = hash(key);
            h ^= h >> shift;
            return static_cast<uint32_t>(((GOLDEN_RATIO_64 * h) >> shift));
        }
        return 0u;
    }
    template<class k>
    list_node* detach_entry(const k& key)
    {
        const auto ind = index(key);
        auto entry = get_entry(ind, key);
        if (entry) {
            if (entry->prev) {
                entry->prev->next = entry->next;
            } else {
                buckets_[ind] = entry->next;
            }
            if (entry->next) {
                entry->next->prev = entry->prev;
            }
            entry->prev = nullptr;
            entry->next = nullptr;
            --size_;
        }
        return entry;
    }
    void rehash()
    {
        BASE_NS::vector<list_node*> tmp(BASE_NS::move(buckets_));
        shift_amount_++;
        if (shift_amount_ < DEFAULT_SHIFT_AMOUNT) {
            // make sure that we always have atleast 1<<DEFAULT_SHIFT_AMOUNT buckets.
            shift_amount_ = DEFAULT_SHIFT_AMOUNT;
        }
        buckets_.resize(1ull << shift_amount_);
        for (auto b : tmp) {
            for (; b != nullptr;) {
                auto next = b->next;
                const uint32_t ind = index(b->data.first);
                b->next = buckets_[ind];
                buckets_[ind] = b;
                if (buckets_[ind]->next) {
                    buckets_[ind]->next->prev = buckets_[ind];
                }
                b->prev = nullptr;
                b = next;
            }
        }
    }

    // linked list in buckets_...
    size_t size_ { 0 };
    uint32_t shift_amount_ { 0 };
    BASE_NS::vector<list_node*> buckets_;
    iterator make_iterator(list_node* node)
    {
        return iterator { *this, node };
    }
    const_iterator make_const_iterator(const list_node* node) const
    {
        return const_iterator { *this, node };
    }
};

template<class Key, class T>
class unordered_map : public unordered_map_base<Key, T> {
public:
    using unordered_map_base<Key, T>::unordered_map_base;
};

template<class T>
class unordered_map<BASE_NS::string, T> : public unordered_map_base<BASE_NS::string, T> {
    // Specialization of unordered_map with string key but
    // accepts string_view as key in certain basic operations..
    using base = unordered_map_base<BASE_NS::string, T>;

public:
    using unordered_map_base<BASE_NS::string, T>::unordered_map_base;
    auto find(const string_view& key)
    {
        return base::make_iterator(base::get_entry(base::index(key), key));
    }
    auto find(const string_view& key) const
    {
        return base::make_const_iterator(base::get_entry(base::index(key), key));
    }
    bool contains(const string_view& key) const
    {
        return (base::get_entry(base::index(key), key) != nullptr);
    }
    size_t count(const string_view& key) const
    {
        return contains(key) ? 1U : 0U;
    }

    // expose string overloads as well
    using base::operator[];
    using base::erase;

    auto& operator[](const string_view& key)
    {
        const auto ind = base::index(key);
        if (auto b = base::get_entry(ind, key)) {
            return b->data.second;
        }
        return base::create_entry(ind, typename base::key_type(key))->data.second;
    }

    size_t erase(const string_view& key)
    {
        if (auto* entry = base::detach_entry(key)) {
            base::release(entry);
            return 1u;
        }
        return 0u;
    }

    pair<typename base::iterator, bool> insert(pair<string_view, T>&& v)
    {
        const auto ind = base::index(v.first);
        auto entry = base::get_entry(ind, v.first);
        if (entry) {
            return { base::make_iterator(entry), false };
        }
        entry = base::create_entry(
            ind, typename base::key_type(v.first), BASE_NS::forward<typename base::mapped_type>(v.second));
        return { base::make_iterator(entry), true };
    }

    pair<typename base::iterator, bool> insert(const pair<string_view, T>& v)
    {
        const auto ind = base::index(v.first);
        auto entry = base::get_entry(ind, v.first);
        if (entry) {
            return { base::make_iterator(entry), false };
        }
        entry = base::create_entry(ind, typename base::key_type(v.first), v.second);
        return { base::make_iterator(entry), true };
    }

    template<class M>
    pair<typename base::iterator, bool> insert_or_assign(const string_view& key, M&& value)
    {
        const auto ind = base::index(key);
        auto entry = base::get_entry(ind, key);
        if (entry) {
            entry->data.second = BASE_NS::forward<M>(value);
            return { base::make_iterator(entry), false };
        }
        entry = base::create_entry(ind, typename base::key_type(key), BASE_NS::forward<M>(value));
        return { base::make_iterator(entry), true };
    }
};
BASE_END_NAMESPACE()

#endif // API_BASE_CONTAINERS_UNORDERED_MAP_H
