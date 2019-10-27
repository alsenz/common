#pragma once

#include <type_traits>
#include <sstream>
#include <tuple>

#include "caf/all.hpp"

#include "common/logger.hpp"
#include "common/throwaway-event.hpp"
#include "common/virtual-handler.hpp"

// This is so close to caf functionality, we're just going to hijack the namespace a bit
namespace caf {


    namespace detail {

        template<typename Event>
        using event_sig = std::conditional_t<
            std::is_void_v<typename Event::result_t>,
            typename caf::reacts_to<Event>,
            typename caf::replies_to<Event>::template with<typename Event::result_t>>;

        template<typename... Events>
        using event_typed_actor = typed_actor<event_sig<Events>...>;

        template<typename T>
        struct behaviors_to_handler;

        template<typename O, typename... Is>
        struct behaviors_to_handler<typed_mpi < detail::type_list<Is...>, output_tuple <O>>> {
            template<typename T>
            auto operator()(T *thiz) {
                return [thiz](Is... args) -> caf::result<O> {
                    std::stringstream ss;
                    ss << "Handling event with result. This type: " << typeid(T).name() << ", result type: " << typeid(O).name() << ", args tuple: " << typeid(std::tuple<Is...>).name();
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
                    std::stringstream ss;
                    ss << "Handling event with no result (void)! This type: " << typeid(T).name() << ", args tuple: " << typeid(std::tuple<Is...>).name();
                    as::common::logger::trace_a(thiz, ss.str(), "event-handler", __FUNCTION__, __LINE__, __FILE__);
                    thiz->handle(std::forward<Is>(args)...);
                };
            }
        };

        template<typename T>
        struct behaviors_to_throw_away_handler;

        template<typename... Is>
        struct behaviors_to_throw_away_handler<typed_mpi<detail::type_list<Is...>, output_tuple<void>>> {
            template<typename T>
            auto operator()(T *thiz) {
                return [thiz](Is... args) -> void {
                    std::stringstream ss;
                    ss << "Handling throwaway event. This type: " << typeid(T).name() << ", args tuple: " << typeid(std::tuple<Is...>).name();
                    as::common::logger::trace_a(thiz, ss.str(), "event-handler", __FUNCTION__, __LINE__, __FILE__);
                    // Basically call the handle on the base event type for the throw away wrapper.
                    thiz->handle(((const typename Is::base_event_t &) args)...);
                };
            }
        };

    } // ns detail


    // An event handler is a statically typed actor that has a "handle" method for each Event. Event must be a struct with a typedef called result_t, with a result type
    // each handle method must exist for each event and have the signature caf::result<Event::result_t> handle(Event)!

    // The event handler pattern has the advantage that 1) there is no need to manually call make_behavior() (with the wrong spelling!) and
    // 2) the method signatures are a darn-sight clearer and more obvious.

    // As with most cases of actor design, no effort it made to support inheritance.

    // Finally, each event has a throw_away event variant, which is a special type wrapper that causes the event to be handled as before, but ignores the result.

    template<typename... Events>
    class event_handler : public detail::event_typed_actor<Events..., throw_away_event_t<Events>...>::base, virtual_handler_base<Events...> {

    public:

        using super = typename detail::event_typed_actor<Events..., throw_away_event_t<Events>...>::base;

        template<typename SingleClause>
        friend class detail::behaviors_to_handler;

        event_handler(caf::actor_config &cfg) : super(cfg) {}

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
                detail::behaviors_to_throw_away_handler<detail::event_sig<throw_away_event_t<Events>>>()(this)...,
                detail::behaviors_to_handler<detail::event_sig<Events>>()(this)...
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
