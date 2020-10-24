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

#include "nonstd/span.hpp"

#include "variant-match.hpp"

// A collection of small vectors, maps and views useful for reading and writing slices

namespace common {

    //TODO might need to re-work this to allow for BLOCKS of bytes as this is currently BROKEN as designed...
    //TODO i.e. nonstd::span and std::array and std::string_view in a small_vector_view should be transparent.
    //TODO maybe visit range? TODO TODO understand this library again... TODO TODO TODO

    //TODO ALL THE WAY THROUGH WE NEED TO REVIEW EVERY SIZE() USAGE TO CHECK THAT WERE USING THE RIGHT VERSION OF SIZE ON THE ARRAY

    //TODO think about whether this is needed.
    // LargeSearchBuf: determines whether to use linear or binary search to find(...) in the span
    template<typename T, size_t LargeSearchBuf = 128>
    class small_vector_view {

    public:

        typedef std::size_t index_t;

        typedef T element_type;
        typedef std::remove_cv_t<T> value_type;

        typedef T &reference;
        typedef T *pointer;
        typedef T const *const_pointer;
        typedef T const &const_reference;

        typedef index_t index_type;
        typedef index_t difference_type;

        typedef pointer iterator;
        typedef const_pointer const_iterator;

        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        using size_type = std::size_t;

        small_vector_view() : _impl() {}

        small_vector_view(nonstd::span<T> &&spn) : _impl(std::move(spn)) {}

        small_vector_view(const nonstd::span<T> &spn) : _impl(spn) {}

        small_vector_view(T *const ptr, std::size_t size) : _impl(ptr, size) {}

        template<size_t N>
        small_vector_view(std::array<T, N> &spn) : small_vector_view(spn.data(), spn.size()) {}

        template<size_t N>
        small_vector_view(const std::array<T, N> &arr) : small_vector_view(arr.data(), arr.size()) {}

        small_vector_view(std::vector<T> &vec) : _impl(vec.data(), vec.size()) {}

        small_vector_view(const std::vector<T> &vec) : _impl(vec.data(), vec.size()) {}

    protected:

        void reset_span(nonstd::span<T> &&spn) {
            _impl = std::move(spn);
        }

        void reset_span(const nonstd::span<T> &spn) {
            _impl = spn;
        }

        void reset_span(T *const ptr, std::size_t size) {
            _impl = nonstd::span<T>(ptr, size);
        }

        template<size_t N>
        void reset_span(std::array<T, N> &arr) {
            reset_span(arr.data(), arr.size());
        }

        template<size_t N>
        void reset_span(const std::array<T, N> &arr) {
            reset_span(arr.data(), arr.size());
        }

        void reset_span(std::vector<T> &vec) {
            reset_span(vec.data(), vec.size());
        }

        void reset_span(const std::vector<T> &vec) {
            reset_span(vec.data(), vec.size());
        }

        // A special case where an array is used as a buffer but not *all* of the array is in use - some is reserved
        template<size_t N>
        void reset_span(std::pair<std::array<T, N>, size_type> &array_size_pair) {
            reset_span(array_size_pair.first.data(), array_size_pair.second);
        }

    public:

        //TODO replicate operator=


        // ELEMENT ACCESS

        constexpr reference at(size_type pos) {
            return *(_impl.begin() + pos);
        }

        constexpr const_reference at(size_type pos) const {
            return *(_impl.cbegin() + pos);
        }

        // Operator[] skipped

        constexpr reference front() {
            return *(_impl.begin());
        }

        constexpr const_reference front() const {
            return *(_impl.cbegin());
        }

        constexpr reference back() {
            return at(size() - 1);
        }

        constexpr const_reference back() const {
            return at(size() - 1);
        }

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

        constexpr iterator end() noexcept {
            return _impl.end();
        }

        constexpr const_iterator end() const noexcept {
            return _impl.end();
        }

        constexpr const_iterator cend() const noexcept {
            return _impl.cend();
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

        constexpr reverse_iterator rend() noexcept {
            return _impl.rend();
        }

        constexpr const_reverse_iterator rend() const noexcept {
            return _impl.rend();
        }

        constexpr const_reverse_iterator crend() const noexcept {
            return _impl.crend();
        }

        // CAPACITY

        [[nodiscard]] constexpr bool empty() const noexcept {
            return _impl.empty();
        }

        constexpr size_type size() const noexcept {
            return _impl.size();
        }

    private:

        nonstd::span<T> _impl;
    };


