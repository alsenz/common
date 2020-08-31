#pragma once

#include "common/virtual-handler.hpp"
#include "common/reducer-mixin.hpp"
#include "common/logger.hpp"

namespace caf {

    // The idea here is to provide an actor base class which automatically sets up virtual handle(...) member functions for behaviours given a type signature

    //TODO copy the shape out of event handler...

    namespace detail2 { //TODO change this to detail when event-handler.hpp is deprected.

        template<typename T>
        struct behaviors_to_handler;

        template<typename O, typename... Is>
        struct behaviors_to_handler<typed_mpi<detail::type_list<Is...>, output_tuple<O>>> {
            template<typename T>
            auto operator()(T *thiz) {
                return [thiz](Is... args) -> caf::result<O> {
                    //TODO: remove this log... at least put it in a switch
                    std::stringstream ss;
                    ss << "Handling event with result. This type: " << typeid(T).name() << ", result type: "
                       << typeid(O).name() << ", args tuple: " << typeid(std::tuple<Is...>).name();
                    as::common::logger::trace_a(thiz, ss.str(), "event-handler", __FUNCTION__, __LINE__, __FILE__);
                    return thiz->handle(std::forward<Is>(args)...);
                };
            }
        };

        template<typename... Is>
        struct behaviors_to_handler<typed_mpi<detail::type_list<Is...>, output_tuple<void>>> {
            template<typename T>
            auto operator()(T *thiz) {
                return [thiz](Is... args) -> void {
                    //TODO: remove this log... at least put it in a switch
                    std::stringstream ss;
                    ss << "Handling event with no result (void)! This type: " << typeid(T).name() << ", args tuple: "
                       << typeid(std::tuple<Is...>).name();
                    as::common::logger::trace_a(thiz, ss.str(), "event-handler", __FUNCTION__, __LINE__, __FILE__);
                    thiz->handle(std::forward<Is>(args)...);
                };
            }
        };

    }

    template<typename... Sigs>
    class typed_handler_actor
        : public mixin::reducer<typed_actor<Sigs...>::base, typed_handler_actor<Sigs...>>, //Add the reducer mixin for convenience
          public handler_base<Sigs...> {

        using super = typename typed_actor<Sigs...>::base;

        template<typename SingleClause>
        friend class detail2::behaviors_to_handler;

        typed_handler_actor(caf::actor_config &cfg) : super(cfg) {}

        //TODO: the whole rest of the shabang wiz.

    };

} //ns caf