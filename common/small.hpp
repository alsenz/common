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
#include "pack.hpp"
#include "displace.hpp"

//TODO: step -> stride
//TODO: push the Stride back into a call to Stride with a constexpr...
//TODO this way the base iterator never needs to know... so we'd have size() -> byte size... begin end pointer...
//TODO begin_stride<5>() end_stride<5>() ... should NOT have dereference since we can't *to a span although maybe ->
//TODO then map should just know... i.e. key_stride, KeyStride, ValueStride...
//TODO insert needs a stride... erase should be able to take a stride iterator.... that de-refs to a begin and an end...
//TODO then we have at_stride<5>(idx), etc. etc. should all be sensible the stride iterator is key here...

//Fixme: we shoud split small_range and small_byte_map out into separate files.

// A collection of small vectors, maps and views useful for reading and writing slices


//TODO fix copy wherever possible!
//TODO TODO



namespace common {

    //Fixme: performance tune SizeofInitialExtent default value.

    //
    template<typename T, size_t SizeofInitialExtent = 8 * 4 * 4> // Random guess
    class small_range : public small_range_view<T> {

    public:

        using typename small_range_view<T>::size_type;
        using typename small_range_view<T>::iterator;
        using typename small_range_view<T>::const_iterator;
        using typename small_range_view<T>::reference;
        using typename small_range_view<T>::element_type;

        // Contains the array buffer and the size in use
        using stack_buffer_t = std::array<T, SizeofInitialExtent / sizeof(T)>;
        using size_stack_buffer_pair = std::pair<stack_buffer_t, size_type>;
        using heap_vector_t = std::vector<T>;

        //Warning: notice the change of API - reserve not fill!
        //Fixme: make this a bit cheaper - don't allocate the array variant at all if the vector can hold it!
        small_range(std::size_t reserve_cap) : _impl(std::make_pair(stack_buffer_t(), 0)) {
            reserve(reserve_cap);
        }

        small_range() : _impl(std::make_pair(stack_buffer_t(), 0)) {
            sync_view();
        }

    private:

        // Ensures the view base class has exactly what the variant has below
        void sync_view() {
            std::visit(std::match{
                [this](heap_vector_t &vec) {
                    this->reset(vec);
                },
                [this](size_stack_buffer_pair &pair) {
                    this->reset(pair.first.data(), pair.second);
                }
            }, _impl);
        }

    public:

        //TODO operator=

        // ELEMENT ACCESS - EXTENDED

        // CAPACITY

        constexpr size_type max_size() const noexcept {
            if (std::holds_alternative<size_stack_buffer_pair>(_impl)) {
                return std::get<size_stack_buffer_pair>(_impl).first.max_size();
            } else {
                return std::get<heap_vector_t>(_impl).max_size();
            }
        }

        template<size_t N>
        constexpr void reserve_strides(size_type num_strides) {
            reserve(num_strides * N);
        }

        constexpr void reserve(size_type new_cap_bytes) {
            if (std::holds_alternative<size_stack_buffer_pair>(_impl)) {
                auto &pair = std::get<size_stack_buffer_pair>(_impl);
                if (pair.first.size() >= new_cap_bytes) {
                    return; // We're good.
                }
                // New capacity must be greater than fixed size- updgrade to heap
                auto next_impl = heap_vector_t(new_cap_bytes); // Fill
                //Fixme: potentially use move
                std::copy(pair.first.begin(), pair.first.end(), next_impl.begin());
                _impl = std::move(next_impl);
                sync_view();
                return;
            }
            // Reserve on heap
            std::get<heap_vector_t>(_impl).reserve(new_cap_bytes);
            sync_view();
        }

        constexpr size_type capacity() const noexcept {
            if (std::holds_alternative<size_stack_buffer_pair>(_impl)) {
                // Capacity interpreted as the space currently pre-allocated.
                return std::get<size_stack_buffer_pair>(_impl).first.size();
            } else {
                return std::get<size_stack_buffer_pair>(_impl).capacity();
            }
        }

