#pragma once

#include "nonstd/span.hpp"

#include "displace.hpp"
#include "stride.hpp"

namespace gnt {

    /**
     * A wrapper around a span to provide a more "vector" like interface. This replicates the const part of the std::vector API
     */
    template<typename T, size_t Extent = nonstd::dynamic_extent>
    class vector_view {

    public:

        using value_type = std::remove_cv_t<T>;
        using reference = T &;
        using pointer = T *;
        using const_pointer = T const *;
        using const_reference = T const &;

        template<size_t N>
        using stride = gnt::stride<T, N>;

        template<size_t N>
        using const_stride = const gnt::stride<T, N>;

        using size_type = std::size_t;
        using index_type = std::size_t;
        using difference_type = std::size_t;

        using iterator = pointer;
        using const_iterator = const_pointer;
        template<size_t Stride>
        using stride_iterator = gnt::stride_iterator<T, Stride>;
        template<size_t Stride>
        using const_stride_iterator = gnt::stride_iterator<const T, Stride>;

        using reverse_iterator = std::reverse_iterator<pointer>;
        using const_reverse_iterator = std::reverse_iterator<const_pointer>;
        template<size_t Stride>
        using reverse_stride_iterator = std::reverse_iterator<stride_iterator<Stride>>;
        template<size_t Stride>
        using reverse_const_stride_iterator = std::reverse_iterator<const_stride_iterator<Stride>>;


        vector_view() : _impl() {}

        template<size_t Extent2 = Extent>
        vector_view(nonstd::span<T, Extent2>
                    &&spn) : _impl(std::move(spn)) {}

        template<size_t Extent2>
        vector_view(const nonstd::span<T, Extent2> &spn) : _impl(spn) {}

        vector_view(T *const ptr, std::size_t size_bytes) : _impl(ptr, size_bytes) {}

        template<template<typename, typename> typename Container, typename Alloc>
        vector_view(const Container<T, Alloc> &container) : _impl(container.data(), container.size()) {}

        template<size_t N>
        vector_view(std::array<T, N> &spn) : vector_view(spn.data(), spn.size()) {}

        template<size_t Extent2 = Extent>
        void reset(nonstd::span<T, Extent2> &&spn) {
            _impl = std::move(spn);
        }

        template<size_t Extent2 = Extent>
        void reset(const nonstd::span<T, Extent2> &spn) {
            _impl = spn;
        }

        void reset(T *ptr, std::size_t size) {
            _impl = nonstd::span<T>(ptr, size);
        }

        template<size_t N>
        void reset(std::array<T, N> &arr) {
            reset(arr.data(), arr.size());
        }

        template<template<typename, typename> typename Container, typename Alloc>
        void reset(Container<T, Alloc> &container) {
            reset(container.data(), container.size());
        }

    public:

        // ELEMENT ACCESS

        constexpr reference at(size_type idx) {
            return *(_impl.begin() + idx);
        }

        constexpr const_reference at(size_type idx) const {
            return *(_impl.cbegin() + idx);
        }

        template<size_t N>
        constexpr stride<N> at_stride(size_type idx) {
            return stride<N>(_impl.begin() + (N * idx), N);
        }

        template<size_t N>
        constexpr const_stride<N> at_stride(size_type idx) const {
            return const_stride<N>(_impl.begin() + (N * idx), N);
        }

        //Fixme Operator[] skipped

        constexpr reference front() {
            return *(_impl.begin());
        }

        constexpr const_reference front() const {
            return *(_impl.cbegin());
        }

        //Fixme: front_stride

        // Back returns a reference ot the first byte of the last step
        constexpr reference back() {
            return at(size() - 1);
        }

        // Back returns a reference ot the first byte of the last step
        constexpr const_reference back() const {
            return at(size() - 1);
        }

        //Fixme: back_stride

        constexpr T *data() noexcept {
            return _impl.data();
        }

        constexpr const T *data() const noexcept {
            _impl.begin();
            return _impl.data();
        }

        // ITERATORS

        constexpr iterator begin() noexcept {
            return _impl.begin();
        }

        constexpr const_iterator begin() const noexcept {
            return _impl.begin();
        }

        constexpr const_iterator cbegin() const noexcept {
            return _impl.cbegin();
        }

        template<size_t N>
        constexpr stride_iterator<N> begin_stride() noexcept {
            return stride_iterator<N>(begin());
        }

        template<size_t N>
        constexpr const_stride_iterator<N> begin_stride() const noexcept {
            return stride_iterator<N>(begin());
        }

        template<size_t N>
        constexpr const_stride_iterator<N> cbegin_stride() const noexcept {
            return stride_iterator<N>(cbegin());
        }

        constexpr iterator end() noexcept {
            return _impl.end();
        }

        constexpr const_iterator end() const noexcept {
            return _impl.end();
        }

        constexpr const_iterator cend() const noexcept {
            return _impl.cend();
        }

        template<size_t N>
        constexpr stride_iterator<N> end_stride() noexcept {
            return stride_iterator<N>(end());
        }

        template<size_t N>
        constexpr const_stride_iterator<N> end_stride() const noexcept {
            return stride_iterator<N>(end());
        }

        template<size_t N>
        constexpr const_stride_iterator<N> cend_stride() const noexcept {
            return stride_iterator<N>(end());
        }

        constexpr reverse_iterator rbegin() noexcept {
            return _impl.rbegin();
        }

        constexpr const_reverse_iterator rbegin() const noexcept {
            return _impl.rbegin();
        }

        constexpr const_reverse_iterator crbegin() const noexcept {
            return _impl.crbegin();
        }

        //Fixme: rstride_begin()

        constexpr reverse_iterator rend() noexcept {
            return _impl.rend();
        }

        constexpr const_reverse_iterator rend() const noexcept {
            return _impl.rend();
        }

        constexpr const_reverse_iterator crend() const noexcept {
            return _impl.crend();
        }


        //Fixme: rstride_end()

        // CAPACITY

        [[nodiscard]] constexpr bool empty() const noexcept {
            return _impl.empty();
        }

        constexpr size_type size() const noexcept {
            return _impl.size();
        }

        // Return the 'size in steps' for the container.
        template<size_t N>
        constexpr size_type strides() const noexcept {
            return size() / N;
        }

    private:

        nonstd::span<T, Extent> _impl;
    };

} // ns gnt