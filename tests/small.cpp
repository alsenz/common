#include <vector>

#include "common/small.hpp"


#include "gtest/gtest.h"

TEST(CommonSmallTests, PilferingAround) {
    std::vector<uint64_t> vec;
    vec.push_back(5);
    vec.push_back(4);
    vec.push_back(3);
    vec.push_back(2);
    vec.push_back(1);
    EXPECT_EQ(5, vec.size());
    using pointer_type = uint64_t *;
    pointer_type ref = pointer_type(&vec.at(3));
    EXPECT_EQ(2, *ref);
    //TODO: so the legacy iterator is CONSTRUCTIBLE from a pointer ref...
    //TODO: so it's probably fine to use pointers as our underlying iterator type, we will just have to construct it!
    //TODO that's fine. This will still work when resizing vectors based on the pointer type we can just implicitly convert!
    vec.insert(std::vector<uint64_t>::iterator(ref), 42);
    EXPECT_EQ(42, vec.at(3));
}

TEST(CommonSmallTests, SanityCheck) {
    common::small_vector<char> vec;
    EXPECT_EQ(0, vec.size());
}