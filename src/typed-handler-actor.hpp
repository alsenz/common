#pragma once

#include <type_traits>
#include <sstream>
#include <tuple>

#include "caf/all.hpp"

#include "common/virtual-handler.hpp"
#include "common/reducer-mixin.hpp"
#include "common/logger.hpp"

namespace caf {

    // The idea here is to provide an actor base class which automatically sets up virtual handle(...) member functions for behaviours given a type signature

    //TODO add an example here...

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

        //TODO fix Sigs behaviors_to_handler because that's not gonna work right now that's evident actually
        //TODO TODO because my typed_mpi business isn't going to convert nicely to an output tuple etc...

        // This basically just eliminates the boiler plate for make_behavior, and the spelling.
        typename super::behavior_type make_behavior() override {

            super::set_default_handler([this](caf::scheduled_actor *self_ptr, const caf::message_view &msg) mutable -> result<message> {
                std::stringstream ss;
                ss << "Unexpected message [receiver id: " << self_ptr->id();
                ss << ", name: " << self_ptr->name();
                ss << "]: " << msg.content().stringify();
                LOG_ERROR_A("event-handler", ss.str());
                on_unexpected_message(msg);
                return sec::unexpected_message;
            });

            super::set_down_handler([this](caf::scheduled_actor *self_ptr, const caf::down_msg &msg) mutable {
                std::stringstream ss;
                ss << "Handler going down [id: " << self_ptr->id();
                ss << ", name: " << self_ptr->name();
                ss << "]: " << to_string(msg);
                LOG_WARN_A("event-handler", ss.str());
                on_down_msg(msg);
            });

            super::set_exit_handler([this](caf::scheduled_actor *self_ptr, const caf::exit_msg &msg) mutable {
                std::stringstream ss;
                ss << "Received exit message [id: " << self_ptr->id();
                ss << ", name: " << self_ptr->name();
                if(msg.reason) {
                    ss << ", reason: " << this->system().render(msg.reason);
                }
                ss << "]: " << to_string(msg);
                LOG_WARN_A("event-handler", ss.str());
                on_exit(msg);
            });

            return {
                //TODO this isn't going to work
                detail2::behaviors_to_handler<Sigs...>()(this)...
            };
        }


    protected:

        virtual void on_unexpected_message(const caf::message_view &msg) {}

        virtual void on_down_msg(const caf::down_msg &msg) {}

        virtual void on_exit(const caf::exit_msg &msg) {
            super::quit();
        }

    };

} //ns caf