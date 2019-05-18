#pragma once

#include <type_traits>

#include "caf/all.hpp"

namespace as {

    // A simple base structure that adds virtual const-ref handlers for each action. Simples.
    template<typename... Actions>
    struct virtual_handler_base;

    template<>
    struct virtual_handler_base<> {
        // Nothing!

        // ... but convenient little hack is necessary to simplify "using" semantics!
        void handle() {}
    };

    template<typename HeadAction>
    struct virtual_handler_base<HeadAction> {

        virtual std::conditional_t<
            std::is_void_v<typename HeadAction::result_t>,
            void,
            caf::result<typename HeadAction::result_t>>
        handle(const HeadAction &arg1) = 0;

    };

    template<typename HeadAction, typename NextAction, typename... TailActions>
    struct virtual_handler_base<HeadAction, NextAction, TailActions...> : virtual_handler_base<NextAction, TailActions...> {

        using virtual_handler_base<NextAction, TailActions...>::handle;

        virtual std::conditional_t<
            std::is_void_v<typename HeadAction::result_t>,
            void,
            caf::result<typename HeadAction::result_t>>
        handle(const HeadAction &arg1) = 0;

    };

} //ns as