        // This only has an effect if in vector mode
        constexpr void shrink_to_fit() {
            if (std::holds_alternative<heap_vector_t>(_impl)) {
                std::get<heap_vector_t>(_impl).shrink_to_fit();
            }
        }

        //MODIFIERS

        constexpr void clear() noexcept {
            std::visit(std::match{
                [](size_stack_buffer_pair &p) { p.second = 0; },
                [](heap_vector_t &v) { v.clear(); }
            }, _impl);
        }

        // Note: remember size and reserve work on raw bytews

        // Element type is a single T if Stride = 0, otherwise a span of Ts
        constexpr iterator insert(const_iterator pos, const typename small_range_view<T>::value_type &value) {
            const auto diff = pos - this->begin();
            reserve(this->size() + 1);
            return std::visit(std::match{
                [&value, diff, this](size_stack_buffer_pair &pair) {
                    std::copy(pair.first.begin() + diff, pair.first.begin() + pair.second, pair.first.begin() + diff + 1);
                    pair.second++;
                    pair.first.at(diff) = value;
                    return this->begin() + diff;
                },
                [&value, diff, this](heap_vector_t &v) {
                    return _impl.insert(_impl.begin() + diff, value);
                }
            }, _impl);
        }

        constexpr iterator insert(const_iterator pos, T &&value) {
            const auto diff = pos - this->begin();
            reserve(this->size() + 1);
            return std::visit(std::match{
                [&value, diff, this](size_stack_buffer_pair &pair) {
                    std::copy(pair.first.begin() + diff, pair.first.begin() + pair.second, pair.first.begin() + diff + 1);
                    pair.second++;
                    pair.first.at(diff) = std::move(value);
                    return this->begin() + diff;
                },
                [&value, diff, this](heap_vector_t &v) {
                    return _impl.insert(_impl.begin() + diff, std::move(value));
                }
            }, _impl);
        }

        //Fixme: skipped: constexpr iterator insert( const_iterator pos, size_type count,
        //                           const T& value );

        template<class InputIt>
        constexpr iterator insert(const_iterator pos, InputIt first, InputIt last) {
            const auto diff = pos - this->begin();
            reserve(this->size() + diff);
            return std::visit(std::match{
                [&first, &last, diff, this](size_stack_buffer_pair &pair) {
                    const auto dist = last - first;
                    std::copy(pair.first.begin() + diff, pair.first.begin() + pair.second, pair.first.begin() + diff + dist);
                    pair.second += dist;
                    std::copy(first, last, pair.first.begin() + diff);
                    return this->begin() + diff;
                },
                [&first, &last, diff, this](heap_vector_t &v) {
                    return _impl.insert(_impl.begin() + diff, first, last);
                }
            }, _impl);
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
            size_type diff = pos - this->begin(); //We must convert pos to an index since reserve could invalidate it
            reserve(this->size() + 1);
            //Fixme be more clever about this and move the args in with the capture
            auto value = T(std::move(args...));
            return std::visit(std::match{
                [diff, &value, this](size_stack_buffer_pair &pair) {
                    std::copy(pair.first.data() + diff,
                              pair.first.data() + pair.second,
                              pair.first.data() + diff + 1); // Move the data to the right.
                    pair.first.at(diff) = std::move(value);
                    pair.second++;
                    return this->begin() + diff;
                },
                [diff, &value](heap_vector_t &v) {
                    return v.insert(v.begin() + diff, std::move(value));
                }
            }, _impl);
        }

        //TODO - fix this, then do the range-based erase, then do the stride erase, which should be based on the range based.

        constexpr iterator erase(iterator pos) {
            return std::visit(std::match{
                [pos](size_stack_buffer_pair &pair) {
                    //TODO check this works... may need to be copy backward...
                    std::copy(pos + 1, pair.first.data() + pair.second, pos);
                    pair.second--;
                },
                [pos](heap_vector_t &v) {
                    return iterator(v.erase(pos));
                }
            }, _impl);
        }

