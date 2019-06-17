#pragma once

#include "caf/all.hpp"

namespace caf {

    // A very simple wrapper that converts any event into the same event, returning void (i.e. ignoring any response)
    template<typename Event>
    struct throw_away_event_t : Event {
        using Event::Event;
        using result_t = void;
        using base_event_t = Event;
    };

    template<class Inspector, typename Event>
    typename Inspector::result_type inspect(Inspector &f, throw_away_event_t<Event> &x) {
        return f(caf::meta::type_name("throw_away_event_t<T>"), (Event &) x);
    }


} //ns caf
