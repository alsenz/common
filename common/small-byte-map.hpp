#pragma once

#include <utility>
#include <cstddef>

#include "small-vector.hpp"
#include "vector-view.hpp"
#include "stride.hpp"

namespace gnt {
    //Note neither K_Extent nor V_Extent may be 0.
    template<size_t K_Extent, size_t V_Extent, typename byte_type = std::byte>
    struct small_byte_map_iterator {

        using key_stride = nonstd::span<byte_type, K_Extent>;
        using value_stride = nonstd::span<byte_type, V_Extent>;

        using value_type = std::pair<key_stride, value_stride>;

        small_byte_map_iterator() : impl(key_stride((byte_type *) nullptr, (size_t) 0),
                                         value_stride((byte_type *) nullptr, (size_t) 0)) {
            static_assert(K_Extent > 0, "In small_byte_map, K_Extent must be > 0");
            static_assert(V_Extent > 0, "In small_byte_map, V_Extent must be > 0");
        }

        small_byte_map_iterator(const key_stride &key_span,
                                const value_stride &value_span) : impl(key_span, value_span) {}

        small_byte_map_iterator(key_stride &&key_span,
                                value_stride &&value_span)
            : impl(std::move(key_span), std::move(value_span)) {}

        value_type &operator*() {
            return impl;
        }

        value_type *operator->() {
            return &impl;
        }

        small_byte_map_iterator<K_Extent, V_Extent> &operator++() {
            impl.first = key_stride(impl.first.data() + K_Extent, K_Extent);
            impl.second = value_stride(impl.second.data() + V_Extent, V_Extent);
            return *this;
        }

        small_byte_map_iterator<K_Extent, V_Extent> operator++(int) {
            return small_byte_map_iterator<K_Extent, V_Extent>(
                key_stride(impl.first.data() + K_Extent, K_Extent),
                value_stride(impl.second.data() + V_Extent, V_Extent)
            );
        }

        //Fixme: this should be difference type
        small_byte_map_iterator operator+(const std::size_t n) const {
            return small_byte_map_iterator<K_Extent, V_Extent>(
                key_stride(impl.first.data() + n * K_Extent, K_Extent),
                value_stride(impl.second.data() + n * V_Extent, V_Extent)
            );
        }

        //Fixme: this should be difference type
        small_byte_map_iterator operator-(const std::size_t n) const {
            return small_byte_map_iterator<K_Extent, V_Extent>(
                key_stride(impl.first.data() - n * K_Extent, K_Extent),
                value_stride(impl.second.data() - n * V_Extent, V_Extent)
            );
        }

        // Comparison behaviour is the same as comparison of the key pointer, with no extent (so end can have 0 extent)
        bool operator==(const small_byte_map_iterator<K_Extent, V_Extent> &b) const {
            return impl.first.data() == b.impl.first.data();
        }

        bool operator!=(const small_byte_map_iterator<K_Extent, V_Extent> &b) const {
            return impl.first.data() != b.impl.first.data();
        }

        bool operator>(const small_byte_map_iterator<K_Extent, V_Extent> &b) const {
            return impl.first.data() > b.impl.first.data();
        }

        bool operator<(const small_byte_map_iterator<K_Extent, V_Extent> &b) const {
            return impl.first.data() < b.impl.first.data();
        }

        bool operator>=(const small_byte_map_iterator<K_Extent, V_Extent> &b) const {
            return impl.first.data() >= b.impl.first.data();
        }

        bool operator<=(const small_byte_map_iterator<K_Extent, V_Extent> &b) const {
            return impl.first.data() <= b.impl.first.data();
        }

        value_type impl;

    };

    //TODO iter_swap

} //ns gnt

namespace std {

    // Specialised to enable sorting maps using these iters.
    template<size_t K_Extent, size_t V_Extent, typename byte_type = std::byte>
    constexpr void iter_swap(gnt::small_byte_map_iterator<K_Extent, V_Extent, byte_type> lhs, gnt::small_byte_map_iterator<K_Extent, V_Extent, byte_type> rhs) {
        std::swap_ranges(lhs->first.begin(), lhs->first.end(), rhs->first.begin());
        std::swap_ranges(lhs->second.begin(), lhs->second.end(), rhs->second.begin());
    }

} //ns std

