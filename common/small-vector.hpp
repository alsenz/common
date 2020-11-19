#pragma once

#include <array>
#include <variant>
#include <vector>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <cmath>
#include <functional>
#include <iterator>

//TODO enable the comparison features! TODO TODO
#include "nonstd/span.hpp"

#include "variant-match.hpp"
#include "vector-view.hpp"
#include "stack-vector.hpp"
#include "pack.hpp"
#include "displace.hpp"

//Fixme: we shoud split small_vector and small_byte_map out into separate files.

// A collection of small vectors, maps and views useful for reading and writing slices


namespace gnt {

    //Fixme: performance tune StackExtent default value.

    //
    template<typename T, size_t StackExtent = (128 / sizeof(T)) + 1> // Random guess
    class small_vector : public vector_view<T> {

    public:

        using typename vector_view<T>::size_type;
        using typename vector_view<T>::iterator;
        using typename vector_view<T>::const_iterator;
        using typename vector_view<T>::reference;
        using typename vector_view<T>::value_type;

        using heap_vector = std::vector<T>;
        using stack_vector = gnt::stack_vector<T, StackExtent>;
        using vector_variant = std::variant<stack_vector, heap_vector>;

        //Warning: notice the change of API - reserve not fill!
        small_vector(size_type reserve_cap) : _impl(make_impl_with_reserve(reserve_cap)) {
            //TODO try to put this back to sync()
            std::visit([this](auto &vec) { vector_view<T>::reset(vec.data(), vec.size()); }, _impl);
        }

        small_vector() : _impl(stack_vector()) {
            //TODO check if this is enough...
            std::visit([this](auto &vec) { vector_view<T>::reset(vec.data(), vec.size()); }, _impl);
        }

    private:

        static constexpr vector_variant make_impl_with_reserve(size_type reserve_cap) {
            if(reserve_cap > StackExtent) {
                auto hv = heap_vector();
                hv.reserve(reserve_cap);
                return hv;
            }
            return stack_vector();
        }

        // Ensures the view base class has exactly what the variant has below
        void sync() {
            std::visit([this](auto &vec) { this->reset(vec.data(), vec.size()); }, _impl);
        }

        template<typename Visitor>
        auto visit(Visitor &&vis) -> decltype(std::visit(Visitor(), vector_variant())) {
            return std::visit(std::forward<Visitor>(vis), _impl);
        }

    public:

        //TODO operator=

        // CAPACITY

        constexpr size_type max_size() const noexcept {
            //Fixme: consider this API a bit...
            return visit([](const auto &vec){ return vec.max_size(); });
        }

        template<size_t N>
        constexpr void reserve_strides(size_type num_strides) {
            reserve(num_strides * N);
        }

        constexpr void reserve(size_type new_cap_bytes) {
            // Guaranteed by convention, since will never be heap if < StackExtent
            if(new_cap_bytes < StackExtent) {
                return;
            }
            // Must be exceeding stack extent, need to switch to heap.
            if (std::holds_alternative<stack_vector>(_impl)) {
                heap_vector next;
                next.reserve(new_cap_bytes);
                next.resize(this->size());
                std::copy(this->begin(), this->end(), next.begin());
                _impl = std::move(next);
            } else {
                // Reserve on heap
                std::get<heap_vector>(_impl).reserve(new_cap_bytes);
            }
            sync();
        }

        constexpr size_type capacity() const noexcept {
            return visit([](const auto &vec){ return vec.capacity(); });
        }

        // This only has an effect if in vector mode
        constexpr void shrink_to_fit() {
            if (std::holds_alternative<heap_vector>(_impl)) {
                std::get<heap_vector>(_impl).shrink_to_fit();
            }
        }

        //MODIFIERS

        constexpr void clear() noexcept {
            visit([](const auto &vec){ vec.clear(); });
        }

        //TODO work out what's gone wrong with value type...
        // Element type is a single T if Stride = 0, otherwise a span of Ts
        constexpr iterator insert(const_iterator pos, const value_type &value) {
            const auto diff = pos - this->begin();
            reserve(this->size() + 1);
            pos = this->begin() + diff;
            return visit([pos, &value](const auto &vec){ return vec.insert(pos, value); });
        }

        constexpr iterator insert(const_iterator pos, T &&value) {
            const auto diff = pos - this->begin();
            reserve(this->size() + 1);
            pos = this->begin() + diff;
            return visit([pos, &value](const auto &vec){ return vec.insert(pos, std::move(value)); });
        }

        //Fixme: skipped: constexpr iterator insert( const_iterator pos, size_type count,
        //                           const T& value );

        template<class InputIt>
        constexpr iterator insert(const_iterator pos, InputIt first, InputIt last) {
            const auto diff = pos - this->begin();
            reserve(this->size() + 1);
            pos = this->begin() + diff;
            return visit([pos, first, last](const auto &vec){ return vec.insert(pos, first, last); });
        }

        template<size_t N>
        constexpr stride_iterator<T, N> insert_stride(const stride_iterator<T, N> pos, nonstd::span<T, N> stride) {
            return stride_iterator<T, N>(insert(pos.begin(), stride.begin(), stride.end()));
        }

        //Fixme: template<size_t N>
        //Fixme: constexpr stride_iterator<T, N> insert_strides(const stride_iterator<T, N> pos, stride_iterator<T, N> first, stride_iterator<T, N> last)

        //Fixme: skipped: constexpr iterator insert( const_iterator pos, std::initializer_list<T> ilist );

        template<class... Args>
        constexpr iterator emplace(const_iterator pos, Args &&... args) {
            //Fixme be more clever about this and move the args in with the capture
            return insert(pos, T(std::forward<Args>(args)...));
        }

        constexpr iterator erase(iterator pos) {
            return visit([pos](const auto &vec){ return vec.erase(pos); });
        }

        constexpr iterator erase(const_iterator first, const_iterator last) {
            return visit([first, last](const auto &vec){ return vec.erase(first, last); });
        }

        template<size_t Extent>
        constexpr stride_iterator<T, Extent> erase_stride(stride_iterator<T, Extent> pos) {
            return stride_iterator<T, Extent>(erase(pos.cbegin(), pos.cend()));
        }

        constexpr void push_back(const T &value) {
            reserve(this->size() + 1);
            visit([&value](const auto &vec){ vec.push_back(value); });
        }

        constexpr void push_back(T &&value) {
            reserve(this->size() + 1);
            visit([&value](const auto &vec){ vec.push_back(std::move(value)); });
        }

        template<size_t Extent>
        constexpr void push_back_stride(const stride<T, Extent> &value) {
            reserve(this->size() + Extent);
            visit([&value](const auto &vec){ vec.insert(vec.end(), value.begin(), value.end()); });
        }

        template<class... Args>
        constexpr iterator emplace_back(Args &&... args) {
            return emplace(this->end(), std::move(args)...);
        }

        constexpr void pop_back() {
            erase(this->end()-1);
        }

        constexpr void resize(size_type count) {
            reserve(count);
            visit([&count](const auto &vec){ vec.resize(count); });
        }

        //Fixme: skipped: constexpr void resize( size_type count, const value_type& value );

        //Fixme: skipped: constexpr void swap( vector& other ) noexcept

        //Fixme: skipped: comparators


    private:

        vector_variant _impl;

    };



} //ns gnt
