#include <vector>
#include <iostream>

#include "common/small-vector.hpp"


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

TEST(CommonSmallTests, PilferingAround2) {
    //std::vector<char[3]> vec;
    //TODO see how this thing is behaving...
    //TODO also see how span is behaving beneath..
    /*char red[3] = {'r', 'e', 'd'};
    std::cout << "vec.size()" << vec.size() << std::endl;
    vec.push_back(red);
    std::cout << "vec.size()" << vec.size() << std::endl;*/
    std::cout << "sizeof(char[3]): " << sizeof(char[3]) << std::endl;
    std::cout << "sizeof(std::array<char, 3>): " << sizeof(std::array<char, 3>) << std::endl;
    std::cout << "sizeof(std::array<std::array<char, 3>, 4>): " << sizeof(std::array<std::array<char, 3>, 4>) << std::endl;
    EXPECT_EQ(123,456);
}

TEST(CommonSmallTests, VectorSanityCheck) {
    common::small_range<char> vec;
    EXPECT_EQ(0, vec.size());
}


TEST(CommonSmallTests, MapSanityCheck) {
    common::small_byte_map<sizeof(uint64_t), sizeof(char)> smap;
    EXPECT_EQ(smap.begin(), smap.end());
    std::byte bytes[sizeof(uint64_t)];
    EXPECT_EQ(smap.find(bytes), smap.end());
    //TODO do an insert here as well...
}