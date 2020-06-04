#pragma once

#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <vector>
#include <type_traits>

#include "caf/all.hpp"

#include "common/reducer-function.hpp"

namespace caf::detail {

    //TODO: work through error messages...
    //TODO take this off... off we go again... fix this; quorum policy, then we need a quorum actor pool....
    //TODO then back to djq, back to raft, from there we go...

    template <class Cb, class ErrorCb, class ResponseType, class ReducedType, traits::ReducerFn RF>
    struct reducer_helper {


        ResponseType result;
        std::shared_ptr<size_t> pending;

        RF reducer_fn;
        Cb cb; //Callback function
        ErrorCb eCb; // Error callback - optional

        void operator()(ReducedType &x) {
            CAF_LOG_TRACE(CAF_ARG2("pending", *pending));
            if (*pending > 0) {
                if constexpr(traits::is_reducer_function<RF>::with_error_v) {  //Must be a caf error - so we should handle these in the reduction.
                    const auto err = reducer_fn(result, std::move(x));
                    if(err) {
                        *pending = 0; // So this will now never fire the
                        if(eCb) {
                            eCb(err);
                        }
                    }
                } else {
                    reducer_fn(result, std::move(x)); //Slightly less branchey version for reducers that don't possible return errors.
                }
                if (--*pending == 0) {
                    cb(std::move(result));
                }
            }
        }

        // Note that error callback is initialised to nulltpr
        template <class RFun, class Fun>
        reducer_helper(size_t pending, RFun &&rf, Fun&& f)
            : pending(std::make_shared<size_t>(pending)), reducer_fn(std::forward<RFun>(rf)), cb(std::forward<Fun>(f)), eCb(nullptr), result() {} //TODO check eCb can always be nullptr by this design.

        auto wrap() {
            return [this](ReducedType &x) { (*this)(x); };
        }
    };

} // namespace caf::detail


namespace caf::policy {

    //TODO: these docs
    /// Enables a `response_handle` to fan-in all responses messages into a single
    /// result which is reduced via a function (with optional error handling)
    /// @relates mixin::requester
    /// @relates response_handle
    template <class ReducedType, traits::ReducerFn RF>
    class reducer {
    public:
        static constexpr bool is_trivial = false;

        using reduced_type = ReducedType;
        using reducer_fn_type = RF;
        // By convention this is the first argument of the function (and it is a ref)
        using response_type = caf::detail::tl_at_t<typename caf::detail::callable_trait<RF>::decayed_arg_types, 0>;
        using message_id_list = std::vector<message_id>;

        //TODO replace this- find out where it's used...
        //TODO check where we return the right type here... needs to be checked
        /*
        template <class Fun>
        using type_checker
        = detail::type_checker<response_type,
            detail::reducer_helper_t<detail::decay_t<Fun>>>;*/

        explicit reducer(message_id_list ids, reducer_fn_type &&reducer_fn) : _ids(std::move(ids)), _reducer_fn(std::move(reducer_fn)) {
            CAF_ASSERT(_ids.size()
                       <= static_cast<size_t>(std::numeric_limits<int>::max()));
        }

        reducer(reducer&&) noexcept = default;

        reducer& operator=(reducer&&) noexcept = default;

        template <class Self, class F, class OnError>
        void await(Self* self, F&& f, OnError&& g) const {
            CAF_LOG_TRACE(CAF_ARG(_ids));
            auto bhvr = make_behavior(std::forward<F>(f), std::forward<OnError>(g));
            for (auto id : _ids)
                self->add_awaited_response_handler(id, bhvr);
        }

        template <class Self, class F, class OnError>
        void then(Self* self, F&& f, OnError&& g) const {
            CAF_LOG_TRACE(CAF_ARG(_ids));
            auto bhvr = make_behavior(std::forward<F>(f), std::forward<OnError>(g));
            for (auto id : _ids)
                self->add_multiplexed_response_handler(id, bhvr);
        }

        template <class Self, class F, class G>
        void receive(Self* self, F&& f, G&& g) const {
            /*CAF_LOG_TRACE(CAF_ARG(_ids));
            //TODO add this back...
            using helper_type = detail::reducer_helper_t<detail::decay_t<F>>;
            helper_type helper{_ids.size(), std::forward<F>(f)};
            auto error_handler = [&](error& err) {
                if (*helper.pending > 0) {
                    *helper.pending = 0;
                    helper.results.clear();
                    g(err);
                }
            };
            for (auto id : _ids) {
                typename Self::accept_one_cond rc;
                auto error_handler_copy = error_handler;
                self->varargs_receive(rc, id, helper.wrap(), error_handler_copy);
            }*/
        }

        const message_id_list& ids() const noexcept {
            return _ids;
        }

    private:

        template <class Cb, class ErrorCb>
        behavior make_behavior(Cb&& f, ErrorCb&& g) const {
            using namespace detail;
            reducer_helper<Cb, ErrorCb, response_type, reduced_type, reducer_fn_type> helper{_ids.size(), std::move(_reducer_fn), std::move(f)};
            auto pending = helper.pending;
            // Copy the error handler in since we'll need it.
            auto error_handler = [pending{std::move(pending)}, g]
                (error& err) mutable {
                    CAF_LOG_TRACE(CAF_ARG2("pending", *pending));
                    if (*pending > 0) {
                        *pending = 0;
                        g(err);
                    }
            };
            // For this callback, we will manage the pending shared_ptr ourselves, so no need to wrap.
            helper.eCb = g;
            return {
                std::move(helper),
                std::move(error_handler),
            };
        }

        message_id_list _ids;
        reducer_fn_type _reducer_fn; // This is moved-out as soon as make behaviour or equivalent is called.
    };

} // namespace caf::policy

