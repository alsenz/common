#pragma once

#include <variant>

namespace std {

    //TODO extend match to take a pipe on a variant

    //TODO let tuples get expanded as well, will make significantly nicer. TODO TODO

    template<class... Ts>
    struct match : Ts ... {
        using Ts::operator()...;
    };

    template<class... Ts> match(Ts...) -> match<Ts...>;


    //TODO would be nice to define pipe for ranges view as visit...
    //TODO _impl | ranges::view::transform etc.

}