        //TODO do this
        //TODO TODO
        //Fixme skipped: constexpr iterator erase(const_iterator first, const_iterator last)

        //TODO erase_stride! stride_iterator pos...

        constexpr void push_back(const T &value) {
            //Fixme: write more specificly optimised version
            insert(this->end(), value);
        }

        constexpr void push_back(T &&value) {
            //Fixme: write more specificly optimised version
            insert(this->end(), std::move(value));
        }

        template<class... Args>
        constexpr iterator emplace_back(Args &&... args) {
            return emplace(this->end(), std::move(args)...);
        }

        constexpr void pop_back() {
            erase(this->end()-1);
        }

        constexpr void resize(size_type count) {
            std::visit(std::match{
                [count](size_stack_buffer_pair &pair) {
                    // Default initialise the values
                    std::fill(pair.first.begin() + pair.second, pair.first.end(), T());
                    pair.second = count;
                },
                [count](heap_vector_t &v) {
                    v.resize(count);
                }
            }, _impl);
        }
        //Fixme: skipped: constexpr void resize( size_type count, const value_type& value );

        //Fixme: skipped: constexpr void swap( vector& other ) noexcept

        //Fixme: skipped: comparators


    private:

        std::variant<size_stack_buffer_pair, heap_vector_t> _impl;

    };

    //Note neither K_Stride nor V_Stride may be 0.
    template<size_t K_Stride, size_t V_Stride, typename byte_type = std::byte>
    struct small_byte_map_iterator {

        using value_type = std::pair<nonstd::span<byte_type, K_Stride>, nonstd::span<byte_type, V_Stride>>;

        small_byte_map_iterator() : impl(nonstd::span<byte_type, K_Stride>((byte_type *)nullptr, (size_t) 0), nonstd::span<byte_type, V_Stride>((byte_type *)nullptr, (size_t) 0)) {
            static_assert(K_Stride > 0 , "In small_byte_map, K_Stride must be > 0");
            static_assert(V_Stride > 0 , "In small_byte_map, V_Stride must be > 0");
        }

        small_byte_map_iterator(const nonstd::span<byte_type, K_Stride> &key_span,
                                const nonstd::span<byte_type, V_Stride> &value_span) : impl(key_span, value_span){}

        small_byte_map_iterator(nonstd::span<byte_type, K_Stride> &&key_span,
                                nonstd::span<byte_type, V_Stride> &&value_span)
                                : impl(std::move(key_span), std::move(value_span)) {}

        value_type &operator* () {
            return impl;
        }

        value_type *operator-> () {
            return &impl;
        }

        small_byte_map_iterator<K_Stride, V_Stride> &operator++() {
            impl.first = nonstd::span<byte_type, K_Stride>(impl.first.data() + K_Stride, K_Stride);
            impl.second = nonstd::span<byte_type, V_Stride>(impl.second.data() + V_Stride, V_Stride);
            return *this;
        }

        small_byte_map_iterator<K_Stride, V_Stride> operator++(int) {
            return small_byte_map_iterator<K_Stride, V_Stride>(
                nonstd::span<byte_type, K_Stride>(impl.first.data() + K_Stride, K_Stride),
                nonstd::span<byte_type, V_Stride>(impl.second.data() + V_Stride, V_Stride)
            );
        }

        //Fixme: this should be difference type
        small_byte_map_iterator operator+ (const std::size_t n) const {
            return small_byte_map_iterator<K_Stride, V_Stride>(
                nonstd::span<byte_type, K_Stride>(impl.first.data() + n * K_Stride, K_Stride),
                nonstd::span<byte_type, V_Stride>(impl.second.data() + n * V_Stride, V_Stride)
            );
        }

        //Fixme: this should be difference type
        small_byte_map_iterator operator- (const std::size_t n) const {
            return small_byte_map_iterator<K_Stride, V_Stride>(
                nonstd::span<byte_type, K_Stride>(impl.first.data() - n * K_Stride, K_Stride),
                nonstd::span<byte_type, V_Stride>(impl.second.data() - n * V_Stride, V_Stride)
            );
        }

