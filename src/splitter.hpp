#pragma once

#include <cstddef>
#include <memory>
#include <functional>
#include <algorithm>
#include <optional>

#include "caf/all.hpp"

namespace as::common {


    template<typename ClzActor, typename T, typename O = T>
    struct splitter_state {

        // Reducers manipulate state in-place, and optionally return a caf::error if something has gone wrong.
        using reducer_t = std::function<caf::error(T &, O &&)>;
        // Propagate the error if we're continuing
        using error_reducer_t = std::function<caf::error(const caf::error &)>;

        // Reducer both reduces and returns the continue predicate for quorum based reductions, also takes latch as an argument.
        splitter_state(uint64_t latch, const T &init, reducer_t reducer, error_reducer_t error_reducer = {}) //By default we have no error reducer
        : _latch(latch), _fired(false), _data(init), _reducer(reducer), _error_reducer(error_reducer) {}

        bool _fired; //Guards _data from being destroyed when moved out, also prevents result_hdlr from firing multiple times.
        T _data;
        std::function<void(T &&)> _result_hdlr;
        std::function<void(const caf::error &)> _err_hdlr;
        std::size_t _latch;
        reducer_t _reducer;
        error_reducer_t  _error_reducer;

        void hit(O &&reducible) {
            if(_fired) {
                //TODO should we throw an error or what? TODO should we warn maybe?
                return;
            }
            // -- latch needs to come first so we can set the latch to anything we want in the reducer
            --_latch;
            const auto err = _reducer(_data, std::move(reducible));
            if(err) {
                hit(err); //Handle as if hit with an error
                return;
            } else if(_latch == 0) {
                _fired = true;
                _result_hdlr(std::move(_data));
            }
        }

        // Intercept errors and pass them straight on
        void hit(caf::error err) {
            if(_error_reducer) {
                err = _error_reducer(err);
                if(!err) {
                    return; //Nothing to do
                }
            }
            if(_fired) {
                // Note: this may be true with say a quorum based split where we fire even if under half of the targets return errors!
                return;
            }
            _fired = true;
            _err_hdlr(err);
        }

    };


    template<typename ClzActor, typename T, typename O = T, typename State = splitter_state<ClzActor, T, O>>
    class splitter {

    public:

        using reducer_t = typename splitter_state<ClzActor, T, O>::reducer_t;

        static constexpr auto default_reducer = [](T &t, O &&o) -> caf::error {
            t += std::move(o);
            return {}; //No error
        };

        splitter(ClzActor *thiz, std::size_t split_size, const T &init = T(),
            const reducer_t &reducer = default_reducer)
            : _thiz(thiz) {
            _impl = std::make_shared<State>(split_size, init, reducer);
        }

    protected:

        //Separate constructor for state provided - should only be used when extendingss
        splitter(ClzActor *thiz, std::shared_ptr<State> state) : _thiz(thiz) {
            _impl = state;
        }

    public:

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

        // Returns a promise object for the result.
        caf::response_promise make_response_promise() {
            auto rp = _thiz->make_response_promise();
            join_to(rp);
            return rp;
        }

        template<class TargetActorHdl, typename... Args>
        void delegate(TargetActorHdl target, Args... args) {
            //TODO: I've taken this out because i'm not sure it even is a race anymore
            //if (!_join_called) {
            //    throw std::logic_error("Race error: attempt to delegate without first calling join. This would result in a race.");
            //}
            auto impl = _impl;
            _thiz->request(target, caf::infinite, args...).then([impl](O res) mutable {
                impl->hit(std::move(res));
            }, [impl](const caf::error &err){
                impl->hit(err);
            });
        }