namespace gnt {

    //TODO don't use ensure_mode - just infer_mode(sorted_hint).

    // A small multimap backed by contiguous byte storage. Great for operating on small packed + serialised structures.
    /**
     *
     * @tparam K_Extent
     * @tparam V_Extent
     * @tparam byte_type
     * @tparam LinearExtent The extent before which linear search is prefered over binary search for small stack optimisations
     */
    template<size_t K_Extent = 1, size_t V_Extent = 1, typename byte_type = std::byte, size_t LinearExtent = 128>
    class small_byte_map_view {

    public:

        using iterator = small_byte_map_iterator<K_Extent, V_Extent, byte_type>;
        using const_iterator = small_byte_map_iterator<K_Extent, V_Extent, const byte_type>;
        using size_type = std::size_t;
        using key_stride = nonstd::span<byte_type, K_Extent>; //Individual key view
        using value_stride = nonstd::span<byte_type, V_Extent>; //Individual value view

        using key_range_view = vector_view<byte_type>;
        using value_range_view = vector_view<byte_type>;

        //The extent after which the key and value vectors are sorted.
        constexpr const static std::size_t linear_extent = LinearExtent;

        // Helper function to build a view over a single contiguous underlying byte_type array
        static auto build_from_contiguous_bytes(byte_type *data, size_type size_bytes, bool sorted_hint = false) {
            if ((size_bytes % (K_Extent + V_Extent)) != 0) {
                throw std::range_error("small_byte_map_view cannot be built from contiguous bytes that are no divisible by K_Extent + V_step");
            }
            const size_type num_elems = size_bytes / (K_Extent + V_Extent);
            return small_byte_map_view<K_Extent, V_Extent, byte_type, LinearExtent>(data,
                                                                                     data + (K_Extent * num_elems), num_elems, sorted_hint);
        }

        template<size_type Extent>
        static auto build_from_contiguous_bytes(nonstd::span<byte_type, Extent> spn, bool sorted_hint = false) {
            return build_from_contiguous_bytes(spn.data(), spn.size(), sorted_hint);
        }

        template<template<typename, typename> typename C, typename Alloc>
        static auto build_from_contiguous_bytes(C<byte_type, Alloc> &cont, bool sorted_hint = false) {
            return build_from_contiguous_bytes(cont.data(), cont.size(), sorted_hint);
        }

        small_byte_map_view(bool linear_mode = true) : _linear_mode(linear_mode), _keys(), _values() {}

        small_byte_map_view(byte_type *k_ptr, byte_type *v_ptr, size_type num_elems, bool sorted_hint = false)
            : _linear_mode(false), _keys(k_ptr, num_elems * K_Extent), _values(v_ptr, num_elems * V_Extent) {
            infer_mode(sorted_hint);
        }

        template<size_type Extent>
        small_byte_map_view(nonstd::span<byte_type, Extent> key_spn, nonstd::span<byte_type, Extent> value_spn, bool sorted_hint = false)
            : _linear_mode(false), _keys(key_spn), _values(value_spn) {
            infer_mode(sorted_hint);
        }

        template<template <typename, typename> typename C, typename Alloc>
        small_byte_map_view(C<byte_type, Alloc> &key_cont, C<byte_type, Alloc> &value_cont, bool sorted_hint = false)
            : small_byte_map_view(key_cont.data(), value_cont.data(), key_cont.size(), sorted_hint) {}

    protected:

        template<size_t Extent>
        void reset(nonstd::span<byte_type, Extent> key_spn, nonstd::span<byte_type, Extent> value_spn) {
            _keys = key_spn;
            _values = value_spn;
        }

        void reset(const key_range_view &krv, const value_range_view &vrv) {
            _keys = krv;
            _values = vrv;
        }

        //Fixme: size checking.
        void reset(byte_type *k_ptr, byte_type *v_ptr, size_type k_size_bytes, size_type v_size_bytes) {
            _keys = key_range_view(k_ptr, k_size_bytes);
            _values = value_range_view(v_ptr, v_size_bytes);
        }

