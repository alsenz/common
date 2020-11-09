#pragma once

#include <cstddef>
#include <iterator>

#include "nonstd/span.hpp"


namespace gnt {

    template<typename T, size_t Extent = nonstd::dynamic_extent>
    using stride = nonstd::span<T, Extent>;

    template<typename T, size_t Stride>
    struct stride_iterator { //Fixme: rename to small_step_iterator

        using value_type = T;
        using reference = T &;
        using span = nonstd::span<T, Stride>;
        using pointer = T *;

        using difference_type = std::size_t; //Fixme: parameterize
        using iterator_category = std::bidirectional_iterator_tag;

        stride_iterator(pointer data) : p_data(data) {}

        pointer begin() {
            return p_data;
        }

        pointer end() {
            return p_data + Stride;
        }

        const  pointer begin() const {
            return p_data;
        }

        const pointer end() const {
            return p_data + Stride;
        }

        const  pointer cbegin() const {
            return p_data;
        }

        const pointer cend() const {
            return p_data + Stride;
        }

        span operator* () {
            return to_span();
        }

        span operator-> () {
            return to_span();
        }

        span operator* () const {
            return to_span();
        }

        span operator-> () const {
            return to_span();
        }

        span to_span() const {
            return span(p_data, Stride);
        }

        span to_span() {
            return span(p_data, Stride);
        }

        operator span () const {
            return to_span();
        }

        operator span () {
            return to_span();
        }

        stride_iterator<T, Stride> &operator++() {
            p_data += Stride;
            return *this;
        }

        stride_iterator<T, Stride> operator++(int) {
            return stride_iterator(p_data + Stride);
        }

        stride_iterator<T, Stride> &operator--() {
            p_data -= Stride;
            return *this;
        }

        stride_iterator<T, Stride> operator--(int) {
            return stride_iterator(p_data - Stride);
        }

        //Fixme: this should be difference type in the argument
        stride_iterator<T, Stride> operator+ (difference_type n) const {
            return stride_iterator(p_data + n * Stride);
        }

        //Fixme: this should be difference type in the argument
        stride_iterator<T, Stride> operator- (difference_type n) const {
            return stride_iterator(p_data - n * Stride);
        }

        // Gets the number of steps between this iterator and the other
        difference_type operator- (const stride_iterator<T, Stride> &other) const {
            return (p_data - other.p_data) / Stride;
        }

        bool operator== (const stride_iterator<T, Stride> &other) const {
            return to_span() == other.to_span();
        }

        bool operator!= (const stride_iterator<T, Stride> &other) const {
            return to_span() != other.to_span();
        }

        pointer p_data;

    };

} //ns gnt

namespace std {

    template<typename T, size_t Stride>
    struct iterator_traits<gnt::stride_iterator<T, Stride>> {

    using difference_type = typename gnt::stride_iterator<T, Stride>::difference_type;
    using value_type = typename gnt::stride_iterator<T, Stride>::value_type;
    using pointer = typename gnt::stride_iterator<T, Stride>::pointer;
    using reference = typename gnt::stride_iterator<T, Stride>::reference;
    using iterator_category = typename gnt::stride_iterator<T, Stride>::iterator_category;

};

}
