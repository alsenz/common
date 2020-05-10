#pragma once

#include <cstddef>
#include <memory>
#include <functional>
#include <algorithm>
#include <optional>
#include <chrono>
#include <stdexcept>

#include "caf/all.hpp"

namespace as::common {

    //TODO we should be able to get rid of ClzActor: 1) we need request, 2) we need system() for spawn...

    //TODO refactor this further for further simplification by using polymorphism on shared_ptr's instead of having to override as much...

    template<typename T, typename O = T>
    struct splitter_state {

        // Reducers manipulate state in-place, and optionally return a caf::error if something has gone wrong.
        using reducer_t = std::function<caf::error(T &, O &&)>;
        // Propagate the error if we're continuing
        using error_reducer_t = std::function<caf::error(const caf::error &)>;

        // Reducer both reduces and returns the continue predicate for quorum based reductions, also takes latch as an argument.
        template<class ClzActor>
        splitter_state(ClzActor *, uint64_t latch, const T &init, reducer_t reducer, error_reducer_t error_reducer) //By default we have no error reducer
        : _fired(false), _data(init), _latch(latch), _reducer(reducer), _error_reducer(error_reducer) {
            if(!_reducer || !_error_reducer) {
                throw std::logic_error("In splitter, reducer or error_reducer not set!");
            }
        }

        bool _fired; //Guards _data from being destroyed when moved out, also prevents result_hdlr from firing multiple times.
        T _data;
        std::size_t _latch;
        reducer_t _reducer;
        error_reducer_t  _error_reducer;

        std::function<void(T &&)> _result_hdlr;
        std::function<void(const caf::error &)> _err_hdlr;

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
                on_fire();
                if(_result_hdlr) {
                    _result_hdlr(std::move(_data));
                }
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
            on_fire();
            if(_err_hdlr) {
                _err_hdlr(err);
            }
        }

        // Used to test for secenarios where the latch is already 0 - i.e. size 0 spliter
        void pre_hit() {
            if(_latch == 0 && !_fired) {
                _fired = true;
                on_fire();
                if(_result_hdlr) {
                    _result_hdlr(std::move(_data));
                }
            }
        }

