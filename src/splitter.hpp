#pragma once

#include <cstddef>
#include <memory>
#include <functional>
#include <algorithm>

#include "caf/all.hpp"


namespace as::common {

    template<typename ClzActor, typename T, typename O = T>
    class splitter {

    public:

        // Default splitter uses + as an reducer operation and expects T to be default initialised!
        splitter(ClzActor *thiz, std::size_t split_size, const T &init = T(),
            const std::function<T(T &&, const O&)> &reducer = std::plus<T>())
            : _thiz(thiz), _reducer(reducer) {
            _impl = std::make_shared<splitter_impl>(split_size, init);
        }

        void join(std::function<void(T &&)> result_handler, std::function<void(const caf::error &)> err_handler = {}) {
            _impl->_result_hdlr = result_handler;
            if(err_handler) {
                _impl->_err_hdlr = err_handler;
            }
            _join_called = true;
        }

        void join_to(caf::response_promise rp) {
            join([rp](T &&r) mutable {
                rp.deliver(std::move(r));
            },
            [rp](const caf::error &err) mutable {
                rp.deliver(err);
            });
        }

        template<typename S>
        void join_to(caf::response_promise rp, std::function<S(T &&)> transformer) {
            join([rp, transformer](T &&r) mutable {
                     rp.deliver(transformer(std::move(r)));
                 },
                 [rp](const caf::error &err) mutable {
                     rp.deliver(err);
                 });
        }

        template<class TargetActorHdl, typename... Args>
        void delegate(TargetActorHdl target, Args... args) {
            if (!_join_called) {
                throw std::logic_error("Race error: attempt to delegate without first calling join. This would result in a race.");
            }
            auto impl = _impl;
            _thiz->request(target, caf::infinite, args...).then([impl](O &&res) mutable {
                impl.hit(std::move(res));
            }, [impl](const caf::error &err){
                impl.hit(err);
            });
        }

        template<class TargetActorHdl, typename R, typename... Args>
        void delegate_transform(TargetActorHdl target, std::function<O(R&&)> transformer, Args... args) {
            if (!_join_called) {
                throw std::logic_error("Race error: attempt to delegate without first calling join. This would result in a race.");
            }
            auto impl = _impl;
            _thiz->request(target, caf::infinite, args...).then([impl, transformer](R &&res) mutable {
                impl.hit(transformer(std::move(res)));
            }, [impl](const caf::error &err){
                impl.hit(err);
            });
        }

        template<typename... Args>
        void delegate_to_self(Args... args) {
            delegate(caf::actor_cast<typename ClzActor::actor_hdl>(_thiz), args...);
        }

        template<typename R, typename... Args>
        void delegate_to_self(std::function<O(R)> transformer, Args... args) {
            return delegate_transform(caf::actor_cast<typename ClzActor::actor_hdl>(_thiz), transformer, args...);
        }

    protected:

        struct splitter_impl {
            splitter_impl(std::size_t latch, const T &init) : _latch(latch), _data(init) {}
            std::size_t _latch;
            T _data;
            std::function<void(T &&)> _result_hdlr;
            std::function<void(const caf::error &)> _err_hdlr;
        };

        splitter_impl &impl() {
            return _impl;
        }

    private:

        ClzActor *_thiz;
        std::function<T(T &&, const O&)> _reducer;
        bool _join_called = false; //To prevent race conditions.
        std::shared_ptr<splitter_impl> _impl;

    };

    template<template<typename, class> class C, typename T, class A = std::allocator<T>>
    C<T, A> insert_end(C<T, A> &&c, const T &next) {
        c.insert(c.end(), next);
        return std::move(c);
    }

    // Special type of splitter that inserts the received objects into a container
    template<typename ClzActor, template<typename, class> class C, typename T, class A = std::allocator<T>>
    class collector: public splitter<ClzActor, C<T, A>, T> {
        public:
            collector(ClzActor *thiz, std::size_t split_size)
            : splitter<ClzActor, C<T, A>, T>(thiz, split_size, C<T, A>(), insert_end) {
                splitter<ClzActor, C<T, A>, T>::impl()._data.reserve(split_size);
            }
    };

    // An alternative third argument for the collector constructor that insertion sorts elements as they are collected
    template<template<typename, class> class C, typename T, class A = std::allocator<T>>
    C<T, A> insert_in_order(C<T, A> &&c, const T &next) {
        c.insert(std::lower_bound(c.begin(), c.end(), next), next);
        return std::move(c);
    }

    // Special type of splitter that inserts the received objects into a container in their natural ordering
    template<typename ClzActor, template<typename, class> class C, typename T, class A = std::allocator<T>>
    class ordered_collector: public splitter<ClzActor, C<T, A>, T> {
    public:
        ordered_collector(ClzActor *thiz, std::size_t split_size)
            : splitter<ClzActor, C<T, A>, T>(thiz, split_size, C<T, A>(), insert_in_order) {
            splitter<ClzActor, C<T, A>, T>::impl()._data.reserve(split_size);
        }
    };

    //Special type of splitter that merges together containers in sorted order.
    template<typename ClzActor, template<typename, class> class C, typename T, class A = std::allocator<T>>
    class merger : public splitter<ClzActor, C<T, A>, C<T, A>> {
    private:
        C<T, A> insert_merge(C<T, A> &&accumd, C<T, A> next) {
            std::sort(next.begin(), next.end());
            auto first_of_new = accumd.insert(accumd.end(), next.begin(), next.end());
            std::inplace_merge(accumd.begin(), first_of_new, accumd.end());
            return std::move(accumd);
        }
    public:
            merger(ClzActor *thiz, std::size_t split_size)
            : splitter<ClzActor, C<T, A>, C<T, A>>(thiz, split_size, C<T, A>(), insert_merge) {}
    };

} //ns as::common