        template<class TargetActorHdl, typename R, typename... Args>
        void delegate_transform(TargetActorHdl target, std::function<O(R&&)> transformer, Args... args) {
            //TODO I've taken this out because I'm not sure it even is a race anymore
            //if (!_join_called) {
            //    throw std::logic_error("Race error: attempt to delegate without first calling join. This would result in a race.");
            //}
            auto impl = _impl;
            _thiz->request(target, caf::infinite, args...).then([impl, transformer](R res) mutable {
                impl->hit(transformer(std::move(res)));
            }, [impl](const caf::error &err){
                impl->hit(err);
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

        std::shared_ptr<State> impl() {
            return _impl;
        }

    private:

        ClzActor *_thiz;
        bool _join_called = false; //To prevent race conditions.
        std::shared_ptr<State> _impl;

    };

    template<typename ClzActor, typename T, typename O = T>
    struct quorum_splitter_state : splitter_state<ClzActor, T, O> {

        using super = splitter_state<ClzActor, T, O>;
        using reducer_t = typename super::reducer_t;
        using error_reducer_t = typename super::error_reducer_t;

        quorum_splitter_state(uint64_t latch, const T &init, std::size_t split_size, std::size_t quorum_size)
            : splitter_state<ClzActor, T, O>(latch, init, {}/*default ct reducer*/), split_size(split_size), quorum_size(quorum_size), error_ct(0), success_ct(0) {}

        // We need to be able to lazily set the reducer so we can refer to the state itself!
        void set_reducer(const reducer_t &r_fn) {
            super::_reducer = r_fn;
        }

        void set_error_reducer(const error_reducer_t &er_fn) {
            super::_error_reducer = er_fn;
        }

        std::size_t split_size;
        std::size_t quorum_size;
        std::size_t error_ct;
        std::size_t success_ct;

    };

    //Quorum: just like splitter, but a count, a quorum, a reducer, a predicate to check...
    //Note that the reducer is only called *before* quorum is achieved......
    template<typename ClzActor, typename T, typename O = T>
    class quorum_splitter : public splitter<ClzActor, T, O, quorum_splitter_state<ClzActor, T, O>> {

        public:

        using state_t = quorum_splitter_state<ClzActor, T, O>;
        using reducer_t = typename state_t::reducer_t;

    private:

        static std::shared_ptr<quorum_splitter_state<ClzActor, T, O>> make_quorum_state(const T &init, uint64_t latch, uint64_t quorum,
            const reducer_t &reducer = splitter<ClzActor, T, O>::default_binary_reducer) {

            //quorum_splitter_state(uint64_t latch, const T &init, std::size_t split_size, std::size_t quorum_size)
            auto q_state = std::make_shared<quorum_splitter_state<ClzActor, T, O>>();
            auto wrapped_reducer_fn = [q_state, reducer](T &t, O &&o) -> caf::error {
                const auto err = reducer(t, std::move(o));
                if(!err) {
                    // Have we reached quorum? If so, we need a way to 'fire early'
                    q_state->_latch = 0; // Set the latch to 0. A bit of a hack, but it should do the job! //TODO: hack?
                    return {}; //We're goot to go!
                }
                // Can quorum still be reached?
                if(q_state->error_ct < (q_state->split_size - q_state->quorum_size)) {
                    // We wipe out any error since we don't want to hit the error handler
                    return {};
                } else { // Quorum cannot be reached - now we fail!
                    //TODO sort out the error handling a bit better here! - we need a way to say quorum hasn't been reached in a better way
                    return err;
                }
            };
            q_state->set_reducer(wrapped_reducer_fn);
            auto wrapped_error_reducer = [q_state](const caf::error &err) -> caf::error {
                // Can quorum still be reached?
                if(q_state->error_ct < (q_state->split_size - q_state->quorum_size)) {
                    return {}; //Wipe out the error
                }
                //TODO better error handling here.
                return err; // Pass the error on - quorum can no longer be reached
            };
            q_state->set_error_reducer(wrapped_error_reducer);
        }

    public:

            // Quorum splitters always have
            quorum_splitter(ClzActor *thiz, uint64_t split_size, std::size_t quorum_size, const T &init = T(),
                     const reducer_t &reducer = splitter<ClzActor, T, O>::default_binary_reducer)
                     : splitter<ClzActor, T, O, state_t>(make_quorum_state(init, split_size, quorum_size, reducer)) //TODO ctors for this... need a sharewd pointer option!
            {
                if(quorum_size > split_size) {
                    throw std::logic_error("quorum_splitter: quorum greater than split size.");
                }
            }

    };

    template<template<typename, class> class C, typename T, class A = std::allocator<T>>
    caf::error insert_end(C<T, A> &c, const T &next) {
        c.insert(c.end(), next);
        return {};
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
    caf::error insert_in_order(C<T, A> &c, const T &next) {
        c.insert(std::lower_bound(c.begin(), c.end(), next), next);
        return {};
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

    template<template<typename, class> class C, typename T, class A = std::allocator<T>>
    caf::error insert_merge(C<T, A> &accumd, const C<T, A> &next) {
        std::sort(next.begin(), next.end());
        auto first_of_new = accumd.insert(accumd.end(), next.begin(), next.end());
        std::inplace_merge(accumd.begin(), first_of_new, accumd.end());
        return {};
    }

    //Special type of splitter that merges together containers in sorted order.
    template<typename ClzActor, template<typename, class> class C, typename T, class A = std::allocator<T>>
    class merger : public splitter<ClzActor, C<T, A>, C<T, A>> {

    public:
            merger(ClzActor *thiz, std::size_t split_size)
            : splitter<ClzActor, C<T, A>, C<T, A>>(thiz, split_size, C<T, A>(), insert_merge) {}
    };

} //ns as::common