    //Fixme: performance tune SmallBufBytes default value.
    template<typename T, size_t SmallBufBytes = 128> //small enough to fit 8 64->64 bit mappings.
    class small_vector : public small_vector_view<T> {

    public:

        using typename small_vector_view<T>::size_type;
        using typename small_vector_view<T>::iterator;
        using typename small_vector_view<T>::const_iterator;
        using typename small_vector_view<T>::reference;

        // Contains the array buffer and the size in use
        using small_buffer_buff_t = std::array<T, SmallBufBytes / sizeof(T)>;
        using small_buffer_t = std::pair<small_buffer_buff_t, size_type>;
        using heap_vector_t = std::vector<T>;

        //Warning: notice the change of API - reserve not fill!
        //Fixme: make this a bit cheaper - don't allocate the array variant at all if the vector can hold it!
        small_vector(std::size_t reserve_cap) : _impl(std::make_pair(small_buffer_buff_t(), 0)) {
            reserve(reserve_cap);
        }

        small_vector() : _impl(std::make_pair(small_buffer_buff_t(), 0)) {
            sync_view();
        }


    private:

        // Ensures the view base class has exactly what the variant has below
        void sync_view() {
            std::visit([this](auto &elem) { this->reset_span(elem); }, _impl);
        }

    public:

        //TODO operator=

        // ELEMENT ACCESS - EXTENDED

        // CAPACITY

        constexpr size_type max_size() const noexcept {
            if (std::holds_alternative<small_buffer_t>(_impl)) {
                return std::get<small_buffer_t>(_impl).first.max_size();
            } else {
                return std::get<heap_vector_t>(_impl).max_size();
            }
        }

        constexpr void reserve(size_type new_cap) {
            if (std::holds_alternative<small_buffer_t>(_impl)) {
                if (std::get<small_buffer_t>(_impl).first.size() >= new_cap) {
                    return; // We're good.
                }
                // New capacity must be greater than fixed size- updgrade to heap
                _impl = heap_vector_t();
                sync_view();
            }
            // Reserve on heap
            std::get<heap_vector_t>(_impl).reserve(new_cap);
        }

        constexpr size_type capacity() const noexcept {
            if (std::holds_alternative<small_buffer_t>(_impl)) {
                return std::get<small_buffer_t>(_impl).first.size();
            } else {
                return std::get<heap_vector_t>(_impl).capacity();
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
                [](small_buffer_t &v) { v.second = 0; },
                [](heap_vector_t &v) { v.clear(); }
            }, _impl);
        }

        constexpr iterator insert(const_iterator pos, const T &value) {
            reserve(this->size() + 1);
            return std::visit(std::match{
                [&pos, &value](small_buffer_t &v) {
                    std::copy(pos, v.first.data() + v.first.size() - 1, pos + 1); // Move the data to the right.
                    v.first.at(pos) = value;
                    v.second++;
                    return pos;
                },
                [&pos, &value](heap_vector_t &v) { return v.insert(heap_vector_t::iterator(pos), value); }
            }, _impl);
        }

        constexpr iterator insert(const_iterator pos, T &&value) {
            reserve(this->size() + 1);
            return std::visit(std::match{
                [&pos, &value](small_buffer_t &v) {
                    std::copy(pos, v.first.data() + v.first.size() - 1, pos + 1); // Move the data to the right.
                    v.first.at(pos) = std::move(value);
                    v.second++;
                    return pos;
                },
                [&pos, &value](heap_vector_t &v) { return v.insert(heap_vector_t::iterator(pos), std::move(value)); }
            }, _impl);
        }

        //Fixme: skipped: constexpr iterator insert( const_iterator pos, size_type count,
        //                           const T& value );

        //Fixme: skipped: template< class InputIt >
        //constexpr iterator insert( const_iterator pos, InputIt first, InputIt last );

        //Fixme: skipped: constexpr iterator insert( const_iterator pos, std::initializer_list<T> ilist );


