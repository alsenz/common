#pragma once

#include <functional>
#include <memory>

namespace gt {

    template<typename T>
    class atomic_cow_seat;

    template<typename T>
    class atomic_cow_ptr;

    template<typename T>
    class atomic_cow_txn {

    public:

        using ptr = std::shared_ptr<T>;

    private:

        atomic_cow_txn(ptr &backref, const ptr &init) : _backref(backref), _cpy_of_t(std::make_shared<T>(*init)) {}

    public:

        T& operator*() const noexcept {
            return *_cpy_of_t;
        }

        T* operator->() const noexcept {
            return _cpy_of_t.get();
        }

        atomic_cow_ptr<T> commit() {
            std::atomic_store(&_backref, _cpy_of_t); //Fixme: the locking issue here is a source of concern.
            return atomic_cow_ptr(_backref);
        }

        atomic_cow_txn<T> &mutate(std::function<void(T &)> vis) {
            vis(*_cpy_of_t);
            return *this;
        }

    private:

        std::shared_ptr<T> &_backref; //The original pointer in the seat
        std::shared_ptr<T> _cpy_of_t; //This is an entire copy of T!
    };

    // A class that allows for reading of T
    template<typename T>
    class atomic_cow_ptr {

    public:

        using ptr = std::shared_ptr<T>;

    private:

        friend class atomic_cow_seat<T>;

        // We constify our copy of the shared pointer to T, and keep a non-const backreference to the smart pointer in the seat
        atomic_cow_ptr(ptr &backref) : _backref(backref), _shared_impl(backref) {}

    public:

        const T& operator*() const noexcept {
            return *_shared_impl;
        }

        const T* operator->() const noexcept {
            return _shared_impl.get();
        }

        atomic_cow_txn<T> begin_transaction() {
            return atomic_cow_txn<T>(_backref, _shared_impl);
        }

        atomic_cow_ptr<T> &visit(std::function<void(const T &)> vis) {
            vis(*_shared_impl);
            return *this;
        }


    private:

        std::shared_ptr<T> &_backref; //The original pointer in the seat- we don't use this!
        const std::shared_ptr<T> _shared_impl; //The 'safe' copy of the shared pointer.

    };

    // Holds the underlying T that is read and modified
    template<typename T>
    class atomic_cow_seat {

    public:

        atomic_cow_ptr<T> get() const {
            return atomic_cow_ptr<T>(_impl);
        }

        atomic_cow_ptr<T> &visit(std::function<void(const T &)> vis) {
            return get().visit(vis);
        }

        atomic_cow_ptr<T> &mutate(std::function<void(T &)> vis) {
            return get().begin_transaction().mutate(vis).commit();
        }

    private:

        std::shared_ptr<T> _impl;

    };



} //ns gt