        template<template <typename, typename> typename C, typename Alloc>
        void reset(C<byte_type, Alloc> &key_cont, C<byte_type, Alloc> &value_cont) {
            // Fixme (here and elsewhere) more size checking on these pointer ariths?
            reset(key_cont.data(), value_cont.data(), key_cont.size(), value_cont.size());
        }

        // Has the usual find semantics - i.e. if not found will return end()
        constexpr typename key_range_view::template stride_iterator<K_Extent> find_key(key_stride needle) noexcept {
            if (_linear_mode) {
                return std::find(_keys.template begin_stride<K_Extent>(), _keys.template end_stride<K_Extent>(), needle);
            } else {
                auto it = std::lower_bound(_keys.template begin_stride<K_Extent>(), _keys.template end_stride<K_Extent>(), needle);
                if(it != _keys.template end_stride<K_Extent>() && it.to_span() == needle) { //Fixme: span cast so that 0 Stride will work
                    return it;
                }
                return _keys.template end_stride<K_Extent>();
            }
        }

        // Has the usual find semantics - i.e. if not found will return end()
        constexpr typename key_range_view::template const_stride_iterator<K_Extent> find_key(key_stride needle) const noexcept {
            if (_linear_mode) {
                return std::find(_keys.template begin_stride<K_Extent>(), _keys.template end_stride<K_Extent>(), needle);
            } else {
                auto it = std::lower_bound(_keys.template begin_stride<K_Extent>(), _keys.template end_stride<K_Extent>(), needle);
                if(it != _keys.template end_stride<K_Extent>() && it.to_span() == needle) { //Fixme: span cast so that 0 Stride will work
                    return it;
                }
                return _keys.template end_stride<K_Extent>();
            }
        }

        constexpr typename value_range_view::template stride_iterator<K_Extent> find_value_by_key(key_stride needle) noexcept {
            auto pos = find_key(needle);
            if (pos == _keys.template end_stride<K_Extent>()) {
                return _values.template end_stride<V_Extent>();
            }
            auto diff = pos - _keys.begin(); //Which could be 0!
            return _values.template begin_stride<V_Extent>() + diff;
        }

        constexpr typename value_range_view::template const_stride_iterator<K_Extent> find_value_by_key(key_stride needle) const noexcept {
            auto pos = find_key(needle);
            if (pos == _keys.template end_stride<K_Extent>()) {
                return _values.template end_stride<V_Extent>();
            }
            auto diff = pos - _keys.begin(); //Which could be 0!
            return _values.template begin_stride<V_Extent>() + diff;
        }


        void infer_mode(bool presorted_hint = false) {
            if(size() > LinearExtent && presorted_hint) {
                _linear_mode = false;
            }
        }

        //TODO move this into small_byte_map

    public:

        bool linear_mode() const  {
            return _linear_mode;
        }

        iterator begin() noexcept {
            if(_keys.empty()) {
                return iterator();
            }
            return iterator(
                _keys.template begin_stride<K_Extent>().to_span(),
                _values.template begin_stride<V_Extent>().to_span()
            );
        }

        const_iterator begin() const noexcept {
            if(_keys.empty()) {
                return const_iterator();
            }
            // Note this always assumes that the key and value iterators have steps.
            return const_iterator(
                _keys.template begin_stride<K_Extent>().to_span(),
                _values.template begin_stride<V_Extent>().to_span()
            );
        }

        const_iterator cbegin() const noexcept {
            if(_keys.empty()) {
                return const_iterator();
            }
            // Note this always assumes that the key and value iterators have steps.
            return const_iterator(
                _keys.template cbegin_stride<K_Extent>().to_span(),
                _values.template cbegin_stride<V_Extent>().to_span()
            );
        }


        // End iterator points to one-off-end as usual
        iterator end() noexcept {
            if(_keys.empty()) {
                return iterator();
            }
            // Note this always assumes that the key and value iterators have steps.
            return iterator(
                _keys.template end_stride<K_Extent>().to_span(), // Note: this should be safe since the pointer is never de-reffed
                _values.template end_stride<V_Extent>().to_span()
            );
        }

