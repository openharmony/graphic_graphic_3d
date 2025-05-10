
#ifndef META_BASE_ALGORITHMS_H
#define META_BASE_ALGORITHMS_H

#include <stdint.h>

#include <base/containers/type_traits.h>
#include <base/containers/vector.h>

#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

constexpr size_t NPOS = -1;

/**
 * @brief Find index of first matching value in vector or NPOS if no such value
 * @param vec Vector to search
 * @param value Value to search
 * @param pos Index to start the search
 * @return First index where is 'value' or NPOS
 */
template<typename Type>
size_t FindFirstOf(const BASE_NS::vector<Type>& vec, const Type& value, size_t pos = 0)
{
    for (; pos < vec.size(); ++pos) {
        if (vec[pos] == value) {
            return pos;
        }
    }
    return NPOS;
}

META_END_NAMESPACE()

#endif
