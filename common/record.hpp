#pragma once

#include <cstddef>
#include <algorithm>
#include <array>

#include "vector-view.hpp"
#include "small-vector.hpp"
#include "pack.hpp"

namespace gnt {

    //Fixme: don't use size_t here - use a fixed width integer type for forward compatibility reasons!

    template<size_t N, typename ByteType = std::byte>
    using record_view = std::array<nonstd::span<ByteType>, N>;

    template<typename ByteType, size_t N>
    constexpr small_vector<ByteType> make_vector(const record_view<N, ByteType> &rv) {
        const std::size_t total_bytes = std::accumulate(rv.begin(), rv.end(), (std::size_t) 0,
            [](std::size_t acc, auto spn){
            return acc + spn;
        });
        //Header: TOTAL_FIELDS, FIELD_SIZES..., FIELD BYTES (CONTIGUOUS)
        const std::size_t header_bytes = sizeof(std::size_t) + sizeof(std::size_t) * N;
        const std::size_t total_bytes_with_header = header_bytes + total_bytes;
        auto result = small_vector<ByteType>(total_bytes_with_header); //Small vector does resize with this ctor, not fill
        // So we will here:
        result.resize(total_bytes_with_header);

        // Now let's add the first uint64 - the number of fields
        gnt::pack_int(rv.size(), result.at_stride<sizeof(std::size_t)>(0));
        // Then let's pack each of the sizes, followed by each of the
        uint64_t accum = header_bytes;
        for(uint64_t i = 0; i < rv.size(); ++i) {
            const auto &spn = rv.at(i);
            const auto ith_sz = spn.size();
            gnt::pack_int(ith_sz, result.at_stride<sizeof(std::size_t)>(i+1));
            // Accum should be pointing at the first byte of the next field- so we should be able to just copy over
            std::copy(spn.begin(), spn.end(), result.begin() + accum);
            accum += ith_sz;
        }

        return result;
    }

    template<size_t N, size_t Extent, typename ByteType = std::byte>
    constexpr const record_view<N, ByteType> make_record_view(const vector_view<ByteType, Extent> &record_bytes) {
        const std::size_t size_check = gnt::unpack_int(record_bytes.at_stride<sizeof(std::size_t)>(0));
        if(size_check != N) {
            throw std::logic_error("make_record_view underlying field does not have exactly N fields");
        }
        auto result = record_view<N, ByteType>();
        const std::size_t header_bytes = sizeof(std::size_t) + sizeof(std::size_t) * N;
        auto accum = header_bytes;
        for(std::size_t i = 0; i < N; ++i) {
            const auto field_size = gnt::unpack_int(record_bytes.at_stride<sizeof(std::size_t)>(i + 1));
            const auto start_it = record_bytes.begin() + accum;
            const auto end_it = start_it + field_size;
            result.at(i) = nonstd::span<ByteType>(start_it, end_it);
            accum += field_size;
        }
        return result;
    }

} //ns gnt