        template<class... Args>
        constexpr iterator emplace(const_iterator pos, Args &&... args) {
            reserve(this->size() + 1);

            if (std::holds_alternative<small_buffer_t>(_impl)) {
                auto &v = std::get<small_buffer_t>(_impl);
                std::copy(pos, v.first.data() + v.first.size() - 1, pos + 1); // Move the data to the right.
                v.first.at(pos) = T(args...); //Fixme: is this how we emplace properly? Should we use placement new?
                v.second++;
                return pos;
            }

            return std::get<heap_vector_t>(_impl)
                .emplace(heap_vector_t::iterator(pos), std::move(args)...);
        }

        constexpr iterator erase(const_iterator pos) {
            return std::visit(std::match{
                [pos](small_buffer_t &v) {
                    v.second--;
                    std::copy(pos + 1, v.first.size(), pos); //TODO test this works.
                    return pos;
                },
                [pos](heap_vector_t &v) { return iterator(v.erase(heap_vector_t::iterator(pos))); }
            }, _impl);
        }

        //Fixme skipped: constexpr iterator erase(const_iterator first, const_iterator last)

        constexpr void push_back(const T &value) {
            reserve(this->size() + 1);
            std::visit(std::match{
                [&value](small_buffer_t &v) {
                    v.at(v.second) = value;
                    v.second++;
                },
                [&value](heap_vector_t &v) { v.push_back(value); }
            }, _impl);
        }

        constexpr void push_back(T &&value) {
            reserve(this->size() + 1);
            std::visit(std::match{
                [&value](small_buffer_t &v) {
                    v.at(v.second) = std::move(value);
                    v.second++;
                },
                [&value](heap_vector_t &v) { v.push_back(std::move(value)); }
            }, _impl);
        }

        template<class... Args>
        constexpr reference emplace_back(Args &&... args) {
            reserve(this->size() + 1);
            return std::visit(std::match{
                [&args...](small_buffer_t &v) {
                    v.at(v.second) = T(std::move(args)...);
                    v.second++;
                },
                [&args...](heap_vector_t &v) { v.emplace_back(std::move(args)...); }
            }, _impl);
        }

        constexpr void pop_back() {
            erase(this->end());
        }

        //Fixme: skipped: constexpr void resize( size_type count );
        //Fixme: skipped: constexpr void resize( size_type count, const value_type& value );

        //Fixme: skipped: constexpr void swap( vector& other ) noexcept

        //Fixme: skipped: comparators


    private:

        std::variant<small_buffer_t, heap_vector_t> _impl;

    };


    template<size_t K_Size, size_t V_Size, typename byte_type = std::byte>
    struct small_map_view_iterator {

        using value_type = std::pair<nonstd::span<byte_type>, nonstd::span<byte_type>>;

        small_map_view_iterator() : impl(nonstd::span<byte_type>(), nonstd::span<byte_type>()) {}

        small_map_view_iterator(const nonstd::span<byte_type> &key_span,
            const nonstd::span<byte_type> &value_span) : impl(key_span, value_span){}

        small_map_view_iterator(nonstd::span<byte_type> &&key_span,
                                nonstd::span<byte_type> &&value_span)
                                : impl(std::move(key_span), std::move(value_span)) {}

        value_type &operator* () {
            return impl;
        }

        value_type *operator-> () {
            return &impl;
        }

        small_map_view_iterator<K_Size, V_Size> &operator++() {
            impl.first = nonstd::span<byte_type>(impl.first.data() + K_Size, K_Size);
            impl.second = nonstd::span<byte_type>(impl.second.data() + V_Size, V_Size);
            return *this;
        }

        small_map_view_iterator<K_Size, V_Size> operator++(int) {
            return small_map_view_iterator<K_Size, V_Size>(
                nonstd::span<byte_type>(impl.first.data() + K_Size, K_Size),
                nonstd::span<byte_type>(impl.second.data() + V_Size, V_Size)
            );
        }

        //Fixme: this should be difference type
        small_map_view_iterator operator+ (const std::size_t n) const {
            return small_map_view_iterator<K_Size, V_Size>(
                nonstd::span<byte_type>(impl.first.data() + n * K_Size, K_Size),
                nonstd::span<byte_type>(impl.second.data() + n * V_Size, V_Size)
            );
        }