        virtual void on_fire() {
            std::cout << "The virtual void on_fire...()" << std::endl;
            //No-op default
        }

    };

    template<typename T, typename O = T>
    struct timeout_splitter_state : splitter_state<T, O> {

        using timeout_atom = caf::atom_constant<caf::atom("spto")>;
        using timeout_actor_t = caf::typed_actor<caf::reacts_to<timeout_atom>>;
        using super = splitter_state<T, O>;

        using reducer_t = typename splitter_state<T, O>::reducer_t;
        using error_reducer_t = typename splitter_state<T, O>::error_reducer_t;
        static constexpr int s_default_timeout = 100;

        template<typename ClzActor>
        timeout_splitter_state(ClzActor *thiz, uint64_t latch, const T &init, reducer_t reducer,
            error_reducer_t error_reducer, const std::chrono::milliseconds &timeout = std::chrono::milliseconds(s_default_timeout))
            : splitter_state<T, O>(thiz, latch, init, reducer, error_reducer) {
            _timeout_actor = thiz->system().spawn([this, &timeout](timeout_actor_t::pointer self) -> timeout_actor_t::behavior_type {
                self->delayed_send(self, timeout, timeout_atom::value);
                return {
                    [this](timeout_atom) {
                        std::cout << "Going to hit_timeout...." << std::endl;
                        this->fire_timeout();
                    }
                };
            }); //Note: is spawning a new actor the simple way to hit a function on a delay? We could just have a pool of actors that hit functions...
        }

        virtual void on_fire() override {
            std::cout << "TODO TODO TODO THIS ABSOLUTELY NEEDS TO FIRE AT LEAST ONCE...." << std::endl;
            std::cout << "In on_fire..." << std::endl;
            // Kill the timeout
            caf::anon_send_exit(_timeout_actor, caf::exit_reason::kill);
            std::cout << "Done in on_fire..." << std::endl;
        }

        // Used to fire with timeouts if necessary
        void fire_timeout() {
            if(!super::_fired) { // Need to fire an error
                super::_fired = true;
                std::cout << "... pre-A" << std::endl;
                //TODO why are we not hitting the on_fire
                //TODO this continues to be a bit mad... we continue to hit the virtual void which i just don't understand.... from within hit_timeout as well...
                //on_fire(); //TODO do we hit on_fire here.... since it's actually a timeout and we DONT want to kill ourselves as an acotr...
                std::cout << "::A" << std::endl;
                if(super::_err_hdlr) {
                    std::cout << "::B" << std::endl;
                    //TODO so this is there but it's junk... TODO TODO let's find out why... TODO TODO next step!
                    //TODO PUT THIS BACK IN.... and find out why we segfault after ::D as well!
                    super::_err_hdlr(caf::make_error(caf::sec::request_timeout));
                    std::cout << "::C" << std::endl;
                }
                std::cout << "::D" << std::endl;
            }
        }

    private:

        timeout_actor_t _timeout_actor;

    };

    template<typename ClzActor, typename T, typename O = T,  template<typename, typename> typename State = splitter_state>
    class splitter {

    public:

        using splitter_state_t = State<T, O>;
        using reducer_t = typename splitter_state_t::reducer_t;
        using error_reducer_t = typename splitter_state_t::error_reducer_t;

        static constexpr auto default_reducer = [](T &t, O &&o) -> caf::error {
            t += std::move(o);
            return {}; //No error
        };

        static constexpr auto default_error_reducer = [](const caf::error &e) -> caf::error {
            std::cout << "Firing default error reducer..." << std::endl;
            return e;
        };

        splitter(ClzActor *thiz, std::size_t split_size, const T &init = T(),
                 const reducer_t &reducer = default_reducer, const error_reducer_t &error_reducer = default_error_reducer)
                 //Note: the contract with splitter state constructor is that it must *at least* have these arguments.
                 : splitter(thiz, std::make_shared<splitter_state_t>(thiz, split_size, init, reducer, error_reducer)) {} //TODO add

    protected:

        //Separate constructor for state provided - should only be used when extendingss
        splitter(ClzActor *thiz, std::shared_ptr<splitter_state_t> state)
            : _thiz(thiz), _impl(state) {}

    public:

        void join(std::function<void(T &&)> result_handler, std::function<void(const caf::error &)> err_handler = {}) {
            _impl->_result_hdlr = result_handler;
            if(err_handler) {
                std::cout << "Setting error handler..." << std::endl;
                _impl->_err_hdlr = err_handler;
            }
            _impl->pre_hit();
        }

        void join_to(caf::response_promise rp) {
            join([rp](T &&r) mutable {
                rp.deliver(std::move(r));
            },
            [rp](const caf::error &err) mutable {
                std::cout << "In 194 error handler... " << std::endl;
                //TODO me feels that somehow rp is dead inside deliver... that's a bit worrying...
                //TODO log this out or something?
                std::cout << "Is response pending? " << std::endl;
                std::cout << rp.pending() << std::endl;
                std::cout << "Delivering error..." << std::endl;
                caf::error err2;
                rp.deliver(err2);
                std::cout << "The other side of it..." << std::endl;
            });
        }

        template<typename S>
        void join_to(caf::response_promise rp, std::function<S(T &&)> transformer) {
            join([rp, transformer](T &&r) mutable {
                     rp.deliver(transformer(std::move(r)));
                 },
                 [rp](const caf::error &err) mutable {
                     std::cout << "In 206 error handler...." << std::endl;
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
            //TODO: I've taken this out because i'm not sure it even is a race anymore //TODO validate this
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
            //TODO I've taken this out because I'm not sure it even is a race anymore //TODO validate this
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

        std::shared_ptr<splitter_state_t> impl() {
            return _impl;
        }

    private:

        ClzActor *_thiz;
        std::shared_ptr<splitter_state_t> _impl;

    };

    template<typename ClzActor, typename T, typename O = T>
    struct timed_splitter : splitter<ClzActor, T, O, timeout_splitter_state> {

        using super = splitter<ClzActor, T, O, timeout_splitter_state>;
        using reducer_t = typename super::reducer_t;
        using error_reducer_t = typename super::error_reducer_t;

        timed_splitter(ClzActor *thiz, std::size_t split_size, const T &init = T(),
                 const reducer_t &reducer = super::default_reducer, const std::chrono::milliseconds &timeout = std::chrono::milliseconds(0))
        //Note: the contract with splitter state constructor is that it must *at least* have these arguments.
            : super(thiz, std::make_shared<timeout_splitter_state<T, O>>(thiz, split_size, init, reducer, super::default_error_reducer, timeout)) {}

        timed_splitter(ClzActor *thiz, std::size_t split_size, const T &init, const std::chrono::milliseconds &timeout)
        //Note: the contract with splitter state constructor is that it must *at least* have these arguments.
            : super(thiz, std::make_shared<timeout_splitter_state<T, O>>(thiz, split_size, init, super::default_reducer, super::default_error_reducer, timeout)) {}

    };

    template<typename T, typename O = T>
    struct quorum_splitter_state : timeout_splitter_state<T, O> {

        using super = timeout_splitter_state<T, O>;
        using reducer_t = typename super::reducer_t;
        using error_reducer_t = typename super::error_reducer_t;

        template<class ClzActor>
        quorum_splitter_state(ClzActor *thiz, uint64_t latch, const T &init, std::size_t quorum_size,
            reducer_t reducer, error_reducer_t error_reducer = {}, const std::chrono::milliseconds &timeout_millis = std::chrono::milliseconds(super::s_default_timeout))
            : super(thiz, latch, init, reducer, error_reducer, timeout_millis), split_size(split_size), quorum_size(quorum_size), error_ct(0), success_ct(0) {}

    public:

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

    //TODO pick this up -- what's compiling again? TODO from here-- there's a segfault involving error handling...
    //TODO fix this up-- tests including quorum tests...
    //TODO back to raft! Really make some progress here!
    //TODO TODO let's get compiling! TODO from here.... TODO yay let's actually pick this up now...

    //Quorum: just like splitter, but a count, a quorum, a reducer, a predicate to check...
    //Note that the reducer is only called *before* quorum is achieved......
    template<typename ClzActor, typename T, typename O = T>
    class quorum_splitter : public splitter<ClzActor, T, O, quorum_splitter_state> {

        public:

        using state_t = quorum_splitter_state<T, O>;
        using reducer_t = typename state_t::reducer_t;
        using error_reducer_t = typename state_t::error_reducer_t ;
        using super = splitter<ClzActor, T, O, quorum_splitter_state>;

        static constexpr int s_default_timeout = 1000;

    private:

        static std::shared_ptr<quorum_splitter_state<T, O>> make_quorum_state(ClzActor *thiz, const T &init, uint64_t latch, uint64_t quorum,
            const reducer_t &reducer = splitter<ClzActor, T, O>::default_binary_reducer,
            const error_reducer_t &error_reducer = splitter<ClzActor, T, O>::default_error_reducer,
            const std::chrono::milliseconds timeout_millis = std::chrono::milliseconds(s_default_timeout)) {

            // We set nullptr reducer and error reducer
            auto q_state = std::make_shared<quorum_splitter_state<T, O>>(thiz, latch, init, quorum, nullptr, nullptr, timeout_millis);
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
            auto wrapped_error_reducer = [q_state, error_reducer](const caf::error &err) -> caf::error {
                // Can quorum still be reached?
                if(q_state->error_ct < (q_state->split_size - q_state->quorum_size)) {
                    return {}; //Wipe out the error
                }
                return error_reducer(err); // Delegate back through to the error reducer.
            };
            q_state->set_error_reducer(wrapped_error_reducer);
        }

    public:

            quorum_splitter(ClzActor *thiz, uint64_t split_size, std::size_t quorum_size, const T &init = T(),
                     const reducer_t &reducer = super::default_binary_reducer,
                     const error_reducer_t &error_reducer = super::default_error_reducer,
                     const std::chrono::milliseconds timeout_millis = std::chrono::milliseconds(s_default_timeout))
                     : super(thiz, make_quorum_state(thiz, init, split_size, quorum_size, reducer, error_reducer, timeout_millis))
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
    class merger : public splitter<ClzActor, C<T, A>, C<T, A>, splitter_state> {

    public:
            using super = splitter<ClzActor, C<T, A>, C<T, A>, splitter_state>;

            merger(ClzActor *thiz, std::size_t split_size)
            : super(thiz, split_size, C<T, A>(), insert_merge) {}
    };

} //ns as::common
