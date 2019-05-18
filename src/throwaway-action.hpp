#pragma once

#include "caf/all.hpp"

namespace caf {

    // A very simple wrapper that converts any action into the same action, returning void (i.e. ignoring any response)
    template<typename Action>
    struct throw_away_action_t : Action {
        using Action::Action;
        using result_t = void;
        using base_action_t = Action;
    };

    template<class Inspector, typename Action>
    typename Inspector::result_type inspect(Inspector &f, throw_away_action_t<Action> &x) {
        return f(caf::meta::type_name("throw_away_action_t<T>"), (Action &) x);
    }


} //ns caf