        // Comparison behaviour is the same as comparison of the key pointer, with no extent (so end can have 0 extent)
        bool operator ==(const small_byte_map_iterator<K_Stride, V_Stride> &b) const {
            return impl.first.data() == b.impl.first.data();
        }

        bool operator !=(const small_byte_map_iterator<K_Stride, V_Stride> &b) const {
            return impl.first.data() != b.impl.first.data();
        }

        bool operator >(const small_byte_map_iterator<K_Stride, V_Stride> &b) const {
            return impl.first.data() > b.impl.first.data();
        }

        bool operator <(const small_byte_map_iterator<K_Stride, V_Stride> &b) const {
            return impl.first.data() < b.impl.first.data();
        }

        bool operator >=(const small_byte_map_iterator<K_Stride, V_Stride> &b) const {
            return impl.first.data() >= b.impl.first.data();
        }

        bool operator <=(const small_byte_map_iterator<K_Stride, V_Stride> &b) const {
            return impl.first.data() <= b.impl.first.data();
        }

        value_type impl;

    };


    // A small multimap backed by contiguous byte storage. Great for operating on small packed + serialised structures.
    template<size_t K_Stride = 1, size_t V_Stride = 1, typename byte_type = std::byte, size_t InitialExtent = 128>
    class small_byte_map_view {

    public:

        using iterator = small_byte_map_iterator<K_Stride, V_Stride, byte_type>;
        using const_iterator = small_byte_map_iterator<K_Stride, V_Stride, const byte_type>;
        using size_type = std::size_t;
        using key_view_type = nonstd::span<byte_type, K_Stride>; //Individual key view
        using value_view_type = nonstd::span<byte_type, V_Stride>; //Individual value view

        using key_range_view = small_range_view<byte_type, K_Stride>;
        using value_range_view = small_range_view<byte_type, V_Stride>;

        constexpr const static std::size_t initial_extent = InitialExtent;

        // Helper function to build a view over a single contiguous underlying byte_type array
        static auto build_from_contiguous_bytes(byte_type *data, size_type size_bytes, bool sorted = false) {
            if ((size_bytes % (K_Stride + V_Stride)) != 0) {
                throw std::range_error("small_byte_map_view cannot be built from contiguous bytes that are no divisible by K_Stride + V_step");
            }
            const size_type num_elems = size_bytes / (K_Stride + V_Stride);
            return small_byte_map_view<K_Stride, V_Stride, byte_type, InitialExtent>(data,
                data + (K_Stride * num_elems), size_bytes, sorted);
        }

        template<size_type Extent>
        static auto build_from_contiguous_bytes(nonstd::span<byte_type, Extent> spn, bool sorted = false) {
            return build_from_contiguous_bytes(spn.data(), spn.size(), sorted);
        }

        template<template<typename, typename> typename C, typename Alloc>
        static auto build_from_contiguous_bytes(C<byte_type, Alloc> &cont, bool sorted = false) {
            return build_from_contiguous_bytes(cont.data(), cont.size(), sorted);
        }

        small_byte_map_view(bool sorted = false) : _keys(), _values(), _is_sorted(sorted) {}

        small_byte_map_view(byte_type *k_ptr, byte_type *v_ptr, size_type size_bytes, bool sorted = false)
            : _keys(k_ptr, size_bytes), _values(v_ptr, size_bytes), _is_sorted(sorted) {}

        template<size_type Extent>
        small_byte_map_view(nonstd::span<byte_type, Extent> key_spn, nonstd::span<byte_type, Extent> value_spn, bool sorted = false)
            : _keys(key_spn), _values(value_spn), _is_sorted(sorted) {}

        template<template <typename, typename> typename C, typename Alloc>
        small_byte_map_view(C<byte_type, Alloc> &key_cont, C<byte_type, Alloc> &value_cont, bool sorted = false)
            : small_byte_map_view(key_cont.data(), value_cont.data(), key_cont.size(), sorted) {}

    protected:

