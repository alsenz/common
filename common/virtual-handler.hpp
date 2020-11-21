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



//TODO TODO
//TODO stuff in here is deprecated
//TODO remove this when event-handler is deprecated.
namespace as {




    //TODO deprecate this... this needs to become virtual_handler_base_new or something?

    // A simple base structure that adds virtual const-ref handlers for each event. Simples.
    template<typename... Events>
    struct virtual_handler_base;

    template<>
    struct virtual_handler_base<> {
        // Nothing!

        // ... but convenient little hack is necessary to simplify "using" semantics!
        void handle() {}
    };

    template<typename HeadEvent>
    struct virtual_handler_base<HeadEvent> {

        virtual std::conditional_t<
            std::is_void_v<typename HeadEvent::result_t>,
            void,
            caf::result<typename HeadEvent::result_t>>
        handle(const HeadEvent &arg1) = 0;

    };

    template<typename HeadEvent, typename NextEvent, typename... TailEvents>
    struct virtual_handler_base<HeadEvent, NextEvent, TailEvents...> : virtual_handler_base<NextEvent, TailEvents...> {

        using virtual_handler_base<NextEvent, TailEvents...>::handle;

        virtual std::conditional_t<
            std::is_void_v<typename HeadEvent::result_t>,
            void,
            caf::result<typename HeadEvent::result_t>>
        handle(const HeadEvent &arg1) = 0;

    };

} //ns as