        //Fixme: this should be difference type
        small_map_view_iterator operator- (const std::size_t n) const {
            return small_map_view_iterator<K_Size, V_Size>(
                nonstd::span<byte_type>(impl.first.data() - n * K_Size, K_Size),
                nonstd::span<byte_type>(impl.second.data() - n * V_Size, V_Size)
            );
        }

        // Comparison behaviour is the same as comparison of the key pointer, with no extent (so end can have 0 extent)
        bool operator ==(const small_map_view_iterator<K_Size, V_Size> &b) const {
            return impl.first.data() == b.impl.first.data();
        }

        bool operator !=(const small_map_view_iterator<K_Size, V_Size> &b) const {
            return impl.first.data() != b.impl.first.data();
        }

        bool operator >(const small_map_view_iterator<K_Size, V_Size> &b) const {
            return impl.first.data() > b.impl.first.data();
        }

        bool operator <(const small_map_view_iterator<K_Size, V_Size> &b) const {
            return impl.first.data() < b.impl.first.data();
        }

        bool operator >=(const small_map_view_iterator<K_Size, V_Size> &b) const {
            return impl.first.data() >= b.impl.first.data();
        }

        bool operator <=(const small_map_view_iterator<K_Size, V_Size> &b) const {
            return impl.first.data() <= b.impl.first.data();
        }

        value_type impl;

    };

} // ns common


//Fixme: the iterator traits on this.

namespace common {


    //TODO LESS IS MORE
    //TODO let's JUST build this based on std::byte and then have everything else wrap it.

    // A small multimap backed by contiguous byte storage. Great for operating on small packed + serialised structures.
    template<size_t K_Size, size_t V_Size, size_t LargeSearchBuf = 128>
    class small_map_view {

    public:

        using iterator = small_map_view_iterator<K_Size, V_Size>;
        using const_iterator = small_map_view_iterator<K_Size, V_Size, const std::byte>;
        using size_type = std::size_t;
        using key_type = std::byte[K_Size]; //Fixme: use these aliases more often to make things clearer
        using value_type = std::byte[V_Size];

        //TODO TODO this doesn't quite work again... we wanna be able to iterate in CHUNKS
        //TODO TODO need to have a think here... I thought this would work...
        //TODO but it's not since this is working as a pointer to a byte... we need a 4 byte block concept
        //TODO urgh urgh urgh TOOD this is pretty broke.
        using key_container = small_vector_view<std::byte[K_Size], LargeSearchBuf>;
        using value_container = small_vector_view<std::byte[V_Size], LargeSearchBuf>;

        small_map_view() : _keys(), _values() {}

        small_map_view(std::byte *const ptr, std::size_t byte_size)
            : _keys(static_cast<std::byte[K_Size]>(ptr), typed_size(byte_size)),
            _values(static_cast<std::byte[V_Size]>(ptr + (K_Size * typed_size(byte_size))), typed_size(byte_size)) {}

        //Fixme: add some constructors here

    protected:

        constexpr const std::size_t typed_size(const std::size_t byte_size) const {
            return byte_size / (K_Size + V_Size);
        }

        constexpr const bool valid_byte_size(const std::size_t byte_size) const {
            return (byte_size % (K_Size + V_Size)) == 0;
        }

        // Returns an index to the first byte in the underlying byte array that stores the first value in the value vector
        constexpr const std::size_t v_byte_offset(const std::size_t num_elems) const {
            return K_Size * num_elems;
        }

        constexpr const std::size_t k_byte_offset_for_idx(const std::size_t elem_idx) const {
            return elem_idx * K_Size;
        }

        constexpr const std::size_t v_byte_offset_for_idx(const std::size_t total_num_elems, const std::size_t elem_idx) const {
            return v_byte_offset(total_num_elems) + elem_idx * V_Size;
        }

        void reset_span(const nonstd::span<std::byte> &spn) {
            reset_span(spn.data(), spn.size());
        }

        void reset_span(std::byte *const ptr, std::size_t byte_size) {
            //TODO this is completely broken! TODO TODO this is basically working as a pointer-to-byte-* i.e. the beignning of the array.
            //TODO this needs to get std::byte *... with a width... like a std::4_byte_*- that would also fix it...
            //TODO i.e. the mistake in our assumption is sizeof(std::byte[4]). BUT IT DOES WORK THAT WAY...
            //TODO we need to be able to type to std::vector<char{4]> and have it behave normally
            //_keys.reset_span(reinterpret_cast<std::byte[K_Size]>(ptr), typed_size(byte_size));
            //_values.reset_span(reinterpret_cast<std::byte[V_Size]>(ptr + (K_Size * typed_size(byte_size))), typed_size(byte_size));
        }

