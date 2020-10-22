#pragma once

#pragma once

#include "caf/all.hpp"

// Forward declare everything we're adding
namespace gt::common {

    // Block out a silly custom type block
    constexpr caf::type_id_t custom_type_block_start = 19100; //Some sufficiently large far-away type block - doesn't conflict with net
    constexpr caf::type_id_t custom_type_block_end = custom_type_block_start + 32; //Leaves us 32 custom types
    constexpr caf::type_id_t custom_type_block_safe_max = custom_type_block_start + 999; // We promsie not to use more than a thosuand typeids.


} //ns as net

CAF_BEGIN_TYPE_ID_BLOCK(gt_common, gt::common::custom_type_block_start)

//TODO

CAF_END_TYPE_ID_BLOCK(gt_common)