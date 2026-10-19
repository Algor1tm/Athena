#pragma once

#include "Athena/Core/Core.h"
#include <xhash>


namespace Athena
{
    template <class T>
    inline void HashCombine(uint64& seed, const T& v) 
    {
        seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    inline uint64 HashSzudzikPair(uint32 x, uint32 y) 
    {
        return (x >= y) ? (x * x + x + y) : (y * y + x);
    }
}

namespace std
{
    template<typename T>
    struct hash<::Athena::Ref<T>>
    {
        size_t operator()(const ::Athena::Ref<T>& value) const
        {
            return (size_t)value.Raw();
        }
    };

}