        template<size_t Extent>
        void reset(nonstd::span<byte_type, Extent> key_spn, nonstd::span<byte_type, Extent> value_spn) {
            _keys = key_spn;
            _values = value_spn;
        }

        //Fixme: size checking.
        void reset(byte_type *k_ptr, byte_type *v_ptr, size_type k_size_bytes, size_type v_size_bytes) {
            _keys = nonstd::span<byte_type>(k_ptr, k_size_bytes);
            _values = nonstd::span<byte_type>(v_ptr, v_size_bytes);
        }

        template<template <typename, typename> typename C, typename Alloc>
        void reset(C<byte_type, Alloc> &key_cont, C<byte_type, Alloc> &value_cont) {
            // Fixme (here and elsewhere) more size checking on these pointer ariths?
            reset(key_cont.data(), value_cont.data(), key_cont.size());
        }

        template<size_t Extent, size_t K_Stride2, size_t V_Stride2>
        void reset(small_range<byte_type, Extent, K_Stride2> &k_range, small_range<byte_type, Extent, V_Stride2> &v_range) {
            reset(k_range.data(), v_range.data(), k_range.size(), v_range.size());
        }


        // Has the usual find semantics - i.e. if not found will return end()
        constexpr typename key_range_view::iterator find_key(key_view_type needle) noexcept {
            if (_is_sorted) {
                auto it = std::lower_bound(_keys.begin(), _keys.end(), needle);
                if(it != _keys.end() && it.to_span() == needle) { //Fixme: span cast so that 0 Stride will work
                    return it;
                }
                return _keys.end();
            } else {
                // Linear search through a buffer on the stack.
                // Searching through an array linearly - O(N)
                // 16 CPU cores -> 32 virtual CPU cores -> 2x2 4 cpu level parellism
                // Seraching binary wise - O(log(N))
                return std::find(_keys.begin(), _keys.end(), needle);
            }
        }

        // Has the usual find semantics - i.e. if not found will return end()
        constexpr typename key_range_view::const_iterator find_key(key_view_type needle) const noexcept {
            if (_is_sorted) {
                auto it = std::lower_bound(_keys.begin(), _keys.end(), needle);
                if(it != _keys.end() && it.to_span() == needle) { //Fixme: span cast so that 0 Stride will work
                    return it;
                }
                return _keys.end();
            } else {
                return std::find(_keys.begin(), _keys.end(), needle);
            }
        }

        // Call this only once the underlying range has been sorted due to expansions
        void sorted(bool is_sorted = true) {
            _is_sorted = is_sorted;
        }

        bool is_sorted() const  {
            return _is_sorted;
        }

    public:

        iterator begin() noexcept {
            if(_keys.empty()) {
                return iterator();
            }
            // Note this always assumes that the key and value iterators have steps.
            // Fixme: ensure iterator in no-step mode will construct from a byte pointer
            return iterator(
                _keys.begin(),
                _values.begin()
            );
        }

        const_iterator begin() const noexcept {
            if(_keys.empty()) {
                return const_iterator();
            }
            // Note this always assumes that the key and value iterators have steps.
            // Fixme: ensure iterator in no-step mode will construct from a byte pointer
            return const_iterator(
                _keys.begin(),
                _values.begin()
            );
        }

        const_iterator cbegin() const noexcept {
            if(_keys.empty()) {
                return const_iterator();
            }
            // Note this always assumes that the key and value iterators have steps.
            // Fixme: ensure iterator in no-step mode will construct from a byte pointer
            return const_iterator(
                _keys.begin(),
                _values.begin()
            );
        }

        // End iterator points to one-off-end as usual
        iterator end() noexcept {
            if(_keys.empty()) {
                return iterator();
            }
            // Note this always assumes that the key and value iterators have steps.
            // Fixme: ensure iterator in no-step mode will construct from a byte pointer
            return iterator(
                _keys.end(),
                _values.end()
            );
        }

        const_iterator end() const noexcept {
            if(_keys.empty()) {
                return const_iterator();
            }
            // Note this always assumes that the key and value iterators have steps.
            // Fixme: ensure iterator in no-step mode will construct from a byte pointer
            return const_iterator(
                _keys.end(),
                _values.end()
            );
        }