        const_iterator end() const noexcept {
            if(_keys.empty()) {
                return const_iterator();
            }
            // Note this always assumes that the key and value iterators have steps.
            return const_iterator(
                _keys.template end_stride<K_Extent>().to_span(), // Note: this should be safe since the pointer is never de-reffed
                _values.template end_stride<V_Extent>().to_span()
            );
        }

        const_iterator cend() const noexcept {
            if(_keys.empty()) {
                return const_iterator();
            }
            // Note this always assumes that the key and value iterators have steps.
            return const_iterator(
                _keys.template cend_stride<K_Extent>().to_span(), // Note: this should be safe since the pointer is never de-reffed
                _values.template cend_stride<V_Extent>().to_span()
            );
        }

        [[nodiscard]] bool empty() const noexcept {
            return _keys.empty();
        }

        size_type size() const noexcept {
            return _keys.size();
        }

        //Fixme: size_type max_size() const noexcept;

        value_stride at(key_stride key) {
            auto val_pos = find_value_by_key(key);
            if(val_pos == _values.template end_stride<V_Extent>()) {
                throw std::out_of_range("key is not in small_byte_map_view");
            }
            return *val_pos;
        }

        const value_stride &at(key_stride key) const {
            auto val_pos = find_value_by_key(key);
            if(val_pos == _values.template end_stride<V_Extent>()) {
                throw std::out_of_range("key is not in small_byte_map_view");
            }
            return *val_pos;
        }

        size_type count(const key_stride &key) const {
            if(!_linear_mode) { // Our heuristic for whether it's worth searching.
                auto lower = std::lower_bound(_keys.template begin_stride<K_Extent>(), _keys.template end_stride<K_Extent>(), key);
                auto upper = std::upper_bound(lower, _keys.template end_stride<K_Extent>(), key);
                return std::count(lower, upper, key);
            } else { // This is faster for small vectors which we leave unsorted
                auto lower = _keys.template begin_stride<K_Extent>;
                auto upper = _keys.template end_stride<K_Extent>();
                return std::count(lower, upper, key);
            }
        }

        //Fixme: test the "find-key plus diff" approach against the "find-iterator" approach. Either may be faster.
        iterator find(const key_stride& key) {
            auto pos = find_key(key); //Gives me an iterator that gives me a span here
            if(pos == _keys.template end_stride<K_Extent>()) {
                return end();
            }
            auto diff = _keys.template end_stride<K_Extent>() - pos;
            return iterator(*pos, *(_values.template begin_stride<V_Extent>() + diff));
        }

        const_iterator find(const key_stride& key) const {
            auto pos = find_key(key); //Gives me an iterator that gives me a span here
            if(pos == _keys.template end_stride<K_Extent>()) {
                return end();
            }
            auto diff = _keys.template end_stride<K_Extent>() - pos;
            return const_iterator(*pos, *(_values.template begin_stride<V_Extent>() + diff));
        }

        bool contains(const key_stride& key) const {
            return find_key(key) != _keys.template end_stride<K_Extent>();
        }

        //Fixme: std::pair<iterator,iterator> equal_range( const Key& key );

        //Fixme: std::pair<iterator,iterator> equal_range( const Key& key );

    protected:

        bool _linear_mode = true; // If reserve moves to the heap, we sort the keys (and values) and use binary search

    private:

        key_range_view _keys; //This is sorted above InitialExtent, otherwise use linear search for parallelism.
        value_range_view _values;

    };

    template<size_t K_Extent = 1, size_t V_Extent = 1, typename byte_type = std::byte,
        size_t LinearExtent = 128, size_t KStackExtent = LinearExtent, size_t VStackExtent = LinearExtent>
    class small_byte_map : public small_byte_map_view<K_Extent, V_Extent, byte_type, LinearExtent> {

    public:

        using super = small_byte_map_view<K_Extent, V_Extent, byte_type, LinearExtent>;
        using iterator = typename super::iterator;
        using const_iterator = typename super::const_iterator;
        using size_type = typename super::size_type;
        using key_stride = typename super::key_stride;
        using value_stride = typename super::value_stride;
        //The extent after which the key and value vectors are sorted.
        using super::linear_extent;
        using value_type = typename iterator::value_type;

