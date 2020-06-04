#pragma once


#include <type_traits>


namespace traits {

    template<class Functor>
    struct is_reducer_function : std::false_type {
        static constexpr bool with_error_v = false;
    };

    template<typename ErrorOrVoid, class Res, class RedT>
    struct is_reducer_function<ErrorOrVoid(Res &, RedT)> : std::true_type {
        static constexpr bool with_error_v = std::is_void_v<ErrorOrVoid>;
    };

    // member noexcept const function pointer
    template <class C, typename R, class... Ts>
    struct is_reducer_function<R (C::*)(Ts...) const noexcept>
        : is_reducer_function<R(Ts...)> {};

    // member noexcept function pointer
    template <class C, typename R, class... Ts>
    struct is_reducer_function<R (C::*)(Ts...) noexcept> : is_reducer_function<R(Ts...)> {};

    // member const function pointer
    template <class C, typename R, class... Ts>
    struct is_reducer_function<R (C::*)(Ts...) const> : is_reducer_function<R (Ts...)> {};

    // member function pointer
    template <class C, typename R, class... Ts>
    struct is_reducer_function<R (C::*)(Ts...)> : is_reducer_function<R (Ts...)> {};

    // good ol' noexcept function pointer
    template <class R, class... Ts>
    struct is_reducer_function<R (*)(Ts...) noexcept> : is_reducer_function<R(Ts...)> {};

    // good ol' function pointer
    template <class R, class... Ts>
    struct is_reducer_function<R (*)(Ts...)> : is_reducer_function<R (Ts...)> {};

    template<typename T>
    inline constexpr bool is_reducer_function_v = is_reducer_function<T>::value;

    template<typename T>
    concept bool ReducerFn = is_reducer_function_v<T>;

}