        const_iterator cend() const noexcept {
            if(_keys.empty()) {
                return const_iterator();
            }
            // Note this always assumes that the key and value iterators have steps.
            // Fixme: ensure iterator in no-step mode will construct from a byte pointer
            return const_iterator(
                _keys.end(),
                _values.end()
            );
        }

        [[nodiscard]] bool empty() const noexcept {
            return _keys.empty();
        }

        size_type size() const noexcept {
            return _keys.size();
        }

        //Fixme: size_type max_size() const noexcept;

        value_view_type at(key_view_type key) {
            typename key_range_view::iterator pos = find_key(key);
            if(pos == _keys.end()) {
                throw std::out_of_range("key is not in small_byte_map_view");
            }
            auto diff = pos - _keys.begin(); //Which could be 0!
            return _values.begin() + diff;
        }

        const value_view_type &at(key_view_type key) const {
            typename key_range_view::iterator pos = find_key(key);
            if(pos == _keys.end()) {
                throw std::out_of_range("key is not in small_byte_map_view");
            }
            auto diff = pos - _keys.begin(); //Which could be 0!
            return _values.begin() + diff;
        }

        size_type count(const key_view_type &key) const {
            if(is_sorted() && size() > InitialExtent) { // Our heuristic for whether it's worth searching.
                auto lower = std::lower_bound(_keys.begin(), _keys.end(), key);
                auto upper = std::upper_bound(lower, _keys.end(), key);
                return std::count(lower, upper, key);
            } else { // This is faster for small vectors which we leave unsorted
                auto lower = _keys.begin();
                auto upper = _keys.end();
                return std::count(lower, upper, key);
            }
        }

        iterator find(const key_view_type& key) {
            auto pos = find_key(key); //Gives me an iterator that gives me a span here
            if(pos == _keys.end()) {
                return end();
            }
            auto diff = _keys.end() - pos;
            return iterator(*pos, *(_values.begin() + diff));
        }

        const_iterator find(const key_view_type& key) const {
            auto pos = find_key(key); //Gives me an iterator that gives me a span here
            if(pos == _keys.end()) {
                return end();
            }
            auto diff = _keys.end() - pos;
            return const_iterator(*pos, *(_values.begin() + diff));
        }

        bool contains(const key_view_type& key) const {
            return find(key) != end();
        }

        //Fixme: std::pair<iterator,iterator> equal_range( const Key& key );

        //Fixme: std::pair<iterator,iterator> equal_range( const Key& key );

    private:

        key_range_view _keys; //This is sorted above InitialExtent, otherwise use linear search for parallelism.
        value_range_view _values;
        bool _is_sorted = false; // If reserve moves to the heap, we sort the keys (and values) and use binary search

    };

    //TODO in the implementation, no point in having a single underlying buffer.... if we're using the view we can just manually make a nice span..

    //Fixme: allow for steps of 0, which currently is broke where we just return byte for everything.

    // A small map is a map for small maps which *doesn't* break out into a full heap-based std::unordered_map unlike std::small_range does!
    // Instead, it follows the same layout as before just moves it to the heap, so that it can quickly be serialised or deserialised
    template<size_t K_Stride = 1, size_t V_Stride = 1, typename byte_type = std::byte, size_t InitialExtent = 8 * 4 * 4>
    class small_byte_map : public small_byte_map_view<K_Stride, V_Stride, byte_type, InitialExtent> {

    public:

        // Construct with a reserve in *number of elements* (not byte size)!
        small_byte_map(std::size_t reserve_num_entries) : _keys_impl(reserve_num_entries * K_Stride), _values_impl(reserve_num_entries * V_Stride) {
            sync_views();
        }

        small_byte_map() : _keys_impl(), _values_impl() {
            sync_views();
        }

        //Extra APIs

