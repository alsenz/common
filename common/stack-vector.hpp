#pragma once

#include <array>

#include "vector-view.hpp"
#include "stride.hpp"

namespace gnt {

    /**
     * A vector of up to Extent size consisting of an array and a count.
     * Usually used with small_vector for small stack optimisations.
     *
     * @tparam T
     * @tparam Extent Size of the underlying array. By default, at least one or enough
     * ... to store 128 bytes, or 32 uint64_t's.
     */
    template<typename T, size_t Extent = (128 / sizeof(T)) + 1>
    class stack_vector : vector_view<T> {

    public:

        using typename vector_view<T>::size_type;
        using typename vector_view<T>::iterator;
        using typename vector_view<T>::const_iterator;
        using typename vector_view<T>::reference;
        using typename vector_view<T>::element_type;
        using typename vector_view<T>::value_type;

        stack_vector() : _impl(), _count(0) {}

        //Fixme operator=

        //Fixme comparators

        constexpr size_type max_size() const noexcept {
            return Extent;
        }

        constexpr size_type capacity() const noexcept {
            return Extent;
        }

        constexpr void shrink_to_fit() {
            // no-op
        }

        constexpr void clear() noexcept {
            _count = 0;
        }

        //Fixme: for all the inserts, investigate what would usually throw
        //Fixme:... if for example we were out of range (/ array space)

        /**
         *
         * Inserts a value at pos, assuming that end()-begin() <= Extent - 1
         *
         * @param pos Must be a *valid* pos iterator
         * @param value
         * @return
         */
        constexpr iterator insert(const_iterator pos, const value_type &value) {
            displace_right_n(pos, this->end(), 1);
            *pos = value;
            ++_count;
            return pos;
        }

        /**
         *
         * Inserts a value at pos, assuming that end()-begin() != Extent
         *
         * @param pos Must be a *valid* pos iterator
         * @param value
         * @return
         */
        constexpr iterator insert(const_iterator pos, value_type &&value) {
            displace_right_n(pos, this->end(), 1);
            *pos = std::move(value);
            ++_count;
            return pos;
        }

        //Fixme: skipped: constexpr iterator insert( const_iterator pos, size_type count,
        //                           const T& value );

        /**
         *
         * Inserts a range at pos, assuming that end()-begin() <= Extent - (last-first)
         *
         * @param pos Must be a *valid* pos iterator
         * @param value
         * @return
         */
        template<class InputIt>
        constexpr iterator insert(const_iterator pos, InputIt first, InputIt last) {
            constexpr auto range_size = last-first;
            displace_right_n(pos, this->end(), range_size);
            std::copy(first, last, pos);
            _count += range_size;
            return pos;
        }

        template<size_t N>
        constexpr stride_iterator<T, N> insert_stride(const stride_iterator<T, N> pos, gnt::stride<T, N> stride) {
            return stride_iterator<T, N>(insert(pos.begin(), stride.begin(), stride.end()));
        }

        //Fixme: template<size_t N>
        //Fixme: constexpr stride_iterator<T, N> insert_strides(const stride_iterator<T, N> pos, stride_iterator<T, N> first, stride_iterator<T, N> last)

        //Fixme: skipped: constexpr iterator insert( const_iterator pos, std::initializer_list<T> ilist );

        //TODO pick up with emplace etc.
        template<class... Args>
        constexpr iterator emplace(const_iterator pos, Args &&... args) {
            displace_right_n(pos, this->end(), 1);
            *pos = T(std::move(args)...);
            ++_count;
            return pos;
        }

        constexpr iterator erase(iterator pos) {
            displace_left_n(pos, this->end(), 1);
            --_count;
            return pos;
        }

        constexpr iterator erase(const_iterator first, const_iterator last) {
            constexpr auto diff = last-first;
            displace_left_n(last, this->end(), diff);
            _count -= diff;
            return first;
        }

        /**
         * Erase [first, last) where the infinum of last is the first byte of the last stride.
         * @tparam N
         * @param first
         * @param last
         * @return
         */
        template<size_t N>
        constexpr stride_iterator<T, N> erase_stride(const stride_iterator<T, N> first, const stride_iterator<T, N> last) {
            return stride_iterator<T, N>(erase(first.begin(), last.begin()));
        }

        constexpr void push_back(const T &value) {
            *(this->end()) = value;
            ++_count;
        }

        constexpr void push_back(T &&value) {
            *(this->end()) = std::move(value);
            ++_count;
        }

        template<class... Args>
        constexpr iterator emplace_back(Args &&... args) {
            auto it = this->end();
            *it = T(std::move(args)...);
            ++_count;
            return it;
        }

        constexpr void pop_back() {
            erase(this->end()-1);
        }

        constexpr void resize(size_type count) {
            if(count <= this->size()) {
                return;
            }
            constexpr const auto diff = this->size() - count;
            std::fill(this->end() - diff, this->end(), T());
            _count += count;
        }

        //Fixme: skipped: constexpr void resize( size_type count, const value_type& value );

        //Fixme: skipped: constexpr void swap( vector& other ) noexcept

        //Fixme: skipped: comparators







        //TODO copy various bits from the other class... this makes more sense this way....

    private:

        std::array<T, Extent> _impl;
        size_type _count;


    };




} //ns gnt