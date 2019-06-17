#pragma once

#include <type_traits>

#include "caf/all.hpp"

namespace as {

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