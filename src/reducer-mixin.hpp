#pragma once

#include "caf/all.hpp"
#include "caf/policy/select_all.hpp"

#include "common/reducer-policy.hpp"

//Note this is based on:
//https://github.com/actor-framework/actor-framework/blob/master/libcaf_core/caf/mixin/requester.hpp
// and it would be good to reflect changes here as and when necessary...

namespace caf::mixin {

    // A reducer is a requester that can do requests with reductions
    template<class Base, class Subtype>
    class reducer : public Base {

        // The addition of MergePolicy allows customisations to the reducer - e.g. to implement a quorum reducer instead.
        template <template <class, class> class ReducerMergePolicy, //Takes a response type and a ReducerFn type
            typename ReducerFn,
            message_priority Prio = message_priority::normal, class Rep = int,
            class Period = std::ratio<1>, class Container, class... Ts>
        auto reducer_request(const Container& destinations, ReducerFn &&reducer_fn,
                             std::chrono::duration<Rep, Period> timeout, Ts&&... xs) {
            using handle_type = typename Container::value_type;
            using namespace detail;
            static_assert(sizeof...(Ts) > 0, "no message to send");
            using token = type_list<implicit_conversions_t<decay_t<Ts>>...>;
            static_assert(
                response_type_unbox<signatures_of_t<handle_type>, token>::valid,
                "receiver does not accept given message");
            auto dptr = static_cast<Subtype*>(this);
            std::vector<message_id> ids;
            ids.reserve(destinations.size());
            for (const auto& dest : destinations) {
                if (!dest)
                    continue;
                auto req_id = dptr->new_request_id(Prio);
                dest->eq_impl(req_id, dptr->ctrl(), dptr->context(),
                              std::forward<Ts>(xs)...);
                dptr->request_response_timeout(timeout, req_id);
                ids.emplace_back(req_id.response_id());
            }
            if (ids.empty()) {
                auto req_id = dptr->new_request_id(Prio);
                dptr->eq_impl(req_id.response_id(), dptr->ctrl(), dptr->context(),
                              make_error(sec::invalid_argument));
                ids.emplace_back(req_id.response_id());
            }
            using response_type
            = response_type_t<typename handle_type::signatures,
                detail::implicit_conversions_t<detail::decay_t<Ts>>...>;
            using result_type = response_handle<Subtype, ReducerMergePolicy<response_type, ReducerFn>>;
            return result_type{dptr, std::move(ids), std::move(reducer_fn)};
        }

        template <typename ReducerFn,
            message_priority Prio = message_priority::normal, class Rep = int,
            class Period = std::ratio<1>, class Container, class... Ts>
        auto reducer_request(const Container& destinations, ReducerFn &&reducer_fn,
                             std::chrono::duration<Rep, Period> timeout, Ts&&... xs) {
            //TODO change the reducer function default
            return reducer_request<caf::policy::reducer, ReducerFn, Prio, Rep, Period, Container, Ts...>(
                destinations, std::move(reducer_fn), timeout, std::forward<Ts>(xs)...);
        }

    };

} //ns caf