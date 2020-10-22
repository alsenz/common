#pragma once

#include <type_traits>

namespace caf {

    namespace detail {

        template<typename T, typename D = void> struct enabler { typedef D type; };

        template<class StructEvent, class Enable = void>
        struct handles_trait {
            //TODO remove comment
            //using type = reacts_to<StructEvent>;
            using type = result<void>(StructEvent);
        };

        template<class StructEvent>
        struct handles_trait<StructEvent, typename enabler<typename StructEvent::result_t>::type> {
            //TODO remove comment
            //using type = typename replies_to<StructEvent>::template with<typename StructEvent::result_t>;
            using type = result<typename StructEvent::result_t>(StructEvent);
        };

    } //ns detail

    // This helper extends reacts_to and replies_to. handles is an alias for handling a single struct type instance where the
    // handled result type is defined inside the struct using a typedef result_t, or if none is present, then a void (no) result is assumed.
    template<typename StructEvent>
    using handles = typename detail::handles_trait<StructEvent>::type;

} //ns caf