        // Write the key range then the value range to a concatenated vector - useful for serialising
        std::vector<byte_type> to_vector() const {
            std::vector<byte_type> vec;
            to_range(vec);
            return vec;
        }

        template<template<typename, typename> typename C, typename Alloc>
        C<byte_type, Alloc> to_range() const {
            C<byte_type, Alloc> cont;
            to_range(cont);
            return cont;
        }

        // Generic version nice for serialising to e.g. rocks Slices.
        template<template<typename, typename> typename C, typename Alloc>
        void to_range(C<byte_type, Alloc> &target) const {
            target.resize(_keys_impl.size() + _values_impl.size());
            std::copy(_keys_impl.data(), _keys_impl.data() + _keys_impl.size(), target.begin());
            std::copy(_values_impl.data(), _values_impl.data() + _values_impl.size(), target.begin() + _keys_impl.size());
        }

        // map-like APIs

        //TODO TODO

        //TODO we need a reserve that will do a sort if we go above InitialExtent and do a sorted insert...

        //TODO the rest of the API TODO TODO

    protected:

        void sync_views() {
            this->reset(_keys_impl, _values_impl);
        }

    private:

        small_range<byte_type, InitialExtent, K_Stride> _keys_impl; // We create a small vector optimised buffer, with no underlying step.
        small_range<byte_type, InitialExtent, V_Stride> _values_impl;

    };


    template<typename ByteType = std::byte>
    class heterogenous_record_view {

    public:

        using byte_type = ByteType;
        using size_type = uint64_t;

        //Fixme: allow this method to do packs for arbitrary size-types

        //Builds a pack of ranges (spans, range_views, vectors etc.) into a small range, that can be a heterogenous record view.
        template<typename... InputRanges>
        constexpr static small_range<byte_type> build_heterogenous_range(InputRanges... i_ranges) {
            static_assert(std::conjunction_v<std::is_convertible<typename InputRanges::value_type, byte_type>...>,
                "InputRanges must have byte_type value_type's");
            static_assert(sizeof(byte_type) == 1, "byte_type must be sizsof 1 - or it's not a byte type!");
            auto o = small_range_view<byte_type>();
            constexpr auto n_scalars_per_integer = std::max(uint64_t(1), uint64_t(sizeof(uint64_t) / sizeof(byte_type)));
            constexpr auto record_header_size = n_scalars_per_integer * sizeof...(InputRanges) + 1;

            // Reserve size by fold expression that should sum up the sizes of all the ranges
            o.reserve(i_ranges.size() + ... + record_header_size);

            // Add the header
            //TODO TODO
            //TODO check resize exists.
            o.resize(sizeof...(InputRanges) * sizeof(size_type) + sizeof(byte_type));
            unsigned int i = 0;
            gt::pack_int(sizeof...(InputRanges), o.at_stride<sizeof(size_type)>(i++));
            // Fold up the ranges and write their sizes into the output
            (gt::pack_int(i_ranges.size(), o.at_stride<sizeof(size_type)>(i++)), ...);

            // Add the actual data of each of the ranges... (using o.end() this time)
            ((o.insert(o.end(), i_ranges.begin(), i_ranges.end())), ...);
            return o;
        }

        //TODO begin and end need to be super simple iterators too...

        //TODO span at

    };

} //ns gnt


//TODO:
//TODO for keys would be nice to have heterogenous_range_view

//TODO span_tuple<type, Extent1, Extent2, Extent3>

//TODO span_tuple_view<type, Extnet1, Extent2 etc.>
//TODO problem: how can we construct from a range_view - how can we have dynamic extent over a view?
//TODO better to have all the sizes at the beginning... but this has a flaw in that it messes up the ordering on the paths.
//TODO we could have all the sizes at the end...

//TODO of course the comparator makes it fine!!
//TODO so absolutely no need to worry about that... mostly...
//TODO I like the idea of a heterogenous_stride_view with the strides read off the beginning.
//TODO we can then wrap iterators etc that way and underneath it should go to spans... which can then plug into other views...
//TODO absolutely no chance of course of ever having such a thing dynamically but that should be fine