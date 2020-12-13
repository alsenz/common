#pragma once

#include <type_traits>

#include "caf/all.hpp"

#include "common/handles.hpp"

namespace caf {

    //TODO do some traits to ensure the forwarding is sensible here... e.g. scalars should be by value.

    namespace detail {

        // If everything is void it's also void, otherwise caf::result<Out...>
        template<typename... Out>
        using result_or_void_t = std::conditional_t<
            std::conjunction_v<std::is_void<Out>...>,
            void,
            caf::result<Out...>
        >;

        template<typename T, typename... Ts>
        struct first_pack_type {
            using type = T;
        };

        template<typename... Ts>
        using first_pack_type_t = typename first_pack_type<Ts...>::type;

    }

    // A simple base class that adds virtual const-ref handle(...) methods corresponding to each signature
    template<typename... Sigs>
    struct handler_base;

    template<class... Out, class... In>
    struct handler_base<result<Out...>(In...)> {

        virtual detail::first_pack_type_t<Out...> handle(const In &...) = 0; //TODO: handle forwarding correctly here

    };

    template<class... Out, class... In>
    struct handler_base<opaque_result<Out...>(In...)> {

        virtual detail::result_or_void_t<Out...> handle(const In &...) = 0; //TODO: handle forwarding correctly here

    };

    template<class... Sigs, class... Out, class... In>
    struct handler_base<result<Out...>(In...), Sigs...> : handler_base<Sigs...> {

        using handler_base<Sigs...>::handle;

        virtual detail::first_pack_type_t<Out...> handle(const In &...) = 0; //TODO handle forwarding correctly here

    };

    template<class... Sigs, class... Out, class... In>
    struct handler_base<opaque_result<Out...>(In...), Sigs...> : handler_base<Sigs...> {

        using handler_base<Sigs...>::handle;

        virtual detail::result_or_void_t<Out...> handle(const In &...) = 0; //TODO handle forwarding correctly here

    };



}