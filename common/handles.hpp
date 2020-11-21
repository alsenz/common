#pragma once

#include <type_traits>

namespace caf {

    // A distinct class to signify that this result shouldn't be factored away.
    template<typename... Ts>
    struct opaque_result : result<Ts...> {
        using result<Ts...>::result;
    };

    namespace detail {

        template<typename T, typename D = void> struct enabler { typedef D type; };

        template<class StructEvent, class Enable = void>
        struct handles_trait {
            using type = result<void>(StructEvent);
        };

        template<class StructEvent>
        struct handles_trait<StructEvent, typename enabler<typename StructEvent::result_t>::type> {
            using type = result<typename StructEvent::result_t>(StructEvent);
        };

        template<class StructEvent, class Enable = void>
        struct opaque_handles_trait {
            using type = opaque_result<void>(StructEvent);
        };

        template<class StructEvent>
        struct opaque_handles_trait<StructEvent, typename enabler<typename StructEvent::result_t>::type> {
            using type = opaque_result<typename StructEvent::result_t>(StructEvent);
        };

    } //ns detail

    // This helper extends reacts_to and replies_to. handles is an alias for handling a single struct type instance where the
    // handled result type is defined inside the struct using a typedef result_t, or if none is present, then a void (no) result is assumed.
    template<typename StructEvent>
    using handles = typename detail::handles_trait<StructEvent>::type;

    // A helper that signifies that the resultant virtual handler should return a result promise - i.e. the result<T> should not be
    // factored away in the response type.
    template <class... Is>
    struct replies_posthoc_to {
        template <class... Os>
        using with = opaque_result<Os...>(Is...);
    };

    // A handler identical to handles, except in typed actor bases the virtual method resopnse type will include the result<T> - i.e. the result<T>
    // will not be automatically factored away in the virtual base function signature.
    template<typename StructEvent>
    using handles_posthoc = typename detail::opaque_handles_trait<StructEvent>::type;


} //ns caf