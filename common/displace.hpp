#pragma once

#include <algorithm>

namespace gnt {

    // Some slightly more descriptive aliases for copy in cases where InputIt == OutputIt

    // Returns InputIt one past last copied.
    // Note: requires random access iterator
    // Note: requires last+n to be strictly less than the end of the range, or undefined behaviour
    template<class InputIt>
    InputIt displace_right_n(InputIt first, InputIt last, std::size_t n) {
        return std::copy_backward(first, last, first+n);
    }

    // Returns InputIt one past last copied.
    // Note: requires bidirectional random access iterator
    // Note: requires last-n to be greater or equal to the start of the range.
    template<class InputIt>
    InputIt displace_left_n(InputIt first, InputIt last, std::size_t n) {
        return std::copy(first, last, first-n);
    }

} //ns gnt
