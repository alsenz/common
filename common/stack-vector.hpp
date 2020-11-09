#pragma once

#include "vector-view.hpp"

namespace gnt {


    //TODO: create a stack_vector class

    /**
     * A vector of up to Extent size consisting of an array and a count.
     * Usually used with small_vector for small stack optimisations.
     *
     * @tparam T
     * @tparam Extent Size of the underlying array. By default, at least one or enough
     * ... to store 128 bytes, or 32 uint64_t's.
     */
    template<typename T, size_t Extent = (64 / sizeof(T)) + 1>
    class stack_vector : vector_view<T> {
        //TODO copy various bits from the other class... this makes more sense this way....
    };




} //ns gnt