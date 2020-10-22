#pragma once

#include <array>
#include <variant>
#include <vector>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <functional>

#include "nonstd/span.hpp"

#include "variant-match.hpp"

// A collection of small vectors, maps and views useful for reading and writing slices

namespace common {

    // LargeSearchBuf: determines whether to use linear or binary search to find(...) in the span
    template<typename T, size_t LargeSearchBuf = 128>
    class small_vector_view {

    public:

        typedef std::size_t index_t;

        typedef T element_type;
        typedef std::remove_cv_t<T> value_type;

        typedef T &       reference;
        typedef T *       pointer;
        typedef T const * const_pointer;
        typedef T const & const_reference;

        typedef index_t   index_type;
        typedef index_t   difference_type;

        typedef pointer       iterator;
        typedef const_pointer const_iterator;

        typedef std::reverse_iterator< iterator >       reverse_iterator;
        typedef std::reverse_iterator< const_iterator > const_reverse_iterator;

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
        void reset_span(std::pair<std::array<T, N> , size_type> &array_size_pair) {
            reset_span(array_size_pair.first.data(), array_size_pair.second);
        }

        constexpr iterator find_linear(const value_type &needle) noexcept {
            return std::find(begin(), end(), needle);
        }

        constexpr const_iterator find_linear(const value_type &needle) const noexcept {
            return std::find(cbegin(), cend(), needle);
        }

        constexpr iterator find_binary(const value_type &needle) noexcept {
            return std::binary_search(begin(), end(), needle);
        }

        constexpr const_iterator find_binary(const value_type &needle) const noexcept {
            return std::binary_search(cbegin(), cend(), needle);
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

        constexpr T* data() noexcept {
            return _impl.data();
        }

        constexpr const T* data() const noexcept {
            _impl.begin();
            return _impl.data();
        }

        // ELEMENT ACCESS - EXTENDED

        constexpr iterator find(const value_type &needle) noexcept {
            if(size() >= LargeSearchBuf) {
                return find_binary(needle);
            } else {
                return find_linear(needle);
            }
        }

        constexpr const_iterator find(const value_type &needle) const noexcept {
            if(size() >= LargeSearchBuf) {
                return find_binary(needle);
            } else {
                return find_linear(needle);
            }
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

        constexpr reverse_iterator rbegin() noexcept{
            return _impl.rbegin();
        }

        constexpr const_reverse_iterator rbegin() const noexcept {
            return _impl.rbegin();
        }

        constexpr const_reverse_iterator crbegin() const noexcept {
            return _impl.crbegin();
        }

        constexpr reverse_iterator rend() noexcept{
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
        using small_buffer_t = std::pair<small_buffer_buff_t , size_type>;
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
            std::visit([this](auto &elem){ this->reset_span(elem); }, _impl);
        }

    public:

        //TODO operator=

        // ELEMENT ACCESS - EXTENDED

        // CAPACITY

        constexpr size_type max_size() const noexcept {
            if(std::holds_alternative<small_buffer_t>(_impl)) {
                return std::get<small_buffer_t>(_impl).first.max_size();
            } else {
                return std::get<heap_vector_t>(_impl).max_size();
            }
        }

        constexpr void reserve(size_type new_cap) {
            if(std::holds_alternative<small_buffer_t>(_impl)) {
                if(std::get<small_buffer_t>(_impl).first.size() >= new_cap) {
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
            if(std::holds_alternative<small_buffer_t>(_impl)) {
                return std::get<small_buffer_t>(_impl).first.size();
            } else {
                return std::get<heap_vector_t>(_impl).capacity();
            }
        }

        // This only has an effect if in vector mode
        constexpr void shrink_to_fit() {
            if(std::holds_alternative<heap_vector_t>(_impl)) {
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
                    std::copy(pos, v.first.data() + v.first.size() - 1, pos+1); // Move the data to the right.
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
                    std::copy(pos, v.first.data() + v.first.size() - 1, pos+1); // Move the data to the right.
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


        template< class... Args >
        constexpr iterator emplace( const_iterator pos, Args&&... args ) {
            reserve(this->size() + 1);

            if(std::holds_alternative<small_buffer_t>(_impl)) {
                auto &v = std::get<small_buffer_t>(_impl);
                std::copy(pos, v.first.data() + v.first.size() - 1, pos+1); // Move the data to the right.
                v.first.at(pos) = T(args...); //Fixme: is this how we emplace properly? Should we use placement new?
                v.second++;
                return pos;
            }

            return std::get<heap_vector_t>(_impl)
                .emplace(heap_vector_t::iterator(pos), std::move(args)...);
        }

        constexpr iterator erase(const_iterator pos) {
            return 0; //TODO as before
        }

        //Fixme skipped: constexpr iterator erase(const_iterator first, const_iterator last)

        constexpr void push_back(const T& value) {
            //TODO
        }

        constexpr void push_back( T&& value ) {
            //TODO
        }

        template<class... Args>
        constexpr reference emplace_back(Args&&... args) {
            return T(); //TODO
        }

        constexpr void pop_back() {
            //TODO
        }

        //Fixme: skipped: constexpr void resize( size_type count );
        //Fixme: skipped: constexpr void resize( size_type count, const value_type& value );

        //Fixme: skipped: constexpr void swap( vector& other ) noexcept

        //Fixme: skipped: comparators


    private:

        std::variant<small_buffer_t, heap_vector_t> _impl;

    };

    //TODO
    template<typename K, typename V, size_t LargeSearchBuf = 128>
    class small_map_view {

    public:

        //TODO cut in two - sum sizeof K + V and divide the whole thing - gets us number of elements...
        //TODO problem is that as we grow we also have to grow to the right... but we live with this... Hmm...
        //TODO another problem is the capacity is just gonna grow. this is difficult in a few ways hmm... really really tricky...
        //TODO hmm... so two moves in the base - once of the values to the right twice, then the keys plus initial values to the right once.. interesting!

        //TODO iterator types here are gonna be a friggin' nightmare... TODO use iterator adaptors where poss.


    protected:

        constexpr const std::size_t typed_size(const std::size_t byte_size) const {
            //TODO need to add up both and divide them out.
            return 0;
        }

        //TODO this now has to be based on std::vector<std::byte> TODO TODO because we want the two contiguous

        void reset_span(const nonstd::span<std::byte> &spn) {
            reset_span(spn.data(), spn.size());
        }

        void reset_span(std::byte *const ptr, std::size_t size) {
            //TODO fix this one up using some nice logic...
            //TOOD TODO we ned to calculate the bits
            //_impl = nonstd::span<T>(ptr, size);
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

    public:


        //TODO copy API in...


    private:

        small_vector_view<K, LargeSearchBuf> _keys; //This is sorted above LargeSearchBuf, otherwise use linear search for parallelism.
        small_vector_view<V, LargeSearchBuf> _values;

    };

    // A small map is a map for small maps which *doesn't* break out into a full heap-based std::unordered_map unlike std::small_vector does!
    // Instead, it follows the same layout as before just moves it to the heap, so that it can quickly be serialised or deserialised
    template<typename K, typename V, size_t LargeSearchBuf = 128>
    class small_map : small_map_view<K, V, LargeSearchBuf> {

    public:

        //TODO various - we'll have to find a way to chopping this in two...

    protected:

        //TODO sync_views- copy in as before, just use _impl .data
        void sync_views() {
            this->reset_span(_impl.data(), _impl.size());
        }

    private:

        small_vector<std::byte> _impl; // We create a small vector optimised buffer.

    };

} //ns common