#pragma once

#include <cstddef>

#include "vector-view.hpp"
#include "small-vector.hpp"

namespace gnt {


    //TODO the NFields needs to be constant... for sure... we'll always know how many fields we have!

    template<typename ByteType = std::byte>
    class heterogenous_record_view {

    public:

        using byte_type = ByteType;
        using size_type = uint64_t;

        //Fixme: allow this method to do packs for arbitrary size-types

        //Builds a pack of ranges (spans, range_views, vectors etc.) into a small range, that can be a heterogenous record view.
        template<typename... InputRanges>
        constexpr static small_vector<byte_type> build_heterogenous_range(InputRanges... i_ranges) {
            static_assert(std::conjunction_v<std::is_convertible<typename InputRanges::value_type, byte_type>...>,
            "InputRanges must have byte_type value_type's");
            static_assert(sizeof(byte_type) == 1, "byte_type must be sizsof 1 - or it's not a byte type!");
            auto o = small_vector<byte_type>();
            constexpr auto n_scalars_per_integer = std::max(uint64_t(1), uint64_t(sizeof(uint64_t) / sizeof(byte_type)));
            constexpr auto record_header_size = n_scalars_per_integer * sizeof...(InputRanges) + 1;

            // Reserve size by fold expression that should sum up the sizes of all the ranges
            o.reserve(i_ranges.size() + ... + record_header_size);

            // Add the header
            //TODO TODO
            //TODO check resize exists.
            o.resize(sizeof...(InputRanges) * sizeof(size_type) + sizeof(byte_type));
            unsigned int i = 0;
            gt::pack_int(sizeof...(InputRanges), o.at_stride<sizeof(size_type)>(i++));
            // Fold up the ranges and write their sizes into the output
            (gt::pack_int(i_ranges.size(), o.at_stride<sizeof(size_type)>(i++)), ...);

            // Add the actual data of each of the ranges... (using o.end() this time)
            ((o.insert(o.end(), i_ranges.begin(), i_ranges.end())), ...);
            return o;
        }

        //TODO begin and end need to be super simple iterators too...

        //TODO span at

    };



//TODO:
//TODO for keys would be nice to have heterogenous_range_view

//TODO span_tuple<type, Extent1, Extent2, Extent3>

//TODO span_tuple_view<type, Extnet1, Extent2 etc.>
//TODO problem: how can we construct from a range_view - how can we have dynamic extent over a view?
//TODO better to have all the sizes at the beginning... but this has a flaw in that it messes up the ordering on the paths.
//TODO we could have all the sizes at the end...

//TODO of course the comparator makes it fine!!
//TODO so absolutely no need to worry about that... mostly...
//TODO I like the idea of a heterogenous_stride_view with the strides read off the beginning.
//TODO we can then wrap iterators etc that way and underneath it should go to spans... which can then plug into other views...
//TODO absolutely no chance of course of ever having such a thing dynamically but that should be fine

} //ns gnt