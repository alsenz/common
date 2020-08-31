#pragma once

#include <type_traits>

//TODO i think this just needs to be "handles"....

namespace caf {

    namespace detail {

        template<typename T, typename D = void> struct enabler { typedef D type; };

        template<class StructEvent, class Enable = void>
        struct handles_trait {
            using type = reacts_to<StructEvent>;
        };

        template<class StructEvent>
        struct handles_trait<StructEvent, typename enabler<typename StructEvent::result_t>::type> {
            using type = result<typename StructEvent::result_t>(StructEvent);
        };

    } //ns detail

    // This helper extends reacts_to and replies_to. handles is an alias for handling a single struct type instance where the
    // handled result type is defined inside the struct using a typedef result_t, or if none is present, then a void (no) result is assumed.
    template<typename StructEvent>
    using handles = typename detail::handles_trait<StructEvent>::type;

} //ns caf




//TODO: new design should come into play here...
//TODO: we have a new class: caf::typed_handler_actor<Signatures...>
//TODO: we create a new caf::adheres_to<Event> which is an alias for caf::replies_to with result_t (or nothing if no result_t...)
//TODO: simples the whole thing is a bit simpler now...