        template<size_t N>
        void reset_span(std::array<std::byte, N> &arr) {
            //Fixme: constexpr check on N modulo
            reset_span(arr.data(), arr.size());
        }

        template<size_t N>
        void reset_span(const std::array<std::byte, N> &arr) {
            //Fixme: constexpr check on N modulo
            reset_span(arr.data(), arr.size());
        }

        void reset_span(std::vector<std::byte> &vec) {
            reset_span(vec.data(), vec.size());
        }

        void reset_span(const std::vector<std::byte> &vec) {
            reset_span(vec.data(), vec.size());
        }

        // A special case where an array is used as a buffer but not *all* of the array is in use - some is reserved
        template<size_t N>
        void reset_span(std::pair<std::array<std::byte, N>, std::size_t> &array_size_pair) {
            reset_span(array_size_pair.first.data(), array_size_pair.second);
        }

        constexpr typename key_container::iterator find_key(const key_type &needle) noexcept {
            if (_got_sorted) {
                return std::binary_search(_keys.begin(), _keys.end(), needle);
            } else {
                return std::find(_keys.begin(), _keys.end(), needle);
            }
        }

        constexpr typename key_container::const_iterator find_key(const key_type &needle) const noexcept {
            if (_got_sorted) {
                return std::binary_search(_keys.begin(), _keys.end(), needle);
            } else {
                return std::find(_keys.begin(), _keys.end(), needle);
            }
        }

        // Call this only once the underlying range has been sorted due to expansions
        void make_sorted() {
            _got_sorted = true;
        }

        bool is_sorted() const  {
            return _got_sorted;
        }

    public:

        iterator begin() noexcept {
            if(_keys.empty()) {
                return iterator();
            }
            return iterator(
                nonstd::span<std::byte>(*_keys.begin(), K_Size),
                nonstd::span<std::byte>(*_values.begin(), V_Size)
            );
        }

        const_iterator begin() const noexcept {
            if(_keys.empty()) {
                return const_iterator();
            }
            return const_iterator(
                nonstd::span<const std::byte>(*_keys.begin(), K_Size),
                nonstd::span<const std::byte>(*_values.begin(), V_Size)
            );
        }

        const_iterator cbegin() const noexcept {
            if(_keys.empty()) {
                return const_iterator();
            }
            return const_iterator(
                nonstd::span<const std::byte>(*_keys.cbegin(), K_Size),
                nonstd::span<const std::byte>(*_values.cbegin(), V_Size)
            );
        }

        // End iterator points to one-off-end as usual
        iterator end() noexcept {
            if(_keys.empty()) {
                return iterator();
            }
            return iterator(
                //TODO ensure this is valid when this iterator is very much pointing "off the end"
                //TODO - this is when compared against ++ in the loop, which WILL point off the end.
                nonstd::span<std::byte>(*_keys.end(), K_Size),
                nonstd::span<std::byte>(*_values.end(), V_Size)
            );
        }

        const_iterator end() const noexcept {
            if(_keys.empty()) {
                return const_iterator();
            }
            return const_iterator(
                //TODO ensure this is valid when this iterator is very much pointing "off the end"
                nonstd::span<const std::byte>(*_keys.end(), K_Size),
                nonstd::span<const std::byte>(*_values.end(), V_Size)
            );
        }

        const_iterator cend() const noexcept {
            if(_keys.empty()) {
                return const_iterator();
            }
            return const_iterator(
                //TODO ensure this is valid when this iterator is very much pointing "off the end"
                nonstd::span<const std::byte>(*_keys.cend(), K_Size),
                nonstd::span<const std::byte>(*_values.cend(), V_Size)
            );
        }

        [[nodiscard]] bool empty() const noexcept {
            return _keys.empty();
        }

        size_type size() const noexcept {
            return _keys.size();
        }

        //Fixme: size_type max_size() const noexcept;

        //TODO check operator = for byte arrays! TODO TODO - this may not work with the find - need to pass through comparator!

