#pragma once

#include <variant>

namespace std {

    template<class... Ts>
    struct match : Ts ... {
        using Ts::operator()...;
    };

    template<class... Ts> match(Ts...) -> match<Ts...>;

}