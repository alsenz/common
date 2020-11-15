#pragma once

#include <cstddef>
#include <string>
#include <cmath>

#include "common/uuid.hpp"
#include "common/optional-inspect.hpp"
#include "common/nonstd/span.hpp"

namespace gnt {

    //TODO: alternate route for packing double:
    //TODO: write the number of significant bits (from right, + some decimal precision). This goes into 1 int.
    //TODO: then write the value of that number up until the precision.
    //TODO: (flipping bits if negative!)

    // Utility method for serialising
    void pack_int(uint64_t i, nonstd::span<char> target);

    uint64_t unpack_int(nonstd::span<char> chars);

    // Another utility for reading straight off a key
    uint64_t unpack_int(const std::string &str, const std::size_t offset);

    //TODO: needs testing
    void pack_double(double d, nonstd::span<char> chars);

    double unpack_double(nonstd::span<char> chars);

    sole::uuid unpack_uuid(nonstd::span<char> chars);

    void pack_uuid(const sole::uuid &u, nonstd::span<char> sv);







} //ns gnt