        value_type &at(const std::byte (&key)[K_Size]) {
            typename key_container::iterator pos;
            if(is_sorted()) {
                pos = std::lower_bound(_keys.begin(), _keys.end(), key);
            } else { // This is faster for small vectors which we leave unsorted
                pos = std::find(_keys.begin(), _keys.end(), key);
            }
            if(pos == _keys.end() || _keys.at(pos) != key) {
                throw std::out_of_range("key is not in small_map_view");
            }
            auto diff = pos - _keys.begin(); //Which could be 0!
            return _values.at(diff);
        }

        const value_type &at(const std::byte (&key)[100]) const {
            typename key_container::iterator pos;
            if(is_sorted()) {
                pos = std::lower_bound(_keys.begin(), _keys.end(), key);
            } else { // This is faster for small vectors which we leave unsorted
                pos = std::find(_keys.begin(), _keys.end(), key);
            }
            if(pos == _keys.end() || _keys.at(pos) != key) {
                throw std::out_of_range("key is not in small_map_view");
            }
            auto diff = pos - _keys.begin(); //Which could be 0!
            return _values.at(diff);
        }

        size_type count(const key_type &key) const {
            if(is_sorted()) {
                auto lower = std::lower_bound(_keys.begin(), _keys.end(), key);
                auto upper = std::upper_bound(lower, _keys.end(), key);
                return std::count(lower, upper, key);
            } else { // This is faster for small vectors which we leave unsorted
                auto lower = _keys.begin();
                auto upper = _keys.end();
                return std::count(lower, upper, key);
            }
        }

        iterator find(const key_type& key) {
            typename key_container::iterator pos;
            if(is_sorted()) {
                pos = std::lower_bound(_keys.begin(), _keys.end(), key);
            } else { // This is faster for small vectors which we leave unsorted
                pos = std::find(_keys.begin(), _keys.end(), key);
            }
            if(pos != _keys.end() && *pos == key) {
                auto diff = pos - _keys.begin(); //Which could be 0!
                return begin() + diff;
            }
            return end();
        }

        const_iterator find(const key_type& key) const {
            typename key_container::const_iterator pos;
            if(is_sorted()) {
                pos = std::lower_bound(_keys.cbegin(), _keys.cend(), key);
            } else { // This is faster for small vectors which we leave unsorted
                pos = std::find(_keys.cbegin(), _keys.cend(), key);
            }
            if(pos != _keys.cend() && *pos == key) {
                auto diff = pos - _keys.cbegin(); //Which could be 0!
                return cbegin() + diff;
            }
            return cend();
        }

        bool contains(const key_type& key) const {
            return find(key) != end();
        }

        //Fixme: std::pair<iterator,iterator> equal_range( const Key& key );

        //Fixme: std::pair<iterator,iterator> equal_range( const Key& key );

    private:

        key_container _keys; //This is sorted above LargeSearchBuf, otherwise use linear search for parallelism.
        value_container _values;
        bool _got_sorted = false; // If reserve moves to the heap, we sort the keys (and values) and use binary search

    };

    // A small map is a map for small maps which *doesn't* break out into a full heap-based std::unordered_map unlike std::small_vector does!
    // Instead, it follows the same layout as before just moves it to the heap, so that it can quickly be serialised or deserialised
    template<size_t K_Size, size_t V_Size, size_t LargeSearchBuf = 128>
    class small_map : public small_map_view<K_Size, V_Size, LargeSearchBuf> {

    public:

        // Constructo with a reserve in *number of elements*!
        small_map(std::size_t reserve_cap) : _impl(required_byte_size(reserve_cap)) {
            sync_views();
        }

        small_map() : _impl() {
            sync_views();
        }


        //TODO various - we'll have to find a way to chopping this in two...

        //TODO reserve will need, confusingly, to do a trick checking for LargeSearchBuf before linear searching...
        //TODO this is easy enough I think -- start with reserve as before

    protected:

        static constexpr const std::size_t required_byte_size(const std::size_t num_elems) {
            return (K_Size + V_Size) * num_elems;
        }

        void sync_views() {
            this->reset_span(_impl.data(), _impl.size());
        }

    private:

        small_vector<std::byte> _impl; // We create a small vector optimised buffer.

    };

} //ns common