        // Construct with a reserve in *number of elements* (not byte size)!
        small_byte_map(std::size_t reserve_num_entries) : _keys_impl(reserve_num_entries * K_Extent), _values_impl(reserve_num_entries * V_Extent) {
            //TODO may not work == try putting this back
            //sync();
            super::reset(_keys_impl, _values_impl);
        }

        small_byte_map() : _keys_impl(), _values_impl() {
            //TODO may not work -- try putting this back
            //sync();
            super::reset(_keys_impl, _values_impl);
        }

        //Fixme: some copy constructors

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

    protected:

        void sync() {
            this->reset(_keys_impl, _values_impl);
        }

        // Ensures linear mode is disabled if we're above the linear extent
        void ensure_mode(bool presorted_hint = false) {
            if(this->size() > LinearExtent) {
                force_linear_mode(false, presorted_hint);
            }
        }

    public:

        // Call this only once the underlying range has been sorted due to expansions
        void force_linear_mode(bool linear_mode = true, bool presorted_hint = false) {
            if(!linear_mode && super::_linear_mode && !presorted_hint) {
                // To disable linear search mode from before, need to sort the data so we can do binary search
                // Note! This is relying on the assumption that std::sort implementation will use std::iter_swap under-the-hood.
                std::sort(this->begin(), this->end(), [](const typename iterator::value_type &lhs, const typename iterator::value_type &rhs) -> bool {
                    return lhs.first < rhs.second();
                });
            }
            super::_linear_mode = linear_mode;
        }

        // map-like APIs

        void reserve(size_type count) {
            _keys_impl.reserve(count * K_Extent);
            _values_impl.reserve(count * V_Extent);
        }

        void clear() noexcept {
            _keys_impl.clear();
            _values_impl.clear();
        }

        //Inserts element(s) into the container, if the container doesn't already contain an element with an equivalent key.
        std::pair<iterator,bool> insert(const value_type& value) {
            auto it = this->find(value.first);
            if(it != this->end()) {
                return {false, it};
            }
            if(this->linear_mode()) { //Linear insert: just put on the end
                _keys_impl.template push_back_stride<K_Extent>(value.first);
                _values_impl.template push_back_stride<V_Extent>(value.second);
                return {true, this->begin() + this->size()};
            } else {
                constexpr auto diff = it - this->begin();
                _keys_impl.insert_stride(_keys_impl.template begin_stride<K_Extent>() + diff, value.first);
                _values_impl.insert_stride(_values_impl.template begin_stride<V_Extent>() + diff, value.second);
                return {true, this->begin() + diff};
            }
        }

        //Fixme std::pair<iterator,bool> insert( value_type&& value );

        template< class InputIt >
        void insert( InputIt first, InputIt last ) {
            for(auto it = first; it < last; ++it) {
                insert(*it);
            }
        }

        std::pair<iterator, bool> insert_or_assign(const key_stride &k_stride, value_stride v_stride) {
            auto b_it_pair = insert(std::make_pair(k_stride, v_stride));
            if(!b_it_pair.first) { //No insert happened - we need to assign it
                //Note: by convention the V_Strides should be equal here, so the copy should be safe
                auto current_v_stride = b_it_pair.second->second;
                std::copy(current_v_stride.begin(), current_v_stride.end(), v_stride.begin());
            }
            return b_it_pair;
        }

        //Fixme: template <class M>
        //Fixme: pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj);

        //Fixme:template< class... Args >
        //Fixme: std::pair<iterator,bool> emplace( Args&&... args );

        iterator erase(const_iterator pos) {
            constexpr auto diff = pos - this->cbegin();
            _keys_impl.erase(_keys_impl.template begin_stride<K_Extent>() + diff);
            _values_impl.erase(_values_impl.template begin_stride<V_Extent>() + diff);
        }

        //Fixme iterator erase( const_iterator first, const_iterator last );

        //Fixme size_type erase( const key_type& key );

        //Fixme: void swap( unordered_map& other );

        //Fixme: rest of the C++17 API!

    private:

        small_vector<byte_type, KStackExtent> _keys_impl; // We create a small vector optimised buffer, with no underlying step.
        small_vector<byte_type, VStackExtent> _values_impl;

    };


}